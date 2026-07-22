#pragma once

#include "../Trajectory/Trajectory.h"

#include <state/GeodesicState.h>

#include <string>
#include <vector>

namespace viz {

// Stage 2 input — physics-agnostic trajectory storage.
// The renderer never sees SimulationConfig / integrators / metrics.
struct StoredTrajectory {
    std::vector<State> history;
    std::string name;
    double characteristic_radius = 1.0;

    // Per-particle drawing style. If left default-constructed, prepare_scene may
    // fall back to VisualizationConfig::trajectory_style when use_style is false.
    bool use_style = false;
    TrajectoryStyle style{};
};

} // namespace viz
