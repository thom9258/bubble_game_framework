#version 450

layout(location = 0) in vec2 in_texcoord;

layout(location = 0) out vec4 out_color;

layout(set = 1, binding = 0) 
uniform sampler2D diffuse;

void main() {
    const vec3 albedo = vec3(texture(diffuse, in_texcoord));
    out_color = vec4(albedo, 1.0);
}
