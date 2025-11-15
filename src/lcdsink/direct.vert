#version 330 core

// Positions/Coordinates
layout (location = 0) in vec3 in_position;
// Texture Coordinates
layout (location = 1) in vec2 in_texCoord;

// Outputs the texture coordinates to the fragment shader
out vec2 vert_texCoord;

void main() {
    // Outputs the positions/coordinates of all vertices
    gl_Position = vec4(in_position, 1.0);
    // Assigns the texture coordinates from the Vertex Data to "vert_texCoord"
    vert_texCoord = in_texCoord;
}