#include "Camera.h"

#include <algorithm>
#include <cmath>

namespace viz {

Camera::Camera()
    : target_(0.0f, 0.0f, 0.0f), distance_(12.0f), yaw_(0.6f), pitch_(0.35f), fov_y_(0.95f),
      near_plane_(0.05f), far_plane_(500.0f) {}

void Camera::set_target(const Vec3& target) { target_ = target; }
void Camera::set_distance(float distance) { distance_ = std::max(0.5f, distance); }
void Camera::set_angles(float yaw_rad, float pitch_rad) {
    yaw_ = yaw_rad;
    pitch_ = std::clamp(pitch_rad, -1.45f, 1.45f);
}
void Camera::set_fov_y(float fov_y_rad) { fov_y_ = std::clamp(fov_y_rad, 0.2f, 2.5f); }
void Camera::set_clipping(float near_plane, float far_plane) {
    near_plane_ = near_plane;
    far_plane_ = far_plane;
}

Vec3 Camera::position() const {
    const float cp = std::cos(pitch_);
    const float sp = std::sin(pitch_);
    const float cy = std::cos(yaw_);
    const float sy = std::sin(yaw_);
    const Vec3 offset(cp * sy * distance_, sp * distance_, cp * cy * distance_);
    return target_ + offset;
}

Vec3 Camera::forward() const { return (target_ - position()).normalized(); }

Vec3 Camera::up() const {
    const Vec3 world_up(0.0f, 1.0f, 0.0f);
    const Vec3 right = forward().cross(world_up).normalized();
    return right.cross(forward()).normalized();
}

Vec3 Camera::world_ray_direction(float u, float v, float aspect) const {
    const float ndc_x = u * 2.0f - 1.0f;
    const float ndc_y = (1.0f - v) * 2.0f - 1.0f;
    const float tan_half = std::tan(fov_y_ * 0.5f);
    const Vec3 fwd = forward();
    const Vec3 right = fwd.cross(up()).normalized();
    const Vec3 cam_up = this->up();
    Vec3 dir = fwd + right * (ndc_x * tan_half * aspect) + cam_up * (ndc_y * tan_half);
    return dir.normalized();
}

Mat4 Camera::view_matrix() const { return Mat4::look_at(position(), target_, up()); }

Mat4 Camera::projection_matrix(float aspect) const {
    return Mat4::perspective(fov_y_, aspect, near_plane_, far_plane_);
}

Mat4 Camera::view_projection(float aspect) const {
    return projection_matrix(aspect) * view_matrix();
}

void Camera::orbit(float delta_yaw, float delta_pitch) {
    yaw_ += delta_yaw;
    pitch_ = std::clamp(pitch_ + delta_pitch, -1.45f, 1.45f);
}

void Camera::pan(float delta_x, float delta_y) {
    const Vec3 right = forward().cross(up()).normalized();
    const Vec3 up = this->up();
    target_ += right * delta_x + up * delta_y;
}

void Camera::zoom(float delta) {
    distance_ = std::clamp(distance_ * std::exp(-delta), 0.5f, 500.0f);
}

} // namespace viz
