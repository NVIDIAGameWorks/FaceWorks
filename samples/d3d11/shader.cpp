//----------------------------------------------------------------------------------
// File:        FaceWorks/samples/sample_d3d11/shader.cpp
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


#include "shader.h"
#include "util.h"
#include "shaders/resources.h"

#include <GFSDK_FaceWorks.h>

// These headers are generated by the build process, and contain precompiled bytecode
// for all of the fixed shaders (not generated dynamically using feature flags)
#include "copy_ps.h"
#include "create_vsm_ps.h"
#include "curvature_vs.h"
#include "curvature_ps.h"
#include "gaussian_ps.h"
#include "hair_ps.h"
#include "screen_vs.h"
#include "shadow_vs.h"
#include "skybox_vs.h"
#include "skybox_ps.h"
#include "tess_vs.h"
#include "tess_hs.h"
#include "tess_ds.h"
#include "thickness_ps.h"
#include "world_vs.h"



// Constant buffer size must be a multiple of 16 bytes
inline unsigned int align16(unsigned int size)
{
	return ((size + 15) / 16) * 16;
}



// CShaderManager implementation

CShaderManager g_shdmgr;

CShaderManager::CShaderManager()
:	m_pPsCopy(nullptr),
	m_pPsCreateVSM(nullptr),
	m_pVsCurvature(nullptr),
	m_pPsCurvature(nullptr),
	m_pPsThickness(nullptr),
	m_pPsGaussian(nullptr),
	m_pPsHair(nullptr),
	m_pVsScreen(nullptr),
	m_pVsShadow(nullptr),
	m_pVsSkybox(nullptr),
	m_pPsSkybox(nullptr),
	m_pVsTess(nullptr),
	m_pHsTess(nullptr),
	m_pDsTess(nullptr),
	m_pVsWorld(nullptr),
	m_pSsPointClamp(nullptr),
	m_pSsBilinearClamp(nullptr),
	m_pSsTrilinearRepeat(nullptr),
	m_pSsTrilinearRepeatAniso(nullptr),
	m_pSsPCF(nullptr),
	m_pInputLayout(nullptr),
	m_pCbufDebug(nullptr),
	m_pCbufFrame(nullptr),
	m_pCbufShader(nullptr),
	m_mapSkinFeaturesToShader(),
	m_mapEyeFeaturesToShader()
{
}

HRESULT CShaderManager::Init(ID3D11Device * pDevice)
{
	HRESULT hr;

	// Initialize all the fixed shaders from bytecode
	V_RETURN(pDevice->CreatePixelShader(copy_ps_bytecode, dim(copy_ps_bytecode), nullptr, &m_pPsCopy));
	V_RETURN(pDevice->CreatePixelShader(create_vsm_ps_bytecode, dim(create_vsm_ps_bytecode), nullptr, &m_pPsCreateVSM));
	V_RETURN(pDevice->CreateVertexShader(curvature_vs_bytecode, dim(curvature_vs_bytecode), nullptr, &m_pVsCurvature));
	V_RETURN(pDevice->CreatePixelShader(curvature_ps_bytecode, dim(curvature_ps_bytecode), nullptr, &m_pPsCurvature));
	V_RETURN(pDevice->CreatePixelShader(gaussian_ps_bytecode, dim(gaussian_ps_bytecode), nullptr, &m_pPsGaussian));
	V_RETURN(pDevice->CreatePixelShader(hair_ps_bytecode, dim(hair_ps_bytecode), nullptr, &m_pPsHair));
	V_RETURN(pDevice->CreateVertexShader(screen_vs_bytecode, dim(screen_vs_bytecode), nullptr, &m_pVsScreen));
	V_RETURN(pDevice->CreateVertexShader(shadow_vs_bytecode, dim(shadow_vs_bytecode), nullptr, &m_pVsShadow));
	V_RETURN(pDevice->CreateVertexShader(skybox_vs_bytecode, dim(skybox_vs_bytecode), nullptr, &m_pVsSkybox));
	V_RETURN(pDevice->CreatePixelShader(skybox_ps_bytecode, dim(skybox_ps_bytecode), nullptr, &m_pPsSkybox));
	V_RETURN(pDevice->CreateVertexShader(tess_vs_bytecode, dim(tess_vs_bytecode), nullptr, &m_pVsTess));
	V_RETURN(pDevice->CreateHullShader(tess_hs_bytecode, dim(tess_hs_bytecode), nullptr, &m_pHsTess));
	V_RETURN(pDevice->CreateDomainShader(tess_ds_bytecode, dim(tess_ds_bytecode), nullptr, &m_pDsTess));
	V_RETURN(pDevice->CreatePixelShader(thickness_ps_bytecode, dim(thickness_ps_bytecode), nullptr, &m_pPsThickness));
	V_RETURN(pDevice->CreateVertexShader(world_vs_bytecode, dim(world_vs_bytecode), nullptr, &m_pVsWorld));

	// Initialize the sampler states

	D3D11_SAMPLER_DESC sampDesc =
	{
		D3D11_FILTER_MIN_MAG_MIP_POINT,
		D3D11_TEXTURE_ADDRESS_CLAMP,
		D3D11_TEXTURE_ADDRESS_CLAMP,
		D3D11_TEXTURE_ADDRESS_CLAMP,
		0.0f,
		1,
		D3D11_COMPARISON_FUNC(0),
		{ 0.0f, 0.0f, 0.0f, 0.0f },
		0.0f,
		FLT_MAX,
	};
	V_RETURN(pDevice->CreateSamplerState(&sampDesc, &m_pSsPointClamp));

	sampDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	V_RETURN(pDevice->CreateSamplerState(&sampDesc, &m_pSsBilinearClamp));
	
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	V_RETURN(pDevice->CreateSamplerState(&sampDesc, &m_pSsTrilinearRepeat));
	
	sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	sampDesc.MaxAnisotropy = 16;
	V_RETURN(pDevice->CreateSamplerState(&sampDesc, &m_pSsTrilinearRepeatAniso));

	sampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.MaxAnisotropy = 1;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	sampDesc.BorderColor[0] = 1.0f;
	sampDesc.BorderColor[1] = 1.0f;
	sampDesc.BorderColor[2] = 1.0f;
	sampDesc.BorderColor[3] = 1.0f;
	V_RETURN(pDevice->CreateSamplerState(&sampDesc, &m_pSsPCF));

	// Initialize the input layout, and validate it against all the vertex shaders

	D3D11_INPUT_ELEMENT_DESC aInputDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, UINT(offsetof(Vertex, m_pos)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, UINT(offsetof(Vertex, m_normal)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, UINT(offsetof(Vertex, m_uv)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, UINT(offsetof(Vertex, m_tangent)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "CURVATURE", 0, DXGI_FORMAT_R32_FLOAT, 0, UINT(offsetof(Vertex, m_curvature)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	V_RETURN(pDevice->CreateInputLayout(
						aInputDescs, dim(aInputDescs),
						curvature_vs_bytecode, dim(curvature_vs_bytecode),
						&m_pInputLayout));

	// Note: calling CreateInputLayout with nullptr for the 5th parameter will crash
	// the VS2012 graphics debugger.  Turn on this define if you need to graphics-debug.
	// Bug filed with MS at https://connect.microsoft.com/VisualStudio/feedback/details/790030/
	// Supposed to be fixed in VS2013.
#define VS_GRAPHICS_DEBUG 0
#if !VS_GRAPHICS_DEBUG
	V_RETURN(pDevice->CreateInputLayout(
						aInputDescs, dim(aInputDescs),
						screen_vs_bytecode, dim(screen_vs_bytecode),
						nullptr));
	V_RETURN(pDevice->CreateInputLayout(
						aInputDescs, dim(aInputDescs),
						shadow_vs_bytecode, dim(shadow_vs_bytecode),
						nullptr));
	V_RETURN(pDevice->CreateInputLayout(
						aInputDescs, dim(aInputDescs),
						skybox_vs_bytecode, dim(skybox_vs_bytecode),
						nullptr));
	V_RETURN(pDevice->CreateInputLayout(
						aInputDescs, dim(aInputDescs),
						tess_vs_bytecode, dim(tess_vs_bytecode),
						nullptr));
	V_RETURN(pDevice->CreateInputLayout(
						aInputDescs, dim(aInputDescs),
						world_vs_bytecode, dim(world_vs_bytecode),
						nullptr));
#endif // !VS_GRAPHICS_DEBUG

	// Initialize the constant buffers

	D3D11_BUFFER_DESC bufDesc =
	{
		align16(sizeof(CbufDebug)),
		D3D11_USAGE_DYNAMIC,
		D3D11_BIND_CONSTANT_BUFFER,
		D3D11_CPU_ACCESS_WRITE,
	};

	V_RETURN(pDevice->CreateBuffer(&bufDesc, nullptr, &m_pCbufDebug));

	bufDesc.ByteWidth = align16(sizeof(CbufFrame));
	V_RETURN(pDevice->CreateBuffer(&bufDesc, nullptr, &m_pCbufFrame));

	bufDesc.ByteWidth = align16(sizeof_field(Material, m_constants));
	V_RETURN(pDevice->CreateBuffer(&bufDesc, nullptr, &m_pCbufShader));

	return S_OK;
}

void CShaderManager::InitFrame(
	ID3D11DeviceContext * pCtx,
	const CbufDebug * pCbufDebug,
	const CbufFrame * pCbufFrame,
	ID3D11ShaderResourceView * pSrvCubeDiffuse,
	ID3D11ShaderResourceView * pSrvCubeSpec,
	ID3D11ShaderResourceView * pSrvCurvatureLUT,
	ID3D11ShaderResourceView * pSrvShadowLUT)
{
	HRESULT hr;

	// Set all the samplers
	pCtx->PSSetSamplers(SAMP_POINT_CLAMP, 1, &m_pSsPointClamp);
	pCtx->PSSetSamplers(SAMP_BILINEAR_CLAMP, 1, &m_pSsBilinearClamp);
	pCtx->PSSetSamplers(SAMP_TRILINEAR_REPEAT, 1, &m_pSsTrilinearRepeat);
	pCtx->PSSetSamplers(SAMP_TRILINEAR_REPEAT_ANISO, 1, &m_pSsTrilinearRepeatAniso);
	pCtx->PSSetSamplers(SAMP_PCF, 1, &m_pSsPCF);

	// Set the input assembler state
	pCtx->IASetInputLayout(m_pInputLayout);
	pCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Update the constant buffers

	D3D11_MAPPED_SUBRESOURCE map = {};
	V(pCtx->Map(m_pCbufDebug, 0, D3D11_MAP_WRITE_DISCARD, 0, &map));
	memcpy(map.pData, pCbufDebug, sizeof(CbufDebug));
	pCtx->Unmap(m_pCbufDebug, 0);

	V(pCtx->Map(m_pCbufFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &map));
	memcpy(map.pData, pCbufFrame, sizeof(CbufFrame));
	pCtx->Unmap(m_pCbufFrame, 0);

	// Set the constant buffers to all shader stages
	pCtx->VSSetConstantBuffers(CB_DEBUG, 1, &m_pCbufDebug);
	pCtx->HSSetConstantBuffers(CB_DEBUG, 1, &m_pCbufDebug);
	pCtx->DSSetConstantBuffers(CB_DEBUG, 1, &m_pCbufDebug);
	pCtx->PSSetConstantBuffers(CB_DEBUG, 1, &m_pCbufDebug);
	pCtx->VSSetConstantBuffers(CB_FRAME, 1, &m_pCbufFrame);
	pCtx->HSSetConstantBuffers(CB_FRAME, 1, &m_pCbufFrame);
	pCtx->DSSetConstantBuffers(CB_FRAME, 1, &m_pCbufFrame);
	pCtx->PSSetConstantBuffers(CB_FRAME, 1, &m_pCbufFrame);
	pCtx->VSSetConstantBuffers(CB_SHADER, 1, &m_pCbufShader);
	pCtx->HSSetConstantBuffers(CB_SHADER, 1, &m_pCbufShader);
	pCtx->DSSetConstantBuffers(CB_SHADER, 1, &m_pCbufShader);
	pCtx->PSSetConstantBuffers(CB_SHADER, 1, &m_pCbufShader);

	// Set the textures that are kept for the whole frame
	pCtx->PSSetShaderResources(TEX_CUBE_DIFFUSE, 1, &pSrvCubeDiffuse);
	pCtx->PSSetShaderResources(TEX_CUBE_SPEC, 1, &pSrvCubeSpec);
	pCtx->PSSetShaderResources(TEX_CURVATURE_LUT, 1, &pSrvCurvatureLUT);
	pCtx->PSSetShaderResources(TEX_SHADOW_LUT, 1, &pSrvShadowLUT);
}

void CShaderManager::BindShadowTextures(
	ID3D11DeviceContext * pCtx,
	ID3D11ShaderResourceView * pSrvShadowMap,
	ID3D11ShaderResourceView * pSrvVSM)
{
	pCtx->PSSetShaderResources(TEX_SHADOW_MAP, 1, &pSrvShadowMap);
	pCtx->PSSetShaderResources(TEX_VSM, 1, &pSrvVSM);
}



// Include helper class for D3DCompile API
struct IncludeHelper : public ID3DInclude
{
	virtual ~IncludeHelper()
	{
	}

	std::wstring						m_dirsToTry[2];
	std::vector<std::vector<char> >		m_openFiles;

	IncludeHelper()
	{
		// Get the path to the .exe is located
		HMODULE hModule = GetModuleHandleW(nullptr);
		wchar_t path[MAX_PATH];
		GetModuleFileNameW(hModule, path, MAX_PATH);

		// Trim off the .exe filename, leaving just the directory
		wchar_t * pBasename = wcsrchr(path, L'\\');
		if (pBasename)
			++pBasename;
		else
			pBasename = path;
		*pBasename = 0;

		// Assemble paths to try for includes, relative to the .exe directory
		m_dirsToTry[0] = path;
		m_dirsToTry[0] += L"..\\..\\d3d11\\shaders\\";
		m_dirsToTry[1] = path;
		m_dirsToTry[1] += L"..\\..\\..\\include\\";
	}

	virtual HRESULT __stdcall Open(
								D3D_INCLUDE_TYPE /*IncludeType*/,
								LPCSTR pFileName,
								LPCVOID /*pParentData*/,
								LPCVOID * ppDataOut,
								UINT * pBytesOut) override
	{
		assert(pFileName);
		assert(ppDataOut);
		assert(pBytesOut);

		m_openFiles.push_back(std::vector<char>());

		// Try to find the include in each possible directory
		for (int i = 0; i < dim(m_dirsToTry); ++i)
		{
			wchar_t path[MAX_PATH];
			swprintf_s(path, L"%s%hs", m_dirsToTry[i].c_str(), pFileName);
			if (SUCCEEDED(LoadFile(path, &m_openFiles.back())))
			{
				*ppDataOut = &m_openFiles.back()[0];
				*pBytesOut = UINT(m_openFiles.back().size());
				DebugPrintf(L"Loaded include file: %hs\n", pFileName);
				return S_OK;
			}
		}

		m_openFiles.pop_back();

		return E_FAIL;
	}

	virtual HRESULT __stdcall Close(LPCVOID pData) override
	{
		assert(pData);

		for (int i = 0, n = int(m_openFiles.size()); i < n; ++i)
		{
			if (!m_openFiles[i].empty() &&
				pData == &m_openFiles[i][0])
			{
				m_openFiles[i].clear();
				return S_OK;
			}
		}

		assert(false);
		return E_FAIL;
	}
};

bool CShaderManager::CompileShader(
	ID3D11Device * pDevice,
	const char * source,
	ID3D11PixelShader ** ppPsOut)
{
	// Compile the source

	UINT flags = 0;
#if defined(_DEBUG)
	flags |= D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_DEBUG | D3DCOMPILE_PREFER_FLOW_CONTROL;
#endif

	ID3DBlob * pBlobCode = nullptr;
	ID3DBlob * pBlobErrors = nullptr;

	IncludeHelper includeHelper;
	HRESULT hr = D3DCompile(
					source, strlen(source), "generated_ps",
					nullptr, &includeHelper,
					"main", "ps_5_0",
					flags, 0,
					&pBlobCode, &pBlobErrors);
	if (pBlobErrors)
	{
		MessageBoxA(
			DXUTGetHWND(),
			static_cast<char *>(pBlobErrors->GetBufferPointer()),
			"Shader Compile Error", MB_OK);
		if (pBlobCode)
			pBlobCode->Release();
		pBlobErrors->Release();
		return false;
	}
	if (FAILED(hr) || !pBlobCode)
	{
		MessageBox(
			DXUTGetHWND(),
			L"Shader failed to compile, but no error messages were generated.",
			L"Shader Compile Error", MB_OK);
		if (pBlobCode)
			pBlobCode->Release();
		return false;
	}

	// Create the shader on the device
	if (FAILED(pDevice->CreatePixelShader(
							pBlobCode->GetBufferPointer(), pBlobCode->GetBufferSize(),
							nullptr, ppPsOut)))
	{
		MessageBox(
			DXUTGetHWND(),
			L"Couldn't create pixel shader on device.",
			L"Shader Compile Error", MB_OK);
		pBlobCode->Release();
		return false;
	}

	pBlobCode->Release();
	return true;
}

void CShaderManager::CreateSkinShader(ID3D11Device * pDevice, SHDFEATURES features)
{
	DebugPrintf(L"Generating skin shader with feature mask %d\n", features);

	assert((features & SHDFEAT_PSMask) == features);

	// Create source code for the shader
	char source[512];
	sprintf_s(
		source,
		"#include \"skin.hlsli\"\n"
		"void main(\n"
		"	in Vertex i_vtx,\n"
		"	in float3 i_vecCamera : CAMERA,\n"
		"	in float4 i_uvzwShadow : UVZW_SHADOW,\n"
		"	out float3 o_rgbLit : SV_Target)\n"
		"{\n"
		"	SkinMegashader(i_vtx, i_vecCamera, i_uvzwShadow, o_rgbLit, %s, %s);\n"
		"}\n",
		(features & SHDFEAT_SSS) ? "true" : "false",
		(features & SHDFEAT_DeepScatter) ? "true" : "false"
	);

	// Compile it
	ID3D11PixelShader * pPs = nullptr;
	if (!CompileShader(pDevice, source, &pPs))
		return;

	// Store it in the map for future reference
	assert(pPs);
	m_mapSkinFeaturesToShader[features] = pPs;
}

void CShaderManager::CreateEyeShader(ID3D11Device * pDevice, SHDFEATURES features)
{
	DebugPrintf(L"Generating eye shader with feature mask %d\n", features);

	assert((features & SHDFEAT_PSMask) == features);

	// Create source code for the shader
	char source[512];
	sprintf_s(
		source,
		"#include \"eye.hlsli\"\n"
		"void main(\n"
		"	in Vertex i_vtx,\n"
		"	in float3 i_vecCamera : CAMERA,\n"
		"	in float4 i_uvzwShadow : UVZW_SHADOW,\n"
		"	out float3 o_rgbLit : SV_Target)\n"
		"{\n"
		"	EyeMegashader(i_vtx, i_vecCamera, i_uvzwShadow, o_rgbLit, %s, %s);\n"
		"}\n",
		(features & SHDFEAT_SSS) ? "true" : "false",
		(features & SHDFEAT_DeepScatter) ? "true" : "false"
	);

	// Compile it
	ID3D11PixelShader * pPs = nullptr;
	if (!CompileShader(pDevice, source, &pPs))
		return;

	// Store it in the map for future reference
	assert(pPs);
	m_mapEyeFeaturesToShader[features] = pPs;
}

ID3D11PixelShader * CShaderManager::GetSkinShader(ID3D11Device * pDevice, SHDFEATURES features)
{
	features &= SHDFEAT_PSMask;

	std::unordered_map<SHDFEATURES, ID3D11PixelShader *>::iterator i =
		m_mapSkinFeaturesToShader.find(features);

	if (i != m_mapSkinFeaturesToShader.end())
	{
		return i->second;
	}

	CreateSkinShader(pDevice, features);
	return m_mapSkinFeaturesToShader[features];
}

ID3D11PixelShader * CShaderManager::GetEyeShader(ID3D11Device * pDevice, SHDFEATURES features)
{
	features &= SHDFEAT_PSMask;

	std::unordered_map<SHDFEATURES, ID3D11PixelShader *>::iterator i =
		m_mapEyeFeaturesToShader.find(features);

	if (i != m_mapEyeFeaturesToShader.end())
	{
		return i->second;
	}

	CreateEyeShader(pDevice, features);
	return m_mapEyeFeaturesToShader[features];
}



void CShaderManager::BindCopy(
	ID3D11DeviceContext * pCtx,
	ID3D11ShaderResourceView * pSrvSrc,
	const DirectX::XMFLOAT4X4 & matTransformColor)
{
	pCtx->VSSetShader(m_pVsScreen, nullptr, 0);
	pCtx->PSSetShader(m_pPsCopy, nullptr, 0);
	pCtx->PSSetShaderResources(TEX_SOURCE, 1, &pSrvSrc);

	// Update constant buffer
	HRESULT hr;
	D3D11_MAPPED_SUBRESOURCE map = {};
	V(pCtx->Map(m_pCbufShader, 0, D3D11_MAP_WRITE_DISCARD, 0, &map));
	memcpy(map.pData, &matTransformColor, sizeof(matTransformColor));
	pCtx->Unmap(m_pCbufShader, 0);
}

void CShaderManager::BindCreateVSM(
	ID3D11DeviceContext * pCtx,
	ID3D11ShaderResourceView * pSrvSrc)
{
	pCtx->VSSetShader(m_pVsScreen, nullptr, 0);
	pCtx->PSSetShader(m_pPsCreateVSM, nullptr, 0);
	pCtx->PSSetShaderResources(TEX_SOURCE, 1, &pSrvSrc);
}

void CShaderManager::BindCurvature(
	ID3D11DeviceContext * pCtx,
	float curvatureScale,
	float curvatureBias)
{
	pCtx->VSSetShader(m_pVsCurvature, nullptr, 0);
	pCtx->PSSetShader(m_pPsCurvature, nullptr, 0);

	// Update constant buffer
	HRESULT hr;
	D3D11_MAPPED_SUBRESOURCE map = {};
	V(pCtx->Map(m_pCbufShader, 0, D3D11_MAP_WRITE_DISCARD, 0, &map));
	static_cast<float *>(map.pData)[0] = curvatureScale;
	static_cast<float *>(map.pData)[1] = curvatureBias;
	pCtx->Unmap(m_pCbufShader, 0);
}

void CShaderManager::BindThickness(
	ID3D11DeviceContext * pCtx,
	GFSDK_FaceWorks_CBData * pFaceWorksCBData)
{
	pCtx->VSSetShader(m_pVsWorld, nullptr, 0);
	pCtx->PSSetShader(m_pPsThickness, nullptr, 0);

	// Update constant buffer
	HRESULT hr;
	D3D11_MAPPED_SUBRESOURCE map = {};
	V(pCtx->Map(m_pCbufShader, 0, D3D11_MAP_WRITE_DISCARD, 0, &map));
	memcpy(map.pData, pFaceWorksCBData, sizeof(GFSDK_FaceWorks_CBData));
	pCtx->Unmap(m_pCbufShader, 0);
}

void CShaderManager::BindGaussian(
	ID3D11DeviceContext * pCtx,
	ID3D11ShaderResourceView * pSrvSrc,
	float blurX,
	float blurY)
{
	pCtx->VSSetShader(m_pVsScreen, nullptr, 0);
	pCtx->PSSetShader(m_pPsGaussian, nullptr, 0);
	pCtx->PSSetShaderResources(TEX_SOURCE, 1, &pSrvSrc);

	// Update constant buffer
	HRESULT hr;
	D3D11_MAPPED_SUBRESOURCE map = {};
	V(pCtx->Map(m_pCbufShader, 0, D3D11_MAP_WRITE_DISCARD, 0, &map));
	static_cast<float *>(map.pData)[0] = blurX;
	static_cast<float *>(map.pData)[1] = blurY;
	pCtx->Unmap(m_pCbufShader, 0);
}

void CShaderManager::BindShadow(
	ID3D11DeviceContext * pCtx,
	const DirectX::XMFLOAT4X4 & matWorldToClipShadow)
{
	pCtx->VSSetShader(m_pVsShadow, nullptr, 0);
	pCtx->PSSetShader(nullptr, nullptr, 0);

	// Update constant buffer
	HRESULT hr;
	D3D11_MAPPED_SUBRESOURCE map = {};
	V(pCtx->Map(m_pCbufShader, 0, D3D11_MAP_WRITE_DISCARD, 0, &map));
	memcpy(map.pData, &matWorldToClipShadow, sizeof(matWorldToClipShadow));
	pCtx->Unmap(m_pCbufShader, 0);
}

void CShaderManager::BindSkybox(
	ID3D11DeviceContext * pCtx,
	ID3D11ShaderResourceView * pSrvSkybox,
	const DirectX::XMFLOAT4X4 & matClipToWorldAxes)
{
	pCtx->VSSetShader(m_pVsSkybox, nullptr, 0);
	pCtx->PSSetShader(m_pPsSkybox, nullptr, 0);
	pCtx->PSSetShaderResources(TEX_SOURCE, 1, &pSrvSkybox);

	// Update constant buffer
	HRESULT hr;
	D3D11_MAPPED_SUBRESOURCE map = {};
	V(pCtx->Map(m_pCbufShader, 0, D3D11_MAP_WRITE_DISCARD, 0, &map));
	memcpy(map.pData, &matClipToWorldAxes, sizeof(matClipToWorldAxes));
	pCtx->Unmap(m_pCbufShader, 0);
}

void CShaderManager::BindMaterial(
	ID3D11DeviceContext * pCtx,
	SHDFEATURES features,
	const Material * pMtl)
{
	ID3D11Device * pDevice;
	pCtx->GetDevice(&pDevice);

	// Determine which pixel shader to use
	ID3D11PixelShader * pPs;
	switch (pMtl->m_shader)
	{
	case SHADER_Skin:	pPs = GetSkinShader(pDevice, features); break;
	case SHADER_Eye:	pPs = GetEyeShader(pDevice, features); break;
	case SHADER_Hair:	pPs = m_pPsHair; break;
	default:
		assert(false);
		return;
	}
	
	pDevice->Release();
	assert(pPs);

	pCtx->PSSetShader(pPs, nullptr, 0);

	// Determine which vertex/tess shaders to use
	if (features & SHDFEAT_Tessellation)
	{
		pCtx->VSSetShader(m_pVsTess, nullptr, 0);
		pCtx->HSSetShader(m_pHsTess, nullptr, 0);
		pCtx->DSSetShader(m_pDsTess, nullptr, 0);
		pCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
	}
	else
	{
		pCtx->VSSetShader(m_pVsWorld, nullptr, 0);
	}

	// Bind textures
	for (int i = 0; i < dim(pMtl->m_aSrv); ++i)
	{
		if (pMtl->m_aSrv[i])
			pCtx->PSSetShaderResources(pMtl->m_textureSlots[i], 1, &pMtl->m_aSrv[i]);
	}

	// Update constant buffer
	HRESULT hr;
	D3D11_MAPPED_SUBRESOURCE map = {};
	V(pCtx->Map(m_pCbufShader, 0, D3D11_MAP_WRITE_DISCARD, 0, &map));
	memcpy(map.pData, pMtl->m_constants, sizeof(pMtl->m_constants));
	pCtx->Unmap(m_pCbufShader, 0);
}

void CShaderManager::UnbindTess(ID3D11DeviceContext * pCtx)
{
	pCtx->HSSetShader(nullptr, nullptr, 0);
	pCtx->DSSetShader(nullptr, nullptr, 0);
	pCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}


void CShaderManager::DiscardRuntimeCompiledShaders()
{
	for (std::unordered_map<SHDFEATURES, ID3D11PixelShader *>::iterator
			i = m_mapSkinFeaturesToShader.begin(), end = m_mapSkinFeaturesToShader.end();
		 i != end;
		 ++i)
	{
		if (i->second)
			i->second->Release();
	}
	m_mapSkinFeaturesToShader.clear();

	for (std::unordered_map<SHDFEATURES, ID3D11PixelShader *>::iterator
			i = m_mapEyeFeaturesToShader.begin(), end = m_mapEyeFeaturesToShader.end();
		 i != end;
		 ++i)
	{
		if (i->second)
			i->second->Release();
	}
	m_mapEyeFeaturesToShader.clear();
}

void CShaderManager::Release()
{
	if (m_pPsCopy)
	{
		m_pPsCopy->Release();
		m_pPsCopy = nullptr;
	}
	if (m_pPsCreateVSM)
	{
		m_pPsCreateVSM->Release();
		m_pPsCreateVSM = nullptr;
	}
	if (m_pVsCurvature)
	{
		m_pVsCurvature->Release();
		m_pVsCurvature = nullptr;
	}
	if (m_pPsCurvature)
	{
		m_pPsCurvature->Release();
		m_pPsCurvature = nullptr;
	}
	if (m_pPsThickness)
	{
		m_pPsThickness->Release();
		m_pPsThickness = nullptr;
	}
	if (m_pPsGaussian)
	{
		m_pPsGaussian->Release();
		m_pPsGaussian = nullptr;
	}
	if (m_pPsHair)
	{
		m_pPsHair->Release();
		m_pPsHair = nullptr;
	}
	if (m_pVsScreen)
	{
		m_pVsScreen->Release();
		m_pVsScreen = nullptr;
	}
	if (m_pVsShadow)
	{
		m_pVsShadow->Release();
		m_pVsShadow = nullptr;
	}
	if (m_pVsSkybox)
	{
		m_pVsSkybox->Release();
		m_pVsSkybox = nullptr;
	}
	if (m_pPsSkybox)
	{
		m_pPsSkybox->Release();
		m_pPsSkybox = nullptr;
	}
	if (m_pVsTess)
	{
		m_pVsTess->Release();
		m_pVsTess = nullptr;
	}
	if (m_pHsTess)
	{
		m_pHsTess->Release();
		m_pHsTess = nullptr;
	}
	if (m_pDsTess)
	{
		m_pDsTess->Release();
		m_pDsTess = nullptr;
	}
	if (m_pVsWorld)
	{
		m_pVsWorld->Release();
		m_pVsWorld = nullptr;
	}

	if (m_pSsPointClamp)
	{
		m_pSsPointClamp->Release();
		m_pSsPointClamp = nullptr;
	}
	if (m_pSsBilinearClamp)
	{
		m_pSsBilinearClamp->Release();
		m_pSsBilinearClamp = nullptr;
	}
	if (m_pSsTrilinearRepeat)
	{
		m_pSsTrilinearRepeat->Release();
		m_pSsTrilinearRepeat = nullptr;
	}
	if (m_pSsTrilinearRepeatAniso)
	{
		m_pSsTrilinearRepeatAniso->Release();
		m_pSsTrilinearRepeatAniso = nullptr;
	}
	if (m_pSsPCF)
	{
		m_pSsPCF->Release();
		m_pSsPCF = nullptr;
	}

	if (m_pInputLayout)
	{
		m_pInputLayout->Release();
		m_pInputLayout = nullptr;
	}

	if (m_pCbufDebug)
	{
		m_pCbufDebug->Release();
		m_pCbufDebug = nullptr;
	}
	if (m_pCbufFrame)
	{
		m_pCbufFrame->Release();
		m_pCbufFrame = nullptr;
	}
	if (m_pCbufShader)
	{
		m_pCbufShader->Release();
		m_pCbufShader = nullptr;
	}

	DiscardRuntimeCompiledShaders();
}
