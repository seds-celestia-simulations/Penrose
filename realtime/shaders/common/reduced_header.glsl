#version 430 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D skybox;
uniform mat4 uInvProjView;
uniform vec3 uCameraPos;
uniform float uTime;

#include "particle.glsl"
