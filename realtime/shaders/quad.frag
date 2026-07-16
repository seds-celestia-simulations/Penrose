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
const int N_STEPS = 500;
struct ray {
    vec4 x;
    vec4 u;
};

struct particle {
    vec4 stateX; 
    vec4 stateU; 
    vec3 color;
    float mass;
    float radius;
};

layout(std430, binding = 0) readonly buffer ParticleBuffer {
    particle particles[];
};

uniform int uParticleCount;

uniform bool uHighQualityPass;
uniform float uHighQualityRadius;

// mat4 sph_to_cart_Jacobian(float r, float theta, float phi){ 
//     return mat4(
//         1.0, 0.0, 0.0, 0.0,
//         0.0, sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta),
//         0.0, r*cos(theta)*cos(phi), r*cos(theta)*sin(phi), -r*sin(theta),
//         0.0, -r*sin(theta)*sin(phi), r*sin(theta)*cos(phi), 0.0
//     );
// }
// Much faster: only computes position, ignores velocity matrices
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
// mat4 cart_to_sph_Jacobian(float r, float theta, float phi){
//     return inverse(mat4(
//         1.0, 0.0, 0.0, 0.0,
//         0.0, sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta),
//         0.0, r*cos(theta)*cos(phi), r*cos(theta)*sin(phi), -r*sin(theta),
//         0.0, -r*sin(theta)*sin(phi), r*sin(theta)*cos(phi), 0.0
//     ));
// }
float dFactor = 1e-5; //division factor, basic float wont read 1e-10
ray cart_to_sph(ray R)
{
    vec3 p = R.x.yzw;
    vec3 v = R.u.yzw;

    float r = length(p);

    float theta = acos(clamp(p.z / max(r,1e-6),-1.0,1.0));

    float phi = atan(p.y,p.x);

    float vr = dot(p,v) / r;

    float xy = max(p.x*p.x + p.y*p.y,1e-6);

    float vphi = (-p.y*v.x + p.x*v.y) / xy;

    float rho = sqrt(max(xy,1e-6));

    float vtheta = (p.z*(p.x*v.x + p.y*v.y) -xy*v.z)/(r*r*rho);

    return ray(
        vec4(R.x.x,r,theta,phi),
        vec4(R.u.x,vr,vtheta,vphi)
    );
}
ray sph_to_cart(ray R)
{
    float r     = R.x.y;
    float theta = R.x.z;
    float phi   = R.x.w;

    float st = sin(theta);
    float ct = cos(theta);

    float sp = sin(phi);
    float cp = cos(phi);

    vec3 pos = vec3(r*st*cp, r*st*sp, r*ct);

    float vr     = R.u.y;
    float vtheta = R.u.z;
    float vphi   = R.u.w;

    vec3 vel =
          vr * vec3(st*cp,st*sp,ct)
        + r*vtheta * vec3(ct*cp,ct*sp,-st)
        + r*st*vphi * vec3(-sp,cp,0.0);

    return ray(
        vec4(R.x.x,pos),
        vec4(R.u.x,vel)
    );
}

vec4 find_acceleration(ray R){
    float r = R.x[1];
    float theta = clamp(R.x[2], dFactor, PI - dFactor);

    float vt    =R.u[0];
    float vr    =R.u[1];
    float vtheta=R.u[2];
    float vphi  =R.u[3];
    // TODO
    // Replace all  sin cos with precomputes
    // replace r-rs with metric component + 1e-10 for diviide by zero shit
    float sin_theta = sin(theta);
    float cos_theta = cos(theta);
   // float tan_theta = tan(theta);

    // T(r) component(radial component)
    float Tr_tt = rs * (r-rs)/(2.0*r*r*r); 
    float Tr_rr = -rs/(2.0*r*(r-rs) + dFactor);
    float Tr_thth = -(r-rs);
    float Tr_phph = -(r-rs)*sin_theta*sin_theta;
    //polar component
    float Tth_rth = 1.0/r;
    float Tth_phph = -sin_theta*cos_theta;
    //azimuthal component 
    float Tph_rph = 1.0/r;
    float Tph_thph = cos_theta/sin_theta;
    //Time-component
    float Tt_tr = rs/(2*r*(r-rs) + dFactor); 
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

ray integrate(ray R, float dL){
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
float rayParticleHit(vec3 rayPos, particle p) {
    // Both are now Cartesian, so we just subtract!
    vec3 toParticle = p.stateX.xyz - rayPos;
    
    // Fastest squared distance check
    float squaredDist = dot(toParticle, toParticle); 
    float squaredRadius = p.radius * p.radius;
    
    return squaredDist - squaredRadius; 
}

vec3 raymarch(vec3 ro, vec3 rd) {
    ray R = ray(vec4(0.0, ro), vec4(0.0, rd));

    //FIND u.w HERE
    vec3 toBH = -ro;

    // Closest distance from the camera ray to the black-hole center
    float closestApproach = length(cross(rd, normalize(toBH))) * length(toBH);

    // Rays far from the hole barely bend.
    // Sample the sky immediately.
    if (closestApproach > 3.0) {
        return texture(skybox, DirectionToUV(rd)).rgb;
    }
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
    //converting cartesian to polar for u and x using jacobian
    

    for(int i = 0; i < N_STEPS; i++) {
        if(Rp.x.y < rs*1.1) { // A simple sphere at (0,0,5) with radius 0.5
            return vec3(0.0, 0.0, 0.0); // Hit the sphere, return red
        }
      //  R = sph_to_cart(Rp);
// 1. Record where the ray was BEFORE moving
        vec3 prevCartPos = get_cartesian_pos(Rp.x);

       // Adaptive steps 
            float current_dL;
            float r = Rp.x.y;

            if (r < 0.8) {
                current_dL = 0.003;
            }
            else if (r < 2.0) {
                current_dL = 0.01;
            }
            else if (r < 5.0) {
                current_dL = 0.05;
            }
            else {
                current_dL = 0.5;
}
        // 2. Step the ray forward
        ray der = integrate(Rp, current_dL);
        Rp.u = der.u;
        Rp.x = der.x;

        // POLE CROSSING FIX: Keep theta strictly between 0 and PI
        if (Rp.x.z < 0.0) {
            Rp.x.z = -Rp.x.z;           
            Rp.x.w += PI;               
            Rp.u.z = -Rp.u.z;           
        } else if (Rp.x.z > PI) {
            Rp.x.z = 2.0 * PI - Rp.x.z; 
            Rp.x.w += PI;               
            Rp.u.z = -Rp.u.z;           
        }

        // 3. Record where the ray is AFTER moving
        vec3 currentCartPos = get_cartesian_pos(Rp.x);
        
// 4. THE PROCEDURAL ACCRETION DISK
        if (prevCartPos.z * currentCartPos.z < 0.0) {
            
            // Calculate exact piercing point
            float t = abs(prevCartPos.z) / (abs(prevCartPos.z) + abs(currentCartPos.z));
            vec3 hitPos = mix(prevCartPos, currentCartPos, t);
            float hit_r = length(hitPos.xy);

            // Broad-phase: Are we in the disk radius?
            if (hit_r > 3.0 && hit_r < 15.0) {
                
                // 1. Base Density (Smoothly fades out at the inner/outer edges)
                float edgeFade = smoothstep(15.0, 10.0, hit_r) * smoothstep(3.0, 4.0, hit_r);

                // 2. Procedural Swirl (Simulating matter clumping & orbiting)
                float angle = atan(hitPos.y, hitPos.x);
                
                // Fast trigonometric noise layered to look like fluid dynamics
                float swirl = sin(hit_r * 8.0 - angle * 4.0 + (hit_r * 0.1)) * cos(hit_r * 3.0 + angle * 2.0 - (hit_r * 0.05));
                
                // Combine fade and swirl into a final density value
                float density = edgeFade * (0.6 + 0.4 * swirl);

                // 3. Render the matter if the density is high enough
                if (density > 0.15) {
                    // Hotter/yellow near the center, cooler/red on the outer edge
                    float heat = 1.0 - ((hit_r - 3.0) / 12.0);
                    vec3 diskColor = vec3(1.0, heat * 0.7, heat * 0.1);
                    
                    // Return the color mixed with the density for a glowing, dusty look
                    return diskColor * clamp(density, 0.0, 1.0);
                }
            }
        }

        if(Rp.x.y > 20.0) {
            break; 
        }

 }
        // Convert only the final spherical velocity direction to Cartesian.
// No matrix, no sph_to_cart() call.

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

// Cartesian spatial velocity from spherical-coordinate velocity
vec3 dir =
      vr         * vec3(st * cp, st * sp, ct)
    + r * vtheta * vec3(ct * cp, ct * sp, -st)
    + r * st * vphi * vec3(-sp, cp, 0.0);

vec2 skyUV = DirectionToUV(normalize(dir));
return texture(skybox, skyUV).rgb;
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

    vec3 finalColor = raymarch(uCameraPos, worldDir);

    // A small animation test to confirm uniforms still work
    // finalColor *= abs(sin(uTime * 0.2)) * 0.2 + 0.8;

    FragColor = vec4(finalColor, 1.0);
}