//----------------------------------------------------------------------------------
// File:        FaceWorks/samples/sample_d3d11/shaders/hair_ps.hlsl
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

#include "common.hlsli"
#include "lighting.hlsli"

cbuffer cbShader : CB_SHADER
{
	float		g_specReflectance;
	float		g_gloss;
}

Texture2D<float4> g_texDiffuse			: TEX_DIFFUSE0;

void main(
	in Vertex i_vtx,
	in float3 i_vecCamera : CAMERA,
	in float4 i_uvzwShadow : UVZW_SHADOW,
	in bool front : SV_IsFrontFace,
	out float4 o_rgbaLit : SV_Target)
{
	float2 uv = i_vtx.m_uv;

	// Sample textures

	float4 rgbaDiffuse = g_texDiffuse.Sample(g_ssTrilinearRepeatAniso, uv);

	// Perform lighting

	if (!front)
		i_vtx.m_normal = -i_vtx.m_normal;

	LightingMegashader(
		i_vtx,
		i_vecCamera,
		i_uvzwShadow,
		rgbaDiffuse.rgb,
		0.0.xxx,
		0.0.xxx,
		g_specReflectance,
		g_gloss,
		0.0.xxx,
		(GFSDK_FaceWorks_CBData)0,
		o_rgbaLit.rgb,
		false,	// useNormalMap
		false,	// useSSS
		false);	// useDeepScatter

	// Write texture alpha for transparency
	o_rgbaLit.a = rgbaDiffuse.a;
}
