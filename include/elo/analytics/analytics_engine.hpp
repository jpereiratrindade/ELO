#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <cstdint>
#include <algorithm>

namespace elo::analytics {

/// @brief Privacy-preserving aggregate event record. NO PII or personal IDs are stored.
struct AggregateEvent {
    std::string app_id;               // Application context (e.g. "app.elo.bioma-pampa")
    std::string bundle_id;            // Bundle active during event
    std::string event_type;           // "session_start", "atom_view", "recipe_step", "trail_choice", "dwell_tick"
    std::string target_atom_id;       // Content Atom viewed or chosen
    std::string source_atom_id;       // Previous atom in flow transition (if applicable)
    double duration_seconds{0.0};     // Duration in this state/content
    std::uint64_t hour_bucket{0};     // Aggregation timestamp bucket (epoch hour)
};

/// @brief Summarized analytical metrics for a given application or bundle.
struct AnalyticsSummary {
    std::string app_id;
    std::string bundle_id;
    std::uint64_t total_sessions{0};
    double average_dwell_time_seconds{0.0};
    double total_engagement_seconds{0.0};
    std::uint64_t total_atom_views{0};
    std::uint64_t total_recipe_completions{0};
    std::unordered_map<std::string, std::uint64_t> atom_view_counts;
    std::unordered_map<std::string, double> atom_avg_dwell_seconds;
    std::unordered_map<std::string, std::unordered_map<std::string, std::uint64_t>> transition_matrix; // from_atom -> to_atom -> count
};

/// @brief Sovereign zero-PII data analytics engine for kiosks and interactive exhibitions.
class AnalyticsEngine {
public:
    AnalyticsEngine() = default;

    /// @brief Record an anonymized interaction event.
    void record_event(
        std::string app_id,
        std::string bundle_id,
        std::string event_type,
        std::string target_atom_id,
        std::string source_atom_id = "",
        double duration_seconds = 0.0) {

        auto now_sec = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
        std::uint64_t hour_bucket = now_sec - (now_sec % 3600); // Truncate to hour bucket for privacy

        events_.push_back(AggregateEvent{
            .app_id = std::move(app_id),
            .bundle_id = std::move(bundle_id),
            .event_type = std::move(event_type),
            .target_atom_id = std::move(target_atom_id),
            .source_atom_id = std::move(source_atom_id),
            .duration_seconds = duration_seconds,
            .hour_bucket = hour_bucket
        });
    }

    /// @brief Calculate an aggregated analytics summary for a given app or all apps.
    [[nodiscard]] AnalyticsSummary get_summary(const std::string& app_id_filter = "") const {
        AnalyticsSummary summary;
        summary.app_id = app_id_filter.empty() ? "all_applications" : app_id_filter;

        std::unordered_map<std::string, double> atom_dwell_sum;
        std::unordered_map<std::string, std::uint64_t> atom_dwell_count;

        for (const auto& ev : events_) {
            if (!app_id_filter.empty() && ev.app_id != app_id_filter) {
                continue;
            }

            if (ev.event_type == "session_start" || ev.event_type == "presence_engaged") {
                summary.total_sessions++;
            } else if (ev.event_type == "atom_view") {
                summary.total_atom_views++;
                if (!ev.target_atom_id.empty()) {
                    summary.atom_view_counts[ev.target_atom_id]++;
                    if (ev.duration_seconds > 0.0) {
                        atom_dwell_sum[ev.target_atom_id] += ev.duration_seconds;
                        atom_dwell_count[ev.target_atom_id]++;
                        summary.total_engagement_seconds += ev.duration_seconds;
                    }
                }
            } else if (ev.event_type == "recipe_complete") {
                summary.total_recipe_completions++;
            }

            // Record navigation transitions
            if (!ev.source_atom_id.empty() && !ev.target_atom_id.empty() && ev.source_atom_id != ev.target_atom_id) {
                summary.transition_matrix[ev.source_atom_id][ev.target_atom_id]++;
            }
        }

        for (const auto& [atom, sum] : atom_dwell_sum) {
            auto cnt = atom_dwell_count[atom];
            if (cnt > 0) {
                summary.atom_avg_dwell_seconds[atom] = sum / static_cast<double>(cnt);
            }
        }

        if (summary.total_sessions > 0) {
            summary.average_dwell_time_seconds = summary.total_engagement_seconds / static_cast<double>(summary.total_sessions);
        } else if (summary.total_atom_views > 0) {
            summary.average_dwell_time_seconds = summary.total_engagement_seconds / static_cast<double>(summary.total_atom_views);
        }

        return summary;
    }

    [[nodiscard]] std::size_t total_events() const noexcept {
        return events_.size();
    }

    void clear() {
        events_.clear();
    }

private:
    std::vector<AggregateEvent> events_;
};

} // namespace elo::analytics
