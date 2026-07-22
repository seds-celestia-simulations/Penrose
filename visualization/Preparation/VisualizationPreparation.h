#pragma once

#include "StoredTrajectory.h"
#include "VisualizationPreparationSettings.h"
#include "../Presentation/VisualizationConfig.h"
#include "../Scene/Scene.h"

#include <span>
#include <vector>

namespace Simulation {
struct SimulationResult;
}

namespace viz {

// Stage 2 — Visualization Preparation.
// Converts stored physics trajectories into renderable scene geometry.
// Currently a pass-through adapter; interpolation hooks live here for later work.
Scene prepare_scene(std::span<const StoredTrajectory> trajectories,
                    const VisualizationConfig& config);

Scene prepare_scene(const std::vector<StoredTrajectory>& trajectories,
                    const VisualizationConfig& config);

// Convenience: store physics results then prepare (still pass-through).
std::vector<StoredTrajectory> store_trajectories(
    std::span<const Simulation::SimulationResult> results,
    std::span<const TrajectoryStyle> styles = {});

std::vector<StoredTrajectory> store_trajectories(
    const std::vector<Simulation::SimulationResult>& results,
    std::span<const TrajectoryStyle> styles = {});

Scene prepare_scene(std::span<const Simulation::SimulationResult> results,
                    const VisualizationConfig& config,
                    std::span<const TrajectoryStyle> styles = {});

Scene prepare_scene(const std::vector<Simulation::SimulationResult>& results,
                    const VisualizationConfig& config,
                    std::span<const TrajectoryStyle> styles = {});

} // namespace viz
