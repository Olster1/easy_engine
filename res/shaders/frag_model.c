struct Material {
    sampler2D diffuse;
    sampler2D specular;
    
    float specularConstant;

    vec4 ambientDefault;
    vec4 diffuseDefault;
    vec4 specularDefault;
};

struct DirLight {
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;

	vec4 direction;
};

in vec4 color_frag; 
in vec3 normal_frag; //worldspace
in vec2 uv_frag; 
in vec3 fragPos; //worldspace
in vec3 fragPosInViewSpace; //worldspace

uniform Material material;
uniform DirLight lights[16];
out vec4 color;
out vec4 BrightColor;
uniform vec3 eye_worldspace; 

void main (void) {
    float gamma = 2.2;

    vec4 c = color_frag;
    //Normalize in the frag shader since interpolated one across vertexes isn't neccessarily normalized
    vec3 normal = normalize(normal_frag);

    vec4 diffSample = texture(material.diffuse, uv_frag);
    if(diffSample.w == 0.0f) {
        discard;
    }
    vec4 ambTex = material.ambientDefault*diffSample;
    vec4 diffTex = material.diffuseDefault*diffSample;
    vec4 specTex = material.specularDefault*texture(material.specular, uv_frag);

    //NOTE(ol): Gamma correction
    // ambTex = pow(ambTex, vec4(gamma));
    // diffTex = pow(diffTex, vec4(gamma));
    // specTex = pow(specTex, vec4(gamma));
    
    vec3 lightDir = vec3(normalize(-lights[0].direction));
    float diff = max(dot(normal, lightDir), 0.0);

    vec3 viewDir = normalize(eye_worldspace - fragPos);
    vec3 halfwayVector = normalize(viewDir + lightDir);
    float spec = pow(max(dot(normal, halfwayVector), 0.0), material.specularConstant);

    vec4 ambientColor = ambTex * lights[0].ambient;
    vec4 diffuseColor = diff * diffTex * lights[0].diffuse;
    vec4 specularColor = spec * specTex * lights[0].specular;

    ambientColor.w = ambTex.w;
    diffuseColor.w = diffTex.w;
    specularColor.w = specTex.w;

    vec4 tempColor = c * vec4(0.5f*ambientColor + diffuseColor + specularColor);
    tempColor.w = min(1, tempColor.w);
    tempColor.rgb *= tempColor.a;

    float tAt = min(fragPosInViewSpace.z/100.0f, 1.0f);
    color = tempColor;//mix(tempColor, vec4(0.4f, 0.6f, 1.0f, 1.0f), pow(tAt, 3.0f));

    // color = pow(color, vec4(1.0/gamma));

    float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.0)
        BrightColor = vec4(color.rgb, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}