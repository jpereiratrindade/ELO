#include "elo/content/content_bundle.hpp"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QString>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <unistd.h>

namespace elo::content {

namespace {

BundleManifest read_manifest(const std::filesystem::path& path) {
    BundleManifest manifest;
    QFile file(QString::fromStdString(path.string()));
    if (!file.open(QIODevice::ReadOnly)) {
        return manifest;
    }
    const auto doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        return manifest;
    }
    const auto obj = doc.object();
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

bool write_manifest(const std::filesystem::path& path, const BundleManifest& manifest) {
    QJsonObject obj;
    obj[QStringLiteral("bundle_id")] = QString::fromStdString(manifest.bundle_id);
    obj[QStringLiteral("version")] = QString::fromStdString(manifest.version);
    obj[QStringLiteral("schema_version")] = QString::fromStdString(manifest.schema_version);
    obj[QStringLiteral("title")] = QString::fromStdString(manifest.title);
    obj[QStringLiteral("default_theme")] = QString::fromStdString(manifest.default_theme);
    obj[QStringLiteral("description")] = QString::fromStdString(manifest.description);
    obj[QStringLiteral("license")] = QString::fromStdString(manifest.license);
    obj[QStringLiteral("curation_revision")] = static_cast<qint64>(manifest.curation_revision);
    obj[QStringLiteral("content_hash")] = QString::fromStdString(manifest.content_hash);
    obj[QStringLiteral("parent_bundle")] = QString::fromStdString(manifest.parent_bundle);
    obj[QStringLiteral("created_at")] = QString::fromStdString(manifest.created_at);

    QFile file(QString::fromStdString(path.string()));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    return true;
}

} // namespace

ContentBundle::ContentBundle(std::filesystem::path root_dir)
    : root_dir_(std::move(root_dir)) {}

core::Result<BundleManifest> ContentBundle::load_manifest() const {
    if (!std::filesystem::exists(manifest_path())) {
        return std::unexpected(core::make_error(
            core::ErrorCode::ContentError,
            "Manifest file not found: " + manifest_path().string()));
    }
    return read_manifest(manifest_path());
}

core::Result<ContentCatalog> ContentBundle::load_catalog() const {
    ContentCatalog catalog;
    auto target_dir = catalog_path();
    if (!std::filesystem::exists(target_dir)) {
        target_dir = root_dir_;
    }
    auto res = catalog.load_from_directory(target_dir);
    if (!res) return std::unexpected(res.error());
    return catalog;
}

core::Result<std::string> ContentBundle::compute_content_hash() const {
    QCryptographicHash hash(QCryptographicHash::Sha256);

    std::vector<std::filesystem::path> all_files;
    std::error_code ec;

    for (const auto& sub : {catalog_path(), assets_path()}) {
        if (!std::filesystem::exists(sub, ec)) continue;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(sub, ec)) {
            if (entry.is_regular_file(ec)) {
                all_files.push_back(entry.path());
            }
        }
    }

    std::sort(all_files.begin(), all_files.end());

    for (const auto& fpath : all_files) {
        auto rel = std::filesystem::relative(fpath, root_dir_).string();
        hash.addData(QByteArrayView(rel.data(), static_cast<qsizetype>(rel.size())));

        QFile f(QString::fromStdString(fpath.string()));
        if (f.open(QIODevice::ReadOnly)) {
            while (!f.atEnd()) {
                auto chunk = f.read(65536);
                hash.addData(chunk);
            }
        }
    }

    return "sha256:" + hash.result().toHex().toStdString();
}

ValidationReport ContentBundle::validate() const {
    auto cat_res = load_catalog();
    if (!cat_res) {
        ValidationReport report;
        report.valid = false;
        report.errors.push_back(ValidationError{
            .item_id = "bundle",
            .message = cat_res.error().to_string(),
            .is_critical = true
        });
        return report;
    }

    auto report = ContentValidator::validate(*cat_res);

    // Verify physical asset existence
    std::filesystem::path assets_dir = assets_path();
    for (const auto* atom : cat_res->all_atoms()) {
        for (const auto& img : atom->assets.images) {
            std::filesystem::path p = assets_dir / img;
            if (!std::filesystem::exists(p) && !std::filesystem::exists(root_dir_ / img)) {
                report.warnings.push_back("Referenced image missing: " + img + " in atom " + atom->content_id);
            }
        }
        for (const auto& aud : atom->assets.audios) {
            std::filesystem::path p = assets_dir / aud;
            if (!std::filesystem::exists(p) && !std::filesystem::exists(root_dir_ / aud)) {
                report.warnings.push_back("Referenced audio missing: " + aud + " in atom " + atom->content_id);
            }
        }
    }

    return report;
}

BundlePublisher::BundlePublisher(std::filesystem::path content_root_dir)
    : root_dir_(std::move(content_root_dir)) {
    std::error_code ec;
    std::filesystem::create_directories(bundles_dir(), ec);
    std::filesystem::create_directories(staging_dir(), ec);
}

std::vector<BundleManifest> BundlePublisher::list_bundles() const {
    std::vector<BundleManifest> list;
    std::error_code ec;
    if (!std::filesystem::exists(bundles_dir(), ec)) return list;

    for (const auto& entry : std::filesystem::directory_iterator(bundles_dir(), ec)) {
        if (entry.is_directory(ec)) {
            auto m_path = entry.path() / "manifest.json";
            if (std::filesystem::exists(m_path, ec)) {
                list.push_back(read_manifest(m_path));
            }
        }
    }

    std::sort(list.begin(), list.end(), [](const BundleManifest& a, const BundleManifest& b) {
        return a.curation_revision > b.curation_revision;
    });
    return list;
}

std::optional<BundleManifest> BundlePublisher::active_bundle() const {
    std::error_code ec;
    auto cur = current_symlink();
    if (!std::filesystem::exists(cur, ec)) return std::nullopt;

    auto resolved = std::filesystem::canonical(cur, ec);
    if (ec) return std::nullopt;

    auto m_path = resolved / "manifest.json";
    if (!std::filesystem::exists(m_path, ec)) return std::nullopt;

    return read_manifest(m_path);
}

core::Result<void> BundlePublisher::atomic_activate(
    const std::filesystem::path& current_link,
    const std::filesystem::path& target_bundle_path) {

    std::error_code ec;
    if (!std::filesystem::exists(target_bundle_path, ec)) {
        return std::unexpected(core::make_error(
            core::ErrorCode::ContentError,
            "Target bundle does not exist: " + target_bundle_path.string()));
    }

    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    auto tmp_link_name = "current.tmp." + std::to_string(::getpid()) + "." + std::to_string(now);
    auto tmp_link = current_link.parent_path() / tmp_link_name;

    // Remove stale tmp if exists
    std::filesystem::remove(tmp_link, ec);

    // Create symlink to target
    std::filesystem::create_directory_symlink(target_bundle_path, tmp_link, ec);
    if (ec) {
        return std::unexpected(core::make_error(
            core::ErrorCode::ContentError,
            "Failed to create temporary symlink: " + ec.message()));
    }

    // Atomic POSIX rename replaces current_link atomically
    std::filesystem::rename(tmp_link, current_link, ec);
    if (ec) {
        std::filesystem::remove(tmp_link, ec);
        return std::unexpected(core::make_error(
            core::ErrorCode::ContentError,
            "Atomic rename failed: " + ec.message()));
    }

    return {};
}

BundlePublishResult BundlePublisher::publish_and_activate(
    const std::filesystem::path& candidate_dir,
    std::string_view creator) {
    (void)creator;

    BundlePublishResult res;
    ContentBundle candidate(candidate_dir);

    // 1. Validate candidate bundle
    auto report = candidate.validate();
    if (!report.valid) {
        res.success = false;
        std::string errs;
        for (const auto& e : report.errors) {
            if (!errs.empty()) errs += "; ";
            errs += e.item_id + ": " + e.message;
        }
        res.error_message = "Validation failed: " + errs;
        return res;
    }

    // 2. Load manifest and calculate hash
    auto m_res = candidate.load_manifest();
    if (!m_res) {
        res.success = false;
        res.error_message = m_res.error().to_string();
        return res;
    }
    auto manifest = *m_res;

    auto hash_res = candidate.compute_content_hash();
    if (!hash_res) {
        res.success = false;
        res.error_message = hash_res.error().to_string();
        return res;
    }

    auto active = active_bundle();
    if (active) {
        manifest.parent_bundle = active->content_hash;
        manifest.curation_revision = active->curation_revision + 1;
    } else {
        manifest.curation_revision = 1;
    }

    manifest.content_hash = *hash_res;
    manifest.created_at = QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString();

    // 3. Prepare target folder in bundles/
    std::string target_folder_name = manifest.bundle_id + "-" + manifest.version;
    auto target_dir = bundles_dir() / target_folder_name;

    std::error_code ec;
    std::filesystem::remove_all(target_dir, ec);
    std::filesystem::create_directories(target_dir, ec);

    // Copy canonical bundle constituents (skip bundles/, staging/, current symlink)
    const std::vector<std::string> constituents = {"manifest.json", "catalog", "assets", "sources"};
    for (const auto& item : constituents) {
        auto src_item = candidate_dir / item;
        if (std::filesystem::exists(src_item, ec)) {
            auto dst_item = target_dir / item;
            std::filesystem::copy(src_item, dst_item,
                                  std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing, ec);
            if (ec) {
                res.success = false;
                res.error_message = "Failed to copy " + item + " to target: " + ec.message();
                return res;
            }
        }
    }

    // Update sealed manifest with new hash and timestamps
    if (!write_manifest(target_dir / "manifest.json", manifest)) {
        res.success = false;
        res.error_message = "Failed to write sealed manifest in target bundle";
        return res;
    }

    // 4. Atomic symlink activation
    auto act_res = atomic_activate(current_symlink(), target_dir);
    if (!act_res) {
        res.success = false;
        res.error_message = act_res.error().to_string();
        return res;
    }

    res.success = true;
    res.bundle_id = manifest.bundle_id;
    res.version = manifest.version;
    res.content_hash = manifest.content_hash;
    res.bundle_path = target_dir;
    res.current_link = current_symlink();
    return res;
}

core::Result<void> BundlePublisher::rollback_to(const std::string& bundle_dir_or_version) {
    std::error_code ec;
    std::filesystem::path target = bundles_dir() / bundle_dir_or_version;
    if (!std::filesystem::exists(target, ec)) {
        // Try finding by version prefix
        for (const auto& entry : std::filesystem::directory_iterator(bundles_dir(), ec)) {
            if (entry.is_directory(ec) && entry.path().filename().string().find(bundle_dir_or_version) != std::string::npos) {
                target = entry.path();
                break;
            }
        }
    }

    if (!std::filesystem::exists(target, ec)) {
        return std::unexpected(core::make_error(
            core::ErrorCode::ContentError,
            "Rollback target bundle not found: " + bundle_dir_or_version));
    }

    return atomic_activate(current_symlink(), target);
}

std::filesystem::path resolve_system_content_dir(bool bootstrap_from_seed) {
    auto env = QProcessEnvironment::systemEnvironment();
    QString custom = env.value(QStringLiteral("ELO_CONTENT_DIR"));
    if (custom.isEmpty()) {
        custom = env.value(QStringLiteral("ELO_CONTENT_ROOT"));
    }
    if (!custom.isEmpty()) {
        std::filesystem::path p(custom.toStdString());
        std::error_code ec;
        std::filesystem::create_directories(p, ec);
        return p;
    }

    std::filesystem::path target_dir;

    // 1. Production kiosk system path: /var/lib/elo/content
    QFileInfo varLibFi(QStringLiteral("/var/lib/elo/content"));
    if (varLibFi.exists() && varLibFi.isWritable()) {
        target_dir = "/var/lib/elo/content";
    } else {
        // 2. Standard XDG location: $XDG_DATA_HOME/elo/content (default: ~/.local/share/elo/content)
        QString xdg = env.value(QStringLiteral("XDG_DATA_HOME"));
        QString userContentDir;
        if (!xdg.isEmpty()) {
            userContentDir = QDir(xdg).filePath(QStringLiteral("elo/content"));
        } else {
            userContentDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
            if (userContentDir.isEmpty()) {
                userContentDir = QDir::home().filePath(QStringLiteral(".local/share"));
            }
            userContentDir = QDir(userContentDir).filePath(QStringLiteral("elo/content"));
        }
        target_dir = userContentDir.toStdString();
    }

    std::error_code ec;
    std::filesystem::create_directories(target_dir, ec);

    // 3. Initialize default clean manifest and catalog layout if target is empty
    if (!std::filesystem::exists(target_dir / "manifest.json", ec)) {
        std::filesystem::create_directories(target_dir / "catalog" / "atoms", ec);
        std::filesystem::create_directories(target_dir / "catalog" / "relations", ec);
        std::filesystem::create_directories(target_dir / "catalog" / "recipes", ec);
        std::filesystem::create_directories(target_dir / "catalog" / "variants", ec);
        std::filesystem::create_directories(target_dir / "assets" / "images", ec);
        std::filesystem::create_directories(target_dir / "assets" / "audio", ec);
        std::filesystem::create_directories(target_dir / "sources", ec);

        std::ofstream mf(target_dir / "manifest.json");
        if (mf.is_open()) {
            mf << "{\n"
               << "  \"bundle_id\": \"elo-sovereign-content\",\n"
               << "  \"version\": \"1.0.0\",\n"
               << "  \"title\": \"ELO — Catálogo Soberano\",\n"
               << "  \"default_theme\": \"pampa\",\n"
               << "  \"description\": \"Repositório soberano de conteúdo local.\"\n"
               << "}\n";
        }
    }

    return target_dir;
}

} // namespace elo::content
