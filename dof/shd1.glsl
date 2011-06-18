[Pixel shader]

#version 120

// width
#define W (1024.0 / 2.0)
// height
#define H (768.0 / 2.0)
// length
#define L 6
// one pixel
#define PW (1./W)
#define PH (1./H)

uniform sampler2D depthmap, colormap;
uniform int sens;

#define MOP 6.8
#define MARGE 2.0

#define SCALE 34.


float blurratio (float prof, float x)
{
    return clamp (abs (prof - x) / MARGE, 0.0, 1.0);
}

void main ()
{
    vec2 tc, texc = gl_TexCoord[0].xy;
    int i;
    float coeffs[L] = float[L](0.15, 0.15, 0.125, 0.1, 0.05, 0.05);
    vec4 color;
    float d;
    vec2 dir = (sens == 0 ? vec2 (1.0, 0.0) : vec2 (0.0, 1.0));
    float depth = texture2D (depthmap, texc).x * SCALE;
    float tau, gtau = blurratio (MOP, depth);

    vec4 pxcolor = texture2D (colormap, texc);
    color = pxcolor * coeffs[0];

    for (i = 1; i < L; i++)
    {
        tc = texc + dir * PW*i;
        d = texture2D (depthmap, tc).x * SCALE;
        tau = blurratio (MOP, d);
        if (d > depth)
            tau *= gtau;
        color += coeffs[i] * tau * texture2D (colormap, tc);
        color += coeffs[i] * (1. - tau) * pxcolor;
    }

    for (i = 1; i < L; i++)
    {
        tc = texc + dir * -PW*i;
        d = texture2D (depthmap, tc).x * SCALE;
        tau = blurratio (MOP, d);
        if (d > depth)
            tau *= gtau;
        color += coeffs[i] * tau * texture2D (colormap, tc);
        color += coeffs[i] * (1. - tau) * pxcolor;
    }

    gl_FragColor = color;
}
