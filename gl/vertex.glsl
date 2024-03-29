R"HERE(
#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

uniform mat4 projection, view;

out vec3 Normal;
out vec3 FragPos;

void
main ()
{
    gl_Position = projection * view * vec4(position, 1.0f);
    Normal = normal;
    FragPos = position;
}
)HERE";
