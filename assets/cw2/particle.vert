#version 430
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexcoord;

layout(location = 0) uniform mat4 uProjection;

out vec2 vTexcoord;

void main()
{
    vTexcoord = inTexcoord;
    gl_Position = uProjection * vec4(inPosition, 1.0);
}