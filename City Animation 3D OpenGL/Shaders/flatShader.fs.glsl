#version 330 core
out vec4 FragColor;

flat in vec3 TriangleColor;

void main()
{
    FragColor = vec4(TriangleColor, 1.0);
}  