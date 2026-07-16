#include "PresentationPipeline.h"

#include "PresentationDefaults.h"

namespace viz {

void PresentationPipeline::render(Scene& scene, Camera& camera, Framebuffer& framebuffer,
                                  const RenderOptions& render_options,
                                  const PresentationProfile& profile) const {
    rasterizer_.render(scene, camera, framebuffer, render_options);
    if (profile.enabled) {
        post_processor_.apply(framebuffer, camera, scene.settings().schwarzschild_radius, profile);
    }
}

} // namespace viz
