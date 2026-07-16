#pragma once

#include "../Geometry/Math.h"

#include <cstdint>
#include <vector>

namespace viz {

class Framebuffer {
public:
    Framebuffer() = default;
    Framebuffer(int width, int height);

    int width() const { return width_; }
    int height() const { return height_; }
    const std::vector<Color4>& color_buffer() const { return color_; }
    const std::vector<float>& depth_buffer() const { return depth_; }
    std::vector<Color4>& color_buffer() { return color_; }

    void resize(int width, int height);
    void clear(const Color4& color);

    void set_pixel(int x, int y, const Color4& color, float depth);
    void blend_pixel(int x, int y, const Color4& color, float depth);

private:
    int width_ = 0;
    int height_ = 0;
    std::vector<Color4> color_;
    std::vector<float> depth_;
};

} // namespace viz
