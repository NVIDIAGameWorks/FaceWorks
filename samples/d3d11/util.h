//----------------------------------------------------------------------------------
// File:        FaceWorks/samples/sample_d3d11/util.h
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

#include <string>
#include <vector>

#include <DXUT/Core/DXUT.h>
#include <DXUT/Optional/DXUTcamera.h>

// Get the dimension of a static array
template <typename T, int N> char (&dim_helper(T (&)[N]))[N];
#define dim(x) (sizeof(dim_helper(x)))
#define dim_field(S, m) (dim(((S*)0)->m))
#define sizeof_field(S, m) (sizeof(((S*)0)->m))



// General utilities
std::wstring StrPrintf(const wchar_t * fmt, ...);
void DebugPrintf(const wchar_t * fmt, ...);
HRESULT LoadFile(const wchar_t * strFilename, std::vector<char> * pData, bool bText = false);
const wchar_t * BaseFilename(const wchar_t * strFilename);
void SetDebugName(ID3D11DeviceChild * pD3DObject, const char * strName);
void SetDebugName(ID3D11DeviceChild * pD3DObject, const wchar_t * strName);



// Mesh data & loading

struct Vertex
{
	DirectX::XMFLOAT3		m_pos;
	DirectX::XMFLOAT3		m_normal;
	DirectX::XMFLOAT2		m_uv;
	DirectX::XMFLOAT3		m_tangent;
	float					m_curvature;
};

class CMesh
{
public:
	std::vector<Vertex>			m_verts;
	std::vector<int>			m_indices;

	ID3D11Buffer *				m_pVtxBuffer;
	ID3D11Buffer *				m_pIdxBuffer;
	UINT						m_vtxStride;			// Vertex stride for IASetVertexBuffers
	UINT						m_cIdx;					// Index count for DrawIndexed
	D3D11_PRIMITIVE_TOPOLOGY	m_primtopo;
	DirectX::XMFLOAT3			m_posMin, m_posMax;		// Bounding box in local space
	DirectX::XMFLOAT3			m_posCenter;			// Center of bounding box
	float						m_diameter;				// Diameter of bounding box
	float						m_uvScale;				// Average world-space size of 1 UV unit

	CMesh();
	void Draw(ID3D11DeviceContext * pCtx);
	void Release();
};

HRESULT CreateFullscreenMesh(ID3D11Device * pDevice, CMesh * pMesh);
HRESULT LoadObjMesh(
			const wchar_t * strFilename,
			ID3D11Device * pDevice,
			CMesh * pMesh);



// Texture loading

enum LTFLAGS
{
	LT_None = 0,
	LT_Mipmap = 1,
	LT_HDR = 2,
	LT_Cubemap = 4,
	LT_Linear = 8,
};

HRESULT LoadTexture(
			const wchar_t * strFilename,
			ID3D11Device * pDevice,
			ID3D11DeviceContext * pDeviceContext,
			ID3D11ShaderResourceView ** ppSrv,
			int flags = LT_Mipmap);

HRESULT LoadTexture(
	const wchar_t * strFilename,
	ID3D11Device * pDevice,
	ID3D11ShaderResourceView ** ppSrv,
	int flags = 0);



// Camera class, based on DXUT camera but with Maya-style navigation
class CMayaStyleCamera : public CBaseCamera
{
public:
						CMayaStyleCamera();

	virtual void		FrameMove(FLOAT fElapsedTime) override;
	virtual void		SetProjParams(FLOAT fFOV, FLOAT fAspect, FLOAT fNearPlane, FLOAT fFarPlane) override;
	virtual void		SetViewParams(DirectX::FXMVECTOR pvEyePt, DirectX::FXMVECTOR pvLookatPt) override;

protected:
	float				m_fRadius;			// Distance of orbit
};

// First-person camera class with right-handed matrices
class CFirstPersonCameraRH : public CFirstPersonCamera
{
public:
	virtual void		FrameMove(FLOAT fElapsedTime) override;
	virtual void		SetProjParams(FLOAT fFOV, FLOAT fAspect, FLOAT fNearPlane, FLOAT fFarPlane) override;
};



// Very simple shadow map class, fits an orthogonal shadow map around a scene bounding box
class CShadowMap
{
public:
	ID3D11DepthStencilView *		m_pDsv;
	ID3D11ShaderResourceView *		m_pSrv;
	int								m_size;							// Shadow map resolution
	DirectX::XMFLOAT3				m_vecLight;						// Unit vector toward directional light
	DirectX::XMFLOAT3				m_posMinScene, m_posMaxScene;	// AABB of scene in world space
	DirectX::XMFLOAT4X4				m_matProj;						// Projection matrix
	DirectX::XMFLOAT4X4				m_matWorldToClip;				// Matrix for rendering shadow map
	DirectX::XMFLOAT4X4				m_matWorldToUvzw;				// Matrix for sampling shadow map
	DirectX::XMFLOAT4X4				m_matWorldToUvzNormal;			// Matrix for transforming normals to shadow map space
	DirectX::XMFLOAT3				m_vecDiam;						// Diameter in world units along shadow XYZ axes

	CShadowMap();

	HRESULT Init(unsigned int size, ID3D11Device * pDevice);
	void UpdateMatrix();
	void BindRenderTarget(ID3D11DeviceContext * pCtx);

	DirectX::XMFLOAT3 CalcFilterUVZScale(float filterRadius) const;

	void Release()
	{
		SAFE_RELEASE(m_pDsv);
		SAFE_RELEASE(m_pSrv);
	}
};



// Variance shadow map
class CVarShadowMap
{
public:
	ID3D11RenderTargetView *		m_pRtv;				// Main RT, stores (z, z^2)
	ID3D11ShaderResourceView *		m_pSrv;
	ID3D11RenderTargetView *		m_pRtvTemp;			// Temp RT for Gaussian blur
	ID3D11ShaderResourceView *		m_pSrvTemp;
	int								m_size;				// Shadow map resolution
	float							m_blurRadius;		// Radius of Gaussian in UV space

	CVarShadowMap();

	HRESULT Init(unsigned int size, ID3D11Device * pDevice);
	void UpdateFromShadowMap(const CShadowMap & shadow, ID3D11DeviceContext * pCtx);
	void GaussianBlur(ID3D11DeviceContext * pCtx);

	void Release()
	{
		SAFE_RELEASE(m_pRtv);
		SAFE_RELEASE(m_pSrv);
		SAFE_RELEASE(m_pRtvTemp);
		SAFE_RELEASE(m_pSrvTemp);
	}
};



// Perf measurement

// Enum of GPU timestamps to record
enum GTS
{
	GTS_BeginFrame,
	GTS_ShadowMap,
	GTS_Skin,
	GTS_Eyes,
	GTS_EndFrame,
	GTS_Max
};

class CGpuProfiler
{
public:
	CGpuProfiler();

	HRESULT Init(ID3D11Device * pDevice);
	void Release();

	void BeginFrame(ID3D11DeviceContext * pCtx);
	void Timestamp(ID3D11DeviceContext * pCtx, GTS gts);
	void EndFrame(ID3D11DeviceContext * pCtx);

	// Wait on GPU for last frame's data (not the current frame's) to be available
	void WaitForDataAndUpdate(ID3D11DeviceContext * pCtx);

	float Dt(GTS gts) const { return m_adT[gts]; }
	float GPUFrameTime() const { return m_gpuFrameTime; }
	float DtAvg(GTS gts) const { return m_adTAvg[gts]; }
	float GPUFrameTimeAvg() const { return m_gpuFrameTimeAvg; }

protected:
	int m_iFrameQuery;							// Which of the two sets of queries are we currently issuing?
	int m_iFrameCollect;						// Which of the two did we last collect?
	ID3D11Query * m_apQueryTsDisjoint[2];		// "Timestamp disjoint" query; records whether timestamps are valid
	ID3D11Query * m_apQueryTs[GTS_Max][2];		// Individual timestamp queries for each relevant point in the frame
	bool m_fTsUsed[GTS_Max][2];					// Flags recording which timestamps were actually used in a frame

	float m_adT[GTS_Max];						// Last frame's timings (each relative to previous GTS)
	float m_gpuFrameTime;
	float m_adTAvg[GTS_Max];					// Timings averaged over 0.5 second
	float m_gpuFrameTimeAvg;

	float m_adTTotal[GTS_Max];					// Total timings thus far within this averaging period
	float m_gpuFrameTimeTotal;
	int m_frameCount;							// Frames rendered in current averaging period
	DWORD m_tBeginAvg;							// Time (in ms) at which current averaging period started
};

extern CGpuProfiler g_gpuProfiler;
