[vertex shader]

varying vec4 position;

void main ()
{
    position = gl_ModelViewProjectionMatrix * gl_Vertex;
    gl_Position = ftransform ();
}

[pixel shader]

varying vec4 position;

float getintensity (vec3 normal, vec3 lightvec)
{
    return max (dot (normal, -lightvec), 0.0) * 0.7;
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

float getattenuation (float d, float r)
{
    return max (1.0 - pow ((d / r), 8.), 0.0);
}

float spot_att (vec3 light_to_point, vec3 lightdir)
{
  float d = dot (light_to_point, lightdir);
  float angle = sce_light_angle;
  d = (d - angle) / (1.0 - angle);
  return clamp (1.0 - exp (- (d * sce_light_attenuation)), 0.0, 1.0);
}

#if SCE_DEFERRED_USE_SHADOWS
bool outshadow (vec3 coord)
{
    float a = texture2D (sce_shadow_map, coord.xy).x;
    return (a > coord.z);
}

float shadow_intensity (vec3 coord)
{
    if (outshadow (coord))
        return 1.0;
#undef SCE_DEFERRED_USE_SOFT_SHADOWS
#define SCE_DEFERRED_USE_SOFT_SHADOWS 1
#if SCE_DEFERRED_USE_SOFT_SHADOWS
    else {
#define W 512
#define H 512
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

void main ()
{
    vec2 texcoord = position.xy / position.w;
    texcoord.xy = (texcoord.xy + vec2 (1.0)) * 0.5;

    vec4 lightcolor = vec4 (0.0);
    vec3 pos = sce_unpack_position (texcoord);
    float lightradius = sce_light_radius;
    vec3 lightpos = sce_light_position;
    vec3 lightdir = sce_light_direction;
    float dist = distance (pos, lightpos);
    float att = getattenuation (dist, lightradius);
    float intensity = att;

#if SCE_DEFERRED_USE_SHADOWS
    vec4 coord = sce_light_viewproj * vec4 (pos, 1.0);
    coord.xyz /= coord.w;
    coord.xyz = coord.xyz * 0.5 + vec3 (0.5);
    coord.z -= 0.00001;
    att *= shadow_intensity (coord.xyz);
    if (att > 0.0)
#endif
    {
        vec3 normal = sce_unpack_normal (texcoord);
        vec3 lightvec = normalize (pos - lightpos);

        /* specular must be affected by spot-angle attenuation */
        att *= spot_att (lightvec, lightdir);
        intensity = att;
        intensity *= getintensity (normal, lightvec);
        lightcolor = vec4 (sce_light_color * intensity, 0.0);

#if SCE_DEFERRED_USE_SPECULAR
        vec3 eyevec = normalize (pos);
        float specular = getspecular (normal, lightdir, eyevec);
        specular = pow (specular, 16.0);
        lightcolor.w = coeffspec (intensity) * specular;
#endif
    }
    
    gl_FragColor = lightcolor;
}
