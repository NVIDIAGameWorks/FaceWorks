//----------------------------------------------------------------------------------
// File:        FaceWorks/samples/sample_d3d11/shaders/tess_hs.hlsl
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
#include "tess.hlsli"

void calcHSConstants(
	in InputPatch<Vertex, 3> i_cps,
	out PatchConstData o_pcd)
{
	// Backface culling: check if the camera is behind all three tangent planes
	float3 vecNdotV =
	{
		dot(g_posCamera - i_cps[0].m_pos, i_cps[0].m_normal),
		dot(g_posCamera - i_cps[1].m_pos, i_cps[1].m_normal),
		dot(g_posCamera - i_cps[2].m_pos, i_cps[2].m_normal),
	};
	if (all(vecNdotV < 0.0))
	{
		o_pcd.m_tessFactor[0] = 0.0;
		o_pcd.m_tessFactor[1] = 0.0;
		o_pcd.m_tessFactor[2] = 0.0;
		o_pcd.m_insideTessFactor = 0.0;
		return;
	}

	// Frustum culling: check if all three verts are out on the same side of the frustum
	// This isn't quite correct because the displacement could make a patch visible even if
	// it fails this test; but in practice this is nearly impossible to notice
	float4 posClip0 = mul(float4(i_cps[0].m_pos, 1.0), g_matWorldToClip);
	float4 posClip1 = mul(float4(i_cps[1].m_pos, 1.0), g_matWorldToClip);
	float4 posClip2 = mul(float4(i_cps[2].m_pos, 1.0), g_matWorldToClip);
	float3 xs = { posClip0.x, posClip1.x, posClip2.x };
	float3 ys = { posClip0.y, posClip1.y, posClip2.y };
	float3 ws = { posClip0.w, posClip1.w, posClip2.w };
	if (all(xs < -ws) || all(xs > ws) || all(ys < -ws) || all(ys > ws))
	{
		o_pcd.m_tessFactor[0] = 0.0;
		o_pcd.m_tessFactor[1] = 0.0;
		o_pcd.m_tessFactor[2] = 0.0;
		o_pcd.m_insideTessFactor = 0.0;
		return;
	}

	// Adaptive tessellation based on a screen-space error estimate using curvature

	// Calculate approximate screen-space edge length, but including z length as well,
	// so we don't undertessellate edges that are foreshortened
	float edge0 = length(i_cps[2].m_pos - i_cps[1].m_pos) / (0.5 * (posClip2.w + posClip1.w));
	float edge1 = length(i_cps[0].m_pos - i_cps[2].m_pos) / (0.5 * (posClip0.w + posClip2.w));
	float edge2 = length(i_cps[1].m_pos - i_cps[0].m_pos) / (0.5 * (posClip1.w + posClip0.w));

	// Calculate dots of the two normals on each edge - used to give more tessellation
	// in areas with higher curvature
	float normalDot0 = dot(i_cps[2].m_normal, i_cps[1].m_normal);
	float normalDot1 = dot(i_cps[0].m_normal, i_cps[2].m_normal);
	float normalDot2 = dot(i_cps[1].m_normal, i_cps[0].m_normal);

	// Calculate target screen-space error
	static const float errPxTarget = 0.5;
	static const float tanHalfFov = tan(0.5 * 0.5);
	static const float errTarget = errPxTarget * 2.0 * tanHalfFov / 1080.0;

	// Calculate tess factors using curve fitting approximation to screen-space error
	// derived from curvature and edge length
	static const float tessScale = 0.41 / sqrt(errTarget);
	o_pcd.m_tessFactor[0] = g_tessScale * sqrt(edge0) * pow(1.0 - saturate(normalDot0), 0.27);
	o_pcd.m_tessFactor[1] = g_tessScale * sqrt(edge1) * pow(1.0 - saturate(normalDot1), 0.27);
	o_pcd.m_tessFactor[2] = g_tessScale * sqrt(edge2) * pow(1.0 - saturate(normalDot2), 0.27);

	// Clamp to supported range
	o_pcd.m_tessFactor[0] = clamp(o_pcd.m_tessFactor[0], 1.0, s_tessFactorMax);
	o_pcd.m_tessFactor[1] = clamp(o_pcd.m_tessFactor[1], 1.0, s_tessFactorMax);
	o_pcd.m_tessFactor[2] = clamp(o_pcd.m_tessFactor[2], 1.0, s_tessFactorMax);

	// Set interior tess factor to maximum of edge factors
	o_pcd.m_insideTessFactor = max(max(o_pcd.m_tessFactor[0],
									   o_pcd.m_tessFactor[1]),
									   o_pcd.m_tessFactor[2]);
}

[domain("tri")]
[maxtessfactor(s_tessFactorMax)]
[outputcontrolpoints(3)]
[outputtopology("triangle_cw")]
[partitioning("fractional_odd")]
[patchconstantfunc("calcHSConstants")]
Vertex main(
	in InputPatch<Vertex, 3> i_cps,
	in uint iCp : SV_OutputControlPointID)
{
	return i_cps[iCp];
}
