#version 450

#include "animation.shared"

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;
layout(location = 3) in ivec4 in_bone_ids; 
layout(location = 4) in vec4 in_bone_weights;


layout(location = 0) out vec2 out_texcoord;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_world_position;

layout(set = 0, binding = 0, std140) uniform InfoBlock
{
	Info info;
};


void main() {
	mat4 bone_matrices[MAX_BONE_INFLUENCES];
	for (int i = 0; i < MAX_BONE_INFLUENCES; i++) {
		if (in_bone_ids[i] == -1) {
		   continue;
		}

		bone_matrices[i] = info.animation_bones[in_bone_ids[i]];
	}

	const vec4 animated_vertex = animate_vertex(
		 in_position,
		 in_bone_ids,
		 in_bone_weights,
		 bone_matrices);

    const vec4 world_position = info.model * animated_vertex;

	gl_Position = info.projection * info.view * world_position;
    out_texcoord = in_texcoord;
    out_normal = mat3(info.model) * normalize(in_normal);
    out_world_position = vec3(world_position);
}
