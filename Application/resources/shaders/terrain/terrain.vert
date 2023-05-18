#version 460 core

#define MAX_HEIGHT 180.f
#define TERRAIN_INSTANCED_RENDERING

#ifdef TERRAIN_INSTANCED_RENDERING

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 position_instance;
layout (location = 2) in vec2 clipmapcenter_instance;
layout (location = 3) in float level_instance;
layout (location = 4) in mat4 model_instance;

uniform mat4 PV;

out vec3 WorldPos;
out vec3 Normal;
out vec2 TexCoords;
out mat3 TBN;
out vec3 TangentViewPos;
out vec3 TangentFragPos;
out float Scale;
out float Level;

uniform sampler2DArray heightmapArray;
uniform float texSize;
//uniform vec3 camPoss;
uniform vec3 camPos;

void main(void)
{
    // set position in xz plane
    vec3 p = vec3(model_instance * vec4(aPos, 1));
    float scale = pow(2, float(level_instance));
    vec3 pos = vec3(position_instance.x, 0, position_instance.y) + scale * p;

    // set texture coordinates
    float terrainClipSize = scale * texSize;
    vec2 texCoords = mod(vec2(pos.x, pos.z), terrainClipSize);
    texCoords /= terrainClipSize;

    vec2 heightSample = texture(heightmapArray, vec3(texCoords.xy, level_instance)).rg;
    vec2 index0 = textureOffset(heightmapArray, vec3(texCoords.xy, level_instance), ivec2(0, -1)).rg;
    vec2 index1 = textureOffset(heightmapArray, vec3(texCoords.xy, level_instance), ivec2(-1, 0)).rg;
    vec2 index2 = textureOffset(heightmapArray, vec3(texCoords.xy, level_instance), ivec2(1, 0)).rg;
    vec2 index3 = textureOffset(heightmapArray, vec3(texCoords.xy, level_instance), ivec2(0, 1)).rg;

    // bu sekilde gelistirmeye acik birakmak cok mantikli ?
    float height = (heightSample.r * 255 * 256 + heightSample.g * 255) * (MAX_HEIGHT / (256 * 256 - 1));
    pos.y = height;

    float h0 = (index0.r * 255 * 256 + index0.g * 255) * (MAX_HEIGHT / (256 * 256 - 1));
    float h1 = (index1.r * 255 * 256 + index1.g * 255) * (MAX_HEIGHT / (256 * 256 - 1));
    float h2 = (index2.r * 255 * 256 + index2.g * 255) * (MAX_HEIGHT / (256 * 256 - 1));
    float h3 = (index3.r * 255 * 256 + index3.g * 255) * (MAX_HEIGHT / (256 * 256 - 1));

    vec3 normal;
	normal.z = h0 - h3;
	normal.x = h1 - h2;
	normal.y = 2 * scale;
	normal = normalize(normal);

    vec3 T = normalize(vec3(2 * scale, h2 - h1, 0));
    vec3 B = -normalize(cross(T,normal));
    TBN = mat3(T, B, normal);

    vec3 T_ = normalize(vec3(2 * scale, h2 - h1, 0));
    vec3 B_ = normalize(cross(T_,normal));
    mat3 TBN_ = transpose(mat3(T_, B_, normal));

    WorldPos = pos;
    TangentViewPos = TBN_* camPos;
    TangentFragPos = TBN_* pos;
    TexCoords.x = pos.x;
    TexCoords.y = pos.z;
    Scale = scale;
    Normal = normal;
    Level = level_instance;

    gl_Position =  PV * vec4(pos, 1.0);
}


#endif


#ifndef TERRAIN_INSTANCED_RENDERING

layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform vec2 texturePos;
uniform vec2 position;
uniform float scale;
uniform float texSize;
uniform int level;

uniform mat4 PV;
uniform sampler2DArray heightmapArray;

out vec3 WorldPos;
out vec2 TexCoords;
out vec3 Normal;

vec3 getNormal(vec2 texCoords){

    vec2 index0 = textureOffset(heightmapArray, vec3(texCoords.xy, level), ivec2(0, -1)).rg;
    vec2 index1 = textureOffset(heightmapArray, vec3(texCoords.xy, level), ivec2(-1, 0)).rg;
    vec2 index2 = textureOffset(heightmapArray, vec3(texCoords.xy, level), ivec2(1, 0)).rg;
    vec2 index3 = textureOffset(heightmapArray, vec3(texCoords.xy, level), ivec2(0, 1)).rg;

    float h0 = index0.r * 65.536f + index0.g * 0.256f;
    float h1 = index1.r * 65.536f + index1.g * 0.256f;
    float h2 = index2.r * 65.536f + index2.g * 0.256f;
    float h3 = index3.r * 65.536f + index3.g * 0.256f;

    vec3 normal;
	normal.z = h0 - h3;
	normal.x = h1 - h2;
	normal.y = 2;
	return normalize(normal);
}

float getHeightSetNormal(vec2 texCoords){

    vec2 tl = textureOffset(heightmapArray, vec3(texCoords.xy, level), ivec2(-1, -1)).rg;
    vec2 t  = textureOffset(heightmapArray, vec3(texCoords.xy, level), ivec2(0, -1)).rg;
    vec2 tr = textureOffset(heightmapArray, vec3(texCoords.xy, level), ivec2(1, -1)).rg;
    vec2 l  = textureOffset(heightmapArray, vec3(texCoords.xy, level), ivec2(-1, 0)).rg;
    vec2 m  = textureOffset(heightmapArray, vec3(texCoords.xy, level), ivec2(0, 0)).rg;
    vec2 r  = textureOffset(heightmapArray, vec3(texCoords.xy, level), ivec2(1, 0)).rg;
    vec2 bl = textureOffset(heightmapArray, vec3(texCoords.xy, level), ivec2(-1, 1)).rg;
    vec2 b  = textureOffset(heightmapArray, vec3(texCoords.xy, level), ivec2(0, 1)).rg;
    vec2 br = textureOffset(heightmapArray, vec3(texCoords.xy, level), ivec2(1, 1)).rg;

    float h_tl = (tl.r * 65.536f + tl.g * 0.256f) * 2;
    float h_t  = (t.r  * 65.536f + t.g  * 0.256f) * 2;
    float h_tr = (tr.r * 65.536f + tr.g * 0.256f) * 2;
    float h_l  = (l.r  * 65.536f + l.g  * 0.256f) * 2;
    float h_m  = (m.r  * 65.536f + m.g  * 0.256f) * 2;
    float h_r  = (r.r  * 65.536f + r.g  * 0.256f) * 2;
    float h_bl = (bl.r * 65.536f + bl.g * 0.256f) * 2;
    float h_b  = (b.r  * 65.536f + b.g  * 0.256f) * 2;
    float h_br = (br.r * 65.536f + br.g * 0.256f) * 2;

 //0
//    float dX = h_tr + 2 * h_r + h_br - h_tl - 2 * h_l - h_bl;   
//	float dY = h_bl + 2 * h_b + h_br - h_tl - 2 * h_t - h_tr;
//
//	float normalStrength = 1.f;
//	Normal = normalize(vec3(dX, 2.0 / normalStrength, dY));

// 1
    vec3 cp_0 = cross(vec3(0,h_t-h_m,-1), vec3(-1,h_tl-h_m,0));
    vec3 cp_1 = cross(vec3(-1,h_tl-h_m,-1), vec3(-1,h_l-h_m,0));
    vec3 cp_2 = cross(vec3(-1,h_l-h_m,0), vec3(0,h_bl-h_m,1));
    vec3 cp_3 = cross(vec3(-1,h_bl-h_m,0), vec3(0,h_b-h_m,1));
    vec3 cp_4 = cross(vec3(0,h_b-h_m,1), vec3(1,h_br-h_m,0));
    vec3 cp_5 = cross(vec3(0,h_br-h_m,1), vec3(1,h_r-h_m,0));
    vec3 cp_6 = cross(vec3(1,h_r-h_m,0), vec3(0,h_tr-h_m,1));
    vec3 cp_7 = cross(vec3(1,h_tr-h_m,0), vec3(0,h_t-h_m,1));

    Normal = normalize(cp_0 + cp_1 + cp_2 + cp_3 + cp_4 + cp_5 + cp_6 + cp_7);

    return h_m;
}

//vec3 getNormal(vec2 texCoords){
//
//    vec2 index0 = textureOffset(heightmapArray, texCoords.xy, ivec2(0, -1)).rg;
//    vec2 index1 = textureOffset(heightmapArray, texCoords.xy, ivec2(-1, 0)).rg;
//    vec2 index2 = textureOffset(heightmapArray, texCoords.xy, ivec2(1, 0)).rg;
//    vec2 index3 = textureOffset(heightmapArray, texCoords.xy, ivec2(0, 1)).rg;
//
//    float h0 = index0.r * 65.536f + index0.g * 0.256f;
//    float h1 = index1.r * 65.536f + index1.g * 0.256f;
//    float h2 = index2.r * 65.536f + index2.g * 0.256f;
//    float h3 = index3.r * 65.536f + index3.g * 0.256f;
//
//    vec3 normal;
//	normal.z = h0 - h3;
//	normal.x = h1 - h2;
//	normal.y = 2;
//	return normalize(normal);
//}

void main(void)
{
    vec3 p = vec3(model * vec4(aPos, 1));
    vec3 pos = vec3(position.x, 0, position.y) + scale * p;
    vec2 texCoords = vec2(pos.x - texturePos.x, pos.z - texturePos.y) / texSize + 0.5f;

    //Normal = getHeightSetNormal(texCoords);
    //Normal = vec3(0,1,0);
    //vec2 heightSample = texture(heightmapArray, vec3(texCoords.xy, level)).rg;
    //vec2 heightSample = texture(heightmap, texCoords).rg;

    //float height = heightSample.r * 65.536f + heightSample.g * 0.256f;
    float height = getHeightSetNormal(texCoords);
    pos.y = height;
    WorldPos = pos;
    TexCoords = texCoords;
    gl_Position =  PV * vec4(pos, 1.0);
}

#endif
