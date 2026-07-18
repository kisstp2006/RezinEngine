#version 460 core

layout (location = 0) out vec4 fragmentColor;

in vec3 Normal;
in vec3 FragPos;

void main()
{
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    // The visible light marker stays bright and is not lit by itself.
    fragmentColor = vec4(1.0);
}
