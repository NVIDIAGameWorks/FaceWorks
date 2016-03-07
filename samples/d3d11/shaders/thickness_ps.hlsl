//----------------------------------------------------------------------------------
// File:        FaceWorks/samples/sample_d3d11/shaders/thickness_ps.hlsl
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
#include "GFSDK_FaceWorks.hlsli"

cbuffer cbShader : CB_SHADER
{
	GFSDK_FaceWorks_CBData	g_faceworksData;
}

void main(
	in Vertex i_vtx,
	in float3 i_vecCamera : CAMERA,
	in float4 i_uvzwShadow : UVZW_SHADOW,
	out float4 o_rgba : SV_Target)
{
	float3 normalGeom = normalize(i_vtx.m_normal);
	float3 uvzShadow = i_uvzwShadow.xyz / i_uvzwShadow.w;

	// Apply normal offset to avoid silhouette edge artifacts
	// !!!UNDONE: move this to vertex shader
	float3 normalShadow = mul(normalGeom, g_matWorldToUvzShadowNormal);
	uvzShadow += normalShadow * g_deepScatterNormalOffset;

	float thickness = GFSDK_FaceWorks_EstimateThicknessFromParallelShadowPoisson32(
						g_faceworksData,
						g_texShadowMap, g_ssBilinearClamp, uvzShadow);

	o_rgba = float4(thickness.xxx * 0.05, 1.0);
}
