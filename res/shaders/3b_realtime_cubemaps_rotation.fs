#version 330 core
out vec4 FragColor;

uniform vec3 cPos;
uniform vec3 lightPos;

in vec3 Normal;
in vec3 Position;

void main()
{
	vec3 ka = vec3(0.2, 0.2, 0.2);
	vec3 kd = vec3(0.8, 0.7, 0.7);
	vec3 ks = vec3(1.0, 0.9, 0.8);

	vec3 n = normalize(Normal);
	vec3 lightDir = normalize(lightPos - Position);
	vec3 viewDir = normalize(cPos - Position);
	vec3 halfDir = normalize(lightDir + viewDir);
	float spec = max(0, dot(halfDir, n));
	float totalContrib = pow(spec, 64.0);

	vec3 color = ka + kd * max(0, dot(lightDir, n)) + ks * totalContrib;
	gl_FragColor = vec4(color, 1.0);
}