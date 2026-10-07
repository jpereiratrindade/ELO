#include "elo/storage/in_memory_storage.hpp"
#include "elo/ui/kiosk_presentation_model.hpp"
#include "test_content_fixtures.hpp"

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
    TEST_ASSERT(model.activePerson().startsWith(QStringLiteral("person-local://")),
                "First observation creates a local identity");
    auto first_person_id = elo::identity::PersonLocalId(model.activePerson().toStdString());
    TEST_ASSERT(biometric_store->get_templates_for(first_person_id)->size() == 1,
                "Only one derived template is persisted on first observation");

    model.finishSession();
    model.onFacePresenceChanged(false);
    model.onFacePresenceChanged(true);
    submit_three_samples(model, {0.21F, 0.40F, 0.59F, 0.80F});

    TEST_ASSERT(model.currentState() == QStringLiteral("IDENTITY_SUPPORTED"),
                "Returning face is recognized automatically");
    TEST_ASSERT(model.greetingTitle() == QStringLiteral("Que bom ver você novamente"),
                "Returning observation receives the return greeting");
    TEST_ASSERT(model.activePerson() == QString::fromStdString(first_person_id.str()),
                "Returning observation recovers the same local identity");
    TEST_ASSERT(biometric_store->get_templates_for(first_person_id)->size() == 1,
                "Return refines one derived vector without accumulating photos or templates");

    // Test autonomous progression: greeting advances to content without mouse
    TEST_ASSERT(model.behaviorProgress() >= 0.0, "Behavior progress starts valid");
    model.tick(2.1);
    TEST_ASSERT(model.currentState() == QStringLiteral("CONTENT_ACTIVE"),
                "Autonomous timer advances from greeting to content without mouse interaction");

    // Test autonomous absence detection: when visitor walks away, session closes gracefully
    model.onFacePresenceChanged(false);
    model.tick(0.8);
    TEST_ASSERT(model.currentState() == QStringLiteral("CONTENT_ACTIVE"),
                "Brief absence does not immediately drop session");
    model.tick(1.0); // total 1.8s > 1.5s
    TEST_ASSERT(model.currentState() == QStringLiteral("IDLE"),
                "Confirmed absence finishes session and returns to IDLE");

    // Test autonomous presence progression without biometric match (UNKNOWN visitor)
    model.onFacePresenceChanged(true);
    TEST_ASSERT(model.currentState() == QStringLiteral("BIOMETRIC_SESSION"),
                "Visitor approach triggers presence session");
    model.tick(2.6); // > 2.5s biometric grace period
    TEST_ASSERT(model.currentState() == QStringLiteral("CONTENT_ACTIVE"),
                "Non-matched presence autonomously advances to content without blocking");

    // Test autonomous option progression in decoupled catalog recipes (no mouse)
    auto catalog = std::make_shared<elo::content::ContentCatalog>();
    auto fixture_cat = elo::test::create_test_content_fixture() / "catalog";
    if (catalog->load_from_directory(fixture_cat.string())) {
        engine->set_content_catalog(catalog);
        auto sel_res = engine->select_contextual_content(elo::content::ContentRole::Ambient, "");
        TEST_ASSERT(sel_res.has_value(), "Content selection succeeds");
        if (engine->active_content_atom() && !engine->active_content_atom()->canonical_facts.empty()) {
            TEST_ASSERT(model.contentText() == QString::fromStdString(engine->active_content_atom()->canonical_facts.front().statement),
                        "Atom canonical fact from admin is primary authority for text presentation");
        }
        model.startRecipe(QStringLiteral("discover_by_sound"));
        TEST_ASSERT(model.isRecipeActive(), "Recipe starts correctly");
        TEST_ASSERT(!model.contentOptions().isEmpty(), "Step presents interactive choice");
        // Simulate waiting in front of the kiosk without mouse/touch
        model.tick(8.6);
        TEST_ASSERT(model.behaviorProgress() == 0.0, "Progress resets after autonomous choice");
        // 5. Ela vai embora
        model.onFacePresenceChanged(false);
        model.tick(1.6);
        TEST_ASSERT(model.currentState() == QStringLiteral("IDLE"),
                    "Departure gracefully resets kiosk to contemplative ambient IDLE");

        // 6. Retorna depois
        model.onFacePresenceChanged(true);
        submit_three_samples(model, {0.21F, 0.40F, 0.59F, 0.80F});
        TEST_ASSERT(model.currentState() == QStringLiteral("IDENTITY_SUPPORTED"),
                    "Returning visitor is recognized");
        TEST_ASSERT(model.greetingTitle() == QStringLiteral("Que bom ver você novamente"),
                    "Return greeting matches continuity");

        // 7. O ELO continua de onde aquela relação parou
        model.tick(2.1);
        TEST_ASSERT(model.currentState() == QStringLiteral("CONTENT_ACTIVE"),
                    "Kiosk resumes experience for returning visitor");
    }

    std::cout << "Automatic kiosk recognition presentation tests passed.\n";
    return 0;
}
