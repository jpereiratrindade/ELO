#include "admin_http_server.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSaveFile>
#include <QThread>
#include <QUrl>
#include <QUrlQuery>
#include <iostream>

namespace elo::admin {

namespace {

QString sanitize_filename(const QString& name) {
    QFileInfo fi(name);
    QString base = fi.fileName();
    base.replace(QStringLiteral(".."), QStringLiteral(""));
    base.replace(QStringLiteral("/"), QStringLiteral(""));
    base.replace(QStringLiteral("\\"), QStringLiteral(""));
    return base;
}

QString slugify(const QString& text) {
    QString res = text.toLower().trimmed();
    res.replace(' ', '_');
    res.replace('-', '_');
    QString clean;
    for (QChar c : res) {
        if (c.isLetterOrNumber() || c == '_') {
            clean.append(c);
        }
    }
    return clean.isEmpty() ? QStringLiteral("item_%1").arg(QDateTime::currentMSecsSinceEpoch()) : clean;
}

QString canonical_content_id(QString id) {
    for (int i = 0; i < 3; ++i) {
        const QString decoded = QUrl::fromPercentEncoding(id.toUtf8());
        if (decoded == id) break;
        id = decoded;
    }
    return id.trimmed();
}

} // namespace

AdminHttpServer::AdminHttpServer(
    std::filesystem::path content_root,
    std::filesystem::path web_root,
    quint16 port,
    QObject* parent)
    : QObject(parent),
      content_root_(std::move(content_root)),
      web_root_(std::move(web_root)),
      port_(port),
      tcp_server_(std::make_unique<QTcpServer>(this)),
      publisher_(content_root_) {
    connect(tcp_server_.get(), &QTcpServer::newConnection, this, &AdminHttpServer::onNewConnection);

    // Ensure draft catalog has published bundle atoms and assets if missing
    std::error_code ec;
    auto cur_atoms = content_root_ / "current" / "catalog" / "atoms";
    auto draft_atoms = content_root_ / "catalog" / "atoms";
    if (std::filesystem::exists(cur_atoms, ec)) {
        std::filesystem::create_directories(draft_atoms, ec);
        for (const auto& entry : std::filesystem::directory_iterator(cur_atoms, ec)) {
            if (entry.is_regular_file(ec)) {
                auto target = draft_atoms / entry.path().filename();
                if (!std::filesystem::exists(target, ec)) {
                    std::filesystem::copy_file(entry.path(), target, ec);
                }
            }
        }
    }
    auto cur_assets = content_root_ / "current" / "assets";
    auto draft_assets = content_root_ / "assets";
    if (std::filesystem::exists(cur_assets, ec)) {
        std::filesystem::create_directories(draft_assets, ec);
        std::filesystem::copy(cur_assets, draft_assets,
                              std::filesystem::copy_options::recursive | std::filesystem::copy_options::skip_existing, ec);
    }

    // SQLite/WAL is the mutable editorial source of truth. Existing JSON catalogs
    // are imported exactly once; subsequent JSON files are publication snapshots.
    content_database_ = std::make_unique<content::ContentDatabase>(content_root_ / "editorial.sqlite3");
    content_database_->import_legacy_workspace(content_root_);

    // Initialize Sovereign Application Profiles from persistent storage or default seeds
    std::string appsPath = (content_root_ / "applications.json").string();
    if (!app_registry_.load_from_file(appsPath)) {
        app_registry_.register_application(application::ApplicationProfile{
            .app_id = "app.elo.bioma-pampa",
            .name = "Biodiversidade do Bioma Pampa & Campos Sulinos",
            .domain_category = "environmental_sciences",
            .version = "1.0.0",
            .description = "Corpus ecológico sobre avifauna, flora campestre e processos ecológicos.",
            .active_bundle_id = "elo-content-pampa",
            .target_audience = "publico_geral",
            .default_theme = "pampa",
            .tags = {"biodiversidade", "conservacao", "pampa", "ecologia"},
            .metadata_schema = {{"context", "totem_interativo_ambiental"}, {"audience", "publico_geral"}},
            .is_active = true
        });

        app_registry_.register_application(application::ApplicationProfile{
            .app_id = "app.elo.patrimonio-historico",
            .name = "Patrimônio Histórico e Memória Regional",
            .domain_category = "cultural_heritage",
            .version = "0.8.0",
            .description = "Acervo histórico de fotografias antigas, relatos orais e monumentos arquitetônicos.",
            .active_bundle_id = "elo-content-patrimonio",
            .target_audience = "comunidade_local",
            .default_theme = "sepia",
            .tags = {"historia", "memoria", "arquitetura", "patrimonio"},
            .metadata_schema = {{"context", "museu_historico"}, {"curator", "instituto_memoria"}},
            .is_active = false
        });

        app_registry_.register_application(application::ApplicationProfile{
            .app_id = "app.elo.galeria-visual",
            .name = "Galeria de Arte & Expressões Visuais",
            .domain_category = "fine_arts",
            .version = "0.9.5",
            .description = "Exposição imersiva de artes visuais contemporâneas e paisagens sensoriais.",
            .active_bundle_id = "elo-content-artes",
            .target_audience = "estudantes_artistas",
            .default_theme = "minimal_dark",
            .tags = {"artes_visuais", "fotografia", "estetica", "imersao"},
            .metadata_schema = {{"context", "espaco_cultural"}, {"layout", "contemplativo"}},
            .is_active = false
        });
        app_registry_.save_to_file(appsPath);
    }

}

AdminHttpServer::~AdminHttpServer() {
    stop();
}

bool AdminHttpServer::start() {
    stop();
    if (!tcp_server_->listen(QHostAddress::Any, port_)) {
        std::cerr << "[ELO][admin] Failed to bind HTTP server to port " << port_ << ": "
                  << tcp_server_->errorString().toStdString() << '\n';
        return false;
    }
    std::cout << "[ELO][admin] Sovereign Content Studio HTTP server running at http://0.0.0.0:" << port_ << '\n';
    return true;
}

void AdminHttpServer::stop() {
    if (tcp_server_ && tcp_server_->isListening()) {
        tcp_server_->close();
    }
}

void AdminHttpServer::onNewConnection() {
    while (tcp_server_->hasPendingConnections()) {
        auto* socket = tcp_server_->nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, &AdminHttpServer::onClientReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
            client_buffers_.erase(socket);
            socket->deleteLater();
        });
    }
}

void AdminHttpServer::onClientReadyRead() {
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    auto& buf = client_buffers_[socket];
    buf.append(socket->readAll());

    int headerEnd = buf.indexOf("\r\n\r\n");
    if (headerEnd == -1) {
        return; // Wait for full HTTP headers
    }

    int contentLength = 0;
    QByteArray headerBytes = buf.left(headerEnd);
    QByteArray lowerHeader = headerBytes.toLower();
    int clIdx = lowerHeader.indexOf("content-length:");
    if (clIdx != -1) {
        int clEnd = headerBytes.indexOf("\r\n", clIdx);
        if (clEnd == -1) clEnd = headerBytes.size();
        QByteArray clVal = headerBytes.mid(clIdx + 15, clEnd - (clIdx + 15)).trimmed();
        contentLength = clVal.toInt();
    }

    int totalExpected = headerEnd + 4 + contentLength;
    if (buf.size() < totalExpected) {
        return; // Wait for the remaining body chunks
    }

    QByteArray requestData = buf.left(totalExpected);
    buf.remove(0, totalExpected);
    handleHttpRequest(socket, requestData);
}

void AdminHttpServer::sendResponse(QTcpSocket* socket, int statusCode, const QString& contentType, const QByteArray& body) {
    QString statusText = (statusCode == 200) ? QStringLiteral("OK")
                       : (statusCode == 201) ? QStringLiteral("Created")
                       : (statusCode == 409) ? QStringLiteral("Conflict")
                       : (statusCode == 404) ? QStringLiteral("Not Found")
                       : (statusCode == 400) ? QStringLiteral("Bad Request")
                                             : QStringLiteral("Internal Server Error");

    QByteArray response;
    response.append(QString("HTTP/1.1 %1 %2\r\n").arg(statusCode).arg(statusText).toUtf8());
    response.append(QString("Content-Type: %1\r\n").arg(contentType).toUtf8());
    response.append(QString("Content-Length: %1\r\n").arg(body.size()).toUtf8());
    response.append("Access-Control-Allow-Origin: *\r\n");
    response.append("Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n");
    response.append("Access-Control-Allow-Headers: Content-Type\r\n");
    response.append("Connection: close\r\n\r\n");
    response.append(body);

    socket->write(response);
    socket->flush();
    socket->disconnectFromHost();
}

void AdminHttpServer::sendJsonResponse(QTcpSocket* socket, int statusCode, const QByteArray& jsonBytes) {
    sendResponse(socket, statusCode, QStringLiteral("application/json; charset=utf-8"), jsonBytes);
}

void AdminHttpServer::handleAtomsRoute(QTcpSocket* socket, const QString& method, const QString& path, const QByteArray& body) {
    // GET /api/atoms/:id
    if (method == QStringLiteral("GET") && path.startsWith(QStringLiteral("/api/atoms/"))) {
        QString id = canonical_content_id(path.mid(11));
        const auto value = content_database_->atom(id);
        if (!value.isEmpty()) {
            sendJsonResponse(socket, 200, QJsonDocument(value).toJson(QJsonDocument::Compact));
            return;
        }
        sendJsonResponse(socket, 404, "{\"error\":\"Átomo não encontrado\"}");
        return;
    }

    // POST /api/atoms (Create)
    if (method == QStringLiteral("POST") && path == QStringLiteral("/api/atoms")) {
        auto doc = QJsonDocument::fromJson(body);
        if (!doc.isObject()) {
            sendJsonResponse(socket, 400, "{\"error\":\"Payload JSON inválido\"}");
            return;
        }

        auto obj = doc.object();
        QString contentId = obj.value(QStringLiteral("content_id")).toString().trimmed();
        if (contentId.isEmpty()) {
            QString title = obj.value(QStringLiteral("title")).toString();
            contentId = QStringLiteral("atom_") + slugify(title);
            obj[QStringLiteral("content_id")] = contentId;
        }
        contentId = canonical_content_id(contentId);
        obj[QStringLiteral("content_id")] = contentId;

        if (!obj.contains(QStringLiteral("schema_version"))) {
            obj[QStringLiteral("schema_version")] = QStringLiteral("0.1");
        }

        if (!content_database_->atom(contentId).isEmpty()) {
            sendJsonResponse(socket, 409, "{\"error\":\"Já existe um átomo com este content_id\"}");
            return;
        }

        content_database_->upsert_atom(obj);
        draft_modified_ = true;

        QJsonObject resp;
        resp[QStringLiteral("success")] = true;
        resp[QStringLiteral("content_id")] = contentId;
        resp[QStringLiteral("atom")] = obj;
        sendJsonResponse(socket, 201, QJsonDocument(resp).toJson(QJsonDocument::Compact));
        return;
    }

    // PUT /api/atoms/:id (Update)
    if (method == QStringLiteral("PUT") && path.startsWith(QStringLiteral("/api/atoms/"))) {
        QString id = canonical_content_id(path.mid(11));
        auto doc = QJsonDocument::fromJson(body);
        if (!doc.isObject()) {
            sendJsonResponse(socket, 400, "{\"error\":\"Payload JSON inválido\"}");
            return;
        }

        auto obj = doc.object();
        obj[QStringLiteral("content_id")] = id;

        if (content_database_->atom(id).isEmpty()) {
            sendJsonResponse(socket, 404, "{\"error\":\"Átomo não encontrado para atualização\"}");
            return;
        }

        content_database_->upsert_atom(obj);
        draft_modified_ = true;

        QJsonObject resp;
        resp[QStringLiteral("success")] = true;
        resp[QStringLiteral("content_id")] = id;
        sendJsonResponse(socket, 200, QJsonDocument(resp).toJson(QJsonDocument::Compact));
        return;
    }

    // DELETE /api/atoms/:id (Delete)
    if (method == QStringLiteral("DELETE") && path.startsWith(QStringLiteral("/api/atoms/"))) {
        QString id = canonical_content_id(path.mid(11));
        bool removed = content_database_->delete_atom(id);

        if (removed) {
            draft_modified_ = true;
            QJsonObject resp;
            resp[QStringLiteral("success")] = true;
            resp[QStringLiteral("deleted_id")] = id;
            sendJsonResponse(socket, 200, QJsonDocument(resp).toJson(QJsonDocument::Compact));
        } else {
            sendJsonResponse(socket, 404, "{\"error\":\"Átomo não encontrado para exclusão\"}");
        }
        return;
    }

    sendJsonResponse(socket, 400, "{\"error\":\"Ação não suportada para átomos\"}");
}

void AdminHttpServer::handleUploadRoute(QTcpSocket* socket, const QByteArray& body) {
    auto doc = QJsonDocument::fromJson(body);
    if (!doc.isObject()) {
        sendJsonResponse(socket, 400, "{\"error\":\"Payload JSON inválido para upload\"}");
        return;
    }

    auto obj = doc.object();
    QString folder = obj.value(QStringLiteral("folder")).toString(QStringLiteral("images"));
    if (folder != QStringLiteral("audio")) folder = QStringLiteral("images");

    QString filename = sanitize_filename(obj.value(QStringLiteral("filename")).toString());
    if (filename.isEmpty()) {
        sendJsonResponse(socket, 400, "{\"error\":\"Nome de arquivo ausente\"}");
        return;
    }

    QString b64 = obj.value(QStringLiteral("base64_data")).toString();
    int comma = b64.indexOf(QStringLiteral(","));
    if (comma != -1) {
        b64 = b64.mid(comma + 1);
    }

    QByteArray rawData = QByteArray::fromBase64(b64.toUtf8());
    if (rawData.isEmpty()) {
        sendJsonResponse(socket, 400, "{\"error\":\"Dados de mídia em base64 vazios ou corrompidos\"}");
        return;
    }

    QString destDir = QString::fromStdString((content_root_ / "assets" / folder.toStdString()).string());
    QDir().mkpath(destDir);

    QString destPath = QDir(destDir).filePath(filename);
    QFile f(destPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        sendJsonResponse(socket, 500, "{\"error\":\"Falha ao gravar arquivo em assets\"}");
        return;
    }

    f.write(rawData);
    draft_modified_ = true;

    QJsonObject resp;
    resp[QStringLiteral("success")] = true;
    resp[QStringLiteral("path")] = QStringLiteral("assets/") + folder + QStringLiteral("/") + filename;
    resp[QStringLiteral("size_bytes")] = rawData.size();
    sendJsonResponse(socket, 200, QJsonDocument(resp).toJson(QJsonDocument::Compact));
}

void AdminHttpServer::handleMediaRoute(QTcpSocket* socket, const QString& method, const QString& path, const QByteArray& body) {
    // GET /api/media — Lista todos os arquivos de mídia e seus vínculos
    if (method == QStringLiteral("GET")) {
        QJsonArray mediaList;

        // Mapear átomos para identificar quais usam cada mídia
        QDir atomsDir(QString::fromStdString((content_root_ / "catalog" / "atoms").string()));
        QMap<QString, QStringList> mediaUsage;
        for (const auto& fileInfo : atomsDir.entryInfoList(QStringList() << QStringLiteral("*.json"), QDir::Files)) {
            QFile af(fileInfo.absoluteFilePath());
            if (af.open(QIODevice::ReadOnly)) {
                auto doc = QJsonDocument::fromJson(af.readAll());
                if (doc.isObject()) {
                    auto obj = doc.object();
                    QString atomId = obj.value(QStringLiteral("content_id")).toString();
                    QString atomTitle = obj.value(QStringLiteral("title")).toString();
                    if (atomTitle.isEmpty()) atomTitle = atomId;

                    auto modalities = obj.value(QStringLiteral("modalities")).toObject();
                    auto imgArr = modalities.value(QStringLiteral("image")).toArray();
                    for (const auto& imgVal : imgArr) {
                        QString p = imgVal.toString();
                        QFileInfo imgFi(p);
                        mediaUsage[imgFi.fileName()].append(atomTitle);
                        mediaUsage[p].append(atomTitle);
                    }
                    auto audArr = modalities.value(QStringLiteral("audio")).toArray();
                    for (const auto& audVal : audArr) {
                        QString p = audVal.toString();
                        QFileInfo audFi(p);
                        mediaUsage[audFi.fileName()].append(atomTitle);
                        mediaUsage[p].append(atomTitle);
                    }
                }
            }
        }

        // Varrer subpastas de assets: images e audio
        QStringList folders = { QStringLiteral("images"), QStringLiteral("audio") };
        for (const auto& folder : folders) {
            QDir dir(QString::fromStdString((content_root_ / "assets" / folder.toStdString()).string()));
            if (!dir.exists()) continue;

            for (const auto& fi : dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Time)) {
                QJsonObject item;
                QString relPath = QStringLiteral("assets/") + folder + QStringLiteral("/") + fi.fileName();
                item[QStringLiteral("filename")] = fi.fileName();
                item[QStringLiteral("path")] = relPath;
                item[QStringLiteral("folder")] = folder;
                item[QStringLiteral("type")] = (folder == QStringLiteral("images")) ? QStringLiteral("image") : QStringLiteral("audio");
                item[QStringLiteral("size_bytes")] = fi.size();
                item[QStringLiteral("modified")] = fi.lastModified().toString(Qt::ISODate);

                QJsonArray usedArr;
                QStringList users = mediaUsage.value(fi.fileName());
                users.append(mediaUsage.value(relPath));
                users.removeDuplicates();
                for (const auto& u : users) {
                    usedArr.append(u);
                }
                item[QStringLiteral("used_by")] = usedArr;

                mediaList.append(item);
            }
        }

        QJsonObject resp;
        resp[QStringLiteral("media")] = mediaList;
        sendJsonResponse(socket, 200, QJsonDocument(resp).toJson(QJsonDocument::Compact));
        return;
    }

    // DELETE /api/media (ou DELETE /api/upload)
    if (method == QStringLiteral("DELETE")) {
        QString requestedPath;

        // 1. Verificar query parameter: /api/media?path=assets/images/foto.png
        int qIdx = path.indexOf('?');
        if (qIdx != -1) {
            QString query = path.mid(qIdx + 1);
            auto pairs = query.split('&');
            for (const auto& p : pairs) {
                auto kv = p.split('=');
                if (kv.size() == 2 && kv[0] == QStringLiteral("path")) {
                    requestedPath = QUrl::fromPercentEncoding(kv[1].toUtf8());
                    break;
                }
            }
        }

        // 2. Se não estiver no query, verificar corpo JSON
        if (requestedPath.isEmpty() && !body.isEmpty()) {
            auto doc = QJsonDocument::fromJson(body);
            if (doc.isObject()) {
                requestedPath = doc.object().value(QStringLiteral("path")).toString();
                if (requestedPath.isEmpty()) {
                    QString fn = doc.object().value(QStringLiteral("filename")).toString();
                    QString fol = doc.object().value(QStringLiteral("folder")).toString(QStringLiteral("images"));
                    if (!fn.isEmpty()) {
                        requestedPath = QStringLiteral("assets/") + fol + QStringLiteral("/") + fn;
                    }
                }
            }
        }

        if (requestedPath.isEmpty()) {
            sendJsonResponse(socket, 400, "{\"error\":\"Parâmetro 'path' de mídia ausente para exclusão\"}");
            return;
        }

        // Normalização e verificação de segurança (Prevenção rigorosa de path traversal)
        requestedPath.replace('\\', '/');
        while (requestedPath.startsWith('/')) requestedPath.remove(0, 1);
        if (requestedPath.startsWith(QStringLiteral("assets/"))) {
            requestedPath.remove(0, 7);
        }

        if (requestedPath.contains(QStringLiteral("..")) || requestedPath.contains(QStringLiteral(":"))) {
            sendJsonResponse(socket, 400, "{\"error\":\"Caminho de arquivo inválido ou inseguro\"}");
            return;
        }

        std::filesystem::path assetsRoot = content_root_ / "assets";
        std::filesystem::path targetFile = (assetsRoot / requestedPath.toStdString()).lexically_normal();

        // Assegurar que o arquivo resolvido está estritamente dentro da raiz de assets
        std::string assetsNorm = assetsRoot.lexically_normal().string();
        std::string targetNorm = targetFile.string();
        if (targetNorm.find(assetsNorm) != 0) {
            sendJsonResponse(socket, 403, "{\"error\":\"Acesso negado fora do repositório soberano de assets\"}");
            return;
        }

        bool fileRemoved = false;
        if (std::filesystem::exists(targetFile) && std::filesystem::is_regular_file(targetFile)) {
            std::error_code ec;
            fileRemoved = std::filesystem::remove(targetFile, ec);
            if (ec) {
                std::cerr << "[ELO][admin] Erro ao excluir arquivo de mídia: " << ec.message() << std::endl;
            }
        }

        // Limpeza dos átomos que eventualmente referenciavam esta mídia
        QString filenameOnly = QFileInfo(QString::fromStdString(targetFile.string())).fileName();
        QString relAssetPath = QStringLiteral("assets/") + requestedPath;
        int cleanedAtoms = 0;

        QDir atomsDir(QString::fromStdString((content_root_ / "catalog" / "atoms").string()));
        for (const auto& fileInfo : atomsDir.entryInfoList(QStringList() << QStringLiteral("*.json"), QDir::Files)) {
            QFile af(fileInfo.absoluteFilePath());
            if (af.open(QIODevice::ReadOnly)) {
                auto doc = QJsonDocument::fromJson(af.readAll());
                af.close();

                if (doc.isObject()) {
                    auto obj = doc.object();
                    bool atomModified = false;
                    auto modalities = obj.value(QStringLiteral("modalities")).toObject();

                    // Limpar imagem
                    auto imgArr = modalities.value(QStringLiteral("image")).toArray();
                    QJsonArray newImgArr;
                    for (const auto& v : imgArr) {
                        QString p = v.toString();
                        if (p == relAssetPath || p == requestedPath || p.endsWith(filenameOnly)) {
                            atomModified = true;
                        } else {
                            newImgArr.append(v);
                        }
                    }
                    if (atomModified) {
                        modalities[QStringLiteral("image")] = newImgArr;
                    }

                    // Limpar áudio
                    auto audArr = modalities.value(QStringLiteral("audio")).toArray();
                    QJsonArray newAudArr;
                    for (const auto& v : audArr) {
                        QString p = v.toString();
                        if (p == relAssetPath || p == requestedPath || p.endsWith(filenameOnly)) {
                            atomModified = true;
                        } else {
                            newAudArr.append(v);
                        }
                    }
                    if (atomModified) {
                        modalities[QStringLiteral("audio")] = newAudArr;
                        obj[QStringLiteral("modalities")] = modalities;
                    }

                    if (atomModified) {
                        if (af.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                            af.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
                            af.close();
                            cleanedAtoms++;
                        }
                    }
                }
            }
        }

        // Limpeza de referências em variants.json
        QString variantsPath = QString::fromStdString((content_root_ / "catalog" / "variants" / "variants.json").string());
        if (QFile::exists(variantsPath)) {
            QFile vf(variantsPath);
            if (vf.open(QIODevice::ReadOnly)) {
                auto doc = QJsonDocument::fromJson(vf.readAll());
                vf.close();
                if (doc.isArray()) {
                    auto arr = doc.array();
                    bool varModified = false;
                    for (int i = 0; i < arr.size(); ++i) {
                        if (arr[i].isObject()) {
                            auto vObj = arr[i].toObject();
                            auto pres = vObj.value(QStringLiteral("presentation")).toObject();
                            auto mediaArr = pres.value(QStringLiteral("media")).toArray();
                            QJsonArray newMediaArr;
                            bool subMod = false;
                            for (const auto& m : mediaArr) {
                                QString p = m.toString();
                                if (p == relAssetPath || p == requestedPath || p.endsWith(filenameOnly)) {
                                    subMod = true;
                                    varModified = true;
                                } else {
                                    newMediaArr.append(m);
                                }
                            }
                            if (subMod) {
                                pres[QStringLiteral("media")] = newMediaArr;
                                vObj[QStringLiteral("presentation")] = pres;
                                arr[i] = vObj;
                            }
                        }
                    }
                    if (varModified && vf.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                        vf.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
                        vf.close();
                    }
                }
            }
        }

        draft_modified_ = true;

        // Notifica o totem imediatamente via IPC control plane
        system::ControlClient client;
        (void)client.send_reload();

        QJsonObject resp;
        resp[QStringLiteral("success")] = true;
        resp[QStringLiteral("deleted")] = relAssetPath;
        resp[QStringLiteral("file_removed_from_disk")] = fileRemoved;
        resp[QStringLiteral("atoms_cleaned_count")] = cleanedAtoms;
        sendJsonResponse(socket, 200, QJsonDocument(resp).toJson(QJsonDocument::Compact));
        return;
    }

    sendJsonResponse(socket, 400, "{\"error\":\"Método não suportado para /api/media\"}");
}

void AdminHttpServer::handleRelationsRoute(QTcpSocket* socket, const QString& method, const QString& path, const QByteArray& body) {
    QJsonArray relArr = content_database_->relations();

    // POST /api/relations (Add)
    if (method == QStringLiteral("POST") && path == QStringLiteral("/api/relations")) {
        auto doc = QJsonDocument::fromJson(body);
        if (!doc.isObject()) {
            sendJsonResponse(socket, 400, "{\"error\":\"Payload JSON inválido\"}");
            return;
        }

        auto obj = doc.object();
        QString relId = obj.value(QStringLiteral("relation_id")).toString().trimmed();
        if (relId.isEmpty()) {
            relId = QStringLiteral("rel_") + slugify(obj.value(QStringLiteral("from")).toString()) +
                    QStringLiteral("_") + slugify(obj.value(QStringLiteral("to")).toString());
            obj[QStringLiteral("relation_id")] = relId;
        }

        relArr.append(obj);

        content_database_->replace_relations(relArr);
        draft_modified_ = true;

        QJsonObject resp;
        resp[QStringLiteral("success")] = true;
        resp[QStringLiteral("relation")] = obj;
        sendJsonResponse(socket, 201, QJsonDocument(resp).toJson(QJsonDocument::Compact));
        return;
    }

    // DELETE /api/relations/:id
    if (method == QStringLiteral("DELETE") && path.startsWith(QStringLiteral("/api/relations/"))) {
        QString id = path.mid(15);
        QJsonArray newArr;
        bool found = false;
        for (const auto& elem : relArr) {
            if (elem.isObject() && elem.toObject().value(QStringLiteral("relation_id")).toString() == id) {
                found = true;
            } else {
                newArr.append(elem);
            }
        }

        if (found) {
            content_database_->replace_relations(newArr);
            draft_modified_ = true;
            QJsonObject resp;
            resp[QStringLiteral("success")] = true;
            sendJsonResponse(socket, 200, QJsonDocument(resp).toJson(QJsonDocument::Compact));
        } else {
            sendJsonResponse(socket, 404, "{\"error\":\"Relação não encontrada\"}");
        }
        return;
    }

    sendJsonResponse(socket, 400, "{\"error\":\"Ação não suportada para relations\"}");
}

void AdminHttpServer::handleRecipesRoute(QTcpSocket* socket, const QString& method, const QString& path, const QByteArray& body) {
    QJsonArray recArr = content_database_->recipes();

    // POST /api/recipes (Add)
    if (method == QStringLiteral("POST") && path == QStringLiteral("/api/recipes")) {
        auto doc = QJsonDocument::fromJson(body);
        if (!doc.isObject()) {
            sendJsonResponse(socket, 400, "{\"error\":\"Payload JSON inválido\"}");
            return;
        }

        auto obj = doc.object();
        QString recId = obj.value(QStringLiteral("recipe_id")).toString().trimmed();
        if (recId.isEmpty()) {
            recId = QStringLiteral("recipe_") + slugify(obj.value(QStringLiteral("name")).toString());
            obj[QStringLiteral("recipe_id")] = recId;
        }

        recArr.append(obj);

        content_database_->replace_recipes(recArr);
        draft_modified_ = true;

        QJsonObject resp;
        resp[QStringLiteral("success")] = true;
        resp[QStringLiteral("recipe")] = obj;
        sendJsonResponse(socket, 201, QJsonDocument(resp).toJson(QJsonDocument::Compact));
        return;
    }

    // DELETE /api/recipes/:id
    if (method == QStringLiteral("DELETE") && path.startsWith(QStringLiteral("/api/recipes/"))) {
        QString id = path.mid(13);
        QJsonArray newArr;
        bool found = false;
        for (const auto& elem : recArr) {
            if (elem.isObject() && elem.toObject().value(QStringLiteral("recipe_id")).toString() == id) {
                found = true;
            } else {
                newArr.append(elem);
            }
        }

        if (found) {
            content_database_->replace_recipes(newArr);
            draft_modified_ = true;
            QJsonObject resp;
            resp[QStringLiteral("success")] = true;
            sendJsonResponse(socket, 200, QJsonDocument(resp).toJson(QJsonDocument::Compact));
        } else {
            sendJsonResponse(socket, 404, "{\"error\":\"Receita não encontrada\"}");
        }
        return;
    }

    sendJsonResponse(socket, 400, "{\"error\":\"Ação não suportada para recipes\"}");
}

void AdminHttpServer::handleHttpRequest(QTcpSocket* socket, const QByteArray& requestData) {
    int lineEnd = requestData.indexOf("\r\n");
    if (lineEnd == -1) lineEnd = requestData.indexOf('\n');
    if (lineEnd == -1) return;

    QString requestLine = QString::fromUtf8(requestData.left(lineEnd));
    auto parts = requestLine.split(' ');
    if (parts.size() < 2) return;

    QString method = parts[0];
    QString path = parts[1];

    if (method == QStringLiteral("OPTIONS")) {
        sendResponse(socket, 200, QStringLiteral("text/plain"), "");
        return;
    }

    // Extract body
    QByteArray body;
    int bodyStart = requestData.indexOf("\r\n\r\n");
    if (bodyStart != -1) {
        body = requestData.mid(bodyStart + 4);
    } else {
        bodyStart = requestData.indexOf("\n\n");
        if (bodyStart != -1) {
            body = requestData.mid(bodyStart + 2);
        }
    }

    // CRUD: Atoms
    if (path.startsWith(QStringLiteral("/api/atoms"))) {
        handleAtomsRoute(socket, method, path, body);
        return;
    }

    // CRUD: Upload Media
    if (method == QStringLiteral("POST") && path == QStringLiteral("/api/upload")) {
        handleUploadRoute(socket, body);
        return;
    }

    // CRUD: Media Management & Deletion
    if (path.startsWith(QStringLiteral("/api/media")) ||
        (method == QStringLiteral("DELETE") && path.startsWith(QStringLiteral("/api/upload")))) {
        handleMediaRoute(socket, method, path, body);
        return;
    }

    // CRUD: Relations
    if (path.startsWith(QStringLiteral("/api/relations"))) {
        handleRelationsRoute(socket, method, path, body);
        return;
    }

    // CRUD: Recipes
    if (path.startsWith(QStringLiteral("/api/recipes"))) {
        handleRecipesRoute(socket, method, path, body);
        return;
    }

    // API: GET /api/status
    if (method == QStringLiteral("GET") && path == QStringLiteral("/api/status")) {
        system::ControlClient client;
        bool kiosk_online = client.ping(300);
        QString activeAtomId;
        QString activeAtomTitle;
        QString kioskState;

        if (kiosk_online) {
            QString reply = client.send_command(QStringLiteral("STATUS_REQUEST"), 500);
            for (const auto& token : reply.split(' ', Qt::SkipEmptyParts)) {
                if (token.startsWith(QStringLiteral("active_atom_id="))) {
                    activeAtomId = token.mid(15).trimmed();
                } else if (token.startsWith(QStringLiteral("active_atom_title="))) {
                    activeAtomTitle = token.mid(18).trimmed().replace('_', ' ');
                } else if (token.startsWith(QStringLiteral("state="))) {
                    kioskState = token.mid(6).trimmed();
                }
            }
        }

        QJsonObject resp;
        resp[QStringLiteral("kiosk_online")] = kiosk_online;
        resp[QStringLiteral("kiosk_active_atom_id")] = activeAtomId;
        resp[QStringLiteral("kiosk_active_atom_title")] = activeAtomTitle;
        resp[QStringLiteral("kiosk_state")] = kioskState;
        resp[QStringLiteral("control_socket")] = system::resolve_control_socket_path();
        resp[QStringLiteral("editorial_state")] = draft_modified_ ? QStringLiteral("draft") : QStringLiteral("active");
        resp[QStringLiteral("draft_modified")] = draft_modified_;

        auto active = publisher_.active_bundle();
        if (active) {
            QJsonObject actObj;
            actObj[QStringLiteral("bundle_id")] = QString::fromStdString(active->bundle_id);
            actObj[QStringLiteral("version")] = QString::fromStdString(active->version);
            actObj[QStringLiteral("title")] = QString::fromStdString(active->title);
            actObj[QStringLiteral("default_theme")] = QString::fromStdString(active->default_theme);
            actObj[QStringLiteral("description")] = QString::fromStdString(active->description);
            actObj[QStringLiteral("curation_revision")] = static_cast<qint64>(active->curation_revision);
            actObj[QStringLiteral("content_hash")] = QString::fromStdString(active->content_hash);
            actObj[QStringLiteral("created_at")] = QString::fromStdString(active->created_at);
            resp[QStringLiteral("active_bundle")] = actObj;
        } else {
            resp[QStringLiteral("active_bundle")] = QJsonValue::Null;
        }

        QJsonArray bundlesArr;
        for (const auto& b : publisher_.list_bundles()) {
            QJsonObject bObj;
            bObj[QStringLiteral("bundle_id")] = QString::fromStdString(b.bundle_id);
            bObj[QStringLiteral("version")] = QString::fromStdString(b.version);
            bObj[QStringLiteral("title")] = QString::fromStdString(b.title);
            bObj[QStringLiteral("curation_revision")] = static_cast<qint64>(b.curation_revision);
            bObj[QStringLiteral("content_hash")] = QString::fromStdString(b.content_hash);
            bObj[QStringLiteral("created_at")] = QString::fromStdString(b.created_at);
            bundlesArr.append(bObj);
        }
        resp[QStringLiteral("bundles")] = bundlesArr;

        sendJsonResponse(socket, 200, QJsonDocument(resp).toJson(QJsonDocument::Compact));
        return;
    }

    // API: POST /api/kiosk/show (Force Totem to display specific atom)
    if (method == QStringLiteral("POST") && path == QStringLiteral("/api/kiosk/show")) {
        auto doc = QJsonDocument::fromJson(body);
        QString contentId = doc.object().value(QStringLiteral("content_id")).toString().trimmed();
        if (contentId.isEmpty()) {
            sendJsonResponse(socket, 400, "{\"error\":\"Parâmetro content_id obrigatório\"}");
            return;
        }

        system::ControlClient client;
        bool ok = client.show_atom(contentId, 1000);
        QJsonObject resp;
        resp[QStringLiteral("success")] = ok;
        resp[QStringLiteral("content_id")] = contentId;
        if (!ok) {
            resp[QStringLiteral("error")] = QStringLiteral("Totem offline ou não respondeu ao comando");
        }
        sendJsonResponse(socket, ok ? 200 : 503, QJsonDocument(resp).toJson(QJsonDocument::Compact));
        return;
    }

    // API: POST /api/kiosk/advance (Tell Totem to rotate to next content)
    if (method == QStringLiteral("POST") && path == QStringLiteral("/api/kiosk/advance")) {
        system::ControlClient client;
        bool ok = client.advance_content(1000);
        QJsonObject resp;
        resp[QStringLiteral("success")] = ok;
        if (!ok) {
            resp[QStringLiteral("error")] = QStringLiteral("Totem offline ou não respondeu ao comando");
        }
        sendJsonResponse(socket, ok ? 200 : 503, QJsonDocument(resp).toJson(QJsonDocument::Compact));
        return;
    }

    // API: GET /api/catalog
    if (method == QStringLiteral("GET") && path == QStringLiteral("/api/catalog")) {
        content_database_->export_workspace(content_root_);
        // Read directly from draft workspace to always reflect current editing changes
        std::filesystem::path load_path = content_root_ / "catalog";
        if (!std::filesystem::exists(load_path)) {
            load_path = content_root_;
        }

        content::ContentCatalog catalog;
        auto cat_res = catalog.load_from_directory(load_path);
        if (!cat_res) {
            QJsonObject err;
            err[QStringLiteral("error")] = QString::fromStdString(cat_res.error().to_string());
            sendJsonResponse(socket, 500, QJsonDocument(err).toJson(QJsonDocument::Compact));
            return;
        }

        QJsonObject resp;
        auto m = catalog.manifest();
        QJsonObject mObj;
        mObj[QStringLiteral("bundle_id")] = QString::fromStdString(m.bundle_id);
        mObj[QStringLiteral("version")] = QString::fromStdString(m.version);
        mObj[QStringLiteral("title")] = QString::fromStdString(m.title);
        mObj[QStringLiteral("default_theme")] = QString::fromStdString(m.default_theme);
        resp[QStringLiteral("manifest")] = mObj;

        QJsonArray atomsArr;
        for (const auto* atom : catalog.all_atoms()) {
            QJsonObject aObj;
            aObj[QStringLiteral("content_id")] = QString::fromStdString(atom->content_id);
            aObj[QStringLiteral("title")] = QString::fromStdString(atom->title);
            aObj[QStringLiteral("summary")] = QString::fromStdString(atom->summary);
            aObj[QStringLiteral("language")] = QString::fromStdString(atom->language);
            aObj[QStringLiteral("lifecycle_status")] = QString::fromStdString(atom->lifecycle_status);
            aObj[QStringLiteral("type")] = QString::fromStdString(std::string(content::to_string(atom->type)));
            aObj[QStringLiteral("subtype")] = QString::fromStdString(atom->subtype);
            aObj[QStringLiteral("canonical_name")] = QString::fromStdString(atom->subject.canonical_name);
            aObj[QStringLiteral("scientific_name")] = QString::fromStdString(atom->subject.scientific_name);
            aObj[QStringLiteral("type_label")] = QString::fromStdString(atom->subject.type_label);

            QJsonObject metadataObj;
            for (const auto& [key, value] : atom->custom_attributes) {
                metadataObj[QString::fromStdString(key)] = QString::fromStdString(value);
            }
            aObj[QStringLiteral("metadata")] = metadataObj;

            QJsonObject provenanceObj;
            provenanceObj[QStringLiteral("creator")] = QString::fromStdString(atom->creator);
            provenanceObj[QStringLiteral("publisher")] = QString::fromStdString(atom->publisher);
            provenanceObj[QStringLiteral("source_reference")] = QString::fromStdString(atom->source_reference);
            provenanceObj[QStringLiteral("created_at")] = QString::fromStdString(atom->created_at);
            provenanceObj[QStringLiteral("modified_at")] = QString::fromStdString(atom->modified_at);
            provenanceObj[QStringLiteral("reviewed")] = atom->reviewed;
            aObj[QStringLiteral("provenance")] = provenanceObj;

            QJsonObject rightsObj;
            rightsObj[QStringLiteral("license")] = QString::fromStdString(atom->license);
            rightsObj[QStringLiteral("rights_holder")] = QString::fromStdString(atom->rights_holder);
            rightsObj[QStringLiteral("attribution")] = QString::fromStdString(atom->attribution);
            aObj[QStringLiteral("rights")] = rightsObj;

            QJsonObject accessibilityObj;
            accessibilityObj[QStringLiteral("alt_text")] = QString::fromStdString(atom->alt_text);
            accessibilityObj[QStringLiteral("transcript")] = QString::fromStdString(atom->transcript);
            aObj[QStringLiteral("accessibility")] = accessibilityObj;

            QJsonArray themesArr;
            for (const auto& t : atom->themes) themesArr.append(QString::fromStdString(t));
            aObj[QStringLiteral("themes")] = themesArr;

            QJsonArray factsArr;
            for (const auto& fact : atom->canonical_facts) {
                QJsonObject fObj;
                fObj[QStringLiteral("fact_id")] = QString::fromStdString(fact.fact_id);
                fObj[QStringLiteral("statement")] = QString::fromStdString(fact.statement);
                fObj[QStringLiteral("confidence")] = QString::fromStdString(fact.confidence);
                QJsonArray srcArr;
                for (const auto& s : fact.source_ids) srcArr.append(QString::fromStdString(s));
                fObj[QStringLiteral("source_ids")] = srcArr;
                factsArr.append(fObj);
            }
            aObj[QStringLiteral("canonical_facts")] = factsArr;

            QJsonArray imgsArr;
            for (const auto& img : atom->assets.images) imgsArr.append(QString::fromStdString(img));
            aObj[QStringLiteral("images")] = imgsArr;

            QJsonArray audsArr;
            for (const auto& aud : atom->assets.audios) audsArr.append(QString::fromStdString(aud));
            aObj[QStringLiteral("audios")] = audsArr;

            atomsArr.append(aObj);
        }
        resp[QStringLiteral("atoms")] = atomsArr;

        QJsonArray recipesArr;
        for (const auto* rec : catalog.all_recipes()) {
            QJsonObject rObj;
            rObj[QStringLiteral("recipe_id")] = QString::fromStdString(rec->recipe_id);
            rObj[QStringLiteral("name")] = QString::fromStdString(rec->name);
            rObj[QStringLiteral("description")] = QString::fromStdString(rec->description);
            rObj[QStringLiteral("step_count")] = static_cast<qint64>(rec->steps.size());
            recipesArr.append(rObj);
        }
        resp[QStringLiteral("recipes")] = recipesArr;

        sendJsonResponse(socket, 200, QJsonDocument(resp).toJson(QJsonDocument::Compact));
        return;
    }

    // API: GET /api/packages — reusable editorial package plans
    if (method == QStringLiteral("GET") && path == QStringLiteral("/api/packages")) {
        const auto active = publisher_.active_bundle();
        const auto published = publisher_.list_bundles();
        QJsonArray packageList;
        for (const auto& value : content_database_->packages()) {
            auto plan = value.toObject();
            const QString id = plan.value(QStringLiteral("bundle_id")).toString();
            QJsonArray versions;
            for (const auto& bundle : published) {
                if (QString::fromStdString(bundle.bundle_id) == id) versions.append(QString::fromStdString(bundle.version));
            }
            plan[QStringLiteral("published_versions")] = versions;
            plan[QStringLiteral("is_published")] = !versions.isEmpty();
            plan[QStringLiteral("is_active")] = active && QString::fromStdString(active->bundle_id) == id;
            plan[QStringLiteral("active_version")] = plan.value(QStringLiteral("is_active")).toBool()
                ? QString::fromStdString(active->version) : QString();
            packageList.append(plan);
        }
        QJsonObject response;
        response[QStringLiteral("packages")] = packageList;
        response[QStringLiteral("storage")] = QStringLiteral("sqlite");
        response[QStringLiteral("journal_mode")] = content_database_->journal_mode();
        sendJsonResponse(socket, 200, QJsonDocument(response).toJson(QJsonDocument::Compact));
        return;
    }

    if (method == QStringLiteral("POST") && path == QStringLiteral("/api/packages")) {
        const auto document = QJsonDocument::fromJson(body);
        const auto plan = document.object();
        const QString id = plan.value(QStringLiteral("bundle_id")).toString().trimmed();
        if (!document.isObject() || id.isEmpty() || plan.value(QStringLiteral("title")).toString().trimmed().isEmpty()) {
            sendJsonResponse(socket, 400, "{\"error\":\"Pacote exige bundle_id e title\"}");
            return;
        }
        content_database_->upsert_package(plan);
        draft_modified_ = true;
        QJsonObject response{{QStringLiteral("success"), true}, {QStringLiteral("package"), plan}};
        sendJsonResponse(socket, 200, QJsonDocument(response).toJson(QJsonDocument::Compact));
        return;
    }

    if (method == QStringLiteral("DELETE") && path.startsWith(QStringLiteral("/api/packages/"))) {
        const QString id = sanitize_filename(QUrl::fromPercentEncoding(path.mid(14).toUtf8()));
        const auto active = publisher_.active_bundle();
        if (active && QString::fromStdString(active->bundle_id) == id) {
            sendJsonResponse(socket, 409, "{\"error\":\"A aplicação ativa não pode ser excluída. Desative-a ou ative outra aplicação primeiro.\"}");
            return;
        }
        const bool removed = content_database_->delete_package(id);
        sendJsonResponse(socket, removed ? 200 : 404, removed ? "{\"success\":true}" : "{\"error\":\"Pacote não encontrado\"}");
        return;
    }

    if (method == QStringLiteral("POST") && path.startsWith(QStringLiteral("/api/packages/"))
        && path.endsWith(QStringLiteral("/activate"))) {
        const QString encodedId = path.mid(14, path.size() - 14 - 9);
        const QString id = sanitize_filename(QUrl::fromPercentEncoding(encodedId.toUtf8()));
        const auto plan = content_database_->package(id);
        if (plan.isEmpty()) {
            sendJsonResponse(socket, 404, "{\"error\":\"Aplicação não encontrada\"}");
            return;
        }
        content_database_->export_workspace(content_root_);
        QSaveFile manifest(QString::fromStdString((content_root_ / "manifest.json").string()));
        if (!manifest.open(QIODevice::WriteOnly)
            || manifest.write(QJsonDocument(plan).toJson(QJsonDocument::Indented)) < 0
            || !manifest.commit()) {
            sendJsonResponse(socket, 500, "{\"error\":\"Falha ao preparar a versão publicável da aplicação\"}");
            return;
        }
        const auto result = publisher_.publish_and_activate(content_root_, "curator_local");
        QJsonObject response{{QStringLiteral("success"), result.success}};
        if (result.success) {
            draft_modified_ = false;
            response[QStringLiteral("bundle_id")] = QString::fromStdString(result.bundle_id);
            response[QStringLiteral("version")] = QString::fromStdString(result.version);
            response[QStringLiteral("content_hash")] = QString::fromStdString(result.content_hash);
            system::ControlClient client;
            response[QStringLiteral("kiosk_notified")] = client.notify_published(
                QString::fromStdString(result.bundle_id), QString::fromStdString(result.content_hash));
        } else {
            response[QStringLiteral("error")] = QString::fromStdString(result.error_message);
        }
        sendJsonResponse(socket, result.success ? 200 : 400, QJsonDocument(response).toJson(QJsonDocument::Compact));
        return;
    }

    if (method == QStringLiteral("POST") && path.startsWith(QStringLiteral("/api/packages/"))
        && path.endsWith(QStringLiteral("/deactivate"))) {
        const QString encodedId = path.mid(14, path.size() - 14 - 11);
        const QString id = sanitize_filename(QUrl::fromPercentEncoding(encodedId.toUtf8()));
        const auto active = publisher_.active_bundle();
        if (!active || QString::fromStdString(active->bundle_id) != id) {
            sendJsonResponse(socket, 409, "{\"error\":\"Esta aplicação não está ativa\"}");
            return;
        }
        const auto result = publisher_.deactivate();
        QJsonObject response{{QStringLiteral("success"), result.has_value()}};
        if (result) {
            system::ControlClient client;
            response[QStringLiteral("kiosk_notified")] = client.notify_deactivated();
        } else {
            response[QStringLiteral("error")] = QString::fromStdString(result.error().to_string());
        }
        sendJsonResponse(socket, result ? 200 : 500, QJsonDocument(response).toJson(QJsonDocument::Compact));
        return;
    }

    if (method == QStringLiteral("GET") && path.startsWith(QStringLiteral("/api/packages/"))) {
        const QString id = sanitize_filename(QUrl::fromPercentEncoding(path.mid(14).toUtf8()));
        const auto value = content_database_->package(id);
        if (!value.isEmpty()) {
            sendJsonResponse(socket, 200, QJsonDocument(value).toJson(QJsonDocument::Compact));
        } else {
            sendJsonResponse(socket, 404, "{\"error\":\"Aplicação não encontrada\"}");
        }
        return;
    }

    // API: GET /api/manifest
    if (method == QStringLiteral("GET") && path == QStringLiteral("/api/manifest")) {
        auto manifest_path = content_root_ / "manifest.json";
        if (!std::filesystem::exists(manifest_path)) {
            manifest_path = content_root_ / "catalog" / "manifest.json";
        }
        if (std::filesystem::exists(manifest_path)) {
            QFile f(QString::fromStdString(manifest_path.string()));
            if (f.open(QIODevice::ReadOnly)) {
                sendJsonResponse(socket, 200, f.readAll());
                return;
            }
        }
        sendJsonResponse(socket, 404, "{\"error\":\"Manifesto não encontrado\"}");
        return;
    }

    // API: POST /api/manifest (Configure Package Metadata)
    if ((method == QStringLiteral("POST") || method == QStringLiteral("PUT")) && path == QStringLiteral("/api/manifest")) {
        auto doc = QJsonDocument::fromJson(body);
        if (doc.isNull() || !doc.isObject()) {
            sendJsonResponse(socket, 400, "{\"error\":\"JSON inválido\"}");
            return;
        }

        auto manifest_path = content_root_ / "manifest.json";
        QFile f(QString::fromStdString(manifest_path.string()));
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            sendJsonResponse(socket, 500, "{\"error\":\"Não foi possível gravar manifest.json\"}");
            return;
        }

        f.write(doc.toJson(QJsonDocument::Indented));
        f.close();

        content_database_->upsert_package(doc.object());
        draft_modified_ = true;

        QJsonObject resp;
        resp[QStringLiteral("success")] = true;
        resp[QStringLiteral("message")] = QStringLiteral("Pacote de conteúdo configurado com sucesso.");
        sendJsonResponse(socket, 200, QJsonDocument(resp).toJson(QJsonDocument::Compact));
        return;
    }

    // API: POST /api/validate
    if (method == QStringLiteral("POST") && path == QStringLiteral("/api/validate")) {
        content_database_->export_workspace(content_root_);
        content::ContentBundle bundle(content_root_);
        auto report = bundle.validate();

        QJsonObject resp;
        resp[QStringLiteral("valid")] = report.valid;
        QJsonArray errArr;
        for (const auto& e : report.errors) {
            QJsonObject eObj;
            eObj[QStringLiteral("item_id")] = QString::fromStdString(e.item_id);
            eObj[QStringLiteral("message")] = QString::fromStdString(e.message);
            errArr.append(eObj);
        }
        resp[QStringLiteral("errors")] = errArr;

        QJsonArray warnArr;
        for (const auto& w : report.warnings) {
            warnArr.append(QString::fromStdString(w));
        }
        resp[QStringLiteral("warnings")] = warnArr;

        sendJsonResponse(socket, 200, QJsonDocument(resp).toJson(QJsonDocument::Compact));
        return;
    }

    // API: POST /api/publish
    if (method == QStringLiteral("POST") && path == QStringLiteral("/api/publish")) {
        QString targetId;
        if (!body.isEmpty()) {
            auto doc = QJsonDocument::fromJson(body);
            if (doc.isObject()) targetId = doc.object().value(QStringLiteral("bundle_id")).toString().trimmed();
        }
        if (targetId.isEmpty()) {
            const auto active = publisher_.active_bundle();
            if (active) targetId = QString::fromStdString(active->bundle_id);
        }
        if (!targetId.isEmpty()) {
            const auto plan = content_database_->package(targetId);
            if (!plan.isEmpty()) {
                QSaveFile manifest(QString::fromStdString((content_root_ / "manifest.json").string()));
                if (manifest.open(QIODevice::WriteOnly)) {
                    manifest.write(QJsonDocument(plan).toJson(QJsonDocument::Indented));
                    manifest.commit();
                }
            }
        }
        content_database_->export_workspace(content_root_);
        auto pub_res = publisher_.publish_and_activate(content_root_, "curator_local");

        QJsonObject resp;
        resp[QStringLiteral("success")] = pub_res.success;
        if (pub_res.success) {
            draft_modified_ = false;
            resp[QStringLiteral("bundle_id")] = QString::fromStdString(pub_res.bundle_id);
            resp[QStringLiteral("version")] = QString::fromStdString(pub_res.version);
            resp[QStringLiteral("content_hash")] = QString::fromStdString(pub_res.content_hash);

            // Notify kiosk via control socket IPC
            system::ControlClient client;
            bool notified = client.notify_published(
                QString::fromStdString(pub_res.bundle_id),
                QString::fromStdString(pub_res.content_hash));
            resp[QStringLiteral("kiosk_notified")] = notified;
        } else {
            resp[QStringLiteral("error")] = QString::fromStdString(pub_res.error_message);
        }

        sendJsonResponse(socket, pub_res.success ? 200 : 400, QJsonDocument(resp).toJson(QJsonDocument::Compact));
        return;
    }

    // API: POST /api/rollback
    if (method == QStringLiteral("POST") && path == QStringLiteral("/api/rollback")) {
        auto doc = QJsonDocument::fromJson(body);
        QString target = doc.object().value(QStringLiteral("target")).toString();

        auto rb_res = publisher_.rollback_to(target.toStdString());
        QJsonObject resp;
        resp[QStringLiteral("success")] = rb_res.has_value();
        if (rb_res.has_value()) {
            draft_modified_ = false;
            system::ControlClient client;
            (void)client.send_reload();
            resp[QStringLiteral("kiosk_reloaded")] = true;
        } else {
            resp[QStringLiteral("error")] = QString::fromStdString(rb_res.error().to_string());
        }

        sendJsonResponse(socket, rb_res.has_value() ? 200 : 400, QJsonDocument(resp).toJson(QJsonDocument::Compact));
        return;
    }

    // API: POST /api/kiosk/reload
    if (method == QStringLiteral("POST") && path == QStringLiteral("/api/kiosk/reload")) {
        system::ControlClient client;
        bool ok = client.send_reload();
        QJsonObject resp;
        resp[QStringLiteral("success")] = ok;
        sendJsonResponse(socket, ok ? 200 : 503, QJsonDocument(resp).toJson(QJsonDocument::Compact));
        return;
    }

    // Helper to resolve kiosk binary
    auto resolve_kiosk_bin = []() -> QString {
        auto env = QProcessEnvironment::systemEnvironment();
        QString custom = env.value(QStringLiteral("ELO_KIOSK_BIN"));
        if (!custom.isEmpty() && QFile::exists(custom)) return custom;

        QString appDir = QCoreApplication::applicationDirPath();
        for (const auto& cand : {
            appDir + QStringLiteral("/elo-kiosk"),
            appDir + QStringLiteral("/../elo-kiosk/elo-kiosk"),
            QStringLiteral("./build/apps/elo-kiosk/elo-kiosk"),
            QStringLiteral("/usr/bin/elo-kiosk"),
            QStringLiteral("/usr/local/bin/elo-kiosk")
        }) {
            if (QFile::exists(cand)) {
                return QFileInfo(cand).canonicalFilePath();
            }
        }
        return QStringLiteral("./build/apps/elo-kiosk/elo-kiosk");
    };

    // API: POST /api/kiosk/stop (Shutdown Kiosk Process)
    if (method == QStringLiteral("POST") && (path == QStringLiteral("/api/kiosk/stop") || path == QStringLiteral("/api/kiosk/shutdown"))) {
        system::ControlClient client;
        bool ok = client.shutdown_kiosk();
        if (!ok) {
            QProcess::execute(QStringLiteral("pkill"), {QStringLiteral("-f"), QStringLiteral("elo-kiosk")});
            ok = true;
        }
        QJsonObject resp;
        resp[QStringLiteral("success")] = ok;
        resp[QStringLiteral("message")] = QStringLiteral("Comando de encerramento enviado ao totem.");
        sendJsonResponse(socket, 200, QJsonDocument(resp).toJson(QJsonDocument::Compact));
        return;
    }

    // API: POST /api/kiosk/start (Launch Kiosk Process)
    if (method == QStringLiteral("POST") && path == QStringLiteral("/api/kiosk/start")) {
        QString bin = resolve_kiosk_bin();
        QProcess proc;
        proc.setProgram(bin);
        proc.setWorkingDirectory(QDir::currentPath());

        // Forward environment to child process
        auto env = QProcessEnvironment::systemEnvironment();
        proc.setProcessEnvironment(env);

        qint64 pid = 0;
        bool launched = proc.startDetached(&pid);
        std::cout << "[ELO][admin] Launching kiosk binary: " << bin.toStdString() << " (PID=" << pid << ", launched=" << (launched ? "yes" : "no") << ")\n";

        QJsonObject resp;
        resp[QStringLiteral("success")] = launched;
        resp[QStringLiteral("message")] = launched ? QStringLiteral("Totem iniciado com sucesso.") : QStringLiteral("Falha ao iniciar processo do Totem.");
        resp[QStringLiteral("binary")] = bin;
        resp[QStringLiteral("pid")] = pid;
        sendJsonResponse(socket, launched ? 200 : 500, QJsonDocument(resp).toJson(QJsonDocument::Compact));
        return;
    }

    // API: POST /api/kiosk/restart (Restart Kiosk Process)
    if (method == QStringLiteral("POST") && path == QStringLiteral("/api/kiosk/restart")) {
        system::ControlClient client;
        (void)client.shutdown_kiosk();
        QProcess::execute(QStringLiteral("pkill"), {QStringLiteral("-f"), QStringLiteral("elo-kiosk")});
        QThread::msleep(500);

        QString bin = resolve_kiosk_bin();
        QProcess proc;
        proc.setProgram(bin);
        proc.setWorkingDirectory(QDir::currentPath());
        proc.setProcessEnvironment(QProcessEnvironment::systemEnvironment());

        qint64 pid = 0;
        bool launched = proc.startDetached(&pid);
        std::cout << "[ELO][admin] Restarting kiosk binary: " << bin.toStdString() << " (PID=" << pid << ")\n";

        QJsonObject resp;
        resp[QStringLiteral("success")] = launched;
        resp[QStringLiteral("message")] = launched ? QStringLiteral("Totem reiniciado com sucesso.") : QStringLiteral("Falha ao reiniciar Totem.");
        resp[QStringLiteral("binary")] = bin;
        resp[QStringLiteral("pid")] = pid;
        sendJsonResponse(socket, launched ? 200 : 500, QJsonDocument(resp).toJson(QJsonDocument::Compact));
        return;
    }

    // API: GET /api/applications
    if (method == QStringLiteral("GET") && path == QStringLiteral("/api/applications")) {
        QJsonArray appsArr;
        for (const auto& app : app_registry_.list_all()) {
            QJsonObject aObj;
            aObj[QStringLiteral("app_id")] = QString::fromStdString(app.app_id);
            aObj[QStringLiteral("name")] = QString::fromStdString(app.name);
            aObj[QStringLiteral("domain_category")] = QString::fromStdString(app.domain_category);
            aObj[QStringLiteral("version")] = QString::fromStdString(app.version);
            aObj[QStringLiteral("description")] = QString::fromStdString(app.description);
            aObj[QStringLiteral("active_bundle_id")] = QString::fromStdString(app.active_bundle_id);
            aObj[QStringLiteral("target_audience")] = QString::fromStdString(app.target_audience);
            aObj[QStringLiteral("default_theme")] = QString::fromStdString(app.default_theme);
            aObj[QStringLiteral("is_active")] = app.is_active;
            aObj[QStringLiteral("telemetry_enabled")] = app.telemetry_enabled;
            aObj[QStringLiteral("registered_at")] = static_cast<qint64>(app.registered_at);

            QJsonArray tagsArr;
            for (const auto& t : app.tags) tagsArr.append(QString::fromStdString(t));
            aObj[QStringLiteral("tags")] = tagsArr;

            QJsonObject metaObj;
            for (const auto& [k, v] : app.metadata_schema) metaObj[QString::fromStdString(k)] = QString::fromStdString(v);
            aObj[QStringLiteral("metadata_schema")] = metaObj;
            appsArr.append(aObj);
        }
        QJsonObject resp;
        resp[QStringLiteral("applications")] = appsArr;
        sendJsonResponse(socket, 200, QJsonDocument(resp).toJson(QJsonDocument::Compact));
        return;
    }

    // API: POST /api/applications/activate (Activate Project on Totem)
    if (method == QStringLiteral("POST") && path == QStringLiteral("/api/applications/activate")) {
        auto doc = QJsonDocument::fromJson(body);
        QString appId = doc.object().value(QStringLiteral("app_id")).toString().trimmed();
        if (appId.isEmpty()) {
            sendJsonResponse(socket, 400, "{\"error\":\"app_id é obrigatório\"}");
            return;
        }

        bool ok = app_registry_.activate(appId.toStdString());
        if (!ok) {
            sendJsonResponse(socket, 404, "{\"error\":\"Projeto/Aplicação não encontrado\"}");
            return;
        }

        std::string appsPath = (content_root_ / "applications.json").string();
        app_registry_.save_to_file(appsPath);

        // Link active bundle to kiosk if application specifies a bundle
        auto app = app_registry_.find(appId.toStdString());
        if (app && !app->active_bundle_id.empty()) {
            (void)publisher_.rollback_to(app->active_bundle_id);
            system::ControlClient client;
            (void)client.send_reload();
        }

        QJsonObject resp;
        resp[QStringLiteral("success")] = true;
        resp[QStringLiteral("message")] = QStringLiteral("Projeto ") + appId + QStringLiteral(" ativado com sucesso no totem.");
        resp[QStringLiteral("active_app_id")] = appId;
        sendJsonResponse(socket, 200, QJsonDocument(resp).toJson(QJsonDocument::Compact));
        return;
    }

    // API: POST /api/applications (Create or Update Application Profile)
    if ((method == QStringLiteral("POST") || method == QStringLiteral("PUT")) && path == QStringLiteral("/api/applications")) {
        auto doc = QJsonDocument::fromJson(body);
        if (doc.isNull() || !doc.isObject()) {
            sendJsonResponse(socket, 400, "{\"error\":\"JSON inválido\"}");
            return;
        }
        auto obj = doc.object();
        application::ApplicationProfile app;
        app.app_id = obj.value(QStringLiteral("app_id")).toString().trimmed().toStdString();
        app.name = obj.value(QStringLiteral("name")).toString().trimmed().toStdString();
        if (app.app_id.empty() || app.name.empty()) {
            sendJsonResponse(socket, 400, "{\"error\":\"app_id e name são campos obrigatórios\"}");
            return;
        }

        app.domain_category = obj.value(QStringLiteral("domain_category")).toString(QStringLiteral("general")).toStdString();
        app.version = obj.value(QStringLiteral("version")).toString(QStringLiteral("1.0.0")).toStdString();
        app.description = obj.value(QStringLiteral("description")).toString().toStdString();
        app.active_bundle_id = obj.value(QStringLiteral("active_bundle_id")).toString().toStdString();
        app.target_audience = obj.value(QStringLiteral("target_audience")).toString(QStringLiteral("general")).toStdString();
        app.default_theme = obj.value(QStringLiteral("default_theme")).toString(QStringLiteral("default")).toStdString();
        app.is_active = obj.value(QStringLiteral("is_active")).toBool(false);
        app.telemetry_enabled = obj.value(QStringLiteral("telemetry_enabled")).toBool(true);
        
        for (const auto& tVal : obj.value(QStringLiteral("tags")).toArray()) {
            QString tStr = tVal.toString().trimmed();
            if (!tStr.isEmpty()) app.tags.push_back(tStr.toStdString());
        }

        auto metaObj = obj.value(QStringLiteral("metadata_schema")).toObject();
        for (auto it = metaObj.begin(); it != metaObj.end(); ++it) {
            app.metadata_schema[it.key().toStdString()] = it.value().toString().toStdString();
        }

        app_registry_.register_application(std::move(app));
        std::string appsPath = (content_root_ / "applications.json").string();
        app_registry_.save_to_file(appsPath);

        QJsonObject resp;
        resp[QStringLiteral("success")] = true;
        resp[QStringLiteral("message")] = QStringLiteral("Projeto/Aplicação salvo com sucesso.");
        sendJsonResponse(socket, 200, QJsonDocument(resp).toJson(QJsonDocument::Compact));
        return;
    }

    // API: DELETE /api/applications
    if ((method == QStringLiteral("DELETE") && path.startsWith(QStringLiteral("/api/applications"))) ||
        (method == QStringLiteral("POST") && path == QStringLiteral("/api/applications/delete"))) {
        QString targetId;
        if (!body.isEmpty()) {
            auto doc = QJsonDocument::fromJson(body);
            if (doc.isObject()) targetId = doc.object().value(QStringLiteral("app_id")).toString().trimmed();
        }
        if (targetId.isEmpty()) {
            int qIdx = path.indexOf('?');
            if (qIdx != -1) {
                QUrlQuery query(path.mid(qIdx + 1));
                targetId = query.queryItemValue(QStringLiteral("id"));
            }
        }

        if (targetId.isEmpty()) {
            sendJsonResponse(socket, 400, "{\"error\":\"app_id não informado para exclusão\"}");
            return;
        }

        bool removed = app_registry_.remove(targetId.toStdString());
        if (removed) {
            std::string appsPath = (content_root_ / "applications.json").string();
            app_registry_.save_to_file(appsPath);
        }

        QJsonObject resp;
        resp[QStringLiteral("success")] = removed;
        resp[QStringLiteral("message")] = removed ? QStringLiteral("Projeto excluído com sucesso.") : QStringLiteral("Projeto não encontrado.");
        sendJsonResponse(socket, removed ? 200 : 404, QJsonDocument(resp).toJson(QJsonDocument::Compact));
        return;
    }

    // API: GET /api/analytics/summary
    if (method == QStringLiteral("GET") && path.startsWith(QStringLiteral("/api/analytics/summary"))) {
        const QUrl requestUrl(path);
        const QString applicationId = QUrlQuery(requestUrl).queryItemValue(QStringLiteral("application_id"));
        analytics::AnalyticsEngine engine;
        const auto events = content_database_->analytics_events(applicationId);
        for (const auto& value : events) {
            const auto event = value.toObject();
            engine.record_event(
                event.value(QStringLiteral("application_id")).toString().toStdString(),
                event.value(QStringLiteral("application_version")).toString().toStdString(),
                event.value(QStringLiteral("event_type")).toString().toStdString(),
                event.value(QStringLiteral("target_atom_id")).toString().toStdString(),
                event.value(QStringLiteral("source_atom_id")).toString().toStdString(),
                event.value(QStringLiteral("duration_seconds")).toDouble());
        }
        auto summary = engine.get_summary(applicationId.toStdString());
        QJsonObject resp;
        resp[QStringLiteral("application_id")] = applicationId.isEmpty() ? QStringLiteral("all_applications") : applicationId;
        resp[QStringLiteral("event_count")] = events.size();
        resp[QStringLiteral("total_sessions")] = static_cast<qint64>(summary.total_sessions);
        resp[QStringLiteral("average_dwell_time_seconds")] = summary.average_dwell_time_seconds;
        resp[QStringLiteral("total_engagement_seconds")] = summary.total_engagement_seconds;
        resp[QStringLiteral("total_atom_views")] = static_cast<qint64>(summary.total_atom_views);
        resp[QStringLiteral("total_recipe_completions")] = static_cast<qint64>(summary.total_recipe_completions);
        
        QJsonObject viewsObj;
        for (const auto& [atom, count] : summary.atom_view_counts) {
            viewsObj[QString::fromStdString(atom)] = static_cast<qint64>(count);
        }
        resp[QStringLiteral("atom_view_counts")] = viewsObj;

        QJsonObject dwellObj;
        for (const auto& [atom, avg] : summary.atom_avg_dwell_seconds) {
            dwellObj[QString::fromStdString(atom)] = avg;
        }
        resp[QStringLiteral("atom_avg_dwell_seconds")] = dwellObj;

        QJsonObject flowObj;
        for (const auto& [src, targets] : summary.transition_matrix) {
            QJsonObject tObj;
            for (const auto& [tgt, count] : targets) {
                tObj[QString::fromStdString(tgt)] = static_cast<qint64>(count);
            }
            flowObj[QString::fromStdString(src)] = tObj;
        }
        resp[QStringLiteral("transition_matrix")] = flowObj;

        sendJsonResponse(socket, 200, QJsonDocument(resp).toJson(QJsonDocument::Compact));
        return;
    }

    // API: GET /api/biometric/spec
    if (method == QStringLiteral("GET") && path == QStringLiteral("/api/biometric/spec")) {
        QJsonObject resp;
        resp[QStringLiteral("architecture")] = QStringLiteral("elo-arcface-512-v1");
        resp[QStringLiteral("dimension")] = 512;
        resp[QStringLiteral("key_format")] = QStringLiteral("elo://bio/v1/{salt_hmac_sha256}");
        resp[QStringLiteral("privacy_guarantee")] = QStringLiteral("Zero-PII Sovereign Ephemeral Key Derivation (Invariants E12 & E13)");
        resp[QStringLiteral("hardware_isolated")] = true;
        resp[QStringLiteral("non_reversible")] = true;
        sendJsonResponse(socket, 200, QJsonDocument(resp).toJson(QJsonDocument::Compact));
        return;
    }

    // Static Web Assets
    QString filePath;
    QString contentType = QStringLiteral("text/html; charset=utf-8");

    if (path == QStringLiteral("/") || path == QStringLiteral("/index.html")) {
        filePath = QString::fromStdString((web_root_ / "index.html").string());
    } else if (path == QStringLiteral("/app.css") || path == QStringLiteral("/style.css")) {
        filePath = QString::fromStdString((web_root_ / "app.css").string());
        contentType = QStringLiteral("text/css; charset=utf-8");
    } else if (path == QStringLiteral("/app.js")) {
        filePath = QString::fromStdString((web_root_ / "app.js").string());
        contentType = QStringLiteral("application/javascript; charset=utf-8");
    } else if (path.startsWith(QStringLiteral("/assets/"))) {
        QString assetSub = path.mid(8);
        filePath = QString::fromStdString((content_root_ / "assets" / assetSub.toStdString()).string());
        if (!QFile::exists(filePath)) {
            filePath = QString::fromStdString((content_root_ / assetSub.toStdString()).string());
        }
        if (filePath.endsWith(QStringLiteral(".png"))) contentType = QStringLiteral("image/png");
        else if (filePath.endsWith(QStringLiteral(".jpg")) || filePath.endsWith(QStringLiteral(".jpeg"))) contentType = QStringLiteral("image/jpeg");
        else if (filePath.endsWith(QStringLiteral(".webp"))) contentType = QStringLiteral("image/webp");
        else if (filePath.endsWith(QStringLiteral(".wav"))) contentType = QStringLiteral("audio/wav");
        else if (filePath.endsWith(QStringLiteral(".ogg"))) contentType = QStringLiteral("audio/ogg");
        else contentType = QStringLiteral("application/octet-stream");
    }

    if (!filePath.isEmpty() && QFile::exists(filePath)) {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly)) {
            sendResponse(socket, 200, contentType, file.readAll());
            return;
        }
    }

    sendResponse(socket, 404, QStringLiteral("text/plain"), "404 Not Found in ELO Studio");
}

} // namespace elo::admin
