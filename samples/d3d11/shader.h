//----------------------------------------------------------------------------------
// File:        FaceWorks/samples/sample_d3d11/shader.h
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

#include "GFSDK_FaceWorks.h"
#include <unordered_map>
#include <d3d11.h>
#include <DirectXMath.h>




// Shader feature flags
enum SHDFEAT
{
	SHDFEAT_None			= 0x00,
	SHDFEAT_Tessellation	= 0x01,
	SHDFEAT_SSS				= 0x02,
	SHDFEAT_DeepScatter		= 0x04,

	SHDFEAT_PSMask			= 0x06,				// Features that affect the pixel shader
};
typedef int SHDFEATURES;



// Constant buffers

struct CbufDebug	// matches cbuffer cbDebug in common.hlsli
{
	float			m_debug;			// Mapped to spacebar - 0 if up, 1 if down
	float			m_debugSlider0;		// Mapped to debug slider in UI
	float			m_debugSlider1;		// ...
	float			m_debugSlider2;		// ...
	float			m_debugSlider3;		// ...
};

struct CbufFrame	// matches cbuffer cbFrame in lighting.hlsli
{
	DirectX::XMFLOAT4X4	m_matWorldToClip;
	DirectX::XMFLOAT4	m_posCamera;

	DirectX::XMFLOAT4	m_vecDirectionalLight;
	DirectX::XMFLOAT4	m_rgbDirectionalLight;

	DirectX::XMFLOAT4X4	m_matWorldToUvzwShadow;
	DirectX::XMFLOAT4	m_matWorldToUvzShadowNormal[3];	// Matrix for transforming normals to shadow map space

	float				m_vsmMinVariance;			// Minimum variance for variance shadow maps
	float				m_shadowSharpening;
	float				m_tessScale;				// Scale of adaptive tessellation

	float				m_deepScatterIntensity;		// Multiplier on whole deep scattering result
	float				m_deepScatterFalloff;		// Reciprocal of one-sigma radius of deep scatter Gaussian, in cm
	float				m_deepScatterNormalOffset;	// Normal offset for shadow lookup to calculate thickness

	float				m_exposure;					// Exposure multiplier
};



// Material model - textures and constants for a particular material

enum SHADER
{
	SHADER_Skin,
	SHADER_Eye,
	SHADER_Hair,
};

struct Material
{
	SHADER			m_shader;					// Which shader is this?

	ID3D11ShaderResourceView *
					m_aSrv[4];					// Textures
	int				m_textureSlots[4];			// Slots where to bind the textures
	float			m_constants[24];			// Data for CB_SHADER constant buffer - includes
												//   FaceWorks constant buffer data as well
};



// Shader data and loading

class CShaderManager
{
public:
	ID3D11PixelShader *		m_pPsCopy;
	ID3D11PixelShader *		m_pPsCreateVSM;
	ID3D11VertexShader *	m_pVsCurvature;
	ID3D11PixelShader *		m_pPsCurvature;
	ID3D11PixelShader *		m_pPsThickness;
	ID3D11PixelShader *		m_pPsGaussian;
	ID3D11PixelShader *		m_pPsHair;
	ID3D11VertexShader *	m_pVsScreen;
	ID3D11VertexShader *	m_pVsShadow;
	ID3D11VertexShader *	m_pVsSkybox;
	ID3D11PixelShader *		m_pPsSkybox;
	ID3D11VertexShader *	m_pVsTess;
	ID3D11HullShader *		m_pHsTess;
	ID3D11DomainShader *	m_pDsTess;
	ID3D11VertexShader *	m_pVsWorld;

	ID3D11SamplerState *	m_pSsPointClamp;
	ID3D11SamplerState *	m_pSsBilinearClamp;
	ID3D11SamplerState *	m_pSsTrilinearRepeat;
	ID3D11SamplerState *	m_pSsTrilinearRepeatAniso;
	ID3D11SamplerState *	m_pSsPCF;

	ID3D11InputLayout *		m_pInputLayout;

	ID3D11Buffer *			m_pCbufDebug;
	ID3D11Buffer *			m_pCbufFrame;
	ID3D11Buffer *			m_pCbufShader;

	CShaderManager();

	HRESULT Init(ID3D11Device * pDevice);

	void InitFrame(
			ID3D11DeviceContext * pCtx,
			const CbufDebug * pCbufDebug,
			const CbufFrame * pCbufFrame,
			ID3D11ShaderResourceView * pSrvCubeDiffuse,
			ID3D11ShaderResourceView * pSrvCubeSpec,
			ID3D11ShaderResourceView * pSrvCurvatureLUT,
			ID3D11ShaderResourceView * pSrvShadowLUT);

	void BindShadowTextures(
			ID3D11DeviceContext * pCtx,
			ID3D11ShaderResourceView * pSrvShadowMap,
			ID3D11ShaderResourceView * pSrvVSM);

	ID3D11PixelShader *			GetSkinShader(ID3D11Device * pDevice, SHDFEATURES features);
	ID3D11PixelShader *			GetEyeShader(ID3D11Device * pDevice, SHDFEATURES features);

	void BindCopy(
			ID3D11DeviceContext * pCtx,
			ID3D11ShaderResourceView * pSrvSrc,
			const DirectX::XMFLOAT4X4 & matTransformColor);
	void BindCreateVSM(
			ID3D11DeviceContext * pCtx,
			ID3D11ShaderResourceView * pSrvSrc);
	void BindCurvature(
			ID3D11DeviceContext * pCtx,
			float curvatureScale,
			float curvatureBias);
	void BindThickness(
			ID3D11DeviceContext * pCtx,
			GFSDK_FaceWorks_CBData * pFaceWorksCBData);
	void BindGaussian(
			ID3D11DeviceContext * pCtx,
			ID3D11ShaderResourceView * pSrvSrc,
			float blurX,
			float blurY);
	void BindShadow(
			ID3D11DeviceContext * pCtx,
			const DirectX::XMFLOAT4X4 & matWorldToClipShadow);
	void BindSkybox(
			ID3D11DeviceContext * pCtx,
			ID3D11ShaderResourceView * pSrvSkybox,
			const DirectX::XMFLOAT4X4 & matClipToWorldAxes);

	void BindMaterial(
			ID3D11DeviceContext * pCtx,
			SHDFEATURES features,
			const Material * pMtl);
	void UnbindTess(ID3D11DeviceContext * pCtx);

	void DiscardRuntimeCompiledShaders();
	void Release();

private:

	bool CompileShader(
			ID3D11Device * pDevice,
			const char * source,
			ID3D11PixelShader ** ppPsOut);
	void CreateSkinShader(ID3D11Device * pDevice, SHDFEATURES features);
	void CreateEyeShader(ID3D11Device * pDevice, SHDFEATURES features);

	std::unordered_map<SHDFEATURES, ID3D11PixelShader *>		m_mapSkinFeaturesToShader;
	std::unordered_map<SHDFEATURES, ID3D11PixelShader *>		m_mapEyeFeaturesToShader;
};

extern CShaderManager g_shdmgr;
