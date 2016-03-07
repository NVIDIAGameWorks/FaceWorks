//----------------------------------------------------------------------------------
// File:        FaceWorks/samples/sample_d3d11/shaders/tess_ds.hlsl
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

[domain("tri")]
void main(
	in OutputPatch<Vertex, 3> i_cps,
	in PatchConstData i_pcd,
	in float3 i_bary : SV_DomainLocation,
	out Vertex o_vtx,
	out float3 o_vecCamera : CAMERA,
	out float4 o_uvzwShadow : UVZW_SHADOW,
	out float4 o_posClip : SV_Position)
{
	// Lerp all attributes but position
	o_vtx.m_normal = i_bary.x * i_cps[0].m_normal + i_bary.y * i_cps[1].m_normal + i_bary.z * i_cps[2].m_normal;
	o_vtx.m_uv = i_bary.x * i_cps[0].m_uv + i_bary.y * i_cps[1].m_uv + i_bary.z * i_cps[2].m_uv;
	o_vtx.m_tangent = i_bary.x * i_cps[0].m_tangent + i_bary.y * i_cps[1].m_tangent + i_bary.z * i_cps[2].m_tangent;
	o_vtx.m_curvature = i_bary.x * i_cps[0].m_curvature + i_bary.y * i_cps[1].m_curvature + i_bary.z * i_cps[2].m_curvature;

	// Calculate output position using Phong tessellation
	// (http://perso.telecom-paristech.fr/~boubek/papers/PhongTessellation/)

	// Compute lerped position
	float3 posVtx = i_bary.x * i_cps[0].m_pos + i_bary.y * i_cps[1].m_pos + i_bary.z * i_cps[2].m_pos;

	// Calculate deltas to project onto three tangent planes
	float3 vecProj0 = dot(i_cps[0].m_pos - posVtx, i_cps[0].m_normal) * i_cps[0].m_normal;
	float3 vecProj1 = dot(i_cps[1].m_pos - posVtx, i_cps[1].m_normal) * i_cps[1].m_normal;
	float3 vecProj2 = dot(i_cps[2].m_pos - posVtx, i_cps[2].m_normal) * i_cps[2].m_normal;

	// Lerp between projection vectors
	float3 vecOffset = i_bary.x * vecProj0 + i_bary.y * vecProj1 + i_bary.z * vecProj2;

	// Add a fraction of the offset vector to the lerped position
	posVtx += 0.5 * vecOffset;

	o_vtx.m_pos = posVtx;
	o_vecCamera = g_posCamera - posVtx;
	o_uvzwShadow = mul(float4(posVtx, 1.0), g_matWorldToUvzwShadow);
	o_posClip = mul(float4(posVtx, 1.0), g_matWorldToClip);
}
