#version 430 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec4 col;

out vec3 FragPos;
out vec3 Normal;
out vec4 Color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = (model * vec4(pos, 1.0)).xyz;//projection * view * model * vec4(pos, 1.0);    
    Normal = mat3(transpose(inverse(model))) * normalize(norm);
    Color = col;

    gl_Position = projection * view * model * vec4(pos, 1);
}
