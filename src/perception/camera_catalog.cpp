#include "elo/perception/camera_catalog.hpp"

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <set>

#if defined(ELO_HAS_LIBCAMERA)
#include <libcamera/libcamera.h>
#elif defined(__linux__)
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace elo::perception {
namespace {

core::Error selection_error(std::string_view message, std::string_view detail = {}) {
    return core::make_error(core::ErrorCode::InvalidOperation, message, detail);
}

#if defined(__linux__) && !defined(ELO_HAS_LIBCAMERA)
std::string field_text(const __u8* field, std::size_t capacity) {
    const auto* text = reinterpret_cast<const char*>(field);
    return std::string(text, ::strnlen(text, capacity));
}

bool query_capabilities(const std::filesystem::path& path, v4l2_capability& capabilities,
                        std::string& warning) {
    const int descriptor = ::open(path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (descriptor < 0) {
        warning = "Cannot open " + path.string() + ": " + std::strerror(errno);
        return false;
    }

    int result = 0;
    do {
        result = ::ioctl(descriptor, VIDIOC_QUERYCAP, &capabilities);
    } while (result < 0 && errno == EINTR);

    const int query_error = errno;
    ::close(descriptor);

    if (result < 0) {
        if (query_error == EACCES || query_error == EPERM) {
            warning = "Cannot inspect " + path.string() + ": " + std::strerror(query_error);
        }
        return false;
    }
    return true;
}

void append_video_paths(const std::filesystem::path& directory,
                        std::vector<std::filesystem::path>& paths,
                        bool only_video_nodes) {
    std::error_code error;
    if (!std::filesystem::exists(directory, error)) {
        return;
    }

    for (std::filesystem::directory_iterator it(directory, error), end; it != end && !error;
         it.increment(error)) {
        const auto filename = it->path().filename().string();
        if (only_video_nodes && !filename.starts_with("video")) {
            continue;
        }
        if (only_video_nodes && (filename.size() == 5 ||
            !std::all_of(filename.begin() + 5, filename.end(), [](unsigned char character) {
                return character >= '0' && character <= '9';
            }))) {
            continue;
        }
        paths.push_back(it->path());
    }
}
#endif

} // namespace

CameraDiscoveryReport discover_cameras() {
    CameraDiscoveryReport report;

#if defined(ELO_HAS_LIBCAMERA)
    libcamera::CameraManager manager;
    const int start_result = manager.start();
    if (start_result != 0) {
        report.warnings.push_back(
            "libcamera failed to start (error " + std::to_string(start_result) + ")");
        return report;
    }

    for (const auto& camera : manager.cameras()) {
        const auto camera_id = camera->id();
        report.devices.push_back(CameraDevice{
            .id = camera_id,
            .display_name = camera_id,
            .backend = "libcamera",
            .device_path = camera_id
        });
    }
    manager.stop();
#elif defined(__linux__)
    std::vector<std::filesystem::path> candidates;
    // Prefer persistent udev links when they exist. Canonical /dev/videoN
    // nodes are added afterwards as a portable fallback.
    append_video_paths("/dev/v4l/by-id", candidates, false);
    append_video_paths("/dev", candidates, true);
    std::sort(candidates.begin(), candidates.end());

    std::set<std::string> inspected_nodes;
    for (const auto& candidate : candidates) {
        std::error_code canonical_error;
        const auto canonical = std::filesystem::canonical(candidate, canonical_error);
        const auto query_path = canonical_error ? candidate : canonical;
        if (!inspected_nodes.insert(query_path.string()).second) {
            continue;
        }

        v4l2_capability capabilities{};
        std::string warning;
        if (!query_capabilities(query_path, capabilities, warning)) {
            if (!warning.empty()) {
                report.warnings.push_back(std::move(warning));
            }
            continue;
        }

        const auto flags = (capabilities.capabilities & V4L2_CAP_DEVICE_CAPS)
            ? capabilities.device_caps
            : capabilities.capabilities;
        const bool captures_video = (flags & V4L2_CAP_VIDEO_CAPTURE) != 0 ||
                                    (flags & V4L2_CAP_VIDEO_CAPTURE_MPLANE) != 0;
        const bool outputs_video = (flags & V4L2_CAP_VIDEO_OUTPUT) != 0 ||
                                   (flags & V4L2_CAP_VIDEO_OUTPUT_MPLANE) != 0;
        if (!captures_video || outputs_video) {
            continue;
        }

        const auto preferred_path = candidate.parent_path() == "/dev/v4l/by-id"
            ? candidate
            : query_path;
        auto display_name = field_text(capabilities.card, sizeof(capabilities.card));
        if (display_name.empty()) {
            display_name = preferred_path.filename().string();
        }

        report.devices.push_back(CameraDevice{
            .id = preferred_path.string(),
            .display_name = std::move(display_name),
            .backend = "v4l2",
            .device_path = query_path.string()
        });
    }
#else
    report.warnings.emplace_back("Camera discovery is not implemented for this platform");
#endif

    return report;
}

core::Result<CameraDevice> select_camera(
    std::span<const CameraDevice> devices,
    std::string_view selector) {
    if (devices.empty()) {
        return std::unexpected(selection_error("No camera is available"));
    }

    if (selector.empty()) {
        if (devices.size() == 1) {
            return devices.front();
        }
        return std::unexpected(selection_error(
            "More than one camera is available; an explicit selection is required"));
    }

    std::size_t index = 0;
    const auto* begin = selector.data();
    const auto* end = begin + selector.size();
    const auto [position, conversion_error] = std::from_chars(begin, end, index);
    if (conversion_error == std::errc{} && position == end) {
        if (index < devices.size()) {
            return devices[index];
        }
        return std::unexpected(selection_error("Camera index is out of range", selector));
    }

    const auto exact = std::find_if(devices.begin(), devices.end(), [selector](const auto& device) {
        return device.id == selector || device.device_path == selector;
    });
    if (exact != devices.end()) {
        return *exact;
    }

    const CameraDevice* name_match = nullptr;
    for (const auto& device : devices) {
        if (device.display_name != selector) {
            continue;
        }
        if (name_match != nullptr) {
            return std::unexpected(selection_error(
                "Camera name is ambiguous; use its index or id", selector));
        }
        name_match = &device;
    }
    if (name_match != nullptr) {
        return *name_match;
    }

    return std::unexpected(selection_error("Camera selector did not match any device", selector));
}

} // namespace elo::perception
