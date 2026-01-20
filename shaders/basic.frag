#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
in float TexID;

uniform sampler2D textures[6]; // Tablica 6 tekstur
uniform vec4 color;
uniform bool useTexture;

void main() {
    if (useTexture) {
        int id = int(TexID);
        FragColor = texture(textures[id], TexCoord) * color;
    } else {
        FragColor = color;
    }
}