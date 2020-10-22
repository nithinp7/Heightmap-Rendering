#version 430 core
layout(location = 0) out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec4 Color;

void main()
{    
    FragColor = Color;
}
