#pragma once

#include <QLocalServer>
#include <QLocalSocket>
#include <QObject>
#include <QString>

#include <memory>
#include <string>

namespace elo::system {

/// @brief Determines canonical control socket path with safe runtime fallbacks
[[nodiscard]] QString resolve_control_socket_path();

/// @brief ELO-ADMIN-001 Section 5: Control Plane IPC Server (hosted in elo-kiosk)
class ControlServer : public QObject {
    Q_OBJECT
public:
    explicit ControlServer(const QString& socket_path = QString(), QObject* parent = nullptr);
    ~ControlServer() override;

    [[nodiscard]] bool start();
    void stop();
    [[nodiscard]] QString socketPath() const noexcept { return socket_path_; }
    [[nodiscard]] bool isListening() const noexcept;

signals:
    void reloadRequested();
    void bundlePublished(const QString& bundleId, const QString& hash);
    void commandReceived(const QString& command);

private slots:
    void onNewConnection();
    void onReadyRead();

private:
    QString socket_path_;
    std::unique_ptr<QLocalServer> server_;
};

/// @brief Control Plane IPC Client (used by elo-admin to trigger kiosk reloads)
class ControlClient {
public:
    explicit ControlClient(const QString& socket_path = QString());

    [[nodiscard]] bool ping(int timeout_ms = 1000);
    [[nodiscard]] bool send_reload(int timeout_ms = 1000);
    [[nodiscard]] bool notify_published(const QString& bundle_id, const QString& hash, int timeout_ms = 1000);
    [[nodiscard]] QString send_command(const QString& command, int timeout_ms = 1000);

private:
    QString socket_path_;
};

} // namespace elo::system
