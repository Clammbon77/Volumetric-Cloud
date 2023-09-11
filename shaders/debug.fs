#version 330 core

in vec2 texcoord;
out vec4 fColor;

uniform sampler2D gcolor;
uniform sampler2D gnormal;
uniform sampler2D gdepth;
uniform sampler2D gworldpos;
uniform sampler2D shadowtex;

uniform float near;
uniform float far;

float linearizeDepth(float depth, float near, float far) {
    return (2.0 * near) / (far + near - depth * (far - near));
}

void main()
{       
    // фад╩вСоботй╬ gcolor
    if(0<=texcoord.x && texcoord.x<=0.5 && 0<=texcoord.y && texcoord.y<=0.5)
    {
        vec2 coord = vec2(texcoord.x*2, texcoord.y*2);
        fColor = vec4(texture2D(gcolor, coord).rgb, 1); 
    }

    // фад╩сроботй╬ gnormal
    if(0.5<=texcoord.x && texcoord.x<=1 && 0<=texcoord.y && texcoord.y<=0.5)
    {
        vec2 coord = vec2(texcoord.x*2-1, texcoord.y*2);
        fColor = vec4(texture2D(gnormal, coord).rgb, 1); 
    }

    // фад╩вСиоотй╬ gdepth
    if(0<=texcoord.x && texcoord.x<=0.5 && 0.5<=texcoord.y && texcoord.y<=1)
    {
        vec2 coord = vec2(texcoord.x*2, texcoord.y*2-1);
        float d = linearizeDepth(texture2D(gdepth, coord).r, near, far);
        //d = texture2D(shadowtex, coord).r;
        fColor = vec4(vec3(d*0.5+0.5), 1); 
    }

    // фад╩сриоотй╬ gworldpos
    if(0.5<=texcoord.x && texcoord.x<=1 && 0.5<=texcoord.y && texcoord.y<=1)
    {
        vec2 coord = vec2(texcoord.x*2-1, texcoord.y*2-1);
        fColor = vec4(texture2D(gworldpos, coord).rgb, 1); 
    }
}