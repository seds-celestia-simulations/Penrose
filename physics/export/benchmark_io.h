#pragma once

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#ifndef PENROSE_SOURCE_DIR
#define PENROSE_SOURCE_DIR "."
#endif

inline std::string benchmark_timestamp_string() {
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

inline const std::filesystem::path& benchmark_run_dir() {
    static const std::filesystem::path dir = []() {
        const std::filesystem::path root =
            std::filesystem::path(PENROSE_SOURCE_DIR) / "outputs" / "benchmark_data" /
            benchmark_timestamp_string();
        std::error_code ec;
        std::filesystem::create_directories(root, ec);
        if (ec) {
            std::cerr << "[!] Failed to create benchmark output directory: " << root << " ("
                      << ec.message() << ")\n";
        }
        return root;
    }();
    return dir;
}

inline std::filesystem::path benchmark_data_dir() {
    return benchmark_run_dir();
}

inline std::ofstream open_benchmark_csv(const std::string& filename) {
    const std::filesystem::path path = benchmark_run_dir() / filename;

    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    if (ec) {
        std::cerr << "[!] Failed to create output directory: " << path.parent_path() << " ("
                  << ec.message() << ")\n";
        return {};
    }

    std::ofstream out(path);
    if (!out.is_open()) {
        std::cerr << "[!] Failed to open CSV for writing: " << path << "\n";
    }
    return out;
}
