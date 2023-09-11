#version 330 core

in vec3 worldPos;   // World Position of the current fragment
in vec2 texcoord;   // Texture Coordinate of the current fragment
in vec3 normal;     // Normal of the current fragment

out vec4 fColor;    // Output color of the fragment

uniform sampler2D texture;  // Texutre picture
uniform sampler2D shadowtex;	// shadow texture

uniform mat4 shadowVP;  // 转换到光源坐标的变换矩阵 

uniform vec3 lightPos;  // 光源位置
uniform vec3 cameraPos; // 相机位置

float shadowMapping(sampler2D tex, mat4 shadowVP, vec4 worldPos) {
	// 转换到光源坐标
	vec4 lightPos = shadowVP * worldPos;
	lightPos = vec4(lightPos.xyz/lightPos.w, 1.0);
	lightPos = lightPos*0.5 + 0.5;

	// 计算shadowmapping
	float closestDepth = texture2D(tex, lightPos.xy).r;	// shadowmap中最近点的深度
	float currentDepth = lightPos.z;	// 当前点的深度
	float isInShadow = (currentDepth>closestDepth+0.005) ? (1.0) : (0.0);

	return isInShadow;
}

float phong(vec3 worldPos, vec3 cameraPos, vec3 lightPos, vec3 normal)
{
    vec3 N = normalize(normal);
    vec3 V = normalize(worldPos - cameraPos);
    vec3 L = normalize(worldPos - lightPos);
    vec3 R = reflect(L, N);

    float ambient = 0.3;
    float diffuse = max(dot(N, -L), 0) * 0.7;
    float specular = pow(max(dot(-R, V), 0), 50.0) * 1.1;

    return ambient + diffuse + specular;
}

void main()
{
    fColor.rgb =  texture2D(texture, texcoord).rgb;

    float isInShadow = shadowMapping(shadowtex, shadowVP, vec4(worldPos, 1.0));

    if(isInShadow==0) {
        fColor.rgb *= phong(worldPos, cameraPos, lightPos, normal);
    } else {
        fColor.rgb *= 0.3;  // only ambient
    }
}