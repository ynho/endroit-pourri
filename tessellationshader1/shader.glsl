[Vertex shader]

#version 410
#extension GL_EXT_gpu_shader4 : require

void main (void)
{
  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}

[Control shader]

#version 410
#extension GL_EXT_gpu_shader4 : require

layout (vertices = 3) out;

void main (void)
{
    float TessLevelInner = 2;
    float TessLevelOuter = 2;
    #define ID gl_InvocationID

    gl_out[ID].gl_Position = gl_in[ID].gl_Position;
    if (ID == 0) {
        gl_TessLevelInner[0] = TessLevelInner;
        gl_TessLevelInner[1] = TessLevelInner;
        gl_TessLevelOuter[0] = TessLevelOuter;
        gl_TessLevelOuter[1] = TessLevelOuter;
        gl_TessLevelOuter[2] = TessLevelOuter;
        gl_TessLevelOuter[3] = TessLevelOuter;
    }
}

[Evaluation shader]

#version 410
#extension GL_EXT_gpu_shader4 : require

//layout(triangles, equal_spacing, cw) in;
layout(triangles, fractional_odd_spacing, cw) in;
//layout(quads, fractional_odd_spacing, cw) in;

void main (void)
{
#if 1
    vec4 p0 = gl_TessCoord.x * gl_in[0].gl_Position;
    vec4 p1 = gl_TessCoord.y * gl_in[1].gl_Position;
    vec4 p2 = gl_TessCoord.z * gl_in[2].gl_Position;
    gl_Position = p0 + p1 + p2;
#else
    vec4 p0 = mix (gl_in[1].gl_Position, gl_in[0].gl_Position, gl_TessCoord.x);
    vec4 p1 = mix (gl_in[2].gl_Position, gl_in[3].gl_Position, gl_TessCoord.x);
    gl_Position = mix (p0, p1, gl_TessCoord.y);
#endif
}

[Geometry shader]

#version 410
#extension GL_EXT_gpu_shader4 : require

layout (triangles) in;
layout (line_strip, max_vertices = 4) out;

void main (void)
{
  gl_Position = gl_PositionIn[0];
  EmitVertex();
  gl_Position = gl_PositionIn[1];
  EmitVertex();
  gl_Position = gl_PositionIn[2];
  EmitVertex();
  gl_Position = gl_PositionIn[0];
  EmitVertex();
  EndPrimitive ();
}

[Pixel shader]

#version 410
#extension GL_EXT_gpu_shader4 : require

void main (void)
{
  gl_FragColor = vec4 (0.0);
}
