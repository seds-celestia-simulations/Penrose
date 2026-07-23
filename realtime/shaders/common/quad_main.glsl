float rayParticleHit(vec3 rayPos, Particle p) {
    vec3 toParticle = p.stateX.xyz - rayPos;
    float squaredDist = dot(toParticle, toParticle);
    float squaredRadius = p.radius * p.radius;
    return squaredDist - squaredRadius;
}

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
