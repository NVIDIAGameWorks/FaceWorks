//----------------------------------------------------------------------------------
// File:        FaceWorks/samples/sample_d3d11/scenes.cpp
// SDK Version: v1.0
// Email:       gameworks@nvidia.com
// Site:        http://developer.nvidia.com/
//
// Copyright (c) 2014-2016, NVIDIA CORPORATION. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------------

#include "scenes.h"
#include "shaders/resources.h"

#include <algorithm>

#include <DirectXMath.h>

#include <DXUT/Core/DXUT.h>
#include <DXUT/Optional/SDKmisc.h>

using namespace std;
using namespace DirectX;

float	g_normalStrength = 1.0;
float	g_glossSkin = 0.35f;
float	g_glossEye = 0.9f;
float	g_specReflectanceSkinDefault = 0.05f;
float	g_specReflectanceEye = 0.05f;
float	g_rgbDeepScatterEye[3] = { 1.0f, 0.3f, 0.3f };
float	g_irisRadiusSource = 0.200f;	// Radius of iris in iris texture (in UV units)
float	g_irisRadiusDest = 0.205f;		// Radius of iris in schlera texture (in UV units)
float	g_irisEdgeHardness = 30.0f;		// Controls hardness/softness of iris edge
float	g_irisDilation = 0.0f;			// How much the iris is dilated
float	g_specReflectanceHair = 0.05f;
float	g_glossHair = 0.25f;



int GetTextureSize(ID3D11ShaderResourceView * pSrv)
{
	ID3D11Texture2D * pTex;
	pSrv->GetResource(reinterpret_cast<ID3D11Resource **>(&pTex));
	D3D11_TEXTURE2D_DESC texDesc;
	pTex->GetDesc(&texDesc);
	int texSize = (texDesc.Width + texDesc.Height) / 2;
	SAFE_RELEASE(pTex);
	return texSize;
}

ID3D11ShaderResourceView * Create1x1Texture(float r, float g, float b, bool linear = false)
{
	// Convert floats to 8-bit format

	unsigned char rgba[4] =
	{
		static_cast<unsigned char>(min(max(r, 0.0f), 1.0f) * 255.0f + 0.5f),
		static_cast<unsigned char>(min(max(g, 0.0f), 1.0f) * 255.0f + 0.5f),
		static_cast<unsigned char>(min(max(b, 0.0f), 1.0f) * 255.0f + 0.5f),
		255
	};

	D3D11_TEXTURE2D_DESC texDesc = 
	{
		1, 1, 1, 1,
		linear ? DXGI_FORMAT_R8G8B8A8_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		{ 1, 0 },
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_SHADER_RESOURCE,
		0, 0,
	};

	D3D11_SUBRESOURCE_DATA initialData = { rgba, 4, };

	ID3D11Device * pDevice = DXUTGetD3D11Device();
	ID3D11Texture2D * pTex = nullptr;
	HRESULT hr;
	V(pDevice->CreateTexture2D(&texDesc, &initialData, &pTex));

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc =
	{
		texDesc.Format,
		D3D11_SRV_DIMENSION_TEXTURE2D,
	};
	srvDesc.Texture2D.MipLevels = 1;

	ID3D11ShaderResourceView * pSrv = nullptr;
	V(pDevice->CreateShaderResourceView(pTex, &srvDesc, &pSrv));

	SAFE_RELEASE(pTex);
	return pSrv;
}

// CScene implementation

CScene::~CScene() {}

// CSceneDigitalIra implementation

CSceneDigitalIra::CSceneDigitalIra()
:	m_meshHead(),
	m_meshEyeL(),
	m_meshEyeR(),
	m_meshLashes(),
	m_meshBrows(),
	m_pSrvDiffuseHead(nullptr),
	m_pSrvNormalHead(nullptr),
	m_pSrvSpecHead(nullptr),
	m_pSrvDeepScatterHead(nullptr),
	m_pSrvDiffuseEyeSclera(nullptr),
	m_pSrvNormalEyeSclera(nullptr),
	m_pSrvDiffuseEyeIris(nullptr),
	m_pSrvDiffuseLashes(nullptr),
	m_pSrvDiffuseBrows(nullptr),
	m_mtlHead(),
	m_mtlEye(),
	m_mtlLashes(),
	m_mtlBrows(),
	m_camera(),
	m_normalHeadSize(0),
	m_normalEyeSize(0)
{
}

HRESULT CSceneDigitalIra::Init()
{
	HRESULT hr;
	wchar_t strPath[MAX_PATH];
	ID3D11Device * pDevice = DXUTGetD3D11Device();
	ID3D11DeviceContext *pDeviceContext = DXUTGetD3D11DeviceContext();

	// Load meshes

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"DigitalIra\\HumanHead.obj"));
	V_RETURN(LoadObjMesh(strPath, pDevice, &m_meshHead));

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"DigitalIra\\EyeL.obj"));
	V_RETURN(LoadObjMesh(strPath, pDevice, &m_meshEyeL));

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"DigitalIra\\EyeR.obj"));
	V_RETURN(LoadObjMesh(strPath, pDevice, &m_meshEyeR));

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"DigitalIra\\Lashes.obj"));
	V_RETURN(LoadObjMesh(strPath, pDevice, &m_meshLashes));

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"DigitalIra\\Brows.obj"));
	V_RETURN(LoadObjMesh(strPath, pDevice, &m_meshBrows));

	// Load textures

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"DigitalIra\\00_diffuse_albedo.bmp"));
	V_RETURN(LoadTexture(strPath, pDevice, pDeviceContext, &m_pSrvDiffuseHead));

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"DigitalIra\\00_specular_normal_tangent.bmp"));
	V_RETURN(LoadTexture(strPath, pDevice, pDeviceContext, &m_pSrvNormalHead, LT_Mipmap | LT_Linear));

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"DigitalIra\\00_specular_albedo.bmp"));
	V_RETURN(LoadTexture(strPath, pDevice, pDeviceContext, &m_pSrvSpecHead));

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"DigitalIra\\HumanHead_deepscatter.bmp"));
	V_RETURN(LoadTexture(strPath, pDevice, pDeviceContext, &m_pSrvDeepScatterHead));

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"DigitalIra\\sclera_col.bmp"));
	V_RETURN(LoadTexture(strPath, pDevice, pDeviceContext, &m_pSrvDiffuseEyeSclera));

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"DigitalIra\\eyeballNormalMap.bmp"));
	V_RETURN(LoadTexture(strPath, pDevice, pDeviceContext, &m_pSrvNormalEyeSclera, LT_Mipmap | LT_Linear));

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"DigitalIra\\iris.bmp"));
	V_RETURN(LoadTexture(strPath, pDevice, pDeviceContext, &m_pSrvDiffuseEyeIris));

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"DigitalIra\\lashes.dds"));
	V_RETURN(LoadTexture(strPath, pDevice, pDeviceContext, &m_pSrvDiffuseLashes));

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"DigitalIra\\brows.dds"));
	V_RETURN(LoadTexture(strPath, pDevice, pDeviceContext, &m_pSrvDiffuseBrows));

	// Set up materials

	m_mtlHead.m_shader = SHADER_Skin;
	m_mtlHead.m_aSrv[0] = m_pSrvDiffuseHead;		m_mtlHead.m_textureSlots[0] = TEX_DIFFUSE0;
	m_mtlHead.m_aSrv[1] = m_pSrvNormalHead;			m_mtlHead.m_textureSlots[1] = TEX_NORMAL;
	m_mtlHead.m_aSrv[2] = m_pSrvSpecHead;			m_mtlHead.m_textureSlots[2] = TEX_SPEC;
	m_mtlHead.m_aSrv[3] = m_pSrvDeepScatterHead;	m_mtlHead.m_textureSlots[3] = TEX_DEEP_SCATTER_COLOR;
	m_mtlHead.m_constants[0] = g_normalStrength;
	m_mtlHead.m_constants[1] = g_glossSkin;

	m_mtlEye.m_shader = SHADER_Eye;
	m_mtlEye.m_aSrv[0] = m_pSrvDiffuseEyeSclera;	m_mtlEye.m_textureSlots[0] = TEX_DIFFUSE0;
	m_mtlEye.m_aSrv[1] = m_pSrvDiffuseEyeIris;		m_mtlEye.m_textureSlots[1] = TEX_DIFFUSE1;
	m_mtlEye.m_aSrv[2] = m_pSrvNormalEyeSclera;		m_mtlEye.m_textureSlots[2] = TEX_NORMAL;
	m_mtlEye.m_constants[0] = 0.05f;	// normal strength
	m_mtlEye.m_constants[1] = g_specReflectanceEye;
	m_mtlEye.m_constants[2] = g_glossEye;
	m_mtlEye.m_constants[4] = g_rgbDeepScatterEye[0];
	m_mtlEye.m_constants[5] = g_rgbDeepScatterEye[1];
	m_mtlEye.m_constants[6] = g_rgbDeepScatterEye[2];
	m_mtlEye.m_constants[7] = 0.455f;	// Radius of iris in iris texture (in UV units);
	m_mtlEye.m_constants[8] = 0.230f;	// Radius of iris in schlera texture (in UV units);
	m_mtlEye.m_constants[9] = 30.0f;	// Controls hardness/softness of iris edge;
	m_mtlEye.m_constants[10] = 0.5f;	// How much the iris is dilated;

	m_mtlLashes.m_shader = SHADER_Hair;
	m_mtlLashes.m_aSrv[0] = m_pSrvDiffuseLashes;	m_mtlLashes.m_textureSlots[0] = TEX_DIFFUSE0;
	m_mtlLashes.m_constants[0] = g_specReflectanceHair;
	m_mtlLashes.m_constants[1] = g_glossHair;

	m_mtlBrows.m_shader = SHADER_Hair;
	m_mtlBrows.m_aSrv[0] = m_pSrvDiffuseBrows;		m_mtlBrows.m_textureSlots[0] = TEX_DIFFUSE0;
	m_mtlBrows.m_constants[0] = g_specReflectanceHair;
	m_mtlBrows.m_constants[1] = g_glossHair;

	// Set up camera to orbit around the head

	XMVECTOR posLookAt = XMLoadFloat3(&m_meshHead.m_posCenter);
	XMVECTOR posCamera = posLookAt + XMVectorSet(0.0f, 0.0f, 60.0f, 0.0f);
	m_camera.SetViewParams(posCamera, posLookAt);

	// Pull out normal map texture sizes for SSS mip level calculations

	m_normalHeadSize = GetTextureSize(m_pSrvNormalHead);
	m_normalEyeSize = GetTextureSize(m_pSrvNormalEyeSclera);

	return S_OK;
}

void CSceneDigitalIra::Release()
{
	m_meshHead.Release();
	m_meshEyeL.Release();
	m_meshEyeR.Release();
	m_meshLashes.Release();
	m_meshBrows.Release();

	SAFE_RELEASE(m_pSrvDiffuseHead);
	SAFE_RELEASE(m_pSrvNormalHead);
	SAFE_RELEASE(m_pSrvSpecHead);
	SAFE_RELEASE(m_pSrvDeepScatterHead);

	SAFE_RELEASE(m_pSrvDiffuseEyeSclera);
	SAFE_RELEASE(m_pSrvNormalEyeSclera);
	SAFE_RELEASE(m_pSrvDiffuseEyeIris);

	SAFE_RELEASE(m_pSrvDiffuseLashes);
	SAFE_RELEASE(m_pSrvDiffuseBrows);
}

CBaseCamera* CSceneDigitalIra::Camera()
{
	return &m_camera;
}

void CSceneDigitalIra::GetBounds(XMFLOAT3 * pPosMin, XMFLOAT3 * pPosMax)
{
	assert(pPosMin);
	assert(pPosMax);

	*pPosMin = m_meshHead.m_posMin;
	*pPosMax = m_meshHead.m_posMax;
}

void CSceneDigitalIra::GetMeshesToDraw(std::vector<MeshToDraw> * pMeshesToDraw)
{
	assert(pMeshesToDraw);

	// Allow updating normal strength and gloss in real-time
	m_mtlHead.m_constants[0] = g_normalStrength;
	m_mtlHead.m_constants[1] = g_glossSkin;
	m_mtlEye.m_constants[2] = g_glossEye;
	m_mtlLashes.m_constants[0] = g_specReflectanceHair;
	m_mtlLashes.m_constants[1] = g_glossHair;
	m_mtlBrows.m_constants[0] = g_specReflectanceHair;
	m_mtlBrows.m_constants[1] = g_glossHair;

	// Generate draw records

	MeshToDraw aMtd[] = 
	{
		{ &m_mtlHead, &m_meshHead, m_normalHeadSize, m_meshHead.m_uvScale, },
		{ &m_mtlEye, &m_meshEyeL, m_normalEyeSize, m_meshEyeL.m_uvScale, },
		{ &m_mtlEye, &m_meshEyeR, m_normalEyeSize, m_meshEyeR.m_uvScale, },
		{ &m_mtlLashes, &m_meshLashes, },
		{ &m_mtlBrows, &m_meshBrows, },
	};

	pMeshesToDraw->assign(&aMtd[0], &aMtd[dim(aMtd)]);
}



// CSceneTest implementation

CSceneTest::CSceneTest()
:	m_meshPlanes(),
	m_meshShadower(),
	m_aMeshSpheres(),
	m_pSrvDiffuse(nullptr),
	m_pSrvNormalFlat(nullptr),
	m_pSrvSpec(nullptr),
	m_pSrvDeepScatter(nullptr),
	m_mtl(),
	m_camera()
{
}

HRESULT CSceneTest::Init()
{
	HRESULT hr;
	wchar_t strPath[MAX_PATH];
	ID3D11Device * pDevice = DXUTGetD3D11Device();

	// Load meshes

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"testPlanes.obj"));
	V_RETURN(LoadObjMesh(strPath, pDevice, &m_meshPlanes));

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"testShadowCaster.obj"));
	V_RETURN(LoadObjMesh(strPath, pDevice, &m_meshShadower));

	static const wchar_t * aStrSphereNames[] =
	{
		L"testSphere1mm.obj",
		L"testSphere2mm.obj",
		L"testSphere5mm.obj",
		L"testSphere1cm.obj",
		L"testSphere2cm.obj",
		L"testSphere5cm.obj",
		L"testSphere10cm.obj",
	};
	static_assert(dim(aStrSphereNames) == dim(m_aMeshSpheres), "dimension mismatch between array aStrSphereNames and m_aMeshSpheres");

	for (int i = 0; i < dim(m_aMeshSpheres); ++i)
	{
		V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), aStrSphereNames[i]));
		V_RETURN(LoadObjMesh(strPath, pDevice, &m_aMeshSpheres[i]));
	}

	// Create 1x1 textures

	m_pSrvDiffuse = Create1x1Texture(1.0f, 1.0f, 1.0f);
	m_pSrvNormalFlat = Create1x1Texture(0.5f, 0.5f, 1.0f, true);
	m_pSrvSpec = Create1x1Texture(0.05f, 0.05f, 0.05f, true);
	m_pSrvDeepScatter = Create1x1Texture(1.0f, 0.25f, 0.0f);

	// Set up material

	m_mtl.m_shader = SHADER_Skin;
	m_mtl.m_aSrv[0] = m_pSrvDiffuse;		m_mtl.m_textureSlots[0] = TEX_DIFFUSE0;
	m_mtl.m_aSrv[1] = m_pSrvNormalFlat;		m_mtl.m_textureSlots[1] = TEX_NORMAL;
	m_mtl.m_aSrv[2] = m_pSrvSpec;			m_mtl.m_textureSlots[2] = TEX_SPEC;
	m_mtl.m_aSrv[2] = m_pSrvDeepScatter;	m_mtl.m_textureSlots[3] = TEX_DEEP_SCATTER_COLOR;
	m_mtl.m_constants[0] = g_normalStrength;
	m_mtl.m_constants[1] = g_glossSkin;

	// Set up camera in a default location

	XMFLOAT3 posMin, posMax;
	GetBounds(&posMin, &posMax);
	XMVECTOR posLookAt = 0.5f * (XMLoadFloat3(&posMin) + XMLoadFloat3(&posMax));
	XMVECTOR posCamera = posLookAt + XMVectorSet(0.0f, 0.0f, 60.0f, 0.0f);
	m_camera.SetViewParams(posCamera, posLookAt);

	// Adjust camera speed
	m_camera.SetScalers(0.005f, 10.0f);

	return S_OK;
}

void CSceneTest::Release()
{
	m_meshPlanes.Release();
	m_meshShadower.Release();

	for (int i = 0; i < dim(m_aMeshSpheres); ++i)
		m_aMeshSpheres[i].Release();

	SAFE_RELEASE(m_pSrvDiffuse);
	SAFE_RELEASE(m_pSrvNormalFlat);
	SAFE_RELEASE(m_pSrvSpec);
	SAFE_RELEASE(m_pSrvDeepScatter);
}

CBaseCamera* CSceneTest::Camera()
{
	return &m_camera;
}

void CSceneTest::GetBounds(XMFLOAT3 * pPosMin, XMFLOAT3 * pPosMax)
{
	assert(pPosMin);
	assert(pPosMax);

	XMVECTOR posMin = XMLoadFloat3(&m_meshPlanes.m_posMin);
	XMVECTOR posMax = XMLoadFloat3(&m_meshPlanes.m_posMax);

	posMin = XMVectorMin(posMin, XMLoadFloat3(&m_meshShadower.m_posMin));
	posMax = XMVectorMax(posMax, XMLoadFloat3(&m_meshShadower.m_posMax));

	for (int i = 0; i < dim(m_aMeshSpheres); ++i)
	{
		posMin = XMVectorMin(posMin, XMLoadFloat3(&m_aMeshSpheres[i].m_posMin));
		posMax = XMVectorMax(posMax, XMLoadFloat3(&m_aMeshSpheres[i].m_posMax));
	}

	XMStoreFloat3(pPosMin, posMin);
	XMStoreFloat3(pPosMax, posMax);
}

void CSceneTest::GetMeshesToDraw(std::vector<MeshToDraw> * pMeshesToDraw)
{
	assert(pMeshesToDraw);

	// Allow updating normal strength and gloss in real-time
	m_mtl.m_constants[0] = g_normalStrength;
	m_mtl.m_constants[1] = g_glossSkin;

	// Generate draw records

	MeshToDraw aMtd[] = 
	{
		{ &m_mtl, &m_meshShadower, 0, 1.0f, },
		{ &m_mtl, &m_meshPlanes, 0, 1.0f, },
	};
	pMeshesToDraw->assign(&aMtd[0], &aMtd[dim(aMtd)]);

	for (int i = 0; i < dim(m_aMeshSpheres); ++i)
	{
		MeshToDraw mtd = { &m_mtl, &m_aMeshSpheres[i], 0, 1.0f };
		pMeshesToDraw->push_back(mtd);
	}
}



// CSceneHand implementation

CSceneHand::CSceneHand()
:	m_meshHand(),
	m_pSrvDiffuse(nullptr),
	m_pSrvNormalFlat(nullptr),
	m_pSrvSpec(nullptr),
	m_pSrvDeepScatter(nullptr),
	m_mtl(),
	m_camera()
{
}

HRESULT CSceneHand::Init()
{
	HRESULT hr;
	wchar_t strPath[MAX_PATH];
	ID3D11Device * pDevice = DXUTGetD3D11Device();

	// Load meshes

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"hand01.obj"));
	V_RETURN(LoadObjMesh(strPath, pDevice, &m_meshHand));

	// Create 1x1 textures

	m_pSrvDiffuse = Create1x1Texture(0.773f, 0.540f, 0.442f);	// caucasian skin color
	m_pSrvNormalFlat = Create1x1Texture(0.5f, 0.5f, 1.0f, true);
	m_pSrvSpec = Create1x1Texture(0.05f, 0.05f, 0.05f, true);
	m_pSrvDeepScatter = Create1x1Texture(1.0f, 0.25f, 0.0f);

	// Set up material

	m_mtl.m_shader = SHADER_Skin;
	m_mtl.m_aSrv[0] = m_pSrvDiffuse;		m_mtl.m_textureSlots[0] = TEX_DIFFUSE0;
	m_mtl.m_aSrv[1] = m_pSrvNormalFlat;		m_mtl.m_textureSlots[1] = TEX_NORMAL;
	m_mtl.m_aSrv[2] = m_pSrvSpec;			m_mtl.m_textureSlots[2] = TEX_SPEC;
	m_mtl.m_aSrv[3] = m_pSrvDeepScatter;	m_mtl.m_textureSlots[3] = TEX_DEEP_SCATTER_COLOR;
	m_mtl.m_constants[0] = g_normalStrength;
	m_mtl.m_constants[1] = g_glossSkin;

	// Set up camera to orbit around the hand

	XMVECTOR posLookAt = XMLoadFloat3(&m_meshHand.m_posCenter);
	XMVECTOR posCamera = posLookAt + XMVectorSet(0.0f, 0.0f, 60.0f, 0.0f);
	m_camera.SetViewParams(posCamera, posLookAt);

	return S_OK;
}

void CSceneHand::Release()
{
	m_meshHand.Release();

	SAFE_RELEASE(m_pSrvDiffuse);
	SAFE_RELEASE(m_pSrvNormalFlat);
	SAFE_RELEASE(m_pSrvSpec);
	SAFE_RELEASE(m_pSrvDeepScatter);
}

CBaseCamera* CSceneHand::Camera()
{
	return &m_camera;
}

void CSceneHand::GetBounds(XMFLOAT3 * pPosMin, XMFLOAT3 * pPosMax)
{
	assert(pPosMin);
	assert(pPosMax);

	*pPosMin = m_meshHand.m_posMin;
	*pPosMax = m_meshHand.m_posMax;
}

void CSceneHand::GetMeshesToDraw(std::vector<MeshToDraw> * pMeshesToDraw)
{
	assert(pMeshesToDraw);

	// Allow updating normal strength and gloss in real-time
	m_mtl.m_constants[0] = g_normalStrength;
	m_mtl.m_constants[1] = g_glossSkin;

	// Generate draw records

	MeshToDraw aMtd[] = 
	{
		{ &m_mtl, &m_meshHand, 0, 1.0f, },
	};
	pMeshesToDraw->assign(&aMtd[0], &aMtd[dim(aMtd)]);
}



// CSceneDragon implementation

CSceneDragon::CSceneDragon()
:	m_meshDragon(),
	m_pSrvDiffuse(nullptr),
	m_pSrvNormal(nullptr),
	m_pSrvSpec(nullptr),
	m_pSrvDeepScatter(nullptr),
	m_mtl(),
	m_camera(),
	m_normalSize(0)
{
}

HRESULT CSceneDragon::Init()
{
	HRESULT hr;
	wchar_t strPath[MAX_PATH];
	ID3D11Device * pDevice = DXUTGetD3D11Device();
	ID3D11DeviceContext *pDeviceContext = DXUTGetD3D11DeviceContext();

	// Load meshes

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"Dragon\\Dragon_gdc2014_BindPose.obj"));
	V_RETURN(LoadObjMesh(strPath, pDevice, &m_meshDragon));

	// Load textures

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"Dragon\\Dragon_New_D.bmp"));
	V_RETURN(LoadTexture(strPath, pDevice, pDeviceContext, &m_pSrvDiffuse));

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"Dragon\\Dragon_New_N.bmp"));
	V_RETURN(LoadTexture(strPath, pDevice, pDeviceContext, &m_pSrvNormal, LT_Mipmap | LT_Linear));

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"Dragon\\Dragon_New_S.bmp"));
	V_RETURN(LoadTexture(strPath, pDevice, pDeviceContext, &m_pSrvSpec));

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"Dragon\\Dragon_Subsurface.bmp"));
	V_RETURN(LoadTexture(strPath, pDevice, pDeviceContext, &m_pSrvDeepScatter));

	// Set up materials

	m_mtl.m_shader = SHADER_Skin;
	m_mtl.m_aSrv[0] = m_pSrvDiffuse;		m_mtl.m_textureSlots[0] = TEX_DIFFUSE0;
	m_mtl.m_aSrv[1] = m_pSrvNormal;			m_mtl.m_textureSlots[1] = TEX_NORMAL;
	m_mtl.m_aSrv[2] = m_pSrvSpec;			m_mtl.m_textureSlots[2] = TEX_SPEC;
	m_mtl.m_aSrv[3] = m_pSrvDeepScatter;	m_mtl.m_textureSlots[3] = TEX_DEEP_SCATTER_COLOR;
	m_mtl.m_constants[0] = g_normalStrength;
	m_mtl.m_constants[1] = g_glossSkin;

	// Set up camera to orbit around the dragon

	XMVECTOR posLookAt = XMLoadFloat3(&m_meshDragon.m_posCenter) + XMVectorSet(0.0f, 0.0f, 5.0f, 0.0f);
	XMVECTOR posCamera = posLookAt + XMVectorSet(0.0f, 0.0f, 40.0f, 0.0f);
	m_camera.SetViewParams(posCamera, posLookAt);

	// Pull out normal map texture size for SSS mip level calculations

	m_normalSize = GetTextureSize(m_pSrvNormal);

	return S_OK;
}

void CSceneDragon::Release()
{
	m_meshDragon.Release();

	SAFE_RELEASE(m_pSrvDiffuse);
	SAFE_RELEASE(m_pSrvNormal);
	SAFE_RELEASE(m_pSrvSpec);
	SAFE_RELEASE(m_pSrvDeepScatter);
}

void CSceneDragon::GetBounds(XMFLOAT3 * pPosMin, XMFLOAT3 * pPosMax)
{
	assert(pPosMin);
	assert(pPosMax);

	*pPosMin = m_meshDragon.m_posMin;
	*pPosMax = m_meshDragon.m_posMax;
}

void CSceneDragon::GetMeshesToDraw(std::vector<MeshToDraw> * pMeshesToDraw)
{
	assert(pMeshesToDraw);

	// Allow updating normal strength and gloss in real-time
	m_mtl.m_constants[0] = g_normalStrength;
	m_mtl.m_constants[1] = g_glossSkin;

	// Generate draw records

	MeshToDraw aMtd[] = 
	{
		{ &m_mtl, &m_meshDragon, m_normalSize, m_meshDragon.m_uvScale, },
	};
	pMeshesToDraw->assign(&aMtd[0], &aMtd[dim(aMtd)]);
}



// CSceneLPSHead implementation

CSceneLPSHead::CSceneLPSHead()
:	m_meshHead(),
	m_pSrvDiffuseHead(nullptr),
	m_pSrvNormalHead(nullptr),
	m_pSrvSpecHead(nullptr),
	m_pSrvDeepScatterHead(nullptr),
	m_mtlHead(),
	m_camera(),
	m_normalHeadSize(0)
{
}

HRESULT CSceneLPSHead::Init()
{
	HRESULT hr;
	wchar_t strPath[MAX_PATH];
	ID3D11Device * pDevice = DXUTGetD3D11Device();
	ID3D11DeviceContext *pDeviceContext = DXUTGetD3D11DeviceContext();

	// Load meshes

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"LPSHead\\head.obj"));
	V_RETURN(LoadObjMesh(strPath, pDevice, &m_meshHead));

	// Load textures

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"LPSHead\\lambertian.jpg"));
	V_RETURN(LoadTexture(strPath, pDevice, pDeviceContext, &m_pSrvDiffuseHead));

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"LPSHead\\normal.bmp"));
	V_RETURN(LoadTexture(strPath, pDevice, pDeviceContext, &m_pSrvNormalHead, LT_Mipmap | LT_Linear));

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"LPSHead\\deepscatter.bmp"));
	V_RETURN(LoadTexture(strPath, pDevice, pDeviceContext, &m_pSrvDeepScatterHead));

	// Create 1x1 spec texture

	m_pSrvSpecHead = Create1x1Texture(
						g_specReflectanceSkinDefault,
						g_specReflectanceSkinDefault,
						g_specReflectanceSkinDefault,
						true);

	// Set up materials

	m_mtlHead.m_shader = SHADER_Skin;
	m_mtlHead.m_aSrv[0] = m_pSrvDiffuseHead;		m_mtlHead.m_textureSlots[0] = TEX_DIFFUSE0;
	m_mtlHead.m_aSrv[1] = m_pSrvNormalHead;			m_mtlHead.m_textureSlots[1] = TEX_NORMAL;
	m_mtlHead.m_aSrv[2] = m_pSrvSpecHead;			m_mtlHead.m_textureSlots[2] = TEX_SPEC;
	m_mtlHead.m_aSrv[3] = m_pSrvDeepScatterHead;	m_mtlHead.m_textureSlots[3] = TEX_DEEP_SCATTER_COLOR;
	m_mtlHead.m_constants[0] = g_normalStrength;

	// Set up camera to orbit around the head

	XMVECTOR posLookAt = XMLoadFloat3(&m_meshHead.m_posCenter) + XMVectorSet(0.0f, 3.0f, 0.0f, 0.0f);
	XMVECTOR posCamera = posLookAt + XMVectorSet(0.0f, 0.0f, 60.0f, 0.0f);
	m_camera.SetViewParams(posCamera, posLookAt);

	// Pull out normal map texture size for SSS mip level calculations

	m_normalHeadSize = GetTextureSize(m_pSrvNormalHead);

	return S_OK;
}

void CSceneLPSHead::Release()
{
	m_meshHead.Release();

	SAFE_RELEASE(m_pSrvDiffuseHead);
	SAFE_RELEASE(m_pSrvNormalHead);
	SAFE_RELEASE(m_pSrvSpecHead);
	SAFE_RELEASE(m_pSrvDeepScatterHead);
}

void CSceneLPSHead::GetBounds(XMFLOAT3 * pPosMin, XMFLOAT3 * pPosMax)
{
	assert(pPosMin);
	assert(pPosMax);

	*pPosMin = m_meshHead.m_posMin;
	*pPosMax = m_meshHead.m_posMax;
}

void CSceneLPSHead::GetMeshesToDraw(std::vector<MeshToDraw> * pMeshesToDraw)
{
	assert(pMeshesToDraw);

	// Allow updating normal strength and gloss in real-time
	m_mtlHead.m_constants[0] = g_normalStrength;
	m_mtlHead.m_constants[1] = g_glossSkin;

	// Generate draw records

	MeshToDraw aMtd[] = 
	{
		{ &m_mtlHead, &m_meshHead, m_normalHeadSize, m_meshHead.m_uvScale, },
	};

	pMeshesToDraw->assign(&aMtd[0], &aMtd[dim(aMtd)]);
}



// CSceneManjaladon implementation

CSceneManjaladon::CSceneManjaladon()
:	m_meshManjaladon(),
	m_pSrvDiffuse(nullptr),
	m_pSrvNormal(nullptr),
	m_pSrvSpec(nullptr),
	m_pSrvDeepScatter(nullptr),
	m_mtl(),
	m_camera(),
	m_normalSize(0)
{
}

HRESULT CSceneManjaladon::Init()
{
	HRESULT hr;
	wchar_t strPath[MAX_PATH];
	ID3D11Device * pDevice = DXUTGetD3D11Device();
	ID3D11DeviceContext *pDeviceContext = DXUTGetD3D11DeviceContext();

	// Load meshes

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"Manjaladon\\manjaladon.obj"));
	V_RETURN(LoadObjMesh(strPath, pDevice, &m_meshManjaladon));

	// Load textures

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"Manjaladon\\Manjaladon_d.bmp"));
	V_RETURN(LoadTexture(strPath, pDevice, pDeviceContext, &m_pSrvDiffuse));

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"Manjaladon\\Manjaladon_n.bmp"));
	V_RETURN(LoadTexture(strPath, pDevice, pDeviceContext, &m_pSrvNormal, LT_Mipmap | LT_Linear));

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"Manjaladon\\Manjaladon_s.bmp"));
	V_RETURN(LoadTexture(strPath, pDevice, pDeviceContext, &m_pSrvSpec));

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"Manjaladon\\Manjaladon_subsurface.bmp"));
	V_RETURN(LoadTexture(strPath, pDevice, pDeviceContext, &m_pSrvDeepScatter));

	// Set up materials

	m_mtl.m_shader = SHADER_Skin;
	m_mtl.m_aSrv[0] = m_pSrvDiffuse;		m_mtl.m_textureSlots[0] = TEX_DIFFUSE0;
	m_mtl.m_aSrv[1] = m_pSrvNormal;			m_mtl.m_textureSlots[1] = TEX_NORMAL;
	m_mtl.m_aSrv[2] = m_pSrvSpec;			m_mtl.m_textureSlots[2] = TEX_SPEC;
	m_mtl.m_aSrv[3] = m_pSrvDeepScatter;	m_mtl.m_textureSlots[3] = TEX_DEEP_SCATTER_COLOR;
	m_mtl.m_constants[0] = g_normalStrength;
	m_mtl.m_constants[1] = g_glossSkin;

	// Set up camera to orbit around the manjaladon

	XMVECTOR posLookAt = XMLoadFloat3(&m_meshManjaladon.m_posCenter) + XMVectorSet(0.0f, 0.0f, 5.0f, 0.0f);
	XMVECTOR posCamera = posLookAt + XMVectorSet(0.0f, 0.0f, 40.0f, 0.0f);
	m_camera.SetViewParams(posCamera, posLookAt);

	// Pull out normal map texture size for SSS mip level calculations

	m_normalSize = GetTextureSize(m_pSrvNormal);

	return S_OK;
}

void CSceneManjaladon::Release()
{
	m_meshManjaladon.Release();

	SAFE_RELEASE(m_pSrvDiffuse);
	SAFE_RELEASE(m_pSrvNormal);
	SAFE_RELEASE(m_pSrvSpec);
	SAFE_RELEASE(m_pSrvDeepScatter);
}

void CSceneManjaladon::GetBounds(XMFLOAT3 * pPosMin, XMFLOAT3 * pPosMax)
{
	assert(pPosMin);
	assert(pPosMax);

	*pPosMin = m_meshManjaladon.m_posMin;
	*pPosMax = m_meshManjaladon.m_posMax;
}

void CSceneManjaladon::GetMeshesToDraw(std::vector<MeshToDraw> * pMeshesToDraw)
{
	assert(pMeshesToDraw);

	// Allow updating normal strength and gloss in real-time
	m_mtl.m_constants[0] = g_normalStrength;
	m_mtl.m_constants[1] = g_glossSkin;

	// Generate draw records

	MeshToDraw aMtd[] = 
	{
		{ &m_mtl, &m_meshManjaladon, m_normalSize, m_meshManjaladon.m_uvScale, },
	};
	pMeshesToDraw->assign(&aMtd[0], &aMtd[dim(aMtd)]);
}



// CSceneWarriorHead implementation

CSceneWarriorHead::CSceneWarriorHead()
:	m_meshHead(),
	m_meshEyeL(),
	m_meshEyeR(),
	m_meshLashes(),
	m_pSrvDiffuseHead(nullptr),
	m_pSrvNormalHead(nullptr),
	m_pSrvSpecHead(nullptr),
	m_pSrvDeepScatterHead(nullptr),
	m_pSrvDiffuseEyeSclera(nullptr),
	m_pSrvNormalEyeSclera(nullptr),
	m_pSrvDiffuseEyeIris(nullptr),
	m_pSrvDiffuseLashes(nullptr),
	m_mtlHead(),
	m_mtlEye(),
	m_mtlLashes(),
	m_camera(),
	m_normalHeadSize(0),
	m_normalEyeSize(0)
{
}

HRESULT CSceneWarriorHead::Init()
{
	HRESULT hr;
	wchar_t strPath[MAX_PATH];
	ID3D11Device * pDevice = DXUTGetD3D11Device();
	ID3D11DeviceContext *pDeviceContext = DXUTGetD3D11DeviceContext();

	// Load meshes

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"WarriorHead\\WarriorHead.obj"));
	V_RETURN(LoadObjMesh(strPath, pDevice, &m_meshHead));

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"WarriorHead\\EyeL.obj"));
	V_RETURN(LoadObjMesh(strPath, pDevice, &m_meshEyeL));

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"WarriorHead\\EyeR.obj"));
	V_RETURN(LoadObjMesh(strPath, pDevice, &m_meshEyeR));

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"WarriorHead\\Lashes.obj"));
	V_RETURN(LoadObjMesh(strPath, pDevice, &m_meshLashes));

	// Load textures

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"WarriorHead\\diffuse.bmp"));
	V_RETURN(LoadTexture(strPath, pDevice, pDeviceContext, &m_pSrvDiffuseHead));

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"WarriorHead\\normal.bmp"));
	V_RETURN(LoadTexture(strPath, pDevice, pDeviceContext, &m_pSrvNormalHead, LT_Mipmap | LT_Linear));

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"WarriorHead\\deepscatter.bmp"));
	V_RETURN(LoadTexture(strPath, pDevice, pDeviceContext, &m_pSrvDeepScatterHead));

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"WarriorHead\\eyeHazel.bmp"));
	V_RETURN(LoadTexture(strPath, pDevice, pDeviceContext, &m_pSrvDiffuseEyeSclera));

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"DigitalIra\\eyeballNormalMap.bmp"));
	V_RETURN(LoadTexture(strPath, pDevice, pDeviceContext, &m_pSrvNormalEyeSclera, LT_Mipmap | LT_Linear));

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"WarriorHead\\eyeHazel.bmp"));
	V_RETURN(LoadTexture(strPath, pDevice, pDeviceContext, &m_pSrvDiffuseEyeIris));

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), L"WarriorHead\\lashes.bmp"));
	V_RETURN(LoadTexture(strPath, pDevice, pDeviceContext, &m_pSrvDiffuseLashes));

	// Create 1x1 spec texture

	m_pSrvSpecHead = Create1x1Texture(
						g_specReflectanceSkinDefault,
						g_specReflectanceSkinDefault,
						g_specReflectanceSkinDefault,
						true);

	// Set up materials

	m_mtlHead.m_shader = SHADER_Skin;
	m_mtlHead.m_aSrv[0] = m_pSrvDiffuseHead;		m_mtlHead.m_textureSlots[0] = TEX_DIFFUSE0;
	m_mtlHead.m_aSrv[1] = m_pSrvNormalHead;			m_mtlHead.m_textureSlots[1] = TEX_NORMAL;
	m_mtlHead.m_aSrv[2] = m_pSrvSpecHead;			m_mtlHead.m_textureSlots[2] = TEX_SPEC;
	m_mtlHead.m_aSrv[3] = m_pSrvDeepScatterHead;	m_mtlHead.m_textureSlots[3] = TEX_DEEP_SCATTER_COLOR;
	m_mtlHead.m_constants[0] = g_normalStrength;
	m_mtlHead.m_constants[1] = g_glossSkin;

	m_mtlEye.m_shader = SHADER_Eye;
	m_mtlEye.m_aSrv[0] = m_pSrvDiffuseEyeSclera;	m_mtlEye.m_textureSlots[0] = TEX_DIFFUSE0;
	m_mtlEye.m_aSrv[1] = m_pSrvDiffuseEyeIris;		m_mtlEye.m_textureSlots[1] = TEX_DIFFUSE1;
	m_mtlEye.m_aSrv[2] = m_pSrvNormalEyeSclera;		m_mtlEye.m_textureSlots[2] = TEX_NORMAL;
	m_mtlEye.m_constants[0] = 0.05f;	// normal strength
	m_mtlEye.m_constants[1] = g_specReflectanceEye;
	m_mtlEye.m_constants[2] = g_glossEye;
	m_mtlEye.m_constants[4] = g_rgbDeepScatterEye[0];
	m_mtlEye.m_constants[5] = g_rgbDeepScatterEye[1];
	m_mtlEye.m_constants[6] = g_rgbDeepScatterEye[2];
	m_mtlEye.m_constants[7] = 0.200f;	// Radius of iris in iris texture (in UV units)
	m_mtlEye.m_constants[8] = 0.205f;	// Radius of iris in schlera texture (in UV units)
	m_mtlEye.m_constants[9] = 30.0f;	// Controls hardness/softness of iris edge
	m_mtlEye.m_constants[10] = 0.0f;	// How much the iris is dilated

	m_mtlLashes.m_shader = SHADER_Hair;
	m_mtlLashes.m_aSrv[0] = m_pSrvDiffuseLashes;	m_mtlLashes.m_textureSlots[0] = TEX_DIFFUSE0;
	m_mtlLashes.m_constants[0] = g_specReflectanceHair;
	m_mtlLashes.m_constants[1] = g_glossHair;

	// Set up camera to orbit around the head

	XMVECTOR posLookAt = XMLoadFloat3(&m_meshHead.m_posCenter);
	XMVECTOR posCamera = posLookAt + XMVectorSet(0.0f, 0.0f, 60.0f, 0.0f);
	m_camera.SetViewParams(posCamera, posLookAt);

	// Pull out normal map texture sizes for SSS mip level calculations

	m_normalHeadSize = GetTextureSize(m_pSrvNormalHead);
	m_normalEyeSize = GetTextureSize(m_pSrvNormalEyeSclera);

	return S_OK;
}

void CSceneWarriorHead::Release()
{
	m_meshHead.Release();
	m_meshEyeL.Release();
	m_meshEyeR.Release();
	m_meshLashes.Release();

	SAFE_RELEASE(m_pSrvDiffuseHead);
	SAFE_RELEASE(m_pSrvNormalHead);
	SAFE_RELEASE(m_pSrvSpecHead);
	SAFE_RELEASE(m_pSrvDeepScatterHead);

	SAFE_RELEASE(m_pSrvDiffuseEyeSclera);
	SAFE_RELEASE(m_pSrvNormalEyeSclera);
	SAFE_RELEASE(m_pSrvDiffuseEyeIris);

	SAFE_RELEASE(m_pSrvDiffuseLashes);
}

void CSceneWarriorHead::GetBounds(XMFLOAT3 * pPosMin, XMFLOAT3 * pPosMax)
{
	assert(pPosMin);
	assert(pPosMax);

	*pPosMin = m_meshHead.m_posMin;
	*pPosMax = m_meshHead.m_posMax;
}

void CSceneWarriorHead::GetMeshesToDraw(std::vector<MeshToDraw> * pMeshesToDraw)
{
	assert(pMeshesToDraw);

	// Allow updating normal strength and gloss in real-time
	m_mtlHead.m_constants[0] = g_normalStrength;
	m_mtlHead.m_constants[1] = g_glossSkin;
	m_mtlEye.m_constants[2] = g_glossEye;

	m_mtlEye.m_constants[7] = g_irisRadiusSource;
	m_mtlEye.m_constants[8] = g_irisRadiusDest;
	m_mtlEye.m_constants[9] = g_irisEdgeHardness;
	m_mtlEye.m_constants[10] = g_irisDilation;

	m_mtlLashes.m_constants[0] = g_specReflectanceHair;
	m_mtlLashes.m_constants[1] = g_glossHair;

	// Generate draw records

	MeshToDraw aMtd[] = 
	{
		{ &m_mtlHead, &m_meshHead, m_normalHeadSize, m_meshHead.m_uvScale, },
		{ &m_mtlEye, &m_meshEyeL, m_normalEyeSize, m_meshEyeL.m_uvScale, },
		{ &m_mtlEye, &m_meshEyeR, m_normalEyeSize, m_meshEyeR.m_uvScale, },
		{ &m_mtlLashes, &m_meshLashes, },
	};

	pMeshesToDraw->assign(&aMtd[0], &aMtd[dim(aMtd)]);
}



// CBackground implementation

CBackground::CBackground()
:	m_pSrvCubeEnv(nullptr),
	m_pSrvCubeDiff(nullptr),
	m_pSrvCubeSpec(nullptr),
	m_exposure(0.0f)
{
}

HRESULT CBackground::Init(
	const wchar_t * strCubeEnv,
	const wchar_t * strCubeDiff,
	const wchar_t * strCubeSpec,
	float exposure /* = 1.0f */)
{
	HRESULT hr;
	wchar_t strPath[MAX_PATH];
	ID3D11Device * pDevice = DXUTGetD3D11Device();
	ID3D11DeviceContext *pDeviceContext = DXUTGetD3D11DeviceContext();

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), strCubeEnv));
	V_RETURN(LoadTexture(strPath, pDevice, &m_pSrvCubeEnv, LT_HDR | LT_Cubemap));

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), strCubeDiff));
	V_RETURN(LoadTexture(strPath, pDevice, &m_pSrvCubeDiff, LT_HDR | LT_Cubemap));

	V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, dim(strPath), strCubeSpec));
	V_RETURN(LoadTexture(strPath, pDevice, pDeviceContext, &m_pSrvCubeSpec, LT_Mipmap | LT_HDR | LT_Cubemap));

	m_exposure = exposure;

	return S_OK;
}

void CBackground::Release()
{
	SAFE_RELEASE(m_pSrvCubeEnv);
	SAFE_RELEASE(m_pSrvCubeDiff);
	SAFE_RELEASE(m_pSrvCubeSpec);
}
