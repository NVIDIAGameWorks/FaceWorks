//----------------------------------------------------------------------------------
// File:        FaceWorks/samples/sample_d3d11/shaders/gaussian_ps.hlsl
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

cbuffer cbShader : CB_SHADER
{
	float2	g_vecBlur;
}

Texture2D<float2> g_texSrc : TEX_SOURCE;

// Gaussian blur coefficients - x = weight, y = position offset
// Python code to generate:
// import math
// n = 21
// points = [-3.0 + 6.0 * float(i) / float(n - 1) for i in range(n)]
// weights = [math.exp(-0.5 * x**2) for x in points]
// weightSum = sum(weights)
// print(("static const float2 s_aGaussian%d[] =\n{\n" % n) + ''.join(
//     "    { %0.5f, %4.2f },\n" % (weights[i]/weightSum, points[i]) for i in range(n)) + "};")

static const float2 s_aGaussian21[] =
{
	{ 0.00133, -3.00 },
	{ 0.00313, -2.70 },
	{ 0.00673, -2.40 },
	{ 0.01322, -2.10 },
	{ 0.02372, -1.80 },
	{ 0.03892, -1.50 },
	{ 0.05835, -1.20 },
	{ 0.07995, -0.90 },
	{ 0.10012, -0.60 },
	{ 0.11460, -0.30 },
	{ 0.11987,  0.00 },
	{ 0.11460,  0.30 },
	{ 0.10012,  0.60 },
	{ 0.07995,  0.90 },
	{ 0.05835,  1.20 },
	{ 0.03892,  1.50 },
	{ 0.02372,  1.80 },
	{ 0.01322,  2.10 },
	{ 0.00673,  2.40 },
	{ 0.00313,  2.70 },
	{ 0.00133,  3.00 },
};

float2 main(in float2 i_uv : UV) : SV_Target
{
	float2 sum = 0.0;

	[unroll] for (int i = 0; i < 21; ++i)
	{
		float weight = s_aGaussian21[i].x;
		float2 offset = s_aGaussian21[i].y * g_vecBlur;
		sum += weight * g_texSrc.Sample(g_ssBilinearClamp, i_uv + offset);
	}

	return sum;
}
