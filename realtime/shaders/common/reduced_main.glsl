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
    float dPsi = 0.03 * stepSign;
    float psi = 0.0;

    vec3 previousPos = orbitPosition(r0, psi, eR, eT);

    vec3 accumColor = vec3(0.0);
    float accumAlpha = 0.0;

    bool captured = false;
    bool escaped = false;

    for (int i = 0; i < 150; ++i) {
        orbitRK4(u, v, dPsi);
        psi += dPsi;

        if (u <= 0.0) break;

        float r = 1.0 / u;
        vec3 currentPos = orbitPosition(r, psi, eR, eT);

        for (int p = 0; p < particles.length(); ++p) {
            vec3 center = particles[p].stateX.xyz;
            float radius = particles[p].radius;

            vec3 d = currentPos - previousPos;
            vec3 f = previousPos - center;

            float a = dot(d, d);
            float b = 2.0 * dot(f, d);
            float c = dot(f, f) - radius * radius;

            float discriminant = b * b - 4.0 * a * c;

            if (discriminant > 0.0) {
                discriminant = sqrt(discriminant);
                float t1 = (-b - discriminant) / (2.0 * a);
                float t2 = (-b + discriminant) / (2.0 * a);

                if ((t1 >= 0.0 && t1 <= 1.0) || (t2 >= 0.0 && t2 <= 1.0)) {
                    float pAlpha = 0.95;
                    accumColor += (1.0 - accumAlpha) * particles[p].color * pAlpha;
                    accumAlpha += (1.0 - accumAlpha) * pAlpha;
                }
            }
        }

        if (accumulateDisk(previousPos, currentPos, uTime, accumColor, accumAlpha)) break;

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

    if (!captured) {
        float r = 1.0 / u;
        float drdPsi = -v / max(u * u, 1e-8);
        vec3 er = cos(psi) * eR + sin(psi) * eT;
        vec3 epsi = -sin(psi) * eR + cos(psi) * eT;
        vec3 outgoingDir = normalize(drdPsi * er + r * epsi);

        vec3 skyColor = texture(skybox, DirectionToUV(outgoingDir)).rgb;
        finalColor += (1.0 - accumAlpha) * skyColor;
    }

    FragColor = vec4(finalColor, 1.0);
}
