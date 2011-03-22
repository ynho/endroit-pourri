[Vertex Shader]

varying vec4 pos;
varying vec3 nor;
varying vec4 lpos;

uniform mat4 matrix0;           // light camera's matrix
uniform mat4 matrix1;           // object's matrix

void main (void)
{
    nor = gl_NormalMatrix * gl_Normal;
    lpos = gl_LightSource[0].position;

    pos = matrix1 * gl_Vertex;
    
    gl_TexCoord[0] = matrix0 * pos;
    gl_TexCoord[0].xy /= gl_TexCoord[0].w;
    gl_TexCoord[0].xy += vec2 (1., 1.);
    gl_TexCoord[0].xy /= 2.;

    pos = gl_ModelViewMatrix * gl_Vertex;
    gl_Position = ftransform ();
}

[Pixel Shader]

#version 120

#define RANGE 2000.0

varying vec4 pos;
varying vec3 nor;
varying vec4 lpos;

uniform bool dosmooth;
uniform sampler2D depthmap;

#define W (4096./2.)
#define H (4096./2.)

#define MAPW (W)
#define MAPH (H)

bool outshadow (vec3 coord)
{
    float a = texture2D (depthmap, coord.xy).x;
    return (a > coord.z);
}

float getintensity (void)
{
    vec3 ptol = normalize (vec3 (lpos) - vec3 (pos)); // pixel to light
    return max (dot (normalize (nor), ptol), 0.);
        
}

void main (void)
{
    vec4 color;
    vec4 texcolor;
    float intensity = 0.1; // ambient color
    
    texcolor = vec4(1.0);
    vec3 coords = gl_TexCoord[0].xyz;
    coords.z = length (pos - lpos) / RANGE - 0.0001;
    
    if (outshadow (coords)) {
        intensity += getintensity ();
#if 1
        if (dosmooth) {
            float value = (intensity - 0.1) / 10.;
            if (!outshadow (coords + vec3 (0., 1./MAPH, 0.0)))
                intensity -= value;
            if (!outshadow (coords + vec3 (0., -1./MAPH, 0.0)))
                intensity -= value;
            if (!outshadow (coords + vec3 (1./MAPW, 0., 0.0)))
                intensity -= value;
            if (!outshadow (coords + vec3 (-1./MAPW, 0., 0.0)))
                intensity -= value;
                
            if (!outshadow (coords + vec3 (1./MAPW, 1./MAPH, 0.0)))
                intensity -= value;
            if (!outshadow (coords + vec3 (1./MAPW, -1./MAPH, 0.0)))
                intensity -= value;
            if (!outshadow (coords + vec3 (-1./MAPW, 1./MAPH, 0.0)))
                intensity -= value;
            if (!outshadow (coords + vec3 (-1./MAPW, -1./MAPH, 0.0)))
                intensity -= value;
            
            value = (intensity - 0.1) / 16.;
            #undef MAPH
            #undef MAPW
            #define MAPW (W/2.)
            #define MAPH (H/2.)
            
            if (!outshadow (coords + vec3 (0., 1./MAPH, 0.0)))
                intensity -= value;
            if (!outshadow (coords + vec3 (0., -1./MAPH, 0.0)))
                intensity -= value;
            if (!outshadow (coords + vec3 (1./MAPW, 0., 0.0)))
                intensity -= value;
            if (!outshadow (coords + vec3 (-1./MAPW, 0., 0.0)))
                intensity -= value;
                
            if (!outshadow (coords + vec3 (1./MAPW, 1./MAPH, 0.0)))
                intensity -= value;
            if (!outshadow (coords + vec3 (1./MAPW, -1./MAPH, 0.0)))
                intensity -= value;
            if (!outshadow (coords + vec3 (-1./MAPW, 1./MAPH, 0.0)))
                intensity -= value;
            if (!outshadow (coords + vec3 (-1./MAPW, -1./MAPH, 0.0)))
                intensity -= value;
            
            #undef MAPH
            #undef MAPW
            #define MAPW (W/4.)
            #define MAPH (H/4.)
            
            if (!outshadow (coords + vec3 (0., 1./MAPH, 0.0)))
                intensity -= value;
            if (!outshadow (coords + vec3 (0., -1./MAPH, 0.0)))
                intensity -= value;
            if (!outshadow (coords + vec3 (1./MAPW, 0., 0.0)))
                intensity -= value;
            if (!outshadow (coords + vec3 (-1./MAPW, 0., 0.0)))
                intensity -= value;
                
            if (!outshadow (coords + vec3 (1./MAPW, 1./MAPH, 0.0)))
                intensity -= value;
            if (!outshadow (coords + vec3 (1./MAPW, -1./MAPH, 0.0)))
                intensity -= value;
            if (!outshadow (coords + vec3 (-1./MAPW, 1./MAPH, 0.0)))
                intensity -= value;
            if (!outshadow (coords + vec3 (-1./MAPW, -1./MAPH, 0.0)))
                intensity -= value;
        }
#endif
    }
    color = texcolor * intensity;// * vec4 (0.27, 0.37, .7, 1.));
    
    gl_FragColor = color;
    //gl_FragColor = vec4 (gl_TexCoord[0].x, gl_TexCoord[0].y, 0.0, 0.0);
    //gl_FragColor = texture2D (depthmap, gl_TexCoord[0].st);
}
