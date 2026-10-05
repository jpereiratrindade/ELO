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

namespace {

void submit_three_samples(
    elo::ui::KioskPresentationModel& model,
    const QVector<float>& embedding) {
    model.onFaceEmbeddingReady(embedding, 0.96);
    model.onFaceEmbeddingReady(embedding, 0.95);
    model.onFaceEmbeddingReady(embedding, 0.94);
}

} // namespace

int main() {
    auto biometric_store = std::make_shared<elo::storage::InMemoryBiometricStore>();
    auto experience_store = std::make_shared<elo::storage::InMemoryExperienceStore>();
    auto survey_store = std::make_shared<elo::storage::InMemorySurveyStore>();
    auto engine = std::make_shared<elo::experience::ExperienceEngine>(
        elo::identity::EloId("elo://presentation-test"),
        7001,
        biometric_store,
        experience_store,
        survey_store);
    elo::ui::KioskPresentationModel model(engine);

    model.onFacePresenceChanged(true);
    TEST_ASSERT(model.currentState() == QStringLiteral("BIOMETRIC_SESSION"),
                "Face presence starts recognition without a confirmation screen");

    submit_three_samples(model, {0.2F, 0.4F, 0.6F, 0.8F});
    TEST_ASSERT(model.currentState() == QStringLiteral("IDENTITY_SUPPORTED"),
                "Three transient samples resolve the first identity");
    TEST_ASSERT(model.greetingTitle() == QStringLiteral("Bem-vindo ao ELO"),
                "First observation receives the first-visit greeting");
    TEST_ASSERT(model.activePerson() == QStringLiteral("person-local://P01"),
                "First observation creates a local identity");
    TEST_ASSERT(biometric_store->get_templates_for(
                    elo::identity::PersonLocalId("person-local://P01"))->size() == 1,
                "Only one derived template is persisted on first observation");

    model.finishSession();
    model.onFacePresenceChanged(false);
    model.onFacePresenceChanged(true);
    submit_three_samples(model, {0.21F, 0.40F, 0.59F, 0.80F});

    TEST_ASSERT(model.currentState() == QStringLiteral("IDENTITY_SUPPORTED"),
                "Returning face is recognized automatically");
    TEST_ASSERT(model.greetingTitle() == QStringLiteral("Que bom ver você novamente"),
                "Returning observation receives the return greeting");
    TEST_ASSERT(model.activePerson() == QStringLiteral("person-local://P01"),
                "Returning observation recovers the same local identity");
    TEST_ASSERT(biometric_store->get_templates_for(
                    elo::identity::PersonLocalId("person-local://P01"))->size() == 1,
                "Return refines one derived vector without accumulating photos or templates");

    std::cout << "Automatic kiosk recognition presentation tests passed.\n";
    return 0;
}
