[vertex shader]

varying float depth;

#define SCALE 24.

void main ()
{
    float i;
    vec3 n = normalize (gl_NormalMatrix * gl_Normal);
    vec4 vvec = gl_ModelViewMatrix * gl_Vertex;
    vec3 lpos = gl_LightSource[0].position.xyz;
    vec3 lvec = normalize (lpos - vvec.xyz);

    i = dot (n, lvec);
    depth = length (vvec.xyz) / SCALE;
    gl_FrontColor = vec4(0.8, 0.2, 0.2, 1.0) * i;
    gl_Position = gl_ProjectionMatrix * vvec;
}

[pixel shader]

varying float depth;

void main ()
{
    gl_FragData[1] = vec4 (depth);
    gl_FragData[0] = gl_Color;
}
