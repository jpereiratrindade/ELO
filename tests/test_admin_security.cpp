#include "admin_http_server.hpp"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QTemporaryDir>
#include <QTcpSocket>

#include <cstdlib>
#include <iostream>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion FAILED: " << msg << " (" << #cond << ")\n"; \
            std::abort(); \
        } \
    } while (0)

namespace {

QByteArray request(quint16 port, const QByteArray& raw) {
    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, port);
    TEST_ASSERT(socket.waitForConnected(1000), "Client connects to loopback admin server");
    socket.write(raw);
    socket.flush();

    QByteArray response;
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 3000 && socket.state() != QAbstractSocket::UnconnectedState) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        if (socket.waitForReadyRead(20)) response.append(socket.readAll());
    }
    response.append(socket.readAll());
    return response;
}

} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QTemporaryDir root;
    QTemporaryDir web;
    TEST_ASSERT(root.isValid() && web.isValid(), "Temporary roots are available");
    QFile index(web.filePath(QStringLiteral("index.html")));
    TEST_ASSERT(index.open(QIODevice::WriteOnly), "Test index can be created");
    index.write("ok");
    index.close();

    elo::admin::AdminHttpServer closedByDefault(
        root.path().toStdString(), web.path().toStdString(), QByteArray{}, 0);
    TEST_ASSERT(!closedByDefault.start(), "Missing token fails closed");

    const QByteArray token("0123456789abcdef0123456789abcdef");
    elo::admin::AdminHttpServer server(
        root.path().toStdString(), web.path().toStdString(), token, 0);
    TEST_ASSERT(server.start(), "Configured admin server starts");
    TEST_ASSERT(server.listenAddress().isLoopback(), "Admin server binds only to loopback");

    const auto denied = request(server.port(),
        "POST /api/kiosk/advance HTTP/1.1\r\nHost: localhost\r\nContent-Length: 0\r\n\r\n");
    TEST_ASSERT(denied.startsWith("HTTP/1.1 401"), "Unauthenticated mutation is rejected");
    TEST_ASSERT(!denied.contains("Access-Control-Allow-Origin"), "Wildcard CORS is absent");

    const auto removedClaim = request(server.port(),
        "GET /api/biometric/spec HTTP/1.1\r\nHost: localhost\r\nAuthorization: Bearer "
        + token + "\r\n\r\n");
    TEST_ASSERT(removedClaim.startsWith("HTTP/1.1 404"),
                "Unsupported biometric guarantees are not exposed");

    const auto traversal = request(server.port(),
        "GET /assets/../../etc/passwd HTTP/1.1\r\nHost: localhost\r\n\r\n");
    TEST_ASSERT(traversal.startsWith("HTTP/1.1 404"), "Asset traversal is denied");

    std::cout << "Admin containment G0 tests passed.\n";
    return 0;
}
