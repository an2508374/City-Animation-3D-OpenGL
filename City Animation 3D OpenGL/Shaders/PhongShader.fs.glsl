#version 330 core

out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;
in vec4 ViewCoordsPos;

uniform int isDay;
uniform vec3 viewPos;


struct Material {
    sampler2D texture_diffuse;
    sampler2D texture_specular;
    float     shininess;
}; 
uniform Material material;


struct DirLight {
    vec3 direction;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};  
uniform DirLight dirLight;


struct PointLight {    
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;  

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};  
uniform PointLight pointLight;


struct SpotLight {
    vec3  position;
    vec3  direction;

    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;  

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
#define SPOT_LIGHTS_COUNTER 3
uniform SpotLight spotLights[SPOT_LIGHTS_COUNTER];


struct FogParameters
{
	vec3 color;
	float linearStart;
	float linearEnd;
	float density;
	
	int equation;
	int isEnabled;
};
uniform FogParameters fogParams;


vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
float getFogFactor(FogParameters params, float fogCoordinate);


void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 result = vec3(0.0, 0.0, 0.0);

    // directional lighting
    if (isDay == 1)
        result += CalcDirLight(dirLight, norm, viewDir);

    // point lighting
    result += CalcPointLight(pointLight, norm, FragPos, viewDir);

    // spot lighting
    for(int i = 0; i < SPOT_LIGHTS_COUNTER; i++)
        result += CalcSpotLight(spotLights[i], norm, FragPos, viewDir);

    FragColor = vec4(result, 1.0);

    if (fogParams.isEnabled == 1)
    {
        float fogCoordinate = abs(ViewCoordsPos.z / ViewCoordsPos.w);
        FragColor = mix(FragColor, vec4(fogParams.color, 1.0), getFogFactor(fogParams, fogCoordinate));
    }
}


vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    vec3 reflectDir = reflect(-lightDir, normal);

    float diff = max(dot(normal, lightDir), 0.0);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    vec3 ambient  = light.ambient  * vec3(texture(material.texture_diffuse, TexCoords));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(material.texture_diffuse, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.texture_specular, TexCoords));

    return (ambient + diffuse + specular);
}


vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    vec3 reflectDir = reflect(-lightDir, normal);

    float diff = max(dot(normal, lightDir), 0.0);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    

    vec3 ambient  = light.ambient  * vec3(texture(material.texture_diffuse, TexCoords));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(material.texture_diffuse, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.texture_specular, TexCoords));

    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;

    return (ambient + diffuse + specular);
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);

    float theta     = dot(lightDir, normalize(-light.direction));
    float epsilon   = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    if (theta > light.cutOff)
    {
        vec3 reflectDir = reflect(-lightDir, normal);

        float diff = max(dot(normal, lightDir), 0.0);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

        float distance    = length(light.position - fragPos);
        float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    

        vec3 ambient  = light.ambient  * vec3(texture(material.texture_diffuse, TexCoords));
        vec3 diffuse  = light.diffuse  * diff * vec3(texture(material.texture_diffuse, TexCoords));
        vec3 specular = light.specular * spec * vec3(texture(material.texture_specular, TexCoords));

        ambient  *= attenuation;
        diffuse  *= attenuation;
        specular *= attenuation;

        diffuse  *= intensity;
        specular *= intensity;

        return (ambient + diffuse + specular);
    }
    else
    {
        return (light.ambient * vec3(texture(material.texture_diffuse, TexCoords)));
    }
}

// https://www.mbsoftworks.sk/tutorials/opengl4/020-fog/
float getFogFactor(FogParameters params, float fogCoordinate)
{
	float result = 0.0;
	if (params.equation == 0)
	{
		float fogLength = params.linearEnd - params.linearStart;
		result = (params.linearEnd - fogCoordinate) / fogLength;
	}
	else if (params.equation == 1)
    {
		result = exp(-params.density * fogCoordinate);
	}
	else if (params.equation == 2)
    {
		result = exp(-pow(params.density * fogCoordinate, 2.0));
	}
	
	result = 1.0 - clamp(result, 0.0, 1.0);
	return result;
}