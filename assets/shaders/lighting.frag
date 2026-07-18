#version 460 core

struct Material
{
    sampler2D diffuse;
    sampler2D specular;

    float shininess;
};

uniform Material material;


struct Light
{
    vec3 position;

    // Each Phong component can have its own light color and intensity.
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform Light light;


layout (location = 0) out vec4 fragmentColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 viewPos;

void main()
{
    vec3 ambient  = light.ambient  * vec3(texture(material.diffuse, TexCoords));

    // Diffuse light becomes stronger as the surface turns toward the light.
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(material.diffuse, TexCoords));

    // Specular light produces a highlight where the reflected light direction
    // points toward the camera.
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));

    vec3 result = ambient + diffuse + specular;
    fragmentColor = vec4(ambient + diffuse + specular, 1.0);
}
