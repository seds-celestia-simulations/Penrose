/**
 * Minimal direct-integration example: Scientific Engine -> Scene -> Presentation render -> PPM.
 *
 * Build target: visualization_example
 * Run: ./build/visualization_example
 */

#include "geodesics/GeodesicDynamics.h"
#include "metrics/SchwarzschildMetric.h"
#include "simulation/TerminationPolicy.h"
#include "simulation/TrajectorySolver.h"
#include "state/State.h"

#include "Camera/Camera.h"
#include "IO/OutputPaths.h"
#include "IO/PpmWriter.h"
#include "Presentation/PresentationDefaults.h"
#include "Presentation/PresentationPipeline.h"
#include "Renderer/Framebuffer.h"
#include "Scene/Scene.h"
#include "Trajectory/TrajectoryAdapter.h"

#include <cmath>
#include <iostream>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main() {
    constexpr double rs = 1.0;
    constexpr double r0 = 6.0;
    constexpr double vph = 0.06;
    constexpr double dt = 0.01;
    constexpr int max_steps = 100000;

    const double f = 1.0 - rs / r0;
    const double inner = (1.0 + r0 * r0 * vph * vph) / f;
    const double vt = std::sqrt(inner);

    State initial(Eigen::Vector4d(0.0, r0, M_PI / 2.0, 0.0), Eigen::Vector4d(vt, 0.0, 0.0, vph));

    Spacetime::SchwarzschildMetric metric(rs);
    Dynamics::GeodesicDynamics dynamics(metric);
    Simulation::HorizonTermination policy(rs, 1.0);

    const std::vector<State> history =
        Simulation::TrajectorySolver::solve(initial, dynamics, policy, dt, max_steps);

    std::cout << "Integrated " << history.size() << " states\n";

    viz::Scene scene(viz::cinematic_scene_settings());
    viz::TrajectoryAdapterOptions options;
    options.name = "orbital";
    scene.add_trajectory(viz::adapt_states(history, options));
    scene.playback().time = scene.playback().duration;

    viz::Camera camera;
    viz::apply_cinematic_camera(camera);

    viz::Framebuffer framebuffer(960, 540);
    viz::PresentationPipeline pipeline;
    pipeline.render(scene, camera, framebuffer);

    const auto output_path = viz::default_render_still_path(viz::detect_repo_root(), "direct_integration.ppm");
    if (!viz::write_ppm(framebuffer, output_path.string())) {
        std::cerr << "Failed to write " << output_path << "\n";
        return 1;
    }

    std::cout << "Wrote " << output_path << "\n";
    return 0;
}
