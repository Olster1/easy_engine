in vec3 vertex;
in vec3 normal;
in vec2 texUV;	


//Instanced variables
in mat4 M;
in mat4 V;

in vec4 color;
in vec4 uvAtlas;	//not using a atlas

uniform mat4 projection;

//NOTE(ollie): This matrix means we can't batch multiple calls, however we could get rid of it by putting the divide by w in the matrix (but not sure if you can do this? SInce the your dividing by the value of the coordinate, like double differential equations?)
uniform mat4 orthoToScreen; //this is the matrix the transforms lerps between the x & y positions then from there transforms it back into NDC space

//outgoing variables
out vec4 color_frag; 
out vec3 normal_frag;
out vec2 uv_frag;
out vec3 fragPos;
out vec3 fragPosInViewSpace;

void main() {
    
    vec4 pos = projection * V * M * vec4(vertex, 1);
    //Now in clip 
    pos.x /= pos.w;
    pos.y /= pos.w;
    pos.z /= pos.w;
    pos.w  = 1.0f;

    //Convert between -1 -> 1 to 0 -> 1
    pos.x += 1.0f;
    pos.x /= 2.0f;
    
    pos.y += 1.0f;
    pos.y /= 2.0f;

    //We don't do z since we are using the z-coordinate based off the z we send in the ortho matrix

    //Now in NDC space
    gl_Position = orthoToScreen * pos; //homegenous coordinrate since we want the translation component as well
    color_frag = color;
    normal_frag = mat3(transpose(inverse(M))) * normal; //transpose(inverse(M))
    uv_frag = texUV;
    fragPos = vec3(M * vec4(vertex, 1));
    fragPosInViewSpace = vec3(V*M * vec4(vertex, 1));
}