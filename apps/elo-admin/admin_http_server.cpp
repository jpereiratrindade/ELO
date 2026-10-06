#include "admin_http_server.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
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
        if (clEnd != -1) {
            QByteArray clVal = headerBytes.mid(clIdx + 15, clEnd - (clIdx + 15)).trimmed();
            contentLength = clVal.toInt();
        }
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
    QString atomsDir = QString::fromStdString((content_root_ / "catalog" / "atoms").string());
    QDir().mkpath(atomsDir);

    // GET /api/atoms/:id
    if (method == QStringLiteral("GET") && path.startsWith(QStringLiteral("/api/atoms/"))) {
        QString id = path.mid(11);
        QString filePath = QDir(atomsDir).filePath(id + QStringLiteral(".json"));
        if (!QFile::exists(filePath)) {
            filePath = QString::fromStdString((content_root_ / "catalog" / "objects" / (id.toStdString() + ".json")).string());
        }

        if (QFile::exists(filePath)) {
            QFile f(filePath);
            if (f.open(QIODevice::ReadOnly)) {
                sendJsonResponse(socket, 200, f.readAll());
                return;
            }
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

        if (!obj.contains(QStringLiteral("schema_version"))) {
            obj[QStringLiteral("schema_version")] = QStringLiteral("0.1");
        }

        QString filePath = QDir(atomsDir).filePath(contentId + QStringLiteral(".json"));
        QFile f(filePath);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            sendJsonResponse(socket, 500, "{\"error\":\"Não foi possível gravar arquivo do átomo\"}");
            return;
        }

        f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
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
        QString id = path.mid(11);
        auto doc = QJsonDocument::fromJson(body);
        if (!doc.isObject()) {
            sendJsonResponse(socket, 400, "{\"error\":\"Payload JSON inválido\"}");
            return;
        }

        auto obj = doc.object();
        obj[QStringLiteral("content_id")] = id;

        QString filePath = QDir(atomsDir).filePath(id + QStringLiteral(".json"));
        QFile f(filePath);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            sendJsonResponse(socket, 500, "{\"error\":\"Não foi possível atualizar arquivo do átomo\"}");
            return;
        }

        f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
        draft_modified_ = true;

        QJsonObject resp;
        resp[QStringLiteral("success")] = true;
        resp[QStringLiteral("content_id")] = id;
        sendJsonResponse(socket, 200, QJsonDocument(resp).toJson(QJsonDocument::Compact));
        return;
    }

    // DELETE /api/atoms/:id (Delete)
    if (method == QStringLiteral("DELETE") && path.startsWith(QStringLiteral("/api/atoms/"))) {
        QString id = path.mid(11);
        QString filePath = QDir(atomsDir).filePath(id + QStringLiteral(".json"));
        bool removed = false;
        if (QFile::exists(filePath)) {
            removed = QFile::remove(filePath);
        } else {
            QString alt = QString::fromStdString((content_root_ / "catalog" / "objects" / (id.toStdString() + ".json")).string());
            if (QFile::exists(alt)) {
                removed = QFile::remove(alt);
            }
        }

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
    QString relPath = QString::fromStdString((content_root_ / "catalog" / "relations" / "relations.json").string());
    QDir().mkpath(QFileInfo(relPath).absolutePath());

    QJsonArray relArr;
    if (QFile::exists(relPath)) {
        QFile f(relPath);
        if (f.open(QIODevice::ReadOnly)) {
            auto d = QJsonDocument::fromJson(f.readAll());
            if (d.isArray()) relArr = d.array();
        }
    }

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

        QFile f(relPath);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            sendJsonResponse(socket, 500, "{\"error\":\"Falha ao salvar relations.json\"}");
            return;
        }
        f.write(QJsonDocument(relArr).toJson(QJsonDocument::Indented));
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
            QFile f(relPath);
            if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                f.write(QJsonDocument(newArr).toJson(QJsonDocument::Indented));
            }
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
    QString recPath = QString::fromStdString((content_root_ / "catalog" / "recipes" / "recipes.json").string());
    QDir().mkpath(QFileInfo(recPath).absolutePath());

    QJsonArray recArr;
    if (QFile::exists(recPath)) {
        QFile f(recPath);
        if (f.open(QIODevice::ReadOnly)) {
            auto d = QJsonDocument::fromJson(f.readAll());
            if (d.isArray()) recArr = d.array();
        }
    }

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

        QFile f(recPath);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            sendJsonResponse(socket, 500, "{\"error\":\"Falha ao salvar recipes.json\"}");
            return;
        }
        f.write(QJsonDocument(recArr).toJson(QJsonDocument::Indented));
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
            QFile f(recPath);
            if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                f.write(QJsonDocument(newArr).toJson(QJsonDocument::Indented));
            }
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

        QJsonObject resp;
        resp[QStringLiteral("kiosk_online")] = kiosk_online;
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

    // API: GET /api/catalog
    if (method == QStringLiteral("GET") && path == QStringLiteral("/api/catalog")) {
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
            aObj[QStringLiteral("type")] = QString::fromStdString(std::string(content::to_string(atom->type)));
            aObj[QStringLiteral("subtype")] = QString::fromStdString(atom->subtype);
            aObj[QStringLiteral("canonical_name")] = QString::fromStdString(atom->subject.canonical_name);
            aObj[QStringLiteral("scientific_name")] = QString::fromStdString(atom->subject.scientific_name);
            aObj[QStringLiteral("type_label")] = QString::fromStdString(atom->subject.type_label);

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

    // API: POST /api/validate
    if (method == QStringLiteral("POST") && path == QStringLiteral("/api/validate")) {
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
