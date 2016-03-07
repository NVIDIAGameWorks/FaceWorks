//----------------------------------------------------------------------------------
// File:        FaceWorks/samples/sample_d3d11/shaders/eye.hlsli
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

#ifndef EYE_HLSLI
#define EYE_HLSLI

#include "common.hlsli"
#include "lighting.hlsli"
#include "GFSDK_FaceWorks.hlsli"

#pragma warning(disable: 3571)	// pow() doesn't handle negative numbers

cbuffer cbShader : CB_SHADER
{
	float		g_normalStrength;
	float		g_specReflectance;
	float		g_gloss;
	float3		g_rgbDeepScatter;

	// Iris parameters
	float		g_irisRadiusSource;		// Radius of iris in iris texture (in UV units)
	float		g_irisRadiusDest;		// Radius of iris in schlera texture (in UV units)
	float		g_irisEdgeHardness;		// Controls hardness/softness of iris edge
	float		g_irisDilation;			// How much the iris is dilated

	GFSDK_FaceWorks_CBData	g_faceworksData;
}

Texture2D<float3> g_texDiffuseSclera	: TEX_DIFFUSE0;
Texture2D<float3> g_texDiffuseIris		: TEX_DIFFUSE1;
Texture2D<float3> g_texNormal			: TEX_NORMAL;



void EyeMegashader(
	in Vertex i_vtx,
	in float3 i_vecCamera,
	in float4 i_uvzwShadow,
	out float3 o_rgbLit,
	uniform bool useSSS,
	uniform bool useDeepScatter)
{
	float2 uv = i_vtx.m_uv;

	// Calculate diffuse color, overlaying iris on sclera

	float3 rgbDiffuse = g_texDiffuseSclera.Sample(g_ssTrilinearRepeatAniso, uv);
	float irisAlpha = 0.0;

	float radiusDest = length(uv - 0.5.xx);
	if (radiusDest < g_irisRadiusDest)
	{
		// Use a power function to remap the radius, to simulate dilation of the iris
		float radiusSource = (1.0 - pow(1.0 - radiusDest / g_irisRadiusDest, 1.0 - g_irisDilation)) * g_irisRadiusSource;
		float2 uvIris = (uv - 0.5.xx) * (radiusSource / radiusDest) + 0.5.xx;
		float3 rgbIris = g_texDiffuseIris.Sample(g_ssTrilinearRepeatAniso, uvIris);

		// Calculate alpha using a smoothstep-like falloff at the edge of the iris
		irisAlpha = saturate((g_irisRadiusDest - radiusDest) * g_irisEdgeHardness);
		irisAlpha = (3.0 - 2.0 * irisAlpha) * square(irisAlpha);
		rgbDiffuse = lerp(rgbDiffuse, rgbIris, irisAlpha);
	}

	// Sample other textures
	float3 normalTangent = UnpackNormal(g_texNormal.Sample(g_ssTrilinearRepeatAniso, uv),
										g_normalStrength);

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

	// Lerp normals to flat, and gloss to 1.0 in the iris region
	normalTangent = lerp(normalTangent, float3(0, 0, 1), irisAlpha);
	normalTangentBlurred = lerp(normalTangentBlurred, float3(0, 0, 1), irisAlpha);
	float gloss = lerp(g_gloss, 1.0, irisAlpha);

	LightingMegashader(
		i_vtx,
		i_vecCamera,
		i_uvzwShadow,
		rgbDiffuse,
		normalTangent,
		normalTangentBlurred,
		g_specReflectance,
		gloss,
		g_rgbDeepScatter,
		g_faceworksData,
		o_rgbLit,
		true,	// useNormalMap
		useSSS,
		useDeepScatter);
}

#endif // EYE_HLSLI
