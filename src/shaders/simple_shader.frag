#version 450 core
out vec4 FragColor;

in vec3 normal;
in vec2 uv;

void main()
{
    vec3 color = normalize(normal) * 0.5 + 0.5;
    FragColor = vec4(color, 1.0);
}