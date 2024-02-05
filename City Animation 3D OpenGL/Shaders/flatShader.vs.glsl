#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

flat out vec3 TriangleColor;

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

uniform vec3 materialAmbient;
uniform vec3 materialDiffuse;
uniform vec3 materialSpecular;
uniform float materialShininess;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);

    vec3 Position = vec3(model * vec4(aPos, 1.0));
    vec3 Normal = mat3(transpose(inverse(model))) * aNormal;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - Position);
    vec3 viewDir = normalize(viewPos - Position);
    vec3 reflectDir = reflect(-lightDir, norm);

    vec3 ambient = materialAmbient * lightColor;

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = materialDiffuse * diff * lightColor;

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), materialShininess);
    vec3 specular = materialSpecular * spec * lightColor;

    TriangleColor = (ambient + diffuse + specular) * objectColor;
}