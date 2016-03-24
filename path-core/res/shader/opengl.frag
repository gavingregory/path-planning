
uniform sampler2D myTexture;

void main (void)
{
    //vec4 col = texture2D(myTexture, vec2(gl_TexCoord[0]));
    //col *= gl_Color;
	vec4 col = gl_Color;
	col.a = 0.1;
    gl_FragColor = col; //gl_Color; //col * 1.0;
}