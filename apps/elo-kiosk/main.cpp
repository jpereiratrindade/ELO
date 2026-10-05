#include "elo/experience/experience_engine.hpp"
#include "elo/storage/in_memory_storage.hpp"
#include "elo/ui/kiosk_presentation_model.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <iostream>
#include <memory>

int main(int argc, char* argv[]) {
    // Invariant E5 & E6: Offline local kiosk
    QGuiApplication app(argc, argv);
    app.setApplicationName("ELO Kiosk");
    app.setOrganizationName("ELO Project");

    std::cout << "[ELO] Initializing local-first offline kiosk system...\n";

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
