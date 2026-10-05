#include "elo/biometric/face_template.hpp"
#include "elo/experience/experience_engine.hpp"
#include "elo/storage/sqlite_storage.hpp"

#include <QTemporaryDir>

#include <cstdlib>
#include <iostream>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion FAILED: " << msg << " (" << #cond << ")\n"; \
            std::abort(); \
        } \
    } while (0)

int main() {
    QTemporaryDir directory;
    TEST_ASSERT(directory.isValid(), "Temporary storage directory created");
    const auto database_path = directory.filePath(QStringLiteral("elo-test.sqlite3")).toStdString();
    const elo::identity::PersonLocalId person("person-local://P01");

    {
        auto stores_result = elo::storage::open_sqlite_stores(database_path);
        TEST_ASSERT(stores_result.has_value(), "SQLite stores opened");
        const auto stores = *stores_result;

        auto biometric_result = stores.biometric->save_template(elo::biometric::FaceTemplate{
            .template_id = "tmpl-person-local://P01",
            .person_local_id = person,
            .model_id = "opencv_sface",
            .model_version = "2021dec-int8",
            .representation = {0.1F, 0.2F, 0.3F, 0.4F},
            .quality = 0.93,
            .created_at = 123456,
            .integrity_digest = "test-digest"
        });
        TEST_ASSERT(biometric_result.has_value(), "Biometric template persisted");
        TEST_ASSERT(stores.experience->record_content_served(person, "content_stage_1").has_value(),
                    "Experience history persisted");
        TEST_ASSERT(stores.survey->record_response(elo::survey::SurveyResponse{
            .question_id = "question-1",
            .selected_option = "option-A",
            .timestamp = 987654,
            .policy = elo::survey::LinkagePolicy::LinkedToIdentity,
            .linked_person = person
        }).has_value(), "Survey response persisted");
    }

    {
        auto reopened_result = elo::storage::open_sqlite_stores(database_path);
        TEST_ASSERT(reopened_result.has_value(), "SQLite stores reopened after simulated restart");
        const auto stores = *reopened_result;

        const auto templates = stores.biometric->get_templates_for(person);
        TEST_ASSERT(templates && templates->size() == 1, "Biometric identity survived restart");
        TEST_ASSERT(templates->front().representation.size() == 4,
                    "Biometric representation survived restart");

        const auto history = stores.experience->get_history(person);
        TEST_ASSERT(history && history->size() == 1 && history->front() == "content_stage_1",
                    "Experience continuity survived restart");

        const auto responses = stores.survey->get_all_responses();
        TEST_ASSERT(responses && responses->size() == 1 &&
                    responses->front().linked_person == person,
                    "Linked survey response survived restart");

        elo::experience::ExperienceEngine engine(
            elo::identity::EloId("elo://sqlite-test"),
            9001,
            stores.biometric,
            stores.experience,
            stores.survey);

        engine.on_presence_detected();
        engine.begin_automatic_continuity();
        const auto recognized_identity = engine.identify_or_enroll_local_face(
            {0.1F, 0.2F, 0.3F, 0.4F}, 0.95);
        TEST_ASSERT(recognized_identity && recognized_identity->resolved_person_id == person,
                    "Persisted face is recognized after process restart");
        TEST_ASSERT(!recognized_identity->newly_enrolled,
                    "Returning identity is distinguished from first enrollment");
        TEST_ASSERT(stores.biometric->get_templates_for(person)->size() == 1,
                    "Returning recognition refines one data-minimized vector template");
        TEST_ASSERT(engine.select_next_content() == "content_stage_2",
                    "Recognized participant resumes at the next content");
        engine.finish_session();

        engine.on_presence_detected();
        engine.begin_automatic_continuity();
        const auto second_identity = engine.identify_or_enroll_local_face(
            {1.0F, 0.0F, 0.0F, 0.0F}, 0.95);
        TEST_ASSERT(second_identity && second_identity->resolved_person_id &&
                    second_identity->resolved_person_id->str() == "person-local://P02",
                    "Person id sequence resumes after process restart");
        TEST_ASSERT(engine.request_forget(*second_identity->resolved_person_id).value_or(false),
                    "Second test identity forgotten");

        TEST_ASSERT(stores.biometric->forget_person(person).value_or(false),
                    "Biometric identity forgotten");
        TEST_ASSERT(stores.experience->forget_person(person).value_or(false),
                    "Experience history forgotten");
        TEST_ASSERT(stores.survey->forget_person(person).value_or(false),
                    "Survey identity link forgotten");
    }

    {
        auto reopened_result = elo::storage::open_sqlite_stores(database_path);
        TEST_ASSERT(reopened_result.has_value(), "SQLite stores reopened after forgetting");
        const auto stores = *reopened_result;
        TEST_ASSERT(stores.biometric->get_templates_for(person)->empty(),
                    "Forgotten biometric identity remains absent");
        TEST_ASSERT(stores.experience->get_history(person)->empty(),
                    "Forgotten experience history remains absent");
        const auto responses = stores.survey->get_all_responses();
        TEST_ASSERT(responses && responses->size() == 1 &&
                    !responses->front().linked_person.has_value(),
                    "Survey response remains irreversibly unlinked");
    }

    std::cout << "SQLite offline persistence tests passed.\n";
    return 0;
}
