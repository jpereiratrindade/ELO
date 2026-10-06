#include "admin_http_server.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <iostream>

namespace elo::admin {

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
        connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    }
}

void AdminHttpServer::onClientReadyRead() {
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    QByteArray requestData = socket->readAll();
    handleHttpRequest(socket, requestData);
}

void AdminHttpServer::sendResponse(QTcpSocket* socket, int statusCode, const QString& contentType, const QByteArray& body) {
    QString statusText = (statusCode == 200) ? QStringLiteral("OK")
                       : (statusCode == 404) ? QStringLiteral("Not Found")
                       : (statusCode == 400) ? QStringLiteral("Bad Request")
                                             : QStringLiteral("Internal Server Error");

    QByteArray response;
    response.append(QString("HTTP/1.1 %1 %2\r\n").arg(statusCode).arg(statusText).toUtf8());
    response.append(QString("Content-Type: %1\r\n").arg(contentType).toUtf8());
    response.append(QString("Content-Length: %1\r\n").arg(body.size()).toUtf8());
    response.append("Access-Control-Allow-Origin: *\r\n");
    response.append("Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n");
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

    // Extract body if POST
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

    // API: GET /api/status
    if (method == QStringLiteral("GET") && path == QStringLiteral("/api/status")) {
        system::ControlClient client;
        bool kiosk_online = client.ping(300);

        QJsonObject resp;
        resp[QStringLiteral("kiosk_online")] = kiosk_online;
        resp[QStringLiteral("control_socket")] = system::resolve_control_socket_path();
        resp[QStringLiteral("editorial_state")] = QStringLiteral("active");

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
        auto cur_res = publisher_.current_symlink();
        std::filesystem::path load_path = std::filesystem::exists(cur_res) ? cur_res : content_root_;

        content::ContentBundle bundle(load_path);
        auto cat_res = bundle.load_catalog();
        if (!cat_res) {
            QJsonObject err;
            err[QStringLiteral("error")] = QString::fromStdString(cat_res.error().to_string());
            sendJsonResponse(socket, 500, QJsonDocument(err).toJson(QJsonDocument::Compact));
            return;
        }

        QJsonObject resp;
        auto m_res = bundle.load_manifest();
        if (m_res) {
            QJsonObject mObj;
            mObj[QStringLiteral("bundle_id")] = QString::fromStdString(m_res->bundle_id);
            mObj[QStringLiteral("version")] = QString::fromStdString(m_res->version);
            mObj[QStringLiteral("title")] = QString::fromStdString(m_res->title);
            mObj[QStringLiteral("default_theme")] = QString::fromStdString(m_res->default_theme);
            resp[QStringLiteral("manifest")] = mObj;
        }

        QJsonArray atomsArr;
        for (const auto* atom : cat_res->all_atoms()) {
            QJsonObject aObj;
            aObj[QStringLiteral("content_id")] = QString::fromStdString(atom->content_id);
            aObj[QStringLiteral("title")] = QString::fromStdString(atom->title);
            aObj[QStringLiteral("type")] = QString::fromStdString(std::string(content::to_string(atom->type)));
            aObj[QStringLiteral("canonical_name")] = QString::fromStdString(atom->subject.canonical_name);
            aObj[QStringLiteral("scientific_name")] = QString::fromStdString(atom->subject.scientific_name);
            aObj[QStringLiteral("type_label")] = QString::fromStdString(atom->subject.type_label);

            QJsonArray factsArr;
            for (const auto& fact : atom->canonical_facts) {
                QJsonObject fObj;
                fObj[QStringLiteral("fact_id")] = QString::fromStdString(fact.fact_id);
                fObj[QStringLiteral("statement")] = QString::fromStdString(fact.statement);
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
        for (const auto* rec : cat_res->all_recipes()) {
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
