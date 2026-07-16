// Headless render benchmark.
#include "Camera/Camera.h"
#include "Presentation/PresentationDefaults.h"
#include "Presentation/PresentationPipeline.h"
#include "Presentation/PresentationProfile.h"
#include "Renderer/CPURasterizer.h"
#include "Renderer/Framebuffer.h"
#include "Scene/SceneBuilder.h"
#include "Presentation/PostProcessor.h"

#include <chrono>
#include <cstdio>

namespace {

double bench_total(viz::PresentationPipeline& pipeline, viz::Scene& scene, viz::Camera& camera,
                   viz::Framebuffer& fb, const viz::PresentationProfile& profile,
                   const viz::RenderOptions& options, int frames) {
    for (int i = 0; i < 2; ++i) {
        pipeline.render(scene, camera, fb, options, profile);
    }

    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < frames; ++i) {
        pipeline.render(scene, camera, fb, options, profile);
    }
    const auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count() / static_cast<double>(frames);
}

void bench_breakdown(viz::Scene& scene, viz::Camera& camera, viz::Framebuffer& fb,
                     const viz::PresentationProfile& profile, const viz::RenderOptions& options,
                     int frames) {
    viz::CPURasterizer rasterizer;
    viz::PostProcessor post;

    for (int i = 0; i < 2; ++i) {
        rasterizer.render(scene, camera, fb, options);
        if (profile.enabled) {
            post.apply(fb, camera, scene.settings().schwarzschild_radius, profile);
        }
    }

    double raster_sum = 0.0;
    double post_sum = 0.0;
    for (int i = 0; i < frames; ++i) {
        const auto r0 = std::chrono::steady_clock::now();
        rasterizer.render(scene, camera, fb, options);
        const auto r1 = std::chrono::steady_clock::now();
        if (profile.enabled) {
            post.apply(fb, camera, scene.settings().schwarzschild_radius, profile);
        }
        const auto r2 = std::chrono::steady_clock::now();
        raster_sum += std::chrono::duration<double, std::milli>(r1 - r0).count();
        post_sum += std::chrono::duration<double, std::milli>(r2 - r1).count();
    }

    const double inv = 1.0 / static_cast<double>(frames);
    const double total = (raster_sum + post_sum) * inv;
    std::printf("  breakdown: raster=%.1f ms  post=%.1f ms  total=%.1f ms\n", raster_sum * inv,
                post_sum * inv, total);
}

void run_case(const char* label, const viz::PresentationProfile& profile, int width, int height,
              int frames, bool starfield_half = false) {
    viz::Scene scene(viz::cinematic_scene_settings());
    viz::Camera camera;
    viz::apply_cinematic_camera(camera);
    viz::Framebuffer fb(width, height);
    viz::PresentationPipeline pipeline;
    viz::RenderOptions options{};
    options.starfield_half_resolution = starfield_half;

    const double ms = bench_total(pipeline, scene, camera, fb, profile, options, frames);
    std::printf("%s (%dx%d): %.1f ms/frame (~%.1f fps)\n", label, width, height, ms,
                1000.0 / ms);
    bench_breakdown(scene, camera, fb, profile, options, frames);
}

} // namespace

int main() {
    const int w = 1280;
    const int h = 720;
    const int frames = 10;

    std::printf("Penrose CPU visualization benchmark\n\n");

    run_case("Interactive (viewer defaults)", viz::PresentationProfile::interactive_default(), w, h,
             frames, true);
    run_case("Cinematic export quality", viz::PresentationProfile::cinematic_default(), w, h, frames,
             false);

    auto classic = viz::PresentationProfile::cinematic_default();
    classic.enabled = false;
    run_case("Classic (no post-process)", classic, w, h, frames, true);

    return 0;
}
