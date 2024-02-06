#version 330 core
out vec4 FragColor;

flat in vec4 LightingColor;

uniform vec3 objectColor;

void main()
{
    FragColor = LightingColor;
}  