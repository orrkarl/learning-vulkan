#version 450

layout (binding = 0) uniform MVPTransform
{
    mat4 transform;
} uMVP;

layout (location = 0) in vec2 iPosition;
layout (location = 1) in vec3 iColor;

layout (location = 0) out vec3 oFragColor;

void main()
{
    gl_Position = uMVP.transform * vec4(iPosition, 0.0, 1.0);
    oFragColor = iColor;
}