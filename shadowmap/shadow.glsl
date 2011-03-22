[Vertex Shader]

varying vec4 pos;
varying vec3 nor;
varying vec4 lpos;
varying vec2 texcoord;

uniform mat4 matrix0;
uniform mat4 matrix1;

float f (float x)
{
    return x - 0.0003;
}

void main (void)
{
    texcoord = gl_MultiTexCoord0.st;
    nor = gl_NormalMatrix * gl_Normal;
    lpos = gl_LightSource[0].position;

    // application de la matrice de l'objet
    pos = matrix1 * gl_Vertex;
    
    // application de la matrice de vue et projection
    gl_TexCoord[0] = matrix0 * pos;
    gl_TexCoord[0].xyz /= gl_TexCoord[0].w;
    // recadrage
    gl_TexCoord[0].xyz += vec3 (1., 1., 1.);
    gl_TexCoord[0].xyz /= 2.;
    gl_TexCoord[0].z = f (gl_TexCoord[0].z);
    pos = gl_ModelViewMatrix * gl_Vertex;
    gl_Position = ftransform ();
}

[Pixel Shader]

#version 120

#define A 0
#define B 1
#define C 0

#define RANGE 2000.0

varying vec4 pos;
varying vec3 nor;
varying vec4 lpos;
varying vec2 texcoord;

uniform bool dosmooth;
#if C
uniform sampler2DShadow depthmap;
#else
uniform sampler2D depthmap;
#endif

#define W (4096./2.)
#define H (4096./2.)

#define MAPW (W)
#define MAPH (H)

bool outshadow (vec3 coord)
{
#if A
    vec4 c = texture2D (depthmap, coord.xy);
    float a = (c.x + c.y + c.z + c.w) / 4.0;
    return (a > coord.z);
#elif B    
    float a = texture2D (depthmap, coord.xy).x;
    return (a > coord.z);
#elif C
    return (shadow2D (depthmap, gl_TexCoord[0].xyz).x > 0.0 ? true : false);
#else
    return false;
#endif
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
