#include "PpmWriter.h"

#include <fstream>

namespace viz {

bool write_ppm(const Framebuffer& framebuffer, const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) {
        return false;
    }

    out << "P6\n" << framebuffer.width() << " " << framebuffer.height() << "\n255\n";
    const auto& pixels = framebuffer.color_buffer();
    for (int y = 0; y < framebuffer.height(); ++y) {
        for (int x = 0; x < framebuffer.width(); ++x) {
            const Color4& c = pixels[static_cast<std::size_t>(y * framebuffer.width() + x)];
            const char rgb[3] = {static_cast<char>(c.r), static_cast<char>(c.g), static_cast<char>(c.b)};
            out.write(rgb, 3);
        }
    }
    return out.good();
}

} // namespace viz
