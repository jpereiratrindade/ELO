#include "admin_http_server.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QProcessEnvironment>
#include <iostream>

namespace {

std::filesystem::path resolve_web_root() {
    for (const auto& candidate : {"web/admin", "../web/admin", "../../web/admin", "/usr/share/elo/web/admin"}) {
        if (QDir(candidate).exists()) {
            return std::filesystem::canonical(candidate);
        }
    }
    return "web/admin";
}

} // namespace

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("ELO Content Studio");
    app.setOrganizationName("ELO Project");

    quint16 port = 8080;
    auto portEnv = qEnvironmentVariable("ELO_ADMIN_PORT");
    if (!portEnv.isEmpty()) {
        bool ok = false;
        quint16 p = portEnv.toUShort(&ok);
        if (ok && p > 0) port = p;
    }

    auto content_root = elo::content::resolve_system_content_dir(true);
    auto web_root = resolve_web_root();

    std::cout << "========================================================\n"
              << "  ELO Content Studio — Local Sovereign Administration\n"
              << "  ELO-ADMIN-001 & ELO-PUBLISHING-001\n"
              << "========================================================\n"
              << "  Content Root: " << content_root << '\n'
              << "  Web Studio:   " << web_root << '\n'
              << "  Control IPC:  " << elo::system::resolve_control_socket_path().toStdString() << '\n'
              << "  Studio URL:   http://localhost:" << port << '\n'
              << "========================================================\n";

    elo::admin::AdminHttpServer server(content_root, web_root, port);
    if (!server.start()) {
        std::cerr << "[ELO][admin] Fatal error: failed to launch studio server.\n";
        return 1;
    }

    return app.exec();
}
