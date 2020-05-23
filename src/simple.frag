#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 1) uniform sampler2D uTexSampler;

layout (location = 0) in vec3 iFragColor;
layout (location = 1) in vec2 iFragTexCoord;

layout (location = 0) out vec4 oOutColor;

void main() 
{
    vec4 texColor = texture(uTexSampler, iFragTexCoord);
    vec3 multipliedColor = vec3(iFragColor * texColor.rgb);
    vec3 whitened = mix(multipliedColor, vec3(1.0), 0.1);
    oOutColor = vec4(whitened, texColor.a);
}