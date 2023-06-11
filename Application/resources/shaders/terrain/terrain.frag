// Copyright (c) Abdullah Gulcur 2022-2023
// 
// This project is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

// Terrain Fragment Shader

// REFERENCES
// Procedural shader splatting. Reference : https://media.contentapi.ea.com/content/dam/eacom/frostbite/files/chapter5-andersson-terrain-rendering-in-frostbite.pdf

#version 460 core

struct Color
{
    vec3 albedo;
    vec3 normal;
    float specular;
    float emission; // it is just for how much light is absorbed. Sometimes material color is affected by light power too much. For example snow...
    float ao;
};

in vec3 WorldPos;
in vec3 Normal;
in vec2 TexCoords;
in mat3 TBN;
in vec3 debugColor;

out vec4 FragColor;

const float PI = 3.14159265359;

uniform vec3 camPos;

//uniform samplerCube irradianceMap;

// TODO: ADD LIGHT EMISSION

// Extra color
uniform sampler2D macroTexture;
uniform sampler2D noiseTexture;

uniform sampler2D albedoT0;
uniform sampler2D albedoT1;
uniform sampler2D albedoT2;
uniform sampler2D albedoT3;
uniform sampler2D albedoT4;
uniform sampler2D albedoT5;
uniform sampler2D albedoT6;

uniform sampler2D normalT0;
uniform sampler2D normalT1;
uniform sampler2D normalT2;
uniform sampler2D normalT3;
uniform sampler2D normalT4;
uniform sampler2D normalT5;
uniform sampler2D normalT6;
uniform sampler2D normalT7;
uniform sampler2D normalT8;

uniform sampler2D aoT0;

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

uniform	float scale_color7_dist0;
uniform	float scale_color7_dist1;

uniform	float scale_color8_dist0;
uniform	float scale_color8_dist1;

uniform	float macroScale_0;
uniform	float macroScale_1;
uniform	float macroScale_2;
uniform	float macroPower;
uniform	float macroOpacity;
uniform	float macroAmountLayer0;
uniform	float macroAmountLayer1;
uniform	float macroAmountLayer2;
uniform	float macroAmountLayer3;
uniform	float macroAmountLayer4;

uniform	float overlayBlendScale0;
uniform	float overlayBlendAmount0;
uniform	float overlayBlendPower0;
uniform	float overlayBlendOpacity0;

uniform	float overlayBlendScale1;
uniform	float overlayBlendAmount1;
uniform	float overlayBlendPower1;
uniform	float overlayBlendOpacity1;

uniform	float overlayBlendScale2;
uniform	float overlayBlendAmount2;
uniform	float overlayBlendPower2;
uniform	float overlayBlendOpacity2;

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

uniform float distanceNear;
uniform float fogBlendDistance;
uniform vec3 fogColor;
uniform float maxFog;

vec3 PbrMaterialWorkflow(vec3 albedo, vec3 normal, float specular, float ao){

    albedo = pow(albedo, vec3(2.2));
    normal = normal * 2 - 1;
    vec3 N = TBN * normal;

    vec3 lightDir = lightDirection;
    vec3 L = normalize(-lightDir);
    vec3 radiance = vec3(lightPow);
            
    float NdotL = max(dot(N, L), 0.0);        

    vec3 viewDir = normalize(camPos - WorldPos);
    vec3 reflectDir = reflect(-L, N);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), specularPower) * specular * specularAmount;

    vec3 Lo = (albedo / PI + spec) * radiance * NdotL;
    vec3 color = albedo * ambientAmount + Lo;
    return color * vec3(ao);
}

float LinearizeDepth(float depth) 
{
    float near = 0.1;
    float far = 10000;
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

float GetMacroValue(){

    float macro0 = texture(macroTexture, TexCoords * macroScale_0).r + 0.2;
    float macro1 = texture(macroTexture, TexCoords * macroScale_1).r + 0.2;
    float macro2 = texture(macroTexture, TexCoords * macroScale_2).r + 0.2;
    float macro = macro0 * macro1 * macro2;
    macro = 1 - macro;
    macro *= macroOpacity;
    macro = pow(macro,macroPower);
    macro = clamp(macro,0,1);
    return mix(1,10,macro);
}

float GetNoiseValue(float overlayBlendScale, float overlayBlendAmount, float overlayBlendPower, float overlayBlendOpacity){

    float noiseVal = texture(noiseTexture, TexCoords * overlayBlendScale).r;
    noiseVal += texture(noiseTexture, TexCoords * overlayBlendScale * 0.2).r;
    noiseVal += texture(noiseTexture, TexCoords * overlayBlendScale * 0.05).r;
    noiseVal * 0.33;

    noiseVal *= overlayBlendAmount;
    noiseVal = clamp(pow(noiseVal , overlayBlendPower), 0, 1) * overlayBlendOpacity;
    return noiseVal;
}

vec4 getColorDistanceBlendedRGBA(float dist0, float dist1, float distanceBlend, sampler2D textureUnit){

    vec4 textureUnit_dist_0 = texture(textureUnit, vec2(TexCoords * dist0)).rgba;
    vec4 textureUnit_dist_1 = texture(textureUnit, vec2(TexCoords * dist1)).rgba;
    return mix(textureUnit_dist_1, textureUnit_dist_0, distanceBlend);
}

vec3 getColorDistanceBlendedRGB(float dist0, float dist1, float distanceBlend, sampler2D textureUnit){

    vec3 textureUnit_dist_0 = texture(textureUnit, vec2(TexCoords * dist0)).rgb;
    vec3 textureUnit_dist_1 = texture(textureUnit, vec2(TexCoords * dist1)).rgb;
    return mix(textureUnit_dist_1, textureUnit_dist_0, distanceBlend);
}

/*
* Gets albedo, normal and specular color calculated with distance blending. 
* Same texture is blended according to distance with different scale values to get rid of tiling.
*/
Color getColorDistanceBlended(float distanceBlend, float dist0, float dist1, sampler2D albedoTexture, sampler2D normalTexture){
    
    vec4 albedo = getColorDistanceBlendedRGBA(dist0, dist1, distanceBlend, albedoTexture);
    vec3 normal = getColorDistanceBlendedRGB(dist0, dist1, distanceBlend, normalTexture);
    float specular = clamp(albedo.r, 0, 0.75) * 0.75;

    Color color;
    color.albedo = albedo.rgb;
    color.normal = normal;
    color.specular = specular;
    color.ao = albedo.a;

    return color;
}

Color blendColors(Color color0, Color color1, float alpha){

    vec3 albedo = mix(color0.albedo, color1.albedo, alpha);
    vec3 normal = mix(color0.normal, color1.normal, alpha);
    float specular = mix(color0.specular, color1.specular, alpha);
    float ao = mix(color0.ao, color1.ao, alpha);

    Color color;
    color.albedo = albedo;
    color.normal = normal;
    color.specular = specular;
    color.ao = ao;

    return color;
}

Color blendLayersWithNoise(Color color0, Color color1, float overlayBlendScale, float overlayBlendAmount, float overlayBlendPower, float overlayBlendOpacity){

    float noiseVal = GetNoiseValue(overlayBlendScale, overlayBlendAmount, overlayBlendPower, overlayBlendOpacity);
    return blendColors(color0, color1, noiseVal);
}

float getSlopeBlend(vec3 normal, float worldSpaceSlope, float slopeBias, float slopeSharpness){

    float worldSpaceSlopeBlend = clamp((slopeBias - worldSpaceSlope) / slopeSharpness + 0.5, 0, 1);
    vec3 tangentSpaceNormal = TBN * (normal * 2 - 1);
    float worldSpaceTextureSlope = dot(tangentSpaceNormal, vec3(0,1,0));
    float slope = mix(worldSpaceSlope, worldSpaceTextureSlope, worldSpaceSlopeBlend);
    float slopeBlend = clamp((slopeBias - slope) / slopeSharpness + 0.5, 0, 1);
    return slopeBlend;
}

float getDistanceBlend(){

    float cameraToFragDist = distance(camPos, WorldPos);
    float distanceBlend = clamp((blendDistance - cameraToFragDist) / blendAmount + 0.5, 0, 1);
    return distanceBlend;
}

vec3 getColorAfterFogFilter(vec3 color){

    float depth = LinearizeDepth(gl_FragCoord.z);// / 10000;
    float fogBlend = clamp((depth - distanceNear) / fogBlendDistance + 0.5, 0, maxFog);
    return mix(color, fogColor, fogBlend); 
}

void main(){

    float distanceBlend = getDistanceBlend();
    Color c0 = getColorDistanceBlended(distanceBlend, scale_color0_dist0, scale_color0_dist1, albedoT0, normalT0); // grasslawn
    Color c1 = getColorDistanceBlended(distanceBlend, scale_color1_dist0, scale_color1_dist1, albedoT1, normalT1); // grasswild
    Color c2 = getColorDistanceBlended(distanceBlend, scale_color2_dist0, scale_color2_dist1, albedoT2, normalT2); // soilmulch
    Color c3 = getColorDistanceBlended(distanceBlend, scale_color3_dist0, scale_color3_dist1, albedoT3, normalT3); // groundforest
    Color c4 = getColorDistanceBlended(distanceBlend, scale_color4_dist0, scale_color4_dist1, albedoT4, normalT4); // groundsandy
    Color c5 = getColorDistanceBlended(distanceBlend, scale_color5_dist0, scale_color5_dist1, albedoT5, normalT5); // cliffgranite
    Color c6 = getColorDistanceBlended(distanceBlend, scale_color6_dist0, scale_color6_dist1, albedoT6, normalT6); // lichenedrock

    Color c7;
    c7.albedo = color0;
    c7.specular = clamp(c7.albedo.r, 0, 0.5) * 0.5;
    c7.normal = getColorDistanceBlendedRGB(distanceBlend, scale_color7_dist0, scale_color7_dist1, normalT7);           // snowpure 
    c7.ao = 1.f;

    Color c8;
    c8.albedo = color1;
    c8.specular = clamp(c8.albedo.r, 0, 0.5) * 0.5;
    c8.normal = getColorDistanceBlendedRGB(distanceBlend, scale_color8_dist0, scale_color8_dist1, normalT8);           // snowfresh 
    c8.ao = 1.f;

    Color layer0 = blendLayersWithNoise(c0, c1, overlayBlendScale0, overlayBlendAmount0, overlayBlendPower0, overlayBlendOpacity0); // grass
    //Color layer0 = blendLayersWithNoise(c7, c8, overlayBlendScale3, overlayBlendAmount3, overlayBlendPower3, overlayBlendOpacity3); // snow

    Color layer1 = blendLayersWithNoise(c2, c3, overlayBlendScale1, overlayBlendAmount1, overlayBlendPower1, overlayBlendOpacity1); // mud
    Color layer2 = c4;                                                                                                              // stone
    Color layer3 = blendLayersWithNoise(c5, c6, overlayBlendScale2, overlayBlendAmount2, overlayBlendPower2, overlayBlendOpacity2); // cliff
    Color layer4 = blendLayersWithNoise(c7, c8, overlayBlendScale3, overlayBlendAmount3, overlayBlendPower3, overlayBlendOpacity3); // snow 

    float macro = GetMacroValue();
    layer0.albedo *= mix(1, macro, macroAmountLayer0);
    layer1.albedo *= mix(1, macro, macroAmountLayer1);
    layer2.albedo *= mix(1, macro, macroAmountLayer2);
    layer3.albedo *= mix(1, macro, macroAmountLayer3);
    layer4.albedo *= mix(1, macro, macroAmountLayer4);

    // SLOPES AND HEIGHTS
    float worldSpaceSlope = dot(Normal, vec3(0,1,0));
    float slopeBlend1 = getSlopeBlend(layer1.normal, worldSpaceSlope, slopeBias0, slopeSharpness0);
    float slopeBlend2 = getSlopeBlend(layer3.normal, worldSpaceSlope, slopeBias1, slopeSharpness1); // layer2: store, layer3: cliff
    float slopeBlend3 = getSlopeBlend(layer3.normal, worldSpaceSlope, slopeBias2, slopeSharpness2);
    float heightBlend0 = clamp((WorldPos.y - heightBias0) / heightSharpness0 + 0.5, 0, 1);

    Color final = blendColors(layer0, layer1, slopeBlend1);
    final = blendColors(final, layer4, heightBlend0);
    final = blendColors(final, layer2, slopeBlend2);
    final = blendColors(final, layer3, slopeBlend3);

    vec3 color = PbrMaterialWorkflow(final.albedo, final.normal, final.specular, final.ao);
    color = getColorAfterFogFilter(color);

    // ---- GAMMA CORRECT
    color = pow(color, vec3(1.0/2.2));

    // ---- OUTPUT
    FragColor = vec4(color, 1.f);
    //FragColor = vec4(debugColor, 1.f);
}
