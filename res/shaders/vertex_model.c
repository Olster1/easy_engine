in mat4 M;
in mat4 V;
in mat4 P;
in vec4 color;
in vec4 uvAtlas;	//not using a atlas

in vec3 vertex;
in vec3 normal;
in vec2 texUV;	

//outgoing variables
out vec4 color_frag; 
out vec3 normal_frag;
out vec2 uv_frag;
out vec3 fragPos;

void main() {
    
    gl_Position = P * V * M * vec4(vertex, 1);
    color_frag = color;
    normal_frag = mat3(transpose(inverse(M))) * normal;
    uv_frag = texUV;
    fragPos = vec3(M * vec4(vertex, 1));
}