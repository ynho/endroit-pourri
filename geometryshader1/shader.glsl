[Vertex shader]

out vec3 normal;

void main (void)
{
  gl_FrontColor = vec4 (0.5, 0.0, 0.0, 0.0);
  normal = normalize (gl_NormalMatrix * gl_Normal);
  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}

[Geometry shader]

#define MODE 2

#if MODE == 1
layout (triangle_strip, max_vertices = 12) out;
#elif MODE == 2
layout (line_strip, max_vertices = 20) out;
#else
layout (line_strip, max_vertices = 8) out;
#endif

in vec3 normal[];
out vec3 nor;

void main (void)
{
  vec4 mid0 = (gl_PositionIn[0] + gl_PositionIn[1]) / vec4 (2.0);
  vec4 mid1 = (gl_PositionIn[1] + gl_PositionIn[2]) / vec4 (2.0);
  vec4 mid2 = (gl_PositionIn[2] + gl_PositionIn[0]) / vec4 (2.0);

#if 0                           /* default, no modification */
  gl_FrontColor = gl_FrontColorIn[0];
  nor = normal[0];
  gl_Position = gl_PositionIn[0];
  EmitVertex();

  nor = normal[1];
  gl_Position = gl_PositionIn[1];
  EmitVertex();

  nor = normal[2];
  gl_Position = gl_PositionIn[2];
  EmitVertex();

  EndPrimitive ();

#elif 1

  vec3 midn0 = normalize (normal[0] + normal[1]);
  vec3 midn1 = normalize (normal[1] + normal[2]);
  vec3 midn2 = normalize (normal[2] + normal[0]);

  /* triangle subdivided */
#if MODE == 1

  /* begin */
  gl_FrontColor = gl_FrontColorIn[0];
  nor = normal[0];
  gl_Position = gl_PositionIn[0];
  EmitVertex();

  nor = midn2;
  gl_Position = mid2;
  EmitVertex();

  nor = midn0;
  gl_Position = mid0;
  EmitVertex();

  nor = midn1;
  gl_Position = mid1;
  EmitVertex();

  nor = normal[1];
  gl_Position = gl_PositionIn[1];
  EmitVertex();

  EndPrimitive();

  /* begin */
  nor = midn1;
  gl_Position = mid1;
  EmitVertex();

  nor = midn2;
  gl_Position = mid2;
  EmitVertex();

  nor = normal[2];
  gl_Position = gl_PositionIn[2];
  EmitVertex();

  EndPrimitive();

  /* wire frame subdivided */
#else
  /* begin */
  gl_FrontColor = gl_FrontColorIn[0];
  nor = normal[0];
  gl_Position = gl_PositionIn[0];
  EmitVertex();

  nor = normal[1];
  gl_Position = gl_PositionIn[1];
  EmitVertex();

  nor = normal[2];
  gl_Position = gl_PositionIn[2];
  EmitVertex();

  nor = normal[0];
  gl_Position = gl_PositionIn[0];
  EmitVertex();

  EndPrimitive();

  /* begin */
  nor = midn0;
  gl_Position = mid0;
  EmitVertex();

  nor = midn1;
  gl_Position = mid1;
  EmitVertex();

  nor = midn2;
  gl_Position = mid2;
  EmitVertex();

  nor = midn0;
  gl_Position = mid0;
  EmitVertex();

  EndPrimitive();

  /* draw normals */
#if MODE == 2
  vec3 coef = vec3 (0.02);

  /* always point to light */
  nor = vec3 (1.0, 1.0, 0.0);
  gl_FrontColor = vec4 (0.2, 0.2, 1.0, 0.0);

  gl_Position = mid0;
  EmitVertex();
  gl_Position = mid0 + vec4 (midn0 * coef, 0.0);
  EmitVertex();
  EndPrimitive ();

  gl_Position = mid1;
  EmitVertex();
  gl_Position = mid1 + vec4 (midn1 * coef, 0.0);
  EmitVertex();
  EndPrimitive ();

  gl_Position = mid2;
  EmitVertex();
  gl_Position = mid2 + vec4 (midn2 * coef, 0.0);
  EmitVertex();
  EndPrimitive ();

  gl_Position = gl_PositionIn[0];
  EmitVertex();
  gl_Position = gl_PositionIn[0] + vec4 (normal[0] * coef, 0.0);
  EmitVertex();
  EndPrimitive ();

  gl_Position = gl_PositionIn[1];
  EmitVertex();
  gl_Position = gl_PositionIn[1] + vec4 (normal[1] * coef, 0.0);
  EmitVertex();
  EndPrimitive ();

  gl_Position = gl_PositionIn[2];
  EmitVertex();
  gl_Position = gl_PositionIn[2] + vec4 (normal[2] * coef, 0.0);
  EmitVertex();
  EndPrimitive ();

#endif

#endif  /* TRI */
#endif  /* simple */
}

[Pixel shader]

in vec3 nor;

void main (void)
{
  gl_FragColor = gl_Color * clamp (dot (normalize (nor),
                                        normalize (vec3 (1.0, 1.0, 0.3))), 0.0, 1.0);
}

