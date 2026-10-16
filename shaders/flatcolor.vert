#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

layout(location = 0) out vec3 out_color;

layout (set = 0, binding = 0, std140)
uniform Info
{
	mat4 model;
	mat4 view;
	mat4 projection;
    vec3 color;
} info;

void main() {
    const mat4 transform = info.projection * info.view * info.model;
	const vec4 position = transform * vec4(in_position, 1.0f);
	gl_Position = position;
    out_color = info.color;
}
