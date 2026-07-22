#include "VisualizationPreparation.h"

#include "../Trajectory/TrajectoryAdapter.h"

#include <simulation/SimulationConfig.h>

#include <algorithm>
#include <cmath>

namespace viz {
namespace {

TrajectoryStyle resolve_style(const StoredTrajectory& trajectory,
                              const VisualizationConfig& config) {
    return trajectory.use_style ? trajectory.style : config.trajectory_style;
}

// Pass-through preparation. Future: interpolate / resample using
// config.preparation before adapting to Cartesian Trajectory samples.
Trajectory prepare_renderable(const StoredTrajectory& stored,
                              const VisualizationConfig& config) {
    // Reserved knobs — intentionally unused until interpolation lands.
    (void)config.preparation.interpolation_method;
    (void)config.preparation.render_samples_per_segment;
    (void)config.preparation.trajectory_resolution;

    TrajectoryAdapterOptions options;
    options.name = stored.name;
    options.style = resolve_style(stored, config);
    return adapt_states(stored.history, options);
}

float scene_horizon_radius(std::span<const StoredTrajectory> trajectories,
                           const VisualizationConfig& config) {
    float radius = config.scene.horizon_radius;
    for (const StoredTrajectory& trajectory : trajectories) {
        radius = std::max(radius, static_cast<float>(trajectory.characteristic_radius));
    }
    return radius;
}

} // namespace

Scene prepare_scene(std::span<const StoredTrajectory> trajectories,
                    const VisualizationConfig& config) {
    SceneSettings settings = config.scene;
    settings.horizon_radius = scene_horizon_radius(trajectories, config);

    Scene scene(settings);
    for (const StoredTrajectory& stored : trajectories) {
        scene.add_trajectory(prepare_renderable(stored, config));
    }
    return scene;
}

Scene prepare_scene(const std::vector<StoredTrajectory>& trajectories,
                    const VisualizationConfig& config) {
    return prepare_scene(std::span<const StoredTrajectory>(trajectories), config);
}

std::vector<StoredTrajectory> store_trajectories(
    std::span<const Simulation::SimulationResult> results,
    std::span<const TrajectoryStyle> styles) {
    std::vector<StoredTrajectory> stored;
    stored.reserve(results.size());
    for (std::size_t i = 0; i < results.size(); ++i) {
        StoredTrajectory trajectory;
        trajectory.history = results[i].history;
        trajectory.name = results[i].name;
        trajectory.characteristic_radius = results[i].characteristic_radius;
        if (i < styles.size()) {
            trajectory.use_style = true;
            trajectory.style = styles[i];
        }
        stored.push_back(std::move(trajectory));
    }
    return stored;
}

std::vector<StoredTrajectory> store_trajectories(
    const std::vector<Simulation::SimulationResult>& results,
    std::span<const TrajectoryStyle> styles) {
    return store_trajectories(std::span<const Simulation::SimulationResult>(results), styles);
}

Scene prepare_scene(std::span<const Simulation::SimulationResult> results,
                    const VisualizationConfig& config,
                    std::span<const TrajectoryStyle> styles) {
    return prepare_scene(store_trajectories(results, styles), config);
}

Scene prepare_scene(const std::vector<Simulation::SimulationResult>& results,
                    const VisualizationConfig& config,
                    std::span<const TrajectoryStyle> styles) {
    return prepare_scene(std::span<const Simulation::SimulationResult>(results), config, styles);
}

} // namespace viz
