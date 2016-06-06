//--------------------------------------------------------------------------------------
// File: Mesh2Points.cpp
//
// This code sample demonstrates the use of DX11 Hull & Domain shaders to implement the 
// PN-Triangles tessellation technique
//
// Contributed by the AMD Developer Relations Team
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTgui.h"
#include "DXUTsettingsdlg.h"
#include "SDKmisc.h"
#include "SDKMesh.h"
#include <D3DX11tex.h>
#include <D3DX11.h>
#include <D3DX11core.h>
#include <D3DX11async.h>
#include <fstream>
#include "TglMeshReader.h"
#include "resource.h"

// defines
#define FIELD_SIZE 128
const UINT NUM_PARTICLES_8K		 = 8 * 1024;
const UINT NUM_PARTICLES_16K		 = 16 * 1024;
const UINT NUM_PARTICLES_32K		 = 32 * 1024;
const UINT NUM_PARTICLES_64K		 = 64 * 1024;
const UINT NUM_PARTICLES_128K	 = 128 * 1024;
const UINT NUM_PARTICLES_256K	 = 256 * 1024;

const UINT num_particles_list[] =
{
	NUM_PARTICLES_8K,	
	NUM_PARTICLES_16K,	
	NUM_PARTICLES_32K,	
	NUM_PARTICLES_64K,	
	NUM_PARTICLES_128K,
	NUM_PARTICLES_256K
};

const WCHAR* num_particles_name_list[] =
{
	L"8K",	
	L"16K",	
	L"32K",	
	L"64K",	
	L"128K",
	L"256K"
};

UINT g_iNumParticles = NUM_PARTICLES_16K;

const UINT NUM_GRID_DIM_X = 32;
const UINT NUM_GRID_DIM_Y = 32;
const UINT NUM_GRID_DIM_Z = 32;
const UINT NUM_GRID_INDICES = NUM_GRID_DIM_X * NUM_GRID_DIM_Y * NUM_GRID_DIM_Z;

const UINT MY_TRAY_ICON_ID = 'M2CS';
// The different meshes
typedef enum _MESH_TYPE
{
    MESH_TYPE_TIGER      = 0,
    MESH_TYPE_MAX       = 1,
}MESH_TYPE;

CDXUTDialogResourceManager  g_DialogResourceManager;    // Manager for shared resources of dialogs
CModelViewerCamera          g_Camera[MESH_TYPE_MAX];    // A model viewing camera for each mesh scene
CModelViewerCamera          g_LightCamera;              // A model viewing camera for the light
CD3DSettingsDlg             g_D3DSettingsDlg;           // Device settings dialog
CDXUTDialog                 g_HUD;                      // Dialog for standard controls
CDXUTDialog                 g_SampleUI;                 // Dialog for sample specific controls
CDXUTTextHelper*            g_pTxtHelper = NULL;

// The scene meshes 
MeshObj                g_SceneMesh[MESH_TYPE_MAX];
static ID3D11InputLayout*   g_pSceneVertexLayout = NULL;
MESH_TYPE                   g_eMeshType = MESH_TYPE_TIGER;
D3DXMATRIX                  g_m4x4MeshMatrix[MESH_TYPE_MAX];
D3DXVECTOR3                 g_v3AdaptiveTessParams[MESH_TYPE_MAX];

// Samplers
ID3D11SamplerState*         g_pSamplePoint = NULL;
ID3D11SamplerState*         g_pSampleLinear = NULL;

// Shaders
ID3D11VertexShader*         g_pSceneVS = NULL;
ID3D11VertexShader*         g_pSceneWithTessellationVS = NULL;
ID3D11HullShader*           g_pPNTrianglesHS = NULL;
ID3D11DomainShader*         g_pPNTrianglesDS = NULL;
ID3D11PixelShader*          g_pScenePS = NULL;
ID3D11PixelShader*          g_pTexturedScenePS = NULL;
ID3D11GeometryShader*		g_pFieldGS = NULL;
ID3D11PixelShader*			g_pFieldPS = NULL;
ID3D11VertexShader*			g_pVisualizeVS = NULL;
ID3D11PixelShader*			g_pVisualizePS = NULL;
ID3D11VertexShader*         g_pParticleVS = NULL;
ID3D11GeometryShader*       g_pParticleGS = NULL;
ID3D11PixelShader*          g_pParticlePS = NULL;

ID3D11ComputeShader*		g_pSortBitonic = NULL;
ID3D11ComputeShader*		g_pSortTranspose = NULL;
ID3D11ComputeShader*		g_pSortBitonicUint = NULL;
ID3D11ComputeShader*		g_pSortTransposeUint = NULL;
ID3D11ComputeShader*		g_pBuildGridCS = NULL;
ID3D11ComputeShader*		g_pBuildGridIndicesCS = NULL;
ID3D11ComputeShader*		g_pRearrangeParticlesCS = NULL;
ID3D11ComputeShader*		g_pVelocityCS = NULL;
ID3D11ComputeShader*		g_pDensityCS = NULL;
ID3D11ComputeShader*		g_pArrayTo3DCS = NULL;

// Resources
ID3D11Texture3D*			g_pTexField = NULL;
ID3D11Texture2D*			g_pTexFieldDepth = NULL;
ID3D11Texture2D*			g_pTexFieldProxy = NULL;
ID3D11ShaderResourceView*	g_pSRVField = NULL;
ID3D11UnorderedAccessView*	g_pUAVField = NULL;
ID3D11ShaderResourceView*	g_pSRVFieldProxy = NULL;
ID3D11RenderTargetView*		g_pRTVFieldProxy = NULL;
ID3D11DepthStencilView*		g_pDSVField = NULL;


ID3D11Buffer*                       g_pParticles = NULL;
ID3D11ShaderResourceView*           g_pParticlesSRV = NULL;
ID3D11UnorderedAccessView*          g_pParticlesUAV = NULL;

ID3D11Buffer*                       g_pSortedParticles = NULL;
ID3D11ShaderResourceView*           g_pSortedParticlesSRV = NULL;
ID3D11UnorderedAccessView*          g_pSortedParticlesUAV = NULL;

ID3D11Buffer*                       g_pParticleDensity = NULL;
ID3D11ShaderResourceView*           g_pParticleDensitySRV = NULL;
ID3D11UnorderedAccessView*          g_pParticleDensityUAV = NULL;

ID3D11Buffer*                       g_pGrid = NULL;
ID3D11ShaderResourceView*           g_pGridSRV = NULL;
ID3D11UnorderedAccessView*          g_pGridUAV = NULL;

ID3D11Buffer*                       g_pGridPingPong = NULL;
ID3D11ShaderResourceView*           g_pGridPingPongSRV = NULL;
ID3D11UnorderedAccessView*          g_pGridPingPongUAV = NULL;

ID3D11Buffer*                       g_pGridIndices = NULL;
ID3D11ShaderResourceView*           g_pGridIndicesSRV = NULL;
ID3D11UnorderedAccessView*          g_pGridIndicesUAV = NULL;

// States
ID3D11BlendState*					g_pBSAlpha = NULL;

ID3D11Buffer*				g_pNullBuffer = NULL;
ID3D11ShaderResourceView*	g_pNullSRV = NULL;
ID3D11RenderTargetView*		g_pNullRTV = NULL;
ID3D11UnorderedAccessView*	g_pNullUAV = NULL;	
UINT						g_iNullUINT = NULL;
FLOAT						g_fSpeed = 1.0f;
FLOAT						g_fKScale = 2.0f;
FLOAT						g_fSurface = 0.05f;

WCHAR g_default_mesh_fn[MAX_PATH] = L"tiger\\cow32k.obj";

// Constant buffer layout for transfering data to the PN-Triangles HLSL functions
struct CB_PNTRIANGLES
{
    D3DXMATRIX f4x4World;               // World matrix for object
    D3DXMATRIX f4x4ViewProjection;      // View * Projection matrix
    D3DXMATRIX f4x4WorldViewProjection; // World * View * Projection matrix  
    float fLightDir[4];                 // Light direction vector
    float fEye[4];                      // Eye
    float fTessFactors[4];              // Tessellation factors ( x=Edge, y=Inside, z=MinDistance, w=Range )
	float fBoundBoxMin[4];
	float fBoundBoxMax[4];
	float fBoundSize[4];
	float fInvBoundSize[4];
	float fVolSlicing[4];
	float fParticleParameter[4];
	float fGridDim[4];
	float fInvGridDim[4];
	UINT  iGridDot[4];
	float fKernel[4];
};
UINT                    g_iPNTRIANGLESCBBind = 0;

#define _DECLSPEC_ALIGN_16_ __declspec(align(16))
_DECLSPEC_ALIGN_16_ struct SortCB
{
    UINT iLevel;
    UINT iLevelMask;
    UINT iWidth;
    UINT iHeight;
};

// Various Constant buffers
static ID3D11Buffer*    g_pcbPNTriangles = NULL;                 
static ID3D11Buffer*    g_pSortCB = NULL;       

// State objects
ID3D11RasterizerState*  g_pRasterizerStateWireframe = NULL;
ID3D11RasterizerState*  g_pRasterizerStateSolid = NULL;

// Capture texture
static ID3D11Texture2D*    g_pCaptureTexture = NULL;

// User supplied data
static bool g_bUserMesh = false;
static ID3D11ShaderResourceView* g_pDiffuseTextureSRV = NULL;

// Tess factor
static unsigned int g_uTessFactor = 1.0f;

// Back face culling epsilon
static float g_fBackFaceCullEpsilon = 0.5f;

// Silhoutte epsilon
static float g_fSilhoutteEpsilon = 0.25f;

// Range scale (for distance adaptive tessellation)
static float g_fRangeScale = 1.0f;

// Edge scale (for screen space adaptive tessellation)
static unsigned int g_uEdgeSize = 16; 

// Edge scale (for screen space adaptive tessellation)
static float g_fResolutionScale = 1.0f; 

// View frustum culling epsilon
static float g_fViewFrustumCullEpsilon = 0.5f;

static float g_fParticleRenderSize = 0.0125f;

static float g_fParticleAspectRatio = 1.0f;

static float g_fSmoothlen = 0.012f;

static float g_fMaxAllowableTimeStep = 0.0075f;

static float g_fParticleMass = 0.0002f;

static float g_fRestDensity = 1000.0f;

static float g_fNormalScalar = 1.0f;

static D3DXVECTOR3 g_vInitOffset = D3DXVECTOR3(0,0,0);

static UINT g_iFieldSize[3] = {FIELD_SIZE, FIELD_SIZE, FIELD_SIZE};

BOOL g_bFieldUpdated = FALSE;
BOOL g_bSavePoints = FALSE;
BOOL g_bSaveSurfacePoints = FALSE;
BOOL g_bNoSimulating = FALSE;

// Cmd line params
typedef struct _CmdLineParams
{
    unsigned int uWidth;
    unsigned int uHeight;
    bool bCapture;
    WCHAR strCaptureFilename[256];
    int iExitFrame;
    bool bRenderHUD;
}CmdLineParams;
static CmdLineParams g_CmdLineParams;


//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------

// Standard device control
#define IDC_TOGGLEFULLSCREEN                    1
#define IDC_TOGGLEREF                           3
#define IDC_CHANGEDEVICE                        4

// Sample UI
#define IDC_STATIC_MESH                         5
#define IDC_COMBOBOX_MESH                       6
#define IDC_CHECKBOX_WIREFRAME                  7
#define IDC_CHECKBOX_TEXTURED                   8
#define IDC_CHECKBOX_TESSELLATION               9
#define IDC_CHECKBOX_DISTANCE_ADAPTIVE          10
#define IDC_CHECKBOX_ORIENTATION_ADAPTIVE       11
#define IDC_STATIC_TESS_FACTOR                  12
#define IDC_SLIDER_TESS_FACTOR                  13
#define IDC_SLIDER_SIMUL_SPEED                  14
#define IDC_STATIC_SIMUL_SPEED                  18
#define IDC_SLIDER_KERNEL_SCALER                  15
#define IDC_BUTTON_SAVE                 16
#define IDC_BUTTON_LOADOBJ		17
#define IDC_STATIC_KERNEL_SCALER                   19
#define IDC_SLIDER_INIT_X			20
#define IDC_SLIDER_INIT_Y			21
#define IDC_SLIDER_INIT_Z			22
#define IDC_STATIC_INIT_X			23
#define IDC_STATIC_INIT_Y			24
#define IDC_STATIC_INIT_Z			25
#define IDC_STATIC_NUM_PARTICLE                  26
#define IDC_COMBO_NUM_PARTICLE                  27
#define IDC_BUTTON_RESET		28
#define IDC_CHECKBOX_INVERT_NORMAL  29
#define IDC_STATIC_SURFACE_SCALER                   30
#define IDC_SLIDER_SURFACE_SCALER                  31
#define IDC_BUTTON_SAVE_SURFACE                 32


//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext );
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                         void* pUserContext );
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );

bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                      DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext );
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                     void* pUserContext );
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                         const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext );
void CALLBACK OnD3D11DestroyDevice( void* pUserContext );
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                 float fElapsedTime, void* pUserContext );

void InitApp();
void RenderText();

// Helper functions
HRESULT CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, 
                               LPCSTR szShaderModel, ID3DBlob** ppBlobOut, D3D_SHADER_MACRO* pDefines );
bool IsNextArg( WCHAR*& strCmdLine, WCHAR* strArg );
bool GetCmdParam( WCHAR*& strCmdLine, WCHAR* strFlag );
void ParseCommandLine();
void CaptureFrame();
void RenderMesh( CDXUTSDKMesh* pDXUTMesh, UINT uMesh, 
                 D3D11_PRIMITIVE_TOPOLOGY PrimType = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED, 
                 UINT uDiffuseSlot = INVALID_SAMPLER_SLOT, UINT uNormalSlot = INVALID_SAMPLER_SLOT,
                 UINT uSpecularSlot = INVALID_SAMPLER_SLOT );
bool FileExists( WCHAR* pFileName );
HRESULT CreateHullShader();
void NormalizePlane( D3DXVECTOR4* pPlaneEquation );
void ExtractPlanesFromFrustum( D3DXVECTOR4* pPlaneEquation, const D3DXMATRIX* pMatrix );
bool ShowLoadDlg( WCHAR* szfn, const WCHAR* szfilter);
bool ShowSaveDlg( WCHAR* szfn, const WCHAR* szfilter);
HRESULT ResetGeometry();
HRESULT ResetParticles();

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
	AllocConsole();
	AttachConsole(GetCurrentProcessId()); 
	freopen("CONIN$", "r+t", stdin); 
	freopen("CONOUT$", "w+t", stdout); 
    // Disable gamma correction on this sample
    DXUTSetIsInGammaCorrectMode( false );

    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackKeyboard( OnKeyboard );
    DXUTSetCallbackFrameMove( OnFrameMove );
    
    DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
    DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
    DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );
    DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
    DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );

    ParseCommandLine();

    InitApp();
    
    DXUTInit( true, true );                 // Use this line instead to try to create a hardware device

    DXUTSetCursorSettings( true, true );    // Show the cursor and clip it when in full screen
    DXUTCreateWindow( L"PN-Triangles Direct3D 11" );

    DXUTCreateDevice( D3D_FEATURE_LEVEL_11_0, true, g_CmdLineParams.uWidth, g_CmdLineParams.uHeight );
    DXUTMainLoop();                         // Enter into the DXUT render loop

    return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
    g_D3DSettingsDlg.Init( &g_DialogResourceManager );
    g_HUD.Init( &g_DialogResourceManager );
    g_SampleUI.Init( &g_DialogResourceManager );
    g_SampleUI.GetFont( 0 );

    g_HUD.SetCallback( OnGUIEvent ); 
    int iY = 30;
    g_HUD.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 0, iY, 170, 23 );
    g_HUD.AddButton( IDC_TOGGLEREF, L"Toggle REF (F3)", 0, iY += 26, 170, 23, VK_F3 );
    g_HUD.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 0, iY += 26, 170, 23, VK_F2 );
    

    g_SampleUI.SetCallback( OnGUIEvent );
    iY = 0;


    WCHAR szTemp[256];
    
	g_SampleUI.AddStatic( IDC_STATIC_NUM_PARTICLE, L"# Centroids: ", 20, iY += 25, 108, 24 );
	g_SampleUI.AddComboBox( IDC_COMBO_NUM_PARTICLE, -100, iY, 100, 24);
	for(int i = 0; i < ARRAYSIZE(num_particles_list); ++i)
		g_SampleUI.GetComboBox( IDC_COMBO_NUM_PARTICLE )->AddItem( num_particles_name_list[i], (void*)&num_particles_list[i] );
	g_SampleUI.GetComboBox( IDC_COMBO_NUM_PARTICLE )->SetSelectedByIndex(1);

    // Tess factor
    swprintf_s( szTemp, L"Tess Level: %d", g_uTessFactor );
    g_SampleUI.AddStatic( IDC_STATIC_TESS_FACTOR, szTemp, 20, iY += 25, 108, 24 );
    g_SampleUI.AddSlider( IDC_SLIDER_TESS_FACTOR, -100, iY, 100, 24, 1, 11, 1 + ( g_uTessFactor - 1 ) / 2, false );
	swprintf_s( szTemp, L"Speed: %f", g_fSpeed );
    g_SampleUI.AddStatic( IDC_STATIC_SIMUL_SPEED, szTemp, 20, iY += 25, 108, 24 );
    g_SampleUI.AddSlider( IDC_SLIDER_SIMUL_SPEED, -100, iY, 100, 24, 0, 5000, 1000, false );

	swprintf_s( szTemp, L"Kernel Scale: %f", g_fKScale );
    g_SampleUI.AddStatic( IDC_STATIC_KERNEL_SCALER, szTemp, 20, iY += 25, 108, 24 );
    g_SampleUI.AddSlider( IDC_SLIDER_KERNEL_SCALER, -100, iY, 100, 24, 1, 5000, (int)(g_fKScale * 1000), false );
	g_SampleUI.AddStatic(IDC_STATIC_INIT_X, L"0", 20, iY += 25, 108, 24 );
	g_SampleUI.AddSlider(IDC_SLIDER_INIT_X, -100, iY, 100, 24, -10000, 10000, 0 );
	g_SampleUI.AddStatic(IDC_STATIC_INIT_Y, L"0", 20, iY += 25, 108, 24 );
	g_SampleUI.AddSlider(IDC_SLIDER_INIT_Y, -100, iY, 100, 24, -10000, 10000, 0 );
	g_SampleUI.AddStatic(IDC_STATIC_INIT_Z, L"0", 20, iY += 25, 108, 24 );
	g_SampleUI.AddSlider(IDC_SLIDER_INIT_Z, -100, iY, 100, 24, -10000, 10000, 0 );

	g_SampleUI.AddCheckBox (IDC_CHECKBOX_INVERT_NORMAL, L"Inverted Normal", -100, iY += 25, 228, 24 );
	g_SampleUI.AddButton(IDC_BUTTON_LOADOBJ, L"Load OBJ", -100, iY += 25, 228, 24 );
	g_SampleUI.AddButton( IDC_BUTTON_RESET, L"Reset Particles", -100, iY += 25, 228, 24 );
	g_SampleUI.AddButton( IDC_BUTTON_SAVE, L"Save Result", -100, iY += 25, 228, 24 );

	swprintf_s( szTemp, L"Surf Criterion: %f", g_fSurface );
	g_SampleUI.AddStatic(IDC_STATIC_SURFACE_SCALER, szTemp, 20, iY += 25, 108, 24 );
    g_SampleUI.AddSlider( IDC_SLIDER_SURFACE_SCALER, -100, iY, 100, 24, 1, 5000,  (int)(g_fSurface * 10000), false );
	g_SampleUI.AddButton( IDC_BUTTON_SAVE_SURFACE, L"Save Surface Points", -100, iY += 25, 228, 24 );
}


//--------------------------------------------------------------------------------------
// This callback function is called immediately before a device is created to allow the 
// application to modify the device settings. The supplied pDeviceSettings parameter 
// contains the settings that the framework has selected for the new device, and the 
// application can make any desired changes directly to this structure.  Note however that 
// DXUT will not correct invalid device settings so care must be taken 
// to return valid device settings, otherwise CreateDevice() will fail.  
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    assert( pDeviceSettings->ver == DXUT_D3D11_DEVICE );

    // For the first device created if it is a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;

        if( pDeviceSettings->d3d11.DriverType == D3D_DRIVER_TYPE_REFERENCE )
        {
            DXUTDisplaySwitchingToREFWarning( pDeviceSettings->ver );
        }
    }

    return true;
}


//--------------------------------------------------------------------------------------
// This callback function will be called once at the beginning of every frame. This is the
// best location for your application to handle updates to the scene, but is not 
// intended to contain actual rendering calls, which should instead be placed in the 
// OnFrameRender callback.  
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
    // Update the camera's position based on user input 
    g_Camera[g_eMeshType].FrameMove( fElapsedTime );
    g_LightCamera.FrameMove( fElapsedTime );
}


//--------------------------------------------------------------------------------------
// Render stats
//--------------------------------------------------------------------------------------
void RenderText()
{
    g_pTxtHelper->Begin();
    
    g_pTxtHelper->SetInsertionPos( 2, 0 );
    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );

    g_pTxtHelper->SetInsertionPos( 2, DXUTGetDXGIBackBufferSurfaceDesc()->Height - 35 );
    g_pTxtHelper->DrawTextLine( L"Toggle GUI    : G" );
    g_pTxtHelper->DrawTextLine( L"Frame Capture : C" );

    g_pTxtHelper->End();
}


//--------------------------------------------------------------------------------------
// Before handling window messages, DXUT passes incoming windows 
// messages to the application through this callback function. If the application sets 
// *pbNoFurtherProcessing to TRUE, then DXUT will not process this message.
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                         void* pUserContext )
{
    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass messages to settings dialog if its active
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;
    *pbNoFurtherProcessing = g_SampleUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass all windows messages to camera so it can respond to user input
    g_Camera[g_eMeshType].HandleMessages( hWnd, uMsg, wParam, lParam );
    g_LightCamera.HandleMessages( hWnd, uMsg, wParam, lParam );

    return 0;
}


float AvgDensity() {
	D3D11_BUFFER_DESC bufdesc;	
	g_pParticleDensity->GetDesc(&bufdesc);
	bufdesc.BindFlags = 0;
	bufdesc.Usage = D3D11_USAGE_STAGING;
	bufdesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
	bufdesc.StructureByteStride = 0;
	bufdesc.MiscFlags = 0;
	ID3D11Buffer* pStagDensity;
	DXUTGetD3D11Device()->CreateBuffer(&bufdesc, NULL, &pStagDensity);
	DXUTGetD3D11DeviceContext()->CopyResource(pStagDensity, g_pParticleDensity);
	D3D11_MAPPED_SUBRESOURCE ms1;
	DXUTGetD3D11DeviceContext()->Map(pStagDensity, 0, D3D11_MAP_READ_WRITE, 0, &ms1);

	FLOAT* density = (FLOAT*)ms1.pData;
	FLOAT sum = 0;
	for(int i = 0; i < g_iNumParticles; i++)
	{
		sum += density[i];
	}
	sum /= (FLOAT)g_iNumParticles;

	DXUTGetD3D11DeviceContext()->Unmap(pStagDensity, 0);
	pStagDensity->Release();

	return sum;
}

void SavePointsBuffer(bool bSaveSurface = false)
{

	WCHAR strOff[MAX_PATH] = {0};
	const WCHAR* filter = L"COFF\0*.OFF\0";
	bool isOK = ShowSaveDlg(strOff, filter);
	if(!isOK) return;

	std::wofstream InFile( strOff );
    if( !InFile ) {
		printf("wofstream::open\n");
		return;
	}

	InFile << "COFF" << std::endl;

	D3D11_BUFFER_DESC bufdesc;
	g_pParticles->GetDesc(&bufdesc);
	bufdesc.BindFlags = 0;
	bufdesc.Usage = D3D11_USAGE_STAGING;
	bufdesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
	bufdesc.StructureByteStride = 0;
	bufdesc.MiscFlags = 0;

	ID3D11Buffer* pStag;
	DXUTGetD3D11Device()->CreateBuffer(&bufdesc, NULL, &pStag);
	DXUTGetD3D11DeviceContext()->CopyResource(pStag, g_pParticles);
	D3D11_MAPPED_SUBRESOURCE ms;
	DXUTGetD3D11DeviceContext()->Map(pStag, 0, D3D11_MAP_READ_WRITE, 0, &ms);

	D3DXVECTOR3 vMove = -g_SceneMesh[g_eMeshType].GetMeshBBoxCenter();
	vMove += g_SceneMesh[g_eMeshType].GetMeshBBoxExtents();

	FLOAT fAd = AvgDensity();
	FLOAT fScale = powf(fAd / g_fRestDensity, 0.3333333333333f) * 0.2f;

	D3DXVECTOR4* pVert = (D3DXVECTOR4*)(ms.pData);

	int count = g_iNumParticles;
	if(bSaveSurface)
	{
		for(UINT i = 0; i < g_iNumParticles; i++) {
			D3DXVECTOR4& ivert = pVert[i];
			//ivert = (ivert + vMove) * fScale;
			if(pVert[i].w < g_fSurface) --count;
		}
	}

	InFile << count << " " << 0 << " " << 0 << std::endl;

	for(UINT i = 0; i < g_iNumParticles; i++) {
		D3DXVECTOR4& ivert = pVert[i];
		//ivert = (ivert + vMove) * fScale;
		if(bSaveSurface && pVert[i].w < g_fSurface) continue;
		InFile << 
			ivert.x << " " << ivert.y << " " << ivert.z << std::endl;
	}
	InFile.close();
	DXUTGetD3D11DeviceContext()->Unmap(pStag, 0);
	pStag->Release();
}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    WCHAR szTemp[256];
    bool bEnable;
    
    switch( nControlID )
    {
		case IDC_COMBO_NUM_PARTICLE:
			g_iNumParticles = *((UINT*)((CDXUTComboBox*)pControl)->GetSelectedData());
			ResetGeometry();
			break;

        case IDC_TOGGLEFULLSCREEN:
            DXUTToggleFullScreen();
            break;

        case IDC_TOGGLEREF:
            DXUTToggleREF();
            break;

        case IDC_CHANGEDEVICE:
            g_D3DSettingsDlg.SetActive( !g_D3DSettingsDlg.IsActive() );
            break;

        case IDC_SLIDER_TESS_FACTOR:
            g_uTessFactor = ( (unsigned int)((CDXUTSlider*)pControl)->GetValue() - 1 ) * 2 + 1;
            swprintf_s( szTemp, L"Tess Level: %d", g_uTessFactor );
            g_SampleUI.GetStatic( IDC_STATIC_TESS_FACTOR )->SetText( szTemp );
			g_bFieldUpdated = FALSE;
            break;

        case IDC_SLIDER_SIMUL_SPEED:
			g_fSpeed = ( (FLOAT)((CDXUTSlider*)pControl)->GetValue()) / 1000.0f;
			swprintf_s( szTemp, L"Speed: %f", g_fSpeed );
            g_SampleUI.GetStatic( IDC_STATIC_SIMUL_SPEED )->SetText( szTemp );
            break;

        case IDC_SLIDER_KERNEL_SCALER:
			g_fKScale = ( (FLOAT)((CDXUTSlider*)pControl)->GetValue()) / 1000.0f;
			swprintf_s( szTemp, L"Kernel Scale: %f", g_fKScale );
            g_SampleUI.GetStatic( IDC_STATIC_KERNEL_SCALER )->SetText( szTemp );
            break;

        case IDC_SLIDER_SURFACE_SCALER:
			g_fSurface = ( (FLOAT)((CDXUTSlider*)pControl)->GetValue()) / 10000.0f;
			swprintf_s( szTemp, L"Surf Criterion: %f", g_fSurface );
            g_SampleUI.GetStatic( IDC_STATIC_SURFACE_SCALER )->SetText( szTemp );
            break;

        case IDC_COMBOBOX_MESH:
            g_eMeshType = (MESH_TYPE)((CDXUTComboBox*)pControl)->GetSelectedIndex();
			g_bFieldUpdated = FALSE;
            break;

		case IDC_BUTTON_SAVE:
			g_bSavePoints = TRUE;
			break;

		case IDC_BUTTON_SAVE_SURFACE:
			g_bSaveSurfacePoints = TRUE;
			break;

		case IDC_BUTTON_LOADOBJ:
			{
				const WCHAR* filter = L"Wavefront OBJ\0*.OBJ\0";
				bool isOK = ShowLoadDlg(g_default_mesh_fn, filter);
				if(!isOK) break;
				ResetGeometry();
			}
			break;
		case IDC_CHECKBOX_INVERT_NORMAL:
			g_fNormalScalar = -g_fNormalScalar;
			ResetParticles();
			break;
		case IDC_BUTTON_RESET:
			ResetParticles();
			break;
		case IDC_SLIDER_INIT_X:
		case IDC_SLIDER_INIT_Y:
		case IDC_SLIDER_INIT_Z:
			if(nEvent == EVENT_SLIDER_VALUE_CHANGED_UP)
			{
				g_bNoSimulating = FALSE;
			} else {
				D3DXVECTOR3 vExt = g_SceneMesh[g_eMeshType].GetMeshBBoxExtents();
				g_bNoSimulating = TRUE;
				g_vInitOffset.x = (float)(g_SampleUI.GetSlider(IDC_SLIDER_INIT_X)->GetValue()) / 10000.0f * vExt.x;
				g_vInitOffset.y = (float)(g_SampleUI.GetSlider(IDC_SLIDER_INIT_Y)->GetValue()) / 10000.0f * vExt.y;
				g_vInitOffset.z = (float)(g_SampleUI.GetSlider(IDC_SLIDER_INIT_Z)->GetValue()) / 10000.0f * vExt.z;
				
				swprintf_s( szTemp, L"Offset X: %f", g_vInitOffset.x );
				g_SampleUI.GetStatic( IDC_STATIC_INIT_X )->SetText( szTemp );
				swprintf_s( szTemp, L"Offset Y: %f", g_vInitOffset.y );
				g_SampleUI.GetStatic( IDC_STATIC_INIT_Y )->SetText( szTemp );
				swprintf_s( szTemp, L"Offset Z: %f", g_vInitOffset.z );
				g_SampleUI.GetStatic( IDC_STATIC_INIT_Z )->SetText( szTemp );

				ResetParticles();
			}
			break;
    }
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
    static int iCaptureNumber = 0;
    #define VK_C (67)
    #define VK_G (71)

    if( bKeyDown )
    {
        switch( nChar )
        {
            case VK_C:    
                swprintf_s( g_CmdLineParams.strCaptureFilename, L"FrameCapture%d.bmp", iCaptureNumber );
                CaptureFrame();
                iCaptureNumber++;
                break;

            case VK_G:
                g_CmdLineParams.bRenderHUD = !g_CmdLineParams.bRenderHUD;
                break;
        }
    }
}


//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                      DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    return true;
}

template <class T>
HRESULT CreateStructuredBuffer(ID3D11Device* pd3dDevice, UINT iNumElements, ID3D11Buffer** ppBuffer, 
	ID3D11ShaderResourceView** ppSRV, ID3D11UnorderedAccessView** ppUAV, const T* pInitialData = NULL, 
	const BOOL bAppend = FALSE
	)
{
    HRESULT hr = S_OK;

    // Create SB
	if(ppBuffer) {
		D3D11_BUFFER_DESC bufferDesc;
		ZeroMemory( &bufferDesc, sizeof(bufferDesc) );
		bufferDesc.ByteWidth = iNumElements * sizeof(T);
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
		bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		bufferDesc.StructureByteStride = sizeof(T);

		D3D11_SUBRESOURCE_DATA bufferInitData;
		ZeroMemory( &bufferInitData, sizeof(bufferInitData) );
		bufferInitData.pSysMem = pInitialData;
		V_RETURN( pd3dDevice->CreateBuffer( &bufferDesc, (pInitialData)? &bufferInitData : NULL, ppBuffer ) );
	}

    // Create SRV
	if(ppBuffer && ppSRV) {
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory( &srvDesc, sizeof(srvDesc) );
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.ElementWidth = iNumElements;
		V_RETURN( pd3dDevice->CreateShaderResourceView( *ppBuffer, &srvDesc, ppSRV ) );
	}

    // Create UAV
	if(ppBuffer && ppUAV) {
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		ZeroMemory( &uavDesc, sizeof(uavDesc) );
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.NumElements = iNumElements;
		if(bAppend) uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_APPEND;
		V_RETURN( pd3dDevice->CreateUnorderedAccessView( *ppBuffer, &uavDesc, ppUAV ) );
	}

    return hr;
}

template <class T>
HRESULT CreateConstantBuffer(ID3D11Device* pd3dDevice, ID3D11Buffer** ppCB)
{
    HRESULT hr = S_OK;

    D3D11_BUFFER_DESC Desc;
    Desc.Usage = D3D11_USAGE_DYNAMIC;
    Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Desc.MiscFlags = 0;
    Desc.ByteWidth = sizeof( T );
    V_RETURN( pd3dDevice->CreateBuffer( &Desc, NULL, ppCB ) );

    return hr;
}

HRESULT CreateTypedBuffer(ID3D11Device* pd3dDevice, DXGI_FORMAT format, DXGI_FORMAT formatUAV, UINT iStride, UINT iNumElements, UINT iNumElementsUAV, ID3D11Buffer** ppBuffer, 
	ID3D11ShaderResourceView** ppSRV, ID3D11UnorderedAccessView** ppUAV, const void* pInitialData = NULL
	)
{
    HRESULT hr = S_OK;

    // Create SB
	if(ppBuffer) {
		D3D11_BUFFER_DESC bufferDesc;
		ZeroMemory( &bufferDesc, sizeof(bufferDesc) );
		bufferDesc.ByteWidth = iNumElements * iStride;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA bufferInitData;
		ZeroMemory( &bufferInitData, sizeof(bufferInitData) );
		bufferInitData.pSysMem = pInitialData;
		V_RETURN( pd3dDevice->CreateBuffer( &bufferDesc, (pInitialData)? &bufferInitData : NULL, ppBuffer ) );
	}

    // Create SRV
	if(ppBuffer && ppSRV) {
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory( &srvDesc, sizeof(srvDesc) );
		srvDesc.Format = format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.ElementWidth = iNumElements;
		V_RETURN( pd3dDevice->CreateShaderResourceView( *ppBuffer, &srvDesc, ppSRV ) );
	}

    // Create UAV
	if(ppBuffer && ppUAV) {
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		ZeroMemory( &uavDesc, sizeof(uavDesc) );
		uavDesc.Format = formatUAV;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.NumElements = iNumElementsUAV;
		V_RETURN( pd3dDevice->CreateUnorderedAccessView( *ppBuffer, &uavDesc, ppUAV ) );
	}

    return hr;
}

HRESULT CreateSimulationBuffers( ID3D11Device* pd3dDevice )
{
	HRESULT hr;

	printf("Creating Simulation Buffers...\n");

    SAFE_RELEASE( g_pParticleDensity );
    SAFE_RELEASE( g_pParticleDensitySRV );
    SAFE_RELEASE( g_pParticleDensityUAV );

    SAFE_RELEASE( g_pGridSRV );
    SAFE_RELEASE( g_pGridUAV );
    SAFE_RELEASE( g_pGrid );

    SAFE_RELEASE( g_pGridPingPongSRV );
    SAFE_RELEASE( g_pGridPingPongUAV );
    SAFE_RELEASE( g_pGridPingPong );

    SAFE_RELEASE( g_pGridIndicesSRV );
    SAFE_RELEASE( g_pGridIndicesUAV );
    SAFE_RELEASE( g_pGridIndices );

	V_RETURN(ResetParticles());

    V_RETURN( CreateStructuredBuffer< FLOAT >( pd3dDevice, g_iNumParticles, &g_pParticleDensity, &g_pParticleDensitySRV, &g_pParticleDensityUAV ) );
    DXUT_SetDebugName( g_pParticleDensity, "Density" );
    DXUT_SetDebugName( g_pParticleDensitySRV, "Density SRV" );
    DXUT_SetDebugName( g_pParticleDensityUAV, "Density UAV" );

	V_RETURN(CreateTypedBuffer( pd3dDevice, DXGI_FORMAT_R32G32_UINT, DXGI_FORMAT_R32_UINT, sizeof(UINT) * 2, g_iNumParticles, g_iNumParticles * 2, &g_pGrid, &g_pGridSRV, &g_pGridUAV));
    DXUT_SetDebugName( g_pGrid, "Grid" );
    DXUT_SetDebugName( g_pGridSRV, "Grid SRV" );
    DXUT_SetDebugName( g_pGridUAV, "Grid UAV" );

    V_RETURN( CreateTypedBuffer( pd3dDevice, DXGI_FORMAT_R32G32_UINT, DXGI_FORMAT_R32_UINT, sizeof(UINT) * 2, g_iNumParticles, g_iNumParticles * 2, &g_pGridPingPong, &g_pGridPingPongSRV, &g_pGridPingPongUAV ) );
    DXUT_SetDebugName( g_pGridPingPong, "PingPong" );
    DXUT_SetDebugName( g_pGridPingPongSRV, "PingPong SRV" );
    DXUT_SetDebugName( g_pGridPingPongUAV, "PingPong UAV" );

	V_RETURN( CreateStructuredBuffer< D3DXVECTOR2 >( pd3dDevice, NUM_GRID_INDICES, &g_pGridIndices, &g_pGridIndicesSRV, &g_pGridIndicesUAV ) );
	DXUT_SetDebugName( g_pGridIndices, "Indices" );
	DXUT_SetDebugName( g_pGridIndicesSRV, "Indices SRV" );
	DXUT_SetDebugName( g_pGridIndicesUAV, "Indices UAV" );

	SAFE_RELEASE(g_pTexField);
	SAFE_RELEASE(g_pSRVField);
	SAFE_RELEASE(g_pUAVField);
	SAFE_RELEASE(g_pTexFieldDepth);
	SAFE_RELEASE(g_pTexFieldProxy);
	SAFE_RELEASE(g_pDSVField);
	SAFE_RELEASE(g_pSRVFieldProxy);
	SAFE_RELEASE(g_pRTVFieldProxy);

	D3D11_TEXTURE3D_DESC t3desc;
	t3desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	t3desc.CPUAccessFlags = 0;
	t3desc.Depth = g_iFieldSize[2];
	t3desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	t3desc.Height = g_iFieldSize[1];
	t3desc.MipLevels = 1;
	t3desc.MiscFlags = 0;
	t3desc.Usage = D3D11_USAGE_DEFAULT;
	t3desc.Width = g_iFieldSize[0];
	V_RETURN( pd3dDevice->CreateTexture3D(&t3desc, NULL, &g_pTexField) );

	D3D11_TEXTURE2D_DESC t2desc;
	t2desc.ArraySize = g_iFieldSize[2];
	t2desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	t2desc.CPUAccessFlags = 0;
	t2desc.Format = DXGI_FORMAT_R32_TYPELESS;
	t2desc.Height = g_iFieldSize[1];
	t2desc.MipLevels = 1;
	t2desc.MiscFlags = 0;
	t2desc.SampleDesc.Count = 1;
	t2desc.SampleDesc.Quality = 0;
	t2desc.Usage = D3D11_USAGE_DEFAULT;
	t2desc.Width = g_iFieldSize[0];
	V_RETURN( pd3dDevice->CreateTexture2D(&t2desc, NULL, &g_pTexFieldDepth) );	

	t2desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	t2desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	V_RETURN( pd3dDevice->CreateTexture2D(&t2desc, NULL, &g_pTexFieldProxy) );	

	V_RETURN( pd3dDevice->CreateShaderResourceView(g_pTexField, NULL, &g_pSRVField) );
	V_RETURN( pd3dDevice->CreateUnorderedAccessView(g_pTexField, NULL, &g_pUAVField) );

	V_RETURN( pd3dDevice->CreateShaderResourceView(g_pTexFieldProxy, NULL, &g_pSRVFieldProxy) );
	V_RETURN( pd3dDevice->CreateRenderTargetView(g_pTexFieldProxy, NULL, &g_pRTVFieldProxy) );

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvdesc;
	dsvdesc.Flags = 0;
	dsvdesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvdesc.Texture2DArray.ArraySize = g_iFieldSize[2];
	dsvdesc.Texture2DArray.FirstArraySlice = 0;
	dsvdesc.Texture2DArray.MipSlice = 0;
	dsvdesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;

	V_RETURN( pd3dDevice->CreateDepthStencilView(g_pTexFieldDepth, &dsvdesc, &g_pDSVField) );

	return hr;
}

//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                     void* pUserContext )
{
    HRESULT hr;

    ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext();
    V_RETURN( g_DialogResourceManager.OnD3D11CreateDevice( pd3dDevice, pd3dImmediateContext ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11CreateDevice( pd3dDevice ) );
    g_pTxtHelper = new CDXUTTextHelper( pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, 15 );

    // Create the shaders
    ID3DBlob* pBlob = NULL;
    D3D_SHADER_MACRO ShaderMacros[4];
    ZeroMemory( ShaderMacros, sizeof(ShaderMacros) );
    ShaderMacros[0].Definition = "1";
    ShaderMacros[1].Definition = "1";
    ShaderMacros[2].Definition = "1";
    ShaderMacros[3].Definition = "1";
        
    // Main scene VS (no tessellation)
    V_RETURN( CompileShaderFromFile( L"Mesh2Points.hlsl", "VS_RenderScene", "vs_4_0", &pBlob, NULL ) ); 
    V_RETURN( pd3dDevice->CreateVertexShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pSceneVS ) );
    DXUT_SetDebugName( g_pSceneVS, "VS_RenderScene" );
    
    // Define our scene vertex data layout
    const D3D11_INPUT_ELEMENT_DESC SceneLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    V_RETURN( pd3dDevice->CreateInputLayout( SceneLayout, ARRAYSIZE( SceneLayout ), pBlob->GetBufferPointer(),
                                                pBlob->GetBufferSize(), &g_pSceneVertexLayout ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pSceneVertexLayout, "Primary" );

    // Main scene VS (with tessellation)
    V_RETURN( CompileShaderFromFile( L"Mesh2Points.hlsl", "VS_RenderSceneWithTessellation", "vs_4_0", &pBlob, NULL ) ); 
    V_RETURN( pd3dDevice->CreateVertexShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pSceneWithTessellationVS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pSceneWithTessellationVS, "VS_RenderSceneWithTessellation" );

    // PNTriangles HS
    V_RETURN( CreateHullShader() );
            
    // PNTriangles DS
    V_RETURN( CompileShaderFromFile( L"Mesh2Points.hlsl", "DS_PNTriangles", "ds_5_0", &pBlob, NULL ) ); 
    V_RETURN( pd3dDevice->CreateDomainShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pPNTrianglesDS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pPNTrianglesDS, "DS_PNTriangles" );

    // Main scene PS (no textures)
    V_RETURN( CompileShaderFromFile( L"Mesh2Points.hlsl", "PS_RenderScene", "ps_4_0", &pBlob, NULL ) ); 
    V_RETURN( pd3dDevice->CreatePixelShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pScenePS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pScenePS, "PS_RenderScene" );

    V_RETURN( CompileShaderFromFile( L"Mesh2Points.hlsl", "VisualizeVS", "vs_4_0", &pBlob, NULL ) ); 
	V_RETURN( pd3dDevice->CreateVertexShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pVisualizeVS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pVisualizeVS, "VisualizeVS" );

    V_RETURN( CompileShaderFromFile( L"Mesh2Points.hlsl", "VisualizePS", "ps_4_0", &pBlob, NULL ) ); 
	V_RETURN( pd3dDevice->CreatePixelShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pVisualizePS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pVisualizePS, "VisualizePS" );

    // Main scene PS (textured)
    V_RETURN( CompileShaderFromFile( L"Mesh2Points.hlsl", "PS_RenderSceneTextured", "ps_4_0", &pBlob, NULL ) ); 
    V_RETURN( pd3dDevice->CreatePixelShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pTexturedScenePS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pTexturedScenePS, "PS_RenderSceneTextured" );

	V_RETURN( CompileShaderFromFile( L"Mesh2Points.hlsl", "TriField3DGS", "gs_4_0", &pBlob, NULL ) ); 
    V_RETURN( pd3dDevice->CreateGeometryShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pFieldGS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pFieldGS, "TriField3DGS" );

	V_RETURN( CompileShaderFromFile( L"Mesh2Points.hlsl", "TriField3DPS", "ps_4_0", &pBlob, NULL ) ); 
    V_RETURN( pd3dDevice->CreatePixelShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pFieldPS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pFieldPS, "TriField3DPS" );

	V_RETURN( CompileShaderFromFile( L"Mesh2Points.hlsl", "ParticleVS", "vs_4_0", &pBlob, NULL  ) );
    V_RETURN( pd3dDevice->CreateVertexShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pParticleVS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pParticleVS, "ParticleVS" );

    V_RETURN( CompileShaderFromFile( L"Mesh2Points.hlsl", "ParticleGS", "gs_4_0", &pBlob, NULL  ) );
    V_RETURN( pd3dDevice->CreateGeometryShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pParticleGS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pParticleGS, "ParticleGS" );

    V_RETURN( CompileShaderFromFile( L"Mesh2Points.hlsl", "ParticlePS", "ps_4_0", &pBlob, NULL  ) );
    V_RETURN( pd3dDevice->CreatePixelShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pParticlePS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pParticlePS, "ParticlePS" );

	   // Sort Shaders
    V_RETURN( CompileShaderFromFile( L"ComputeShaderSort11.hlsl", "BitonicSort", "cs_5_0", &pBlob, NULL ) );
    V_RETURN( pd3dDevice->CreateComputeShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pSortBitonic ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pSortBitonic, "BitonicSort" );

    V_RETURN( CompileShaderFromFile( L"ComputeShaderSort11.hlsl", "MatrixTranspose", "cs_5_0", &pBlob, NULL ) );
    V_RETURN( pd3dDevice->CreateComputeShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pSortTranspose ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pSortTranspose, "MatrixTranspose" );

	V_RETURN( CompileShaderFromFile( L"ComputeShaderSortUint11.hlsl", "BitonicSortUint", "cs_5_0", &pBlob, NULL ) );
    V_RETURN( pd3dDevice->CreateComputeShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pSortBitonicUint ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pSortBitonicUint, "BitonicSortUint" );

    V_RETURN( CompileShaderFromFile( L"ComputeShaderSortUint11.hlsl", "MatrixTransposeUint", "cs_5_0", &pBlob, NULL ) );
    V_RETURN( pd3dDevice->CreateComputeShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pSortTransposeUint ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pSortTransposeUint, "MatrixTransposeUint" );


    V_RETURN( CompileShaderFromFile( L"Mesh2Points.hlsl", "BuildGridCS", "cs_5_0", &pBlob, NULL ) );
    V_RETURN( pd3dDevice->CreateComputeShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pBuildGridCS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pBuildGridCS, "BuildGridCS" );

    V_RETURN( CompileShaderFromFile( L"Mesh2Points.hlsl", "BuildGridIndicesCS", "cs_5_0", &pBlob, NULL ) );
    V_RETURN( pd3dDevice->CreateComputeShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pBuildGridIndicesCS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pBuildGridIndicesCS, "BuildGridIndicesCS" );

    V_RETURN( CompileShaderFromFile( L"Mesh2Points.hlsl", "RearrangeParticlesCS", "cs_5_0", &pBlob, NULL ) );
    V_RETURN( pd3dDevice->CreateComputeShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pRearrangeParticlesCS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pRearrangeParticlesCS, "RearrangeParticlesCS" );

    V_RETURN( CompileShaderFromFile( L"Mesh2Points.hlsl", "VelocityCS", "cs_5_0", &pBlob, NULL ) );
    V_RETURN( pd3dDevice->CreateComputeShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pVelocityCS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pVelocityCS, "VelocityCS" );
 
    V_RETURN( CompileShaderFromFile( L"Mesh2Points.hlsl", "DensityCS", "cs_5_0", &pBlob, NULL ) );
    V_RETURN( pd3dDevice->CreateComputeShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pDensityCS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pDensityCS, "DensityCS" );
 
    V_RETURN( CompileShaderFromFile( L"Mesh2Points.hlsl", "ArrayTo3DCS", "cs_5_0", &pBlob, NULL ) );
    V_RETURN( pd3dDevice->CreateComputeShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pArrayTo3DCS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pArrayTo3DCS, "ArrayTo3DCS" );

    V_RETURN( CreateConstantBuffer< SortCB >( pd3dDevice, &g_pSortCB ) );
    V_RETURN( CreateConstantBuffer< CB_PNTRIANGLES >( pd3dDevice, &g_pcbPNTriangles ) );
    DXUT_SetDebugName( g_pcbPNTriangles, "CB_PNTRIANGLES" );
    DXUT_SetDebugName( g_pSortCB, "Sort" );

	D3DXMatrixIdentity(&g_m4x4MeshMatrix[MESH_TYPE_TIGER]);


	 // Create sampler states for point and linear
    // Point
    D3D11_SAMPLER_DESC SamDesc;
    SamDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    SamDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamDesc.MipLODBias = 0.0f;
    SamDesc.MaxAnisotropy = 1;
    SamDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    SamDesc.BorderColor[0] = SamDesc.BorderColor[1] = SamDesc.BorderColor[2] = SamDesc.BorderColor[3] = 0;
    SamDesc.MinLOD = 0;
    SamDesc.MaxLOD = D3D11_FLOAT32_MAX;
    V_RETURN( pd3dDevice->CreateSamplerState( &SamDesc, &g_pSamplePoint ) );
    DXUT_SetDebugName( g_pSamplePoint, "Point" );

    // Linear
    SamDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    SamDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    SamDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    SamDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    V_RETURN( pd3dDevice->CreateSamplerState( &SamDesc, &g_pSampleLinear ) );
    DXUT_SetDebugName( g_pSampleLinear, "Linear" );

    // Set the raster state
    // Wireframe
    D3D11_RASTERIZER_DESC RasterizerDesc;
    RasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
    RasterizerDesc.CullMode = D3D11_CULL_NONE;
    RasterizerDesc.FrontCounterClockwise = FALSE;
    RasterizerDesc.DepthBias = 0;
    RasterizerDesc.DepthBiasClamp = 0.0f;
    RasterizerDesc.SlopeScaledDepthBias = 0.0f;
    RasterizerDesc.DepthClipEnable = TRUE;
    RasterizerDesc.ScissorEnable = FALSE;
    RasterizerDesc.MultisampleEnable = FALSE;
    RasterizerDesc.AntialiasedLineEnable = FALSE;
    V_RETURN( pd3dDevice->CreateRasterizerState( &RasterizerDesc, &g_pRasterizerStateWireframe ) );
    DXUT_SetDebugName( g_pRasterizerStateWireframe, "Wireframe" );

    // Solid
    RasterizerDesc.FillMode = D3D11_FILL_SOLID;
    V_RETURN( pd3dDevice->CreateRasterizerState( &RasterizerDesc, &g_pRasterizerStateSolid ) );
    DXUT_SetDebugName( g_pRasterizerStateSolid, "Solid" );;


	D3D11_BLEND_DESC bdesc;
	ZeroMemory(&bdesc, sizeof(D3D11_BLEND_DESC));
	bdesc.RenderTarget[0].BlendEnable = TRUE;
	bdesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	bdesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	bdesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	bdesc.RenderTarget[0].RenderTargetWriteMask = 0xf;
	bdesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bdesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bdesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	V_RETURN( pd3dDevice->CreateBlendState(&bdesc, &g_pBSAlpha) );

	ResetGeometry();

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Resize
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                         const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );

    // Setup the camera's projection parameters    
    float fAspectRatio = pBackBufferSurfaceDesc->Width / ( FLOAT )pBackBufferSurfaceDesc->Height;
	g_fParticleAspectRatio = fAspectRatio;
	for( int iMeshType=0; iMeshType<MESH_TYPE_MAX; iMeshType++ )
    {
        g_Camera[iMeshType].SetProjParams( D3DX_PI / 4, fAspectRatio,0.5f, 5000.0f);
        g_Camera[iMeshType].SetWindow( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );
        g_Camera[iMeshType].SetButtonMasks( MOUSE_MIDDLE_BUTTON, MOUSE_WHEEL, MOUSE_LEFT_BUTTON );
    }

    // Setup the light camera's projection params
    g_LightCamera.SetProjParams( D3DX_PI / 4, fAspectRatio, 0.5f, 5000.0f );
    g_LightCamera.SetWindow( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );
    g_LightCamera.SetButtonMasks( MOUSE_RIGHT_BUTTON, MOUSE_WHEEL, MOUSE_RIGHT_BUTTON );

    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 180, 0 );
    g_HUD.SetSize( 170, 170 );
    g_SampleUI.SetLocation( pBackBufferSurfaceDesc->Width - 180, pBackBufferSurfaceDesc->Height - 600 );
    g_SampleUI.SetSize( 150, 110 );

    // We need a screen-sized STAGING resource for frame capturing
    D3D11_TEXTURE2D_DESC TexDesc;
    DXGI_SAMPLE_DESC SingleSample = { 1, 0 };
    TexDesc.Width = pBackBufferSurfaceDesc->Width;
    TexDesc.Height = pBackBufferSurfaceDesc->Height;
    TexDesc.Format = pBackBufferSurfaceDesc->Format;
    TexDesc.SampleDesc = SingleSample;
    TexDesc.MipLevels = 1;
    TexDesc.Usage = D3D11_USAGE_STAGING;
    TexDesc.MiscFlags = 0;
    TexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    TexDesc.BindFlags = 0;
    TexDesc.ArraySize = 1;
    V_RETURN( pd3dDevice->CreateTexture2D( &TexDesc, NULL, &g_pCaptureTexture ) )
    DXUT_SetDebugName( g_pCaptureTexture, "Capture" );
    
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Debug function which copies a GPU buffer to a CPU readable buffer
//--------------------------------------------------------------------------------------
template<class T>
VOID CheckBuffer(ID3D11Buffer* pBuffer) {
#ifdef _DEBUG
	DXUTGetD3D11DeviceContext()->Flush();
	D3D11_BUFFER_DESC bufdesc;
	pBuffer->GetDesc(&bufdesc);
	bufdesc.BindFlags = 0;
	bufdesc.Usage = D3D11_USAGE_STAGING;
	bufdesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
	bufdesc.StructureByteStride = 0;
	bufdesc.MiscFlags = 0;

	ID3D11Buffer* pStag;
	DXUTGetD3D11Device()->CreateBuffer(&bufdesc, NULL, &pStag);
	DXUTGetD3D11DeviceContext()->CopyResource(pStag, pBuffer);
	D3D11_MAPPED_SUBRESOURCE ms;
	DXUTGetD3D11DeviceContext()->Map(pStag, 0, D3D11_MAP_READ_WRITE, 0, &ms);

	T* pData = (T*)ms.pData;

	DXUTGetD3D11DeviceContext()->Unmap(pStag, 0);
	pStag->Release();
#endif
}

void RenderFluid( ID3D11DeviceContext* pd3dImmediateContext )
{
    // Set the shaders
    pd3dImmediateContext->VSSetShader( g_pParticleVS, NULL, 0 );
    pd3dImmediateContext->GSSetShader( g_pParticleGS, NULL, 0 );
    pd3dImmediateContext->PSSetShader( g_pParticlePS, NULL, 0 );

    // Setup the particles buffer and IA
    pd3dImmediateContext->VSSetShaderResources( 3, 1, &g_pParticlesSRV );
    pd3dImmediateContext->VSSetShaderResources( 4, 1, &g_pParticleDensitySRV );
	pd3dImmediateContext->IASetInputLayout(NULL);
    pd3dImmediateContext->IASetVertexBuffers( 0, 1, &g_pNullBuffer, &g_iNullUINT, &g_iNullUINT );
    pd3dImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_POINTLIST );
	pd3dImmediateContext->OMSetBlendState(g_pBSAlpha, NULL, -1U);
    // Draw the mesh
	float ClearColor[4] = { 0.05f, 0.05f, 0.05f, 0.0f };
 	ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
 	pd3dImmediateContext->ClearRenderTargetView( pRTV, ClearColor );
	pd3dImmediateContext->OMSetRenderTargets(1, &pRTV, NULL);

    pd3dImmediateContext->Draw( g_iNumParticles, 0 );
	pd3dImmediateContext->OMSetBlendState(NULL, NULL, -1U);
    // Unset the particles buffer
    pd3dImmediateContext->VSSetShaderResources( 3, 1, &g_pNullSRV );
    pd3dImmediateContext->VSSetShaderResources( 4, 1, &g_pNullSRV );
}

void VisualizeField( ID3D11DeviceContext* pd3dImmediateContext )
{
	float ClearColor[4] = { 0.05f, 0.05f, 0.05f, 0.0f };
 	ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
 	pd3dImmediateContext->ClearRenderTargetView( pRTV, ClearColor );
	pd3dImmediateContext->OMSetRenderTargets(1, &pRTV, NULL);

	pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pd3dImmediateContext->VSSetShader(g_pVisualizeVS, NULL, 0);
	pd3dImmediateContext->PSSetShader(g_pVisualizePS, NULL, 0);
	pd3dImmediateContext->PSSetShaderResources(1, 1, &g_pSRVFieldProxy);
	pd3dImmediateContext->PSSetShaderResources(2, 1, &g_pSRVField);
	pd3dImmediateContext->IASetInputLayout(NULL);
	UINT iStride = 0;
	UINT iOffset = 0;
	pd3dImmediateContext->IASetVertexBuffers(0, 1, &g_pNullBuffer, &iStride, &iOffset);
	pd3dImmediateContext->Draw(6, 0);
	pd3dImmediateContext->PSSetShaderResources(1, 1, &g_pNullSRV);
	pd3dImmediateContext->PSSetShaderResources(2, 1, &g_pNullSRV);
}

#define SIMULATION_BLOCK_SIZE 512
#define BITONIC_BLOCK_SIZE 512
#define TRANSPOSE_BLOCK_SIZE 16
//--------------------------------------------------------------------------------------
// GPU Bitonic Sort
// For more information, please see the ComputeShaderSort11 sample
//--------------------------------------------------------------------------------------
HRESULT GPUSort(ID3D11DeviceContext* pd3dImmediateContext, UINT nElements, BOOL isDouble,
             ID3D11UnorderedAccessView* inUAV, ID3D11ShaderResourceView* inSRV,
             ID3D11UnorderedAccessView* tempUAV, ID3D11ShaderResourceView* tempSRV)
{
    pd3dImmediateContext->CSSetConstantBuffers( 0, 1, &g_pSortCB );

	const UINT NUM_ELEMENTS = nElements;
    const UINT MATRIX_WIDTH = BITONIC_BLOCK_SIZE;
    const UINT MATRIX_HEIGHT = NUM_ELEMENTS / BITONIC_BLOCK_SIZE;

	D3D11_MAPPED_SUBRESOURCE ms;
    // Sort the data
    // First sort the rows for the levels <= to the block size
    for( UINT level = 2 ; level <= BITONIC_BLOCK_SIZE ; level <<= 1 )
    {
        SortCB constants = { level, level, MATRIX_HEIGHT, MATRIX_WIDTH };
		pd3dImmediateContext->Map(g_pSortCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		memcpy(ms.pData, &constants, sizeof(SortCB));
		pd3dImmediateContext->Unmap(g_pSortCB, 0);

        // Sort the row data
        UINT UAVInitialCounts = 0;
        pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &inUAV, &UAVInitialCounts );
		pd3dImmediateContext->CSSetShader( isDouble ? g_pSortBitonic : g_pSortBitonicUint, NULL, 0 );
        pd3dImmediateContext->Dispatch( NUM_ELEMENTS / BITONIC_BLOCK_SIZE, 1, 1 );
    }

    // Then sort the rows and columns for the levels > than the block size
    // Transpose. Sort the Columns. Transpose. Sort the Rows.

	for( UINT level = (BITONIC_BLOCK_SIZE << 1) ; level <= NUM_ELEMENTS ; level <<= 1 )
    {
        SortCB constants1 = { (level / BITONIC_BLOCK_SIZE), (level & ~NUM_ELEMENTS) / BITONIC_BLOCK_SIZE, MATRIX_WIDTH, MATRIX_HEIGHT };
		pd3dImmediateContext->Map(g_pSortCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		memcpy(ms.pData, &constants1, sizeof(SortCB));
		pd3dImmediateContext->Unmap(g_pSortCB, 0);

        // Transpose the data from buffer 1 into buffer 2
        ID3D11ShaderResourceView* pViewNULL = NULL;
        UINT UAVInitialCounts = 0;
        pd3dImmediateContext->CSSetShaderResources( 0, 1, &pViewNULL );
        pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &tempUAV, &UAVInitialCounts );
        pd3dImmediateContext->CSSetShaderResources( 0, 1, &inSRV );
		pd3dImmediateContext->CSSetShader( isDouble ? g_pSortTranspose : g_pSortTransposeUint, NULL, 0 );
        pd3dImmediateContext->Dispatch( MATRIX_WIDTH / TRANSPOSE_BLOCK_SIZE, MATRIX_HEIGHT / TRANSPOSE_BLOCK_SIZE, 1 );

        // Sort the transposed column data
        pd3dImmediateContext->CSSetShader( isDouble ? g_pSortBitonic : g_pSortBitonicUint, NULL, 0 );
        pd3dImmediateContext->Dispatch( NUM_ELEMENTS / BITONIC_BLOCK_SIZE, 1, 1 );

        SortCB constants2 = { BITONIC_BLOCK_SIZE, level, MATRIX_HEIGHT, MATRIX_WIDTH };
		pd3dImmediateContext->Map(g_pSortCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		memcpy(ms.pData, &constants2, sizeof(SortCB));
		pd3dImmediateContext->Unmap(g_pSortCB, 0);


        // Transpose the data from buffer 2 back into buffer 1
        pd3dImmediateContext->CSSetShaderResources( 0, 1, &pViewNULL );
        pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &inUAV, &UAVInitialCounts );
        pd3dImmediateContext->CSSetShaderResources( 0, 1, &tempSRV );
        pd3dImmediateContext->CSSetShader( isDouble ? g_pSortTranspose : g_pSortTransposeUint, NULL, 0 );
        pd3dImmediateContext->Dispatch( MATRIX_HEIGHT / TRANSPOSE_BLOCK_SIZE, MATRIX_WIDTH / TRANSPOSE_BLOCK_SIZE, 1 );

        // Sort the row data
        pd3dImmediateContext->CSSetShader( isDouble ? g_pSortBitonic : g_pSortBitonicUint, NULL, 0 );
        pd3dImmediateContext->Dispatch( NUM_ELEMENTS / BITONIC_BLOCK_SIZE, 1, 1 );
    }
	return S_OK;
}


void SimulateFluid_Grid( ID3D11DeviceContext* pd3dImmediateContext )
{
    UINT UAVInitialCounts = 0;
    pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &g_pGridUAV, &UAVInitialCounts );
    pd3dImmediateContext->CSSetShaderResources( 3, 1, &g_pParticlesSRV );
//	CheckBuffer<D3DXVECTOR3>(g_pParticles);

    // Build Grid
    pd3dImmediateContext->CSSetShader( g_pBuildGridCS, NULL, 0 );
    pd3dImmediateContext->Dispatch( g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1 );
//	CheckBuffer<LARGE_INTEGER>(g_pGrid);

    GPUSort(pd3dImmediateContext, g_iNumParticles, TRUE, g_pGridUAV, g_pGridSRV, g_pGridPingPongUAV, g_pGridPingPongSRV);
//	CheckBuffer<LARGE_INTEGER>(g_pGrid); 

    pd3dImmediateContext->CSSetConstantBuffers( g_iPNTRIANGLESCBBind, 1, &g_pcbPNTriangles );
	pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &g_pGridIndicesUAV, &UAVInitialCounts );
    pd3dImmediateContext->CSSetShaderResources( 5, 1, &g_pGridSRV );
	UINT pUINTClr[4] = {0};

	pd3dImmediateContext->ClearUnorderedAccessViewUint(g_pGridIndicesUAV, pUINTClr);
    pd3dImmediateContext->CSSetShader( g_pBuildGridIndicesCS, NULL, 0 );
    pd3dImmediateContext->Dispatch( g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1 );
//	CheckBuffer<LARGE_INTEGER>(g_pGridIndices);

    // Setup
    // Rearrange
    pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &g_pSortedParticlesUAV, &UAVInitialCounts );
    pd3dImmediateContext->CSSetShaderResources( 3, 1, &g_pParticlesSRV );
    pd3dImmediateContext->CSSetShaderResources( 5, 1, &g_pGridSRV );
	pd3dImmediateContext->CSSetShader( g_pRearrangeParticlesCS, NULL, 0 );
    pd3dImmediateContext->Dispatch( g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1 );
//	CheckBuffer<D3DXVECTOR3>(g_pSortedParticles);

    // Setup
    pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &g_pNullUAV, &UAVInitialCounts );
	pd3dImmediateContext->CSSetShaderResources( 2, 1, &g_pSRVField );
    pd3dImmediateContext->CSSetShaderResources( 3, 1, &g_pSortedParticlesSRV );
    pd3dImmediateContext->CSSetShaderResources( 5, 1, &g_pGridSRV );
    pd3dImmediateContext->CSSetShaderResources( 6, 1, &g_pGridIndicesSRV );

	pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &g_pParticlesUAV, &UAVInitialCounts );
	pd3dImmediateContext->CSSetShader( g_pVelocityCS, NULL, 0 );
	pd3dImmediateContext->Dispatch( g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1 );

	pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &g_pParticleDensityUAV, &UAVInitialCounts );
	pd3dImmediateContext->CSSetShader( g_pDensityCS, NULL, 0 );
	pd3dImmediateContext->Dispatch( g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1 );

	if(g_bSavePoints) {
		SavePointsBuffer();
		g_bSavePoints = FALSE;
	}

	if(g_bSaveSurfacePoints) {
		SavePointsBuffer(true);
		g_bSaveSurfacePoints = FALSE;
	}

	//CheckBuffer<D3DXVECTOR3>(g_pParticleDensity);
    pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &g_pNullUAV, &UAVInitialCounts );
	pd3dImmediateContext->CSSetShaderResources( 2, 1, &g_pNullSRV );
	pd3dImmediateContext->CSSetShaderResources( 3, 1, &g_pNullSRV );
    pd3dImmediateContext->CSSetShaderResources( 5, 1, &g_pNullSRV );
    pd3dImmediateContext->CSSetShaderResources( 6, 1, &g_pNullSRV );

}
//--------------------------------------------------------------------------------------
// Render
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                 float fElapsedTime, void* pUserContext )
{
    // If the settings dialog is being shown, then render it instead of rendering the app's scene
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.OnRender( fElapsedTime );
        return;
    }

    // Array of our samplers
    ID3D11SamplerState* ppSamplerStates[2] = { g_pSamplePoint, g_pSampleLinear };

    // Get the projection & view matrix from the camera class
    D3DXMATRIXA16 mWorld;
    D3DXMATRIXA16 mView;
    D3DXMATRIXA16 mProj;
    D3DXMATRIXA16 mWorldViewProjection;
    D3DXMATRIXA16 mViewProjection;
    mWorld =  g_m4x4MeshMatrix[g_eMeshType] * *g_Camera[g_eMeshType].GetWorldMatrix();
    mView = *g_Camera[g_eMeshType].GetViewMatrix();
    mProj = *g_Camera[g_eMeshType].GetProjMatrix();
    mWorldViewProjection = mWorld * mView * mProj;
    mViewProjection = mView * mProj;

    UINT UAVInitialCounts = 0;
    
    // Get the direction of the light.
    D3DXVECTOR3 v3LightDir = *g_LightCamera.GetEyePt() - *g_LightCamera.GetLookAtPt();
    D3DXVec3Normalize( &v3LightDir, &v3LightDir );

    // Calculate the plane equations of the frustum in world space
	FLOAT aspect = (FLOAT) DXUTGetWindowWidth() / (FLOAT) DXUTGetWindowHeight();

	UINT nSx = (UINT)(sqrtf((FLOAT)(g_iFieldSize[2]) * aspect));
	UINT nSy = (g_iFieldSize[2] / nSx) + (UINT)(g_iFieldSize[2] % nSx > 0);

	UINT nPix = DXUTGetWindowHeight() / nSy;
	FLOAT bX = (FLOAT) (nPix * nSx) / (FLOAT) DXUTGetWindowWidth();
	FLOAT bY = (FLOAT) (nPix * nSy) / (FLOAT) DXUTGetWindowHeight();

	FLOAT mSize = powf(g_SceneMesh[g_eMeshType].GetMeshBBoxExtents().x 
		* g_SceneMesh[g_eMeshType].GetMeshBBoxExtents().y 
		* g_SceneMesh[g_eMeshType].GetMeshBBoxExtents().z, 0.3333333f);

	FLOAT fSmoothlen = g_fSmoothlen * mSize * g_fKScale;

    // Setup the constant buffer for the scene vertex shader
    D3D11_MAPPED_SUBRESOURCE MappedResource;
    pd3dImmediateContext->Map( g_pcbPNTriangles, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource );
    CB_PNTRIANGLES* pPNTrianglesCB = ( CB_PNTRIANGLES* )MappedResource.pData;
    D3DXMatrixTranspose( &pPNTrianglesCB->f4x4World, &mWorld );
    D3DXMatrixTranspose( &pPNTrianglesCB->f4x4ViewProjection, &mViewProjection );
    D3DXMatrixTranspose( &pPNTrianglesCB->f4x4WorldViewProjection, &mWorldViewProjection );
    pPNTrianglesCB->fLightDir[0] = v3LightDir.x; 
    pPNTrianglesCB->fLightDir[1] = v3LightDir.y; 
    pPNTrianglesCB->fLightDir[2] = v3LightDir.z; 
    pPNTrianglesCB->fLightDir[3] = 0.0f;
    pPNTrianglesCB->fEye[0] = g_Camera[g_eMeshType].GetEyePt()->x;
    pPNTrianglesCB->fEye[1] = g_Camera[g_eMeshType].GetEyePt()->y;
    pPNTrianglesCB->fEye[2] = g_Camera[g_eMeshType].GetEyePt()->z;
    pPNTrianglesCB->fTessFactors[0] = (float)g_uTessFactor;
	pPNTrianglesCB->fBoundBoxMin[0] = g_SceneMesh[g_eMeshType].GetMeshBBoxCenter().x - g_SceneMesh[g_eMeshType].GetMeshBBoxExtents().x;
	pPNTrianglesCB->fBoundBoxMin[1] = g_SceneMesh[g_eMeshType].GetMeshBBoxCenter().y - g_SceneMesh[g_eMeshType].GetMeshBBoxExtents().y;
	pPNTrianglesCB->fBoundBoxMin[2] = g_SceneMesh[g_eMeshType].GetMeshBBoxCenter().z - g_SceneMesh[g_eMeshType].GetMeshBBoxExtents().z;
	pPNTrianglesCB->fBoundBoxMin[3] = 0;
	pPNTrianglesCB->fBoundBoxMax[0] = g_SceneMesh[g_eMeshType].GetMeshBBoxCenter().x + g_SceneMesh[g_eMeshType].GetMeshBBoxExtents().x;
	pPNTrianglesCB->fBoundBoxMax[1] = g_SceneMesh[g_eMeshType].GetMeshBBoxCenter().y + g_SceneMesh[g_eMeshType].GetMeshBBoxExtents().y;
	pPNTrianglesCB->fBoundBoxMax[2] = g_SceneMesh[g_eMeshType].GetMeshBBoxCenter().z + g_SceneMesh[g_eMeshType].GetMeshBBoxExtents().z;
	pPNTrianglesCB->fBoundBoxMax[3] = 0;
	pPNTrianglesCB->fBoundSize[0] = (g_SceneMesh[g_eMeshType].GetMeshBBoxExtents().x);
	pPNTrianglesCB->fBoundSize[1] = (g_SceneMesh[g_eMeshType].GetMeshBBoxExtents().y);
	pPNTrianglesCB->fBoundSize[2] = (g_SceneMesh[g_eMeshType].GetMeshBBoxExtents().z * 2.0f) / (FLOAT) g_iFieldSize[2];
	pPNTrianglesCB->fBoundSize[3] = 2.0f / (FLOAT)g_iFieldSize[2];
	pPNTrianglesCB->fInvBoundSize[0] = 1.0f / (g_SceneMesh[g_eMeshType].GetMeshBBoxExtents().x);
	pPNTrianglesCB->fInvBoundSize[1] = 1.0f / (g_SceneMesh[g_eMeshType].GetMeshBBoxExtents().y);
	pPNTrianglesCB->fInvBoundSize[2] = (FLOAT) g_iFieldSize[2] / (g_SceneMesh[g_eMeshType].GetMeshBBoxExtents().z * 2.0f);
	pPNTrianglesCB->fInvBoundSize[3] = 0.5f / mSize; 
	pPNTrianglesCB->fVolSlicing[0] = nSx;
	pPNTrianglesCB->fVolSlicing[1] = nSy;
	pPNTrianglesCB->fVolSlicing[2] = bX;
	pPNTrianglesCB->fVolSlicing[3] = bY;
	pPNTrianglesCB->fParticleParameter[0] = g_fParticleRenderSize * mSize * 0.5f;
	pPNTrianglesCB->fParticleParameter[1] = g_fParticleAspectRatio;
	pPNTrianglesCB->fParticleParameter[2] = g_fNormalScalar;
	pPNTrianglesCB->fParticleParameter[3] = g_fSurface; 
	pPNTrianglesCB->fGridDim[0] = NUM_GRID_DIM_X;
	pPNTrianglesCB->fGridDim[1] = NUM_GRID_DIM_Y;
	pPNTrianglesCB->fGridDim[2] = NUM_GRID_DIM_Z;
	pPNTrianglesCB->fGridDim[3] = 0;
	pPNTrianglesCB->fInvGridDim[0] = (float)1.0f / (g_SceneMesh[g_eMeshType].GetMeshBBoxExtents().x * 2.0f);
	pPNTrianglesCB->fInvGridDim[1] = (float)1.0f / (g_SceneMesh[g_eMeshType].GetMeshBBoxExtents().y * 2.0f);
	pPNTrianglesCB->fInvGridDim[2] = (float)1.0f / (g_SceneMesh[g_eMeshType].GetMeshBBoxExtents().z * 2.0f);
	pPNTrianglesCB->fInvGridDim[3] = g_iNumParticles;
	pPNTrianglesCB->iGridDot[0] = NUM_GRID_DIM_Z * NUM_GRID_DIM_X;
	pPNTrianglesCB->iGridDot[1] = NUM_GRID_DIM_X;
	pPNTrianglesCB->iGridDot[2] = 1;
	pPNTrianglesCB->iGridDot[3] = g_iNumParticles;
	pPNTrianglesCB->fKernel[0] = fSmoothlen * fSmoothlen;
	pPNTrianglesCB->fKernel[1] = g_fSpeed;
	pPNTrianglesCB->fKernel[2] = mSize / powf(g_iFieldSize[0] * g_iFieldSize[1] * g_iFieldSize[2], 0.333333f);
	pPNTrianglesCB->fKernel[3] = g_fParticleMass * 315.0f / (64.0f * D3DX_PI * pow(fSmoothlen, 9));;
    pd3dImmediateContext->Unmap( g_pcbPNTriangles, 0 );
    pd3dImmediateContext->VSSetConstantBuffers( g_iPNTRIANGLESCBBind, 1, &g_pcbPNTriangles );
    pd3dImmediateContext->PSSetConstantBuffers( g_iPNTRIANGLESCBBind, 1, &g_pcbPNTriangles );
    pd3dImmediateContext->GSSetConstantBuffers( g_iPNTRIANGLESCBBind, 1, &g_pcbPNTriangles );
    pd3dImmediateContext->CSSetConstantBuffers( g_iPNTRIANGLESCBBind, 1, &g_pcbPNTriangles );
    // Based on app and GUI settings set a bunch of bools that guide the render
    bool bTessellation = true;
    bool bTextured = false;

	if(!g_bFieldUpdated) {
		D3D11_VIEWPORT vp;
		vp.MinDepth = 0;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = vp.TopLeftY = 0;
		vp.Width = g_iFieldSize[0];
		vp.Height = g_iFieldSize[1];
		pd3dImmediateContext->RSSetViewports(1, &vp);

		pd3dImmediateContext->ClearDepthStencilView(g_pDSVField, D3D11_CLEAR_DEPTH, 1.0f, 0);
		FLOAT clrSpc[4] = {0.0f, 0.0f, 0.0f, mSize};
		pd3dImmediateContext->ClearRenderTargetView(g_pRTVFieldProxy, clrSpc);
		pd3dImmediateContext->OMSetRenderTargets(1, &g_pRTVFieldProxy, g_pDSVField);

		// VS
		if( bTessellation )
		{
			pd3dImmediateContext->VSSetShader( g_pSceneWithTessellationVS, NULL, 0 );
		}
		else
		{
			pd3dImmediateContext->VSSetShader( g_pSceneVS, NULL, 0 );
		}
		pd3dImmediateContext->IASetInputLayout( g_pSceneVertexLayout );

		// HS
		ID3D11HullShader* pHS = NULL;
		if( bTessellation )
		{
			pd3dImmediateContext->HSSetConstantBuffers( g_iPNTRIANGLESCBBind, 1, &g_pcbPNTriangles );
			pHS = g_pPNTrianglesHS;
		}
		pd3dImmediateContext->HSSetShader( pHS, NULL, 0 );    
    
		// DS
		ID3D11DomainShader* pDS = NULL;
		if( bTessellation )
		{
			pd3dImmediateContext->DSSetConstantBuffers( g_iPNTRIANGLESCBBind, 1, &g_pcbPNTriangles );
			pDS = g_pPNTrianglesDS;
		}
		pd3dImmediateContext->DSSetShader( pDS, NULL, 0 );
    
		// GS
		pd3dImmediateContext->GSSetShader( g_pFieldGS, NULL, 0 );
		pd3dImmediateContext->GSSetShaderResources(2, 1, &g_pSRVField);

		// PS
		ID3D11PixelShader* pPS = NULL;
		pd3dImmediateContext->PSSetSamplers( 0, 2, ppSamplerStates );
		pd3dImmediateContext->CSSetSamplers( 0, 2, ppSamplerStates );
		pd3dImmediateContext->PSSetShader( g_pFieldPS, NULL, 0 );

		// Set the rasterizer state
		pd3dImmediateContext->RSSetState( g_pRasterizerStateSolid );

		// Render the scene and optionally override the mesh topology and diffuse texture slot
		// Decide whether to use the user diffuse.dds
		UINT uDiffuseSlot = 0;
		// Decide which prim topology to use
		D3D11_PRIMITIVE_TOPOLOGY PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
		if( bTessellation )
		{
			PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
		}
		// Render the meshes    

		g_SceneMesh[g_eMeshType].Render(pd3dImmediateContext, PrimitiveTopology);
 
		pd3dImmediateContext->HSSetShader(NULL, NULL, 0);
		pd3dImmediateContext->DSSetShader(NULL, NULL, 0);
		pd3dImmediateContext->GSSetShader(NULL, NULL, 0);
		pd3dImmediateContext->GSSetShaderResources(2, 1, &g_pNullSRV);

		ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
		ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();

		pd3dImmediateContext->OMSetRenderTargets(1, &pRTV, NULL);
		vp.Width = DXUTGetWindowWidth();
		vp.Height = DXUTGetWindowHeight();
		pd3dImmediateContext->RSSetViewports(1, &vp);

		pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &g_pUAVField, &UAVInitialCounts);
		pd3dImmediateContext->CSSetShaderResources(1, 1, &g_pSRVFieldProxy);
		pd3dImmediateContext->CSSetShader(g_pArrayTo3DCS, NULL, 0);
		pd3dImmediateContext->Dispatch(g_iFieldSize[0] / 16, g_iFieldSize[1] / 16, g_iFieldSize[2]);
		pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &g_pNullUAV, &UAVInitialCounts);
		pd3dImmediateContext->CSSetShaderResources(1, 1, &g_pNullSRV);

		g_bFieldUpdated = TRUE;
	}

	if(!g_bNoSimulating)
		SimulateFluid_Grid( pd3dImmediateContext );
	//VisualizeField( pd3dImmediateContext );
	RenderFluid( pd3dImmediateContext );
    // Render GUI
    if( g_CmdLineParams.bRenderHUD )
    {
        DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" );

        g_HUD.OnRender( fElapsedTime );
        g_SampleUI.OnRender( fElapsedTime );
        
        DXUT_EndPerfEvent();
    }

    // Always render text info 
    RenderText();

    // Decrement the exit frame counter
    g_CmdLineParams.iExitFrame--;

    // Exit on this frame
    if( g_CmdLineParams.iExitFrame == 0 )
    {
        if( g_CmdLineParams.bCapture )
        {
            CaptureFrame();
        }

        ::PostMessage( DXUTGetHWND(), WM_QUIT, 0, 0 );
    }
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11DestroyDevice();
    g_D3DSettingsDlg.OnD3D11DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();
    SAFE_DELETE( g_pTxtHelper );

    for( int iMeshType=0; iMeshType<MESH_TYPE_MAX; iMeshType++ )
    {
		g_SceneMesh[iMeshType].Release();
    }

    SAFE_RELEASE( g_pSceneVS );
    SAFE_RELEASE( g_pSceneWithTessellationVS );
    SAFE_RELEASE( g_pPNTrianglesHS );
    SAFE_RELEASE( g_pPNTrianglesDS );
    SAFE_RELEASE( g_pScenePS );
	SAFE_RELEASE( g_pFieldGS );
	SAFE_RELEASE( g_pFieldPS );
    SAFE_RELEASE( g_pTexturedScenePS );
	SAFE_RELEASE( g_pVisualizeVS );
	SAFE_RELEASE( g_pVisualizePS );
        
    SAFE_RELEASE( g_pcbPNTriangles );
    SAFE_RELEASE( g_pSortCB );

    SAFE_RELEASE( g_pCaptureTexture );

    SAFE_RELEASE( g_pSceneVertexLayout );
    
    SAFE_RELEASE( g_pSamplePoint );
    SAFE_RELEASE( g_pSampleLinear );

    SAFE_RELEASE( g_pRasterizerStateWireframe );
    SAFE_RELEASE( g_pRasterizerStateSolid );

    SAFE_RELEASE( g_pDiffuseTextureSRV );
	SAFE_RELEASE( g_pSRVField );
	SAFE_RELEASE( g_pUAVField );
	SAFE_RELEASE( g_pDSVField );
	SAFE_RELEASE( g_pTexField );
	SAFE_RELEASE( g_pTexFieldDepth );
	SAFE_RELEASE( g_pTexFieldProxy );
	SAFE_RELEASE( g_pSRVFieldProxy );
	SAFE_RELEASE( g_pRTVFieldProxy );

	
    SAFE_RELEASE( g_pParticles );
    SAFE_RELEASE( g_pParticlesSRV );
    SAFE_RELEASE( g_pParticlesUAV );

    SAFE_RELEASE( g_pSortedParticles );
    SAFE_RELEASE( g_pSortedParticlesSRV );
    SAFE_RELEASE( g_pSortedParticlesUAV );

    SAFE_RELEASE( g_pParticleDensity );
    SAFE_RELEASE( g_pParticleDensitySRV );
    SAFE_RELEASE( g_pParticleDensityUAV );

    SAFE_RELEASE( g_pGridSRV );
    SAFE_RELEASE( g_pGridUAV );
    SAFE_RELEASE( g_pGrid );

    SAFE_RELEASE( g_pGridPingPongSRV );
    SAFE_RELEASE( g_pGridPingPongUAV );
    SAFE_RELEASE( g_pGridPingPong );

    SAFE_RELEASE( g_pGridIndicesSRV );
    SAFE_RELEASE( g_pGridIndicesUAV );
    SAFE_RELEASE( g_pGridIndices );

	SAFE_RELEASE( g_pParticleVS );
	SAFE_RELEASE( g_pParticleGS );
	SAFE_RELEASE( g_pParticlePS );

	SAFE_RELEASE( g_pBSAlpha );

    SAFE_RELEASE( g_pSortBitonic );
    SAFE_RELEASE( g_pSortTranspose );
	SAFE_RELEASE( g_pSortBitonicUint );
    SAFE_RELEASE( g_pSortTransposeUint );

    SAFE_RELEASE( g_pBuildGridCS );
    SAFE_RELEASE( g_pBuildGridIndicesCS );
    SAFE_RELEASE( g_pRearrangeParticlesCS );
    SAFE_RELEASE( g_pVelocityCS );
    SAFE_RELEASE( g_pDensityCS );

	SAFE_RELEASE( g_pArrayTo3DCS );
}


//--------------------------------------------------------------------------------------
// Release swap chain
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11ReleasingSwapChain();

    SAFE_RELEASE( g_pCaptureTexture );
}


HRESULT CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, 
	LPCSTR szShaderModel, ID3DBlob** ppBlobOut, D3D_SHADER_MACRO* pDefines )
{
	HRESULT hr = S_OK;
	WCHAR str[MAX_PATH];
	char strc[MAX_PATH];
	// Get the Compiling Flag
	sprintf_s(strc, MAX_PATH, "%s.fxc", szEntryPoint);

	printf("Loading Shader [%s]...\n", szEntryPoint);
	HANDLE hfxc = CreateFileA(strc, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if(INVALID_HANDLE_VALUE != hfxc) {
		//fxc exist, check the date
		FILETIME fxctCreate, fxctAccess, fxctWrite;
		GetFileTime(hfxc, &fxctCreate, &fxctAccess, &fxctWrite );

		V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, szFileName ) );

		HANDLE hfx = CreateFileW(str, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);

		FILETIME fxtCreate, fxtAccess, fxtWrite;

		GetFileTime(hfx, &fxtCreate, &fxtAccess, &fxtWrite );

		if(fxtWrite.dwHighDateTime == fxctWrite.dwHighDateTime && fxtWrite.dwLowDateTime == fxctWrite.dwLowDateTime) {
			//fxc date is correct, load directly
			CloseHandle(hfx);
			LARGE_INTEGER fs;
			GetFileSizeEx(hfxc, &fs);
			D3DCreateBlob(fs.LowPart, ppBlobOut);
			DWORD fsr;
			ReadFile(hfxc, (*ppBlobOut)->GetBufferPointer(), fs.LowPart, &fsr, NULL);

			assert(fs.LowPart == fsr);
			CloseHandle(hfxc);
			return S_OK;
		} else {
			CloseHandle(hfx);
			CloseHandle(hfxc);
		}
	} else {
		V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, szFileName ) );
	}

	// find the file
	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;
#else
	dwShaderFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

	printf("Compiling Shader...%s", szEntryPoint);
	ID3DBlob* pErrorBlob;
	hr = D3DX11CompileFromFile( str, pDefines, NULL, szEntryPoint, szShaderModel, 
		dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL );
	if( FAILED(hr) )
	{
		if( pErrorBlob != NULL )
			printf((char*)pErrorBlob->GetBufferPointer());
		SAFE_RELEASE( pErrorBlob );
		return hr;
	} else {
		printf("Done!\n");
	}
	SAFE_RELEASE( pErrorBlob );

	//Save Compiled Code
	hfxc = CreateFileA(strc, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, NULL, NULL);

	DWORD fsw;
	WriteFile(hfxc, (*ppBlobOut)->GetBufferPointer(), (*ppBlobOut)->GetBufferSize(), &fsw, NULL);

	assert(fsw == (*ppBlobOut)->GetBufferSize());

	HANDLE hfx = CreateFileW(str, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);

	FILETIME fxtCreate, fxtAccess, fxtWrite;

	GetFileTime(hfx, &fxtCreate, &fxtAccess, &fxtWrite );

	SetFileTime(hfxc, &fxtCreate, &fxtAccess, &fxtWrite);

	CloseHandle(hfxc);
	CloseHandle(hfx);

	Sleep(100);

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Helper function for command line retrieval
//--------------------------------------------------------------------------------------
bool IsNextArg( WCHAR*& strCmdLine, WCHAR* strArg )
{
   int nArgLen = (int) wcslen(strArg);
   int nCmdLen = (int) wcslen(strCmdLine);

   if( nCmdLen >= nArgLen && 
      _wcsnicmp( strCmdLine, strArg, nArgLen ) == 0 && 
      (strCmdLine[nArgLen] == 0 || strCmdLine[nArgLen] == L':' || strCmdLine[nArgLen] == L'=' ) )
   {
      strCmdLine += nArgLen;
      return true;
   }

   return false;
}


//--------------------------------------------------------------------------------------
// Helper function for command line retrieval.  Updates strCmdLine and strFlag 
//      Example: if strCmdLine=="-width:1024 -forceref"
// then after: strCmdLine==" -forceref" and strFlag=="1024"
//--------------------------------------------------------------------------------------
bool GetCmdParam( WCHAR*& strCmdLine, WCHAR* strFlag )
{
   if( *strCmdLine == L':' || *strCmdLine == L'=' )
   {       
      strCmdLine++; // Skip ':'

      // Place NULL terminator in strFlag after current token
      wcscpy_s( strFlag, 256, strCmdLine );
      WCHAR* strSpace = strFlag;
      while (*strSpace && (*strSpace > L' '))
         strSpace++;
      *strSpace = 0;

      // Update strCmdLine
      strCmdLine += wcslen(strFlag);
      return true;
   }
   else
   {
      strFlag[0] = 0;
      return false;
   }
}


//--------------------------------------------------------------------------------------
// Helper function to parse the command line
//--------------------------------------------------------------------------------------
void ParseCommandLine()
{
    // set some defaults
    g_CmdLineParams.uWidth = 1024;
    g_CmdLineParams.uHeight = 768;
    g_CmdLineParams.bCapture = false;
    swprintf_s( g_CmdLineParams.strCaptureFilename, L"FrameCapture.bmp" );
    g_CmdLineParams.iExitFrame = -1;
    g_CmdLineParams.bRenderHUD = true;

    // Perform application-dependant command line processing
    WCHAR* strCmdLine = GetCommandLine();
    WCHAR strFlag[MAX_PATH];
    int nNumArgs;
    WCHAR** pstrArgList = CommandLineToArgvW( strCmdLine, &nNumArgs );
    for( int iArg=1; iArg<nNumArgs; iArg++ )
    {
        strCmdLine = pstrArgList[iArg];

        // Handle flag args
        if( *strCmdLine == L'/' || *strCmdLine == L'-' )
        {
            strCmdLine++;

            if( IsNextArg( strCmdLine, L"width" ) )
            {
                if( GetCmdParam( strCmdLine, strFlag ) )
                {
                   g_CmdLineParams.uWidth = _wtoi(strFlag);
                }
                continue;
            }

            if( IsNextArg( strCmdLine, L"height" ) )
            {
                if( GetCmdParam( strCmdLine, strFlag ) )
                {
                   g_CmdLineParams.uHeight = _wtoi(strFlag);
                }
                continue;
            }

            if( IsNextArg( strCmdLine, L"capturefilename" ) )
            {
                if( GetCmdParam( strCmdLine, strFlag ) )
                {
                   swprintf_s( g_CmdLineParams.strCaptureFilename, L"%s", strFlag );
                   g_CmdLineParams.bCapture = true;
                }
                continue;
            }

            if( IsNextArg( strCmdLine, L"nogui" ) )
            {
                g_CmdLineParams.bRenderHUD = false;
                continue;
            }

            if( IsNextArg( strCmdLine, L"exitframe" ) )
            {
                if( GetCmdParam( strCmdLine, strFlag ) )
                {
                   g_CmdLineParams.iExitFrame = _wtoi(strFlag);
                }
                continue;
            }
        }
    }
}

//--------------------------------------------------------------------------------------
// Helper function to capture a frame and dump it to disk 
//--------------------------------------------------------------------------------------
void CaptureFrame()
{
    // Retrieve RT resource
    ID3D11Resource *pRTResource;
    DXUTGetD3D11RenderTargetView()->GetResource(&pRTResource);

    // Retrieve a Texture2D interface from resource
    ID3D11Texture2D* RTTexture;
    pRTResource->QueryInterface( __uuidof( ID3D11Texture2D ), ( LPVOID* )&RTTexture);

    // Check if RT is multisampled or not
    D3D11_TEXTURE2D_DESC    TexDesc;
    RTTexture->GetDesc(&TexDesc);
    if (TexDesc.SampleDesc.Count>1)
    {
        // RT is multisampled, need resolving before dumping to disk

        // Create single-sample RT of the same type and dimensions
        DXGI_SAMPLE_DESC SingleSample = { 1, 0 };
        TexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        TexDesc.MipLevels = 1;
        TexDesc.Usage = D3D11_USAGE_DEFAULT;
        TexDesc.CPUAccessFlags = 0;
        TexDesc.BindFlags = 0;
        TexDesc.SampleDesc = SingleSample;

        ID3D11Texture2D *pSingleSampleTexture;
        DXUTGetD3D11Device()->CreateTexture2D(&TexDesc, NULL, &pSingleSampleTexture );
        DXUT_SetDebugName( pSingleSampleTexture, "Single Sample" );

        DXUTGetD3D11DeviceContext()->ResolveSubresource(pSingleSampleTexture, 0, RTTexture, 0, TexDesc.Format );

        // Copy RT into STAGING texture
        DXUTGetD3D11DeviceContext()->CopyResource(g_pCaptureTexture, pSingleSampleTexture);

        D3DX11SaveTextureToFile(DXUTGetD3D11DeviceContext(), g_pCaptureTexture, D3DX11_IFF_BMP, g_CmdLineParams.strCaptureFilename );

        SAFE_RELEASE(pSingleSampleTexture);
        
    }
    else
    {
        // Single sample case

        // Copy RT into STAGING texture
        DXUTGetD3D11DeviceContext()->CopyResource(g_pCaptureTexture, pRTResource);

        D3DX11SaveTextureToFile(DXUTGetD3D11DeviceContext(), g_pCaptureTexture, D3DX11_IFF_BMP, g_CmdLineParams.strCaptureFilename );
    }

    SAFE_RELEASE(RTTexture);

    SAFE_RELEASE(pRTResource);
}


//--------------------------------------------------------------------------------------
// Helper function that allows the app to render individual meshes of an sdkmesh
// and override the primitive topology
//--------------------------------------------------------------------------------------
void RenderMesh( CDXUTSDKMesh* pDXUTMesh, UINT uMesh, D3D11_PRIMITIVE_TOPOLOGY PrimType, 
                UINT uDiffuseSlot, UINT uNormalSlot, UINT uSpecularSlot )
{
    #define MAX_D3D11_VERTEX_STREAMS D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT

    assert( NULL != pDXUTMesh );

    if( 0 < pDXUTMesh->GetOutstandingBufferResources() )
    {
        return;
    }

    SDKMESH_MESH* pMesh = pDXUTMesh->GetMesh( uMesh );

    UINT Strides[MAX_D3D11_VERTEX_STREAMS];
    UINT Offsets[MAX_D3D11_VERTEX_STREAMS];
    ID3D11Buffer* pVB[MAX_D3D11_VERTEX_STREAMS];

    if( pMesh->NumVertexBuffers > MAX_D3D11_VERTEX_STREAMS )
    {
        return;
    }

    for( UINT64 i = 0; i < pMesh->NumVertexBuffers; i++ )
    {
        pVB[i] = pDXUTMesh->GetVB11( uMesh, (UINT)i );
        Strides[i] = pDXUTMesh->GetVertexStride( uMesh, (UINT)i );
        Offsets[i] = 0;
    }

    ID3D11Buffer* pIB;
    pIB = pDXUTMesh->GetIB11( uMesh );
    DXGI_FORMAT ibFormat = pDXUTMesh->GetIBFormat11( uMesh );
    
    DXUTGetD3D11DeviceContext()->IASetVertexBuffers( 0, pMesh->NumVertexBuffers, pVB, Strides, Offsets );
    DXUTGetD3D11DeviceContext()->IASetIndexBuffer( pIB, ibFormat, 0 );

    SDKMESH_SUBSET* pSubset = NULL;
    SDKMESH_MATERIAL* pMat = NULL;

    for( UINT uSubset = 0; uSubset < pMesh->NumSubsets; uSubset++ )
    {
        pSubset = pDXUTMesh->GetSubset( uMesh, uSubset );

        if( D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED == PrimType )
        {
            PrimType = pDXUTMesh->GetPrimitiveType11( ( SDKMESH_PRIMITIVE_TYPE )pSubset->PrimitiveType );
        }
        
        DXUTGetD3D11DeviceContext()->IASetPrimitiveTopology( PrimType );

        pMat = pDXUTMesh->GetMaterial( pSubset->MaterialID );
        if( uDiffuseSlot != INVALID_SAMPLER_SLOT && !IsErrorResource( pMat->pDiffuseRV11 ) )
        {
            DXUTGetD3D11DeviceContext()->PSSetShaderResources( uDiffuseSlot, 1, &pMat->pDiffuseRV11 );
        }

        if( uNormalSlot != INVALID_SAMPLER_SLOT && !IsErrorResource( pMat->pNormalRV11 ) )
        {
            DXUTGetD3D11DeviceContext()->PSSetShaderResources( uNormalSlot, 1, &pMat->pNormalRV11 );
        }

        if( uSpecularSlot != INVALID_SAMPLER_SLOT && !IsErrorResource( pMat->pSpecularRV11 ) )
        {
            DXUTGetD3D11DeviceContext()->PSSetShaderResources( uSpecularSlot, 1, &pMat->pSpecularRV11 );
        }

        UINT IndexCount = ( UINT )pSubset->IndexCount;
        UINT IndexStart = ( UINT )pSubset->IndexStart;
        UINT VertexStart = ( UINT )pSubset->VertexStart;
        
        DXUTGetD3D11DeviceContext()->DrawIndexed( IndexCount, IndexStart, VertexStart );
    }
}


//--------------------------------------------------------------------------------------
// Helper function to check for file existance
//--------------------------------------------------------------------------------------
bool FileExists( WCHAR* pFileName )
{
    DWORD fileAttr;    
    fileAttr = GetFileAttributes(pFileName);    
    if (0xFFFFFFFF == fileAttr)        
        return false;    
    return true;
}


//--------------------------------------------------------------------------------------
// Creates the appropriate HS based upon current GUI settings
//--------------------------------------------------------------------------------------
HRESULT CreateHullShader()
{
    HRESULT hr;

    // Release any existing shader
    SAFE_RELEASE( g_pPNTrianglesHS );
    
    // Create the shaders
    ID3DBlob* pBlob = NULL;
    
    int i = 0;

    DWORD switches = 0;

    // Create the shader
    hr = CompileShaderFromFile( L"Mesh2Points.hlsl", "HS_PNTriangles", "hs_5_0", &pBlob, NULL); 
    if ( FAILED(hr) )
        return hr;

    hr = DXUTGetD3D11Device()->CreateHullShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pPNTrianglesHS );
    if ( FAILED(hr) )
        return hr;
    SAFE_RELEASE( pBlob );

#if defined(DEBUG) || defined(PROFILE)
    char swstr[64];
    sprintf_s( swstr, sizeof(swstr), "Hull (GUI settings %x)\n", switches );
    DXUT_SetDebugName( g_pPNTrianglesHS, swstr );
#endif

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Helper function to normalize a plane
//--------------------------------------------------------------------------------------
void NormalizePlane( D3DXVECTOR4* pPlaneEquation )
{
    float mag;
    
    mag = sqrt( pPlaneEquation->x * pPlaneEquation->x + 
                pPlaneEquation->y * pPlaneEquation->y + 
                pPlaneEquation->z * pPlaneEquation->z );
    
    pPlaneEquation->x = pPlaneEquation->x / mag;
    pPlaneEquation->y = pPlaneEquation->y / mag;
    pPlaneEquation->z = pPlaneEquation->z / mag;
    pPlaneEquation->w = pPlaneEquation->w / mag;
}


//--------------------------------------------------------------------------------------
// Extract all 6 plane equations from frustum denoted by supplied matrix
//--------------------------------------------------------------------------------------
void ExtractPlanesFromFrustum( D3DXVECTOR4* pPlaneEquation, const D3DXMATRIX* pMatrix )
{
    // Left clipping plane
    pPlaneEquation[0].x = pMatrix->_14 + pMatrix->_11;
    pPlaneEquation[0].y = pMatrix->_24 + pMatrix->_21;
    pPlaneEquation[0].z = pMatrix->_34 + pMatrix->_31;
    pPlaneEquation[0].w = pMatrix->_44 + pMatrix->_41;
    
    // Right clipping plane
    pPlaneEquation[1].x = pMatrix->_14 - pMatrix->_11;
    pPlaneEquation[1].y = pMatrix->_24 - pMatrix->_21;
    pPlaneEquation[1].z = pMatrix->_34 - pMatrix->_31;
    pPlaneEquation[1].w = pMatrix->_44 - pMatrix->_41;
    
    // Top clipping plane
    pPlaneEquation[2].x = pMatrix->_14 - pMatrix->_12;
    pPlaneEquation[2].y = pMatrix->_24 - pMatrix->_22;
    pPlaneEquation[2].z = pMatrix->_34 - pMatrix->_32;
    pPlaneEquation[2].w = pMatrix->_44 - pMatrix->_42;
    
    // Bottom clipping plane
    pPlaneEquation[3].x = pMatrix->_14 + pMatrix->_12;
    pPlaneEquation[3].y = pMatrix->_24 + pMatrix->_22;
    pPlaneEquation[3].z = pMatrix->_34 + pMatrix->_32;
    pPlaneEquation[3].w = pMatrix->_44 + pMatrix->_42;
    
    // Near clipping plane
    pPlaneEquation[4].x = pMatrix->_13;
    pPlaneEquation[4].y = pMatrix->_23;
    pPlaneEquation[4].z = pMatrix->_33;
    pPlaneEquation[4].w = pMatrix->_43;
    
    // Far clipping plane
    pPlaneEquation[5].x = pMatrix->_14 - pMatrix->_13;
    pPlaneEquation[5].y = pMatrix->_24 - pMatrix->_23;
    pPlaneEquation[5].z = pMatrix->_34 - pMatrix->_33;
    pPlaneEquation[5].w = pMatrix->_44 - pMatrix->_43;
    
    // Normalize the plane equations, if requested
    NormalizePlane( &pPlaneEquation[0] );
    NormalizePlane( &pPlaneEquation[1] );
    NormalizePlane( &pPlaneEquation[2] );
    NormalizePlane( &pPlaneEquation[3] );
    NormalizePlane( &pPlaneEquation[4] );
    NormalizePlane( &pPlaneEquation[5] );
}

bool ShowLoadDlg( WCHAR* szfn, const WCHAR* szfilter)
{
	OPENFILENAME ofn ;

	ZeroMemory( &ofn , sizeof( ofn));
	ofn.lStructSize = sizeof ( ofn );
	ofn.hwndOwner = DXUTGetHWND() ;
	ofn.lpstrFile = szfn ;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = szfilter;
	ofn.nFilterIndex =1;
	ofn.lpstrFileTitle = NULL ;
	ofn.nMaxFileTitle = 0 ;
	ofn.lpstrInitialDir=NULL ;
	ofn.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST ;
	return GetOpenFileName( &ofn );
}

bool ShowSaveDlg( WCHAR* szfn, const WCHAR* szfilter)
{
	OPENFILENAME ofn ;

	ZeroMemory( &ofn , sizeof( ofn));
	ofn.lStructSize = sizeof ( ofn );
	ofn.hwndOwner = DXUTGetHWND();
	ofn.lpstrFile = szfn ;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = szfilter;
	ofn.nFilterIndex =1;
	ofn.lpstrFileTitle = NULL ;
	ofn.nMaxFileTitle = 0 ;
	ofn.lpstrInitialDir=NULL ;
	ofn.Flags = OFN_PATHMUSTEXIST ;
	return GetSaveFileName(&ofn);

}

HRESULT ResetGeometry()
{
	HRESULT hr;
	g_bFieldUpdated = FALSE;
	printf("Creating Geometries...\n");

	ID3D11Device* pd3dDevice = DXUTGetD3D11Device();
	ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext();
    // Load the standard scene meshes
    WCHAR str[MAX_PATH];
	V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH,  g_default_mesh_fn ) );
	V_RETURN( g_SceneMesh[MESH_TYPE_TIGER].Create( str, pd3dDevice, pd3dImmediateContext ) );

                
	    // Setup the camera for each scene   
    D3DXVECTOR3 vecEye( 0.0f, 0.0f, 0.0f );
    D3DXVECTOR3 vecAt ( 0.0f, 0.0f, 0.0f );
    
	for(UINT i = 0; i < MESH_TYPE_MAX; i++) 
	{
		FLOAT Rb = powf(g_SceneMesh[i].GetMeshBBoxExtents().x 
			* g_SceneMesh[i].GetMeshBBoxExtents().y 
			* g_SceneMesh[i].GetMeshBBoxExtents().z, 0.3333333f);
		vecEye.x = g_SceneMesh[i].GetMeshBBoxCenter().x; 
		vecEye.y = g_SceneMesh[i].GetMeshBBoxCenter().y; 
		vecEye.z = g_SceneMesh[i].GetMeshBBoxCenter().z - Rb * 3.0f;
		vecAt = g_SceneMesh[i].GetMeshBBoxCenter();
		g_Camera[i].SetViewParams( &vecEye, &vecAt );
	}


	// Setup the light camera
    vecEye.x = 0.0f; vecEye.y = -1.0f; vecEye.z = -1.0f;
    g_LightCamera.SetViewParams( &vecEye, &vecAt );

	V_RETURN(CreateSimulationBuffers(pd3dDevice));
	
	return S_OK;
}

HRESULT ResetParticles()
{
	HRESULT hr = S_OK;

	printf("Creating Particles...\n");

    SAFE_RELEASE( g_pParticles );
    SAFE_RELEASE( g_pParticlesSRV );
    SAFE_RELEASE( g_pParticlesUAV );

    SAFE_RELEASE( g_pSortedParticles );
    SAFE_RELEASE( g_pSortedParticlesSRV );
    SAFE_RELEASE( g_pSortedParticlesUAV );

	ID3D11Device* pd3dDevice = DXUTGetD3D11Device();
	FLOAT Rb = powf(g_SceneMesh[g_eMeshType].GetMeshBBoxExtents().x
		* g_SceneMesh[g_eMeshType].GetMeshBBoxExtents().y
		* g_SceneMesh[g_eMeshType].GetMeshBBoxExtents().z, 0.3333333f) * 0.01f;
	srand(GetTickCount());

	D3DXVECTOR4* particles = new D3DXVECTOR4[g_iNumParticles];
	const std::vector<MeshObj::VERTEX>& vert_ref = g_SceneMesh[g_eMeshType].GetStoredVertices();
	UINT nref = vert_ref.size();

	for(UINT i = 0; i < g_iNumParticles; i++) 
	{
		FLOAT r = (float)rand() / (float)(RAND_MAX) * Rb;
		FLOAT theta = (float)rand() / (float)(RAND_MAX) * D3DX_PI;
		FLOAT phi = (float)rand() / (float)(RAND_MAX) * D3DX_PI * 2.0f;
		FLOAT st = sinf(theta);
		FLOAT ct = cosf(theta);
		FLOAT sp = sinf(phi);
		FLOAT cp = cosf(phi);

		particles[i].x = r * st * cp + g_SceneMesh[g_eMeshType].GetMeshBBoxCenter().x + g_vInitOffset.x;
		particles[i].y = r * st * sp + g_SceneMesh[g_eMeshType].GetMeshBBoxCenter().y + g_vInitOffset.y;
		particles[i].z = r * ct + g_SceneMesh[g_eMeshType].GetMeshBBoxCenter().z + g_vInitOffset.z;
		particles[i].w = 0;
	}

    V_RETURN( CreateStructuredBuffer< D3DXVECTOR4 >( pd3dDevice, g_iNumParticles, &g_pParticles, &g_pParticlesSRV, &g_pParticlesUAV, particles ) );
    DXUT_SetDebugName( g_pParticles, "Particles" );
    DXUT_SetDebugName( g_pParticlesSRV, "Particles SRV" );
    DXUT_SetDebugName( g_pParticlesUAV, "Particles UAV" );

	V_RETURN( CreateStructuredBuffer< D3DXVECTOR4 >( pd3dDevice, g_iNumParticles, &g_pSortedParticles, &g_pSortedParticlesSRV, &g_pSortedParticlesUAV, particles ) );
    DXUT_SetDebugName( g_pSortedParticles, "Sorted" );
    DXUT_SetDebugName( g_pSortedParticlesSRV, "Sorted SRV" );
    DXUT_SetDebugName( g_pSortedParticlesUAV, "Sorted UAV" );
	
	delete particles;
	return hr;
}

//--------------------------------------------------------------------------------------
// EOF
//--------------------------------------------------------------------------------------
