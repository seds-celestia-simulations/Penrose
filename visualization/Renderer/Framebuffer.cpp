#include "Framebuffer.h"

#include <algorithm>
#include <limits>

namespace viz {

Framebuffer::Framebuffer(int width, int height) { resize(width, height); }

void Framebuffer::resize(int width, int height) {
    width_ = std::max(1, width);
    height_ = std::max(1, height);
    color_.assign(static_cast<std::size_t>(width_ * height_), Color4{});
    depth_.assign(static_cast<std::size_t>(width_ * height_), std::numeric_limits<float>::infinity());
}

void Framebuffer::clear(const Color4& color) {
    std::fill(color_.begin(), color_.end(), color);
    std::fill(depth_.begin(), depth_.end(), std::numeric_limits<float>::infinity());
}

void Framebuffer::set_pixel(int x, int y, const Color4& color, float depth) {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) {
        return;
    }
    const std::size_t idx = static_cast<std::size_t>(y * width_ + x);
    if (depth < depth_[idx]) {
        depth_[idx] = depth;
        color_[idx] = color;
    }
}

void Framebuffer::blend_pixel(int x, int y, const Color4& color, float depth) {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) {
        return;
    }
    const std::size_t idx = static_cast<std::size_t>(y * width_ + x);
    if (depth > depth_[idx]) {
        return;
    }

    const float alpha = static_cast<float>(color.a) / 255.0f;
    if (alpha >= 0.999f && depth < depth_[idx]) {
        depth_[idx] = depth;
        color_[idx] = color;
        return;
    }

    Color4& dst = color_[idx];
    const float inv = 1.0f - alpha;
    dst.r = static_cast<std::uint8_t>(std::clamp(color.r * alpha + dst.r * inv, 0.0f, 255.0f));
    dst.g = static_cast<std::uint8_t>(std::clamp(color.g * alpha + dst.g * inv, 0.0f, 255.0f));
    dst.b = static_cast<std::uint8_t>(std::clamp(color.b * alpha + dst.b * inv, 0.0f, 255.0f));
    dst.a = static_cast<std::uint8_t>(std::clamp(color.a + dst.a * inv, 0.0f, 255.0f));
    depth_[idx] = std::min(depth_[idx], depth);
}

} // namespace viz
