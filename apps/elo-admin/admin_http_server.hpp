#pragma once

#include "elo/content/content_bundle.hpp"
#include "elo/system/control_socket.hpp"

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QTcpServer>
#include <QTcpSocket>

#include <filesystem>
#include <memory>

namespace elo::admin {

class AdminHttpServer : public QObject {
    Q_OBJECT
public:
    explicit AdminHttpServer(
        std::filesystem::path content_root,
        std::filesystem::path web_root,
        quint16 port = 8080,
        QObject* parent = nullptr);
    ~AdminHttpServer() override;

    [[nodiscard]] bool start();
    void stop();
    [[nodiscard]] quint16 port() const noexcept { return port_; }

private slots:
    void onNewConnection();
    void onClientReadyRead();

private:
    void handleHttpRequest(QTcpSocket* socket, const QByteArray& requestData);
    void sendResponse(QTcpSocket* socket, int statusCode, const QString& contentType, const QByteArray& body);
    void sendJsonResponse(QTcpSocket* socket, int statusCode, const QByteArray& jsonBytes);

    std::filesystem::path content_root_;
    std::filesystem::path web_root_;
    quint16 port_{8080};
    std::unique_ptr<QTcpServer> tcp_server_;
    content::BundlePublisher publisher_;
};

} // namespace elo::admin
