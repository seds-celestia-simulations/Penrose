#include "SceneBuilder.h"

#include "../Trajectory/TrajectoryAdapter.h"
#include <state/GeodesicState.h>

namespace viz {

Scene SceneBuilder::default_schwarzschild_scene(float rs) {
    SceneSettings settings;
    settings.schwarzschild_radius = rs;
    return Scene(settings);
}

Scene SceneBuilder::from_states(std::span<const State> states, const std::string& name) {
    Scene scene = default_schwarzschild_scene();
    TrajectoryAdapterOptions options;
    options.name = name;
    scene.add_trajectory(adapt_states(states, options));
    return scene;
}

Scene SceneBuilder::from_states(const std::vector<State>& states, const std::string& name) {
    return from_states(std::span<const State>(states), name);
}

Scene SceneBuilder::from_trajectories(std::vector<Trajectory> trajectories, float rs) {
    Scene scene = default_schwarzschild_scene(rs);
    for (Trajectory& traj : trajectories) {
        scene.add_trajectory(std::move(traj));
    }
    return scene;
}

} // namespace viz
