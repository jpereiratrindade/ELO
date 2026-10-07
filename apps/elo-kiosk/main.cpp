#include "camera_image_provider.hpp"

#include "elo/content/content_bundle.hpp"
#include "elo/content/content_database.hpp"
#include "elo/experience/experience_engine.hpp"
#include "elo/perception/camera_catalog.hpp"
#include "elo/perception/vision_service.hpp"
#include "elo/storage/sqlite_storage.hpp"
#include "elo/system/control_socket.hpp"
#include "elo/ui/kiosk_presentation_model.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QGuiApplication>
#include <QJsonObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QString>
#include <QStandardPaths>

#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#ifdef __linux__
#include <unistd.h>
#endif

namespace {

std::optional<elo::perception::CameraDevice> configured_totem_camera(
    const elo::perception::CameraDiscoveryReport& report) {
    for (const auto& warning : report.warnings) {
        std::cerr << "[ELO][camera] warning: " << warning << '\n';
    }
    if (report.devices.empty()) {
        return std::nullopt;
    }

    const auto configured_id = qEnvironmentVariable("ELO_CAMERA_ID");
    if (!configured_id.isEmpty()) {
        auto selected = elo::perception::select_camera(
            report.devices, configured_id.toStdString());
        if (selected) {
            return *selected;
        }
        std::cerr << "[ELO][camera] configured ELO_CAMERA_ID was not found: "
                  << configured_id.toStdString() << '\n';
        return std::nullopt;
    }

    // A kiosk has a fixed physical realization. Stable ids are sorted by the
    // discovery backend, so the first device is deterministic for that unit.
    return report.devices.front();
}

} // namespace

int main(int argc, char* argv[]) {
    // Intelligent platform backend detection for embedded appliances (e.g. Raspberry Pi 5 / TTY console)
    bool has_platform_arg = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-platform" || std::string(argv[i]) == "--platform") {
            has_platform_arg = true;
            break;
        }
    }

    if (!has_platform_arg && qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        // 1. Resolve XDG_RUNTIME_DIR if missing
        QString runtimeDir = qEnvironmentVariable("XDG_RUNTIME_DIR");
        if (runtimeDir.isEmpty()) {
#ifdef __linux__
            QString defaultRuntime = QStringLiteral("/run/user/") + QString::number(getuid());
            if (QDir(defaultRuntime).exists()) {
                runtimeDir = defaultRuntime;
                qputenv("XDG_RUNTIME_DIR", runtimeDir.toUtf8());
            }
#endif
        }

        // 2. Discover Wayland session
        QString waylandEnv = qEnvironmentVariable("WAYLAND_DISPLAY");
        if (waylandEnv.isEmpty() && !runtimeDir.isEmpty()) {
            if (QFile::exists(runtimeDir + QStringLiteral("/wayland-0"))) {
                waylandEnv = QStringLiteral("wayland-0");
                qputenv("WAYLAND_DISPLAY", "wayland-0");
            } else if (QFile::exists(runtimeDir + QStringLiteral("/wayland-1"))) {
                waylandEnv = QStringLiteral("wayland-1");
                qputenv("WAYLAND_DISPLAY", "wayland-1");
            }
        }

        // 3. Discover X11 session
        QString displayEnv = qEnvironmentVariable("DISPLAY");
        if (displayEnv.isEmpty()) {
            if (QFile::exists(QStringLiteral("/tmp/.X11-unix/X0"))) {
                displayEnv = QStringLiteral(":0");
                qputenv("DISPLAY", ":0");
            } else if (QFile::exists(QStringLiteral("/tmp/.X11-unix/X1"))) {
                displayEnv = QStringLiteral(":1");
                qputenv("DISPLAY", ":1");
            }
        }

        if (!waylandEnv.isEmpty()) {
            std::cout << "[ELO] Wayland desktop session detected (" << waylandEnv.toStdString() << "). Selecting Wayland backend...\n";
            qputenv("QT_QPA_PLATFORM", "wayland");
        } else if (!displayEnv.isEmpty()) {
            std::cout << "[ELO] X11 desktop session detected (" << displayEnv.toStdString() << "). Selecting XCB backend...\n";
            qputenv("QT_QPA_PLATFORM", "xcb");
        } else {
            // Running directly from a Linux TTY / console / headless DRM framebuffer (e.g., Raspberry Pi 5 Appliance mode)
            if (std::filesystem::exists("/dev/dri/card0") || std::filesystem::exists("/dev/dri/card1")) {
                std::cout << "[ELO] Embedded TTY console detected. Selecting EGLFS (Direct KMS/DRM) backend...\n";
                qputenv("QT_QPA_PLATFORM", "eglfs");
                qputenv("QT_QPA_EGLFS_ALWAYS_SET_MODE", "1");
                qputenv("QT_QPA_EGLFS_KMS_ATOMIC", "1");
            } else if (std::filesystem::exists("/dev/fb0")) {
                std::cout << "[ELO] Selecting LinuxFB (framebuffer) backend...\n";
                qputenv("QT_QPA_PLATFORM", "linuxfb");
            } else {
                std::cout << "[ELO] Warning: No display server or framebuffer detected. Attempting default platform...\n";
            }
        }
    }

    QGuiApplication app(argc, argv);
    app.setApplicationName("ELO Kiosk");
    app.setOrganizationName("ELO Project");

    std::cout << "[ELO] Initializing autonomous offline kiosk...\n";

    const auto camera_report = elo::perception::discover_cameras();
    const auto selected_camera = configured_totem_camera(camera_report);
    if (selected_camera) {
        std::cout << "[ELO][camera] Fixed camera: " << selected_camera->display_name
                  << " (" << selected_camera->id << ", " << selected_camera->backend << ")\n";
    } else {
        std::cerr << "[ELO][camera] No fixed camera is available; vision is unavailable.\n";
    }

    auto data_directory = qEnvironmentVariable("ELO_DATA_DIR");
    if (data_directory.isEmpty()) {
        data_directory = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    }
    if (!QDir().mkpath(data_directory)) {
        std::cerr << "[ELO][storage] Cannot create local data directory: "
                  << data_directory.toStdString() << '\n';
        return 3;
    }
    const auto database_path = QDir(data_directory).filePath(QStringLiteral("elo-local.sqlite3"));
    auto stores_result = elo::storage::open_sqlite_stores(database_path.toStdString());
    if (!stores_result) {
        std::cerr << "[ELO][storage] " << stores_result.error().to_string() << '\n';
        return 3;
    }
    const auto stores = *stores_result;
    std::cout << "[ELO][storage] Offline database: " << database_path.toStdString() << '\n';

    elo::identity::EloId elo_system_id("elo://S01");
    constexpr std::uint64_t kernel_seed = 0x454c4f5f533031ULL; // "ELO_S01"

    auto content_catalog = std::make_shared<elo::content::ContentCatalog>();
    auto system_content = elo::content::resolve_system_content_dir(true);

    auto resolve_active_catalog = [](const std::filesystem::path& base_dir) -> std::filesystem::path {
        if (std::filesystem::exists(base_dir / "current" / "catalog")) {
            return base_dir / "current" / "catalog";
        }
        std::error_code ec;
        const auto bundles = base_dir / "bundles";
        const bool hasPublishedBundles = std::filesystem::exists(bundles, ec)
            && std::filesystem::directory_iterator(bundles, ec) != std::filesystem::directory_iterator{};
        if (!hasPublishedBundles && std::filesystem::exists(base_dir / "catalog")) {
            return base_dir / "catalog";
        }
        return {};
    };

    auto content_dir = QString::fromStdString(resolve_active_catalog(system_content).string());
    if (!content_dir.isEmpty()) {
        auto load_res = content_catalog->load_from_directory(content_dir.toStdString());
        if (load_res) {
            std::cout << "[ELO][content] Loaded decoupled content catalog: "
                      << content_catalog->atom_count() << " atoms, "
                      << content_catalog->relation_count() << " relations, "
                      << content_catalog->recipe_count() << " recipes.\n";
        } else {
            std::cerr << "[ELO][content] Failed to load content: " << load_res.error().to_string() << '\n';
        }
    }

    auto engine = std::make_shared<elo::experience::ExperienceEngine>(
        elo_system_id,
        kernel_seed,
        stores.biometric,
        stores.experience,
        stores.survey,
        nullptr,
        stores.jev_events,
        content_catalog);

    auto analytics_database = std::make_shared<elo::content::ContentDatabase>(
        system_content / "editorial.sqlite3");
    struct AnalyticsState {
        std::string active_atom;
        std::uint64_t active_since_ms{0};
    };
    auto analytics_state = std::make_shared<AnalyticsState>();
    engine->set_analytics_sink([analytics_database, analytics_state, content_catalog](const elo::judgment::JevEvent& event) {
        const auto& manifest = content_catalog->manifest();
        if (manifest.bundle_id.empty()) return;
        const QString applicationId = QString::fromStdString(manifest.bundle_id);
        const QString applicationVersion = QString::fromStdString(manifest.version);
        const qint64 hourBucket = static_cast<qint64>((event.timestamp_ms / 1000 / 3600) * 3600);
        const auto record = [&](QString type, QString target = {}, QString source = {}, double duration = 0.0) {
            analytics_database->record_analytics_event(QJsonObject{
                {"application_id", applicationId},
                {"application_version", applicationVersion},
                {"event_type", std::move(type)},
                {"target_atom_id", std::move(target)},
                {"source_atom_id", std::move(source)},
                {"duration_seconds", duration},
                {"hour_bucket", hourBucket}
            });
        };
        const auto flushDwell = [&]() {
            if (!analytics_state->active_atom.empty() && analytics_state->active_since_ms > 0
                && event.timestamp_ms >= analytics_state->active_since_ms) {
                record("dwell_tick", QString::fromStdString(analytics_state->active_atom), {},
                       static_cast<double>(event.timestamp_ms - analytics_state->active_since_ms) / 1000.0);
            }
        };
        if (event.event_name == "session.start") {
            analytics_state->active_atom.clear();
            analytics_state->active_since_ms = 0;
            record("session_start");
        } else if (event.event_name == "content.selected") {
            flushDwell();
            record("atom_view", QString::fromStdString(event.payload),
                   QString::fromStdString(analytics_state->active_atom));
            analytics_state->active_atom = event.payload;
            analytics_state->active_since_ms = event.timestamp_ms;
        } else if (event.event_name == "recipe.completed") {
            record("recipe_complete", QString::fromStdString(event.payload));
        } else if (event.event_name == "session.end") {
            flushDwell();
            analytics_state->active_atom.clear();
            analytics_state->active_since_ms = 0;
        }
    });
    auto presentation_model = std::make_unique<elo::ui::KioskPresentationModel>(engine);
    presentation_model->selectContextualContent();

    auto control_server = std::make_unique<elo::system::ControlServer>();
    if (control_server->start()) {
        control_server->setStatusProvider([&presentation_model]() {
            QString atomId = presentation_model->activeAtomId();
            QString title = presentation_model->contentTitle();
            title.replace(' ', '_');
            QString state = presentation_model->currentState();
            return QStringLiteral("STATUS_OK service=elo-kiosk active_atom_id=%1 active_atom_title=%2 state=%3")
                .arg(atomId, title, state);
        });

        QObject::connect(control_server.get(), &elo::system::ControlServer::showAtomRequested,
            [&presentation_model](const QString& atomId) {
                std::cout << "[ELO][kiosk] Remote show atom request received: " << atomId.toStdString() << '\n';
                presentation_model->selectAtomDirectly(atomId);
            });

        QObject::connect(control_server.get(), &elo::system::ControlServer::advanceRequested,
            [&presentation_model]() {
                std::cout << "[ELO][kiosk] Remote advance content request received\n";
                presentation_model->advanceContent();
            });

        QObject::connect(control_server.get(), &elo::system::ControlServer::contentDeactivated,
            [content_catalog, &presentation_model]() {
                std::cout << "[ELO][kiosk] Active content package deactivated\n";
                presentation_model->deactivateContent();
                content_catalog->clear();
            });

        QObject::connect(control_server.get(), &elo::system::ControlServer::reloadRequested,
            [content_catalog, system_content, resolve_active_catalog, &presentation_model]() {
                std::cout << "[ELO][kiosk] Hot reload signal received from control plane. Reloading catalog...\n";
                auto reload_path = resolve_active_catalog(system_content);
                if (reload_path.empty()) {
                    presentation_model->deactivateContent();
                    content_catalog->clear();
                    std::cout << "[ELO][kiosk] No active application; content unloaded.\n";
                    return;
                }

                auto reload_res = content_catalog->load_from_directory(reload_path.string());
                if (reload_res) {
                    std::cout << "[ELO][kiosk] Successfully reloaded content catalog: "
                              << content_catalog->atom_count() << " atoms, "
                              << content_catalog->relation_count() << " relations, "
                              << content_catalog->recipe_count() << " recipes from " << reload_path << ".\n";
                    presentation_model->selectContextualContent();
                } else {
                    std::cerr << "[ELO][kiosk] Hot reload failed: " << reload_res.error().to_string() << '\n';
                }
            });

        QObject::connect(control_server.get(), &elo::system::ControlServer::shutdownRequested,
            []() {
                std::cout << "[ELO][kiosk] Remote shutdown signal received. Terminating kiosk...\n";
                QCoreApplication::quit();
            });
    }

    QQmlApplicationEngine qml_engine;
    auto* image_provider = new CameraImageProvider();
    qml_engine.addImageProvider(QStringLiteral("camera"), image_provider);
    qml_engine.rootContext()->setContextProperty("kioskModel", presentation_model.get());
    qml_engine.rootContext()->setContextProperty("cameraAvailable", selected_camera.has_value());
    qml_engine.rootContext()->setContextProperty(
        "totemCameraName",
        selected_camera ? QString::fromStdString(selected_camera->display_name)
                        : QStringLiteral("INDISPONÍVEL"));

    std::unique_ptr<elo::perception::VisionService> vision;
    if (selected_camera) {
        vision = std::make_unique<elo::perception::VisionService>(
            *selected_camera,
            QStringLiteral(ELO_FACE_DETECTOR_MODEL_PATH),
            QStringLiteral(ELO_FACE_RECOGNIZER_MODEL_PATH));

        QObject::connect(
            vision.get(), &elo::perception::VisionService::frameReady,
            &app, [image_provider, model = presentation_model.get()](const QImage& frame) {
                image_provider->updateFrame(frame);
                model->onCameraFrameReady();
            });
        QObject::connect(
            vision.get(), &elo::perception::VisionService::facePresenceChanged,
            presentation_model.get(), &elo::ui::KioskPresentationModel::onFacePresenceChanged);
        QObject::connect(
            vision.get(), &elo::perception::VisionService::facePresenceChanged,
            &app, [](bool present) {
                std::cout << "[ELO][vision] face presence: "
                          << (present ? "detected" : "absent") << '\n';
            });
        QObject::connect(
            vision.get(), &elo::perception::VisionService::faceEmbeddingReady,
            presentation_model.get(), &elo::ui::KioskPresentationModel::onFaceEmbeddingReady);
        QObject::connect(
            vision.get(), &elo::perception::VisionService::statusChanged,
            presentation_model.get(), &elo::ui::KioskPresentationModel::onVisionStatusChanged);
        QObject::connect(
            vision.get(), &elo::perception::VisionService::failure,
            presentation_model.get(), &elo::ui::KioskPresentationModel::onVisionFailure);
        QObject::connect(
            vision.get(), &elo::perception::VisionService::failure,
            &app, [](const QString& message) {
                std::cerr << "[ELO][vision] failure: " << message.toStdString() << '\n';
            });
        QObject::connect(
            presentation_model.get(), &elo::ui::KioskPresentationModel::biometricAuthorizationChanged,
            vision.get(), &elo::perception::VisionService::setBiometricAuthorized);
        QObject::connect(
            presentation_model.get(), &elo::ui::KioskPresentationModel::recognitionVisualStateChanged,
            vision.get(), &elo::perception::VisionService::setRecognitionConfirmed);
    }

    const QUrl url(QStringLiteral("qrc:/Main.qml"));
    QObject::connect(
        &qml_engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [url](QObject* object, const QUrl& object_url) {
            if (object == nullptr && url == object_url) {
                std::cerr << "[ELO] Failed to load QML interface!\n";
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection);

    qml_engine.load(url);
    if (vision) {
        vision->start();
    }

    std::cout << "[ELO] Kiosk active; continuous face presence detection enabled.\n";
    return app.exec();
}
