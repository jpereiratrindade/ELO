#include "elo/experience/experience_engine.hpp"
#include "elo/perception/camera_catalog.hpp"
#include "elo/storage/in_memory_storage.hpp"
#include "elo/ui/kiosk_presentation_model.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace {

struct CommandLineOptions {
    bool show_help{false};
    bool list_cameras{false};
    bool choose_camera{false};
    bool require_camera{false};
    std::optional<std::string> camera_selector;
    std::optional<std::string> error;
};

CommandLineOptions parse_command_line(int argc, char* argv[]) {
    CommandLineOptions options;
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument(argv[index]);
        if (argument == "-h" || argument == "--help") {
            options.show_help = true;
        } else if (argument == "-l" || argument == "--list-cameras") {
            options.list_cameras = true;
        } else if (argument == "--choose-camera") {
            options.choose_camera = true;
        } else if (argument == "--require-camera") {
            options.require_camera = true;
        } else if (argument == "-c" || argument == "--camera") {
            if (index + 1 >= argc || std::string_view(argv[index + 1]).starts_with("--")) {
                options.error = "--camera requires an index, id, or device path";
                break;
            }
            options.camera_selector = argv[++index];
        } else if (argument.starts_with("--camera=")) {
            options.camera_selector = std::string(argument.substr(std::string_view("--camera=").size()));
        }
    }

    if (options.choose_camera && options.camera_selector) {
        options.error = "--choose-camera and --camera cannot be used together";
    } else if (options.camera_selector && options.camera_selector->empty()) {
        options.error = "--camera requires a non-empty selector";
    }
    return options;
}

void print_help(std::ostream& output) {
    output
        << "Usage: elo-kiosk [camera options] [Qt options]\n\n"
        << "Camera options:\n"
        << "  -l, --list-cameras          List cameras and exit\n"
        << "      --choose-camera         Show an interactive camera selection menu\n"
        << "  -c, --camera <selector>     Select by zero-based index, id, path, or unique name\n"
        << "      --require-camera        Refuse to start without a selected camera\n"
        << "  -h, --help                  Show this help and exit\n";
}

void print_cameras(std::ostream& output, const elo::perception::CameraDiscoveryReport& report) {
    if (report.devices.empty()) {
        output << "No camera detected.\n";
        return;
    }

    output << "Available cameras:\n";
    for (std::size_t index = 0; index < report.devices.size(); ++index) {
        const auto& camera = report.devices[index];
        output << "  [" << index << "] " << camera.display_name
               << "\n      id: " << camera.id
               << "\n      backend: " << camera.backend;
        if (camera.device_path != camera.id) {
            output << "\n      device: " << camera.device_path;
        }
        output << '\n';
    }
}

void print_discovery_warnings(const elo::perception::CameraDiscoveryReport& report) {
    for (const auto& warning : report.warnings) {
        std::cerr << "[ELO][camera] warning: " << warning << '\n';
    }
}

} // namespace

int main(int argc, char* argv[]) {
    const auto command_line = parse_command_line(argc, argv);
    if (command_line.error) {
        std::cerr << "[ELO] " << *command_line.error << "\n\n";
        print_help(std::cerr);
        return 2;
    }
    if (command_line.show_help) {
        print_help(std::cout);
        return 0;
    }

    const auto camera_report = elo::perception::discover_cameras();
    print_discovery_warnings(camera_report);
    if (command_line.list_cameras) {
        print_cameras(std::cout, camera_report);
        return 0;
    }

    std::optional<elo::perception::CameraDevice> selected_camera;
    if (command_line.choose_camera) {
        print_cameras(std::cout, camera_report);
        if (camera_report.devices.empty()) {
            std::cerr << "[ELO] Interactive camera selection requires at least one camera.\n";
            return 2;
        }

        std::cout << "Choose a camera by index or id: " << std::flush;
        std::string selector;
        if (!std::getline(std::cin, selector)) {
            std::cerr << "[ELO] Camera selection was cancelled.\n";
            return 2;
        }
        auto selection = elo::perception::select_camera(camera_report.devices, selector);
        if (!selection) {
            std::cerr << "[ELO] " << selection.error().to_string() << '\n';
            return 2;
        }
        selected_camera = std::move(*selection);
    } else if (command_line.camera_selector) {
        auto selection = elo::perception::select_camera(
            camera_report.devices, *command_line.camera_selector);
        if (!selection) {
            std::cerr << "[ELO] " << selection.error().to_string() << '\n';
            print_cameras(std::cerr, camera_report);
            return 2;
        }
        selected_camera = std::move(*selection);
    } else if (camera_report.devices.size() == 1) {
        selected_camera = camera_report.devices.front();
    } else if (camera_report.devices.size() > 1) {
        std::cerr << "[ELO][camera] Multiple cameras detected; use --choose-camera or --camera.\n";
    }

    if (command_line.require_camera && !selected_camera) {
        std::cerr << "[ELO] A camera is required, but none was selected.\n";
        return 2;
    }

    // Invariant E5 & E6: Offline local kiosk
    QGuiApplication app(argc, argv);
    app.setApplicationName("ELO Kiosk");
    app.setOrganizationName("ELO Project");

    std::cout << "[ELO] Initializing local-first offline kiosk system...\n";
    if (selected_camera) {
        std::cout << "[ELO][camera] Selected: " << selected_camera->display_name
                  << " (" << selected_camera->id << ", " << selected_camera->backend << ")\n";
    } else {
        std::cout << "[ELO][camera] No camera selected; capture remains disabled.\n";
    }

    // Isolated stores (Section 27 & 43)
    auto biometric_store = std::make_shared<elo::storage::InMemoryBiometricStore>();
    auto experience_store = std::make_shared<elo::storage::InMemoryExperienceStore>();
    auto survey_store = std::make_shared<elo::storage::InMemorySurveyStore>();

    // Identity separation (Section 19): EloId != KernelId != DeviceId != PersonLocalId
    elo::identity::EloId elo_system_id("elo://S01");
    uint64_t kernel_seed = 0x454c4f5f533031ULL; // "ELO_S01"

    auto engine = std::make_shared<elo::experience::ExperienceEngine>(
        elo_system_id,
        kernel_seed,
        biometric_store,
        experience_store,
        survey_store
    );

    auto presentation_model = std::make_unique<elo::ui::KioskPresentationModel>(engine);

    QQmlApplicationEngine qml_engine;
    qml_engine.rootContext()->setContextProperty("kioskModel", presentation_model.get());
    qml_engine.rootContext()->setContextProperty("cameraSelected", selected_camera.has_value());
    qml_engine.rootContext()->setContextProperty(
        "selectedCameraName",
        selected_camera ? QString::fromStdString(selected_camera->display_name) : QStringLiteral("NONE"));
    qml_engine.rootContext()->setContextProperty(
        "selectedCameraBackend",
        selected_camera ? QString::fromStdString(selected_camera->backend) : QStringLiteral("disabled"));

    const QUrl url(QStringLiteral("qrc:/Main.qml"));
    QObject::connect(
        &qml_engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [url](QObject* obj, const QUrl& objUrl) {
            if (!obj && url == objUrl) {
                std::cerr << "[ELO] Failed to load QML interface!\n";
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection);

    qml_engine.load(url);

    std::cout << "[ELO] Kiosk GUI active. System ready.\n";
    return app.exec();
}
