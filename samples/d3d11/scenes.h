//----------------------------------------------------------------------------------
// File:        FaceWorks/samples/sample_d3d11/scenes.h
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


#pragma once

#include "util.h"
#include "shader.h"



struct MeshToDraw
{
	Material *	m_pMtl;
	CMesh *		m_pMesh;

	// Parameters for SSS
	int			m_normalMapSize;			// Pixel size of normal map
	float		m_averageUVScale;			// Average UV scale of the mesh
};

class CScene
{
public:
	virtual ~CScene() = 0;

	virtual HRESULT Init() = 0;
	virtual void Release() = 0;

	virtual CBaseCamera * Camera() = 0;

	virtual void GetBounds(DirectX::XMFLOAT3 * pPosMin, DirectX::XMFLOAT3 * pPosMax) = 0;
	virtual void GetMeshesToDraw(std::vector<MeshToDraw> * pMeshesToDraw) = 0;
};

class CSceneDigitalIra : public CScene
{
public:
	CSceneDigitalIra();

	virtual HRESULT Init() override;
	virtual void Release() override;

	virtual CBaseCamera* Camera() override;

	virtual void GetBounds(DirectX::XMFLOAT3 * pPosMin, DirectX::XMFLOAT3 * pPosMax) override;
	virtual void GetMeshesToDraw(std::vector<MeshToDraw> * pMeshesToDraw) override;

	CMesh						m_meshHead;
	CMesh						m_meshEyeL;
	CMesh						m_meshEyeR;
	CMesh						m_meshLashes;
	CMesh						m_meshBrows;

	ID3D11ShaderResourceView *	m_pSrvDiffuseHead;
	ID3D11ShaderResourceView *	m_pSrvNormalHead;
	ID3D11ShaderResourceView *	m_pSrvSpecHead;
	ID3D11ShaderResourceView *	m_pSrvDeepScatterHead;

	ID3D11ShaderResourceView *	m_pSrvDiffuseEyeSclera;
	ID3D11ShaderResourceView *	m_pSrvNormalEyeSclera;
	ID3D11ShaderResourceView *	m_pSrvDiffuseEyeIris;

	ID3D11ShaderResourceView *	m_pSrvDiffuseLashes;
	ID3D11ShaderResourceView *	m_pSrvDiffuseBrows;

	Material					m_mtlHead;
	Material					m_mtlEye;
	Material					m_mtlLashes;
	Material					m_mtlBrows;

	CMayaStyleCamera			m_camera;

	int							m_normalHeadSize;
	int							m_normalEyeSize;
};

class CSceneTest : public CScene
{
public:
	CSceneTest();

	virtual HRESULT Init() override;
	virtual void Release() override;

	virtual CBaseCamera* Camera() override;

	virtual void GetBounds(DirectX::XMFLOAT3 * pPosMin, DirectX::XMFLOAT3 * pPosMax) override;
	virtual void GetMeshesToDraw(std::vector<MeshToDraw> * pMeshesToDraw) override;

	CMesh						m_meshPlanes;
	CMesh						m_meshShadower;
	CMesh						m_aMeshSpheres[7];

	ID3D11ShaderResourceView *	m_pSrvDiffuse;
	ID3D11ShaderResourceView *	m_pSrvNormalFlat;
	ID3D11ShaderResourceView *	m_pSrvSpec;
	ID3D11ShaderResourceView *	m_pSrvDeepScatter;

	Material					m_mtl;

	CFirstPersonCameraRH		m_camera;
};

class CSceneHand : public CScene
{
public:
	CSceneHand();

	virtual HRESULT Init() override;
	virtual void Release() override;

	virtual CBaseCamera* Camera() override;

	virtual void GetBounds(DirectX::XMFLOAT3 * pPosMin, DirectX::XMFLOAT3 * pPosMax) override;
	virtual void GetMeshesToDraw(std::vector<MeshToDraw> * pMeshesToDraw) override;

	CMesh						m_meshHand;

	ID3D11ShaderResourceView *	m_pSrvDiffuse;
	ID3D11ShaderResourceView *	m_pSrvNormalFlat;
	ID3D11ShaderResourceView *	m_pSrvSpec;
	ID3D11ShaderResourceView *	m_pSrvDeepScatter;

	Material					m_mtl;

	CMayaStyleCamera			m_camera;
};

class CSceneDragon : public CScene
{
public:
	CSceneDragon();

	virtual HRESULT Init();
	virtual void Release();

	virtual CBaseCamera * Camera() { return &m_camera; }

	virtual void GetBounds(DirectX::XMFLOAT3 * pPosMin, DirectX::XMFLOAT3 * pPosMax);
	virtual void GetMeshesToDraw(std::vector<MeshToDraw> * pMeshesToDraw);

	CMesh						m_meshDragon;

	ID3D11ShaderResourceView *	m_pSrvDiffuse;
	ID3D11ShaderResourceView *	m_pSrvNormal;
	ID3D11ShaderResourceView *	m_pSrvSpec;
	ID3D11ShaderResourceView *	m_pSrvDeepScatter;

	Material					m_mtl;

	CMayaStyleCamera			m_camera;

	int							m_normalSize;
};

class CSceneLPSHead : public CScene
{
public:
	CSceneLPSHead();

	virtual HRESULT Init();
	virtual void Release();

	virtual CBaseCamera * Camera() { return &m_camera; }

	virtual void GetBounds(DirectX::XMFLOAT3 * pPosMin, DirectX::XMFLOAT3 * pPosMax);
	virtual void GetMeshesToDraw(std::vector<MeshToDraw> * pMeshesToDraw);

	CMesh						m_meshHead;

	ID3D11ShaderResourceView *	m_pSrvDiffuseHead;
	ID3D11ShaderResourceView *	m_pSrvNormalHead;
	ID3D11ShaderResourceView *	m_pSrvSpecHead;
	ID3D11ShaderResourceView *	m_pSrvDeepScatterHead;

	Material					m_mtlHead;

	CMayaStyleCamera			m_camera;

	int							m_normalHeadSize;
};

class CSceneManjaladon : public CScene
{
public:
	CSceneManjaladon();

	virtual HRESULT Init();
	virtual void Release();

	virtual CBaseCamera * Camera() { return &m_camera; }

	virtual void GetBounds(DirectX::XMFLOAT3 * pPosMin, DirectX::XMFLOAT3 * pPosMax);
	virtual void GetMeshesToDraw(std::vector<MeshToDraw> * pMeshesToDraw);

	CMesh						m_meshManjaladon;

	ID3D11ShaderResourceView *	m_pSrvDiffuse;
	ID3D11ShaderResourceView *	m_pSrvNormal;
	ID3D11ShaderResourceView *	m_pSrvSpec;
	ID3D11ShaderResourceView *	m_pSrvDeepScatter;

	Material					m_mtl;

	CMayaStyleCamera			m_camera;

	int							m_normalSize;
};

class CSceneWarriorHead : public CScene
{
public:
	CSceneWarriorHead();

	virtual HRESULT Init();
	virtual void Release();

	virtual CBaseCamera * Camera() { return &m_camera; }

	virtual void GetBounds(DirectX::XMFLOAT3 * pPosMin, DirectX::XMFLOAT3 * pPosMax);
	virtual void GetMeshesToDraw(std::vector<MeshToDraw> * pMeshesToDraw);

	CMesh						m_meshHead;
	CMesh						m_meshEyeL;
	CMesh						m_meshEyeR;
	CMesh						m_meshLashes;

	ID3D11ShaderResourceView *	m_pSrvDiffuseHead;
	ID3D11ShaderResourceView *	m_pSrvNormalHead;
	ID3D11ShaderResourceView *	m_pSrvSpecHead;
	ID3D11ShaderResourceView *	m_pSrvDeepScatterHead;

	ID3D11ShaderResourceView *	m_pSrvDiffuseEyeSclera;
	ID3D11ShaderResourceView *	m_pSrvNormalEyeSclera;
	ID3D11ShaderResourceView *	m_pSrvDiffuseEyeIris;

	ID3D11ShaderResourceView *	m_pSrvDiffuseLashes;

	Material					m_mtlHead;
	Material					m_mtlEye;
	Material					m_mtlLashes;

	CMayaStyleCamera			m_camera;

	int							m_normalHeadSize;
	int							m_normalEyeSize;
};



class CBackground
{
public:
	CBackground();

	HRESULT Init(
		const wchar_t * strCubeEnv,
		const wchar_t * strCubeDiff,
		const wchar_t * strCubeSpec,
		float exposure = 1.0f);
	void Release();

	ID3D11ShaderResourceView *	m_pSrvCubeEnv;
	ID3D11ShaderResourceView *	m_pSrvCubeDiff;
	ID3D11ShaderResourceView *	m_pSrvCubeSpec;
	float						m_exposure;
};
