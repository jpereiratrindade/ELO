#include "elo/system/control_socket.hpp"

#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <iostream>
#ifdef __linux__
#include <unistd.h>
#endif

namespace elo::system {

QString resolve_control_socket_path() {
    auto env = QProcessEnvironment::systemEnvironment();
    QString custom = env.value(QStringLiteral("ELO_CONTROL_SOCKET"));
    if (!custom.isEmpty()) {
        return custom;
    }

    // 1. Prefer user runtime directory (/run/user/<uid>/elo-control.sock)
    QString xdg = env.value(QStringLiteral("XDG_RUNTIME_DIR"));
    if (xdg.isEmpty()) {
#ifdef __linux__
        QString defaultXdg = QStringLiteral("/run/user/") + QString::number(getuid());
        if (QDir(defaultXdg).exists()) {
            xdg = defaultXdg;
        }
#endif
    }
    if (!xdg.isEmpty() && QDir(xdg).exists()) {
        return QDir(xdg).filePath(QStringLiteral("elo-control.sock"));
    }

    // 2. Try /run/elo (only if writable)
    QFileInfo runElo(QStringLiteral("/run/elo"));
    if (runElo.exists() && runElo.isWritable()) {
        return QStringLiteral("/run/elo/control.sock");
    }

    // 3. Fallback to /tmp with user-specific socket name
#ifdef __linux__
    return QStringLiteral("/tmp/elo-control-") + QString::number(getuid()) + QStringLiteral(".sock");
#else
    return QStringLiteral("/tmp/elo-control.sock");
#endif
}

ControlServer::ControlServer(const QString& socket_path, QObject* parent)
    : QObject(parent),
      socket_path_(socket_path.isEmpty() ? resolve_control_socket_path() : socket_path),
      server_(std::make_unique<QLocalServer>(this)) {
    connect(server_.get(), &QLocalServer::newConnection, this, &ControlServer::onNewConnection);
}

ControlServer::~ControlServer() {
    stop();
}

bool ControlServer::start() {
    stop();

    // Ensure parent directory exists
    QFileInfo fi(socket_path_);
    QDir().mkpath(fi.absolutePath());

    // Remove stale socket if it exists
    QLocalServer::removeServer(socket_path_);

    if (!server_->listen(socket_path_)) {
        std::cerr << "[ELO][ipc] Failed to listen on control socket "
                  << socket_path_.toStdString() << ": "
                  << server_->errorString().toStdString() << '\n';
        return false;
    }

    std::cout << "[ELO][ipc] Control plane listening on "
              << socket_path_.toStdString() << '\n';
    return true;
}

void ControlServer::stop() {
    if (server_ && server_->isListening()) {
        server_->close();
        QLocalServer::removeServer(socket_path_);
    }
}

bool ControlServer::isListening() const noexcept {
    return server_ && server_->isListening();
}

void ControlServer::onNewConnection() {
    while (server_->hasPendingConnections()) {
        auto* socket = server_->nextPendingConnection();
        connect(socket, &QLocalSocket::readyRead, this, &ControlServer::onReadyRead);
        connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);
    }
}

void ControlServer::onReadyRead() {
    auto* socket = qobject_cast<QLocalSocket*>(sender());
    if (!socket) return;

    while (socket->canReadLine() || socket->bytesAvailable() > 0) {
        QString line = QString::fromUtf8(socket->readLine()).trimmed();
        if (line.isEmpty() && socket->bytesAvailable() > 0) {
            line = QString::fromUtf8(socket->readAll()).trimmed();
        }
        if (line.isEmpty()) break;
        emit commandReceived(line);

        if (line == QStringLiteral("CONTENT_RELOAD")) {
            emit reloadRequested();
            socket->write("OK RELOAD_ACK\n");
        } else if (line.startsWith(QStringLiteral("SHOW_ATOM "))) {
            QString atomId = line.mid(10).trimmed();
            emit showAtomRequested(atomId);
            socket->write("OK SHOW_ACK\n");
        } else if (line == QStringLiteral("ADVANCE_CONTENT")) {
            emit advanceRequested();
            socket->write("OK ADVANCE_ACK\n");
        } else if (line == QStringLiteral("SHUTDOWN") || line == QStringLiteral("QUIT") || line == QStringLiteral("STOP")) {
            socket->write("OK SHUTDOWN_ACK\n");
            socket->flush();
            emit shutdownRequested();
        } else if (line.startsWith(QStringLiteral("CONTENT_PUBLISHED"))) {
            auto parts = line.split(' ');
            QString bundleId = parts.size() > 1 ? parts[1] : QString();
            QString hash = parts.size() > 2 ? parts[2] : QString();
            emit bundlePublished(bundleId, hash);
            emit reloadRequested();
            socket->write("OK PUBLISHED_ACK\n");
        } else if (line == QStringLiteral("PING")) {
            socket->write("PONG\n");
        } else if (line == QStringLiteral("STATUS_REQUEST")) {
            if (status_provider_) {
                QString status = status_provider_();
                if (!status.endsWith('\n')) status.append('\n');
                socket->write(status.toUtf8());
            } else {
                socket->write("STATUS_OK service=elo-kiosk\n");
            }
        } else {
            socket->write("UNKNOWN_COMMAND\n");
        }
        socket->flush();
    }
}

ControlClient::ControlClient(const QString& socket_path)
    : socket_path_(socket_path.isEmpty() ? resolve_control_socket_path() : socket_path) {}

bool ControlClient::ping(int timeout_ms) {
    auto reply = send_command(QStringLiteral("PING"), timeout_ms);
    return reply.trimmed() == QStringLiteral("PONG");
}

bool ControlClient::send_reload(int timeout_ms) {
    auto reply = send_command(QStringLiteral("CONTENT_RELOAD"), timeout_ms);
    return reply.contains(QStringLiteral("OK"));
}

bool ControlClient::notify_published(const QString& bundle_id, const QString& hash, int timeout_ms) {
    auto reply = send_command(QString("CONTENT_PUBLISHED %1 %2").arg(bundle_id, hash), timeout_ms);
    return reply.contains(QStringLiteral("OK"));
}

bool ControlClient::show_atom(const QString& atom_id, int timeout_ms) {
    auto reply = send_command(QStringLiteral("SHOW_ATOM %1").arg(atom_id), timeout_ms);
    return reply.contains(QStringLiteral("OK"));
}

bool ControlClient::advance_content(int timeout_ms) {
    auto reply = send_command(QStringLiteral("ADVANCE_CONTENT"), timeout_ms);
    return reply.contains(QStringLiteral("OK"));
}

bool ControlClient::shutdown_kiosk(int timeout_ms) {
    auto reply = send_command(QStringLiteral("SHUTDOWN"), timeout_ms);
    return reply.contains(QStringLiteral("OK"));
}

QString ControlClient::send_command(const QString& command, int timeout_ms) {
    QLocalSocket socket;
    socket.connectToServer(socket_path_);
    if (!socket.waitForConnected(timeout_ms)) {
        return QString();
    }

    QByteArray msg = command.toUtf8() + "\n";
    socket.write(msg);
    socket.flush();
    if (socket.bytesToWrite() > 0) {
        socket.waitForBytesWritten(timeout_ms);
    }

    if (!socket.waitForReadyRead(timeout_ms)) {
        std::cerr << "[client error] waitForReadyRead failed: " << socket.errorString().toStdString() << "\n";
        return QString();
    }

    auto reply = QString::fromUtf8(socket.readAll()).trimmed();
    return reply;
}

} // namespace elo::system
