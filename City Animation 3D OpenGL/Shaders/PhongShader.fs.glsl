#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

uniform vec3 mAmbient;
uniform vec3 mDiffuse;
uniform vec3 mSpecular;
uniform float mShininess;

void main()
{
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    vec3 ambient = mAmbient * lightColor;

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = mDiffuse * diff * lightColor;

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), mShininess);
    vec3 specular = mSpecular * spec * lightColor;

    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(result, 1.0);
}  