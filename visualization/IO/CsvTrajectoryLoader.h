#pragma once

#include "../Trajectory/Trajectory.h"

#include <optional>
#include <string>

namespace viz {

enum class CsvSchema {
    Freefall,
    Orbital,
    NullGeodesic,
    Unknown
};

struct CsvLoadResult {
    Trajectory trajectory;
    CsvSchema schema = CsvSchema::Unknown;
    bool inferred_equatorial = false;
    std::string message;
};

// Optional offline adapter for benchmark CSV outputs.
std::optional<CsvLoadResult> load_trajectory_csv(const std::string& path);

} // namespace viz
