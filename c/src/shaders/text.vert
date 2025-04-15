#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoords;

layout(binding = 0, set = 0) uniform UniformBufferObject {
    mat4 projection;
    vec4 textColor;
    float flashIntensity;
} ubo;

layout(location = 0) out vec2 fragTexCoords;
layout(location = 1) out vec4 fragColor;

void main() {
    gl_Position = ubo.projection * vec4(inPosition, 0.0, 1.0);
    fragTexCoords = inTexCoords;
    fragColor = ubo.textColor;
}