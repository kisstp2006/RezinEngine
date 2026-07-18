#version 460 core

layout (location = 0) in vec3 aPosition;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // The light marker only needs the vertex position. Its normal data remains
    // in the shared vertex buffer but is intentionally not used here.
    gl_Position = projection * view * model * vec4(aPosition, 1.0);
}
