in vec3 vertex;
in vec2 texUV;	

in mat4 M;
in mat4 V;

in vec4 color;
in vec4 uvAtlas;	

uniform mat4 projection;

out vec4 colorOut; //out going
out vec2 texUV_out;

void main() {
    
    gl_Position = projection * V * M * vec4(vertex, 1);
    colorOut = color;
    
    int xAt = int(texUV.x*2);
    int yAt = int(texUV.y*2) + 1;
    texUV_out = vec2(uvAtlas[xAt], uvAtlas[yAt]);
}