#include "PresentationDefaults.h"

#include "../Camera/Camera.h"
#include "../Scene/Scene.h"

#include <algorithm>
#include <cmath>

namespace viz {

SceneSettings cinematic_scene_settings(float rs) {
    SceneSettings settings;
    settings.schwarzschild_radius = rs;
    settings.show_photon_sphere = false;
    settings.show_reference_ring = false;
    settings.show_event_horizon = true;
    settings.show_accretion_disk = false;
    settings.show_starfield = true;
    settings.use_image_starfield = false;
    settings.background = Color4::rgb(2, 4, 12);
    return settings;
}

void apply_cinematic_camera(Camera& camera) {
    camera.set_target(Vec3(0.0f, 0.0f, 0.0f));
    // Closer framing with a oblique view onto the equatorial orbit / infall plane.
    camera.set_distance(7.5f);
    camera.set_angles(-1.05f, 0.52f);
    camera.set_fov_y(0.95f);
    camera.set_clipping(0.05f, 500.0f);
}

void frame_camera_on_trajectories(Camera& camera, const Scene& scene) {
    if (scene.trajectories().empty()) {
        apply_cinematic_camera(camera);
        return;
    }

    float max_radius = 1.0f;
    Vec3 dominant_sample{0.0f, 0.0f, 0.0f};
    for (const Trajectory& trajectory : scene.trajectories()) {
        for (const TrajectorySample& sample : trajectory.samples()) {
            const float radius = sample.position.length();
            if (radius >= max_radius) {
                max_radius = radius;
                dominant_sample = sample.position;
            }
        }
    }

    camera.set_target(Vec3(0.0f, 0.0f, 0.0f));
    camera.set_distance(std::clamp(max_radius * 1.2f, 7.0f, 120.0f));

    if (dominant_sample.length_squared() > 1e-8f) {
        const float yaw = std::atan2(dominant_sample.x, dominant_sample.z) + 3.14159265359f;
        camera.set_angles(yaw, 0.52f);
    } else {
        camera.set_angles(-1.05f, 0.52f);
    }

    camera.set_fov_y(0.95f);
    camera.set_clipping(0.05f, 500.0f);
}

} // namespace viz
