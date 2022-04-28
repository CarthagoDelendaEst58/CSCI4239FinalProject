// Barebones vertex shader

#version 330 core

uniform mat4 ProjectionMatrix;
uniform mat4 ModelViewMatrix;

in vec4 Vertex;

void main()
{	
    gl_Position = ProjectionMatrix * ModelViewMatrix * Vertex;
}