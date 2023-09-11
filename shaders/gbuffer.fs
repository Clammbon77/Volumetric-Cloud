#version 330 core

in vec3 worldPos;   
in vec2 texcoord;   
in vec3 normal;     

uniform sampler2D texture;

void main()
{
    gl_FragData[0] = texture2D(texture, texcoord);  // write gcolor
    gl_FragData[1] = vec4(normalize(normal), 0.0);  // write gnormal
    gl_FragData[2] = vec4(worldPos, 1.0);           // write gworldpos
}