#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTex;
layout (location = 2) in float aTexID; // Nowy atrybut

out vec2 TexCoord;
out float TexID; 

uniform mat4 MVP;

void main() {
    TexCoord = aTex;
    TexID = aTexID;
    gl_Position = MVP * vec4(aPos, 1.0);
}