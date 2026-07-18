#version 460 core

layout (location = 0) out vec4 fragmentColor;

void main()
{
    // The visible light marker stays bright and is not lit by itself.
    fragmentColor = vec4(1.0);
}
