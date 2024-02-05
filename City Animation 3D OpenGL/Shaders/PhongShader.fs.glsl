#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

struct Material {
    sampler2D texture_diffuse;
    sampler2D texture_specular;
    float     shininess;
}; 

struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform vec3 viewPos;
uniform Material material;
uniform Light light;

void main()
{
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    vec3 ambient = light.ambient * vec3(texture(material.texture_diffuse, TexCoords));

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse, TexCoords));

    //float spec = pow(max(dot(viewDir, reflectDir), 0.0001), material.shininess);
    float spec = pow(0.001, material.shininess);
    vec3 specular = light.specular * spec * vec3(texture(material.texture_specular, TexCoords));

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}  