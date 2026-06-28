#version 430 core

out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D uLowResImage;

void main()
{
    FragColor = texture(uLowResImage, TexCoords);
}