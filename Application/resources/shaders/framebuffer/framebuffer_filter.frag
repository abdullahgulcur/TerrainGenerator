#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;

void main()
{
    vec3 col = texture(screenTexture, TexCoords).rgb;
    col = vec3(col.r+col.g+col.b) * 0.33f;
    FragColor = vec4(col, 1.0);
} 