#include "../common/skybox.glsl"

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
