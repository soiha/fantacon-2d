#version 450

// Inputs from vertex shader
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;

// Output color
layout(location = 0) out vec4 outColor;

// Texture sampler (set 1)
layout(set = 1, binding = 0) uniform sampler2D texSampler;

void main() {
    // Sample texture and modulate with vertex color (includes tint and opacity)
    outColor = texture(texSampler, fragTexCoord) * fragColor;
}
