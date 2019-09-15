struct Material {
    sampler2D diffuse;
    sampler2D specular;
    
    float specularConstant;

    vec3 ambientDefault;
    vec3 diffuseDefault;
    vec3 specularDefault;

    int useDefault;
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
    float gamma = 2.2;

    vec4 c = color_frag;
    vec3 normal = normalize(normal_frag);
    vec3 ambTex = material.ambientDefault;
    vec3 diffTex = material.diffuseDefault;
    vec3 specTex = material.specularDefault;
    if(material.useDefault == 0) {
        ambTex = diffTex = vec3(texture(material.diffuse, uv_frag));
        specTex = vec3(texture(material.specular, uv_frag));
    } 

    //NOTE(ol): Gamma correction
    // ambTex = pow(ambTex, vec3(gamma));
    // diffTex = pow(diffTex, vec3(gamma));
    // specTex = pow(specTex, vec3(gamma));
    
    vec3 lightDir = vec3(normalize(-lights[0].direction));
    float diff = max(dot(normal, lightDir), 0.0);

    vec3 viewDir = normalize(eye_worldspace - fragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.specularConstant);

    vec3 ambientColor = ambTex * lights[0].ambient;
    vec3 diffuseColor = diff * diffTex * lights[0].diffuse;
    vec3 specularColor = spec * specTex * lights[0].specular;

    color = c * vec4(ambientColor + diffuseColor + specularColor, 1.0);

    // color.rgb = pow(color.rgb, vec3(1.0/gamma));
}