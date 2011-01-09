[vertex shader]

varying vec2 texcoord;

void main ()
{
    texcoord = gl_MultiTexCoord0.xy;
    gl_Position = ftransform ();
}

[pixel shader]

#define USE_SPECULAR 1

varying vec2 texcoord;

uniform sampler2D normalmap, colormap, depthmap;

vec3 getpixpos (mat4 invviewproj, vec2 coord, float depth)
{
    vec4 pos = vec4 (coord, depth, 1.0);
    pos *= 2.0;
    pos -= vec4 (1.0);
#if 0
    pos = invviewproj * pos;
    pos.xyz /= pos.w;
    return pos.xyz;
#else
    vec4 final = invviewproj[2] * pos.z + invviewproj[3];
    final += invviewproj[0] * pos.x;
    final += invviewproj[1] * pos.y;
    final.xyz /= final.w;
    return final.xyz;
#endif
}

vec3 getnormal (sampler2D normalmap, vec2 texcoord)
{
    return normalize (2.0 * (texture2D (normalmap, texcoord).xyz - vec3 (0.5)));
}

float getintensity (vec3 normal, vec3 lightvec)
{
    return max (dot (normal, -lightvec), 0.0) * 0.7;
}

float getattenuation (float d, float r)
{
    return max (1.0 - pow ((d / r), 8.), 0.0);
}

void main ()
{
    vec4 fragcolor = vec4 (0.0);

    float depth = texture2D (depthmap, texcoord).x;
    vec3 pos = getpixpos (gl_TextureMatrix[0], texcoord, depth);

    float lightradius = 3.0;
    vec3 lightpos = gl_LightSource[0].position.xyz;
    float dist = distance (pos, lightpos);
    float att = getattenuation (dist, lightradius);
    float intensity = att;

    if (intensity > 0.0)
    {
        vec3 normal = getnormal (normalmap, texcoord);
        vec3 lightvec = normalize (pos - lightpos);
        intensity *= getintensity (normal, lightvec);
        //if (intensity > 0.01)
        {
            vec4 lightcolor = gl_LightSource[0].diffuse;
#if USE_SPECULAR
            vec3 eyepos = getpixpos (gl_TextureMatrix[0], vec2 (0.5, 0.5), 0.0);
            vec3 eyevec = normalize (pos - eyepos);
            float specular = clamp (dot (reflect (lightvec, normal), -eyevec), 0.0, 1.0);
            specular = pow (specular, 16.0);
            specular *= att;
#endif

            fragcolor = texture2D (colormap, texcoord) * lightcolor;
#if USE_SPECULAR
            fragcolor = fragcolor * (intensity + specular);
#else
            fragcolor *= intensity;
#endif
        }
    }
    
    gl_FragColor = fragcolor;
}
