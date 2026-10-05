#pragma once

#include "elo/core/result.hpp"

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace elo::perception {

/// @brief A camera advertised by the active platform backend.
/// Discovery does not open a capture session or read frames.
struct CameraDevice {
    std::string id;
    std::string display_name;
    std::string backend;
    std::string device_path;
};

struct CameraDiscoveryReport {
    std::vector<CameraDevice> devices;
    std::vector<std::string> warnings;
};

/// @brief Discover cameras through libcamera when available, otherwise V4L2
/// on Linux. This operation never captures image data.
[[nodiscard]] CameraDiscoveryReport discover_cameras();

/// @brief Select by zero-based index, stable id, device path, or unique name.
[[nodiscard]] core::Result<CameraDevice> select_camera(
    std::span<const CameraDevice> devices,
    std::string_view selector);

} // namespace elo::perception
