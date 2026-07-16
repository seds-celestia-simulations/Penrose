#version 430 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D skybox;
uniform mat4 uInvProjView;
uniform vec3 uCameraPos;
uniform float uTime;

const float PI = 3.14159265359;
const float rs = 0.25;
const float DISK_INNER = 3.0;
const float DISK_OUTER = 15.0;

struct Particle {
    vec4 stateX;      // xyz = position, w = radius tracking
    vec4 stateU;      // velocity (unused in rendering)
    vec3 color;       // RGB
    float mass;       
    float radius;     // Collision radius
    float pad0, pad1, pad2; // 16-byte alignment padding
};

// Bind to slot 0 (Matches your C++ binding point)
layout(std430, binding = 0) readonly buffer ParticleBuffer {
    Particle particles[];
};

// --- FAST NOISE FUNCTIONS ---
float hash(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float valueNoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i + vec2(0.0, 0.0)), hash(i + vec2(1.0, 0.0)), u.x),
               mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), u.x), u.y);
}

float fBm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    for (int i = 0; i < 4; i++) {
        value += amplitude * valueNoise(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

vec2 DirectionToUV(vec3 dir) {
    float u = 0.5 + atan(dir.x, dir.z) / (2.0 * PI);
    float v = 0.5 - asin(dir.y) / PI;
    return vec2(u, v);
}

// --- RK4 PHYSICS ---
void orbitDerivative(float u, float v, out float du, out float dv) {
    du = v;
    dv = -u + 1.5 * rs * u * u;
}

void orbitRK4(inout float u, inout float v, float h) {
    float k1u, k1v, k2u, k2v, k3u, k3v, k4u, k4v;
    orbitDerivative(u, v, k1u, k1v);
    orbitDerivative(u + 0.5 * h * k1u, v + 0.5 * h * k1v, k2u, k2v);
    orbitDerivative(u + 0.5 * h * k2u, v + 0.5 * h * k2v, k3u, k3v);
    orbitDerivative(u + h * k3u, v + h * k3v, k4u, k4v);
    u += (h / 6.0) * (k1u + 2.0 * k2u + 2.0 * k3u + k4u);
    v += (h / 6.0) * (k1v + 2.0 * k2v + 2.0 * k3v + k4v);
}

vec3 orbitPosition(float r, float psi, vec3 eR, vec3 eT) {
    return r * (cos(psi) * eR + sin(psi) * eT);
}

void main() {
    vec4 ndcPos = vec4(TexCoords.x * 2.0 - 1.0, TexCoords.y * 2.0 - 1.0, 1.0, 1.0);
    vec4 worldPos = uInvProjView * ndcPos;
    vec3 rd = normalize((worldPos.xyz / worldPos.w) - uCameraPos);
    vec3 ro = uCameraPos;

    float r0 = length(ro);
    vec3 planeNormal = cross(ro, rd);
    float planeLen = length(planeNormal);

    if (planeLen < 1e-6) {
        FragColor = vec4(texture(skybox, DirectionToUV(rd)).rgb, 1.0);
        return;
    }

    planeNormal /= planeLen;
    vec3 eR = ro / r0;
    vec3 eT = normalize(cross(planeNormal, eR));

    float radialSpeed = dot(rd, eR);
    float tangentialSpeed = dot(rd, eT);

    float u = 1.0 / r0;
    float v = -radialSpeed / (r0 * tangentialSpeed);

    float stepSign = sign(tangentialSpeed);
    float dPsi = 0.03 * stepSign; // Optimized adaptive step
    float psi = 0.0;
    
    vec3 previousPos = orbitPosition(r0, psi, eR, eT);
    
    // Alpha Compositing Accumulators
    vec3 accumColor = vec3(0.0);
    float accumAlpha = 0.0;
    
    bool captured = false;
    bool escaped = false;

    // Mini-Raymarch (Maximum 150 steps keeps FPS above 150+)
    for (int i = 0; i < 150; ++i) {
        orbitRK4(u, v, dPsi);
        psi += dPsi;

        if (u <= 0.0) break; // Numerical safety

        float r = 1.0 / u;
        vec3 currentPos = orbitPosition(r, psi, eR, eT);

        // --- 2. PARTICLE INTERSECTION (Line Segment vs Sphere) ---
        for(int p = 0; p < particles.length(); ++p) {
            vec3 center = particles[p].stateX.xyz;
            float radius = particles[p].radius;

            vec3 d = currentPos - previousPos;
            vec3 f = previousPos - center;

            float a = dot(d, d);
            float b = 2.0 * dot(f, d);
            float c = dot(f, f) - radius * radius;

            float discriminant = b * b - 4.0 * a * c;
            
            // If discriminant > 0, the curving ray pierced the particle!
            if (discriminant > 0.0) {
                discriminant = sqrt(discriminant);
                float t1 = (-b - discriminant) / (2.0 * a);
                float t2 = (-b + discriminant) / (2.0 * a);

                // Check if the intersection happened strictly between the previous and current step
                if ((t1 >= 0.0 && t1 <= 1.0) || (t2 >= 0.0 && t2 <= 1.0)) {
                    // Blend the particle color into the ray!
                    float pAlpha = 0.95; // High opacity so they look solid
                    accumColor += (1.0 - accumAlpha) * particles[p].color * pAlpha;
                    accumAlpha += (1.0 - accumAlpha) * pAlpha;
                }
            }
        }

        // --- EXACT PLANE INTERSECTION ---
        float z0 = previousPos.z;
        float z1 = currentPos.z;

        // Did the curving ray pierce the equator?
        if (z0 * z1 <= 0.0) {
            float t = abs(z0) / max(abs(z0) + abs(z1), 1e-8);
            vec3 hitPos = mix(previousPos, currentPos, t);
            float diskR = length(hitPos.xy);

            if (diskR > DISK_INNER && diskR < DISK_OUTER) {
                // 1. Keplerian Shearing
                float phi = atan(hitPos.y, hitPos.x);
                float omega = 3.0 / pow(diskR, 1.5); 
                float shearedPhi = phi + omega * uTime;
                
                // 2. THE SEAMLESS FIX: Map back to Cartesian for noise
                vec2 noiseCoord = vec2(diskR * cos(shearedPhi), diskR * sin(shearedPhi));
                float noiseVal = fBm(noiseCoord * 0.8);
                
                // 3. Density and Color
                float radial01 = (diskR - DISK_INNER) / (DISK_OUTER - DISK_INNER);
                float edgeFade = smoothstep(0.0, 0.1, radial01) * (1.0 - smoothstep(0.8, 1.0, radial01));
                
                // Cap max opacity per crossing so multiple crossings blend beautifully
                float alpha = smoothstep(0.2, 0.7, noiseVal) * edgeFade * 0.85; 
                
                float heat = 1.0 - radial01;
                vec3 innerColor = vec3(1.0, 0.8, 0.4);
                vec3 middleColor = vec3(0.9, 0.2, 0.01);
                vec3 outerColor = vec3(0.1, 0.0, 0.0);
                
                vec3 color = mix(outerColor, middleColor, heat);
                color = mix(color, innerColor, heat * heat);
                color *= (1.0 + heat * 2.0); // Make the core glow

                // 4. Accumulate (Allows the halo to render OVER the main disk!)
                accumColor += (1.0 - accumAlpha) * color * alpha;
                accumAlpha += (1.0 - accumAlpha) * alpha;
                
                // Early exit if the gas is fully opaque
                if (accumAlpha > 0.99) break;
            }
        }

        if (r <= rs * 1.05) {
            captured = true;
            break;
        }

        float drdPsi = -v / max(u * u, 1e-8);
        if (r > 25.0 && drdPsi * dPsi > 0.0) {
            escaped = true;
            break;
        }
        previousPos = currentPos;
    }

    vec3 finalColor = accumColor;

    // If the ray didn't fall into the black hole, sample the skybox behind the disk
    if (!captured) {
        float r = 1.0 / u;
        float drdPsi = -v / max(u * u, 1e-8);
        vec3 er = cos(psi) * eR + sin(psi) * eT;
        vec3 epsi = -sin(psi) * eR + cos(psi) * eT;
        vec3 outgoingDir = normalize(drdPsi * er + r * epsi);
        
        vec3 skyColor = texture(skybox, DirectionToUV(outgoingDir)).rgb;
        
        // Blend skybox behind the accumulated accretion disk
        finalColor += (1.0 - accumAlpha) * skyColor;
    }

    FragColor = vec4(finalColor, 1.0);
}