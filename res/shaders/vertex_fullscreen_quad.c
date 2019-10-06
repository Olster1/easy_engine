in vec3 vertex;
in vec3 normal;
in vec2 texUV;	

//outgoing variables
out vec2 texUV_out; 

void main() {
    gl_Position = vec4(vertex.x, vertex.y, 1.0f, 1);
    texUV_out = texUV;
}