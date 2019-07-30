in mat4 M;
in mat4 V;
in mat4 P;
in vec4 color;
in vec4 uvAtlas;	

in vec3 vertex;
in vec2 texUV;	

out vec4 colorOut; //out going
out vec2 texUV_out;

void main() {
    
    gl_Position = P * V * M * vec4(vertex, 1);
    colorOut = color;
    
    int xAt = int(texUV.x*2);
    int yAt = int(texUV.y*2) + 1;
    texUV_out = vec2(uvAtlas[xAt], uvAtlas[yAt]);
}