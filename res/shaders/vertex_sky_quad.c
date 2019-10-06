in vec3 vertex;
in vec3 normal;
in vec2 texUV;	

uniform float fov;
uniform float aspectRatio;
uniform mat4 camera_to_world_transform;
uniform float nearZ;

//outgoing variables
out vec3 direction_frag; 

void main() {
    
    gl_Position = vec4(vertex.x, vertex.y, 1.0f, 1);
    float t = tan(fov/2)*nearZ;
	float r = t*aspectRatio;
	vec3 ray = vec3(mix(-r, r, texUV.x), mix(-t, t, texUV.y), nearZ);

    direction_frag = mat3(camera_to_world_transform) * ray;
}