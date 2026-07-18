#version 460 core

layout (location = 0) out vec4 fragmentColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;

void main()
{
    // Ambient light is a small constant amount of the light source's color.
    // It keeps surfaces visible even when they do not face the light source.
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse light becomes stronger as the surface turns toward the light.
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 result = (ambient + diffuse) * objectColor;
    fragmentColor = vec4(result, 1.0);
}
