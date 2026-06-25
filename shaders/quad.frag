#version 430 core
out vec4 FragColor;
in vec2 TexCoords;

uniform float uTime;
uniform vec2 uResolution;
uniform vec3 uCameraPos;

uniform sampler2D skybox;       // The equirectangular texture sampler
uniform mat4 uInvProjView;      // The inverse View-Projection matrix

const float PI = 3.14159265359;
const float dL = 0.01;
const float rs = 0.25;
const int N_STEPS = 1000;

struct ray {
    vec4 x;
    vec4 u;
};

struct particle {
    vec4 x;
    vec4 u;
    vec3 color;
    float m;
    float r;
};

layout(std430, binding = 0) readonly buffer ParticleBuffer {
    particle particles[];
};

uniform int uParticleCount;

mat4 sph_to_cart_Jacobian(float r, float theta, float phi){ 
    return mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0, sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta),
        0.0, r*cos(theta)*cos(phi), r*cos(theta)*sin(phi), -r*sin(theta),
        0.0, -r*sin(theta)*sin(phi), r*sin(theta)*cos(phi), 0.0
    );
}
mat4 cart_to_sph_Jacobian(float r, float theta, float phi){
    return inverse(mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0, sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta),
        0.0, r*cos(theta)*cos(phi), r*cos(theta)*sin(phi), -r*sin(theta),
        0.0, -r*sin(theta)*sin(phi), r*sin(theta)*cos(phi), 0.0
    ));
}
ray cart_to_sph(ray R) {
    vec3 p = R.x.yzw;
    float r = length(p);
    float theta = acos(clamp(p.z/(r+1e-10), -1.0, 1.0)); // Avoid division by zero
    float phi = atan(p.y, p.x);
    return ray(vec4(R.x.x,r,theta,phi), cart_to_sph_Jacobian(r, theta, phi) * R.u);
}
ray sph_to_cart(ray R) {
    float r = R.x.y;
    float theta = R.x.z;
    float phi = R.x.w;
    return ray(vec4(R.x.x, r*sin(theta)*cos(phi), r*sin(theta)*sin(phi), r*cos(theta)), sph_to_cart_Jacobian(r, theta, phi) * R.u);
}

vec4 find_acceleration(ray R){
    float r = R.x[1];
    float theta = clamp(R.x[2], 1e-10, PI - 1e-10);

    float vt    =R.u[0];
    float vr    =R.u[1];
    float vtheta=R.u[2];
    float vphi  =R.u[3];
    // TODO
    // Replace all  sin cos with precomputes
    // replace r-rs with metric component + 1e-10 for diviide by zero shit
    float sin_theta = sin(theta);
    float cos_theta = cos(theta);
    float tan_theta = tan(theta);

    // T(r) component(radial component)
    float Tr_tt = rs * (r-rs)/(2.0*r*r*r); 
    float Tr_rr = -rs/(2.0*r*(r-rs));
    float Tr_thth = -(r-rs);
    float Tr_phph = -(r-rs)*sin_theta*sin_theta;
    //polar component
    float Tth_rth = 1.0/r;
    float Tth_phph = -sin_theta*cos_theta;
    //azimuthal component 
    float Tph_rph = 1.0/r;
    float Tph_thph = 1.0/(tan_theta);
    //Time-component
    float Tt_tr = rs/(2*r*(r-rs)); 
    // T(t) component(accelerationequations)
    float at = -2*(Tt_tr)*vt*vr;
    float ar = -(Tr_tt*vt*vt + Tr_rr*vr*vr + Tr_thth*vtheta*vtheta + Tr_phph*vphi*vphi);
    float atheta = -(2*Tth_rth*vr*vtheta + Tth_phph*vphi*vphi);
    float aphi = -(2*Tph_rph*vr*vphi + 2* Tph_thph*vtheta*vphi);

    return vec4(at, ar, atheta, aphi);
}

ray create_ray_derivative(ray R){
    ray res=ray(vec4(R.u),find_acceleration(R));
    return res;
}

ray integrate(ray R){
    ray k1 = create_ray_derivative(R);    
    ray k2 = create_ray_derivative(ray(R.x + (k1.x * (dL/2.0)),R.u + (k1.u * (dL/2.0))));
    ray k3 = create_ray_derivative(ray(R.x + (k2.x * (dL/2.0)),R.u + (k2.u * (dL/2.0))));
    ray k4 = create_ray_derivative(ray(R.x + (k3.x * dL),R.u + (k3.u * dL)));

    ray s_final = ray(R.x + (dL/6.0 )*(k1.x + 2.0*k2.x + 2.0*k3.x + k4.x),R.u + (dL/6.0 )*(k1.u + 2.0*k2.u + 2.0*k3.u + k4.u));

    return s_final;    
}

vec2 DirectionToUV(vec3 dir) {
    float u = 0.5 + atan(dir.x, dir.z) / (2.0 * PI);
    float v = 0.5 - asin(dir.y) / PI;
    // float v = dir.y*0.5+0.5;
    return vec2(u, v);
}

// Check if a ray hits a particle
float rayParticleHit(ray R, particle p) {
    // Extract particle position from stateX (spherical coords -> cartesian)
    float r = p.x.y;
    float theta = p.x.z;
    float phi = p.x.w;
    
    vec3 particlePos = vec3(
        r * sin(theta) * cos(phi),
        r * sin(theta) * sin(phi),
        r * cos(theta)
    );
    // vec3 particlePos = p.x.yzw;
    
    // Simple sphere intersection
    vec3 toParticle = particlePos - R.x.yzw;
    float d= length(toParticle);
    return d-p.r;
}

vec3 raymarch(vec3 ro, vec3 rd) {
    ray R = ray(vec4(0.0, ro), vec4(0.0, rd));

    //FIND u.w HERE
    R.u = normalize(R.u);

    ray Rp = cart_to_sph(R);
    //t-0
    //r-1
    //theta-2
    //phi-3
    Rp.u.x = sqrt(
        (
            ((Rp.u.y*Rp.u.y)/(1.0-(rs/Rp.x.y)))
            +(Rp.x.y*Rp.x.y*Rp.u.z*Rp.u.z)
            +(Rp.u.w*Rp.u.w*Rp.x.y*Rp.x.y*sin(Rp.x.z)*sin(Rp.x.z)))
        /(1.0-(rs/Rp.x.y))
        ); //NULL CONSTRAINT

    R = sph_to_cart(Rp);

    //converting cartesian to polar for u and x using jacobian
    

    for(int i = 0; i < N_STEPS; i++) {
        if(length(Rp.x[1]) < rs*1.1) { // A simple sphere at (0,0,5) with radius 0.5
            return vec3(0.0, 0.0, 0.0); // Hit the sphere, return red
        }
        R = sph_to_cart(Rp);
       
        for (int j = 0; j < uParticleCount; j++) {
            if (rayParticleHit(R, particles[j]) <0.0) {
                return particles[j].color; // Return the particle's color if hit
            }
        }

        if(Rp.x.y > 20.0) {
            break; 
        }
        ray der = integrate(Rp);
        Rp.u = der.u;
        Rp.x = der.x;

        // POLE CROSSING FIX: Keep theta strictly between 0 and PI
        if (Rp.x.z < 0.0) {
            Rp.x.z = -Rp.x.z;           // Bounce theta back to positive
            Rp.x.w += PI;               // Rotate to the other side of the pole
            Rp.u.z = -Rp.u.z;           // Reverse theta velocity
        } else if (Rp.x.z > PI) {
            Rp.x.z = 2.0 * PI - Rp.x.z; // Bounce back from PI
            Rp.x.w += PI;               // Rotate to the other side of the pole
            Rp.u.z = -Rp.u.z;           // Reverse theta velocity
        }
    }
    R = sph_to_cart(Rp);

    vec2 skyUV = DirectionToUV(normalize(R.u.yzw));
    return texture(skybox, skyUV).rgb; // Return black for now
    // return vec3(skyUV,1.0).rgb; // Return black for now
}

void main() {
    // 1. Generate the initial ray u from this pixel
    // Start with NDC coordinates: x,y in [-1,1]
    
    vec4 ndcPos = vec4(TexCoords.x * 2.0 - 1.0, TexCoords.y * 2.0 - 1.0, 1.0, 1.0);
    // Un-project from NDC to World Space
    vec4 worldPos = uInvProjView * ndcPos;
    // The ray u is the un-projected u, normalized
    vec3 worldDir = normalize((worldPos.xyz / worldPos.w)-uCameraPos);

    vec3 finalColor = raymarch(uCameraPos, worldDir);

    // A small animation test to confirm uniforms still work
    // finalColor *= abs(sin(uTime * 0.2)) * 0.2 + 0.8;

    FragColor = vec4(finalColor, 1.0);
}