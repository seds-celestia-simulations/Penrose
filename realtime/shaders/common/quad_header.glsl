#version 430 core
out vec4 FragColor;
in vec2 TexCoords;

uniform float uTime;
uniform vec2 uResolution;
uniform vec3 uCameraPos;

uniform sampler2D skybox;
uniform mat4 uInvProjView;

uniform bool uHighQualityPass;
uniform float uHighQualityRadius;

#include "particle.glsl"

uniform int uParticleCount;
