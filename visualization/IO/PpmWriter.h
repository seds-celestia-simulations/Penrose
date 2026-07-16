#pragma once

#include "../Renderer/Framebuffer.h"

#include <string>

namespace viz {

bool write_ppm(const Framebuffer& framebuffer, const std::string& path);

} // namespace viz
