#include "elo/content/content_catalog.hpp"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

#include <algorithm>
#include <iostream>

namespace elo::content {

namespace {

ContentAtom parse_atom_json(const QJsonObject& obj) {
    ContentAtom atom;
    atom.schema_version = obj.value(QStringLiteral("schema_version")).toString(QStringLiteral("0.1")).toStdString();
    atom.content_id = obj.value(QStringLiteral("content_id")).toString().toStdString();
    atom.type = parse_content_type(obj.value(QStringLiteral("type")).toString().toStdString());
    atom.subtype = obj.value(QStringLiteral("subtype")).toString().toStdString();
    atom.title = obj.value(QStringLiteral("title")).toString().toStdString();

    const auto subj_obj = obj.value(QStringLiteral("subject")).toObject();
    atom.subject.canonical_name = subj_obj.value(QStringLiteral("canonical_name")).toString().toStdString();
    atom.subject.scientific_name = subj_obj.value(QStringLiteral("scientific_name")).toString().toStdString();
    atom.subject.type_label = subj_obj.value(QStringLiteral("type_label")).toString().toStdString();

    const auto themes_arr = obj.value(QStringLiteral("themes")).toArray();
    for (const auto& t : themes_arr) {
        atom.themes.push_back(t.toString().toStdString());
    }

    const auto facts_arr = obj.value(QStringLiteral("canonical_facts")).toArray();
    for (const auto& f_val : facts_arr) {
        const auto f_obj = f_val.toObject();
        CanonicalFact fact;
        fact.fact_id = f_obj.value(QStringLiteral("fact_id")).toString().toStdString();
        fact.statement = f_obj.value(QStringLiteral("statement")).toString().toStdString();
        const auto src_arr = f_obj.value(QStringLiteral("source_ids")).toArray();
        for (const auto& s : src_arr) {
            fact.source_ids.push_back(s.toString().toStdString());
        }
        fact.confidence = f_obj.value(QStringLiteral("confidence")).toString(QStringLiteral("reviewed")).toStdString();
        atom.canonical_facts.push_back(std::move(fact));
    }

    const auto mod_obj = obj.value(QStringLiteral("modalities")).toObject();
    for (const auto& img : mod_obj.value(QStringLiteral("image")).toArray()) {
        atom.assets.images.push_back(img.toString().toStdString());
    }
    for (const auto& aud : mod_obj.value(QStringLiteral("audio")).toArray()) {
        atom.assets.audios.push_back(aud.toString().toStdString());
    }
    for (const auto& vid : mod_obj.value(QStringLiteral("video")).toArray()) {
        atom.assets.videos.push_back(vid.toString().toStdString());
    }

    const auto roles_arr = obj.value(QStringLiteral("supported_roles")).toArray();
    for (const auto& r : roles_arr) {
        auto role = parse_content_role(r.toString().toStdString());
        if (role != ContentRole::Unknown) {
            atom.supported_roles.push_back(role);
        }
    }

    const auto aud_arr = obj.value(QStringLiteral("audiences")).toArray();
    for (const auto& a : aud_arr) {
        atom.audiences.push_back(a.toString().toStdString());
    }

    const auto rel_arr = obj.value(QStringLiteral("relations")).toArray();
    for (const auto& r : rel_arr) {
        atom.relation_ids.push_back(r.toString().toStdString());
    }

    const auto prov_obj = obj.value(QStringLiteral("provenance")).toObject();
    atom.reviewed = prov_obj.value(QStringLiteral("reviewed")).toBool(true);

    return atom;
}

ContentRelation parse_relation_json(const QJsonObject& obj) {
    ContentRelation rel;
    rel.relation_id = obj.value(QStringLiteral("relation_id")).toString().toStdString();
    rel.type = parse_relation_type(obj.value(QStringLiteral("type")).toString().toStdString());
    rel.from_content_id = obj.value(QStringLiteral("from")).toString().toStdString();
    rel.to_content_id = obj.value(QStringLiteral("to")).toString().toStdString();
    rel.confidence = obj.value(QStringLiteral("confidence")).toDouble(1.0);
    rel.description = obj.value(QStringLiteral("description")).toString().toStdString();

    for (const auto& s : obj.value(QStringLiteral("source_ids")).toArray()) {
        rel.source_ids.push_back(s.toString().toStdString());
    }
    return rel;
}

ContentVariant parse_variant_json(const QJsonObject& obj) {
    ContentVariant variant;
    variant.variant_id = obj.value(QStringLiteral("variant_id")).toString().toStdString();
    variant.content_id = obj.value(QStringLiteral("content_id")).toString().toStdString();
    variant.role = parse_content_role(obj.value(QStringLiteral("role")).toString().toStdString());
    variant.audience = obj.value(QStringLiteral("audience")).toString(QStringLiteral("general")).toStdString();
    variant.interaction = parse_interaction_type(obj.value(QStringLiteral("interaction")).toString().toStdString());
    variant.duration_hint_seconds = static_cast<std::uint32_t>(obj.value(QStringLiteral("duration_hint_seconds")).toInt(8));

    const auto pres_obj = obj.value(QStringLiteral("presentation")).toObject();
    variant.presentation.title = pres_obj.value(QStringLiteral("title")).toString().toStdString();
    variant.presentation.text = pres_obj.value(QStringLiteral("text")).toString().toStdString();
    variant.presentation.correct_option = pres_obj.value(QStringLiteral("correct_option")).toString().toStdString();

    for (const auto& m : pres_obj.value(QStringLiteral("media")).toArray()) {
        variant.presentation.media_refs.push_back(m.toString().toStdString());
    }
    for (const auto& opt : pres_obj.value(QStringLiteral("options")).toArray()) {
        variant.presentation.options.push_back(opt.toString().toStdString());
    }
    return variant;
}

ExperienceRecipe parse_recipe_json(const QJsonObject& obj) {
    ExperienceRecipe rec;
    rec.recipe_id = obj.value(QStringLiteral("recipe_id")).toString().toStdString();
    rec.name = obj.value(QStringLiteral("name")).toString().toStdString();
    rec.description = obj.value(QStringLiteral("description")).toString().toStdString();

    for (const auto& m : obj.value(QStringLiteral("requires")).toArray()) {
        rec.required_modalities.push_back(m.toString().toStdString());
    }
    for (const auto& s : obj.value(QStringLiteral("steps")).toArray()) {
        rec.steps.push_back(s.toString().toStdString());
    }
    return rec;
}

std::optional<QJsonDocument> read_json_file(const QString& file_path) {
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return std::nullopt;
    }
    QJsonParseError parse_err;
    auto doc = QJsonDocument::fromJson(file.readAll(), &parse_err);
    if (parse_err.error != QJsonParseError::NoError) {
        return std::nullopt;
    }
    return doc;
}

BundleManifest parse_manifest_json(const QJsonObject& obj) {
    BundleManifest manifest;
    manifest.bundle_id = obj.value(QStringLiteral("bundle_id")).toString().toStdString();
    manifest.version = obj.value(QStringLiteral("version")).toString(QStringLiteral("0.1.0")).toStdString();
    manifest.schema_version = obj.value(QStringLiteral("schema_version")).toString(QStringLiteral("0.1")).toStdString();
    manifest.title = obj.value(QStringLiteral("title")).toString().toStdString();
    manifest.default_theme = obj.value(QStringLiteral("default_theme")).toString().toStdString();
    manifest.description = obj.value(QStringLiteral("description")).toString().toStdString();
    manifest.license = obj.value(QStringLiteral("license")).toString(QStringLiteral("GPL-3.0-only")).toStdString();
    manifest.curation_revision = static_cast<std::uint64_t>(obj.value(QStringLiteral("curation_revision")).toInteger(1));
    manifest.content_hash = obj.value(QStringLiteral("content_hash")).toString().toStdString();
    manifest.parent_bundle = obj.value(QStringLiteral("parent_bundle")).toString().toStdString();
    manifest.created_at = obj.value(QStringLiteral("created_at")).toString().toStdString();
    return manifest;
}

} // namespace

core::Result<void> ContentCatalog::load_from_directory(const std::filesystem::path& catalog_dir) {
    const QString base = QString::fromStdString(catalog_dir.string());
    QDir dir(base);
    if (!dir.exists()) {
        return std::unexpected(core::make_error(
            core::ErrorCode::ContentError,
            "Content directory does not exist: " + catalog_dir.string()));
    }

    clear();

    // 0. Load manifest.json if present (in catalog dir or parent bundle dir)
    QString manifest_path = dir.filePath(QStringLiteral("manifest.json"));
    if (!QFile::exists(manifest_path)) {
        manifest_path = QDir(dir.filePath(QStringLiteral(".."))).filePath(QStringLiteral("manifest.json"));
    }
    if (QFile::exists(manifest_path)) {
        auto doc = read_json_file(manifest_path);
        if (doc && doc->isObject()) {
            manifest_ = parse_manifest_json(doc->object());
        }
    }

    // 1. Load atoms from catalog/atoms or objects
    QDir atoms_dir(dir.filePath(QStringLiteral("atoms")));
    if (!atoms_dir.exists()) {
        atoms_dir = QDir(dir.filePath(QStringLiteral("objects")));
    }
    if (atoms_dir.exists()) {
        for (const auto& file_info : atoms_dir.entryInfoList(QStringList() << QStringLiteral("*.json"), QDir::Files)) {
            auto doc = read_json_file(file_info.absoluteFilePath());
            if (doc && doc->isObject()) {
                add_atom(parse_atom_json(doc->object()));
            }
        }
    }

    // 2. Load relations
    QDir rel_dir(dir.filePath(QStringLiteral("relations")));
    if (rel_dir.exists()) {
        for (const auto& file_info : rel_dir.entryInfoList(QStringList() << QStringLiteral("*.json"), QDir::Files)) {
            auto doc = read_json_file(file_info.absoluteFilePath());
            if (doc) {
                if (doc->isArray()) {
                    for (const auto& elem : doc->array()) {
                        if (elem.isObject()) {
                            add_relation(parse_relation_json(elem.toObject()));
                        }
                    }
                } else if (doc->isObject()) {
                    add_relation(parse_relation_json(doc->object()));
                }
            }
        }
    }

    // 3. Load variants
    QDir var_dir(dir.filePath(QStringLiteral("variants")));
    if (var_dir.exists()) {
        for (const auto& file_info : var_dir.entryInfoList(QStringList() << QStringLiteral("*.json"), QDir::Files)) {
            auto doc = read_json_file(file_info.absoluteFilePath());
            if (doc) {
                if (doc->isArray()) {
                    for (const auto& elem : doc->array()) {
                        if (elem.isObject()) {
                            add_variant(parse_variant_json(elem.toObject()));
                        }
                    }
                } else if (doc->isObject()) {
                    add_variant(parse_variant_json(doc->object()));
                }
            }
        }
    }

    // 4. Load recipes
    QDir rec_dir(dir.filePath(QStringLiteral("recipes")));
    if (rec_dir.exists()) {
        for (const auto& file_info : rec_dir.entryInfoList(QStringList() << QStringLiteral("*.json"), QDir::Files)) {
            auto doc = read_json_file(file_info.absoluteFilePath());
            if (doc) {
                if (doc->isArray()) {
                    for (const auto& elem : doc->array()) {
                        if (elem.isObject()) {
                            add_recipe(parse_recipe_json(elem.toObject()));
                        }
                    }
                } else if (doc->isObject()) {
                    add_recipe(parse_recipe_json(doc->object()));
                }
            }
        }
    }

    return {};
}

void ContentCatalog::add_atom(ContentAtom atom) {
    if (atom.content_id.empty()) return;
    atoms_[atom.content_id] = std::move(atom);
}

void ContentCatalog::add_relation(ContentRelation relation) {
    if (relation.relation_id.empty()) return;
    std::size_t idx = relations_.size();
    relations_.push_back(relation);
    relations_by_from_[relation.from_content_id].push_back(idx);
    relations_by_to_[relation.to_content_id].push_back(idx);
}

void ContentCatalog::add_variant(ContentVariant variant) {
    if (variant.variant_id.empty()) return;
    std::string cid = variant.content_id;
    variants_by_content_id_[cid].push_back(variant.variant_id);
    variants_[variant.variant_id] = std::move(variant);
}

void ContentCatalog::add_recipe(ExperienceRecipe recipe) {
    if (recipe.recipe_id.empty()) return;
    recipes_[recipe.recipe_id] = std::move(recipe);
}

const ContentAtom* ContentCatalog::find_atom(const std::string& content_id) const noexcept {
    auto it = atoms_.find(content_id);
    return (it != atoms_.end()) ? &it->second : nullptr;
}

std::vector<const ContentAtom*> ContentCatalog::all_atoms() const {
    std::vector<const ContentAtom*> res;
    res.reserve(atoms_.size());
    for (const auto& [_, a] : atoms_) {
        res.push_back(&a);
    }
    return res;
}

std::vector<const ContentAtom*> ContentCatalog::find_by_type(ContentType type) const {
    std::vector<const ContentAtom*> res;
    for (const auto& [_, a] : atoms_) {
        if (a.type == type) res.push_back(&a);
    }
    return res;
}

std::vector<const ContentAtom*> ContentCatalog::find_by_theme(std::string_view theme) const {
    std::vector<const ContentAtom*> res;
    for (const auto& [_, a] : atoms_) {
        for (const auto& t : a.themes) {
            if (t == theme) {
                res.push_back(&a);
                break;
            }
        }
    }
    return res;
}

std::vector<const ContentAtom*> ContentCatalog::find_by_role(ContentRole role) const {
    std::vector<const ContentAtom*> res;
    for (const auto& [_, a] : atoms_) {
        for (auto r : a.supported_roles) {
            if (r == role) {
                res.push_back(&a);
                break;
            }
        }
    }
    return res;
}

std::vector<const ContentRelation*> ContentCatalog::find_relations_from(const std::string& content_id) const {
    std::vector<const ContentRelation*> res;
    auto it = relations_by_from_.find(content_id);
    if (it != relations_by_from_.end()) {
        for (auto idx : it->second) {
            res.push_back(&relations_[idx]);
        }
    }
    return res;
}

std::vector<const ContentRelation*> ContentCatalog::find_relations_to(const std::string& content_id) const {
    std::vector<const ContentRelation*> res;
    auto it = relations_by_to_.find(content_id);
    if (it != relations_by_to_.end()) {
        for (auto idx : it->second) {
            res.push_back(&relations_[idx]);
        }
    }
    return res;
}

std::vector<const ContentVariant*> ContentCatalog::find_variants_for(const std::string& content_id) const {
    std::vector<const ContentVariant*> res;
    auto it = variants_by_content_id_.find(content_id);
    if (it != variants_by_content_id_.end()) {
        for (const auto& vid : it->second) {
            auto vit = variants_.find(vid);
            if (vit != variants_.end()) {
                res.push_back(&vit->second);
            }
        }
    }
    return res;
}

const ContentVariant* ContentCatalog::find_variant(const std::string& variant_id) const noexcept {
    auto it = variants_.find(variant_id);
    return (it != variants_.end()) ? &it->second : nullptr;
}

const ExperienceRecipe* ContentCatalog::find_recipe(const std::string& recipe_id) const noexcept {
    auto it = recipes_.find(recipe_id);
    return (it != recipes_.end()) ? &it->second : nullptr;
}

std::vector<const ExperienceRecipe*> ContentCatalog::all_recipes() const {
    std::vector<const ExperienceRecipe*> res;
    res.reserve(recipes_.size());
    for (const auto& [_, r] : recipes_) {
        res.push_back(&r);
    }
    return res;
}

void ContentCatalog::clear() noexcept {
    manifest_ = {};
    atoms_.clear();
    relations_.clear();
    variants_.clear();
    recipes_.clear();
    relations_by_from_.clear();
    relations_by_to_.clear();
    variants_by_content_id_.clear();
}

} // namespace elo::content
