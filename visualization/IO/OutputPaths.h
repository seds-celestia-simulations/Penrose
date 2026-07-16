#pragma once

#include <filesystem>
#include <string>

namespace viz {

// Timestamped run directory under outputs/rendered_frames/.
std::filesystem::path create_render_run_dir(const std::filesystem::path& repo_root);

// Default still path inside a new timestamped render run directory.
std::filesystem::path default_render_still_path(const std::filesystem::path& repo_root,
                                                const std::string& basename = "frame.ppm");

// Default frame sequence directory under outputs/rendered_frames/<timestamp>/.
std::filesystem::path default_render_sequence_dir(const std::filesystem::path& repo_root);

std::filesystem::path detect_repo_root();

} // namespace viz
