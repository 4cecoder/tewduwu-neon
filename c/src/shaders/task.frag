#version 450

layout(location = 0) in vec2 fragTexCoords;
layout(location = 1) in vec4 fragColor; // Base color
layout(location = 2) in float fragFlashIntensity; // Flash intensity

// Sampler removed - task background doesn't use texture
// layout(binding = 0, set = 1) uniform sampler2D taskSampler; 

layout(location = 0) out vec4 outColor;

// Flash color (Neon Green)
const vec4 flashColor = vec4(0.1, 1.0, 0.1, 1.0);

void main() {
    // Mix base color with flash color based on intensity
    outColor = mix(fragColor, flashColor, fragFlashIntensity);
}