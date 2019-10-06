in vec3 vertex;
in vec3 normal;
in vec2 texUV;	

out vec2 uv_frag;

uniform mat4 projection;
// uniform mat4 view;
// uniform mat4 model;

in mat4 M;
in mat4 V;

in vec4 color;
in vec4 uvAtlas;	//not using a atlas

void main()
{
    uv_frag = texUV;
    gl_Position = projection * V * M * vec4(vertex, 1.0);
}  