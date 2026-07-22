/**
 * Headless export entry point.
 *
 * Pipeline:
 *   Physics (independent SimulationRequest per particle)
 *        ↓
 *   Trajectory storage
 *        ↓
 *   Visualization preparation (prepare_scene)
 *        ↓
 *   Rendering (PresentationPipeline → PPM)
 *
 *   cmake --build build --target visualization_export
 *   ./build/visualization_export
 */

#include "simulation/SimulationRequest.h"

#include "adapter/SimulationTrajectoryAdapter.h"
#include "IO/OutputPaths.h"
#include "IO/PpmWriter.h"
#include "Presentation/PresentationPipeline.h"
#include "Presentation/VisualizationConfig.h"
#include "Renderer/Framebuffer.h"

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

int main() {
    // =========================================================================
    // STAGE 1 — PHYSICS (one independent request per particle)
    // =========================================================================

    Simulation::SimulationRequest orbit;
    orbit.config.spacetime = Simulation::SpacetimeKind::Schwarzschild;
    orbit.config.scenario = Simulation::Scenario::BoundOrbit;
    orbit.config.geodesic = Simulation::GeodesicKind::Timelike;
    orbit.config.dt = 0.01;
    orbit.config.max_steps = 100000;
    orbit.config.name = "orbital";
    orbit.metric.mass = 1.0;
    {
        Simulation::BoundOrbitInitialConditions initial;
        initial.r0 = 6.0;
        initial.vr = 0.0;
        initial.vphi = 0.06;
        orbit.initial = initial;
    }

    std::vector<Simulation::SimulationRequest> simulations = {orbit};

    // =========================================================================
    // STAGE 2 / 3 — VISUALIZATION PREP + RENDER SETTINGS
    // =========================================================================

    viz::VisualizationConfig viz;
    viz.width = 1280;
    viz.height = 720;
    viz.frame_count = 1;
    viz.still_basename = "frame.ppm";

    viz.preparation.interpolation_method = viz::InterpolationMethod::PassThrough;
    viz.preparation.render_samples_per_segment = 1;
    viz.preparation.trajectory_resolution = 1.0f;

    viz.scene.horizon_radius = 1.0f;
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

    viz::Scene scene = viz::prepare_scene_from_results(trajectories, viz);
    viz::Camera camera = viz::make_camera(scene, viz.camera);

    viz::Framebuffer framebuffer(viz.width, viz.height);
    viz::PresentationPipeline pipeline;

    const auto repo_root = viz::detect_repo_root();
    const int frames = std::max(1, viz.frame_count);
    std::string output_dir;
    std::string still_path;
    if (frames > 1) {
        output_dir = viz::default_render_sequence_dir(repo_root).string();
    } else {
        still_path = viz::default_render_still_path(repo_root, viz.still_basename).string();
    }

    for (int frame = 0; frame < frames; ++frame) {
        if (frames == 1) {
            scene.playback().time = scene.playback().duration;
        } else {
            scene.playback().time = scene.playback().duration *
                                    (static_cast<double>(frame) / static_cast<double>(frames - 1));
        }

        pipeline.render(scene, camera, framebuffer, viz.render, viz.presentation);

        std::string path;
        if (frames == 1) {
            path = still_path;
        } else {
            char name[64];
            std::snprintf(name, sizeof(name), "%s/frame_%06d.ppm", output_dir.c_str(), frame);
            path = name;
        }

        if (!viz::write_ppm(framebuffer, path)) {
            std::cerr << "Failed to write " << path << "\n";
            return 1;
        }
        std::cout << "Wrote " << path << " (t=" << scene.playback().time << ")\n";
    }

    return 0;
}
