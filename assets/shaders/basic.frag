#version 460 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D texture1; // Samples the texture assigned to texture unit 0.
uniform sampler2D texture2; // Samples the texture assigned to texture unit 1.

void main()
{
    float ambientStrength = 0.1;
    // Keep 80% of texture1 and blend in 20% of texture2.
    FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2);
}
