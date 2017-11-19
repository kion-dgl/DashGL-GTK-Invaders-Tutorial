#version 120

varying vec2 texcoord;
uniform sampler2D mytexture;

void main(void) {
	
	vec2 flipped_texcoord = vec2(texcoord.x, 1.0 - texcoord.y);
	gl_FragColor = texture2D(mytexture, flipped_texcoord);

}
