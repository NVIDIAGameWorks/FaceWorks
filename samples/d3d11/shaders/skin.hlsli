//----------------------------------------------------------------------------------
// File:        FaceWorks/samples/sample_d3d11/shaders/skin.hlsli
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

#ifndef SKIN_HLSLI
#define SKIN_HLSLI

#include "common.hlsli"
#include "lighting.hlsli"
#include "GFSDK_FaceWorks.hlsli"

cbuffer cbShader : CB_SHADER
{
	float		g_normalStrength;
	float		g_gloss;

	GFSDK_FaceWorks_CBData	g_faceworksData;
}

Texture2D<float3> g_texDiffuse			: TEX_DIFFUSE0;
Texture2D<float3> g_texNormal			: TEX_NORMAL;
Texture2D<float> g_texSpec				: TEX_SPEC;
Texture2D<float3> g_texDeepScatterColor	: TEX_DEEP_SCATTER_COLOR;



void SkinMegashader(
	in Vertex i_vtx,
	in float3 i_vecCamera,
	in float4 i_uvzwShadow,
	out float3 o_rgbLit,
	uniform bool useSSS,
	uniform bool useDeepScatter)
{
	float2 uv = i_vtx.m_uv;

	// Sample textures

	float3 rgbDiffuse = g_texDiffuse.Sample(g_ssTrilinearRepeatAniso, uv);
	float3 normalTangent = UnpackNormal(g_texNormal.Sample(g_ssTrilinearRepeatAniso, uv),
										g_normalStrength);
	float specReflectance = g_texSpec.Sample(g_ssTrilinearRepeatAniso, uv);

	float3 normalTangentBlurred;
	if (useSSS || useDeepScatter)
	{
		// Sample normal map with level clamped based on blur, to get normal for SSS
		float level = GFSDK_FaceWorks_CalculateMipLevelForBlurredNormal(
						g_faceworksData, g_texNormal, g_ssTrilinearRepeatAniso, uv);
		normalTangentBlurred = UnpackNormal(
									g_texNormal.SampleLevel(g_ssTrilinearRepeatAniso, uv, level),
									g_normalStrength);
	}

	float3 rgbDeepScatter;
	if (useDeepScatter)
	{
		rgbDeepScatter = g_texDeepScatterColor.Sample(g_ssTrilinearRepeatAniso, uv);
	}

	LightingMegashader(
		i_vtx,
		i_vecCamera,
		i_uvzwShadow,
		rgbDiffuse,
		normalTangent,
		normalTangentBlurred,
		specReflectance,
		g_gloss,
		rgbDeepScatter,
		g_faceworksData,
		o_rgbLit,
		true,	// useNormalMap
		useSSS,
		useDeepScatter);
}

#endif // SKIN_HLSLI
