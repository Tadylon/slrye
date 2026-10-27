#version 450

layout(push_constant) uniform ViewportPushConstants
{
    mat4 view_projection;
} camera;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_color;

layout(location = 0) out vec3 fragment_normal;
layout(location = 1) out vec3 fragment_color;

void main()
{
    gl_Position = camera.view_projection * vec4(in_position, 1.0);
    fragment_normal = in_normal;
    fragment_color = in_color;
}
