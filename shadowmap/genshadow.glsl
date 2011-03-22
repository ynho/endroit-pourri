[Vertex Shader]

varying vec4 pos;

void main (void)
{
    gl_TexCoord[0] = gl_MultiTexCoord0;
    pos = gl_ModelViewMatrix * gl_Vertex;
    gl_Position = ftransform ();
}

[Pixel Shader]

#define RANGE 2000.0

varying vec4 pos;

void main (void)
{
    gl_FragColor = vec4 (1.0);
    gl_FragDepth = length (pos) / RANGE;
}
