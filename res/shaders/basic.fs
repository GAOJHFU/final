#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
} fs_in;


uniform sampler2D diffuseTex;

void main()
{           
    vec3 color = texture(diffuseTex, fs_in.TexCoords).rgb;
    FragColor = vec4(color, 1.0);
}