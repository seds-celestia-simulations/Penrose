#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#ifndef PENROSE_SOURCE_DIR
#define PENROSE_SOURCE_DIR "."
#endif

inline std::filesystem::path benchmark_data_dir() {
    return std::filesystem::path(PENROSE_SOURCE_DIR) / "src" / "benchmarking" / "data";
}

inline std::ofstream open_benchmark_csv(const std::string& filename) {
    const std::filesystem::path path = benchmark_data_dir() / filename;

    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    if (ec) {
        std::cerr << "[!] Failed to create output directory: "
                  << path.parent_path() << " (" << ec.message() << ")\n";
        return {};
    }

    std::ofstream out(path);
    if (!out.is_open()) {
        std::cerr << "[!] Failed to open CSV for writing: " << path << "\n";
    }
    return out;
}
