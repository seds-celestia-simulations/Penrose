#pragma once

namespace viz {

// Non-physical presentation tuning applied after the deterministic CPU rasterizer.
struct PresentationProfile {
    bool enabled = true;

    float bloom_threshold = 0.55f;
    float bloom_intensity = 0.85f;
    int bloom_radius = 3;

    // Very subtle screen-space warp within ~1.5–2 projected Schwarzschild radii.
    float lensing_strength = 0.07f;
    float lensing_radius_scale = 1.85f;

    float contrast = 1.12f;
    float saturation = 1.15f;
    float vignette = 0.22f;

    float halo_radius_scale = 1.12f;
    float halo_strength = 0.0f;

    // Thin warm photon ring just outside the shadow (not a broad halo).
    float lens_ring_strength = 0.62f;
    float lens_ring_radius_scale = 1.045f;
    float lens_ring_width_scale = 0.032f;

    static PresentationProfile cinematic_default();
    static PresentationProfile interactive_default();
};

} // namespace viz
