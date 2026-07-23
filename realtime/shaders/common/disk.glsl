const float DISK_INNER = 3.0;
const float DISK_OUTER = 15.0;

bool accumulateDisk(vec3 previousPos, vec3 currentPos, float uTime,
                    inout vec3 accumColor, inout float accumAlpha) {
    float z0 = previousPos.z;
    float z1 = currentPos.z;

    if (z0 * z1 <= 0.0) {
        float t = abs(z0) / max(abs(z0) + abs(z1), 1e-8);
        vec3 hitPos = mix(previousPos, currentPos, t);
        float diskR = length(hitPos.xy);

        if (diskR > DISK_INNER && diskR < DISK_OUTER) {
            float phi = atan(hitPos.y, hitPos.x);
            float omega = 3.0 / pow(diskR, 1.5);
            float shearedPhi = phi + omega * uTime;

            vec2 noiseCoord = vec2(diskR * cos(shearedPhi), diskR * sin(shearedPhi));
            float noiseVal = fBm(noiseCoord * 0.8);

            float radial01 = (diskR - DISK_INNER) / (DISK_OUTER - DISK_INNER);
            float edgeFade = smoothstep(0.0, 0.1, radial01) * (1.0 - smoothstep(0.8, 1.0, radial01));

            float alpha = smoothstep(0.2, 0.7, noiseVal) * edgeFade * 0.85;

            float heat = 1.0 - radial01;
            vec3 innerColor = vec3(1.0, 0.8, 0.4);
            vec3 middleColor = vec3(0.9, 0.2, 0.01);
            vec3 outerColor = vec3(0.1, 0.0, 0.0);

            vec3 color = mix(outerColor, middleColor, heat);
            color = mix(color, innerColor, heat * heat);
            color *= (1.0 + heat * 2.0);

            accumColor += (1.0 - accumAlpha) * color * alpha;
            accumAlpha += (1.0 - accumAlpha) * alpha;

            if (accumAlpha > 0.99) return true;
        }
    }
    return false;
}
