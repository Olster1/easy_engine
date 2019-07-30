struct Material {
    sampler2D diffuse;
    sampler2D specular;
    // sampler2D specular;
    
    float specularConstant;
};

struct DirLight {
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	vec4 direction;
};

in vec4 color_frag; 
in vec3 normal_frag; //worldspace
in vec2 uv_frag; 
in vec3 fragPos; //worldspace

uniform Material material;
uniform DirLight lights[16];
out vec4 color;
uniform vec3 eye_worldspace; 

void main (void) {
    vec4 c = color_frag;

    vec3 diffTex = vec3(texture(material.diffuse, uv_frag));
    vec3 specTex = vec3(texture(material.specular, uv_frag));

    vec3 lightDir = vec3(normalize(-lights[0].direction));
    float diff = max(dot(normal_frag, lightDir), 0.0);

    vec3 viewDir = normalize(eye_worldspace - fragPos);
    vec3 reflectDir = reflect(-lightDir, normal_frag);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.specularConstant);

    vec3 ambientColor = diffTex * lights[0].ambient;
    vec3 diffuseColor = diff * diffTex * lights[0].diffuse;
    vec3 specularColor = spec * specTex * lights[0].specular;

    color = c * vec4(ambientColor + diffuseColor + specularColor, 1.0);
}