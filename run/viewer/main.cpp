/**
 * Interactive viewer entry point.
 *
 * Pipeline:
 *   Physics (independent SimulationRequest per particle)
 *        ↓
 *   Trajectory storage (SimulationResult / PhysicsTrajectory)
 *        ↓
 *   Visualization preparation (prepare_scene — pass-through today)
 *        ↓
 *   Rendering (ViewerApp)
 *
 *   cmake --build build --target visualization_viewer
 *   ./build/visualization_viewer
 */

#include "simulation/SimulationRequest.h"

#include "Apps/ViewerApp.h"
#include "Preparation/VisualizationPreparation.h"
#include "Presentation/VisualizationConfig.h"

#include <iostream>
#include <vector>

int main() {
    // =========================================================================
    // STAGE 1 — PHYSICS (one independent request per particle)
    // =========================================================================

    Simulation::SimulationRequest particle_a;
    particle_a.config.spacetime = Simulation::SpacetimeKind::Schwarzschild;
    particle_a.config.scenario = Simulation::Scenario::NullScatter;
    particle_a.config.geodesic = Simulation::GeodesicKind::Null;
    particle_a.config.dt = 0.005;
    particle_a.config.max_steps = 100000;
    particle_a.config.name = "null_scatter";
    particle_a.metric.mass = 1.0;
    {
        Simulation::NullScatterInitialConditions initial;
        initial.r0 = 6.0;
        initial.phi0 = 3.1415926535897932;
        initial.impact_parameter = 0.0;
        particle_a.initial = initial;
    }

    Simulation::SimulationRequest particle_b;
    particle_b.config.spacetime = Simulation::SpacetimeKind::Schwarzschild;
    particle_b.config.scenario = Simulation::Scenario::NullScatter;
    particle_b.config.geodesic = Simulation::GeodesicKind::Null;
    particle_b.config.dt = 0.005;
    particle_b.config.max_steps = 100000;
    particle_b.config.name = "null_scatter";
    particle_b.metric.mass = 1.0;
    {
        Simulation::NullScatterInitialConditions initial;
        initial.r0 = 2.0;
        initial.impact_parameter = 0.0;
        particle_b.initial = initial;
    }

    // Add more particles by pushing additional SimulationRequest entries.
    // Each is integrated independently — no coupled physics.
    std::vector<Simulation::SimulationRequest> simulations = {particle_a, particle_b};

    // =========================================================================
    // STAGE 2 / 3 — VISUALIZATION PREP + RENDER SETTINGS
    // =========================================================================

    viz::VisualizationConfig viz;
    viz.width = 1280;
    viz.height = 720;
    viz.title = "Penrose Viewer";
    viz.playback_speed = 4.0f;

    // Visualization resolution (independent of physics dt). Pass-through for now.
    viz.preparation.interpolation_method = viz::InterpolationMethod::PassThrough;
    viz.preparation.render_samples_per_segment = 1;
    viz.preparation.trajectory_resolution = 1.0f;

    viz.scene.horizon_radius = 1.0f; // refined from stored trajectories in prepare_scene
    viz.scene.show_starfield = true;
    viz.scene.show_event_horizon = true;
    viz.scene.show_photon_sphere = false;
    viz.scene.show_accretion_disk = false;
    viz.scene.show_reference_ring = false;
    viz.scene.background = viz::Color4::rgb(2, 4, 12);

    viz.camera.auto_frame = true;
    viz.camera.auto_frame_distance_scale = 1.2f;
    viz.camera.auto_frame_min_distance = 7.0f;
    viz.camera.auto_frame_max_distance = 120.0f;
    viz.camera.auto_frame_pitch = 0.52f;
    viz.camera.fov_y = 0.95f;
    viz.camera.distance = 12.0f;
    viz.camera.yaw = 0.0f;
    viz.camera.pitch = 0.45f;

    viz.presentation.enabled = true;
    viz.presentation.lensing_strength = 0.07f;
    viz.presentation.lensing_radius_scale = 1.85f;
    viz.presentation.bloom_threshold = 0.55f;
    viz.presentation.bloom_intensity = 0.85f;
    viz.presentation.bloom_radius = 3;
    viz.presentation.vignette = 0.22f;
    viz.presentation.contrast = 1.12f;
    viz.presentation.saturation = 1.15f;
    viz.presentation.lens_ring_strength = 0.62f;
    viz.presentation.lens_ring_radius_scale = 1.045f;
    viz.presentation.lens_ring_width_scale = 0.032f;
    viz.presentation.halo_strength = 0.0f;

    viz.trajectory_style.color = viz::Color4::rgb(255, 220, 80);
    viz.trajectory_style.glow_color = viz::Color4::rgb(255, 180, 50, 130);
    viz.trajectory_style.line_width = 2.5f;
    viz.trajectory_style.marker_radius = 0.07f;
    viz.trajectory_style.marker_glow_scale = 2.5f;
    viz.trajectory_style.marker_brightness = 1.27f;
    viz.trajectory_style.trail_rgb_min = 0.38f;
    viz.trajectory_style.trail_alpha_min = 0.10f;
    viz.trajectory_style.show_trail = true;
    viz.trajectory_style.show_marker = true;

    viz.render.trail_max_dense_segments = 1200;
    viz.render.trail_full_detail_tail = 200;

    // =========================================================================
    // RUN PIPELINE
    // =========================================================================

    const std::vector<Simulation::SimulationResult> trajectories = Simulation::run_all(simulations);
    for (const auto& trajectory : trajectories) {
        std::cout << "Integrated " << trajectory.name << ": " << trajectory.history.size()
                  << " states\n";
    }

    viz::Scene scene = viz::prepare_scene(trajectories, viz);
    viz::Camera camera = viz::make_camera(scene, viz.camera);
    return viz::run_interactive_viewer(std::move(scene), camera, viz);
}
