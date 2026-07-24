const float DISK_INNER = 0.75;
const float DISK_OUTER = 5.0;
const float DISK_HEIGHT = 1.5; 

bool accumulateVolume(vec3 currentPos, vec3 previousPos, float uTime,
                      inout vec3 accumColor, inout float accumAlpha) {
    
    float diskR = length(currentPos.xy);
    float zDist = abs(currentPos.z);
    
    // Bounding Volume Check: Are we inside the thick disk?
    if (diskR > DISK_INNER && diskR < DISK_OUTER && zDist < DISK_HEIGHT) {
        
        float phi = atan(currentPos.y, currentPos.x);
        float omega = 3.0 / pow(diskR, 1.5);
        float shearedPhi = phi + omega * uTime;
        
        // Sample 3D noise with original trig method but reduced scaling for precision
        vec3 noiseCoord = vec3(diskR * cos(shearedPhi), diskR * sin(shearedPhi), currentPos.z);
        float noiseVal = fBm(noiseCoord * 4.0);
        noiseVal += 0.5 * fBm(noiseCoord * 4.0 + vec3(10.5, 20.3, 5.7));
        noiseVal += 0.25 * fBm(noiseCoord * 16.0 + vec3(42.1, 13.9, 31.2));
        noiseVal /= 1.75;
        
        float radial01 = (diskR - DISK_INNER) / (DISK_OUTER - DISK_INNER);
        float edgeFade = smoothstep(0.0, 0.15, radial01) * (1.0 - smoothstep(0.6, 1.0, radial01));
        // Stable vertical fade: normalize z relative to disk height with proper epsilon
        float normalizedZ = min(zDist / max(DISK_HEIGHT, 1e-6), 1.0);
        float verticalFade = 1.0 - smoothstep(0.5, 1.0, normalizedZ);
        
        // Softer noise thresholding to reduce ring artifacts
        float density = smoothstep(0.2, 0.8, noiseVal) * edgeFade * verticalFade;
        
        if (density > 0.0) {
            // 1. Calculate the retrograde Keplerian velocity vector of the plasma
            vec3 plasmaVel = normalize(vec3(currentPos.y, -currentPos.x, 0.0));
            
            // 2. Calculate Doppler beaming factor with stability guard
            vec3 delta = currentPos - previousPos;
            vec3 rayDir = normalize(delta + vec3(1e-8));
            float beaming = dot(plasmaVel, rayDir);
            
            // 3. Map to exponential brightness (reduced exponent to prevent oversaturation)
            float dopplerFactor = pow(1.5 + beaming, 3.0); 
            
            float stepSize = length(currentPos - previousPos);
            float alpha = clamp(density * stepSize * 1.5, 0.0, 1.0);
            
            float heat = 1.0 - radial01;
            vec3 innerColor = vec3(1.0, 0.8, 0.4);
            vec3 middleColor = vec3(0.9, 0.2, 0.01);
            vec3 outerColor = vec3(0.1, 0.0, 0.0);
            
            vec3 color = mix(outerColor, middleColor, heat);
            color = mix(color, innerColor, heat * heat);
            
            // Apply clamped Doppler shift to prevent unbounded saturation
            float dopplerBrightness = clamp((1.0 + heat * 0.5) * dopplerFactor, 0.5, 2.5);
            color *= dopplerBrightness;
            
            accumColor += (1.0 - accumAlpha) * color * alpha;
            accumAlpha += (1.0 - accumAlpha) * alpha;

            if (accumAlpha > 0.99) return true;
        }
    }
    return false;
}