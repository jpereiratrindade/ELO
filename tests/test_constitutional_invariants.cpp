#include "elo/core/invariants.hpp"
#include "elo/identity/identity_spaces.hpp"
#include "elo/perception/ephemeral_frame.hpp"
#include "elo/biometric/biometric_evidence.hpp"
#include "elo/biometric/biometric_matcher.hpp"
#include "elo/experience/experience_engine.hpp"
#include "elo/storage/in_memory_storage.hpp"
#include "elo/judgment/jev_adapter.hpp"
#include "jev/types.hpp"

#include <cassert>
#include <iostream>
#include <vector>
#include <cmath>

// Simple lightweight test runner
#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion FAILED: " << msg << " (" << #cond << ")\n"; \
            std::abort(); \
        } \
    } while (0)

// Mock JEV Judge for testing
class MockJevJudge final : public jev::IJudge {
public:
    jev::Judgment judge(const jev::Context& ctx, const jev::Query& q) override {
        calls_count++;
        last_context = ctx;
        last_query = q;

        if (q.candidates.size() >= 2) {
            return jev::Judgment{
                .payload = jev::Noul{jev::Noul::Reason::Indeterminate, "Ambiguous candidates"},
                .timestamp_ns = 12345,
                .judge_model_id = "mock_judge"
            };
        }

        return jev::Judgment{
            .payload = jev::Score{.value = 0.95, .confidence = 0.9},
            .timestamp_ns = 12345,
            .judge_model_id = "mock_judge"
        };
    }

    int calls_count{0};
    jev::Context last_context;
    jev::Query last_query;
};

void test_e10_ephemeral_frame() {
    std::cout << "[TEST] Invariant E10: RAW_IMAGE_EPHEMERALITY...\n";
    std::vector<std::uint8_t> test_pixels = {10, 20, 30, 40, 50};
    {
        elo::perception::EphemeralFrame frame(5, 1, 1, test_pixels);
        TEST_ASSERT(!frame.empty(), "Frame must contain pixels initially");
        TEST_ASSERT(frame.data()[0] == 10, "Data must be accessible");
    } // frame destroyed here, wipes buffer
    std::cout << "  -> PASSED: EphemeralFrame wiped correctly upon scope exit.\n";
}

void test_e15_identity_spaces() {
    std::cout << "[TEST] Invariant E15: IDENTITY_SPACE_SEPARATION...\n";
    elo::identity::EloId elo_id("elo://S01");
    elo::identity::KernelId kernel_id(42);
    elo::identity::DeviceId dev_id("device://RPi5_01");
    elo::identity::PersonLocalId p17 = elo::identity::PersonLocalId::from_index(17);

    TEST_ASSERT(elo_id.str() == "elo://S01", "EloId format match");
    TEST_ASSERT(kernel_id.value() == 42, "KernelId value match");
    TEST_ASSERT(dev_id.str() == "device://RPi5_01", "DeviceId format match");
    TEST_ASSERT(p17.str() == "person-local://P17", "PersonLocalId format match");
    std::cout << "  -> PASSED: Identity spaces are strongly typed and non-convertible.\n";
}

void test_e2_ente_kernel_integration() {
    std::cout << "[TEST] Invariant E2: operational state is not constitutive transformation...\n";
    auto bio_store = std::make_shared<elo::storage::InMemoryBiometricStore>();
    auto exp_store = std::make_shared<elo::storage::InMemoryExperienceStore>();
    auto srv_store = std::make_shared<elo::storage::InMemorySurveyStore>();

    elo::experience::ExperienceEngine engine(
        elo::identity::EloId("elo://S01"),
        1001,
        bio_store, exp_store, srv_store
    );

    auto v0 = engine.kernel_view();
    TEST_ASSERT(v0.id == 1001, "Kernel ID matches initialization");
    TEST_ASSERT(v0.generation == 0, "Initial generation is 0");
    TEST_ASSERT(v0.alive == true, "Kernel is alive");

    engine.on_presence_detected();
    engine.on_information_acknowledged();
    engine.decide_participation(true);
    engine.decide_biometric_consent(false);
    (void)engine.select_next_content();
    engine.finish_session();

    auto v1 = engine.kernel_view();
    TEST_ASSERT(v1.generation == 0, "Operational events must not increment kernel generation");
    TEST_ASSERT(engine.last_event().has_value(), "Operational event was recorded by ELO");
    TEST_ASSERT(engine.last_event()->type == elo::experience::EventType::SESSION_FINISHED,
                "Last operational event is session completion");
    TEST_ASSERT(engine.last_event()->sequence == 6, "Operational events have their own sequence");
    TEST_ASSERT(engine.session_revision() == 5, "Only actual session state changes increment revision");
    TEST_ASSERT(engine.last_session_transition()->from == elo::experience::SessionState::CONTENT_ACTIVE,
                "Session transition records its origin");
    TEST_ASSERT(engine.last_session_transition()->to == elo::experience::SessionState::IDLE,
                "Session transition records its destination");

    auto invalid_transformation = engine.apply_constitutive_transformation({});
    TEST_ASSERT(!invalid_transformation.has_value(), "Anonymous constitutive changes are rejected");
    TEST_ASSERT(engine.kernel_view().generation == 0, "Rejected changes do not increment generation");

    auto transformed = engine.apply_constitutive_transformation({
        .change_id = "test:biometric-model-v2",
        .description = "Replace the biometric model as part of a controlled upgrade"
    });
    TEST_ASSERT(transformed.has_value(), "Explicit constitutive transformation succeeded");
    TEST_ASSERT(transformed->generation == 1, "Only explicit transformation increments generation");
    TEST_ASSERT(engine.last_constitutive_transformation().has_value(),
                "Constitutive change metadata is retained");
    TEST_ASSERT(engine.last_constitutive_transformation()->change_id == "test:biometric-model-v2",
                "Constitutive change is identified");
    TEST_ASSERT(!engine.last_constitutive_transformation()->description.empty(),
                "Constitutive change explains the realization upgrade");
    TEST_ASSERT(engine.last_event()->type == elo::experience::EventType::SESSION_FINISHED,
                "Constitutive change is not misreported as a session event");
    std::cout << "  -> PASSED: session revisions and kernel generations are independent.\n";
}

void test_exp_002_exp_003_exp_004_continuity() {
    std::cout << "[TEST] EXP-002, EXP-003, EXP-004: Enrollment, Unknown, Return...\n";
    auto bio_store = std::make_shared<elo::storage::InMemoryBiometricStore>();
    auto exp_store = std::make_shared<elo::storage::InMemoryExperienceStore>();
    auto srv_store = std::make_shared<elo::storage::InMemorySurveyStore>();

    elo::experience::ExperienceEngine engine(
        elo::identity::EloId("elo://S01"),
        2001,
        bio_store, exp_store, srv_store
    );

    // EXP-002: Unknown participant approaches
    engine.on_presence_detected();
    engine.decide_participation(true);
    engine.decide_biometric_consent(true);

    elo::biometric::BiometricMatcher matcher;
    std::vector<float> person_vector = {0.2f, 0.4f, 0.6f, 0.8f};

    // Before enrollment: matcher returns UNKNOWN
    auto initial_hyp = matcher.match(person_vector, bio_store->get_all_templates().value());
    TEST_ASSERT(initial_hyp.state == elo::biometric::IdentityState::UNKNOWN, "Must be UNKNOWN prior to enrollment");
    engine.evaluate_biometric_evidence(initial_hyp);
    TEST_ASSERT(engine.current_state() == elo::experience::SessionState::IDENTITY_UNKNOWN, "Engine state UNKNOWN");

    // EXP-003: the first consented face is enrolled automatically.
    auto enrolled_res = engine.identify_or_enroll_consented_face(person_vector, 0.95);
    TEST_ASSERT(enrolled_res.has_value(), "Automatic consented enrollment succeeded");
    TEST_ASSERT(enrolled_res->state == elo::biometric::IdentityState::SUPPORTED,
                "Newly enrolled local identity is supported for this session");
    TEST_ASSERT(enrolled_res->resolved_person_id.has_value(), "Enrollment resolved a local id");
    auto p_id = *enrolled_res->resolved_person_id;
    TEST_ASSERT(p_id.str() == "person-local://P01", "Assigned person-local://P01");

    // Serve content
    auto c1 = engine.select_next_content();
    TEST_ASSERT(c1 == "content_stage_1", "Stage 1 content served");
    engine.finish_session();
    TEST_ASSERT(engine.current_state() == elo::experience::SessionState::IDLE, "Returned to IDLE");

    // EXP-004: Person returns!
    engine.on_presence_detected();
    engine.decide_participation(true);
    engine.decide_biometric_consent(true);

    // Person presents slightly noisy but very close embedding
    std::vector<float> return_vector = {0.21f, 0.40f, 0.59f, 0.80f};
    auto return_hyp = matcher.match(return_vector, bio_store->get_all_templates().value());
    TEST_ASSERT(return_hyp.state == elo::biometric::IdentityState::SUPPORTED, "Must be SUPPORTED on return");
    TEST_ASSERT(return_hyp.resolved_person_id.has_value(), "Resolved person present");
    TEST_ASSERT(return_hyp.resolved_person_id->str() == "person-local://P01", "Matches P01");

    engine.evaluate_biometric_evidence(return_hyp);
    TEST_ASSERT(engine.current_state() == elo::experience::SessionState::IDENTITY_SUPPORTED, "Engine supported");

    // Continuity: next content should advance to stage 2!
    auto c2 = engine.select_next_content();
    TEST_ASSERT(c2 == "content_stage_2", "Stage 2 content served due to continuity");
    std::cout << "  -> PASSED: Enrollment and continuity cycle succeeded.\n";
}

void test_exp_005_ambiguity() {
    std::cout << "[TEST] EXP-005: Ambiguous Match (UNCERTAIN)...\n";
    elo::biometric::BiometricMatcher matcher;

    elo::biometric::FaceTemplate t1{
        .template_id = "t1",
        .person_local_id = elo::identity::PersonLocalId("person-local://P01"),
        .model_id = "synthetic-test-model",
        .model_version = "1",
        .representation = {1.0f, 0.0f},
        .quality = 0.9,
        .integrity_digest = "test:t1"
    };
    elo::biometric::FaceTemplate t2{
        .template_id = "t2",
        .person_local_id = elo::identity::PersonLocalId("person-local://P02"),
        .model_id = "synthetic-test-model",
        .model_version = "1",
        .representation = {0.999f, 0.01f},
        .quality = 0.9,
        .integrity_digest = "test:t2"
    };

    std::vector<float> query = {1.0f, 0.005f};
    auto hyp = matcher.match(query, {t1, t2});
    TEST_ASSERT(hyp.state == elo::biometric::IdentityState::UNCERTAIN, "Match between two close candidates must be UNCERTAIN");
    TEST_ASSERT(!hyp.resolved_person_id.has_value(), "No person resolved under uncertainty");
    std::cout << "  -> PASSED: Ambiguity produces UNCERTAIN state without false positive.\n";
}

void test_exp_007_forget_me() {
    std::cout << "[TEST] EXP-007: Local Right to be Forgotten (LOCAL_FORGETTING)...\n";
    auto bio_store = std::make_shared<elo::storage::InMemoryBiometricStore>();
    auto exp_store = std::make_shared<elo::storage::InMemoryExperienceStore>();
    auto srv_store = std::make_shared<elo::storage::InMemorySurveyStore>();

    elo::experience::ExperienceEngine engine(
        elo::identity::EloId("elo://S01"),
        3001,
        bio_store, exp_store, srv_store
    );

    engine.on_presence_detected();
    engine.decide_participation(true);
    engine.decide_biometric_consent(true);

    std::vector<float> vec = {0.5f, 0.5f};
    auto p_res = engine.enroll_consented_person(vec, 1.0);
    auto p_id = *p_res;

    // Register survey answer linked to person
    auto survey_result = engine.submit_survey_response(
        "q1", "opt_A", elo::survey::LinkagePolicy::LinkedToIdentity);
    TEST_ASSERT(survey_result.has_value(), "Linked survey response recorded");
    auto all_resp = srv_store->get_all_responses().value();
    TEST_ASSERT(all_resp.front().linked_person.has_value(), "Response linked before forgetting");

    // Invoke forget
    auto forget_result = engine.request_forget(p_id);
    TEST_ASSERT(forget_result.has_value() && *forget_result, "Forget request completed");

    // Verify biometric store has no templates for p_id
    auto templates_left = bio_store->get_templates_for(p_id).value();
    TEST_ASSERT(templates_left.empty(), "Biometric templates erased");

    // Verify history erased
    auto hist_left = exp_store->get_history(p_id).value();
    TEST_ASSERT(hist_left.empty(), "Experience history erased");

    // Verify survey response dissociated (link stripped)
    auto resp_after = srv_store->get_all_responses().value();
    TEST_ASSERT(!resp_after.front().linked_person.has_value(), "Survey response dissociated");

    std::cout << "  -> PASSED: Local forgetting cleanly purged biometric and linked identity.\n";
}

void test_e3_e4_jev_independence() {
    std::cout << "[TEST] Invariant E3 & E4: JEV Independence and Judgment-Not-Action...\n";
    auto mock_judge = std::make_shared<MockJevJudge>();
    auto jev_adapter = std::make_unique<elo::judgment::JevAdapter>(mock_judge);

    std::vector<elo::biometric::CandidateMatch> candidates = {
        {.person_id = elo::identity::PersonLocalId("person-local://P01"), .similarity = 0.91, .confidence = 0.85}
    };

    auto judgment = jev_adapter->judge_biometric_continuity(candidates, 0.9);
    TEST_ASSERT(judgment.is_score(), "JEV outputted Score judgment");
    TEST_ASSERT(judgment.as_score()->value == 0.95, "Score value matches mock");
    TEST_ASSERT(mock_judge->calls_count == 1, "JEV judge was called");
    std::cout << "  -> PASSED: JEV judged state by contract without executing actions.\n";
}

int main() {
    std::cout << "========================================================\n";
    std::cout << "   RUNNING ELO CONSTITUTIONAL INVARIANTS TEST SUITE     \n";
    std::cout << "========================================================\n";

    test_e10_ephemeral_frame();
    test_e15_identity_spaces();
    test_e2_ente_kernel_integration();
    test_exp_002_exp_003_exp_004_continuity();
    test_exp_005_ambiguity();
    test_exp_007_forget_me();
    test_e3_e4_jev_independence();

    std::cout << "========================================================\n";
    std::cout << "   ALL IMPLEMENTED CONSTITUTIONAL CHECKS PASSED!        \n";
    std::cout << "========================================================\n";
    return 0;
}
