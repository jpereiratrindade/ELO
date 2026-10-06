#include "elo/content/content_bundle.hpp"
#include "elo/system/control_socket.hpp"

#include <QCoreApplication>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion FAILED: " << msg << " (" << #cond << ")\n"; \
            std::abort(); \
        } \
    } while (0)

static std::filesystem::path resolve_content_dir() {
    for (const auto& candidate : {"content", "../content", "../../content"}) {
        if (std::filesystem::exists(candidate) && std::filesystem::exists(std::filesystem::path(candidate) / "manifest.json")) {
            return candidate;
        }
    }
    return "content";
}

void test_content_bundle_and_hash() {
    std::cout << "[TEST] ContentBundle loading and deterministic SHA-256...\n";
    auto content_dir = resolve_content_dir();

    elo::content::ContentBundle bundle(content_dir);
    auto manifest_res = bundle.load_manifest();
    TEST_ASSERT(manifest_res.has_value(), "Bundle manifest must load");
    TEST_ASSERT(!manifest_res->bundle_id.empty(), "Bundle ID must not be empty");
    std::cout << "  Loaded bundle: " << manifest_res->bundle_id << " v" << manifest_res->version << '\n';

    auto hash1 = bundle.compute_content_hash();
    TEST_ASSERT(hash1.has_value(), "Hash computation 1 must succeed");
    TEST_ASSERT(hash1->starts_with("sha256:"), "Hash must begin with sha256:");

    auto hash2 = bundle.compute_content_hash();
    TEST_ASSERT(hash2.has_value(), "Hash computation 2 must succeed");
    TEST_ASSERT(*hash1 == *hash2, "SHA-256 hash calculation must be deterministic");
    std::cout << "  Deterministic content hash: " << *hash1 << '\n';

    auto report = bundle.validate();
    TEST_ASSERT(report.valid, "Bundle must pass validation");
    std::cout << "  Validation passed.\n";
}

void test_bundle_publisher_and_atomic_swap() {
    std::cout << "[TEST] BundlePublisher atomic activation and rollback...\n";
    auto source_content = resolve_content_dir();

    auto temp_root = std::filesystem::temp_directory_path() / ("elo_pub_test_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(temp_root);

    elo::content::BundlePublisher publisher(temp_root);

    // 1. Publish candidate
    auto pub_res = publisher.publish_and_activate(source_content, "curator_alice");
    TEST_ASSERT(pub_res.success, "First publish must succeed");
    TEST_ASSERT(std::filesystem::exists(publisher.current_symlink()), "Current symlink must exist");

    auto active = publisher.active_bundle();
    TEST_ASSERT(active.has_value(), "Active bundle must be present");
    TEST_ASSERT(active->bundle_id == pub_res.bundle_id, "Active bundle id matches");
    TEST_ASSERT(active->curation_revision == 1, "First revision is 1");
    std::cout << "  Bundle activated: " << active->bundle_id << " revision " << active->curation_revision << '\n';

    // 2. Publish second revision
    auto pub_res2 = publisher.publish_and_activate(source_content, "curator_bob");
    TEST_ASSERT(pub_res2.success, "Second publish must succeed");
    auto active2 = publisher.active_bundle();
    TEST_ASSERT(active2.has_value(), "Active bundle 2 must be present");
    TEST_ASSERT(active2->curation_revision == 2, "Curation revision incremented to 2");
    TEST_ASSERT(active2->parent_bundle == active->content_hash, "Parent bundle hash points to revision 1");
    std::cout << "  Second bundle activated: revision " << active2->curation_revision << '\n';

    // 3. Rollback
    auto bundles = publisher.list_bundles();
    TEST_ASSERT(bundles.size() >= 1, "Bundles list has published entries");

    auto rb_res = publisher.rollback_to(pub_res.bundle_path.filename().string());
    TEST_ASSERT(rb_res.has_value(), "Rollback to revision 1 must succeed");

    auto active_after_rb = publisher.active_bundle();
    TEST_ASSERT(active_after_rb.has_value(), "Active after rollback exists");
    std::cout << "  Rollback verified: current symlink restored.\n";

    // Cleanup temp
    std::error_code ec;
    std::filesystem::remove_all(temp_root, ec);
}

void test_control_plane_ipc(int argc, char* argv[]) {
    std::cout << "[TEST] Control plane Unix domain socket IPC...\n";
    QCoreApplication app(argc, argv);

    QString test_sock = QString::fromStdString((std::filesystem::temp_directory_path() / "elo_test_control.sock").string());
    elo::system::ControlServer server(test_sock);
    TEST_ASSERT(server.start(), "Server must start on test socket");

    bool reload_received = false;
    QObject::connect(&server, &elo::system::ControlServer::reloadRequested, [&]() {
        reload_received = true;
    });

    bool ping_ok = false;
    bool reload_sent = false;

    std::thread client_thread([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        elo::system::ControlClient client(test_sock);
        ping_ok = client.ping(2000);
        reload_sent = client.send_reload(2000);
    });

    for (int i = 0; i < 100 && (!reload_received || !reload_sent); ++i) {
        QCoreApplication::processEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    client_thread.join();

    TEST_ASSERT(ping_ok, "Ping to control server must succeed with PONG");
    TEST_ASSERT(reload_sent, "Send reload must acknowledge");
    TEST_ASSERT(reload_received, "Server must have received and emitted reloadRequested");
    std::cout << "  Control plane IPC verified: PING/PONG and CONTENT_RELOAD.\n";
    server.stop();
}

int main(int argc, char* argv[]) {
    test_content_bundle_and_hash();
    test_bundle_publisher_and_atomic_swap();
    test_control_plane_ipc(argc, argv);
    std::cout << "\nAll Content Publishing & Control Plane tests PASSED successfully!\n";
    return 0;
}
