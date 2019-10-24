out vec4 FragColor;
out vec4 BrightColor;

in vec2 texUV_out;

uniform float horizontal;
uniform sampler2D image;

uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{    
	vec2 tex_offset = textureSize(image, 0); // gets size of single texel
	tex_offset.x = 1.0f / tex_offset.x;
	tex_offset.y = 1.0f / tex_offset.y;

    vec3 result = texture(image, texUV_out).rgb * weight[0]; // current fragment's contribution
    if(horizontal > 0.0f)
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(image, texUV_out + vec2(tex_offset.x * i, 0.0)).rgb * weight[i]; //convert per pixel to uv space
            result += texture(image, texUV_out - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
        }
    }
    else
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(image, texUV_out + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
            result += texture(image, texUV_out - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
        }
    }

    FragColor = vec4(result, 1.0f);

   
}