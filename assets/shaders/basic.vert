#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;


void main()
{
    TexCoord = aTexCoord;
    // Convert a local-space vertex to clip space. Matrix transformations are
    // applied from right to left: model first, then view, then projection.
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
