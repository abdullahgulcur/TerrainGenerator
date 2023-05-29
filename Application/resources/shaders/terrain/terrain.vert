#version 460 core

#define MAX_HEIGHT 180.f
#define TERRAIN_INSTANCED_RENDERING

#ifdef TERRAIN_INSTANCED_RENDERING

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 position_instance;
layout (location = 3) in float level_instance;
layout (location = 4) in mat4 model_instance;
layout (location = 8) in vec3 color_instance;

uniform mat4 PV;

out vec3 WorldPos;
out vec3 Normal;
out vec2 TexCoords;
out mat3 TBN;
//out vec3 TangentViewPos;
//out vec3 TangentFragPos;
out float Scale;
out float Level;
out vec3 debugColor;

uniform sampler2DArray heightmapArray;
uniform float texSize;
uniform vec3 camPos;

void main(void)
{
    // set position in xz plane
    vec3 p = vec3(model_instance * vec4(aPos.x, 0, aPos.y, 1));
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

    // for normal mapping
    vec3 T = normalize(vec3(2 * scale, h2 - h1, 0));
    vec3 B = -normalize(cross(T,normal));
    TBN = mat3(T, B, normal);

    // for parallax mapping
    vec3 T_ = normalize(vec3(2 * scale, h2 - h1, 0));
    vec3 B_ = normalize(cross(T_,normal));
    mat3 TBN_ = transpose(mat3(T_, B_, normal));

    WorldPos = pos;
    //TangentViewPos = TBN_* camPos;
    //TangentFragPos = TBN_* pos;
    TexCoords.x = pos.x;
    TexCoords.y = pos.z;
    Scale = scale;
    Normal = normal;
    Level = level_instance;
    debugColor = color_instance;

    gl_Position =  PV * vec4(pos, 1.0);
}

#endif