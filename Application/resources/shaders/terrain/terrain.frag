#version 460 core

in vec3 WorldPos;
in vec3 Normal;
in vec2 TexCoords;
in mat3 TBN;
in vec3 TangentViewPos;
in vec3 TangentFragPos;

out vec4 FragColor;

const float PI = 3.14159265359;

uniform vec3 camPos;

// IBL
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

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

// Landscape parameters
uniform vec3 lightDirection;
uniform float heightScale;
uniform float ambientAmount;
uniform float specularAmount;
uniform float specularPower;

uniform	float blendDistance0;
uniform	float blendAmount0;
uniform	float blendDistance1;
uniform	float blendAmount1;

uniform	float scale_color0_dist0;
uniform	float scale_color0_dist1;
uniform	float scale_color0_dist2;

uniform	float scale_color1_dist0;
uniform	float scale_color1_dist1;
uniform	float scale_color1_dist2;

uniform	float scale_color2_dist0;
uniform	float scale_color2_dist1;
uniform	float scale_color2_dist2;

uniform	float scale_color3_dist0;
uniform	float scale_color3_dist1;
uniform	float scale_color3_dist2;

uniform	float scale_color4_dist0;
uniform	float scale_color4_dist1;
uniform	float scale_color4_dist2;

uniform	float scale_color5_dist0;
uniform	float scale_color5_dist1;
uniform	float scale_color5_dist2;

uniform	float scale_color6_dist0;
uniform	float scale_color6_dist1;
uniform	float scale_color6_dist2;

uniform	float macroScale_0;
uniform	float macroScale_1;
uniform	float macroScale_2;
uniform	float macroAmount;
uniform	float macroPower;
uniform	float macroOpacity;

uniform	float overlayBlendScale0;
uniform	float overlayBlendAmount0;
uniform	float overlayBlendPower0;
uniform	float overlayBlendOpacity0;

uniform	float overlayBlendScale1;
uniform	float overlayBlendAmount1;
uniform	float overlayBlendPower1;
uniform	float overlayBlendOpacity1;

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

vec3 PhongMaterialWorkflow(vec3 albedo, vec3 normal, float specular){

    // ambient
    float ambientStrength = 0.1;
    float lightPower = 3;

    vec3 ambient = albedo * ambientStrength;
  	
    // diffuse 
    vec3 norm = TBN * normalize(normal);
    vec3 lightDir = normalize(-lightDirection);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * albedo * lightPower;

    // specular
    float specularStrength = specular * 0.5;
    vec3 viewDir = normalize(camPos - WorldPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specu = specularStrength * spec * vec3(1,1,1);  
            
    vec3 result = ambient + diffuse + specu;
    return result;
}

vec3 PbrMaterialWorkflowNoSpecular(vec3 albedo, vec3 normal, float specular){

    vec3 N = TBN * normal;
    float ambient = ambientAmount;

    vec3 lightDir = lightDirection;
    vec3 L = normalize(-lightDir);
    float lightPow = 5.f;
    vec3 radiance = vec3(lightPow);
            
    // scale light by NdotL
    float NdotL = max(dot(N, L), 0.0);        

    //vec3 specular = vec3(0);
    vec3 viewDir = normalize(camPos - WorldPos);
    vec3 reflectDir = reflect(-L, N);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), specularPower) * specular * specularAmount;

    // add to outgoing radiance Lo
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
    float distanceBlend0 = clamp((blendDistance0 - cameraToFragDist) / blendAmount0 + 0.5, 0, 1);

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
    vec3 albedo0 = mix(albedo_0_dist_1, albedo_0_dist_0, distanceBlend0) * macro;
    vec3 normal0 = mix(normal_0_dist_1, normal_0_dist_0, distanceBlend0);
    float spec0 = albedo0.r; 
    spec0 = clamp(spec0, 0, 0.5) * 0.5; 

    // GRASS ------------
    vec3 albedo_1_dist_0 = pow(texture(albedoT1, vec2(TexCoords * scale_color1_dist0)).rgb, vec3(2.2));
    vec3 normal_1_dist_0 = texture(normalT1,     vec2(TexCoords * scale_color1_dist0)).rgb * 2 - 1;
    vec3 albedo_1_dist_1 = pow(texture(albedoT1, vec2(TexCoords * scale_color1_dist1)).rgb, vec3(2.2));
    vec3 normal_1_dist_1 = texture(normalT1,     vec2(TexCoords * scale_color1_dist1)).rgb * 2 - 1;
    vec3 albedo1 = mix(albedo_1_dist_1, albedo_1_dist_0, distanceBlend0) * macro;
    vec3 normal1 = mix(normal_1_dist_1, normal_1_dist_0, distanceBlend0);
    float spec1 = albedo1.r; 
    spec1 = clamp(spec0, 0, 0.5) * 0.5; 

    // CLIFF ------------
    vec3 albedo_2_dist_0 = pow(texture(albedoT2, vec2(TexCoords * scale_color2_dist0)).rgb, vec3(2.2));
    vec3 normal_2_dist_0 = texture(normalT2,     vec2(TexCoords * scale_color2_dist0)).rgb * 2 - 1;
    vec3 albedo_2_dist_1 = pow(texture(albedoT2, vec2(TexCoords * scale_color2_dist1)).rgb, vec3(2.2));
    vec3 normal_2_dist_1 = texture(normalT2,     vec2(TexCoords * scale_color2_dist1)).rgb * 2 - 1;
    vec3 albedo2 = mix(albedo_2_dist_1, albedo_2_dist_0, distanceBlend0) * macro;
    vec3 normal2 = mix(normal_2_dist_1, normal_2_dist_0, distanceBlend0);
    float spec2 = albedo2.r; 
    spec2 = clamp(spec0, 0, 0.5) * 0.5; 

    // ROCKS ------------
    vec3 albedo_3_dist_0 = pow(texture(albedoT3, vec2(TexCoords * scale_color3_dist0)).rgb, vec3(2.2));
    vec3 normal_3_dist_0 = texture(normalT3,     vec2(TexCoords * scale_color3_dist0)).rgb * 2 - 1;
    vec3 albedo_3_dist_1 = pow(texture(albedoT3, vec2(TexCoords * scale_color3_dist1)).rgb, vec3(2.2));
    vec3 normal_3_dist_1 = texture(normalT3,     vec2(TexCoords * scale_color3_dist1)).rgb * 2 - 1;
    vec3 albedo3 = mix(albedo_3_dist_1, albedo_3_dist_0, distanceBlend0) * macro;
    vec3 normal3 = mix(normal_3_dist_1, normal_3_dist_0, distanceBlend0);
    float spec3 = albedo3.r; 
    spec3 = clamp(spec3, 0, 0.5) * 0.5; 

    // SNOW ------------
    vec3 albedo_4_dist_0 = pow(texture(albedoT4, vec2(TexCoords * scale_color4_dist0)).rgb, vec3(2.2));
    vec3 normal_4_dist_0 = texture(normalT4,     vec2(TexCoords * scale_color4_dist0)).rgb * 2 - 1;
    vec3 albedo_4_dist_1 = pow(texture(albedoT4, vec2(TexCoords * scale_color4_dist1)).rgb, vec3(2.2));
    vec3 normal_4_dist_1 = texture(normalT4,     vec2(TexCoords * scale_color4_dist1)).rgb * 2 - 1;
    vec3 albedo4 = mix(albedo_4_dist_1, albedo_4_dist_0, distanceBlend0);
    vec3 normal4 = mix(normal_4_dist_1, normal_4_dist_0, distanceBlend0);
    float spec4 = albedo4.r; 
    spec4 = clamp(spec4, 0, 0.5) * 0.5;

    // SAND ------------
    vec3 albedo_5_dist_0 = pow(texture(albedoT5, vec2(TexCoords * scale_color5_dist0)).rgb, vec3(2.2));
    vec3 normal_5_dist_0 = texture(normalT5,     vec2(TexCoords * scale_color5_dist0)).rgb * 2 - 1;
    vec3 albedo_5_dist_1 = pow(texture(albedoT5, vec2(TexCoords * scale_color5_dist1)).rgb, vec3(2.2));
    vec3 normal_5_dist_1 = texture(normalT5,     vec2(TexCoords * scale_color5_dist1)).rgb * 2 - 1;
    vec3 albedo5 = mix(albedo_5_dist_1, albedo_5_dist_0, distanceBlend0);
    vec3 normal5 = mix(normal_5_dist_1, normal_5_dist_0, distanceBlend0);
    float spec5 = albedo5.r; 
    spec5 = clamp(spec5, 0, 0.5) * 0.5;

    // CLIFF 1 ------------
    vec3 albedo_6_dist_0 = pow(texture(albedoT6, vec2(TexCoords * scale_color6_dist0)).rgb, vec3(2.2));
    vec3 normal_6_dist_0 = texture(normalT6,     vec2(TexCoords * scale_color6_dist0)).rgb * 2 - 1;
    vec3 albedo_6_dist_1 = pow(texture(albedoT6, vec2(TexCoords * scale_color6_dist1)).rgb, vec3(2.2));
    vec3 normal_6_dist_1 = texture(normalT6,     vec2(TexCoords * scale_color6_dist1)).rgb * 2 - 1;
    vec3 albedo6 = mix(albedo_6_dist_1, albedo_6_dist_0, distanceBlend0) * macro;
    vec3 normal6 = mix(normal_6_dist_1, normal_6_dist_0, distanceBlend0);
    float spec6 = albedo6.r; 
    spec6 = clamp(spec6, 0, 0.5) * 0.5;

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

    // ------- (GRASS UNCUT & GRASS DRIED) & SNOW (Interpolate)
    albedoVariation0 = mix(albedoVariation0, albedo4, worldSpaceHeightBlend1);
    normalVariation0 = mix(normalVariation0, normal4, worldSpaceHeightBlend1);
    specVariation0 = mix(specVariation0, spec4, worldSpaceHeightBlend1);


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

    float slope2 = mix(worldSpaceSlope, worldSpaceTextureSlope2, worldSpaceSlopeBlend1);
    float slopeBlend2 = clamp((slopeBias1 - slope2) / slopeSharpness1 + 0.5, 0, 1);
    float slope3 = mix(worldSpaceSlope, worldSpaceTextureSlope3, worldSpaceSlopeBlend0);
    float slopeBlend3 = clamp((slopeBias0 - slope3) / slopeSharpness0 + 0.5, 0, 1);


    // ---- soil,mulch,snow + rocks ----
    vec3 albedo = mix(albedoVariation0, albedo3, slopeBlend3);
    vec3 normal = mix(normalVariation0, normal3, slopeBlend3);
    float specular = mix(specVariation0, spec3, slopeBlend3);

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
    vec3 color = PbrMaterialWorkflowNoSpecular(albedo, normal, specular);

    // ---- FOG
    float depth = LinearizeDepth(gl_FragCoord.z);// / 10000;
    float fogBlend = clamp((depth - distanceNear) / fogBlendDistance + 0.5, 0, maxFog);
    color = mix(color, fogColor, fogBlend); 

    // ---- GAMMA CORRECT
    color = pow(color, vec3(1.0/2.2));

    // ---- OUTPUT
    FragColor = vec4(color, 1.f);
}
