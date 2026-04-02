#version 330 core
out vec4 FragColor;
in vec2 TexCoords;
void main() {
    vec2 check = floor(TexCoords * 8.0);
    float m = mod(check.x + check.y, 2.0);
    FragColor = mix(vec4(1.0, 0.0, 1.0, 1.0), vec4(0.0, 0.0, 0.0, 1.0), m);
}

