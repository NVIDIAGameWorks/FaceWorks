//----------------------------------------------------------------------------------
// File:        FaceWorks/samples/sample_d3d11/shaders/resources.h
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

#ifndef RESOURCES_H
#define RESOURCES_H

// This file is included from both C++ and HLSL; it defines shared resource slot assignments

#ifdef __cplusplus
#	define CBREG(n)						n
#	define TEXREG(n)					n
#	define SAMPREG(n)					n
#else
#	define CBREG(n)						register(b##n)
#	define TEXREG(n)					register(t##n)
#	define SAMPREG(n)					register(s##n)
#endif

#define CB_DEBUG						CBREG(0)
#define CB_FRAME						CBREG(1)
#define CB_SHADER						CBREG(2)

#define TEX_CUBE_DIFFUSE				TEXREG(0)
#define TEX_CUBE_SPEC					TEXREG(1)
#define TEX_SHADOW_MAP					TEXREG(2)
#define TEX_VSM							TEXREG(3)
#define TEX_DIFFUSE0					TEXREG(4)
#define TEX_DIFFUSE1					TEXREG(5)
#define TEX_NORMAL						TEXREG(6)
#define TEX_SPEC						TEXREG(7)
#define TEX_GLOSS						TEXREG(8)
#define TEX_SSS_MASK					TEXREG(9)
#define TEX_DEEP_SCATTER_COLOR			TEXREG(10)
#define TEX_SOURCE						TEXREG(11)
#define TEX_CURVATURE_LUT				TEXREG(12)
#define TEX_SHADOW_LUT					TEXREG(13)

#define SAMP_POINT_CLAMP				SAMPREG(0)
#define SAMP_BILINEAR_CLAMP				SAMPREG(1)
#define SAMP_TRILINEAR_REPEAT			SAMPREG(2)
#define SAMP_TRILINEAR_REPEAT_ANISO		SAMPREG(3)
#define SAMP_PCF						SAMPREG(4)

#endif // RESOURCES_H
