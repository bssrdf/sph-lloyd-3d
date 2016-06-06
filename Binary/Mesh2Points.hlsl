//--------------------------------------------------------------------------------------
// File: Mesh2Points.hlsl
//
// These shaders implement the PN-Triangles tessellation technique
//

cbuffer cbPNTriangles : register( b0 )
{
    float4x4    g_f4x4World;                // World matrix for object
    float4x4    g_f4x4ViewProjection;       // View * Projection matrix
    float4x4    g_f4x4WorldViewProjection;  // World * View * Projection matrix
    float4      g_f4LightDir;               // Light direction vector
    float4      g_f4Eye;                    // Eye
    float4      g_f4TessFactors;            // Tessellation factors ( x=Edge, y=Inside, z=MinDistance, w=Range )
	float4		g_fBoundBoxMin;
	float4		g_fBoundBoxMax;
	float4		g_fBoundSize;
	float4		g_fInvBoundSize;
	float4		g_fVolSlicing;
	float4		g_fParticleParameter;
	float4		g_fGridDim;
	float4		g_fInvGridDim;
	uint4		g_iGridDot;
	float4		g_fKernel;
}

// Some global lighting constants
static float4 g_f4MaterialDiffuseColor  = float4( 1.0f, 1.0f, 1.0f, 1.0f );
static float4 g_f4LightDiffuse          = float4( 1.0f, 1.0f, 1.0f, 1.0f );
static float4 g_f4MaterialAmbientColor  = float4( 0.2f, 0.2f, 0.2f, 1.0f );

// Some global epsilons for adaptive tessellation
static float g_fMaxScreenWidth = 2560.0f;
static float g_fMaxScreenHeight = 1600.0f;


//--------------------------------------------------------------------------------------
// Buffers, Textures and Samplers
//--------------------------------------------------------------------------------------

// Textures
Texture2D<float4> g_txDiffuse : register( t0 );
Texture2DArray<float4> DensityFieldROProxy : register( t1 );
Texture3D<float4> DensityFieldRO  : register( t2 );
RWTexture3D<float4> DensityFieldRW : register(u0);

RWStructuredBuffer<float4> ParticlesRW : register( u0 );
RWStructuredBuffer<float> ParticlesDensityRW : register( u0 );
StructuredBuffer<float4> ParticlesRO : register( t3 );
StructuredBuffer<float> ParticlesForceRO : register( t4 );

RWBuffer<uint> GridRW : register( u0 );
Buffer<uint2> GridRO : register( t5 );

RWStructuredBuffer<uint2> GridIndicesRW : register( u0 );
StructuredBuffer<uint2> GridIndicesRO : register( t6 );

// Samplers
SamplerState g_SamplePoint  : register( s0 );
SamplerState g_SampleLinear : register( s1 );


//--------------------------------------------------------------------------------------
// Shader structures
//--------------------------------------------------------------------------------------

struct VS_RenderSceneInput
{
    float3 f3Position   : POSITION;  
    float3 f3Normal     : NORMAL;     
};

struct HS_Input
{
    float3 f3Position   : POSITION;
    float3 f3Normal     : NORMAL;
};

struct HS_ConstantOutput
{
    // Tess factor for the FF HW block
    float fTessFactor[3]    : SV_TessFactor;
    float fInsideTessFactor : SV_InsideTessFactor;
    
    // Geometry cubic generated control points
    float3 f3B0    : POSITION3;
    float3 f3B1    : POSITION4;
    float3 f3B2    : POSITION5;
    
    // Normal quadratic generated control points
    float3 f3N0    : NORMAL3;      
    float3 f3N1    : NORMAL4;
    float3 f3N2    : NORMAL5;
};

struct HS_ControlPointOutput
{
    float3 f3Position    : POSITION;
    float3 f3Normal      : NORMAL;
};

struct DS_Output
{
    float4 f4Position   : SV_Position;
	float3 f3Normal		: TEXCOORD0;
	float3 f3Position	: TEXCOORD1;

};

struct PS_RenderSceneInput
{
    float4 f4Position   : SV_Position;
	float3 f3Normal		: TEXCOORD0;
};

struct PS_RenderOutput
{
    float4 f4Color      : SV_Target0;
};

struct VSParticleOut
{
    float3 position : POSITION;
    float4 color : COLOR;
};

struct GSParticleOut
{
    float4 position : SV_Position;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
};

//--------------------------------------------------------------------------------------
// This vertex shader computes standard transform and lighting, with no tessellation stages following
//--------------------------------------------------------------------------------------
PS_RenderSceneInput VS_RenderScene( VS_RenderSceneInput I )
{
    PS_RenderSceneInput O;
    float3 f3NormalWorldSpace;
    
    // Transform the position from object space to homogeneous projection space
    O.f4Position = mul( float4( I.f3Position, 1.0f ), g_f4x4WorldViewProjection );
    
    // Transform the normal from object space to world space    
    f3NormalWorldSpace = normalize( mul( I.f3Normal, (float3x3)g_f4x4World ) );
    
    // Calc diffuse color    
    O.f3Normal = f3NormalWorldSpace;  
    
    return O;    
}


//--------------------------------------------------------------------------------------
// This vertex shader is a pass through stage, with HS, tessellation, and DS stages following
//--------------------------------------------------------------------------------------
HS_Input VS_RenderSceneWithTessellation( VS_RenderSceneInput I )
{
    HS_Input O;
    
    // Pass through world space position
    O.f3Position = I.f3Position;
    
    // Pass through normalized world space normal    
    O.f3Normal = I.f3Normal;
        
    
    return O;    
}


//--------------------------------------------------------------------------------------
// This hull shader passes the tessellation factors through to the HW tessellator, 
// and the 10 (geometry), 6 (normal) control points of the PN-triangular patch to the domain shader
//--------------------------------------------------------------------------------------
HS_ConstantOutput HS_PNTrianglesConstant( InputPatch<HS_Input, 3> I )
{
    HS_ConstantOutput O = (HS_ConstantOutput)0;
 
    // Skip the rest of the function if culling
 
    // Now setup the PNTriangle control points...
    // Store positions
    O.f3B0 = I[0].f3Position;
    O.f3B1 = I[1].f3Position;
    O.f3B2 = I[2].f3Position;
    
    O.f3N0 = I[0].f3Normal;
    O.f3N1 = I[1].f3Normal;
    O.f3N2 = I[2].f3Normal;

	float3 ls = max(1.0f, float3(
	length(O.f3B0 - O.f3B1), 
	length(O.f3B2 - O.f3B1),
	length(O.f3B0 - O.f3B2)
	) * g_fInvBoundSize.w / g_fBoundSize.w * 3.0f * g_f4TessFactors.x);

	O.fTessFactor[0] = ls.x;
	O.fTessFactor[1] = ls.y;
	O.fTessFactor[2] = ls.z;

    // Inside tess factor is just he average of the edge factors
    O.fInsideTessFactor = max( O.fTessFactor[0], max(O.fTessFactor[1], O.fTessFactor[2] ) );
               
    return O;
}

[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[patchconstantfunc("HS_PNTrianglesConstant")]
[outputcontrolpoints(3)]
[maxtessfactor(9)]
HS_ControlPointOutput HS_PNTriangles( InputPatch<HS_Input, 3> I, uint uCPID : SV_OutputControlPointID )
{
    HS_ControlPointOutput O = (HS_ControlPointOutput)0;

    // Just pass through inputs = fast pass through mode triggered
    O.f3Position = I[uCPID].f3Position;
    O.f3Normal = I[uCPID].f3Normal;
    
    return O;
}

float3 project(float3 p, float3 c, float3 n)
{
    return p - dot(p - c, n) * n;
}

// Computes the position of a point in the Phong Tessellated triangle
float3 PhongGeometry(float u, float v, float w, HS_ConstantOutput hsc)
{
    // Find local space point
    float3 p = w * hsc.f3B0 + u * hsc.f3B1 + v * hsc.f3B2;
    // Find projected vectors
    float3 c0 = project(p, hsc.f3B0, hsc.f3N0);
    float3 c1 = project(p, hsc.f3B1, hsc.f3N1);
    float3 c2 = project(p, hsc.f3B2, hsc.f3N2);
    // Interpolate
    float3 q = w * c0 + u * c1 + v * c2;
    // For blending between tessellated and untessellated model:
    //float3 r = LERP(p, q, alpha);
    return q;
}

// Computes the normal of a point in the Phong Tessellated triangle
float3 PhongNormal(float u, float v, float w, HS_ConstantOutput hsc)
{
    // Interpolate
    return normalize(w * hsc.f3N0 + u * hsc.f3N1 + v * hsc.f3N2);
}

//--------------------------------------------------------------------------------------
// This domain shader applies contol point weighting to the barycentric coords produced by the FF tessellator 
//--------------------------------------------------------------------------------------
[domain("tri")]
DS_Output DS_PNTriangles( HS_ConstantOutput HSConstantData, const OutputPatch<HS_ControlPointOutput, 3> I, float3 f3BarycentricCoords : SV_DomainLocation )
{
    DS_Output O = (DS_Output)0;

    // The barycentric coordinates
    float fU = f3BarycentricCoords.x;
    float fV = f3BarycentricCoords.y;
    float fW = f3BarycentricCoords.z;
    
    // Compute position
    float3 f3Position = PhongGeometry(fU, fV, fW, HSConstantData);
    
    // Compute normal
    float3 f3Normal =   PhongNormal(fU, fV, fW, HSConstantData);

    // Transform model position with view-projection matrix
    O.f4Position = mul( float4( f3Position.xyz, 1.0 ), g_f4x4ViewProjection );
	O.f3Position = f3Position.xyz;
	O.f3Normal = f3Normal;
        
    return O;
}

struct SO_Output_Photon3D
{
	float4 f4Position   : SV_Position;
	float3 f3Position	: TEXCOORD0;
	float3 f3Center		: TEXCOORD1;
	float3 f3Normal		: TEXCOORD2;
	uint iZ				: SV_RenderTargetArrayIndex;
};

const static float2 QuadOffset[6] =
{
	{1.0f,   1.0f}, 
	{1.0f,  -1.0f},
	{-1.0f, -1.0f},
	{1.0f,   1.0f}, 
	{-1.0f, -1.0f},
	{-1.0f,  1.0f}
};

[maxvertexcount(30)]
void TriField3DGS (triangle DS_Output input[3], inout TriangleStream<SO_Output_Photon3D> pStream )
{
	SO_Output_Photon3D O;

	float3 vPos = (input[0].f3Position + input[1].f3Position + input[2].f3Position) * 0.33333333f;

	float3 vPosLocal = (vPos - g_fBoundBoxMin.xyz) * g_fInvBoundSize.xyz;

	float2 Scr_pos = vPosLocal.xy - 1.0f;
	Scr_pos.y = -Scr_pos.y;

	float z_pos = vPosLocal.z;
	
	O.f4Position = float4(Scr_pos, 0.5f, 1.0f);
	O.f3Normal = (input[0].f3Normal + input[1].f3Normal + input[2].f3Normal) * 0.33333333f;
	O.f3Center = vPos;
	int iZBase = (uint)(z_pos + 0.5f);

	int w, h, d;
	DensityFieldRO.GetDimensions(w, h, d);
	d--;
	
	for(int i = -2; i <= 2; i++) {
		for(int j = 0; j < 2; j++) {
			for(int k = 0; k < 3; k++) {
				float2 Scr_offset = QuadOffset[j * 3 + k] * g_fBoundSize.w * 2.0f;
				float3 vOffset = float3(Scr_offset, i) * g_fBoundSize.xyz;

				O.f4Position.xy = Scr_pos + Scr_offset;
				O.f3Position = vPos + vOffset;
				O.iZ = min(d, max(0, iZBase + i));
				pStream.Append(O);
			}
			pStream.RestartStrip();
		}
	}
}

struct PS_FieldOutput
{
	float4 norm : SV_Target;
	float dist : SV_Depth;
};

PS_FieldOutput TriField3DPS (SO_Output_Photon3D input)
{
	PS_FieldOutput o;
	o.norm = float4(input.f3Normal, distance(input.f3Position, input.f3Center));
	o.dist = o.norm.w * g_fInvBoundSize.w;

	return o;
}
//--------------------------------------------------------------------------------------
// This shader outputs the pixel's color by passing through the lit 
// diffuse material color & modulating with the diffuse texture
//--------------------------------------------------------------------------------------
PS_RenderOutput PS_RenderSceneTextured( PS_RenderSceneInput I )
{
    PS_RenderOutput O;
    
    O.f4Color = saturate(dot(g_f4LightDir.xyz, I.f3Normal)) * 0.85f + 0.15f;
    
    return O;
}


//--------------------------------------------------------------------------------------
// This shader outputs the pixel's color by passing through the lit 
// diffuse material color
//--------------------------------------------------------------------------------------
PS_RenderOutput PS_RenderScene( PS_RenderSceneInput I )
{
    PS_RenderOutput O;
    
	O.f4Color = saturate(dot(g_f4LightDir.xyz, I.f3Normal)) * 0.85f + 0.15f;
     
    return O;
}


struct VVSOutput {
	float4 Pos  : SV_Position;
	float2 tex	: TEXCOORD0;
};

VVSOutput VisualizeVS(uint ID : SV_VertexID)
{
	VVSOutput o;
	o.Pos = float4(QuadOffset[ID], 0.5f, 1.0f);
	o.tex = o.Pos.xy * float2(0.5f, -0.5f) + 0.5f;
	return o;
}

float4 VisualizePS(VVSOutput input) : SV_Target
{
	float2 tex = input.tex;
	float4 fVS = g_fVolSlicing;

	float2 utex = tex / fVS.zw;
	// Which Piece in Vol
	float2 wp = tex * fVS.xy;
	uint2 iwp = (uint2) wp;

	// i = piece in Vol
	uint i = iwp.y * (uint) fVS.x + iwp.x;

	uint w,h,n;
	DensityFieldRO.GetDimensions(w,h,n);

	// Which pos in piece
	float2 fp = frac(wp);
	float fpz = (float)i / (float)n;

	clip(1.0f - fpz);

	return DensityFieldRO.Sample(g_SampleLinear, float3(fp, fpz)).w * 0.05f;

	//return DensityFieldROProxy.Sample(g_SampleLinear, float3(fp, fpz)) * 0.5f + 0.5f;
}


//--------------------------------------------------------------------------------------
// Visualization Helper
//--------------------------------------------------------------------------------------

static const float4 Rainbow[5] = {
    float4(1, 0, 0, 1), // red
    float4(1, 1, 0, 1), // orange
    float4(0, 1, 0, 1), // green
    float4(0, 1, 1, 1), // teal
    float4(0, 0, 1, 1), // blue
};

float4 VisualizeNumber(float n)
{
    return lerp( Rainbow[ floor(n * 4.0f) ], Rainbow[ ceil(n * 4.0f) ], frac(n * 4.0f) );
}

float4 VisualizeNumber(float n, float lower, float upper)
{
    return VisualizeNumber( saturate( (n - lower) / (upper - lower) ) );
}

VSParticleOut ParticleVS(uint ID : SV_VertexID)
{
    VSParticleOut Out = (VSParticleOut)0;
    Out.position = ParticlesRO[ID].xyz;
    Out.color = VisualizeNumber(ParticlesForceRO[ID], 0.0f, 0.05f);
	if(ParticlesRO[ID].w > g_fParticleParameter.w)
		Out.color = 1.0 - Out.color;
    return Out;
}

static const float2 g_positions[4] = { float2(-1, 1), float2(1, 1), float2(-1, -1), float2(1, -1) };
static const float2 g_texcoords[4] = { float2(0, 1), float2(1, 1), float2(0, 0), float2(1, 0) };

[maxvertexcount(4)]
void ParticleGS(point VSParticleOut In[1], inout TriangleStream<GSParticleOut> SpriteStream)
{
    float4 position = mul(float4(In[0].position, 1.0f), g_f4x4ViewProjection);
    GSParticleOut Out = (GSParticleOut)0;
    Out.color = In[0].color;

    [unroll]
    for (int i = 0; i < 4; i++)
    {
		Out.position = position + g_fParticleParameter.x * float4(g_positions[i] * float2(1.0f, g_fParticleParameter.y), 0, 0);
        Out.texcoord = g_texcoords[i];
        SpriteStream.Append(Out);
    }
    SpriteStream.RestartStrip();
}


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

float4 ParticlePS(GSParticleOut In) : SV_Target
{
	float2 tx = In.texcoord * 2.0f - 1.0f;
	float dis = length(tx);
	float w = dis < 0.85? 1.0f : (exp(-80.0f * (dis - 0.85f) * (dis - 0.85f)));
    return float4(In.color.xyz, w * 0.5f);
}

#define SIMULATION_BLOCK_SIZE 512

float3 UnitPos(float3 position) 
{
	return (position - g_fBoundBoxMin.xyz) * g_fInvGridDim.xyz;
}

float3 GridCalculateCell(float3 position)
{
    return clamp(UnitPos(position) * g_fGridDim.xyz, float3(0, 0, 0), g_fGridDim.xyz - 1);
}

unsigned int GridConstuctKey(uint3 xyz)
{
	// Bit pack [----Y---][----Z---][----X---]
	//              8-bit		8-bit		8-bit
    return dot(xyz.yzx, uint3(g_iGridDot.xy, 1));
}

uint2 GridConstuctKeyValuePair(uint3 xyz, uint value)
{
    // Bit pack [----Z---][----Y---][----X---][-------------VALUE--------------]
    //             8-bit		8-bit	   8-bit				   32-bit
    return uint2(GridConstuctKey(xyz), value);
}

uint GridGetKey(uint2 keyvaluepair)
{
    return keyvaluepair.y;
}

uint GridGetValue(uint2 keyvaluepair)
{
    return keyvaluepair.x;
}
//--------------------------------------------------------------------------------------
// Build Grid
//--------------------------------------------------------------------------------------
[numthreads(SIMULATION_BLOCK_SIZE, 1, 1)]
void BuildGridCS( uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex )
{
    const unsigned int P_ID = DTid.x; // Particle ID to operate on
    
    float3 position = ParticlesRO[P_ID].xyz;
	
    int3 grid_xyz = (int3) GridCalculateCell( position );
    
    uint2 result = GridConstuctKeyValuePair((uint3)grid_xyz, P_ID);

	GridRW[P_ID * 2] = result.y;
	GridRW[P_ID * 2 + 1] = result.x;
}


//--------------------------------------------------------------------------------------
// Build Grid Indices
//--------------------------------------------------------------------------------------

[numthreads(SIMULATION_BLOCK_SIZE, 1, 1)]
void BuildGridIndicesCS( uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex )
{
	const unsigned int g_iNumParticles = g_iGridDot.w;
    const unsigned int G_ID = DTid.x; // Grid ID to operate on
    unsigned int G_ID_PREV = (G_ID == 0)? g_iNumParticles : G_ID; G_ID_PREV--;
    unsigned int G_ID_NEXT = G_ID + 1; if (G_ID_NEXT == g_iNumParticles) { G_ID_NEXT = 0; }
    
    unsigned int cell = GridGetKey( GridRO[G_ID] );
    unsigned int cell_prev = GridGetKey( GridRO[G_ID_PREV] );
    unsigned int cell_next = GridGetKey( GridRO[G_ID_NEXT] );
    if (cell != cell_prev)
    {
        // I'm the start of a cell
        GridIndicesRW[cell].x = G_ID;
    }
    if (cell != cell_next)
    {
        // I'm the end of a cell
        GridIndicesRW[cell].y = G_ID + 1;
    }
}


//--------------------------------------------------------------------------------------
// Rearrange Particles
//--------------------------------------------------------------------------------------

[numthreads(SIMULATION_BLOCK_SIZE, 1, 1)]
void RearrangeParticlesCS( uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex )
{
    const unsigned int ID = DTid.x; // Particle ID to operate on
    const unsigned int G_ID = GridGetValue( GridRO[ ID ] );
    ParticlesRW[ID] = ParticlesRO[ G_ID ];
}

float4 CalculateForce(float3 p)
{
	float r = length(p);
	float3 n = p;
	return float4(-n, 1.0f) * exp(-r * r / g_fKernel.x);
}

[numthreads(SIMULATION_BLOCK_SIZE, 1, 1)]
void VelocityCS( uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex )
{
    const unsigned int P_ID = DTid.x;
    const float h_sq = g_fKernel.x;
    float3 P_position = ParticlesRO[P_ID];

	float4 velocity = 0;

    // Calculate the density based on neighbors from the 8 adjacent cells + current cell
    int3 G_XY = GridCalculateCell( P_position );
	
	for (int Z = max(G_XY.z - 1, 0) ; Z <= min(G_XY.z + 1, g_fGridDim.z-1) ; Z++)
	{
		for (int Y = max(G_XY.y - 1, 0) ; Y <= min(G_XY.y + 1, g_fGridDim.y-1) ; Y++)
		{
			for (int X = max(G_XY.x - 1, 0) ; X <= min(G_XY.x + 1, g_fGridDim.x-1) ; X++)
			{
				unsigned int G_CELL = GridConstuctKey(uint3(X, Y, Z));
				uint2 G_START_END = GridIndicesRO[G_CELL];
				for (unsigned int N_ID = G_START_END.x ; N_ID < G_START_END.y ; N_ID++)
				{
					float3 N_position = ParticlesRO[N_ID];

					float3 diff = N_position - P_position;
					float r_sq = dot(diff, diff);

					velocity += CalculateForce(diff);
				}
			}
		}
	}

	if(velocity.w > 0)
		velocity.xyz /= velocity.w;

	float4 dist = DensityFieldRO.SampleLevel(g_SampleLinear, UnitPos(P_position), 0);
	float3 norm = normalize(dist.xyz) * g_fParticleParameter.z;
	float vn = dot(velocity.xyz, norm);
	float dl = length(dist.xyz);
	bool valid = dl > g_fParticleParameter.w && vn > 0;
	if(valid) {
		float3 proj = vn * norm;
		float3 tang = velocity.xyz - proj;

		float w = (exp(-dist.w * dist.w / g_fKernel.x) - 0.5f) * g_fKernel.z;
		velocity.xyz = tang - norm * w;
	} 

	ParticlesRW[P_ID] = float4(max(g_fBoundBoxMin, min(g_fBoundBoxMax, P_position + velocity.xyz * g_fKernel.y)), dl);
}

float CalculateDensity(float r_sq)
{
    const float h_sq = g_fKernel.x;
    // Implements this equation:
    // W_poly6(r, h) = 315 / (64 * pi * h^9) * (h^2 - r^2)^3
    // g_fDensityCoef = fParticleMass * 315.0f / (64.0f * PI * fSmoothlen^9)
    return g_fKernel.w * (h_sq - r_sq) * (h_sq - r_sq) * (h_sq - r_sq);
}

[numthreads(SIMULATION_BLOCK_SIZE, 1, 1)]
void DensityCS( uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex )
{
    const unsigned int P_ID = DTid.x;
    const float h_sq = g_fKernel.x;
    float3 P_position = ParticlesRO[P_ID].xyz;
   
	float density = 0;

    // Calculate the density based on neighbors from the 8 adjacent cells + current cell
    int3 G_XY = GridCalculateCell( P_position );
	
	for (int Z = max(G_XY.z - 1, 0) ; Z <= min(G_XY.z + 1, g_fGridDim.z-1) ; Z++)
	{
		for (int Y = max(G_XY.y - 1, 0) ; Y <= min(G_XY.y + 1, g_fGridDim.y-1) ; Y++)
		{
			for (int X = max(G_XY.x - 1, 0) ; X <= min(G_XY.x + 1, g_fGridDim.x-1) ; X++)
			{
				unsigned int G_CELL = GridConstuctKey(uint3(X, Y, Z));
				uint2 G_START_END = GridIndicesRO[G_CELL];
				for (unsigned int N_ID = G_START_END.x ; N_ID < G_START_END.y ; N_ID++)
				{
					float3 N_position = ParticlesRO[N_ID].xyz;

					float3 diff = N_position - P_position;
					float r_sq = dot(diff, diff);

					if (r_sq < h_sq)
					{
						density += CalculateDensity(r_sq);
					}
				}
			}
		}
	}

	ParticlesDensityRW[P_ID] = density;
}


[numthreads(16, 16, 1)]
void ArrayTo3DCS( uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex )
{
	DensityFieldRW[DTid] = DensityFieldROProxy[DTid];
}

//--------------------------------------------------------------------------------------
// EOF
//--------------------------------------------------------------------------------------
