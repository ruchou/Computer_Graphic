#version 430

in vec3 Color;

out vec4 FragColor;

struct LightInfo{
	vec4 Position;	// Light position in eye coords
	vec3 La;		// Ambient light intensity
	vec3 Ld;		// Diffuse light intensity
	vec3 Ls;		// Specular light intensity
};

uniform LightInfo light[3];

void main() {
	FragColor = light[0].Position;
}
