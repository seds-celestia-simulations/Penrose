#pragma once

#include "../Geometry/Math.h"

namespace viz {

class Camera {
public:
    Camera();

    void set_target(const Vec3& target);
    void set_distance(float distance);
    void set_angles(float yaw_rad, float pitch_rad);
    void set_fov_y(float fov_y_rad);
    void set_clipping(float near_plane, float far_plane);

    const Vec3& target() const { return target_; }
    float distance() const { return distance_; }
    float yaw() const { return yaw_; }
    float pitch() const { return pitch_; }
    float fov_y() const { return fov_y_; }

    Vec3 position() const;
    Vec3 forward() const;
    Vec3 up() const;

    // Normalized world-space ray through normalized viewport coordinates (u,v in [0,1]).
    Vec3 world_ray_direction(float u, float v, float aspect) const;

    Mat4 view_matrix() const;
    Mat4 projection_matrix(float aspect) const;
    Mat4 view_projection(float aspect) const;

    void orbit(float delta_yaw, float delta_pitch);
    void pan(float delta_x, float delta_y);
    void zoom(float delta);

private:
    Vec3 target_;
    float distance_;
    float yaw_;
    float pitch_;
    float fov_y_;
    float near_plane_;
    float far_plane_;
};

} // namespace viz
