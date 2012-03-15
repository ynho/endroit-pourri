[vertex shader]

varying vec4 position;

void main ()
{
    position = gl_ModelViewProjectionMatrix * gl_Vertex;
    gl_Position = ftransform ();
}

[pixel shader]

varying vec4 position;

float getintensity (vec3 normal, vec3 lightdir)
{
    return max (dot (normal, -lightdir), 0.0);
}

float getspecular (vec3 normal, vec3 lightdir, vec3 eyedir)
{
    float d = dot (reflect (lightdir, normal), -eyedir);
    return clamp (d, 0.0, 1.0);
}

/* modulate specular with diffuse intensity */
float coeffspec (float intensity)
{
    return 1.0 - exp (4.0 * -intensity);
}


#if SCE_DEFERRED_USE_SHADOWS

#undef SCE_DEFERRED_USE_SOFT_SHADOWS
#define SCE_DEFERRED_USE_SOFT_SHADOWS 1

#define N_SPLITS 4

#if 0
#define W (512 * N_SPLITS)
#define H (512)
#elif 1
#define W (1024 * N_SPLITS)
#define H (1024)
#else
#define W (2048 * N_SPLITS)
#define H (2048)
#endif

float outshadow (vec3 coord)
{
    /* TODO: ugly */
    if (abs (coord.x) > 1.0 || abs (coord.y) > 1.0 || abs (coord.z) > 1.0)
        return 1.0;
    float a = texture2D (sce_shadow_map, coord.xy).x;
    return (a > coord.z) ? 1.0 : 0.0;
}

/* new soft shadows */
#if 1
float shadow_intensity (vec3 coord)
{
    if (outshadow (coord) > 0.5)
        return 1.0;
#if SCE_DEFERRED_USE_SOFT_SHADOWS
    else {
        float map[9]; /* intensities on the sides of the pixel and middle */
        vec2 px = vec2 (1.0 / W, 1.0 / H);
        vec3 center;

        center.xy = (floor (coord.xy * vec2 (W, H)) + vec2 (0.5, 0.5)) / vec2 (W, H);
        center.z = coord.z;

        vec2 dec = (coord.xy - center.xy) * vec2 (W, H); /* decal */

        /* fillup map */
        map[0] = outshadow (center + vec3 (-px.x, px.y, 0.0));
        map[1] = outshadow (center + vec3 (0.0, px.y, 0.0));
        map[2] = outshadow (center + vec3 (px.x, px.y, 0.0));

        map[3] = outshadow (center + vec3 (-px.x, 0.0, 0.0));
        map[4] = outshadow (center);
        map[5] = outshadow (center + vec3 (px.x, 0.0, 0.0));

        map[6] = outshadow (center + vec3 (-px.x, -px.y, 0.0));
        map[7] = outshadow (center + vec3 (0.0, -px.y, 0.0));
        map[8] = outshadow (center + vec3 (px.x, -px.y, 0.0));

        float a;

        if (dec.x < 0.0) {
            float left = mix (map[4], map[3], abs (dec.x));
            if (dec.y < 0.0) {
                a = mix (map[7], map[6], abs (dec.x));
                return mix (left, a, abs (dec.y));
            } else {
                a = mix (map[1], map[0], abs (dec.x));
                return mix (left, a, abs (dec.y));
            }
        } else {
            float right = mix (map[4], map[5], abs (dec.x));
            if (dec.y > 0.0) {
                a = mix (map[1], map[2], abs (dec.x));
                return mix (right, a, abs (dec.y));
            } else {
                a = mix (map[7], map[8], abs (dec.x));
                return mix (right, a, abs (dec.y));
            }
        }
    }
#else
    return 0.0;
#endif  /* SCE_DEFERRED_USE_SOFT_SHADOWS */
}

/* old soft shadows */
#else
float shadow_intensity (vec3 coord)
{
    if (outshadow (coord))
        return 1.0;
#if SCE_DEFERRED_USE_SOFT_SHADOWS
    else {

#define MAPW (W)
#define MAPH (H)
        float intensity = 0.0;
        float value = 1.0 / 10.;
        if (outshadow (coord + vec3 (0., 1./MAPH, 0.0)))
            intensity += value;
        if (outshadow (coord + vec3 (0., -1./MAPH, 0.0)))
            intensity += value;
        if (outshadow (coord + vec3 (1./MAPW, 0., 0.0)))
            intensity += value;
        if (outshadow (coord + vec3 (-1./MAPW, 0., 0.0)))
            intensity += value;
                
        if (outshadow (coord + vec3 (1./MAPW, 1./MAPH, 0.0)))
            intensity += value;
        if (outshadow (coord + vec3 (1./MAPW, -1./MAPH, 0.0)))
            intensity += value;
        if (outshadow (coord + vec3 (-1./MAPW, 1./MAPH, 0.0)))
            intensity += value;
        if (outshadow (coord + vec3 (-1./MAPW, -1./MAPH, 0.0)))
            intensity += value;

        value = (1.0 - intensity) / 16.;
#undef MAPH
#undef MAPW
#define MAPW (W/2.)
#define MAPH (H/2.)
            
        if (outshadow (coord + vec3 (0., 1./MAPH, 0.0)))
            intensity += value;
        if (outshadow (coord + vec3 (0., -1./MAPH, 0.0)))
            intensity += value;
        if (outshadow (coord + vec3 (1./MAPW, 0., 0.0)))
            intensity += value;
        if (outshadow (coord + vec3 (-1./MAPW, 0., 0.0)))
            intensity += value;
                
        if (outshadow (coord + vec3 (1./MAPW, 1./MAPH, 0.0)))
            intensity += value;
        if (outshadow (coord + vec3 (1./MAPW, -1./MAPH, 0.0)))
            intensity += value;
        if (outshadow (coord + vec3 (-1./MAPW, 1./MAPH, 0.0)))
            intensity += value;
        if (outshadow (coord + vec3 (-1./MAPW, -1./MAPH, 0.0)))
            intensity += value;
            
#undef MAPH
#undef MAPW
#define MAPW (W/4.)
#define MAPH (H/4.)
            
        if (outshadow (coord + vec3 (0., 1./MAPH, 0.0)))
            intensity += value;
        if (outshadow (coord + vec3 (0., -1./MAPH, 0.0)))
            intensity += value;
        if (outshadow (coord + vec3 (1./MAPW, 0., 0.0)))
            intensity += value;
        if (outshadow (coord + vec3 (-1./MAPW, 0., 0.0)))
            intensity += value;
                
        if (outshadow (coord + vec3 (1./MAPW, 1./MAPH, 0.0)))
            intensity += value;
        if (outshadow (coord + vec3 (1./MAPW, -1./MAPH, 0.0)))
            intensity += value;
        if (outshadow (coord + vec3 (-1./MAPW, 1./MAPH, 0.0)))
            intensity += value;
        if (outshadow (coord + vec3 (-1./MAPW, -1./MAPH, 0.0)))
            intensity += value;

        intensity = max (intensity, 0.0);
        return intensity;
    }
#else
    return 0.0;
#endif
}
#endif

#endif

#if SCE_DEFERRED_USE_SHADOWS
vec4 csm_getcoord (vec3 pos, out vec4 clr)
{
    vec4 coord;
    int i;
    for (i = 0; i < sce_csm_num_splits; i++) {
        coord = sce_light_viewproj[i] * vec4 (pos, 1.0);
        coord.xy /= coord.w;
        if (abs (coord.x) < 1.0 && abs (coord.y) < 1.0 && coord.z < 1.0)
            break;
    }

    coord.xy = coord.xy * 0.5 + vec2 (0.5);
    coord.x = (i + coord.x) / sce_csm_num_splits;
    coord.z -= 0.000028 * ((i * 3.0) + 1);
#if 1
    clr = vec4 (1.0);
#else
    if (i == 0)
        clr = vec4 (1.0, 0.4, 0.4, 0.0);
    else if (i == 1)
        clr = vec4 (0.4, 1.0, 0.4, 0.0);
    else if (i == 2)
        clr = vec4 (0.4, 0.4, 1.0, 0.0);
    else if (i == 3)
        clr = vec4 (1.0, 0.4, 1.0, 0.0);
    else if (i == 4)
        clr = vec4 (0.4, 1.0, 1.0, 0.0);
    else if (i == 5)
        clr = vec4 (1.0, 1.0, 0.4, 0.0);
    else if (i == 6)
        clr = vec4 (0.2, 0.7, 1.0, 0.0);
#endif

    return coord;
}
#endif

void main ()
{
    vec2 texcoord = position.xy / position.w;
    texcoord.xy = (texcoord.xy + vec2 (1.0)) * 0.5;

    vec4 lightcolor = vec4 (0.0);
    vec3 pos = sce_unpack_position (texcoord);
    float att = 1.0;
    vec4 clr = vec4 (1.0);

#if SCE_DEFERRED_USE_SHADOWS
    vec4 coord = csm_getcoord (pos, clr);
    att = shadow_intensity (coord.xyz);
    if (att > 0.0)
#endif
    {
        vec3 lightdir = sce_light_direction;
        vec3 normal = sce_unpack_normal (texcoord);

        float intensity = getintensity (normal, lightdir) * att;

        lightcolor = vec4 (sce_light_color * intensity, 0.0);

#if SCE_DEFERRED_USE_SPECULAR
        vec3 eyevec = normalize (pos);
        float specular = getspecular (normal, lightdir, eyevec);
        specular = pow (specular, 16.0);
        lightcolor.w = coeffspec (intensity) * specular;
#endif
    }
    
    gl_FragColor = lightcolor * clr;
}
