// Copyright (c) Abdullah Gulcur 2022-2023
// 
// This project is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

// Terrain Fragment Shader

// REFERENCES
// Procedural shader splatting. Reference : https://media.contentapi.ea.com/content/dam/eacom/frostbite/files/chapter5-andersson-terrain-rendering-in-frostbite.pdf

#version 460 core

in vec3 WorldPos;
in vec3 Normal;
in vec2 TexCoords;
in mat3 TBN;
in vec3 debugColor;

out vec4 FragColor;

const float PI = 3.14159265359;

uniform vec3 camPos;

uniform samplerCube irradianceMap;

// TODO: ADD LIGHT EMISSION

// Extra color
uniform sampler2D macroTexture;
uniform sampler2D noiseTexture;

uniform sampler2D albedoT0;
uniform sampler2D albedoT1;
uniform sampler2D albedoT2;
uniform sampler2D albedoT3;
uniform sampler2D albedoT5;
uniform sampler2D albedoT6;
uniform sampler2D albedoT7;
uniform sampler2D albedoT8;

uniform sampler2D normalT0;
uniform sampler2D normalT1;
uniform sampler2D normalT2;
uniform sampler2D normalT3;
uniform sampler2D normalT4;
uniform sampler2D normalT5;
uniform sampler2D normalT6;
uniform sampler2D normalT7;
uniform sampler2D normalT8;
uniform sampler2D normalT9;

// Snow Color
uniform vec3 color0;
uniform vec3 color1;

// Landscape parameters
uniform vec3 lightDirection;
uniform float lightPow;
uniform float ambientAmount;
uniform float specularAmount;
uniform float specularPower;

uniform	float blendDistance;
uniform	float blendAmount;

uniform	float scale_color0_dist0;
uniform	float scale_color0_dist1;

uniform	float scale_color1_dist0;
uniform	float scale_color1_dist1;

uniform	float scale_color2_dist0;
uniform	float scale_color2_dist1;

uniform	float scale_color3_dist0;
uniform	float scale_color3_dist1;

uniform	float scale_color4_dist0;
uniform	float scale_color4_dist1;

uniform	float scale_color5_dist0;
uniform	float scale_color5_dist1;

uniform	float scale_color6_dist0;
uniform	float scale_color6_dist1;

// Snow
uniform	float scale_color7_dist0;
uniform	float scale_color7_dist1;

uniform	float scale_color8_dist0;
uniform	float scale_color8_dist1;

uniform	float scale_color9_dist0;
uniform	float scale_color9_dist1;

uniform	float macroScale_0;
uniform	float macroScale_1;
uniform	float macroScale_2;
uniform	float macroAmount;
uniform	float macroPower;
uniform	float macroOpacity;

// MUD LAYER
uniform	float overlayBlendScale0;
uniform	float overlayBlendAmount0;
uniform	float overlayBlendPower0;
uniform	float overlayBlendOpacity0;

// ROCK LAYER
uniform	float overlayBlendScale1;
uniform	float overlayBlendAmount1;
uniform	float overlayBlendPower1;
uniform	float overlayBlendOpacity1;

// SNOW LAYER
uniform	float overlayBlendScale2;
uniform	float overlayBlendAmount2;
uniform	float overlayBlendPower2;
uniform	float overlayBlendOpacity2;

// SNOW LAYER
uniform	float overlayBlendScale3;
uniform	float overlayBlendAmount3;
uniform	float overlayBlendPower3;
uniform	float overlayBlendOpacity3;

uniform	float slopeSharpness0;
uniform	float slopeSharpness1;
uniform	float slopeSharpness2;

uniform	float slopeBias0;
uniform	float slopeBias1;
uniform	float slopeBias2;

uniform	float heightBias0;
uniform	float heightSharpness0;
uniform	float heightBias1;
uniform	float heightSharpness1;

uniform float distanceNear;
uniform float fogBlendDistance;
uniform vec3 fogColor;
uniform float maxFog;

vec3 PbrMaterialWorkflow(vec3 albedo, vec3 normal, float specular){

    vec3 N = TBN * normal;
    float ambient = ambientAmount;

    vec3 lightDir = lightDirection;
    vec3 L = normalize(-lightDir);
    vec3 radiance = vec3(lightPow);
            
    float NdotL = max(dot(N, L), 0.0);        

    vec3 viewDir = normalize(camPos - WorldPos);
    vec3 reflectDir = reflect(-L, N);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), specularPower) * specular * specularAmount;

    vec3 Lo = (albedo / PI + spec) * radiance * NdotL;
   
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse = irradiance * albedo;
    
    vec3 color = diffuse * ambient + Lo;

    return color;
}

float LinearizeDepth(float depth) 
{
    float near = 0.1;
    float far = 10000;
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

float GetMacroValue(){

    float macroOffset = 0.0f;
    float macro0 = texture(macroTexture, TexCoords * macroScale_0).r + macroOffset;
    float macro1 = texture(macroTexture, TexCoords * macroScale_1).r + macroOffset;
    float macro2 = texture(macroTexture, TexCoords * macroScale_2).r + macroOffset;
    float macro = macro0 * macro1 * macro2;
    //macro = 1 - macro;
    macro *= macroAmount;
    macro = pow(macro,macroPower);
    macro = clamp(macro,0,1);
    macro = mix(macro, 1, macroOpacity);
    return macro;
}

void main(){

    // COMMON
    float macro = GetMacroValue();

    float cameraToFragDist = distance(camPos, WorldPos);
    float distanceBlend = clamp((blendDistance - cameraToFragDist) / blendAmount + 0.5, 0, 1);

    float worldSpaceSlope = dot(Normal, vec3(0,1,0));
    float worldSpaceSlopeBlend0 = clamp((slopeBias0 - worldSpaceSlope) / slopeSharpness0 + 0.5, 0, 1);
    float worldSpaceSlopeBlend1 = clamp((slopeBias1 - worldSpaceSlope) / slopeSharpness1 + 0.5, 0, 1);
    float worldSpaceSlopeBlend2 = clamp((slopeBias2 - worldSpaceSlope) / slopeSharpness2 + 0.5, 0, 1);

    float worldSpaceHeight = WorldPos.y;
    float worldSpaceHeightBlend0 = clamp((worldSpaceHeight - heightBias0) / heightSharpness0 + 0.5, 0, 1);
    float worldSpaceHeightBlend1 = clamp((worldSpaceHeight - heightBias1) / heightSharpness1 + 0.5, 0, 1);

    // SOIL ------------
    vec3 albedo_0_dist_0 = pow(texture(albedoT0, vec2(TexCoords * scale_color0_dist0)).rgb, vec3(2.2));
    vec3 normal_0_dist_0 = texture(normalT0,     vec2(TexCoords * scale_color0_dist0)).rgb * 2 - 1;
    vec3 albedo_0_dist_1 = pow(texture(albedoT0, vec2(TexCoords * scale_color0_dist1)).rgb, vec3(2.2));
    vec3 normal_0_dist_1 = texture(normalT0,     vec2(TexCoords * scale_color0_dist1)).rgb * 2 - 1;
    vec3 albedo0 = mix(albedo_0_dist_1, albedo_0_dist_0, distanceBlend) * macro;
    vec3 normal0 = mix(normal_0_dist_1, normal_0_dist_0, distanceBlend);
    float spec0 = albedo0.r; 
    spec0 = clamp(spec0, 0, 0.5) * 0.5; 

    // GRASS ------------
    vec3 albedo_1_dist_0 = pow(texture(albedoT1, vec2(TexCoords * scale_color1_dist0)).rgb, vec3(2.2));
    vec3 normal_1_dist_0 = texture(normalT1,     vec2(TexCoords * scale_color1_dist0)).rgb * 2 - 1;
    vec3 albedo_1_dist_1 = pow(texture(albedoT1, vec2(TexCoords * scale_color1_dist1)).rgb, vec3(2.2));
    vec3 normal_1_dist_1 = texture(normalT1,     vec2(TexCoords * scale_color1_dist1)).rgb * 2 - 1;
    vec3 albedo1 = mix(albedo_1_dist_1, albedo_1_dist_0, distanceBlend) * macro;
    vec3 normal1 = mix(normal_1_dist_1, normal_1_dist_0, distanceBlend);
    float spec1 = albedo1.r; 
    spec1 = clamp(spec0, 0, 0.5) * 0.5; 

    // CLIFF ------------
    vec3 albedo_2_dist_0 = pow(texture(albedoT2, vec2(TexCoords * scale_color2_dist0)).rgb, vec3(2.2));
    vec3 normal_2_dist_0 = texture(normalT2,     vec2(TexCoords * scale_color2_dist0)).rgb * 2 - 1;
    vec3 albedo_2_dist_1 = pow(texture(albedoT2, vec2(TexCoords * scale_color2_dist1)).rgb, vec3(2.2));
    vec3 normal_2_dist_1 = texture(normalT2,     vec2(TexCoords * scale_color2_dist1)).rgb * 2 - 1;
    vec3 albedo2 = mix(albedo_2_dist_1, albedo_2_dist_0, distanceBlend) * macro;
    vec3 normal2 = mix(normal_2_dist_1, normal_2_dist_0, distanceBlend);
    float spec2 = albedo2.r; 
    spec2 = clamp(spec0, 0, 0.5) * 0.5; 

    // ROCKS ------------
    vec3 albedo_3_dist_0 = pow(texture(albedoT3, vec2(TexCoords * scale_color3_dist0)).rgb, vec3(2.2));
    vec3 normal_3_dist_0 = texture(normalT3,     vec2(TexCoords * scale_color3_dist0)).rgb * 2 - 1;
    vec3 albedo_3_dist_1 = pow(texture(albedoT3, vec2(TexCoords * scale_color3_dist1)).rgb, vec3(2.2));
    vec3 normal_3_dist_1 = texture(normalT3,     vec2(TexCoords * scale_color3_dist1)).rgb * 2 - 1;
    vec3 albedo3 = mix(albedo_3_dist_1, albedo_3_dist_0, distanceBlend) * macro;
    vec3 normal3 = mix(normal_3_dist_1, normal_3_dist_0, distanceBlend);
    float spec3 = albedo3.r; 
    spec3 = clamp(spec3, 0, 0.5) * 0.5; 

    // SNOW 0 ------------
    vec3 normal_4_dist_0 = texture(normalT4,     vec2(TexCoords * scale_color4_dist0)).rgb * 2 - 1;
    vec3 normal_4_dist_1 = texture(normalT4,     vec2(TexCoords * scale_color4_dist1)).rgb * 2 - 1;
    vec3 albedo4 = color0;
    vec3 normal4 = mix(normal_4_dist_1, normal_4_dist_0, distanceBlend);
    float spec4 = albedo4.r; 
    spec4 = clamp(spec4, 0, 0.5) * 0.5;

    // SNOW 1 ------------
    vec3 normal_7_dist_0 = texture(normalT7,     vec2(TexCoords * scale_color7_dist0)).rgb * 2 - 1;
    vec3 normal_7_dist_1 = texture(normalT7,     vec2(TexCoords * scale_color7_dist1)).rgb * 2 - 1;
    vec3 albedo7 = color1;
    vec3 normal7 = mix(normal_7_dist_1, normal_7_dist_0, distanceBlend);
    float spec7 = albedo7.r; 
    spec7 = clamp(spec7, 0, 0.5) * 0.5;

    // SAND ------------
    vec3 albedo_5_dist_0 = pow(texture(albedoT5, vec2(TexCoords * scale_color5_dist0)).rgb, vec3(2.2));
    vec3 normal_5_dist_0 = texture(normalT5,     vec2(TexCoords * scale_color5_dist0)).rgb * 2 - 1;
    vec3 albedo_5_dist_1 = pow(texture(albedoT5, vec2(TexCoords * scale_color5_dist1)).rgb, vec3(2.2));
    vec3 normal_5_dist_1 = texture(normalT5,     vec2(TexCoords * scale_color5_dist1)).rgb * 2 - 1;
    vec3 albedo5 = mix(albedo_5_dist_1, albedo_5_dist_0, distanceBlend);
    vec3 normal5 = mix(normal_5_dist_1, normal_5_dist_0, distanceBlend);
    float spec5 = albedo5.r; 
    spec5 = clamp(spec5, 0, 0.5) * 0.5;

    // CLIFF 1 ------------
    vec3 albedo_6_dist_0 = pow(texture(albedoT6, vec2(TexCoords * scale_color6_dist0)).rgb, vec3(2.2));
    vec3 normal_6_dist_0 = texture(normalT6,     vec2(TexCoords * scale_color6_dist0)).rgb * 2 - 1;
    vec3 albedo_6_dist_1 = pow(texture(albedoT6, vec2(TexCoords * scale_color6_dist1)).rgb, vec3(2.2));
    vec3 normal_6_dist_1 = texture(normalT6,     vec2(TexCoords * scale_color6_dist1)).rgb * 2 - 1;
    vec3 albedo6 = mix(albedo_6_dist_1, albedo_6_dist_0, distanceBlend) * macro;
    vec3 normal6 = mix(normal_6_dist_1, normal_6_dist_0, distanceBlend);
    float spec6 = albedo6.r; 
    spec6 = clamp(spec6, 0, 0.5) * 0.5;

    // GRASS LAWN ------------
    vec3 albedo_8_dist_0 = pow(texture(albedoT7, vec2(TexCoords * scale_color8_dist0)).rgb, vec3(2.2));
    vec3 normal_8_dist_0 = texture(normalT8,     vec2(TexCoords * scale_color8_dist0)).rgb * 2 - 1;
    vec3 albedo_8_dist_1 = pow(texture(albedoT7, vec2(TexCoords * scale_color8_dist1)).rgb, vec3(2.2));
    vec3 normal_8_dist_1 = texture(normalT8,     vec2(TexCoords * scale_color8_dist1)).rgb * 2 - 1;
    vec3 albedo8 = mix(albedo_8_dist_1, albedo_8_dist_0, distanceBlend) * macro;
    vec3 normal8 = mix(normal_8_dist_1, normal_8_dist_0, distanceBlend);
    float spec8 = albedo8.r; 
    spec8 = clamp(spec8, 0, 0.5) * 0.5;

    // GRASS WILD ------------
    vec3 albedo_9_dist_0 = pow(texture(albedoT8, vec2(TexCoords * scale_color9_dist0)).rgb, vec3(2.2));
    vec3 normal_9_dist_0 = texture(normalT9,     vec2(TexCoords * scale_color9_dist0)).rgb * 2 - 1;
    vec3 albedo_9_dist_1 = pow(texture(albedoT8, vec2(TexCoords * scale_color9_dist1)).rgb, vec3(2.2));
    vec3 normal_9_dist_1 = texture(normalT9,     vec2(TexCoords * scale_color9_dist1)).rgb * 2 - 1;
    vec3 albedo9 = mix(albedo_9_dist_1, albedo_9_dist_0, distanceBlend) * macro;
    vec3 normal9 = mix(normal_9_dist_1, normal_9_dist_0, distanceBlend);
    float spec9 = albedo9.r; 
    spec9 = clamp(spec9, 0, 0.5) * 0.5;

    // ------- GRASS UNCUT & GRASS DRIED (Interpolate)
    float noiseVal = texture(noiseTexture, TexCoords * overlayBlendScale0).r;
    noiseVal += texture(noiseTexture, TexCoords * overlayBlendScale0 * 0.2).r;
    noiseVal += texture(noiseTexture, TexCoords * overlayBlendScale0 * 0.05).r;
    noiseVal * 0.33;

    noiseVal *= overlayBlendAmount0;
    noiseVal = clamp(pow(noiseVal , overlayBlendPower0), 0, 1) * overlayBlendOpacity0;

    vec3 albedoVariation0 = mix(albedo0, albedo1, noiseVal);
    vec3 normalVariation0 = mix(normal0, normal1, noiseVal);
    float specVariation0 = mix(spec0, spec1, noiseVal);

    // ------- GRASS LAWN & GRASS WILD (Interpolate)

    noiseVal = texture(noiseTexture, TexCoords * overlayBlendScale3).r;
    noiseVal += texture(noiseTexture, TexCoords * overlayBlendScale3 * 0.2).r;
    noiseVal += texture(noiseTexture, TexCoords * overlayBlendScale3 * 0.05).r;
    noiseVal * 0.33;
    noiseVal *= overlayBlendAmount3;
    noiseVal = clamp(pow(noiseVal , overlayBlendPower3), 0, 1) * overlayBlendOpacity3;

    vec3 albedoVariation3 = mix(albedo8, albedo9, noiseVal);
    vec3 normalVariation3 = mix(normal8, normal9, noiseVal);
    float specVariation3 = mix(spec8, spec9, noiseVal);

    // ------- SNOW 0 & SNOW 1 (Interpolate)

    noiseVal = texture(noiseTexture, TexCoords * overlayBlendScale2).r;
    noiseVal += texture(noiseTexture, TexCoords * overlayBlendScale2 * 0.2).r;
    noiseVal += texture(noiseTexture, TexCoords * overlayBlendScale2 * 0.05).r;
    noiseVal * 0.33;
    noiseVal *= overlayBlendAmount2;
    noiseVal = clamp(pow(noiseVal , overlayBlendPower2), 0, 1) * overlayBlendOpacity2;

    vec3 albedoVariation2 = mix(albedo4, albedo7, noiseVal);
    vec3 normalVariation2 = mix(normal4, normal7, noiseVal);
    float specVariation2 = mix(spec4, spec7, noiseVal);

    // ------- GRASS & SNOW 
    albedoVariation0 = mix(albedoVariation0, albedoVariation2, worldSpaceHeightBlend1);
    normalVariation0 = mix(normalVariation0, normalVariation2, worldSpaceHeightBlend1);
    specVariation0 = mix(specVariation0, specVariation2, worldSpaceHeightBlend1);
    // sogurma miktarlarini da mixle, ona gore light calculationlarinin oldugu yere input ver.

    // ------- CLIFF & GRANITE (Interpolate)
    noiseVal = texture(noiseTexture, TexCoords * overlayBlendScale1).r;
    noiseVal += texture(noiseTexture, TexCoords * overlayBlendScale1 * 0.2).r;
    noiseVal += texture(noiseTexture, TexCoords * overlayBlendScale1 * 0.05).r;
    noiseVal * 0.33;
    noiseVal *= overlayBlendAmount1;
    noiseVal = clamp(pow(noiseVal , overlayBlendPower1), 0, 1) * overlayBlendOpacity1;


    vec3 albedoVariation1 = mix(albedo2, albedo6, noiseVal);
    vec3 normalVariation1 = mix(normal2, normal6, noiseVal);
    float specVariation1 = mix(spec2, spec6, noiseVal);

    // SLOPES
    vec3 TextureNormal2 = TBN * normal2;
    float worldSpaceTextureSlope2 = dot(TextureNormal2, vec3(0,1,0));
    vec3 TextureNormal3 = TBN * normal3;
    float worldSpaceTextureSlope3 = dot(TextureNormal3, vec3(0,1,0));
    vec3 TextureNormal0 = TBN * normal0;
    float worldSpaceTextureSlope0 = dot(TextureNormal0, vec3(0,1,0));
    vec3 TextureNormal8 = TBN * normal8;
    float worldSpaceTextureSlope1 = dot(TextureNormal8, vec3(0,1,0));

    float slope2 = mix(worldSpaceSlope, worldSpaceTextureSlope2, worldSpaceSlopeBlend1);
    float slopeBlend2 = clamp((slopeBias1 - slope2) / slopeSharpness1 + 0.5, 0, 1);
    float slope3 = mix(worldSpaceSlope, worldSpaceTextureSlope3, worldSpaceSlopeBlend0);
    float slopeBlend3 = clamp((slopeBias0 - slope3) / slopeSharpness0 + 0.5, 0, 1);
    float slope4 = mix(worldSpaceSlope, worldSpaceTextureSlope1, worldSpaceSlopeBlend2);
    float slopeBlend4 = clamp((slopeBias2 - slope4) / slopeSharpness2 + 0.5, 0, 1);


    // ---- soil,mulch,snow + rocks ----
    vec3 albedo = mix(albedoVariation0, albedo3, slopeBlend3);
    vec3 normal = mix(normalVariation0, normal3, slopeBlend3);
    float specular = mix(specVariation0, spec3, slopeBlend3);

    albedo = mix(albedoVariation3, albedo, slopeBlend4);
    normal = mix(normalVariation3, normal, slopeBlend4);
    specular = mix(specVariation3, specular, slopeBlend4);

    // ---- soil,mulch,snow,rocks + cliff ----
    albedo = mix(albedo, albedoVariation1, slopeBlend2);
    normal = mix(normal, normalVariation1, slopeBlend2);
    specular = mix(specular, specVariation1, slopeBlend2);

    // ---- ADD SAND
    albedo = mix(albedo5, albedo, worldSpaceHeightBlend0);
    //albedo = mix(albedo, vec3(1,1,1), macro);
    normal = mix(normal5, normal, worldSpaceHeightBlend0);
    specular = mix(spec5, specular, worldSpaceHeightBlend0);

    // ---- EDITED PBR
    vec3 color = PbrMaterialWorkflow(albedo, normal, specular);

    // ---- FOG
    float depth = LinearizeDepth(gl_FragCoord.z);// / 10000;
    float fogBlend = clamp((depth - distanceNear) / fogBlendDistance + 0.5, 0, maxFog);
    color = mix(color, fogColor, fogBlend); 

    // ---- GAMMA CORRECT
    color = pow(color, vec3(1.0/2.2));

    // ---- OUTPUT
    FragColor = vec4(color, 1.f);
    //FragColor = vec4(debugColor, 1.f);

}
