struct Particle {
    vec4 stateX;
    vec4 stateU;
    vec3 color;
    float mass;
    float radius;
    float pad0, pad1, pad2;
};

layout(std430, binding = 0) readonly buffer ParticleBuffer {
    Particle particles[];
};

bool intersectRaySegmentSphere(vec3 segStart, vec3 segEnd, vec3 center, float radius,
                                out float tHit) {
    vec3 d = segEnd - segStart;
    vec3 f = segStart - center;

    float a = dot(d, d);
    float b = 2.0 * dot(f, d);
    float c = dot(f, f) - radius * radius;

    float discriminant = b * b - 4.0 * a * c;

    if (discriminant <= 0.0) return false;

    discriminant = sqrt(discriminant);
    float t1 = (-b - discriminant) / (2.0 * a);
    float t2 = (-b + discriminant) / (2.0 * a);

    if ((t1 >= 0.0 && t1 <= 1.0) || (t2 >= 0.0 && t2 <= 1.0)) {
        tHit = (t1 >= 0.0 && t1 <= 1.0) ? t1 : t2;
        return true;
    }
    return false;
}

void accumulateParticles(vec3 previousPos, vec3 currentPos,
                         inout vec3 accumColor, inout float accumAlpha) {
    for (int p = 0; p < particles.length(); ++p) {
        vec3 center = particles[p].stateX.xyz;
        float radius = particles[p].radius;

        float tHit;
        if (intersectRaySegmentSphere(previousPos, currentPos, center, radius, tHit)) {
            float pAlpha = 0.95;
            accumColor += (1.0 - accumAlpha) * particles[p].color * pAlpha;
            accumAlpha += (1.0 - accumAlpha) * pAlpha;
        }
    }
}
