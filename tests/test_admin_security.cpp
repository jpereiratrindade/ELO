#include "admin_http_server.hpp"
#include "test_content_fixtures.hpp"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTcpSocket>

#include <cstdlib>
#include <filesystem>
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

    // Seed root with complete fixture catalog
    const auto fixture_dir = elo::test::create_test_content_fixture();
    std::filesystem::copy(fixture_dir, std::filesystem::path(root.path().toStdString()),
                          std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);

    QFile index(web.filePath(QStringLiteral("index.html")));
    TEST_ASSERT(index.open(QIODevice::WriteOnly), "Test index can be created");
    index.write("ok");
    index.close();

    elo::admin::AdminHttpServer closedByDefault(
        root.path().toStdString(), web.path().toStdString(), QByteArray{}, 0);
    TEST_ASSERT(!closedByDefault.start(), "Missing token fails closed");

    elo::admin::AdminHttpServer shortToken(
        root.path().toStdString(), web.path().toStdString(), QByteArray("short_token_under_32_bytes"), 0);
    TEST_ASSERT(!shortToken.start(), "Token under 32 bytes fails closed");

    const QByteArray token("0123456789abcdef0123456789abcdef");
    elo::admin::AdminHttpServer server(
        root.path().toStdString(), web.path().toStdString(), token, 0);
    TEST_ASSERT(server.start(), "Configured admin server starts");
    TEST_ASSERT(server.listenAddress().isLoopback(), "Admin server binds only to loopback");

    // 1. Unauthenticated mutation rejection and lack of wildcard CORS
    const auto denied = request(server.port(),
        "POST /api/kiosk/advance HTTP/1.1\r\nHost: localhost\r\nContent-Length: 0\r\n\r\n");
    TEST_ASSERT(denied.startsWith("HTTP/1.1 401"), "Unauthenticated mutation is rejected");
    TEST_ASSERT(!denied.contains("Access-Control-Allow-Origin"), "Wildcard CORS is absent");
    TEST_ASSERT(denied.contains("Content-Security-Policy"), "CSP header is present on responses");
    TEST_ASSERT(denied.contains("X-Frame-Options: DENY"), "X-Frame-Options header is present");
    TEST_ASSERT(denied.contains("X-Content-Type-Options: nosniff"), "nosniff header is present");

    // 2. Unsupported biometric endpoint
    const auto removedClaim = request(server.port(),
        "GET /api/biometric/spec HTTP/1.1\r\nHost: localhost\r\nAuthorization: Bearer "
        + token + "\r\n\r\n");
    TEST_ASSERT(removedClaim.startsWith("HTTP/1.1 404"),
                "Unsupported biometric guarantees are not exposed");

    // 3. Path traversal protection
    const auto traversal = request(server.port(),
        "GET /assets/../../etc/passwd HTTP/1.1\r\nHost: localhost\r\n\r\n");
    TEST_ASSERT(traversal.startsWith("HTTP/1.1 404"), "Asset traversal is denied");

    // 4. Header limit enforcement when terminator is already present in buffer (> 16 KiB)
    const QByteArray oversizedHeader = "GET / HTTP/1.1\r\nHost: localhost\r\nX-Padding: "
                                       + QByteArray(17 * 1024, 'X') + "\r\n\r\n";
    const auto headerTooLarge = request(server.port(), oversizedHeader);
    TEST_ASSERT(headerTooLarge.startsWith("HTTP/1.1 413"),
                "Headers exceeding 16 KiB are rejected with 413 even when terminator is present");

    // 5. Duplicate Content-Length header rejection (RFC 7230 request smuggling prevention)
    const auto duplicateCL = request(server.port(),
        "POST /api/publish HTTP/1.1\r\nHost: localhost\r\nAuthorization: Bearer " + token +
        "\r\nContent-Length: 0\r\nContent-Length: 0\r\n\r\n");
    TEST_ASSERT(duplicateCL.startsWith("HTTP/1.1 400"),
                "Duplicate Content-Length header is rejected with 400 Bad Request");

    // 6. Invalid non-numeric Content-Length
    const auto invalidCL = request(server.port(),
        "POST /api/publish HTTP/1.1\r\nHost: localhost\r\nAuthorization: Bearer " + token +
        "\r\nContent-Length: invalid_len\r\n\r\n");
    TEST_ASSERT(invalidCL.startsWith("HTTP/1.1 400"),
                "Non-numeric Content-Length is rejected with 400 Bad Request");

    // 7. Negative Content-Length
    const auto negativeCL = request(server.port(),
        "POST /api/publish HTTP/1.1\r\nHost: localhost\r\nAuthorization: Bearer " + token +
        "\r\nContent-Length: -42\r\n\r\n");
    TEST_ASSERT(negativeCL.startsWith("HTTP/1.1 400"),
                "Negative Content-Length is rejected with 400 Bad Request");

    // 8. Excessive Content-Length (> 16 MiB)
    const auto excessiveCL = request(server.port(),
        "POST /api/publish HTTP/1.1\r\nHost: localhost\r\nAuthorization: Bearer " + token +
        "\r\nContent-Length: 25000000\r\n\r\n");
    TEST_ASSERT(excessiveCL.startsWith("HTTP/1.1 413"),
                "Content-Length exceeding 16 MiB is rejected with 413 Payload Too Large");

    // 9. Incomplete connection handling (early client disconnection without terminator)
    {
        QTcpSocket socket;
        socket.connectToHost(QHostAddress::LocalHost, server.port());
        TEST_ASSERT(socket.waitForConnected(1000), "Socket connects for incomplete test");
        socket.write("GET /api/status HTTP/1.1\r\nHost: local");
        socket.flush();
        socket.disconnectFromHost();
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        // Server continues operating normally
    }

    // 10. Authenticated administrative workflow with untrusted HTML content
    const QJsonObject untrustedAtom{
        {QStringLiteral("content_id"), QStringLiteral("atom_untrusted_html_test")},
        {QStringLiteral("schema_version"), QStringLiteral("0.1")},
        {QStringLiteral("type"), QStringLiteral("entity")},
        {QStringLiteral("title"), QStringLiteral("<script>alert('xss_title')</script>")},
        {QStringLiteral("canonical_name"), QStringLiteral("Espécie Teste <img src=x onerror=alert(1)>")},
        {QStringLiteral("subtitle"), QStringLiteral("Subtítulo com <b onmouseover=evil()>tag</b>")},
        {QStringLiteral("canonical_facts"), QJsonArray{
            QJsonObject{
                {QStringLiteral("fact_id"), QStringLiteral("fact_injection_01")},
                {QStringLiteral("statement"), QStringLiteral("</textarea><script>alert('fact')</script>")},
                {QStringLiteral("confidence"), QStringLiteral("high")},
                {QStringLiteral("source_ids"), QJsonArray{QStringLiteral("source_icmbio_2018")}}
            }
        }},
        {QStringLiteral("metadata"), QJsonObject{
            {QStringLiteral("autor_html"), QStringLiteral("<a href=\"javascript:evil()\">Clique</a>")},
            {QStringLiteral("nota"), QStringLiteral("Normal <em>texto</em>")}
        }}
    };

    const QByteArray atomPayload = QJsonDocument(untrustedAtom).toJson(QJsonDocument::Compact);
    const QByteArray postAtomReq =
        "POST /api/atoms HTTP/1.1\r\nHost: localhost\r\nAuthorization: Bearer " + token +
        "\r\nContent-Type: application/json\r\nContent-Length: " + QByteArray::number(atomPayload.size()) +
        "\r\n\r\n" + atomPayload;

    const auto atomResp = request(server.port(), postAtomReq);
    TEST_ASSERT(atomResp.startsWith("HTTP/1.1 201 Created"),
                "Authenticated atom creation with untrusted HTML succeeds with 201 Created");
    TEST_ASSERT(atomResp.contains("atom_untrusted_html_test"),
                "Created atom ID is returned in response");
    TEST_ASSERT(atomResp.contains("Content-Type: application/json; charset=utf-8"),
                "JSON response has strict Content-Type charset");

    // 11. Retrieve catalog and verify content is persisted and returned safely as JSON
    const auto catalogResp = request(server.port(),
        "GET /api/catalog HTTP/1.1\r\nHost: localhost\r\nAuthorization: Bearer " + token + "\r\n\r\n");
    TEST_ASSERT(catalogResp.startsWith("HTTP/1.1 200 OK"), "Catalog is accessible with authentication");
    TEST_ASSERT(catalogResp.contains("atom_untrusted_html_test"), "Untrusted atom is stored and retrieved in catalog");
    TEST_ASSERT(catalogResp.contains("Content-Security-Policy"), "Catalog response contains CSP");
    TEST_ASSERT(catalogResp.contains("X-Content-Type-Options: nosniff"), "Catalog response contains nosniff");

    // 12. Authenticated publication and persistence verification
    const auto publishResp = request(server.port(),
        "POST /api/publish HTTP/1.1\r\nHost: localhost\r\nAuthorization: Bearer " + token +
        "\r\nContent-Length: 0\r\n\r\n");
    TEST_ASSERT(publishResp.startsWith("HTTP/1.1 200 OK"), "Publishing succeeds with 200 OK");
    TEST_ASSERT(publishResp.contains("\"success\":true"), "Publish response indicates success");
    TEST_ASSERT(publishResp.contains("\"content_hash\":\"sha256:"), "Publish generates SHA-256 hash");

    // Verify published bundle exists on filesystem
    const std::filesystem::path current_link = std::filesystem::path(root.path().toStdString()) / "current";
    TEST_ASSERT(std::filesystem::exists(current_link), "Published 'current' bundle link exists on filesystem");

    std::cout << "Admin containment G0 tests passed successfully with full security hardening.\n";
    return 0;
}
