#include "elo/content/content_validator.hpp"

namespace elo::content {

ValidationReport ContentValidator::validate(const ContentCatalog& catalog) {
    ValidationReport report;

    // 1. Validate Atoms
    for (const auto* atom : catalog.all_atoms()) {
        if (atom->content_id.empty()) {
            report.errors.push_back({"", "ContentAtom has empty content_id", true});
            report.valid = false;
        }

        if (atom->type == ContentType::Unknown) {
            report.errors.push_back({atom->content_id, "ContentAtom has Unknown content type", true});
            report.valid = false;
        }

        if (atom->title.empty()) {
            report.errors.push_back({atom->content_id, "ContentAtom has empty title", true});
            report.valid = false;
        }
        if (atom->language.empty()) {
            report.warnings.push_back("ContentAtom " + atom->content_id + " has no language metadata");
        }
        if (atom->lifecycle_status != "draft" && atom->lifecycle_status != "in_review"
            && atom->lifecycle_status != "approved" && atom->lifecycle_status != "archived") {
            report.errors.push_back({atom->content_id, "ContentAtom has invalid lifecycle_status", true});
            report.valid = false;
        }
        if (!atom->assets.images.empty() && atom->alt_text.empty()) {
            report.warnings.push_back("ContentAtom " + atom->content_id + " has images but no accessibility alt_text");
        }
        if (atom->license.empty()) {
            report.warnings.push_back("ContentAtom " + atom->content_id + " has no rights license");
        }

        if (atom->supported_roles.empty()) {
            report.warnings.push_back("ContentAtom " + atom->content_id + " has no supported roles declared");
        }

        // Canonical facts must have provenance (Section 4 & 40)
        for (const auto& fact : atom->canonical_facts) {
            if (fact.source_ids.empty()) {
                report.errors.push_back({
                    atom->content_id,
                    "Canonical fact " + fact.fact_id + " lacks required source_ids provenance",
                    true
                });
                report.valid = false;
            }
        }
    }

    // 2. Validate Relations
    for (const auto* atom : catalog.all_atoms()) {
        auto from_rels = catalog.find_relations_from(atom->content_id);
        for (const auto* rel : from_rels) {
            if (rel->to_content_id.empty()) {
                report.errors.push_back({rel->relation_id, "Relation has empty target 'to_content_id'", true});
                report.valid = false;
            } else if (!catalog.find_atom(rel->to_content_id)) {
                report.errors.push_back({
                    rel->relation_id,
                    "Broken relation target: '" + rel->to_content_id + "' not found in catalog",
                    true
                });
                report.valid = false;
            }

            if (rel->type == RelationType::Unknown) {
                report.warnings.push_back("Relation " + rel->relation_id + " has Unknown relation type");
            }
        }
    }

    // 3. Validate Variants
    for (const auto* atom : catalog.all_atoms()) {
        auto variants = catalog.find_variants_for(atom->content_id);
        for (const auto* var : variants) {
            if (var->role == ContentRole::Unknown) {
                report.errors.push_back({var->variant_id, "Variant has Unknown role", true});
                report.valid = false;
            }
            if (var->presentation.text.empty() && var->presentation.media_refs.empty()) {
                report.warnings.push_back("Variant " + var->variant_id + " has empty presentation text and media");
            }
        }
    }

    // 4. Validate Recipes
    for (const auto* recipe : catalog.all_recipes()) {
        if (recipe->steps.empty()) {
            report.errors.push_back({recipe->recipe_id, "Recipe has no execution steps defined", true});
            report.valid = false;
        }
    }

    return report;
}

} // namespace elo::content
