const float DISK_INNER = 3.0;
const float DISK_OUTER = 15.0;
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
        
        // Sample 3D noise
        vec3 noiseCoord = vec3(diskR * cos(shearedPhi), diskR * sin(shearedPhi), currentPos.z * 1.5);
        float noiseVal = fBm(noiseCoord * 0.8);
        
        float radial01 = (diskR - DISK_INNER) / (DISK_OUTER - DISK_INNER);
        float edgeFade = smoothstep(0.0, 0.15, radial01) * (1.0 - smoothstep(0.8, 1.0, radial01));
        float verticalFade = 1.0 - (zDist / DISK_HEIGHT);
        
        // Threshold the noise to create clumpy plasma clouds
        float density = smoothstep(0.3, 0.7, noiseVal) * edgeFade * verticalFade;
        
        if (density > 0.0) {
            // 1. Calculate the Keplerian velocity vector of the plasma
            vec3 plasmaVel = normalize(vec3(-currentPos.y, currentPos.x, 0.0));
            
            // 2. Calculate Doppler beaming factor
            vec3 rayDir = normalize(currentPos - previousPos); 
            float beaming = dot(plasmaVel, rayDir);
            
            // 3. Map to exponential brightness
            float dopplerFactor = pow(1.5 + beaming, 3.0); 
            
            float stepSize = length(currentPos - previousPos);
            float alpha = clamp(density * stepSize * 1.5, 0.0, 1.0);
            
            float heat = 1.0 - radial01;
            vec3 innerColor = vec3(1.0, 0.8, 0.4);
            vec3 middleColor = vec3(0.9, 0.2, 0.01);
            vec3 outerColor = vec3(0.1, 0.0, 0.0);
            
            vec3 color = mix(outerColor, middleColor, heat);
            color = mix(color, innerColor, heat * heat);
            
            // Apply Doppler shift!
            color *= (1.0 + heat * 0.5) * dopplerFactor;
            
            accumColor += (1.0 - accumAlpha) * color * alpha;
            accumAlpha += (1.0 - accumAlpha) * alpha;
            
            if (accumAlpha > 0.99) return true;
        }
    }
    return false;
}