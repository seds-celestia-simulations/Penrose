#version 430 core
out vec4 FragColor;
in vec2 TexCoords;

uniform float uTime;
uniform vec2 uResolution;
uniform vec3 uCameraPos;

uniform sampler2D skybox;
uniform mat4 uInvProjView;

uniform bool uHighQualityPass;
uniform float uHighQualityRadius;

// --- common/particle.glsl ---
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

uniform int uParticleCount;

// --- common/skybox.glsl ---
const float PI = 3.14159265359;

vec2 DirectionToUV(vec3 dir) {
    float u = 0.5 + atan(dir.x, dir.z) / (2.0 * PI);
    float v = 0.5 - asin(dir.y) / PI;
    return vec2(u, v);
}

// --- metrics/schwarzschild_full.glsl ---
const float rs = 0.25;
const int N_STEPS = 500;

struct ray {
    vec4 x;
    vec4 u;
};

vec3 get_cartesian_pos(vec4 sph_pos) {
    float r = sph_pos.y;
    float theta = sph_pos.z;
    float phi = sph_pos.w;
    return vec3(
        r * sin(theta) * cos(phi),
        r * sin(theta) * sin(phi),
        r * cos(theta)
    );
}

float dFactor = 1e-5;

ray cart_to_sph(ray R) {
    vec3 p = R.x.yzw;
    vec3 v = R.u.yzw;

    float r = length(p);
    float theta = acos(clamp(p.z / max(r, 1e-6), -1.0, 1.0));
    float phi = atan(p.y, p.x);

    float vr = dot(p, v) / r;
    float xy = max(p.x * p.x + p.y * p.y, 1e-6);
    float vphi = (-p.y * v.x + p.x * v.y) / xy;
    float rho = sqrt(max(xy, 1e-6));
    float vtheta = (p.z * (p.x * v.x + p.y * v.y) - xy * v.z) / (r * r * rho);

    return ray(
        vec4(R.x.x, r, theta, phi),
        vec4(R.u.x, vr, vtheta, vphi)
    );
}

ray sph_to_cart(ray R) {
    float r     = R.x.y;
    float theta = R.x.z;
    float phi   = R.x.w;

    float st = sin(theta);
    float ct = cos(theta);
    float sp = sin(phi);
    float cp = cos(phi);

    vec3 pos = vec3(r * st * cp, r * st * sp, r * ct);

    float vr     = R.u.y;
    float vtheta = R.u.z;
    float vphi   = R.u.w;

    vec3 vel =
          vr * vec3(st * cp, st * sp, ct)
        + r * vtheta * vec3(ct * cp, ct * sp, -st)
        + r * st * vphi * vec3(-sp, cp, 0.0);

    return ray(
        vec4(R.x.x, pos),
        vec4(R.u.x, vel)
    );
}

vec4 find_acceleration(ray R) {
    float r = R.x[1];
    float theta = clamp(R.x[2], dFactor, PI - dFactor);

    float vt     = R.u[0];
    float vr     = R.u[1];
    float vtheta = R.u[2];
    float vphi   = R.u[3];

    float sin_theta = sin(theta);
    float cos_theta = cos(theta);

    float Tr_tt   = rs * (r - rs) / (2.0 * r * r * r);
    float Tr_rr   = -rs / (2.0 * r * (r - rs) + dFactor);
    float Tr_thth = -(r - rs);
    float Tr_phph = -(r - rs) * sin_theta * sin_theta;

    float Tth_rth   = 1.0 / r;
    float Tth_phph  = -sin_theta * cos_theta;

    float Tph_rph   = 1.0 / r;
    float Tph_thph  = cos_theta / sin_theta;

    float Tt_tr = rs / (2.0 * r * (r - rs) + dFactor);

    float at   = -2.0 * Tt_tr * vt * vr;
    float ar   = -(Tr_tt * vt * vt + Tr_rr * vr * vr + Tr_thth * vtheta * vtheta + Tr_phph * vphi * vphi);
    float atheta = -(2.0 * Tth_rth * vr * vtheta + Tth_phph * vphi * vphi);
    float aphi  = -(2.0 * Tph_rph * vr * vphi + 2.0 * Tph_thph * vtheta * vphi);

    return vec4(at, ar, atheta, aphi);
}

ray create_ray_derivative(ray R) {
    return ray(vec4(R.u), find_acceleration(R));
}

ray integrate(ray R, float dL) {
    ray k1 = create_ray_derivative(R);
    ray k2 = create_ray_derivative(ray(R.x + (k1.x * (dL / 2.0)), R.u + (k1.u * (dL / 2.0))));
    ray k3 = create_ray_derivative(ray(R.x + (k2.x * (dL / 2.0)), R.u + (k2.u * (dL / 2.0))));
    ray k4 = create_ray_derivative(ray(R.x + (k3.x * dL), R.u + (k3.u * dL)));

    return ray(
        R.x + (dL / 6.0) * (k1.x + 2.0 * k2.x + 2.0 * k3.x + k4.x),
        R.u + (dL / 6.0) * (k1.u + 2.0 * k2.u + 2.0 * k3.u + k4.u)
    );
}

// --- quad-specific: ray-particle hit test ---
float rayParticleHit(vec3 rayPos, Particle p) {
    vec3 toParticle = p.stateX.xyz - rayPos;
    float squaredDist = dot(toParticle, toParticle);
    float squaredRadius = p.radius * p.radius;
    return squaredDist - squaredRadius;
}

// --- raymarch (500-step full Christoffel integration) ---
vec3 raymarch(vec3 ro, vec3 rd) {
    ray R = ray(vec4(0.0, ro), vec4(0.0, rd));

    vec3 toBH = -ro;
    float closestApproach = length(cross(rd, normalize(toBH))) * length(toBH);

    if (closestApproach > 3.0) {
        return texture(skybox, DirectionToUV(rd)).rgb;
    }
    R.u = normalize(R.u);

    ray Rp = cart_to_sph(R);
    Rp.u.x = sqrt(
        (
            ((Rp.u.y * Rp.u.y) / (1.0 - (rs / Rp.x.y)))
            + (Rp.x.y * Rp.x.y * Rp.u.z * Rp.u.z)
            + (Rp.u.w * Rp.u.w * Rp.x.y * Rp.x.y * sin(Rp.x.z) * sin(Rp.x.z)))
        / (1.0 - (rs / Rp.x.y))
        );

    for (int i = 0; i < N_STEPS; i++) {
        if (Rp.x.y < rs * 1.1) {
            return vec3(0.0, 0.0, 0.0);
        }

        vec3 prevCartPos = get_cartesian_pos(Rp.x);

        float current_dL;
        float r = Rp.x.y;

        if (r < 0.8) {
            current_dL = 0.003;
        } else if (r < 2.0) {
            current_dL = 0.01;
        } else if (r < 5.0) {
            current_dL = 0.05;
        } else {
            current_dL = 0.5;
        }

        ray der = integrate(Rp, current_dL);
        Rp.u = der.u;
        Rp.x = der.x;

        // Pole crossing fix
        if (Rp.x.z < 0.0) {
            Rp.x.z = -Rp.x.z;
            Rp.x.w += PI;
            Rp.u.z = -Rp.u.z;
        } else if (Rp.x.z > PI) {
            Rp.x.z = 2.0 * PI - Rp.x.z;
            Rp.x.w += PI;
            Rp.u.z = -Rp.u.z;
        }

        vec3 currentCartPos = get_cartesian_pos(Rp.x);

        // Accretion disk (swirl-based, quad-specific rendering)
        if (prevCartPos.z * currentCartPos.z < 0.0) {
            float t = abs(prevCartPos.z) / (abs(prevCartPos.z) + abs(currentCartPos.z));
            vec3 hitPos = mix(prevCartPos, currentCartPos, t);
            float hit_r = length(hitPos.xy);

            if (hit_r > 3.0 && hit_r < 15.0) {
                float edgeFade = smoothstep(15.0, 10.0, hit_r) * smoothstep(3.0, 4.0, hit_r);

                float angle = atan(hitPos.y, hitPos.x);
                float swirl = sin(hit_r * 8.0 - angle * 4.0 + (hit_r * 0.1)) * cos(hit_r * 3.0 + angle * 2.0 - (hit_r * 0.05));

                float density = edgeFade * (0.6 + 0.4 * swirl);

                if (density > 0.15) {
                    float heat = 1.0 - ((hit_r - 3.0) / 12.0);
                    vec3 diskColor = vec3(1.0, heat * 0.7, heat * 0.1);
                    return diskColor * clamp(density, 0.0, 1.0);
                }
            }
        }

        if (Rp.x.y > 20.0) {
            break;
        }
    }

    // Final direction conversion (spherical velocity → Cartesian)
    float r     = Rp.x.y;
    float theta = Rp.x.z;
    float phi   = Rp.x.w;

    float st = sin(theta);
    float ct = cos(theta);
    float sp = sin(phi);
    float cp = cos(phi);

    float vr     = Rp.u.y;
    float vtheta = Rp.u.z;
    float vphi   = Rp.u.w;

    vec3 dir =
          vr * vec3(st * cp, st * sp, ct)
        + r * vtheta * vec3(ct * cp, ct * sp, -st)
        + r * st * vphi * vec3(-sp, cp, 0.0);

    vec2 skyUV = DirectionToUV(normalize(dir));
    return texture(skybox, skyUV).rgb;
}

// --- main ---
void main() {
    if (uHighQualityPass &&
        length(TexCoords - vec2(0.5)) > uHighQualityRadius)
    {
        discard;
    }

    vec4 ndcPos = vec4(TexCoords.x * 2.0 - 1.0, TexCoords.y * 2.0 - 1.0, 1.0, 1.0);
    vec4 worldPos = uInvProjView * ndcPos;
    vec3 worldDir = normalize((worldPos.xyz / worldPos.w) - uCameraPos);

    vec3 finalColor = raymarch(uCameraPos, worldDir);

    FragColor = vec4(finalColor, 1.0);
}
