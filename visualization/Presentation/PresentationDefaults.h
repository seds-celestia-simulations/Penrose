#pragma once

#include "../Camera/Camera.h"
#include "../Scene/Scene.h"

namespace viz {

SceneSettings cinematic_scene_settings(float rs = 1.0f);
void apply_cinematic_camera(Camera& camera);
void frame_camera_on_trajectories(Camera& camera, const Scene& scene);

} // namespace viz
