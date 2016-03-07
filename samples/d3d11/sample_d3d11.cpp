// This code contains NVIDIA Confidential Information and is disclosed
// under the Mutual Non-Disclosure Agreement.
//
// Notice
// ALL NVIDIA DESIGN SPECIFICATIONS AND CODE ("MATERIALS") ARE PROVIDED "AS IS" NVIDIA MAKES
// NO REPRESENTATIONS, WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ANY IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
//
// NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. No third party distribution is allowed unless
// expressly authorized by NVIDIA.  Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright © 2014 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//

#include <cstdarg>
#include <cstdio>
#include <limits>
#include <algorithm>

#include <DirectXMath.h>

#include <DXUT/Core/DXUT.h>
#include <DXUT/Optional/DXUTcamera.h>
#include <DXUT/Optional/DXUTgui.h>
#include <DXUT/Optional/SDKmisc.h>

#include <GFSDK_FaceWorks.h>

#include "util.h"
#include "scenes.h"
#include "shader.h"


using namespace DirectX;
using namespace std;

// DXUT objects

CDXUTDialogResourceManager	g_DialogResourceManager;	// manager for shared resources of dialogs
CDXUTDialog					g_HUD;						// dialog for standard controls
CDXUTTextHelper *			g_pTxtHelper = nullptr;

bool g_ShowHelp = false;
bool g_ShowGUI = true;
bool g_ShowText = true;
bool g_bWireframe = false;
bool g_bShowPerf = true;
bool g_bCopyPerfToClipboard = false;
bool g_bTessellation = true;

// DXUT callbacks

HRESULT InitScene();
void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void * pUserContext);
void CALLBACK OnD3D11FrameRender(ID3D11Device * pd3dDevice, ID3D11DeviceContext * pd3dContext, double fTime, float fElapsedTime, void * pUserContext);

bool CALLBACK IsD3D11DeviceAcceptable(const CD3D11EnumAdapterInfo * AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo * DeviceInfo, DXGI_FORMAT BackBufferFormat, bool bWindowed, void * pUserContext);
bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings * pDeviceSettings, void * pUserContext);
HRESULT CALLBACK OnD3D11CreateDevice(ID3D11Device * pd3dDevice, const DXGI_SURFACE_DESC * pBackBufferSurfaceDesc, void * pUserContext);
HRESULT CALLBACK OnD3D11ResizedSwapChain(ID3D11Device * pd3dDevice, IDXGISwapChain * pSwapChain, const DXGI_SURFACE_DESC * pBackBufferSurfaceDesc, void * pUserContext);
void CALLBACK OnD3D11ReleasingSwapChain(void * pUserContext);
void CALLBACK OnD3D11DestroyDevice(void * pUserContext);

void InitUI();
void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl * pControl, void * pUserContext);
void UpdateSliders();
void RenderText();
LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool * pbNoFurtherProcessing, void * pUserContext);
void CALLBACK OnKeyboard(UINT nChar, bool bKeyDown, bool bAltDown, void * pUserContext);



int WINAPI wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPWSTR /*lpszCmdLine*/, int /*nCmdShow*/)
{
	// Enable run-time memory check for debug builds.

#if defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	// Set up DXUT callbacks

	DXUTSetCallbackFrameMove(&OnFrameMove);
	DXUTSetCallbackD3D11FrameRender(&OnD3D11FrameRender);

	DXUTSetCallbackD3D11DeviceAcceptable(&IsD3D11DeviceAcceptable);
	DXUTSetCallbackDeviceChanging(&ModifyDeviceSettings);
	DXUTSetCallbackD3D11DeviceCreated(&OnD3D11CreateDevice);
	DXUTSetCallbackD3D11SwapChainResized(&OnD3D11ResizedSwapChain);
	DXUTSetCallbackD3D11SwapChainReleasing(&OnD3D11ReleasingSwapChain);
	DXUTSetCallbackD3D11DeviceDestroyed(&OnD3D11DestroyDevice);

	DXUTSetCallbackMsgProc(&MsgProc);
	DXUTSetCallbackKeyboard(&OnKeyboard);

	InitUI();

	DXUTInit(
		true,	// Parse command line
		true);	// Show msgboxes on error
	DXUTSetCursorSettings(
		true,	// Show cursor when fullscreen
		true);	// Clip cursor when fullscreen
	DXUTCreateWindow(L"GeForce SDK FaceWorks D3D11 Test");
	DXUTCreateDevice(
		D3D_FEATURE_LEVEL_11_0,
		true,	// Windowed mode
		1920,
		1080);

	DXUTMainLoop();
	return DXUTGetExitCode();
}



// Rendering methods

enum RM
{
	RM_None,
	RM_SSS,
	RM_SSSAndDeep,
	RM_ViewCurvature,
	RM_ViewThickness,

	RM_Max
};

RM g_renderMethod = RM_SSSAndDeep;



// Viewable buffers

enum VIEWBUF
{
	VIEWBUF_None,
	VIEWBUF_ShadowMap,

	VIEWBUF_Max
};

VIEWBUF g_viewbuf = VIEWBUF_None;
static const XMFLOAT4 s_rectViewBuffer(10, 50, 512, 512);



// Scene resources

const float					g_FOV = 0.5f;		// vertical fov, radians

CSceneDigitalIra			g_sceneDigitalIra;
CSceneTest					g_sceneTest;
CSceneHand					g_sceneHand;
CSceneDragon				g_sceneDragon;
CSceneLPSHead				g_sceneLPSHead;
CSceneManjaladon			g_sceneManjaladon;
CSceneWarriorHead			g_sceneWarriorHead;
CScene *					g_pSceneCur = nullptr;

CBackground					g_bkgndBlack;
CBackground					g_bkgndCharcoal;
CBackground					g_bkgndForest;
CBackground					g_bkgndNight;
CBackground					g_bkgndTunnel;
CBackground *				g_pBkgndCur = nullptr;

CMesh						g_meshFullscreen;

ID3D11ShaderResourceView *	g_pSrvCurvatureLUT = nullptr;
static const float			g_curvatureRadiusMinLUT = 0.1f;		// cm
static const float			g_curvatureRadiusMaxLUT = 10.0f;	// cm

ID3D11ShaderResourceView *	g_pSrvShadowLUT = nullptr;
static const float			g_shadowWidthMinLUT = 0.8f;		// cm
static const float			g_shadowWidthMaxLUT = 10.0f;	// cm

ID3D11DepthStencilState *	g_pDssDepthTest = nullptr;
ID3D11DepthStencilState *	g_pDssNoDepthTest = nullptr;
ID3D11DepthStencilState *	g_pDssNoDepthWrite = nullptr;
ID3D11RasterizerState *		g_pRsSolid = nullptr;
ID3D11RasterizerState *		g_pRsWireframe = nullptr;
ID3D11RasterizerState *		g_pRsSolidDoubleSided = nullptr;
ID3D11BlendState *			g_pBsAlphaBlend = nullptr;

CShadowMap					g_shadowmap;
CVarShadowMap				g_vsm;

ID3D11RenderTargetView *	g_pRtvNonSrgb = nullptr;

float						g_radYawDirectionalLight = 0.70f;
float						g_radPitchDirectionalLight = 0.40f;
XMVECTOR					g_vecDirectionalLight;
XMVECTOR					g_rgbDirectionalLight;
float						g_directionalLightBrightness = 1.0f;

float						g_vsmBlurRadius = 0.15f;		// One-sigma radius, in cm
float						g_vsmMinVariance = 1e-4f;
float						g_shadowSharpening = 10.0f;

float						g_sssBlurRadius = 0.27f;		// One-sigma radius of widest Gaussian, in cm

float						g_tessErrPx = 0.5f;				// Target tessellation error, in pixels

float						g_deepScatterIntensity = 0.5f;	// Multiplier on whole deep scattering result
float						g_deepScatterRadius = 0.6f;		// One-sigma radius of deep scatter Gaussian, in cm
float						g_deepScatterNormalOffset = -0.0007f;	// Normal offset for shadow lookup to calculate thickness
float						g_deepScatterShadowRadius = 1.1f;		// Poisson disk radius, in cm

float						g_debugSlider0 = 0.0f;
float						g_debugSlider1 = 0.0f;
float						g_debugSlider2 = 0.0f;
float						g_debugSlider3 = 0.0f;

// Copy a texture to a rectangle of a render target
// Rectangle is specified as a XMFLOAT4 of (left, top, width, height) in pixels
void CopyTexture(
	ID3D11DeviceContext * pCtx,
	ID3D11ShaderResourceView * pSrvSrc,
	ID3D11RenderTargetView * pRtvDest,
	ID3D11DepthStencilView * pDsvDest,
	const XMFLOAT4 & rectDest,
	const XMFLOAT4X4 * pMatTransformColor = nullptr);

// Macros for checking FaceWorks return codes
#if defined(_DEBUG)
#	define NV(x)			{ GFSDK_FaceWorks_Result res = (x); if (res != GFSDK_FaceWorks_OK) { DXUTTrace(__FILE__, __LINE__, E_FAIL, L#x, true); } }
#	define NV_RETURN(x)		{ GFSDK_FaceWorks_Result res = (x); if (res != GFSDK_FaceWorks_OK) { return DXUTTrace(__FILE__, __LINE__, E_FAIL, L#x, true); } }
#else
#	define NV(x)			x
#	define NV_RETURN(x)		{ GFSDK_FaceWorks_Result res = (x); if (res != GFSDK_FaceWorks_OK) { return E_FAIL; } }
#endif



HRESULT InitScene()
{
	HRESULT hr;
	wchar_t strPath[MAX_PATH];
	ID3D11Device * pDevice = DXUTGetD3D11Device();

	// Init FaceWorks
	NV_RETURN(GFSDK_FaceWorks_Init());

	V_RETURN(CreateFullscreenMesh(pDevice, &g_meshFullscreen));

	// Load scenes
	V_RETURN(g_sceneDigitalIra.Init());
	V_RETURN(g_sceneTest.Init());
	V_RETURN(g_sceneHand.Init());
	V_RETURN(g_sceneDragon.Init());
	V_RETURN(g_sceneLPSHead.Init());
	V_RETURN(g_sceneManjaladon.Init());
	V_RETURN(g_sceneWarriorHead.Init());
	g_pSceneCur = &g_sceneDigitalIra;

	// Load backgrounds
	V_RETURN(g_bkgndBlack.Init(L"HDREnvironments\\black_cube.dds", L"HDREnvironments\\black_cube.dds", L"HDREnvironments\\black_cube.dds"));
	V_RETURN(g_bkgndCharcoal.Init(L"HDREnvironments\\charcoal_cube.dds", L"HDREnvironments\\charcoal_cube.dds", L"HDREnvironments\\charcoal_cube.dds"));
	V_RETURN(g_bkgndForest.Init(L"HDREnvironments\\forest_env_cubemap.dds", L"HDREnvironments\\forest_diffuse_cubemap.dds", L"HDREnvironments\\forest_spec_cubemap.dds", 0.25f));
	V_RETURN(g_bkgndNight.Init(L"HDREnvironments\\night_env_cubemap.dds", L"HDREnvironments\\night_diffuse_cubemap.dds", L"HDREnvironments\\night_spec_cubemap.dds"));
	V_RETURN(g_bkgndTunnel.Init(L"HDREnvironments\\tunnel_env_cubemap.dds", L"HDREnvironments\\tunnel_diffuse_cubemap.dds", L"HDREnvironments\\tunnel_spec_cubemap.dds", 0.5f));
	g_pBkgndCur = &g_bkgndCharcoal;

	// Load textures

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"curvatureLUT.bmp"));
	V_RETURN(LoadTexture(strPath, pDevice, &g_pSrvCurvatureLUT, LT_Linear));

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"shadowLUT.bmp"));
	V_RETURN(LoadTexture(strPath, pDevice, &g_pSrvShadowLUT, LT_None));

	// Load shaders
	V_RETURN(g_shdmgr.Init(pDevice));

	// Create depth-stencil states

	D3D11_DEPTH_STENCIL_DESC dssDepthTestDesc = 
	{
		true,							// DepthEnable
		D3D11_DEPTH_WRITE_MASK_ALL,
		D3D11_COMPARISON_LESS_EQUAL,
	};
	V_RETURN(pDevice->CreateDepthStencilState(&dssDepthTestDesc, &g_pDssDepthTest));

	D3D11_DEPTH_STENCIL_DESC dssNoDepthTestDesc = 
	{
		false,							// DepthEnable
	};
	V_RETURN(pDevice->CreateDepthStencilState(&dssNoDepthTestDesc, &g_pDssNoDepthTest));

	D3D11_DEPTH_STENCIL_DESC dssNoDepthWriteDesc = 
	{
		true,							// DepthEnable
		D3D11_DEPTH_WRITE_MASK_ZERO,
		D3D11_COMPARISON_LESS_EQUAL,
	};
	V_RETURN(pDevice->CreateDepthStencilState(&dssNoDepthWriteDesc, &g_pDssNoDepthWrite));

	// Create rasterizer states

	D3D11_RASTERIZER_DESC rssSolidDesc =
	{
		D3D11_FILL_SOLID,
		D3D11_CULL_BACK,
		true,							// FrontCounterClockwise
		0, 0, 0,						// depth bias
		true,							// DepthClipEnable
		false,							// ScissorEnable
		true,							// MultisampleEnable
	};
	V_RETURN(pDevice->CreateRasterizerState(&rssSolidDesc, &g_pRsSolid));

	D3D11_RASTERIZER_DESC rssWireframeDesc =
	{
		D3D11_FILL_WIREFRAME,
		D3D11_CULL_BACK,
		true,							// FrontCounterClockwise
		0, 0, 0,						// depth bias
		true,							// DepthClipEnable
		false,							// ScissorEnable
		true,							// MultisampleEnable
	};
	V_RETURN(pDevice->CreateRasterizerState(&rssWireframeDesc, &g_pRsWireframe));

	D3D11_RASTERIZER_DESC rssSolidDoubleSidedDesc =
	{
		D3D11_FILL_SOLID,
		D3D11_CULL_NONE,
		true,							// FrontCounterClockwise
		0, 0, 0,						// depth bias
		true,							// DepthClipEnable
		false,							// ScissorEnable
		true,							// MultisampleEnable
	};
	V_RETURN(pDevice->CreateRasterizerState(&rssSolidDoubleSidedDesc, &g_pRsSolidDoubleSided));

	// Initialize the blend state for alpha-blending

	D3D11_BLEND_DESC bsDesc =
	{
		false, false,
		{
			true,
			D3D11_BLEND_SRC_ALPHA,
			D3D11_BLEND_INV_SRC_ALPHA,
			D3D11_BLEND_OP_ADD,
			D3D11_BLEND_SRC_ALPHA,
			D3D11_BLEND_INV_SRC_ALPHA,
			D3D11_BLEND_OP_ADD,
			D3D11_COLOR_WRITE_ENABLE_ALL,
		},
	};

	V_RETURN(pDevice->CreateBlendState(&bsDesc, &g_pBsAlphaBlend));

	// Create shadow map
	g_shadowmap.Init(1024, pDevice);
	g_vsm.Init(1024, pDevice);

	// Set up GPU profiler
	V_RETURN(g_gpuProfiler.Init(pDevice));

	// Set up directional light
	g_rgbDirectionalLight = XMVectorSet(0.984f, 1.0f, 0.912f, 0.0f);	// Note: linear RGB space

	return S_OK;
}

void CALLBACK OnD3D11DestroyDevice(void * /*pUserContext*/)
{
	g_DialogResourceManager.OnD3D11DestroyDevice();
	DXUTGetGlobalResourceCache().OnDestroyDevice();
	SAFE_DELETE(g_pTxtHelper);

	g_sceneDigitalIra.Release();
	g_sceneTest.Release();
	g_sceneHand.Release();
	g_sceneDragon.Release();
	g_sceneLPSHead.Release();
	g_sceneManjaladon.Release();
	g_sceneWarriorHead.Release();

	g_bkgndBlack.Release();
	g_bkgndCharcoal.Release();
	g_bkgndForest.Release();
	g_bkgndNight.Release();
	g_bkgndTunnel.Release();

	g_meshFullscreen.Release();

	SAFE_RELEASE(g_pSrvCurvatureLUT);
	SAFE_RELEASE(g_pSrvShadowLUT);

	g_shdmgr.Release();

	SAFE_RELEASE(g_pDssDepthTest);
	SAFE_RELEASE(g_pDssNoDepthTest);
	SAFE_RELEASE(g_pDssNoDepthWrite);
	SAFE_RELEASE(g_pRsSolid);
	SAFE_RELEASE(g_pRsWireframe);
	SAFE_RELEASE(g_pRsSolidDoubleSided);
	SAFE_RELEASE(g_pBsAlphaBlend);

	g_shadowmap.Release();
	g_vsm.Release();

	g_gpuProfiler.Release();

#if defined(_DEBUG)
	HRESULT hr;

	// Report any remaining live objects
	ID3D11Debug * pD3DDebug = nullptr;
	V(DXUTGetD3D11Device()->QueryInterface(IID_ID3D11Debug, reinterpret_cast<void **>(&pD3DDebug)));

	ID3D11InfoQueue * pInfoQueue = nullptr;
	V(DXUTGetD3D11Device()->QueryInterface(IID_ID3D11InfoQueue, reinterpret_cast<void **>(&pInfoQueue)));

	// Destroy all pending objects
	DXUTGetD3D11DeviceContext()->ClearState();
	DXUTGetD3D11DeviceContext()->Flush();

	// Turn off breaking on warnings (DXUT automatically checks live objects on exit)
	pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, false);

	SAFE_RELEASE(pInfoQueue);
	SAFE_RELEASE(pD3DDebug);
#endif
}

void CALLBACK OnFrameMove(double /*fTime*/, float fElapsedTime, void * /*pUserContext*/)
{
	g_pSceneCur->Camera()->FrameMove(fElapsedTime);
}

void CALLBACK OnD3D11FrameRender(
	ID3D11Device * /*pd3dDevice*/,
	ID3D11DeviceContext * pd3dContext,
	double /*fTime*/, float fElapsedTime,
	void * /*pUserContext*/)
{
	HRESULT hr;

	pd3dContext->ClearState();

	g_gpuProfiler.BeginFrame(pd3dContext);

	// Set up directional light and shadow map
	g_vecDirectionalLight = XMVectorSet(
		sinf(g_radYawDirectionalLight) * cosf(g_radPitchDirectionalLight),
		sinf(g_radPitchDirectionalLight),
		cosf(g_radYawDirectionalLight) * cosf(g_radPitchDirectionalLight),
		0.0f);


	XMStoreFloat3(&g_shadowmap.m_vecLight, g_vecDirectionalLight);

	g_pSceneCur->GetBounds(&g_shadowmap.m_posMinScene, &g_shadowmap.m_posMaxScene);
	g_shadowmap.UpdateMatrix();

	// Calculate UV space blur radius using shadow map's diameter, to attempt to maintain a
	// constant blur radius in world space.
	g_vsm.m_blurRadius = g_vsmBlurRadius / min(g_shadowmap.m_vecDiam.x, g_shadowmap.m_vecDiam.y);

	// Get the list of stuff to draw for the current scene
	std::vector<MeshToDraw> meshesToDraw;
	g_pSceneCur->GetMeshesToDraw(&meshesToDraw);
	int cMeshToDraw = int(meshesToDraw.size());

	// Set up whole-frame textures, constant buffers, etc.

	bool bDebug = (GetAsyncKeyState(' ') != 0);
	float debug = bDebug ? 1.0f : 0.0f;

	CbufDebug cbDebug =
	{
		debug,
		g_debugSlider0,
		g_debugSlider1,
		g_debugSlider2,
		g_debugSlider3,
	};

	// Get the view and proj matrices from the camera
	XMMATRIX matView = g_pSceneCur->Camera()->GetViewMatrix();
	XMMATRIX matProj = g_pSceneCur->Camera()->GetProjMatrix();
	XMFLOAT4X4 matWorldToClip; XMStoreFloat4x4(&matWorldToClip, matView * matProj);
	XMFLOAT4 posCamera; XMStoreFloat4(&posCamera, g_pSceneCur->Camera()->GetEyePt());

	XMMATRIX matViewAxes(
		matView.r[0],
		matView.r[1],
		matView.r[2],
		XMVectorSelect(matView.r[3], g_XMZero, g_XMSelect1110));
	XMMATRIX matViewAxesInverse = XMMatrixInverse(nullptr, matViewAxes);
	XMMATRIX matProjInverse = XMMatrixInverse(nullptr, matProj);
	XMFLOAT4X4 matClipToWorldAxes; XMStoreFloat4x4(&matClipToWorldAxes, matProjInverse * matViewAxesInverse);

	// Calculate tessellation scale using screen-space error.  The 0.41 is a curve fitting constant.
	// Error goes inversely as the square of the tess factor (for edges that are small wrt curvature),
	// so tess factor goes inversely as the square root of the target error.
	float tessScale = 0.41f / sqrtf(g_tessErrPx * 2.0f * tanf(0.5f * g_FOV) / float(DXUTGetWindowHeight()));

	XMFLOAT4 vecDirectionalLight; XMStoreFloat4(&vecDirectionalLight, g_vecDirectionalLight);
	XMFLOAT4 rgbDirectionalLight; XMStoreFloat4(&rgbDirectionalLight, g_directionalLightBrightness * g_rgbDirectionalLight);

	CbufFrame cbFrame =
	{
		matWorldToClip,
		posCamera,
		vecDirectionalLight,
		rgbDirectionalLight,
		g_shadowmap.m_matWorldToUvzw,
		{
			XMFLOAT4(g_shadowmap.m_matWorldToUvzNormal.m[0]),
			XMFLOAT4(g_shadowmap.m_matWorldToUvzNormal.m[1]),
			XMFLOAT4(g_shadowmap.m_matWorldToUvzNormal.m[2]),
		},
		g_vsmMinVariance,
		g_shadowSharpening,
		tessScale,
		g_deepScatterIntensity,
		g_deepScatterNormalOffset,
		g_pBkgndCur->m_exposure,
	};

	g_shdmgr.InitFrame(
				pd3dContext,
				&cbDebug,
				&cbFrame,
				g_pBkgndCur->m_pSrvCubeDiff,
				g_pBkgndCur->m_pSrvCubeSpec,
				g_pSrvCurvatureLUT,
				g_pSrvShadowLUT);

	// Clear the shadow map
	pd3dContext->ClearDepthStencilView(g_shadowmap.m_pDsv, D3D11_CLEAR_DEPTH, 1.0f, 0);

	g_shadowmap.BindRenderTarget(pd3dContext);
	pd3dContext->OMSetDepthStencilState(g_pDssDepthTest, 0);
	pd3dContext->RSSetState(g_pRsSolid);

	// Draw shadow map

	g_shdmgr.BindShadow(pd3dContext, g_shadowmap.m_matWorldToClip);
	for (int i = 0; i < cMeshToDraw; ++i)
	{
		meshesToDraw[i].m_pMesh->Draw(pd3dContext);
	}

	g_vsm.UpdateFromShadowMap(g_shadowmap, pd3dContext);
	g_vsm.GaussianBlur(pd3dContext);

	g_gpuProfiler.Timestamp(pd3dContext, GTS_ShadowMap);

	// Bind the non-SRGB render target view, for rendering with tone mapping
	// (which outputs in SRGB gamma space natively)
	V(DXUTSetupD3D11Views(pd3dContext));
	pd3dContext->OMSetRenderTargets(1, &g_pRtvNonSrgb, DXUTGetD3D11DepthStencilView());
	pd3dContext->RSSetState(g_bWireframe ? g_pRsWireframe : g_pRsSolid);

	// Clear the screen
	float rgbaZero[4] = {};
	pd3dContext->ClearRenderTargetView(DXUTGetD3D11RenderTargetView(), rgbaZero);
	pd3dContext->ClearDepthStencilView(DXUTGetD3D11DepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	g_shdmgr.BindShadowTextures(
				pd3dContext,
				g_shadowmap.m_pSrv,
				g_vsm.m_pSrv);

	SHDFEATURES features = 0;
	if (g_bTessellation)
		features |= SHDFEAT_Tessellation;

	switch (g_renderMethod)
	{
	case RM_None:
		{
			// Note: two loops for skin and eye materials so we can have GPU timestamps around
			// each shader individually. Gack.

			// Draw skin shaders
			for (int i = 0; i < cMeshToDraw; ++i)
			{
				if (meshesToDraw[i].m_pMtl->m_shader == SHADER_Skin)
				{
					g_shdmgr.BindMaterial(pd3dContext, features, meshesToDraw[i].m_pMtl);
					meshesToDraw[i].m_pMesh->Draw(pd3dContext);
				}
			}
			g_gpuProfiler.Timestamp(pd3dContext, GTS_Skin);

			// Draw eye shaders
			for (int i = 0; i < cMeshToDraw; ++i)
			{
				if (meshesToDraw[i].m_pMtl->m_shader == SHADER_Eye)
				{
					g_shdmgr.BindMaterial(pd3dContext, features, meshesToDraw[i].m_pMtl);
					meshesToDraw[i].m_pMesh->Draw(pd3dContext);
				}
			}
			g_gpuProfiler.Timestamp(pd3dContext, GTS_Eyes);
		}
		break;

	case RM_SSS:
	case RM_SSSAndDeep:
		{
			GFSDK_FaceWorks_SSSConfig sssConfigSkin = {}, sssConfigEye;
			sssConfigSkin.m_diffusionRadius = max(g_sssBlurRadius, 0.01f);
			sssConfigSkin.m_diffusionRadiusLUT = 0.27f;

			sssConfigSkin.m_curvatureRadiusMinLUT = g_curvatureRadiusMinLUT;
			sssConfigSkin.m_curvatureRadiusMaxLUT = g_curvatureRadiusMaxLUT;
			sssConfigSkin.m_shadowWidthMinLUT = g_shadowWidthMinLUT;
			sssConfigSkin.m_shadowWidthMaxLUT = g_shadowWidthMaxLUT;

			// Filter width is ~6 times the Gaussian sigma
			sssConfigSkin.m_shadowFilterWidth = max(6.0f * g_vsmBlurRadius, 0.01f);

			sssConfigEye = sssConfigSkin;

			GFSDK_FaceWorks_DeepScatterConfig deepScatterConfigSkin = {}, deepScatterConfigEye;
			deepScatterConfigSkin.m_radius = max(g_deepScatterRadius, 0.01f);
			deepScatterConfigSkin.m_shadowProjType = GFSDK_FaceWorks_ParallelProjection;
			memcpy(&deepScatterConfigSkin.m_shadowProjMatrix, &g_shadowmap.m_matProj, 16 * sizeof(float));
			deepScatterConfigSkin.m_shadowFilterRadius = g_deepScatterShadowRadius / min(g_shadowmap.m_vecDiam.x, g_shadowmap.m_vecDiam.y);

			deepScatterConfigEye = deepScatterConfigSkin;

			features |= SHDFEAT_SSS;
			if (g_renderMethod == RM_SSSAndDeep)
				features |= SHDFEAT_DeepScatter;

			GFSDK_FaceWorks_ErrorBlob faceworksErrors = {};

			// Note: two loops for skin and eye materials so we can have GPU timestamps around
			// each shader individually. Gack.

			// Draw skin shaders
			for (int i = 0; i < cMeshToDraw; ++i)
			{
				if (meshesToDraw[i].m_pMtl->m_shader == SHADER_Skin)
				{
					sssConfigSkin.m_normalMapSize = meshesToDraw[i].m_normalMapSize;
					sssConfigSkin.m_averageUVScale = meshesToDraw[i].m_averageUVScale;
					NV(GFSDK_FaceWorks_WriteCBDataForSSS(
						&sssConfigSkin, reinterpret_cast<GFSDK_FaceWorks_CBData *>(&meshesToDraw[i].m_pMtl->m_constants[4]), &faceworksErrors));
					NV(GFSDK_FaceWorks_WriteCBDataForDeepScatter(
						&deepScatterConfigSkin, reinterpret_cast<GFSDK_FaceWorks_CBData *>(&meshesToDraw[i].m_pMtl->m_constants[4]), &faceworksErrors));

					g_shdmgr.BindMaterial(pd3dContext, features, meshesToDraw[i].m_pMtl);
					meshesToDraw[i].m_pMesh->Draw(pd3dContext);
				}
			}
			g_gpuProfiler.Timestamp(pd3dContext, GTS_Skin);

			// Draw eye shaders
			for (int i = 0; i < cMeshToDraw; ++i)
			{
				if (meshesToDraw[i].m_pMtl->m_shader == SHADER_Eye)
				{
					sssConfigEye.m_normalMapSize = meshesToDraw[i].m_normalMapSize;
					sssConfigEye.m_averageUVScale = meshesToDraw[i].m_averageUVScale;
					NV(GFSDK_FaceWorks_WriteCBDataForSSS(
						&sssConfigEye, reinterpret_cast<GFSDK_FaceWorks_CBData *>(&meshesToDraw[i].m_pMtl->m_constants[12]), &faceworksErrors));
					NV(GFSDK_FaceWorks_WriteCBDataForDeepScatter(
						&deepScatterConfigEye, reinterpret_cast<GFSDK_FaceWorks_CBData *>(&meshesToDraw[i].m_pMtl->m_constants[12]), &faceworksErrors));

					g_shdmgr.BindMaterial(pd3dContext, features, meshesToDraw[i].m_pMtl);
					meshesToDraw[i].m_pMesh->Draw(pd3dContext);
				}
			}
			g_gpuProfiler.Timestamp(pd3dContext, GTS_Eyes);

			if (faceworksErrors.m_msg)
			{
#if defined(_DEBUG)
				wchar_t msg[512];
				_snwprintf_s(msg, dim(msg),
					L"FaceWorks rendering error:\n%hs", faceworksErrors.m_msg);
				DXUTTrace(__FILE__, __LINE__, E_FAIL, msg, true);
#endif
				GFSDK_FaceWorks_FreeErrorBlob(&faceworksErrors);
			}
		}
		break;

	case RM_ViewCurvature:
		{
			// Calculate scale-bias for mapping curvature to LUT coordinate,
			// given the min and max curvature encoded in the LUT.
			
			float curvatureScale = 1.0f / (1.0f / g_curvatureRadiusMinLUT - 1.0f / g_curvatureRadiusMaxLUT);
			float curvatureBias = 1.0f / (1.0f - g_curvatureRadiusMaxLUT / g_curvatureRadiusMinLUT);

			g_shdmgr.BindCurvature(pd3dContext, curvatureScale, curvatureBias);

			for (int i = 0; i < cMeshToDraw; ++i)
			{
				meshesToDraw[i].m_pMesh->Draw(pd3dContext);
			}
		}
		break;

	case RM_ViewThickness:
		{
			GFSDK_FaceWorks_DeepScatterConfig deepScatterConfigSkin = {};
			deepScatterConfigSkin.m_radius = max(g_deepScatterRadius, 0.01f);
			deepScatterConfigSkin.m_shadowProjType = GFSDK_FaceWorks_ParallelProjection;
			memcpy(&deepScatterConfigSkin.m_shadowProjMatrix, &g_shadowmap.m_matProj, 16 * sizeof(float));
			deepScatterConfigSkin.m_shadowFilterRadius = g_deepScatterShadowRadius / min(g_shadowmap.m_vecDiam.x, g_shadowmap.m_vecDiam.y);

			GFSDK_FaceWorks_CBData faceworksCBData = {};
			GFSDK_FaceWorks_ErrorBlob faceworksErrors = {};

			NV(GFSDK_FaceWorks_WriteCBDataForDeepScatter(
				&deepScatterConfigSkin, &faceworksCBData, &faceworksErrors));

			if (faceworksErrors.m_msg)
			{
#if defined(_DEBUG)
				wchar_t msg[512];
				_snwprintf_s(msg, dim(msg),
					L"FaceWorks rendering error:\n%hs", faceworksErrors.m_msg);
				DXUTTrace(__FILE__, __LINE__, E_FAIL, msg, true);
#endif
				GFSDK_FaceWorks_FreeErrorBlob(&faceworksErrors);
			}

			g_shdmgr.BindThickness(pd3dContext, &faceworksCBData);

			for (int i = 0; i < cMeshToDraw; ++i)
			{
				meshesToDraw[i].m_pMesh->Draw(pd3dContext);
			}
		}
		break;

	default:
		assert(false);
		break;
	}

	g_shdmgr.UnbindTess(pd3dContext);

	// Draw the skybox
	pd3dContext->RSSetState(g_pRsSolid);
	g_shdmgr.BindSkybox(pd3dContext, g_pBkgndCur->m_pSrvCubeEnv, matClipToWorldAxes);
	g_meshFullscreen.Draw(pd3dContext);

	// Draw hair shaders, with alpha blending
	pd3dContext->RSSetState(g_pRsSolidDoubleSided);
	pd3dContext->OMSetDepthStencilState(g_pDssNoDepthWrite, 0);
	pd3dContext->OMSetBlendState(g_pBsAlphaBlend, nullptr, ~0UL);
	for (int i = 0; i < cMeshToDraw; ++i)
	{
		if (meshesToDraw[i].m_pMtl->m_shader == SHADER_Hair)
		{
			g_shdmgr.BindMaterial(pd3dContext, 0, meshesToDraw[i].m_pMtl);
			meshesToDraw[i].m_pMesh->Draw(pd3dContext);
		}
	}
	pd3dContext->RSSetState(g_pRsSolid);
	pd3dContext->OMSetDepthStencilState(g_pDssDepthTest, 0);
	pd3dContext->OMSetBlendState(nullptr, nullptr, ~0UL);

	// Now switch to the SRGB back buffer view, for compositing UI
	V(DXUTSetupD3D11Views(pd3dContext));

	// Show the shadow map if desired

	if (g_viewbuf == VIEWBUF_ShadowMap)
	{
		// Copy red channel to RGB channels
		XMFLOAT4X4 matTransformColor(
			1, 1, 1, 0,
			0, 0, 0, 0,
			0, 0, 0, 0,
			0, 0, 0, 1);

		CopyTexture(
			pd3dContext, g_shadowmap.m_pSrv,
			DXUTGetD3D11RenderTargetView(), nullptr,
			s_rectViewBuffer,
			&matTransformColor);
		V(DXUTSetupD3D11Views(pd3dContext));
	}

	// Render GUI and text

	if (g_ShowGUI)
	{
		UpdateSliders();
		g_HUD.OnRender(fElapsedTime);
	}

	g_gpuProfiler.WaitForDataAndUpdate(pd3dContext);

	if (g_ShowText)
	{
		RenderText();
	}

	g_gpuProfiler.EndFrame(pd3dContext);
}



bool CALLBACK IsD3D11DeviceAcceptable(
	const CD3D11EnumAdapterInfo * /*AdapterInfo*/,
	UINT /*Output*/,
	const CD3D11EnumDeviceInfo * /*DeviceInfo*/,
	DXGI_FORMAT /*BackBufferFormat*/,
	bool /*bWindowed*/,
	void * /*pUserContext*/)
{
	// Any D3D11 device is ok
	return true;
}

bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings * pDeviceSettings, void * /*pUserContext*/)
{
	// Disable vsync
	pDeviceSettings->d3d11.SyncInterval = 0;

	// For the first device created if it is a REF device, display a warning dialog box
	static bool s_bFirstTime = true;
	if (s_bFirstTime)
	{
		s_bFirstTime = false;

		if (pDeviceSettings->d3d11.DriverType == D3D_DRIVER_TYPE_REFERENCE)
		{
			DXUTDisplaySwitchingToREFWarning();
		}

		// Request 4x MSAA
		pDeviceSettings->d3d11.sd.SampleDesc.Count = 4;
	}

	return true;
}

HRESULT CALLBACK OnD3D11CreateDevice(
	ID3D11Device * pd3dDevice,
	const DXGI_SURFACE_DESC * /*pBackBufferSurfaceDesc*/,
	void * /*pUserContext*/)
{
	HRESULT hr;

#if defined(_DEBUG)
	// Set up D3D11 debug layer settings
	ID3D11InfoQueue * pInfoQueue = nullptr;
	V_RETURN(pd3dDevice->QueryInterface(IID_ID3D11InfoQueue, reinterpret_cast<void **>(&pInfoQueue)));

	// Break in the debugger when an error or warning is issued
	pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
	pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
	pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, true);

	// Disable warning about setting private data (i.e. debug names of resources)
	D3D11_MESSAGE_ID aMsgToFilter[] =
	{
		D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
	};
	D3D11_INFO_QUEUE_FILTER filter = {};
	filter.DenyList.NumIDs = dim(aMsgToFilter);
	filter.DenyList.pIDList = aMsgToFilter;
	pInfoQueue->AddStorageFilterEntries(&filter);

	SAFE_RELEASE(pInfoQueue);
#endif

	ID3D11DeviceContext * pd3dContext = DXUTGetD3D11DeviceContext();
	V_RETURN(g_DialogResourceManager.OnD3D11CreateDevice(pd3dDevice, pd3dContext));
	g_pTxtHelper = new CDXUTTextHelper(pd3dDevice, pd3dContext, &g_DialogResourceManager, 15);

	V_RETURN(InitScene());

	return S_OK;
}

HRESULT CALLBACK OnD3D11ResizedSwapChain(
	ID3D11Device * pd3dDevice,
	IDXGISwapChain * pSwapChain,
	const DXGI_SURFACE_DESC * pBackBufferSurfaceDesc,
	void * /*pUserContext*/)
{
	HRESULT hr;

	// Inform UI elements of the new size

	V_RETURN(g_DialogResourceManager.OnD3D11ResizedSwapChain(pd3dDevice, pBackBufferSurfaceDesc));

	UINT MainHudWidth = 200;
	g_HUD.SetLocation(pBackBufferSurfaceDesc->Width - MainHudWidth, 0);
	g_HUD.SetSize(MainHudWidth, pBackBufferSurfaceDesc->Height);

	// Regenerate camera projection matrices with the new aspect ratio

	static const float NEAR_PLANE = 0.1f;
	static const float FAR_PLANE = 1e5f;

	g_sceneDigitalIra.Camera()->SetProjParams(
		g_FOV,
		float(pBackBufferSurfaceDesc->Width) / float(pBackBufferSurfaceDesc->Height),
		NEAR_PLANE,
		FAR_PLANE);

	g_sceneTest.Camera()->SetProjParams(
		g_FOV,
		float(pBackBufferSurfaceDesc->Width) / float(pBackBufferSurfaceDesc->Height),
		NEAR_PLANE,
		FAR_PLANE);

	g_sceneHand.Camera()->SetProjParams(
		g_FOV,
		float(pBackBufferSurfaceDesc->Width) / float(pBackBufferSurfaceDesc->Height),
		NEAR_PLANE,
		FAR_PLANE);

	g_sceneDragon.Camera()->SetProjParams(
		g_FOV,
		float(pBackBufferSurfaceDesc->Width) / float(pBackBufferSurfaceDesc->Height),
		NEAR_PLANE,
		FAR_PLANE);

	g_sceneLPSHead.Camera()->SetProjParams(
		g_FOV,
		float(pBackBufferSurfaceDesc->Width) / float(pBackBufferSurfaceDesc->Height),
		NEAR_PLANE,
		FAR_PLANE);

	g_sceneManjaladon.Camera()->SetProjParams(
		g_FOV,
		float(pBackBufferSurfaceDesc->Width) / float(pBackBufferSurfaceDesc->Height),
		NEAR_PLANE,
		FAR_PLANE);

	g_sceneWarriorHead.Camera()->SetProjParams(
		g_FOV,
		float(pBackBufferSurfaceDesc->Width) / float(pBackBufferSurfaceDesc->Height),
		NEAR_PLANE,
		FAR_PLANE);

	// Regenerate the non-SRGB render target view

	if (g_pRtvNonSrgb)
	{
		g_pRtvNonSrgb->Release();
		g_pRtvNonSrgb = nullptr;
	}

	ID3D11Texture2D * pBackBuffer;
	V_RETURN(pSwapChain->GetBuffer(0, IID_ID3D11Texture2D, reinterpret_cast<void **>(&pBackBuffer)));

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc =
	{
		DXGI_FORMAT_R8G8B8A8_UNORM,
		D3D11_RTV_DIMENSION_TEXTURE2DMS,
	};
	V_RETURN(pd3dDevice->CreateRenderTargetView(pBackBuffer, &rtvDesc, &g_pRtvNonSrgb));

	pBackBuffer->Release();

	return S_OK;
}

void CALLBACK OnD3D11ReleasingSwapChain(void * /*pUserContext*/)
{
	g_DialogResourceManager.OnD3D11ReleasingSwapChain();

	if (g_pRtvNonSrgb)
	{
		g_pRtvNonSrgb->Release();
		g_pRtvNonSrgb = nullptr;
	}
}



// UI control IDs
enum
{
	IDC_ABOUT = 1,

	IDC_SCENE,
	IDC_BKGND,
	IDC_SSS_METHOD,

	IDC_CAMERA_WIDE,
	IDC_CAMERA_CLOSE,
	IDC_CAMERA_EYE,
	IDC_VIEW_BUFFER,

	IDC_SLIDER_BASE,
};

enum
{
	GROUP_LIGHT,
	GROUP_SUBSURFACE,
	GROUP_SHADOW,
	GROUP_MTL,
	GROUP_EYE,
	GROUP_DEEPSCATTER,
	GROUP_DEBUG,
};

struct Slider
{
	int					m_group;
	const wchar_t *		m_strCaption;
	float				m_min, m_max;
	float *				m_pValue;
	int					m_steps;
};

extern float g_normalStrength;
extern float g_glossSkin;
extern float g_glossEye;
extern float g_irisRadiusSource;
extern float g_irisRadiusDest;
extern float g_irisEdgeHardness;
extern float g_irisDilation;
extern float g_specReflectanceHair;
extern float g_glossHair;

static const Slider g_aSlider[] =
{
	{ GROUP_LIGHT, L"Light yaw: %0.2f", -XM_PI, XM_PI, &g_radYawDirectionalLight },
	{ GROUP_LIGHT, L"Light pitch: %0.2f", -XM_PIDIV2, XM_PIDIV2, &g_radPitchDirectionalLight },
	{ GROUP_LIGHT, L"Light brightness: %0.1f", 0.0f, 5.0f, &g_directionalLightBrightness },

	{ GROUP_SUBSURFACE, L"SSS radius (cm): %0.2f", 0.0f, 1.0f, &g_sssBlurRadius },

	{ GROUP_MTL, L"Normal strength: %0.2f", 0.0f, 2.0f, &g_normalStrength },
	{ GROUP_MTL, L"Skin gloss: %0.2f", 0.0f, 1.0f, &g_glossSkin },
	{ GROUP_MTL, L"Eye gloss: %0.2f", 0.0f, 1.0f, &g_glossEye },

#if 0
	{ GROUP_EYE, L"Iris radius src: %0.3f", 0.0f, 0.5f, &g_irisRadiusSource },
	{ GROUP_EYE, L"Iris radius dst: %0.3f", 0.0f, 0.5f, &g_irisRadiusDest },
	{ GROUP_EYE, L"Iris hardness: %0.0f", 0.0f, 100.0f, &g_irisEdgeHardness },
	{ GROUP_EYE, L"Iris dilation: %0.2f", 0.0f, 0.75f, &g_irisDilation },
	{ GROUP_MTL, L"Hair spec: %0.2f", 0.0f, 1.0f, &g_specReflectanceHair },
	{ GROUP_MTL, L"Hair gloss: %0.2f", 0.0f, 1.0f, &g_glossHair },
#endif

	{ GROUP_DEEPSCATTER, L"DpSc intensity: %0.2f", 0.0f, 1.0f, &g_deepScatterIntensity },
	{ GROUP_DEEPSCATTER, L"DpSc radius (cm): %0.2f", 0.0f, 2.0f, &g_deepScatterRadius },
	{ GROUP_DEEPSCATTER, L"DpSc nrml ofs: %0.5f", -0.002f, 0.0f, &g_deepScatterNormalOffset },
	{ GROUP_DEEPSCATTER, L"DpSc shdw R (cm): %0.2f", 0.0f, 2.0f, &g_deepScatterShadowRadius },

	{ GROUP_SHADOW, L"VSM radius (cm): %0.2f", 0.0f, 1.0f, &g_vsmBlurRadius },
#if 0
	{ GROUP_SHADOW, L"Shadow sharpen: %0.1f", 1.0f, 10.0f, &g_shadowSharpening },
	{ GROUP_SHADOW, L"VSM min var: %0.6f", 0.0f, 5e-4f, &g_vsmMinVariance },
#endif

#if 0
	{ GROUP_DEBUG, L"Debug0: %0.2f", 0.0f, 1.0f, &g_debugSlider0 },
	{ GROUP_DEBUG, L"Debug1: %0.2f", 0.0f, 1.0f, &g_debugSlider1 },
	{ GROUP_DEBUG, L"Debug2: %0.2f", 0.0f, 1.0f, &g_debugSlider2 },
	{ GROUP_DEBUG, L"Debug3: %0.2f", 0.0f, 1.0f, &g_debugSlider3 },
#endif
};

static const int g_sliderStepsDefault = 100;

void InitUI()
{
	g_HUD.Init(&g_DialogResourceManager);
	g_HUD.SetBackgroundColors(D3DCOLOR_ARGB(128, 0, 0, 0));
	g_HUD.SetCallback(&OnGUIEvent);

	// Hard-coded HUD layout

	static const int ITEM_HEIGHT = 20;
	static const int ITEM_DELTA	= ITEM_HEIGHT + 2;
	static const int GROUP_DELTA = ITEM_DELTA;
	static const int ITEM_WIDTH = 170;
	static const int ITEM_LEFT = 15;

	int iY = 0;

	// About button
	g_HUD.AddButton(IDC_ABOUT, L"About FaceWorks", ITEM_LEFT, iY += ITEM_DELTA, ITEM_WIDTH, ITEM_HEIGHT);

	// Add combo box for scene selection

	iY += GROUP_DELTA;

	g_HUD.AddStatic(IDC_SCENE, L"Model", ITEM_LEFT, iY += ITEM_DELTA, ITEM_WIDTH, ITEM_HEIGHT);

	CDXUTComboBox * pComboBoxScene = nullptr;
	g_HUD.AddComboBox(IDC_SCENE, ITEM_LEFT, iY += ITEM_DELTA, ITEM_WIDTH, ITEM_HEIGHT, 0, false, &pComboBoxScene);
	pComboBoxScene->AddItem(L"Digital Ira", &g_sceneDigitalIra);
	pComboBoxScene->AddItem(L"LPS Head", &g_sceneLPSHead);
	pComboBoxScene->AddItem(L"Warrior Head", &g_sceneWarriorHead);
	pComboBoxScene->AddItem(L"Hand", &g_sceneHand);
	pComboBoxScene->AddItem(L"Dragon", &g_sceneDragon);
	pComboBoxScene->AddItem(L"Manjaladon", &g_sceneManjaladon);
	pComboBoxScene->AddItem(L"Test", &g_sceneTest);
	pComboBoxScene->SetSelectedByIndex(0);

	// Add combo box for background selection

	g_HUD.AddStatic(IDC_SCENE, L"Background", ITEM_LEFT, iY += ITEM_DELTA, ITEM_WIDTH, ITEM_HEIGHT);

	CDXUTComboBox * pComboBoxBkgnd = nullptr;
	g_HUD.AddComboBox(IDC_BKGND, ITEM_LEFT, iY += ITEM_DELTA, ITEM_WIDTH, ITEM_HEIGHT, 0, false, &pComboBoxBkgnd);
	pComboBoxBkgnd->AddItem(L"Black", &g_bkgndBlack);
	pComboBoxBkgnd->AddItem(L"Charcoal", &g_bkgndCharcoal);
	pComboBoxBkgnd->AddItem(L"Forest", &g_bkgndForest);
	pComboBoxBkgnd->AddItem(L"Night", &g_bkgndNight);
	pComboBoxBkgnd->AddItem(L"Tunnel", &g_bkgndTunnel);
	pComboBoxBkgnd->SetSelectedByIndex(1);

	// Add combo box for SSS method

	g_HUD.AddStatic(IDC_SSS_METHOD, L"SSS Method", ITEM_LEFT, iY += ITEM_DELTA, ITEM_WIDTH, ITEM_HEIGHT);

	CDXUTComboBox * pComboBoxRMethod = nullptr;
	g_HUD.AddComboBox(IDC_SSS_METHOD, ITEM_LEFT, iY += ITEM_DELTA, ITEM_WIDTH, ITEM_HEIGHT, 0, false, &pComboBoxRMethod);

	static const wchar_t * s_aStrRMethod[] =
	{
		L"None",			// RM_None
		L"SSS",				// RM_SSS
		L"SSS+Deep",		// RM_SSSAndDeep
		L"View Curvature",	// RM_ViewCurvature
		L"View Thickness",	// RM_ViewThickness
	};
	static_assert(dim(s_aStrRMethod) == RM_Max, "Size of s_aStrRMethod must match the number of entries in enum RM");

	for (int i = 0; i < RM_Max; ++i)
	{
		pComboBoxRMethod->AddItem(s_aStrRMethod[i], nullptr);
	}

	pComboBoxRMethod->SetSelectedByIndex(g_renderMethod);

	// Add sliders

	std::wstring str;
	int groupCur = -1;

	for (int i = 0; i < dim(g_aSlider); ++i)
	{
		const Slider & slider = g_aSlider[i];

		if (slider.m_group != groupCur)
		{
			iY += GROUP_DELTA;
			groupCur = slider.m_group;
		}

		str = StrPrintf(slider.m_strCaption, *slider.m_pValue);
		int steps = slider.m_steps ? slider.m_steps : g_sliderStepsDefault;
		int sliderValue = int((*slider.m_pValue - slider.m_min) / (slider.m_max - slider.m_min) * steps);

		g_HUD.AddStatic(IDC_SLIDER_BASE + i, str.c_str(), ITEM_LEFT, iY += ITEM_DELTA, ITEM_WIDTH, ITEM_HEIGHT);
		g_HUD.AddSlider(IDC_SLIDER_BASE + i, ITEM_LEFT, iY += ITEM_DELTA, ITEM_WIDTH, ITEM_HEIGHT, 0, steps, sliderValue);
	}

	iY += GROUP_DELTA;

	g_HUD.AddButton(IDC_CAMERA_WIDE, L"Wide shot", ITEM_LEFT, iY += ITEM_DELTA, ITEM_WIDTH, ITEM_HEIGHT);
	g_HUD.AddButton(IDC_CAMERA_CLOSE, L"Close shot", ITEM_LEFT, iY += ITEM_DELTA, ITEM_WIDTH, ITEM_HEIGHT);
	g_HUD.AddButton(IDC_CAMERA_EYE, L"Eyeball shot", ITEM_LEFT, iY += ITEM_DELTA, ITEM_WIDTH, ITEM_HEIGHT);

	iY += GROUP_DELTA;

	g_HUD.AddStatic(IDC_VIEW_BUFFER, L"View buffer", ITEM_LEFT, iY += ITEM_DELTA, ITEM_WIDTH, ITEM_HEIGHT);

	CDXUTComboBox * pComboBoxViewBuffer = nullptr;
	g_HUD.AddComboBox(IDC_VIEW_BUFFER, ITEM_LEFT, iY += ITEM_DELTA, ITEM_WIDTH, ITEM_HEIGHT, 0, false, &pComboBoxViewBuffer);

	static const wchar_t * s_aStrViewBuffer[] =
	{
		L"None",			// VIEWBUF_None
		L"Shadow Map",		// VIEWBUF_ShadowMap
	};
	static_assert(dim(s_aStrViewBuffer) == VIEWBUF_Max, "Size of s_aStrViewBuffer must match the number of entries in enum VIEWBUF");

	for (int i = 0; i < VIEWBUF_Max; ++i)
	{
		pComboBoxViewBuffer->AddItem(s_aStrViewBuffer[i], nullptr);
	}

	pComboBoxViewBuffer->SetSelectedByIndex(g_viewbuf);
}

void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl * /*pControl*/, void * /*pUserContext*/)
{
	switch (nControlID)
	{
	case IDC_ABOUT:
		MessageBoxA(DXUTGetHWND(), GFSDK_FaceWorks_GetBuildInfo(), "NVIDIA FaceWorks", MB_ICONINFORMATION);
		break;

	case IDC_SCENE:
		g_pSceneCur = static_cast<CScene *>(g_HUD.GetComboBox(nControlID)->GetSelectedData());
		break;

	case IDC_BKGND:
		g_pBkgndCur = static_cast<CBackground *>(g_HUD.GetComboBox(nControlID)->GetSelectedData());
		break;

	case IDC_SSS_METHOD:
		{
			int i = g_HUD.GetComboBox(nControlID)->GetSelectedIndex();
			assert(i >= 0 && i < RM_Max);
			g_renderMethod = RM(i);
		}
		break;

	case IDC_CAMERA_WIDE:
		{
			XMVECTOR posLookAt = XMLoadFloat3(&g_sceneDigitalIra.m_meshHead.m_posCenter);
			XMVECTOR posCamera = posLookAt + XMVectorSet(0.0f, 0.0f, 150.0f, 0.0f);
			g_sceneDigitalIra.Camera()->SetViewParams(posCamera, posLookAt);
		}
		break;

	case IDC_CAMERA_CLOSE:
		{
			XMVECTOR posLookAt = XMLoadFloat3(&g_sceneDigitalIra.m_meshHead.m_posCenter);
			XMVECTOR posCamera = posLookAt + XMVectorSet(0.0f, 0.0f, 60.0f, 0.0f);
			g_sceneDigitalIra.Camera()->SetViewParams(posCamera, posLookAt);
		}
		break;

	case IDC_CAMERA_EYE:
		{
			XMVECTOR posLookAt = XMLoadFloat3(&g_sceneDigitalIra.m_meshEyeL.m_posCenter);
			XMVECTOR posCamera = posLookAt + XMVectorSet(0.0f, 0.0f, 10.0f, 0.0f);
			g_sceneDigitalIra.Camera()->SetViewParams(posCamera, posLookAt);
		}
		break;

	case IDC_VIEW_BUFFER:
		{
			int i = g_HUD.GetComboBox(nControlID)->GetSelectedIndex();
			assert(i >= 0 && i < VIEWBUF_Max);
			g_viewbuf = VIEWBUF(i);
		}
		break;

	default:
		if (nControlID >= IDC_SLIDER_BASE &&
			nEvent == EVENT_SLIDER_VALUE_CHANGED)
		{
			const Slider & slider = g_aSlider[nControlID - IDC_SLIDER_BASE];

			int sliderValue = g_HUD.GetSlider(nControlID)->GetValue();
			int steps = slider.m_steps ? slider.m_steps : g_sliderStepsDefault;
			*slider.m_pValue = slider.m_min + float(sliderValue) / float(steps) * (slider.m_max - slider.m_min);
			g_HUD.GetStatic(nControlID)->SetText(StrPrintf(slider.m_strCaption, *slider.m_pValue).c_str());
		}
		break;
	}
}

void UpdateSliders()
{
	for (int i = 0; i < dim(g_aSlider); ++i)
	{
		const Slider & slider = g_aSlider[i];

		int steps = slider.m_steps ? slider.m_steps : g_sliderStepsDefault;
		int sliderValue = int((*slider.m_pValue - slider.m_min) / (slider.m_max - slider.m_min) * steps);
		g_HUD.GetSlider(IDC_SLIDER_BASE + i)->SetValue(sliderValue);
		g_HUD.GetStatic(IDC_SLIDER_BASE + i)->SetText(StrPrintf(slider.m_strCaption, *slider.m_pValue).c_str());
	}
}

LRESULT CALLBACK MsgProc(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam, LPARAM lParam,
	bool * pbNoFurtherProcessing,
	void * /*pUserContext*/)
{
	// Pass messages to dialog resource manager so GUI state is updated correctly
	*pbNoFurtherProcessing = g_DialogResourceManager.MsgProc(hWnd, uMsg, wParam, lParam);
	if (*pbNoFurtherProcessing)
		return 0;

	// Give the dialogs a chance to handle the message first
	*pbNoFurtherProcessing = g_HUD.MsgProc(hWnd, uMsg, wParam, lParam);
	if (*pbNoFurtherProcessing)
		return 0;

	if (g_pSceneCur)
		g_pSceneCur->Camera()->HandleMessages(hWnd, uMsg, wParam, lParam);

	return 0;
}

void CALLBACK OnKeyboard(UINT nChar, bool bKeyDown, bool /*bAltDown*/, void * /*pUserContext*/)
{
	if (!bKeyDown)
		return;

	switch (nChar)
	{
	case VK_F1:
		g_ShowHelp = !g_ShowHelp;
		break;

	case VK_F2:
		g_pSceneCur = &g_sceneDigitalIra;
		g_HUD.GetComboBox(IDC_SCENE)->SetSelectedByData(&g_sceneDigitalIra);
		break;

	case VK_F3:
		g_pSceneCur = &g_sceneLPSHead;
		g_HUD.GetComboBox(IDC_SCENE)->SetSelectedByData(&g_sceneLPSHead);
		break;

	case VK_F4:
		g_pSceneCur = &g_sceneWarriorHead;
		g_HUD.GetComboBox(IDC_SCENE)->SetSelectedByData(&g_sceneWarriorHead);
		break;

	case VK_F5:
		g_pSceneCur = &g_sceneHand;
		g_HUD.GetComboBox(IDC_SCENE)->SetSelectedByData(&g_sceneHand);
		break;

	case VK_F6:
		g_pSceneCur = &g_sceneDragon;
		g_HUD.GetComboBox(IDC_SCENE)->SetSelectedByData(&g_sceneDragon);
		break;

	case VK_F7:
		g_pSceneCur = &g_sceneManjaladon;
		g_HUD.GetComboBox(IDC_SCENE)->SetSelectedByData(&g_sceneManjaladon);
		break;

	case VK_F8:
		g_pSceneCur = &g_sceneTest;
		g_HUD.GetComboBox(IDC_SCENE)->SetSelectedByData(&g_sceneTest);
		break;

	case 'G':
		g_ShowGUI = !g_ShowGUI;
		break;

	case 'T':
		g_ShowText = !g_ShowText;
		break;

	case 'F':
		g_bWireframe = !g_bWireframe;
		break;

	case 'P':
		g_bShowPerf = !g_bShowPerf;
		break;

	case 'C':
		g_bCopyPerfToClipboard = true;
		break;

	case 'Y':
		g_bTessellation = !g_bTessellation;
		break;

	case 'R':
		g_shdmgr.DiscardRuntimeCompiledShaders();
		break;

	case VK_ESCAPE:
		PostQuitMessage(0);
		break;

	default:
		if (nChar >= '1' && nChar < '1' + RM_Max)
		{
			g_renderMethod = RM(nChar - '1');
			g_HUD.GetComboBox(IDC_SSS_METHOD)->SetSelectedByIndex(g_renderMethod);
		}
		break;
	}
}

void RenderText()
{
	g_pTxtHelper->Begin();
	g_pTxtHelper->SetInsertionPos(2, 0);
	g_pTxtHelper->SetForegroundColor(XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));

	const DXGI_SURFACE_DESC * pBackBufferDesc = DXUTGetDXGIBackBufferSurfaceDesc();
	g_pTxtHelper->DrawTextLine(
		StrPrintf(
			L"D3D11, %0.2f ms, vsync %s, %dx%d, %s, MS%d Q%d",
			1000.0f / DXUTGetFPS(),
			DXUTIsVsyncEnabled() ? L"on" : L"off",
			pBackBufferDesc->Width,
			pBackBufferDesc->Height,
			DXUTDXGIFormatToString(pBackBufferDesc->Format, false),
			pBackBufferDesc->SampleDesc.Count,
			pBackBufferDesc->SampleDesc.Quality).c_str());

	g_pTxtHelper->DrawTextLine(DXUTGetDeviceStats());

	if (g_ShowHelp)
	{
		static const int HELP_HEIGHT = 300;
		UINT nBackBufferHeight = DXUTGetDXGIBackBufferSurfaceDesc()->Height;
		g_pTxtHelper->SetInsertionPos(2, nBackBufferHeight - HELP_HEIGHT);

		g_pTxtHelper->DrawTextLine(
			L"--Controls--\n"
			L"Rotate Camera: Left Mouse\n"
			L"Zoom Camera: Right Mouse or Mouse Wheel\n"
			L"Move Camera: Middle Mouse\n"
			L"Reset Camera: Home\n"
			L"Toggle Wireframe: F\n"
			L"Toggle GUI: G\n"
			L"Toggle Text: T\n"
			L"Toggle Perf Readouts: P\n"
			L"Copy Perf To Clipboard: C\n"
			L"Toggle Tessellation: Y\n"
			L"Reload Runtime-Compiled Shaders: R\n"
			L"Toggle Help: F1\n"
			L"Quit: Esc\n"
			);
	}
	else
	{
		g_pTxtHelper->DrawTextLine(L"Press F1 for help" );
	}

	if (g_bShowPerf)
	{
		static const wchar_t * s_aStrRMethod[] =
		{
			L"No SSS",					// RM_None
			L"SSS",						// RM_SSS
			L"SSS + Deep Scattering",	// RM_SSSAndDeep
			L"Curvature",				// RM_ViewCurvature
			L"View Thickness",			// RM_ViewThickness
		};
		static_assert(dim(s_aStrRMethod) == RM_Max, "Size of s_aStrRMethod must match the number of entries in enum RM");

		g_pTxtHelper->SetInsertionPos(2, 65);
		g_pTxtHelper->DrawTextLine(
			StrPrintf(
				L"---Perf - %s---\n"
				L"Shadow map: %0.2f ms\n"
				L"Skin: %0.2f ms\n"
				L"Eyes: %0.2f ms\n"
				L"Sky/UI: %0.2f ms\n"
				L"Total GPU frame time: %0.2f ms\n",
				s_aStrRMethod[g_renderMethod],
				g_gpuProfiler.DtAvg(GTS_ShadowMap) * 1000.0f,
				g_gpuProfiler.DtAvg(GTS_Skin) * 1000.0f,
				g_gpuProfiler.DtAvg(GTS_Eyes) * 1000.0f,
				g_gpuProfiler.DtAvg(GTS_EndFrame) * 1000.0f,
				g_gpuProfiler.GPUFrameTimeAvg() * 1000.0f
			).c_str());
	}

	if (g_bCopyPerfToClipboard)
	{
		std::wstring strToCopy;
		strToCopy = StrPrintf(
						L"%0.2f\t%0.2f\t%0.2f",
						g_gpuProfiler.DtAvg(GTS_ShadowMap) * 1000.0f,
						g_gpuProfiler.DtAvg(GTS_Skin) * 1000.0f,
						g_gpuProfiler.DtAvg(GTS_Eyes) * 1000.0f
					);

		if (OpenClipboard(DXUTGetHWND()))
		{
			EmptyClipboard();
			size_t cbString = sizeof(wchar_t) * (strToCopy.size() + 1);
			HGLOBAL clipbuffer = GlobalAlloc(GMEM_DDESHARE, cbString);
			if (clipbuffer) {
				wchar_t * buffer = static_cast<wchar_t *>(GlobalLock(clipbuffer));
				if (buffer) {
					memcpy(buffer, strToCopy.c_str(), cbString);
					GlobalUnlock(clipbuffer);
					SetClipboardData(CF_UNICODETEXT, clipbuffer);
				}
			}
			CloseClipboard();
		}

		g_bCopyPerfToClipboard = false;
	}

	g_pTxtHelper->End();
}

void CopyTexture(
	ID3D11DeviceContext * pCtx,
	ID3D11ShaderResourceView * pSrvSrc,
	ID3D11RenderTargetView * pRtvDest,
	ID3D11DepthStencilView * pDsvDest,
	const XMFLOAT4 & rectDest,
	const XMFLOAT4X4 * pMatTransformColor /* = nullptr */)
{
	D3D11_VIEWPORT viewport =
	{
		rectDest.x, rectDest.y,		// left, top
		rectDest.z, rectDest.w,		// width, height
		0.0f, 1.0f,					// min/max depth
	};

	pCtx->RSSetViewports(1, &viewport);
	pCtx->OMSetRenderTargets(1, &pRtvDest, pDsvDest);

	XMFLOAT4X4 matIdentity;
	if (!pMatTransformColor)
	{
		pMatTransformColor = &matIdentity;
		XMStoreFloat4x4(&matIdentity, XMMatrixIdentity());
	}

	g_shdmgr.BindCopy(pCtx, pSrvSrc, *pMatTransformColor);
	g_meshFullscreen.Draw(pCtx);
}
