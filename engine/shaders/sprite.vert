#version 450

// Vertex attributes
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec4 inColor;

// Outputs to fragment shader
layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec4 fragColor;

// Uniform buffer for projection matrix (set 0)
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 projection;
} ubo;

// Push constants for per-sprite data
layout(push_constant) uniform PushConstants {
    mat4 model;
    vec4 tintColor;
} pushConstants;

void main() {
    gl_Position = ubo.projection * pushConstants.model * vec4(inPosition, 0.0, 1.0);
    fragTexCoord = inTexCoord;
    fragColor = inColor * pushConstants.tintColor;
}
