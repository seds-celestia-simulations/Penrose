#include "CsvTrajectoryLoader.h"

#include "../Geometry/Coordinates.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace viz {

namespace {

constexpr std::size_t kMaxVisualizationSamples = 3000;

std::string trim(const std::string& s) {
    std::size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])) != 0) {
        ++start;
    }
    std::size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])) != 0) {
        --end;
    }
    return s.substr(start, end - start);
}

std::vector<std::string> split_csv_line(const std::string& line) {
    std::vector<std::string> parts;
    std::stringstream ss(line);
    std::string item;
    while (std::getline(ss, item, ',')) {
        parts.push_back(trim(item));
    }
    return parts;
}

CsvSchema detect_schema(const std::vector<std::string>& header) {
    if (header.size() >= 4 && header[0] == "tau" && header[1] == "r" && header[2] == "vt" && header[3] == "vr") {
        return CsvSchema::Freefall;
    }
    if (header.size() >= 7 && header[0] == "tau" && header[1] == "r" && header[2] == "phi") {
        return CsvSchema::Orbital;
    }
    if (header.size() >= 3 && header[0] == "lambda" && header[1] == "r" && header[2] == "phi") {
        return CsvSchema::NullGeodesic;
    }
    return CsvSchema::Unknown;
}

double parse_double(const std::string& s) {
    return std::stod(s);
}

} // namespace

std::optional<CsvLoadResult> load_trajectory_csv(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        return std::nullopt;
    }

    std::string header_line;
    if (!std::getline(in, header_line)) {
        return std::nullopt;
    }

    const std::vector<std::string> header = split_csv_line(header_line);
    const CsvSchema schema = detect_schema(header);
    if (schema == CsvSchema::Unknown) {
        return std::nullopt;
    }

    TrajectoryStyle style;
    std::vector<TrajectorySample> samples;

    std::string line;
    while (std::getline(in, line)) {
        if (trim(line).empty()) {
            continue;
        }
        const std::vector<std::string> cols = split_csv_line(line);
        if (cols.size() < 2) {
            continue;
        }

        TrajectorySample sample;
        double r = 0.0;
        double phi = 0.0;
        const double equatorial_theta = 1.5707963267948966; // pi/2

        switch (schema) {
        case CsvSchema::Freefall:
            if (cols.size() < 2) {
                continue;
            }
            sample.parameter = parse_double(cols[0]);
            r = parse_double(cols[1]);
            phi = 0.0;
            sample.position = spherical_to_cartesian(r, equatorial_theta, phi);
            style.color = Color4::rgb(120, 200, 255);
            break;
        case CsvSchema::Orbital:
            if (cols.size() < 3) {
                continue;
            }
            sample.parameter = parse_double(cols[0]);
            r = parse_double(cols[1]);
            phi = parse_double(cols[2]);
            sample.position = spherical_to_cartesian(r, equatorial_theta, phi);
            style.color = Color4::rgb(255, 220, 80);
            style.glow_color = Color4::rgb(255, 180, 50, 130);
            break;
        case CsvSchema::NullGeodesic:
            if (cols.size() < 3) {
                continue;
            }
            sample.parameter = parse_double(cols[0]);
            r = parse_double(cols[1]);
            phi = parse_double(cols[2]);
            sample.position = spherical_to_cartesian(r, equatorial_theta, phi);
            style.gradient_along_path = true;
            style.gradient_start = Color4::rgb(255, 255, 245);
            style.gradient_end = Color4::rgb(255, 220, 70);
            style.color = style.gradient_end;
            style.glow_color = Color4::rgb(255, 245, 190, 140);
            break;
        default:
            continue;
        }

        samples.push_back(sample);
    }

    if (samples.empty()) {
        return std::nullopt;
    }

    const std::size_t original_count = samples.size();
    decimate_trajectory_samples(samples, kMaxVisualizationSamples);

    std::string name = path;
    const std::size_t slash = path.find_last_of("/\\");
    if (slash != std::string::npos) {
        name = path.substr(slash + 1);
    }

    return CsvLoadResult{Trajectory(name, std::move(samples), style, true), schema, true,
                         original_count > kMaxVisualizationSamples
                             ? "Loaded CSV (decimated to " + std::to_string(kMaxVisualizationSamples) +
                                   " samples for interactive rendering)"
                             : "Loaded CSV with inferred equatorial theta=pi/2"};
}

} // namespace viz
