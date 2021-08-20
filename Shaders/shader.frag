#version 450

layout (location = 0) in vec3 fragColor;    // interpolated color from vertex (location must match)

layout (location = 0) out vec4 outColor;    // final output color (must also have location)

void main(){
    outColor = vec4(fragColor, 1.0);
}