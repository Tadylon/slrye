#version 450

layout(location = 0) in vec3 fragment_normal;
layout(location = 1) in vec3 fragment_color;

layout(location = 0) out vec4 out_color;

void main()
{
    vec3 light_direction = normalize(vec3(0.35, 0.85, 0.4));
    float diffuse = max(dot(normalize(fragment_normal), light_direction), 0.0);
    float lighting = 0.28 + diffuse * 0.72;
    out_color = vec4(fragment_color * lighting, 1.0);
}
