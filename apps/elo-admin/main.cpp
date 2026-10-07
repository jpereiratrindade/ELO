#include "admin_http_server.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QProcessEnvironment>
#include <iostream>

namespace {

std::filesystem::path resolve_web_root() {
    auto env_root = qEnvironmentVariable("ELO_WEB_ROOT");
    if (!env_root.isEmpty() && QDir(env_root).exists()) {
        return std::filesystem::canonical(env_root.toStdString());
    }

    QString appDir = QCoreApplication::applicationDirPath();
    QString home = QDir::homePath();

    std::vector<QString> candidates = {
        QStringLiteral("/usr/local/share/elo/admin"),
        QStringLiteral("/usr/local/share/elo/web/admin"),
        QStringLiteral("/usr/share/elo/admin"),
        QStringLiteral("/usr/share/elo/web/admin"),
        appDir + QStringLiteral("/../share/elo/admin"),
        appDir + QStringLiteral("/../share/elo/web/admin"),
        QStringLiteral("web/admin"),
        QStringLiteral("../web/admin"),
        QStringLiteral("../../web/admin"),
        home + QStringLiteral("/ELO/web/admin"),
        home + QStringLiteral("/elo/web/admin")
    };

    for (const auto& candidate : candidates) {
        if (QDir(candidate).exists() && QFile::exists(QDir(candidate).filePath(QStringLiteral("index.html")))) {
            return std::filesystem::canonical(candidate.toStdString());
        }
    }

    // Fallback: check directories even if index.html check is loose
    for (const auto& candidate : candidates) {
        if (QDir(candidate).exists()) {
            return std::filesystem::canonical(candidate.toStdString());
        }
    }

    return "/usr/local/share/elo/admin";
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
