#version 450

#include "texture.shared"

layout(location = 0) in vec2 in_texcoord;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_world_position;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0, std140) uniform InfoBlock
{
	Info info;
};

layout(set = 1, binding = 0) 
uniform sampler2D diffuse;

void main() {
    const vec3 normal = normalize(in_normal);
    vec3 light = hemisphere_light(info.hemisphere_light, normal);
    light += directional_light(info.directional_light, normal);

    for (int i = 0; i < POINT_LIGHT_COUNT; i++) {
        light += point_light(info.point_lights[i], normal, in_world_position);
    }

    light = min(light, vec3(2.0));
    const vec4 albedo = texture(diffuse, in_texcoord);
    out_color = vec4(albedo.rgb * light, albedo.a);
}
