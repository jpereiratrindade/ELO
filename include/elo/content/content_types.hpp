#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

namespace elo::content {

/// @brief ELO-CONTENT-001 Section 3: Semantic Content Types
enum class ContentType {
    Fact,
    Concept,
    Entity,
    Place,
    Phenomenon,
    Event,
    MicroStory,
    Question,
    Challenge,
    Comparison,
    Sequence,
    Indicator,
    AmbientScene,
    Unknown
};

[[nodiscard]] constexpr std::string_view to_string(ContentType type) noexcept {
    switch (type) {
        case ContentType::Fact: return "fact";
        case ContentType::Concept: return "concept";
        case ContentType::Entity: return "entity";
        case ContentType::Place: return "place";
        case ContentType::Phenomenon: return "phenomenon";
        case ContentType::Event: return "event";
        case ContentType::MicroStory: return "micro_story";
        case ContentType::Question: return "question";
        case ContentType::Challenge: return "challenge";
        case ContentType::Comparison: return "comparison";
        case ContentType::Sequence: return "sequence";
        case ContentType::Indicator: return "indicator";
        case ContentType::AmbientScene: return "ambient_scene";
        case ContentType::Unknown: return "unknown";
    }
    return "unknown";
}

[[nodiscard]] inline ContentType parse_content_type(std::string_view str) noexcept {
    if (str == "fact") return ContentType::Fact;
    if (str == "concept") return ContentType::Concept;
    if (str == "entity") return ContentType::Entity;
    if (str == "place") return ContentType::Place;
    if (str == "phenomenon") return ContentType::Phenomenon;
    if (str == "event") return ContentType::Event;
    if (str == "micro_story") return ContentType::MicroStory;
    if (str == "question") return ContentType::Question;
    if (str == "challenge") return ContentType::Challenge;
    if (str == "comparison") return ContentType::Comparison;
    if (str == "sequence") return ContentType::Sequence;
    if (str == "indicator") return ContentType::Indicator;
    if (str == "ambient_scene") return ContentType::AmbientScene;
    return ContentType::Unknown;
}

/// @brief ELO-CONTENT-001 Section 18: Presentation Modalities
enum class Modality {
    Text,
    Image,
    Audio,
    Video,
    Animation,
    Map,
    Diagram,
    DataVisualization,
    Unknown
};

[[nodiscard]] constexpr std::string_view to_string(Modality mod) noexcept {
    switch (mod) {
        case Modality::Text: return "text";
        case Modality::Image: return "image";
        case Modality::Audio: return "audio";
        case Modality::Video: return "video";
        case Modality::Animation: return "animation";
        case Modality::Map: return "map";
        case Modality::Diagram: return "diagram";
        case Modality::DataVisualization: return "data_visualization";
        case Modality::Unknown: return "unknown";
    }
    return "unknown";
}

[[nodiscard]] inline Modality parse_modality(std::string_view str) noexcept {
    if (str == "text") return Modality::Text;
    if (str == "image") return Modality::Image;
    if (str == "audio") return Modality::Audio;
    if (str == "video") return Modality::Video;
    if (str == "animation") return Modality::Animation;
    if (str == "map") return Modality::Map;
    if (str == "diagram") return Modality::Diagram;
    if (str == "data_visualization") return Modality::DataVisualization;
    return Modality::Unknown;
}

/// @brief ELO-CONTENT-001 Section 27: Experiential Roles
enum class ContentRole {
    Ambient,
    Attract,
    Engage,
    Reveal,
    Deepen,
    Unknown
};

[[nodiscard]] constexpr std::string_view to_string(ContentRole role) noexcept {
    switch (role) {
        case ContentRole::Ambient: return "ambient";
        case ContentRole::Attract: return "attract";
        case ContentRole::Engage: return "engage";
        case ContentRole::Reveal: return "reveal";
        case ContentRole::Deepen: return "deepen";
        case ContentRole::Unknown: return "unknown";
    }
    return "unknown";
}

[[nodiscard]] inline ContentRole parse_content_role(std::string_view str) noexcept {
    if (str == "ambient") return ContentRole::Ambient;
    if (str == "attract") return ContentRole::Attract;
    if (str == "engage") return ContentRole::Engage;
    if (str == "reveal") return ContentRole::Reveal;
    if (str == "deepen") return ContentRole::Deepen;
    return ContentRole::Unknown;
}

/// @brief ELO-CONTENT-001 Section 33: Supported Interaction Forms
enum class InteractionType {
    None,
    Touch,
    Choose,
    TrueFalse,
    Identify,
    Find,
    Compare,
    ListenAndIdentify,
    Reveal,
    Explore,
    FollowRelation,
    Gesture,
    CollectiveChoice,
    Unknown
};

[[nodiscard]] constexpr std::string_view to_string(InteractionType it) noexcept {
    switch (it) {
        case InteractionType::None: return "none";
        case InteractionType::Touch: return "touch";
        case InteractionType::Choose: return "choose";
        case InteractionType::TrueFalse: return "true_false";
        case InteractionType::Identify: return "identify";
        case InteractionType::Find: return "find";
        case InteractionType::Compare: return "compare";
        case InteractionType::ListenAndIdentify: return "listen_and_identify";
        case InteractionType::Reveal: return "reveal";
        case InteractionType::Explore: return "explore";
        case InteractionType::FollowRelation: return "follow_relation";
        case InteractionType::Gesture: return "gesture";
        case InteractionType::CollectiveChoice: return "collective_choice";
        case InteractionType::Unknown: return "unknown";
    }
    return "unknown";
}

[[nodiscard]] inline InteractionType parse_interaction_type(std::string_view str) noexcept {
    if (str == "none") return InteractionType::None;
    if (str == "touch") return InteractionType::Touch;
    if (str == "choose") return InteractionType::Choose;
    if (str == "true_false") return InteractionType::TrueFalse;
    if (str == "identify") return InteractionType::Identify;
    if (str == "find") return InteractionType::Find;
    if (str == "compare") return InteractionType::Compare;
    if (str == "listen_and_identify") return InteractionType::ListenAndIdentify;
    if (str == "reveal") return InteractionType::Reveal;
    if (str == "explore") return InteractionType::Explore;
    if (str == "follow_relation") return InteractionType::FollowRelation;
    if (str == "gesture") return InteractionType::Gesture;
    if (str == "collective_choice") return InteractionType::CollectiveChoice;
    return InteractionType::Unknown;
}

/// @brief ELO-CONTENT-001 Section 41: Supported Relation Types
enum class RelationType {
    IsA,
    PartOf,
    LocatedIn,
    Inhabits,
    DependsOn,
    InteractsWith,
    ThreatenedBy,
    Causes,
    Affects,
    ContrastsWith,
    SimilarTo,
    Precedes,
    Follows,
    ExampleOf,
    RelatedTo,
    Unknown
};

[[nodiscard]] constexpr std::string_view to_string(RelationType rel) noexcept {
    switch (rel) {
        case RelationType::IsA: return "is_a";
        case RelationType::PartOf: return "part_of";
        case RelationType::LocatedIn: return "located_in";
        case RelationType::Inhabits: return "inhabits";
        case RelationType::DependsOn: return "depends_on";
        case RelationType::InteractsWith: return "interacts_with";
        case RelationType::ThreatenedBy: return "threatened_by";
        case RelationType::Causes: return "causes";
        case RelationType::Affects: return "affects";
        case RelationType::ContrastsWith: return "contrasts_with";
        case RelationType::SimilarTo: return "similar_to";
        case RelationType::Precedes: return "precedes";
        case RelationType::Follows: return "follows";
        case RelationType::ExampleOf: return "example_of";
        case RelationType::RelatedTo: return "related_to";
        case RelationType::Unknown: return "unknown";
    }
    return "unknown";
}

[[nodiscard]] inline RelationType parse_relation_type(std::string_view str) noexcept {
    if (str == "is_a") return RelationType::IsA;
    if (str == "part_of") return RelationType::PartOf;
    if (str == "located_in") return RelationType::LocatedIn;
    if (str == "inhabits") return RelationType::Inhabits;
    if (str == "depends_on") return RelationType::DependsOn;
    if (str == "interacts_with") return RelationType::InteractsWith;
    if (str == "threatened_by") return RelationType::ThreatenedBy;
    if (str == "causes") return RelationType::Causes;
    if (str == "affects") return RelationType::Affects;
    if (str == "contrasts_with") return RelationType::ContrastsWith;
    if (str == "similar_to") return RelationType::SimilarTo;
    if (str == "precedes") return RelationType::Precedes;
    if (str == "follows") return RelationType::Follows;
    if (str == "example_of") return RelationType::ExampleOf;
    if (str == "related_to") return RelationType::RelatedTo;
    return RelationType::Unknown;
}

/// @brief Canonical fact with required source provenance (Section 4 & 40)
struct CanonicalFact {
    std::string fact_id;
    std::string statement;
    std::vector<std::string> source_ids{};
    std::string confidence{"reviewed"};
};

/// @brief Subject metadata for entities, places, etc.
struct ContentSubject {
    std::string canonical_name;
    std::string scientific_name{};
    std::string type_label{};
};

/// @brief Media asset references grouped by modality
struct ModalityAssets {
    std::vector<std::string> images{};
    std::vector<std::string> audios{};
    std::vector<std::string> videos{};
    std::vector<std::string> animations{};
    std::vector<std::string> maps{};
    std::vector<std::string> diagrams{};
    std::vector<std::string> data_visualizations{};
};

/// @brief ELO-CONTENT-001 Section 40: ContentAtom
struct ContentAtom {
    std::string schema_version{"0.2"};
    std::string content_id;
    ContentType type{ContentType::Unknown};
    std::string subtype{};
    std::string title;
    std::string summary{};
    std::string language{"pt-BR"};
    std::string lifecycle_status{"draft"};
    ContentSubject subject;
    std::vector<std::string> themes{};
    std::vector<CanonicalFact> canonical_facts{};
    ModalityAssets assets;
    std::vector<ContentRole> supported_roles{};
    std::vector<std::string> audiences{"general"};
    std::vector<std::string> relation_ids{};
    std::unordered_map<std::string, std::string> custom_attributes{};
    std::string creator{};
    std::string publisher{};
    std::string source_reference{};
    std::string license{};
    std::string rights_holder{};
    std::string attribution{};
    std::string created_at{};
    std::string modified_at{};
    std::string alt_text{};
    std::string transcript{};
    bool reviewed{true};
};

/// @brief ELO-CONTENT-001 Section 41: ContentRelation
struct ContentRelation {
    std::string relation_id;
    RelationType type{RelationType::RelatedTo};
    std::string from_content_id;
    std::string to_content_id;
    std::vector<std::string> source_ids{};
    double confidence{1.0};
    std::string description{};
};

/// @brief Presentation block within a variant
struct VariantPresentation {
    std::string text;
    std::string title{};
    std::vector<std::string> media_refs{};
    std::vector<std::string> options{};
    std::string correct_option{};
};

/// @brief ELO-CONTENT-001 Section 42: ContentVariant
struct ContentVariant {
    std::string variant_id;
    std::string content_id;
    ContentRole role{ContentRole::Attract};
    std::string audience{"general"};
    VariantPresentation presentation;
    InteractionType interaction{InteractionType::None};
    std::uint32_t duration_hint_seconds{8};
};

/// @brief ELO-CONTENT-001 Section 44: ExperienceRecipe
struct ExperienceRecipe {
    std::string recipe_id;
    std::string name;
    std::vector<std::string> required_modalities{};
    std::vector<std::string> steps{};
    std::string description{};
};

/// @brief ELO-CONTENT-001 Section 56: SelectionReason
struct SelectionReason {
    std::string selected_content_id;
    std::string selected_variant_id;
    std::string recipe_id;
    ContentRole requested_role{ContentRole::Ambient};
    std::string theme_matched{};
    bool not_seen_in_session{true};
    bool media_requirements_met{true};
    std::string explanation{};
};

/// @brief ELO-CONTENT-001 Section 54: PresentationAction
enum class PresentationActionType {
    ShowText,
    ShowImage,
    PlayAudio,
    PlayVideo,
    ShowMap,
    ShowDiagram,
    ShowData,
    Animate,
    AskChoice,
    AskTrueFalse,
    WaitForTouch,
    WaitForGesture,
    Reveal,
    OfferDeepen,
    Clear
};

[[nodiscard]] constexpr std::string_view to_string(PresentationActionType act) noexcept {
    switch (act) {
        case PresentationActionType::ShowText: return "ShowText";
        case PresentationActionType::ShowImage: return "ShowImage";
        case PresentationActionType::PlayAudio: return "PlayAudio";
        case PresentationActionType::PlayVideo: return "PlayVideo";
        case PresentationActionType::ShowMap: return "ShowMap";
        case PresentationActionType::ShowDiagram: return "ShowDiagram";
        case PresentationActionType::ShowData: return "ShowData";
        case PresentationActionType::Animate: return "Animate";
        case PresentationActionType::AskChoice: return "AskChoice";
        case PresentationActionType::AskTrueFalse: return "AskTrueFalse";
        case PresentationActionType::WaitForTouch: return "WaitForTouch";
        case PresentationActionType::WaitForGesture: return "WaitForGesture";
        case PresentationActionType::Reveal: return "Reveal";
        case PresentationActionType::OfferDeepen: return "OfferDeepen";
        case PresentationActionType::Clear: return "Clear";
    }
    return "Unknown";
}

struct PresentationAction {
    PresentationActionType type{PresentationActionType::ShowText};
    std::string title{};
    std::string text{};
    std::string asset_path{};
    std::vector<std::string> options{};
    std::string target_content_id{};
};

/// @brief ELO-PUBLISHING-001 Section 3: Bundle Manifest
struct BundleManifest {
    std::string bundle_id{"elo-content-default"};
    std::string version{"0.1.0"};
    std::string schema_version{"0.1"};
    std::string title{};
    std::string default_theme{};
    std::string description{};
    std::string license{"GPL-3.0-only"};
    std::uint64_t curation_revision{1};
    std::string content_hash{};
    std::string parent_bundle{};
    std::string created_at{};
    std::string application_id{};
    std::vector<std::string> atom_ids{};
};

} // namespace elo::content
