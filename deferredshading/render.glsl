[vertex shader]

varying vec3 normal;

void main ()
{
    normal = normalize (gl_Normal);
    normal /= 2.0;
    normal += vec3 (.5);

    gl_FrontColor = vec4 (1.0);
    gl_Position = ftransform ();
}

[pixel shader]

varying vec3 normal;

void main ()
{
    gl_FragData[1] = vec4 (normal, 0.0);
    gl_FragData[0] = gl_Color;
}
