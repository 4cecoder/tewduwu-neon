#version 450

layout(location = 0) in vec2 fragTexCoords;
layout(location = 1) in vec4 fragColor;

layout(binding = 0, set = 1) uniform sampler2D textSampler;

layout(location = 0) out vec4 outColor;

void main() {
    // TEMPORARY: Output vertex color directly, ignoring texture alpha
    outColor = fragColor; 
    // Original code:
    // float alpha = texture(textSampler, fragTexCoords).r;
    // outColor = vec4(fragColor.rgb, fragColor.a * alpha);
}