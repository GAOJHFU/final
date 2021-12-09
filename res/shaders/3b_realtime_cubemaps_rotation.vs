#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform float time;
out vec3 Normal;
out vec3 Position;

void main()
{
    Position = vec3(model * vec4(aPos, 1.0));
	vec3 wPos = Position;
    Position.x += 5 * cos(time);
    Position.z += 5 * sin(time);

    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * model * vec4(Position, 1.0);
}