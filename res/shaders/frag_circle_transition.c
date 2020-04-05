in vec2 texUV_out;
out vec4 color;

uniform float circleSize_01;

//NOTE(ollie): Assumes width is the larger axis
uniform float aspectRatio_y_over_x;

float smoothing = 0.01f;

void main()
{   

    //NOTE(ollie): Make copy of the uv, since texUV_out is read only if think?
    vec2 altered_uv = texUV_out;

    altered_uv.x = 2.0f*altered_uv.x - 1.0f;
    altered_uv.y = 2.0f*altered_uv.y - 1.0f;

    //NOTE(ollie): Correct for the aspect ratio
    altered_uv.y *= aspectRatio_y_over_x;

    //NOTE(ollie): Get the vector length      
    float vecLen = length(altered_uv);

    //NOTE(ollie): Do sdf smoothing
    float alpha = smoothstep(circleSize_01 - smoothing, circleSize_01, vecLen);

    //NOTE(ollie): Assign the value
    color = vec4(0, 0, 0, alpha);

   
}