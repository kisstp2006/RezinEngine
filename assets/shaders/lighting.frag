#version 460 core

layout (location = 0) out vec4 fragmentColor;

uniform vec3 objectColor;
uniform vec3 lightColor;

void main()
{
    // Ambient light is a small constant amount of the light source's color.
    // It keeps the object visible even before directional light is added.
    float ambientStrength = 0.001;
    vec3 ambient = ambientStrength * lightColor;

    vec3 result = ambient * objectColor;
    fragmentColor = vec4(result, 1.0);
}
