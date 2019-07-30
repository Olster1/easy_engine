in vec3 vertex;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords = vertex;
    vec4 pos = projection * view * vec4(vertex, 1.0);
    gl_Position = pos.xyww;
}  