// FXAA Implementation
// ref: https://github.com/mitsuhiko/webgl-meincraft

#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform vec2 resolution;

void main()
{
    float FXAA_REDUCE_MIN = (1.0/ 128.0);
	float FXAA_REDUCE_MUL = (1.0 / 8.0);
	float FXAA_SPAN_MAX =  8.0;
	vec2 inverseVP = vec2(1.0 / resolution.x, 1.0 / resolution.y);
	vec3 rgbNW = textureOffset(screenTexture, TexCoords, ivec2(-1, -1)).xyz;
	vec3 rgbNE = textureOffset(screenTexture, TexCoords, ivec2(1, -1)).xyz;
	vec3 rgbSW = textureOffset(screenTexture, TexCoords, ivec2(-1, 1)).xyz;
	vec3 rgbSE = textureOffset(screenTexture, TexCoords, ivec2(1, 1)).xyz;
	vec3 rgbM  = texture(screenTexture, TexCoords).xyz;
	vec3 luma  = vec3(0.299, 0.587, 0.114);
	float lumaNW = dot(rgbNW, luma);
	float lumaNE = dot(rgbNE, luma);
	float lumaSW = dot(rgbSW, luma);
	float lumaSE = dot(rgbSE, luma);
	float lumaM  = dot(rgbM,  luma);
	float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
	float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
	
	vec2 dir;
	dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
	dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));
	float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) *
		(0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);
	float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
	dir = min(vec2(FXAA_SPAN_MAX, FXAA_SPAN_MAX), max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX), dir * rcpDirMin)) * inverseVP;
	
	vec3 rgbA = (1.0/2.0) * (texture(screenTexture,TexCoords + dir * (1.0/3.0 - 0.5)).xyz + texture(screenTexture,TexCoords + dir * (2.0/3.0 - 0.5)).xyz);
	vec3 rgbB = rgbA * (1.0/2.0) + (1.0/4.0) * (texture(screenTexture,TexCoords + dir * (0.0/3.0 - 0.5)).xyz + texture(screenTexture,TexCoords + dir * (3.0/3.0 - 0.5)).xyz);
	
	float lumaB = dot(rgbB, luma);
	if ((lumaB < lumaMin) || (lumaB > lumaMax)){
		FragColor.rgb = rgbA;
	}else{
		FragColor.rgb = rgbB;
	}
} 