// Barebones vertex shader

#version 400 core

uniform mat4 ProjectionMatrix;
uniform mat4 ModelViewMatrix;

in vec4 Vertex;

void main()
{	
    gl_Position = ProjectionMatrix * ModelViewMatrix * Vertex;
}