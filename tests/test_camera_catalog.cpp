#include "elo/perception/camera_catalog.hpp"

#include <cstdlib>
#include <iostream>
#include <vector>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion FAILED: " << msg << " (" << #cond << ")\n"; \
            std::abort(); \
        } \
    } while (0)

int main() {
    using elo::perception::CameraDevice;

    const std::vector<CameraDevice> cameras{
        {
            .id = "/dev/v4l/by-id/camera-front",
            .display_name = "Front Camera",
            .backend = "v4l2",
            .device_path = "/dev/video0"
        },
        {
            .id = "platform/rpi/csi0",
            .display_name = "CSI Camera",
            .backend = "libcamera",
            .device_path = "platform/rpi/csi0"
        }
    };

    TEST_ASSERT(!elo::perception::select_camera({}, "0").has_value(),
                "Selection fails when no camera exists");
    TEST_ASSERT(!elo::perception::select_camera(cameras, "").has_value(),
                "Multiple cameras require an explicit selection");

    auto by_index = elo::perception::select_camera(cameras, "1");
    TEST_ASSERT(by_index && by_index->display_name == "CSI Camera",
                "Camera can be selected by zero-based index");

    auto by_id = elo::perception::select_camera(cameras, "/dev/v4l/by-id/camera-front");
    TEST_ASSERT(by_id && by_id->device_path == "/dev/video0",
                "Camera can be selected by stable id");

    auto by_path = elo::perception::select_camera(cameras, "/dev/video0");
    TEST_ASSERT(by_path && by_path->display_name == "Front Camera",
                "Camera can be selected by device path");

    auto by_name = elo::perception::select_camera(cameras, "CSI Camera");
    TEST_ASSERT(by_name && by_name->backend == "libcamera",
                "Camera can be selected by unique display name");

    TEST_ASSERT(!elo::perception::select_camera(cameras, "9").has_value(),
                "Out-of-range camera index is rejected");
    TEST_ASSERT(!elo::perception::select_camera(cameras, "missing").has_value(),
                "Unknown camera selector is rejected");

    const std::vector<CameraDevice> one_camera{cameras.front()};
    auto automatic = elo::perception::select_camera(one_camera, "");
    TEST_ASSERT(automatic && automatic->id == cameras.front().id,
                "A single available camera can be selected automatically");

    std::cout << "Camera catalog selection tests passed.\n";
    return 0;
}
