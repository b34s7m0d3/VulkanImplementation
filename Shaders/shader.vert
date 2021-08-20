#version 450 // version 4.5

layout (location = 0) out vec3 fragColor;   // output color for vertex (location is required)

// triangle vertex poositons (will put into vertex buffer later)
vec3 positions[3] = vec3[](
    vec3(0.0,-0.4,0.0),
    vec3(0.4,0.4,0.0),
    vec3(-0.4,0.4,0.0)
);

// triangle vertex colors
vec3 colors[3] = vec3[](
    vec3(1.0,0.0,0.0),
    vec3(0.0,1.0,0.0),
    vec3(0.0,0.0,1.0)
);

void main(){
    gl_Position = vec4(positions[gl_VertexIndex],1.0);
    fragColor = colors[gl_VertexIndex];
}