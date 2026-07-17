#version 460 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D texture1; //0th uniform
uniform sampler2D texture2; //1th uniform

void main()
{
    FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2);
}
