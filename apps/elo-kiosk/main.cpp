#include "camera_image_provider.hpp"

#include "elo/content/content_bundle.hpp"
#include "elo/experience/experience_engine.hpp"
#include "elo/perception/camera_catalog.hpp"
#include "elo/perception/vision_service.hpp"
#include "elo/storage/sqlite_storage.hpp"
#include "elo/system/control_socket.hpp"
#include "elo/ui/kiosk_presentation_model.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QString>
#include <QStandardPaths>

#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>

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
        if (std::filesystem::exists(base_dir / "catalog")) {
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
    auto presentation_model = std::make_unique<elo::ui::KioskPresentationModel>(engine);

    auto control_server = std::make_unique<elo::system::ControlServer>();
    if (control_server->start()) {
        QObject::connect(control_server.get(), &elo::system::ControlServer::reloadRequested,
            [content_catalog, system_content, resolve_active_catalog, &presentation_model]() {
                std::cout << "[ELO][kiosk] Hot reload signal received from control plane. Reloading catalog...\n";
                auto reload_path = resolve_active_catalog(system_content);
                if (reload_path.empty()) reload_path = system_content / "catalog";

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
