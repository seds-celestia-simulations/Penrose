#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image2D imgOutput;
layout(binding = 1) uniform sampler2D uGeodesicLUT; // Matches GL_TEXTURE1 in GeodesicPass

uniform sampler2D skybox;
uniform mat4 uInvProjView;
uniform vec3 uCameraPos;
uniform float uTime;
uniform vec2 uResolution; // Added to map thread ID to UV

uniform float uLutRMin; 
uniform float uLutRMax;

#include "particle.glsl"