#version 330 core

#extension GL_NV_shadow_samplers_cube : enable

in vec3 texcoord;
out vec4 fColor;

uniform samplerCube skybox;

uniform float near;
uniform float far;

void main()
{
    gl_FragData[0] = textureCube(skybox, texcoord); // write gcolor
    gl_FragData[1] = vec4(vec3(0), 0.0);            // write gnormal
    gl_FragData[2] = vec4(texcoord*far*2, 1.0);     // write gworldpos
}
