#version 130

varying vec2 texcoord;
uniform sampler2D mytexture;

void main(void) {
	
	vec2 flipped_texcoord = vec2(texcoord.x, 1.0 - texcoord.y);
	vec4 color = texture2D(mytexture, flipped_texcoord);
	
	if(color.w == 0) {
		discard;
	}
	
	gl_FragColor = color;
}
