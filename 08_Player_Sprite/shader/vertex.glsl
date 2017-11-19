#version 120

uniform mat4 orthograph;
uniform mat4 mvp;
attribute vec2 coord2d;
attribute vec2 uv_coord;
varying vec2 texcoord;

void main (void) {
	gl_Position = orthograph * mvp * vec4(coord2d, 0.0, 1.0);
	texcoord = uv_coord;
}
