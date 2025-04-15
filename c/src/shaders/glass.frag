#version 450

layout(binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform GlassParams {
    float blurRadius;
    float opacity;
    vec4 edgeColor;
    float edgeThickness;
    vec4 glowColor;
    float glowIntensity;
    float animationProgress;
} params;

void main() {
    // Base color from background (blurred)
    vec4 baseColor = vec4(0.0);
    float blurSize = params.blurRadius * 0.01; // Normalize for texture coordinates
    
    // Apply Gaussian blur
    for(float x = -blurSize; x <= blurSize; x += blurSize/4.0) {
        for(float y = -blurSize; y <= blurSize; y += blurSize/4.0) {
            vec2 offset = vec2(x, y);
            baseColor += texture(texSampler, fragTexCoord + offset);
        }
    }
    baseColor /= 25.0; // Normalize (5x5 samples)
    
    // Apply glass tint and opacity
    vec4 glassColor = vec4(fragColor.rgb * 0.3, params.opacity);
    vec4 blendedColor = mix(baseColor, glassColor, glassColor.a);
    
    // Add edge glow effect
    float distanceFromEdge = min(min(fragTexCoord.x, 1.0 - fragTexCoord.x), 
                               min(fragTexCoord.y, 1.0 - fragTexCoord.y));
    float edgeGlow = smoothstep(params.edgeThickness, 0.0, distanceFromEdge);
    
    // Mix in the edge color with the blended color
    vec4 withEdgeColor = mix(blendedColor, params.edgeColor, edgeGlow);
    
    // Add animated neon glow
    float glowPulse = 0.7 + 0.3 * sin(params.animationProgress * 6.28318);
    vec4 withGlow = mix(withEdgeColor, params.glowColor, 
                       edgeGlow * params.glowIntensity * glowPulse);
    
    outColor = withGlow;
}