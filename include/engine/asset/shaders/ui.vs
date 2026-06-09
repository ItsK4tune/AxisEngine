#version 460 core



layout (location = 0) in vec4 vertex; 



uniform mat4 u_Projection;

uniform mat4 u_Model;



out vec2 TexCoords;



void main() {

    TexCoords = vertex.zw;

    gl_Position = u_Projection * u_Model * vec4(vertex.xy, 0.0, 1.0);

}


