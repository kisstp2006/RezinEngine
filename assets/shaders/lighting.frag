#version 460 core

#define NR_POINT_LIGHTS 4

struct Material
{
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct DirLight
{
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight
{
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight
{
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform Material material;
uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLight;
uniform vec3 viewPos;

layout (location = 0) out vec4 fragmentColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

// GLSL needs these prototypes because main() calls functions whose definitions
// appear later in this file.
vec3 calcDirLight(DirLight light, vec3 normal, vec3 viewDirection);
vec3 calcPointLight(
    PointLight light,
    vec3 normal,
    vec3 fragmentPosition,
    vec3 viewDirection
);
vec3 calcSpotLight(
    SpotLight light,
    vec3 normal,
    vec3 fragmentPosition,
    vec3 viewDirection
);

void main()
{
    const vec3 normal = normalize(Normal);
    const vec3 viewDirection = normalize(viewPos - FragPos);

    vec3 result = calcDirLight(dirLight, normal, viewDirection);

    for (int index = 0; index < NR_POINT_LIGHTS; ++index)
    {
        result += calcPointLight(
            pointLights[index],
            normal,
            FragPos,
            viewDirection
        );
    }

    result += calcSpotLight(
        spotLight,
        normal,
        FragPos,
        viewDirection
    );

    fragmentColor = vec4(result, 1.0);
}

vec3 calcDirLight(DirLight light, vec3 normal, vec3 viewDirection)
{
    const vec3 lightDirection = normalize(-light.direction);
    const float diffuseStrength = max(dot(normal, lightDirection), 0.0);

    const vec3 reflectionDirection = reflect(-lightDirection, normal);
    const float specularStrength = pow(
        max(dot(viewDirection, reflectionDirection), 0.0),
        material.shininess
    );

    const vec3 diffuseSample = vec3(texture(material.diffuse, TexCoords));
    const vec3 specularSample = vec3(texture(material.specular, TexCoords));

    const vec3 ambient = light.ambient * diffuseSample;
    const vec3 diffuse = light.diffuse * diffuseStrength * diffuseSample;
    const vec3 specular =
        light.specular * specularStrength * specularSample;

    return ambient + diffuse + specular;
}

vec3 calcPointLight(
    PointLight light,
    vec3 normal,
    vec3 fragmentPosition,
    vec3 viewDirection
)
{
    const vec3 lightDirection = normalize(light.position - fragmentPosition);
    const float diffuseStrength = max(dot(normal, lightDirection), 0.0);

    const vec3 reflectionDirection = reflect(-lightDirection, normal);
    const float specularStrength = pow(
        max(dot(viewDirection, reflectionDirection), 0.0),
        material.shininess
    );

    const float distanceToLight = length(light.position - fragmentPosition);
    const float attenuation = 1.0 / (
        light.constant
        + light.linear * distanceToLight
        + light.quadratic * distanceToLight * distanceToLight
    );

    const vec3 diffuseSample = vec3(texture(material.diffuse, TexCoords));
    const vec3 specularSample = vec3(texture(material.specular, TexCoords));

    vec3 ambient = light.ambient * diffuseSample;
    vec3 diffuse = light.diffuse * diffuseStrength * diffuseSample;
    vec3 specular = light.specular * specularStrength * specularSample;

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    return ambient + diffuse + specular;
}

vec3 calcSpotLight(
    SpotLight light,
    vec3 normal,
    vec3 fragmentPosition,
    vec3 viewDirection
)
{
    const vec3 lightDirection = normalize(light.position - fragmentPosition);
    const float diffuseStrength = max(dot(normal, lightDirection), 0.0);

    const vec3 reflectionDirection = reflect(-lightDirection, normal);
    const float specularStrength = pow(
        max(dot(viewDirection, reflectionDirection), 0.0),
        material.shininess
    );

    const float distanceToLight = length(light.position - fragmentPosition);
    const float attenuation = 1.0 / (
        light.constant
        + light.linear * distanceToLight
        + light.quadratic * distanceToLight * distanceToLight
    );

    const float theta = dot(lightDirection, normalize(-light.direction));
    const float epsilon = max(light.cutOff - light.outerCutOff, 0.0001);
    const float intensity = clamp(
        (theta - light.outerCutOff) / epsilon,
        0.0,
        1.0
    );

    const vec3 diffuseSample = vec3(texture(material.diffuse, TexCoords));
    const vec3 specularSample = vec3(texture(material.specular, TexCoords));

    vec3 ambient = light.ambient * diffuseSample;
    vec3 diffuse = light.diffuse * diffuseStrength * diffuseSample;
    vec3 specular = light.specular * specularStrength * specularSample;

    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;

    return ambient + diffuse + specular;
}
