#version 430 core
out vec4 FragColor;
in vec2 TexCoords;

uniform float uTime;
uniform vec2 uResolution;
uniform vec3 uCameraPos;

uniform sampler2D skybox;       // The equirectangular texture sampler
uniform mat4 uInvProjView;      // The inverse View-Projection matrix

const float PI = 3.14159265359;
//const float dL = 0.01;
const float rs = 0.25;
const float DISK_INNER = 3.0;
const float DISK_OUTER = 15.0;


uniform int uParticleCount;

uniform bool uHighQualityPass;
uniform float uHighQualityRadius;


vec2 DirectionToUV(vec3 dir) {
    float u = 0.5 + atan(dir.x, dir.z) / (2.0 * PI);
    float v = 0.5 - asin(dir.y) / PI;
    // float v = dir.y*0.5+0.5;
    return vec2(u, v);
}

void orbitDerivative(float u, float v, out float du, out float dv)
{
    du = v;
    dv = -u + 1.5 * rs * u * u;
}

void orbitRK4(inout float u, inout float v, float h)
{
    float k1u, k1v;
    orbitDerivative(u, v, k1u, k1v);

    float k2u, k2v;
    orbitDerivative(u + 0.5 * h * k1u, v + 0.5 * h * k1v, k2u, k2v);

    float k3u, k3v;
    orbitDerivative(u + 0.5 * h * k2u, v + 0.5 * h * k2v, k3u, k3v);

    float k4u, k4v;
    orbitDerivative(u + h * k3u, v + h * k3v, k4u, k4v);

    u += (h / 6.0) * (k1u + 2.0 * k2u + 2.0 * k3u + k4u);
    v += (h / 6.0) * (k1v + 2.0 * k2v + 2.0 * k3v + k4v);
}
vec3 orbitPosition(
    float r,
    float psi,
    vec3 eR,
    vec3 eT
)
{
    return r * (
        cos(psi) * eR +
        sin(psi) * eT
    );
}

vec3 raymarchReduced(vec3 ro, vec3 rd)
{
    float r0 = length(ro);

    vec3 planeNormal = cross(ro, rd);
    float planeLen = length(planeNormal);

    if (planeLen < 1e-6) {
        if (dot(ro, rd) < 0.0) return vec3(0.0);
        return texture(skybox, DirectionToUV(rd)).rgb;
    }

    planeNormal /= planeLen;

    vec3 eR = ro / r0;
    vec3 eT = normalize(cross(planeNormal, eR));

    float radialSpeed = dot(rd, eR);
    float tangentialSpeed = dot(rd, eT);

    if (abs(tangentialSpeed) < 1e-6) {
        if (radialSpeed < 0.0) return vec3(0.0);
        return texture(skybox, DirectionToUV(rd)).rgb;
    }

    float u = 1.0 / r0;

    // u' = du/dpsi = -radial / (r * tangential)
    float v = -radialSpeed / (r0 * tangentialSpeed);

    float stepSign = sign(tangentialSpeed);
    float dPsi = 0.010 * stepSign;
    float psi = 0.0;
    vec3 previousPos = orbitPosition(r0, psi, eR, eT);

    const int MAX_ORBIT_STEPS = 480;

    bool captured = false;
    bool escaped = false;

    for (int i = 0; i < MAX_ORBIT_STEPS; ++i) {
        orbitRK4(u, v, dPsi);
        psi += dPsi;

        // Numerical safety.
        if (u <= 0.0 || u > 100.0) {
            break;
        }

float r = 1.0 / u;

// Reconstruct the photon position in world coordinates for this orbit step.
vec3 currentPos = orbitPosition(r, psi, eR, eT);

// Test whether this segment crossed the fixed world disk plane: z = 0.
// Thin but non-zero disk thickness in world units.
const float DISK_HALF_THICKNESS = 0.035;

// We only care whether the segment enters the disk slab.
float z0 = previousPos.z;
float z1 = currentPos.z;

// If the Z signs are different, they multiply to a negative number.
// This mathematically guarantees we NEVER miss a ray crossing the disk!
bool crossesEquator = (z0 * z1) <= 0.0;

if (crossesEquator)
{
    // Find the exact fractional point 't' where Z equals 0.0
    float t = abs(z0) / (abs(z0) + abs(z1));
    vec3 hitPos = mix(previousPos, currentPos, t);

    float diskR = length(hitPos.xy);

    if (diskR > DISK_INNER && diskR < DISK_OUTER)
    {
        float radial01 = clamp(
            (diskR - DISK_INNER) / (DISK_OUTER - DISK_INNER),
            0.0, 1.0
        );

        float edgeFade =
            smoothstep(DISK_INNER, DISK_INNER + 0.8, diskR) *
            (1.0 - smoothstep(DISK_OUTER - 2.5, DISK_OUTER, diskR));

        float heat = 1.0 - radial01;

        vec3 outerColor = vec3(0.22, 0.006, 0.001);
        vec3 middleColor = vec3(0.85, 0.10, 0.01);
        vec3 innerColor = vec3(1.0, 0.72, 0.18);

        vec3 diskColor = mix(outerColor, middleColor, heat);
        diskColor = mix(diskColor, innerColor, heat * heat);

        return diskColor * edgeFade * (0.25 + 1.15 * heat);
    }
}

if (r <= rs * 1.001) {
            captured = true;
            break;
        }

        // dr/dpsi = -v/u².
        float drdPsi = -v / max(u * u, 1e-8);
        
        bool pastDisk =r > DISK_OUTER + 1.0 && drdPsi * dPsi > 0.0;

        // Ray has travelled outward to the far sky.
        if (r > 30.0 && drdPsi * dPsi > 0.0) {
            escaped = true;
            break;
        }
        previousPos = currentPos;
    }
    if (captured || !escaped) {
        return vec3(0.0);
    }
    
    vec3 er = cos(psi) * eR + sin(psi) * eT;
    vec3 epsi = -sin(psi) * eR + cos(psi) * eT;

    // Tangent direction of the orbit:
    //
    // dr/dpsi = -v/u²
    // direction ∝ dr/dpsi * er + r * epsi
    float r = 1.0 / u;
    float drdPsi = -v / max(u * u, 1e-8);

    vec3 outgoingDir = normalize(drdPsi * er + r * epsi);

    return texture(skybox, DirectionToUV(outgoingDir)).rgb;
}

void main() {
    if (uHighQualityPass &&
    length(TexCoords - vec2(0.5)) > uHighQualityRadius)
{
    discard;
}
    // 1. Generate the initial ray u from this pixel
    // Start with NDC coordinates: x,y in [-1,1]
    
    vec4 ndcPos = vec4(TexCoords.x * 2.0 - 1.0, TexCoords.y * 2.0 - 1.0, 1.0, 1.0);
    // Un-project from NDC to World Space
    vec4 worldPos = uInvProjView * ndcPos;
    // The ray u is the un-projected u, normalized
    vec3 worldDir = normalize((worldPos.xyz / worldPos.w)-uCameraPos);

    vec3 finalColor = raymarchReduced(uCameraPos, worldDir);

    // A small animation test to confirm uniforms still work
    // finalColor *= abs(sin(uTime * 0.2)) * 0.2 + 0.8;

    FragColor = vec4(finalColor, 1.0);
}