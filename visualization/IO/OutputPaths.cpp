#include "OutputPaths.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace viz {

namespace {

std::string timestamp_string() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S");
    return oss.str();
}

std::filesystem::path rendered_frames_root(const std::filesystem::path& repo_root) {
    return repo_root / "outputs" / "rendered_frames";
}

} // namespace

std::filesystem::path detect_repo_root() {
#ifdef PENROSE_SOURCE_DIR
    return std::filesystem::path(PENROSE_SOURCE_DIR);
#else
    return std::filesystem::current_path();
#endif
}

std::filesystem::path create_render_run_dir(const std::filesystem::path& repo_root) {
    const std::filesystem::path dir = rendered_frames_root(repo_root) / timestamp_string();
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir;
}

std::filesystem::path default_render_still_path(const std::filesystem::path& repo_root,
                                                const std::string& basename) {
    return create_render_run_dir(repo_root) / basename;
}

std::filesystem::path default_render_sequence_dir(const std::filesystem::path& repo_root) {
    return create_render_run_dir(repo_root);
}

} // namespace viz
