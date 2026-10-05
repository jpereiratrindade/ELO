#pragma once

#include "elo/storage/storage_interfaces.hpp"
#include <unordered_map>
#include <mutex>
#include <algorithm>

namespace elo::storage {

class InMemoryBiometricStore final : public IBiometricStore {
public:
    core::Result<void> save_template(const biometric::FaceTemplate& tmpl) override {
        std::lock_guard lock(mutex_);
        templates_.push_back(tmpl);
        return {};
    }

    core::Result<std::vector<biometric::FaceTemplate>> get_all_templates() const override {
        std::lock_guard lock(mutex_);
        return templates_;
    }

    core::Result<std::vector<biometric::FaceTemplate>> get_templates_for(
        const identity::PersonLocalId& person_id) const override {
        std::lock_guard lock(mutex_);
        std::vector<biometric::FaceTemplate> result;
        for (const auto& t : templates_) {
            if (t.person_local_id == person_id) {
                result.push_back(t);
            }
        }
        return result;
    }

    core::Result<bool> forget_person(const identity::PersonLocalId& person_id) override {
        std::lock_guard lock(mutex_);
        auto orig_size = templates_.size();
        std::erase_if(templates_, [&](const biometric::FaceTemplate& t) {
            return t.person_local_id == person_id;
        });
        return templates_.size() < orig_size;
    }

    core::Result<void> clear() override {
        std::lock_guard lock(mutex_);
        templates_.clear();
        return {};
    }

private:
    mutable std::mutex mutex_;
    std::vector<biometric::FaceTemplate> templates_;
};

class InMemoryExperienceStore final : public IExperienceStore {
public:
    core::Result<void> record_content_served(
        const identity::PersonLocalId& person_id,
        const std::string& content_id) override {
        std::lock_guard lock(mutex_);
        history_[person_id.str()].push_back(content_id);
        return {};
    }

    core::Result<std::vector<std::string>> get_history(
        const identity::PersonLocalId& person_id) const override {
        std::lock_guard lock(mutex_);
        auto it = history_.find(person_id.str());
        if (it != history_.end()) {
            return it->second;
        }
        return std::vector<std::string>{};
    }

    core::Result<bool> forget_person(const identity::PersonLocalId& person_id) override {
        std::lock_guard lock(mutex_);
        return history_.erase(person_id.str()) > 0;
    }

    core::Result<void> clear() override {
        std::lock_guard lock(mutex_);
        history_.clear();
        return {};
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::vector<std::string>> history_;
};

class InMemorySurveyStore final : public ISurveyStore {
public:
    core::Result<void> record_response(const survey::SurveyResponse& response) override {
        std::lock_guard lock(mutex_);
        responses_.push_back(response);
        return {};
    }

    core::Result<std::vector<survey::SurveyResponse>> get_all_responses() const override {
        std::lock_guard lock(mutex_);
        return responses_;
    }

    core::Result<bool> forget_person(const identity::PersonLocalId& person_id) override {
        std::lock_guard lock(mutex_);
        bool modified = false;
        for (auto& r : responses_) {
            if (r.linked_person && *r.linked_person == person_id) {
                // Dissociate response if configured to strip, or remove entirely
                r.linked_person = std::nullopt;
                r.policy = survey::LinkagePolicy::UnlinkedAnonymous;
                modified = true;
            }
        }
        return modified;
    }

    core::Result<void> clear() override {
        std::lock_guard lock(mutex_);
        responses_.clear();
        return {};
    }

private:
    mutable std::mutex mutex_;
    std::vector<survey::SurveyResponse> responses_;
};

} // namespace elo::storage
