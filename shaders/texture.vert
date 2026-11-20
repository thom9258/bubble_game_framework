#version 450

#include "texture.shared"

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

layout(location = 0) out vec2 out_texcoord;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_world_position;

layout(set = 0, binding = 0, std140) uniform InfoBlock
{
	Info info;
};


void main() {
    const vec4 world_position = info.model * vec4(in_position, 1.0);
	gl_Position = info.projection * info.view * world_position;
    out_texcoord = in_texcoord;
    //out_normal = mat3(info.model) * normalize(in_normal);
    out_normal = mat3(transpose(inverse(info.model))) * in_normal;
    out_world_position = vec3(world_position);
}
