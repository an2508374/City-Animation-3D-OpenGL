#version 330 core
out vec4 FragColor;

in vec4 LightingColor;

uniform vec3 objectColor;

void main()
{
    FragColor = LightingColor;
}  