#version 330 core

in vec2 texcoord;
out vec4 fColor;

// texture data
uniform sampler2D gcolor;
uniform sampler2D gnormal;
uniform sampler2D gdepth;
uniform sampler2D gworldpos;
uniform sampler2D shadowtex;	    

// near/far clipping face 
uniform float near;
uniform float far;

//transformation matrix to light source coord 
uniform mat4 shadowVP;  
uniform mat4 view;
uniform mat4 projection;

uniform vec3 lightPos;  
uniform vec3 cameraPos; 

uniform int FrameCounter;

// from screen depth to linera depth
float linearizeDepth(float depth) {
    return (2.0 * near) / (far + near - depth * (far - near));
}

float shadowMapping(sampler2D tex, mat4 shadowVP, vec4 worldPos) {
	// Transform to light source coord
	vec4 lightPos = shadowVP * worldPos;
	lightPos = vec4(lightPos.xyz/lightPos.w, 1.0);
	lightPos = lightPos*0.5 + 0.5;

    // Return a special value when out of shadow mapping range (skybox)
    if(lightPos.x<0 || lightPos.x>1 || lightPos.y<0 || lightPos.y>1 || lightPos.z<0 || lightPos.z>1) {
        return 2.0;
    }

	// Calculate shadowmapping
	float closestDepth = texture2D(tex, lightPos.xy).r;	
	float currentDepth = lightPos.z;	
	float isInShadow = (currentDepth>closestDepth+0.005) ? (1.0) : (0.0);

	return isInShadow;
}

// ------------------------------------------------------------------------ // 
// Phong illumination
struct PhongStruct
{
    float ambient;
    float diffuse;
    float specular;
};
PhongStruct phong(vec3 worldPos, vec3 cameraPos, vec3 lightPos, vec3 normal)
{
    vec3 N = normalize(normal);
    vec3 V = normalize(worldPos - cameraPos);
    vec3 L = normalize(worldPos - lightPos);
    vec3 R = reflect(L, N);

    PhongStruct phong;
    phong.ambient = 0.3;
    phong.diffuse = max(dot(N, -L), 0);
    phong.specular = pow(max(dot(-R, V), 0), 50.0) * 1.1;

    return phong;
}

// ------------------------------------------------------------------------ // 
// Cloud rendering
#define bottom 13   // BOTTOM OF CLOUD RANGE
#define top 20      // TOP OF CLOUD RANGE
#define width 100    // xz coord range of cloud [-width, width]

#define baseBright  vec3(1.26,1.25,1.29)    // Bright base color 
#define baseDark    vec3(0.31,0.31,0.32)    // Dark base color

#define lightBright vec3(1.29, 1.17, 1.05)  // Bright light color
#define lightDark   vec3(0.7,0.75,0.8)      // Dark light color

float random (in vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);
}

float noise (in vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);

    // Four corners in 2D of a tile
    float a = random(i);
    float b = random(i + vec2(1.0, 0.0));
    float c = random(i + vec2(0.0, 1.0));
    float d = random(i + vec2(1.0, 1.0));

    vec2 u = f * f * (3.0 - 2.0 * f);

    return mix(a, b, u.x) +
            (c - a)* u.y * (1.0 - u.x) +
            (d - b) * u.x * u.y;
}

#define NUM_OCTAVES 5
float fbm ( in vec2 st) {
    float v = 0.0;
    float a = 0.5;
    vec2 shift = vec2(100.0);
    // Rotate to reduce axial bias
    mat2 rot = mat2(cos(0.5), sin(0.5),
                    -sin(0.5), cos(0.50));
    for (int i = 0; i < NUM_OCTAVES; ++i) {
        v += a * noise(st);
        st = rot * st * 2.0 + shift;
        a *= 0.5;
    }
    return v;
}

// Cloud density 
float getDensity(vec3 pos) {

    // The shape of cloud 
    // Density high at middle and low at bottom and top
    float mid = (bottom+top)/2.0;
    float h = top - bottom;
    float weight = 1.0 - 2.0 * abs(mid - pos.y) / h;
    weight = pow(weight, 0.5);

    vec2 coord = pos.xz * 0.2;
    
    vec2 q = vec2(0.);
    q.x = fbm( coord );
    q.y = fbm( coord + vec2(1.0));

    vec2 r = vec2(0.);
    r.x = fbm( coord + 1.0*q + vec2(1.7,9.2)+ 0.15 * float(FrameCounter)*0.006 );
    r.y = fbm( coord + 1.0*q + vec2(8.3,2.8)+ 0.126 * float(FrameCounter)*0.006 );

    float f = fbm(coord+r);

    float noise = mix(0, 1, clamp((f*f)*4.0,0.0,1.0));
    noise = mix(noise, 0.5, clamp(length(q),0.0,1.0));
    noise =  mix(noise, 1.024, clamp(length(r.x),0.0,1.0));

    noise = (f*f*f+.6*f*f+.5*f) * noise;

    noise *= weight;

    // Cut cloud to individuals
    if(noise < 0.45){
        noise = 0;
    }
    return noise;
}

// Cloud color
vec4 getCloud(vec3 worldPos, vec3 cameraPos) {
    vec3 direction = normalize(worldPos - cameraPos);   
    vec3 step = direction * 0.25;   
    vec4 colorSum = vec4(0);        // accumlated color
    vec3 point = cameraPos;         // start from camera

    // Move the start point to the bottom when the camera is under the cloud
    if(point.y<bottom) {
        point += direction * (abs(bottom - cameraPos.y) / abs(direction.y));
    }
    // Move the start point to the top when the camera is above the cloud
    if(top<point.y) {
        point += direction * (abs(cameraPos.y - top) / abs(direction.y));
    }

    // Stop test if the target pixel cover the cloud
    float len1 = length(point - cameraPos);     // length from cloud to the camera
    float len2 = length(worldPos - cameraPos);  // length from target pixel to the camera
    if(len2<len1) {
        return vec4(0);
    }

    // Ray Marching
    for(int i=0; i<100; i++) {
        point += step * (1.0 + i * 0.05);
        if(bottom>point.y || point.y>top || -width>point.x || point.x>width || -width>point.z || point.z>width) {
            break;
        }

        // Transform screen coord
        vec4 screenPos = projection * view * vec4(point, 1.0);
        screenPos /= screenPos.w;
        screenPos.xyz = screenPos.xyz * 0.5 + 0.5;

        // Depth sampling
        float sampleDepth = texture2D(gdepth, screenPos.xy).r;   
        float testDepth = screenPos.z;  

        // Depth linearization 
        sampleDepth = linearizeDepth(sampleDepth);
        testDepth = linearizeDepth(testDepth);

        // Break if hit
        if(sampleDepth<testDepth) {
             break;
         }

        // Illumination effect
        float density = getDensity(point);           
        vec3 L = normalize(lightPos - point);           
        float lightDensity = getDensity(point + L); 
        float delta = clamp(density - lightDensity, 0.0, 1.0); 

        // Transparecncy
        density *= 0.5;
 
        vec3 base = mix(baseBright, baseDark, density) * density;   
        vec3 light = mix(lightDark, lightBright, delta);           

        vec4 color = vec4(base*light, density);             // color of the current point
        colorSum = colorSum + color * (1.0 - colorSum.a);   // mix with the accumlated color
    }

    return colorSum;
}


// ------------------------------------------------------------------------ // 
void main()
{   
    fColor.rgb = texture2D(gcolor, texcoord).rgb;
    vec3 worldPos = texture2D(gworldpos, texcoord).xyz;
    vec3 normal = texture2D(gnormal, texcoord).xyz;

    float isInShadow = shadowMapping(shadowtex, shadowVP, vec4(worldPos, 1.0));
    PhongStruct phong = phong(worldPos, cameraPos, lightPos, normal);

    if(isInShadow==0) {
        fColor.rgb *= phong.ambient + phong.diffuse + phong.specular;
    } else if(isInShadow==1.0) {
        fColor.rgb *= phong.ambient;  // only ambient if it is in shadow
    }
     
    vec4 cloud = getCloud(worldPos, cameraPos);             // cloud color
    fColor.rgb = fColor.rgb*(1.0 - cloud.a) + cloud.rgb;    // mix color with cloud
}