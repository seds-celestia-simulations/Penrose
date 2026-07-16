#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace viz {

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    Vec3() = default;
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vec3 operator/(float s) const { return {x / s, y / s, z / s}; }

    Vec3& operator+=(const Vec3& o) {
        x += o.x;
        y += o.y;
        z += o.z;
        return *this;
    }

    float dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }
    Vec3 cross(const Vec3& o) const {
        return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x};
    }

    float length() const { return std::sqrt(dot(*this)); }
    float length_squared() const { return dot(*this); }

    Vec3 normalized() const {
        const float len = length();
        if (len <= 1e-12f) {
            return {0.0f, 0.0f, 0.0f};
        }
        return *this / len;
    }
};

inline Vec3 operator*(float s, const Vec3& v) { return v * s; }

struct Color4 {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 255;

    static Color4 rgb(std::uint8_t r_, std::uint8_t g_, std::uint8_t b_, std::uint8_t a_ = 255) {
        return {r_, g_, b_, a_};
    }

    static Color4 from_float(float r_, float g_, float b_, float a_ = 1.0f) {
        auto clamp_byte = [](float v) {
            return static_cast<std::uint8_t>(std::clamp(v, 0.0f, 1.0f) * 255.0f);
        };
        return {clamp_byte(r_), clamp_byte(g_), clamp_byte(b_), clamp_byte(a_)};
    }
};

struct Mat4 {
    std::array<float, 16> m{};

    static Mat4 identity() {
        Mat4 out{};
        out.m[0] = out.m[5] = out.m[10] = out.m[15] = 1.0f;
        return out;
    }

    static Mat4 perspective(float fov_y_rad, float aspect, float near_plane, float far_plane) {
        Mat4 out{};
        const float f = 1.0f / std::tan(fov_y_rad * 0.5f);
        out.m[0] = f / aspect;
        out.m[5] = f;
        out.m[10] = (far_plane + near_plane) / (near_plane - far_plane);
        out.m[11] = -1.0f;
        out.m[14] = (2.0f * far_plane * near_plane) / (near_plane - far_plane);
        return out;
    }

    static Mat4 look_at(const Vec3& eye, const Vec3& center, const Vec3& up) {
        const Vec3 f = (center - eye).normalized();
        const Vec3 s = f.cross(up).normalized();
        const Vec3 u = s.cross(f);

        Mat4 out = identity();
        out.m[0] = s.x;
        out.m[4] = s.y;
        out.m[8] = s.z;
        out.m[1] = u.x;
        out.m[5] = u.y;
        out.m[9] = u.z;
        out.m[2] = -f.x;
        out.m[6] = -f.y;
        out.m[10] = -f.z;
        out.m[12] = -s.dot(eye);
        out.m[13] = -u.dot(eye);
        out.m[14] = f.dot(eye);
        return out;
    }

    Mat4 operator*(const Mat4& o) const {
        Mat4 out{};
        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                float sum = 0.0f;
                for (int k = 0; k < 4; ++k) {
                    sum += m[row + k * 4] * o.m[k + col * 4];
                }
                out.m[row + col * 4] = sum;
            }
        }
        return out;
    }

    Vec3 transform_point(const Vec3& p) const {
        const float x = m[0] * p.x + m[4] * p.y + m[8] * p.z + m[12];
        const float y = m[1] * p.x + m[5] * p.y + m[9] * p.z + m[13];
        const float z = m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14];
        const float w = m[3] * p.x + m[7] * p.y + m[11] * p.z + m[15];
        if (std::abs(w) <= 1e-12f) {
            return {x, y, z};
        }
        return {x / w, y / w, z / w};
    }

    Vec3 transform_direction(const Vec3& d) const {
        const float x = m[0] * d.x + m[4] * d.y + m[8] * d.z;
        const float y = m[1] * d.x + m[5] * d.y + m[9] * d.z;
        const float z = m[2] * d.x + m[6] * d.y + m[10] * d.z;
        return {x, y, z};
    }
};

} // namespace viz
