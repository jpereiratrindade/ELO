#include "elo/storage/in_memory_storage.hpp"
#include "elo/ui/kiosk_presentation_model.hpp"

#include <QVector>

#include <cstdlib>
#include <iostream>
#include <memory>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion FAILED: " << msg << " (" << #cond << ")\n"; \
            std::abort(); \
        } \
    } while (0)

int main() {
    auto biometric_store = std::make_shared<elo::storage::InMemoryBiometricStore>();
    auto experience_store = std::make_shared<elo::storage::InMemoryExperienceStore>();
    auto survey_store = std::make_shared<elo::storage::InMemorySurveyStore>();
    auto engine = std::make_shared<elo::experience::ExperienceEngine>(
        elo::identity::EloId("elo://presentation-test"), 7001,
        biometric_store, experience_store, survey_store);
    elo::ui::KioskPresentationModel model(engine);

    model.onFacePresenceChanged(true);
    TEST_ASSERT(model.currentState() == QStringLiteral("PRESENCE_DETECTED"),
                "Presence starts an anonymous session");
    TEST_ASSERT(model.activePerson() == QStringLiteral("UNKNOWN"),
                "Anonymous session has no identity");
    TEST_ASSERT(model.greetingTitle() == QStringLiteral("Presença detectada"),
                "Presentation describes presence, not recognition");

    const QVector<float> embedding{0.2F, 0.4F, 0.6F, 0.8F};
    model.onFaceEmbeddingReady(embedding, 0.96);
    model.onFaceEmbeddingReady(embedding, 0.95);
    model.onFaceEmbeddingReady(embedding, 0.94);
    TEST_ASSERT(biometric_store->get_all_templates()->empty(),
                "Embeddings after presence are never persisted");
    TEST_ASSERT(model.currentState() == QStringLiteral("PRESENCE_DETECTED"),
                "Embeddings do not activate biometric continuity");

    model.tick(2.6);
    TEST_ASSERT(model.currentState() == QStringLiteral("CONTENT_ACTIVE"),
                "Anonymous session reaches content without identification");

    model.onVisionFailure(QStringLiteral("camera disconnected"));
    model.tick(2.0);
    TEST_ASSERT(model.currentState() == QStringLiteral("CONTENT_ACTIVE"),
                "Camera failure does not terminate anonymous experience");
    TEST_ASSERT(biometric_store->get_all_templates()->empty(),
                "Camera failure cannot cause biometric persistence");

    model.finishSession();
    TEST_ASSERT(model.currentState() == QStringLiteral("IDLE"),
                "Anonymous session finishes normally");

    std::cout << "Anonymous kiosk presentation G0 tests passed.\n";
    return 0;
}
