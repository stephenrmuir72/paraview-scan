//VTK::System::Dec

//VTK::PositionVC::Dec
in vec4 vertexMCVSOutput[];
in vec3 vertexMCVSUnscaled[];

//VTK::TCoord::Dec

out vec3 normalVCGSOutput;

uniform mat4 MCDCMatrix;
uniform mat3 normalMatrix;
uniform vec3 vertexScaleMC;

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;
void main()
{
  vec3 d1MC = (vertexMCVSUnscaled[1] - vertexMCVSUnscaled[0]);
  vec3 d2MC = (vertexMCVSUnscaled[2] - vertexMCVSUnscaled[0]);

  vec3 normalMC = normalize(cross(d1MC, d2MC));
  vec3 normalVC = normalMatrix * normalMC;

  for (int i = 0; i < 3; i++)
  {
    //VTK::PrimID::Impl
    //VTK::TCoord::Impl
    //VTK::PositionVC::Impl
    normalVCGSOutput = normalVC;
    gl_Position = MCDCMatrix * vertexMCVSOutput[i];
    EmitVertex();
  }
  EndPrimitive();
}
