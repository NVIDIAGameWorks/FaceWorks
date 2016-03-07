//----------------------------------------------------------------------------------
// File:        FaceWorks/include/GFSDK_FaceWorks.hlsli
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

#ifndef GFSDK_FACEWORKS_HLSLI
#define GFSDK_FACEWORKS_HLSLI

// =================================================================================
//	Constant buffer data
// =================================================================================

/// Include this struct in your constant buffer; provides data to the SSS and deep scatter APIs
/// (matches the corresponding struct in GFSDK_FaceWorks.h)
struct GFSDK_FaceWorks_CBData
{
	float4 data[3];
};



// =================================================================================
//	Shader API for SSS
// =================================================================================

/// Calculate mip level at which to sample normal map, to get the blurred normal to pass to
/// GFSDK_FaceWorks_EvaluateSSSDiffuseLight.
///
/// \param cbdata			[in] the cbdata structure
/// \param texNormal		[in] the texture to use
/// \param samp				[in] the sampler to use
/// \param uv				[in] the UV to sample at
///
/// \return					the mip level to sample at.
float GFSDK_FaceWorks_CalculateMipLevelForBlurredNormal(
	GFSDK_FaceWorks_CBData cbdata,
	Texture2D texNormal,
	SamplerState samp,
	float2 uv);

/// Calculate mip level at which to sample normal map, to get the blurred normal to pass to
/// GFSDK_FaceWorks_EvaluateSSSDiffuseLight.
///
/// \param cbdata			[in] the cbdata structure
/// \param texNormal		[in] the texture to use (float3)
/// \param samp				[in] the sampler to use
/// \param uv				[in] the UV to sample at
///
/// \return					the mip level to sample at.
float GFSDK_FaceWorks_CalculateMipLevelForBlurredNormal(
	GFSDK_FaceWorks_CBData cbdata,
	Texture2D<float3> texNormal,
	SamplerState samp,
	float2 uv);

/// Calculate mip level at which to sample normal map, to get the blurred normal to pass to
/// GFSDK_FaceWorks_EvaluateSSSDiffuseLight.
///
/// \param cbdata			[in] the cbdata structure
/// \param texNormal		[in] the texture to use (float2)
/// \param samp				[in] the sampler to use
/// \param uv				[in] the UV to sample at
///
/// \return					the mip level to sample at.
float GFSDK_FaceWorks_CalculateMipLevelForBlurredNormal(
	GFSDK_FaceWorks_CBData cbdata,
	Texture2D<float2> texNormal,
	SamplerState samp,
	float2 uv);

/// Evaluate SSS diffuse light for a single light source.
///
/// \param cbdata			[in] the cbdata structure
/// \param normalGeom		[in] the geometric normal
/// \param normalShade		[in] the shading normal
/// \param normalBlurred	[in] the blurred shading normal
/// \param vecToLight		[in] the normalized vector toward light
/// \param curvature		[in] curvature of the surface being shaded (interpolated from precomputed 
/// 						per-vertex values)
/// \param texCurvatureLUT	[in] the texture containing curvature look up table
/// \param ssBilinearClamp	[in] the sampler state
///
/// \return					the SSS lighting value, to be multiplied by the shadow color, diffuse color, and light color.
float3 GFSDK_FaceWorks_EvaluateSSSDirectLight(
	GFSDK_FaceWorks_CBData cbdata,
	float3 normalGeom,
	float3 normalShade,
	float3 normalBlurred,
	float3 vecToLight,
	float curvature,
	Texture2D texCurvatureLUT,
	SamplerState ssBilinearClamp);

/// Evaluate SSS shadow color for a single light source.
///
/// \param cbdata			[in] the cbdata structure
/// \param normalGeom		[in] the geometric normal
/// \param vecToLight		[in] the normalized vector toward light
/// \param shadow			[in] wide shadow filter (0 = fully shadowed, 1 = fully lit).
/// \param texShadowLUT		[in] the texture containing curvature look up table
/// \param ssBilinearClamp	[in] the sampler state
///
/// \return					the shadow color, including sharpened shadows and SSS light bleeding.
float3 GFSDK_FaceWorks_EvaluateSSSShadow(
	GFSDK_FaceWorks_CBData cbdata,
	float3 normalGeom,
	float3 vecToLight,
	float shadow,
	Texture2D texShadowLUT,
	SamplerState ssBilinearClamp);

/// Sharpen an input shadow value to approximate the output shadow after mapping through the
/// shadow LUT.  This sharpened shadow can be used for specular or other lighting components.
///
/// \param shadow			shadow value (0 = full shadow, 1 = full light)
/// \param shadowSharpening	sharpening factor (same as m_shadowSharpening in GFSDK_FaceWorks_LUTConfig)
///
/// \return					the sharpened shadow value
float GFSDK_FaceWorks_SharpenShadow(
	float shadow,
	float shadowSharpening);

/// Calculate three normals at which to sample diffuse ambient light (such as SH light or
/// a preconvolved cubemap) for SSS ambient light.
/// 
/// \param normalShade		[in] the shading normal
/// \param normalBlurred	[in] the blurred shading normal
/// \param o_normalAmbient0	[out] first normal at which to sample diffuse ambient light
/// \param o_normalAmbient1	[out] second normal at which to sample diffuse ambient light
/// \param o_normalAmbient2	[out] third normal at which to sample diffuse ambient light
void GFSDK_FaceWorks_CalculateNormalsForAmbientLight(
	float3 normalShade,
	float3 normalBlurred,
	out float3 o_normalAmbient0,
	out float3 o_normalAmbient1,
	out float3 o_normalAmbient2);

/// Evaluate SSS ambient light.
/// \param rgbAmbient0		[in] ambient light evaluated at the first normal calculated by GFSDK_FaceWorks_CalculateNormalsForAmbientLight.
/// \param rgbAmbient1		[in] ambient light evaluated at the first normal calculated by GFSDK_FaceWorks_CalculateNormalsForAmbientLight.
/// \param rgbAmbient2		[in] ambient light evaluated at the first normal calculated by GFSDK_FaceWorks_CalculateNormalsForAmbientLight.
///
/// \return					the SSS lighting value, to be multiplied by the diffuse color.
float3 GFSDK_FaceWorks_EvaluateSSSAmbientLight(
	float3 rgbAmbient0,
	float3 rgbAmbient1,
	float3 rgbAmbient2);



// =================================================================================
//	Shader API for deep scatter
// =================================================================================

/// Estimate thickness from parallel shadow map using 8-tap Poisson disc filter
///
/// \param cbdata			[in] the cbdata structure
/// \param texDepth			[in] the shadow map (as a depth texture)
/// \param ss				[in] the sampler state
/// \param uvzShadow		[in] and the position in [0, 1]-space at which to sample the shadow map.
///
/// \return					the estimated thickness value
float GFSDK_FaceWorks_EstimateThicknessFromParallelShadowPoisson8(
	GFSDK_FaceWorks_CBData cbdata,
	Texture2D<float> texDepth,
	SamplerState ss,
	float3 uvzShadow);

/// Estimate thickness from perspective shadow map using 8-tap Poisson disc filter
///
/// \param cbdata			[in] the cbdata structure
/// \param texDepth			[in] the shadow map (as a depth texture)
/// \param ss				[in] the sampler state
/// \param uvzShadow		[in] and the position in [0, 1]-space at which to sample the shadow map.
///
/// \return					the estimated thickness value
float GFSDK_FaceWorks_EstimateThicknessFromPerspectiveShadowPoisson8(
	GFSDK_FaceWorks_CBData cbdata,
	Texture2D<float> texDepth,
	SamplerState ss,
	float3 uvzShadow);

/// Estimate thickness from parallel shadow map using 32-tap Poisson disc filter
///
/// \param cbdata			[in] the cbdata structure
/// \param texDepth			[in] the shadow map (as a depth texture)
/// \param ss				[in] the sampler state
/// \param uvzShadow		[in] and the position in [0, 1]-space at which to sample the shadow map.
///
/// \return					the estimated thickness value
float GFSDK_FaceWorks_EstimateThicknessFromParallelShadowPoisson32(
	GFSDK_FaceWorks_CBData cbdata,
	Texture2D<float> texDepth,
	SamplerState ss,
	float3 uvzShadow);

/// Estimate thickness from perspective shadow map using 32-tap Poisson disc filter
///
/// \param cbdata			[in] the cbdata structure
/// \param texDepth			[in] the shadow map (as a depth texture)
/// \param ss				[in] the sampler state
/// \param uvzShadow		[in] and the position in [0, 1]-space at which to sample the shadow map.
///
/// \return					the estimated thickness value
float GFSDK_FaceWorks_EstimateThicknessFromPerspectiveShadowPoisson32(
	GFSDK_FaceWorks_CBData cbdata,
	Texture2D<float> texDepth,
	SamplerState ss,
	float3 uvzShadow);

/// Evaluate deep scattered light for a single light source.
///
/// \param cbdata			[in] the cbdata structure
/// \param normalBlurred	[in] the blurred shading normal
/// \param vecToLight		[in] the normalized vector toward light
/// \param thickness		[in] the object thickness estimated from shadow map
///
/// \return					the scalar deep scatter lighting value, to be multiplied by the deep scatter
/// 						color, diffuse color and light color.
float GFSDK_FaceWorks_EvaluateDeepScatterDirectLight(
	GFSDK_FaceWorks_CBData cbdata,
	float3 normalBlurred,
	float3 vecToLight,
	float thickness);



// ======================================================================================
//	Implementation
// ======================================================================================

// Structure used to unpack the CB data
struct nvsf_CBData
{
	// SSS constants
	float2	nvsf_CurvatureScaleBias;
	float2	nvsf_ShadowScaleBias;
	float	nvsf_MinLevelForBlurredNormal;

	// Deep scatter constants
	float	nvsf_DeepScatterFalloff;
	float	nvsf_ShadowFilterRadius;
	float	nvsf_DecodeDepthScale, nvsf_DecodeDepthBias;
};

nvsf_CBData nvsf_UnpackCBData(GFSDK_FaceWorks_CBData nvsf_opaqueData)
{
	nvsf_CBData nvsf_out = (nvsf_CBData)0;
	nvsf_out.nvsf_CurvatureScaleBias = nvsf_opaqueData.data[0].xy;
	nvsf_out.nvsf_ShadowScaleBias = nvsf_opaqueData.data[0].zw;
	nvsf_out.nvsf_MinLevelForBlurredNormal = nvsf_opaqueData.data[1].x;
	nvsf_out.nvsf_DeepScatterFalloff = nvsf_opaqueData.data[1].y;
	nvsf_out.nvsf_ShadowFilterRadius = nvsf_opaqueData.data[1].z;
	nvsf_out.nvsf_DecodeDepthScale = nvsf_opaqueData.data[1].w;
	nvsf_out.nvsf_DecodeDepthBias = nvsf_opaqueData.data[2].x;
	return nvsf_out;
}



// ======================================================================================
//	Shader API for SSS
// ======================================================================================

float GFSDK_FaceWorks_CalculateMipLevelForBlurredNormal(
	GFSDK_FaceWorks_CBData nvsf_opaqueData,
	Texture2D nvsf_texNormal,
	SamplerState nvsf_ss,
	float2 nvsf_uv)
{
	nvsf_CBData nvsf_cb = nvsf_UnpackCBData(nvsf_opaqueData);
	return max(nvsf_texNormal.CalculateLevelOfDetail(nvsf_ss, nvsf_uv),
				nvsf_cb.nvsf_MinLevelForBlurredNormal);
}
float GFSDK_FaceWorks_CalculateMipLevelForBlurredNormal(
	GFSDK_FaceWorks_CBData nvsf_opaqueData,
	Texture2D<float3> nvsf_texNormal,
	SamplerState nvsf_ss,
	float2 nvsf_uv)
{
	nvsf_CBData nvsf_cb = nvsf_UnpackCBData(nvsf_opaqueData);
	return max(nvsf_texNormal.CalculateLevelOfDetail(nvsf_ss, nvsf_uv),
				nvsf_cb.nvsf_MinLevelForBlurredNormal);
}
float GFSDK_FaceWorks_CalculateMipLevelForBlurredNormal(
	GFSDK_FaceWorks_CBData nvsf_opaqueData,
	Texture2D<float2> nvsf_texNormal,
	SamplerState nvsf_ss,
	float2 nvsf_uv)
{
	nvsf_CBData nvsf_cb = nvsf_UnpackCBData(nvsf_opaqueData);
	return max(nvsf_texNormal.CalculateLevelOfDetail(nvsf_ss, nvsf_uv),
				nvsf_cb.nvsf_MinLevelForBlurredNormal);
}

float3 GFSDK_FaceWorks_EvaluateSSSDirectLight(
	GFSDK_FaceWorks_CBData nvsf_opaqueData,
	float3 nvsf_normalGeom,
	float3 nvsf_normalShade,
	float3 nvsf_normalBlurred,
	float3 nvsf_vecToLight,
	float nvsf_curvature,
	Texture2D nvsf_texCurvatureLUT,
	SamplerState nvsf_ss)
{
	nvsf_CBData nvsf_cb = nvsf_UnpackCBData(nvsf_opaqueData);

	// Curvature-based scattering
	float nvsf_NdotLBlurredUnclamped = dot(nvsf_normalBlurred, nvsf_vecToLight);
	float nvsf_curvatureScaled = nvsf_curvature * nvsf_cb.nvsf_CurvatureScaleBias.x + nvsf_cb.nvsf_CurvatureScaleBias.y;
	float2 nvsf_uvCurvatureLUT = { nvsf_NdotLBlurredUnclamped * 0.5 + 0.5, nvsf_curvatureScaled, };
	float3 nvsf_rgbCurvature = nvsf_texCurvatureLUT.Sample(nvsf_ss, nvsf_uvCurvatureLUT).rgb * 0.5 - 0.25;

	// Normal map scattering using separate normals for R, G, B; here, G and B
	// normals are generated by lerping between the specular and R normals.
	// The lerp factor to generate the G and B normals is increased near the light/dark edge,
	// to try to prevent bumps from showing up as too blue.
	// This will be more complex when arbitrary diffusion profiles are supported.
	float3 nvsf_normalSmoothFactor = saturate(1.0 - nvsf_NdotLBlurredUnclamped);
	nvsf_normalSmoothFactor *= nvsf_normalSmoothFactor;
	float3 nvsf_normalShadeG = normalize(lerp(nvsf_normalShade, nvsf_normalBlurred, 0.3 + 0.7 * nvsf_normalSmoothFactor));
	float3 nvsf_normalShadeB = normalize(lerp(nvsf_normalShade, nvsf_normalBlurred, nvsf_normalSmoothFactor));
	float nvsf_NdotLShadeG = saturate(dot(nvsf_normalShadeG, nvsf_vecToLight));
	float nvsf_NdotLShadeB = saturate(dot(nvsf_normalShadeB, nvsf_vecToLight));
	float3 nvsf_rgbNdotL = float3(saturate(nvsf_NdotLBlurredUnclamped), nvsf_NdotLShadeG, nvsf_NdotLShadeB);

	return saturate(nvsf_rgbCurvature + nvsf_rgbNdotL);
}

float3 GFSDK_FaceWorks_EvaluateSSSShadow(
	GFSDK_FaceWorks_CBData nvsf_opaqueData,
	float3 nvsf_normalGeom,
	float3 nvsf_vecToLight,
	float nvsf_shadow,
	Texture2D nvsf_texShadowLUT,
	SamplerState nvsf_ss)
{
	nvsf_CBData nvsf_cb = nvsf_UnpackCBData(nvsf_opaqueData);

	// Shadow penumbra scattering
	float nvsf_NdotLGeom = saturate(dot(nvsf_normalGeom, nvsf_vecToLight));
	float2 nvsf_uvShadowLUT =
	{
		nvsf_shadow,
		nvsf_NdotLGeom * nvsf_cb.nvsf_ShadowScaleBias.x + nvsf_cb.nvsf_ShadowScaleBias.y,
	};
	return nvsf_texShadowLUT.Sample(nvsf_ss, nvsf_uvShadowLUT).rgb;
}

float GFSDK_FaceWorks_SharpenShadow(
	float nvsf_shadow,
	float nvsf_shadowSharpening)
{
	// Use smoothstep to approximate the transfer function of a disc or Gaussian shadow filter
	float nvsf_scale = nvsf_shadowSharpening * 0.9;
	float nvsf_bias = -0.5 * nvsf_scale + 0.5;
	return smoothstep(0, 1, nvsf_shadow * nvsf_scale + nvsf_bias);
}

void GFSDK_FaceWorks_CalculateNormalsForAmbientLight(
	float3 nvsf_normalShade,
	float3 nvsf_normalBlurred,
	out float3 nvsf_o_normalAmbient0,
	out float3 nvsf_o_normalAmbient1,
	out float3 nvsf_o_normalAmbient2)
{
	// Same normals as for direct light, but no NdotL factor involved.
	// This will be more complex when arbitrary diffusion profiles are supported.
	nvsf_o_normalAmbient0 = nvsf_normalBlurred;
	nvsf_o_normalAmbient1 = normalize(lerp(nvsf_normalShade, nvsf_normalBlurred, 0.3));
	nvsf_o_normalAmbient2 = nvsf_normalShade;
}

float3 GFSDK_FaceWorks_EvaluateSSSAmbientLight(
	float3 nvsf_rgbAmbient0,
	float3 nvsf_rgbAmbient1,
	float3 nvsf_rgbAmbient2)
{
	// This will be more complex when arbitrary diffusion profiles are supported.
	float3 nvsf_result;
	nvsf_result.r = nvsf_rgbAmbient0.r;
	nvsf_result.g = nvsf_rgbAmbient1.g;
	nvsf_result.b = nvsf_rgbAmbient2.b;
	return nvsf_result;
}



// ======================================================================================
//	Shader API for deep scatter
// ======================================================================================

// Poisson disks generated with http://www.coderhaus.com/?p=11

static const float2 nvsf_Poisson8[] = 
{ 	
	{ -0.7494944f, 0.1827986f, },
	{ -0.8572887f, -0.4169083f, },
	{ -0.1087135f, -0.05238153f, },
	{ 0.1045462f, 0.9657645f, },
	{ -0.0135659f, -0.698451f, },
	{ -0.4942278f, 0.7898396f, },
	{ 0.7970678f, -0.4682421f, },
	{ 0.8084122f, 0.533884f },
};

static const float2 nvsf_Poisson32[] = 
{
	{ -0.975402, -0.0711386 },
	{ -0.920347, -0.41142 },
	{ -0.883908, 0.217872 },
	{ -0.884518, 0.568041 },
	{ -0.811945, 0.90521 },
	{ -0.792474, -0.779962 },
	{ -0.614856, 0.386578 },
	{ -0.580859, -0.208777 },
	{ -0.53795, 0.716666 },
	{ -0.515427, 0.0899991 },
	{ -0.454634, -0.707938 },
	{ -0.420942, 0.991272 },
	{ -0.261147, 0.588488 },
	{ -0.211219, 0.114841 },
	{ -0.146336, -0.259194 },
	{ -0.139439, -0.888668 },
	{ 0.0116886, 0.326395 },
	{ 0.0380566, 0.625477 },
	{ 0.0625935, -0.50853 },
	{ 0.125584, 0.0469069 },
	{ 0.169469, -0.997253 },
	{ 0.320597, 0.291055 },
	{ 0.359172, -0.633717 },
	{ 0.435713, -0.250832 },
	{ 0.507797, -0.916562 },
	{ 0.545763, 0.730216 },
	{ 0.56859, 0.11655 },
	{ 0.743156, -0.505173 },
	{ 0.736442, -0.189734 },
	{ 0.843562, 0.357036 },
	{ 0.865413, 0.763726 },
	{ 0.872005, -0.927 },
};

float nvsf_LinearizePerspectiveDepth(nvsf_CBData nvsf_cb, float nvsf_depth)
{
	return 1.0 / (nvsf_depth * nvsf_cb.nvsf_DecodeDepthScale + nvsf_cb.nvsf_DecodeDepthBias);
}

float GFSDK_FaceWorks_EstimateThicknessFromParallelShadowPoisson8(
	GFSDK_FaceWorks_CBData nvsf_opaqueData,
	Texture2D<float> nvsf_texDepth,
	SamplerState nvsf_ss,
	float3 nvsf_uvzShadow)
{
	nvsf_CBData nvsf_cb = nvsf_UnpackCBData(nvsf_opaqueData);

	float nvsf_sampleSum = 0.0;
	[unroll] for (int i = 0; i < 8; ++i)
	{
		float2 nvsf_uvDelta = nvsf_Poisson8[i] * nvsf_cb.nvsf_ShadowFilterRadius;
		float2 nvsf_uvSample = nvsf_uvzShadow.xy + nvsf_uvDelta;
		float nvsf_zShadowMap = nvsf_texDepth.Sample(nvsf_ss, nvsf_uvSample);
		nvsf_sampleSum += max(0, nvsf_uvzShadow.z - nvsf_zShadowMap);
	}

	return nvsf_cb.nvsf_DecodeDepthScale * nvsf_sampleSum * (1.0 / 8.0);
}

float GFSDK_FaceWorks_EstimateThicknessFromPerspectiveShadowPoisson8(
	GFSDK_FaceWorks_CBData nvsf_opaqueData,
	Texture2D<float> nvsf_texDepth,
	SamplerState nvsf_ss,
	float3 nvsf_uvzShadow)
{
	nvsf_CBData nvsf_cb = nvsf_UnpackCBData(nvsf_opaqueData);

	float nvsf_sampleSum = 0.0;
	float nvsf_linearDepth = nvsf_LinearizePerspectiveDepth(nvsf_cb, nvsf_uvzShadow.z);
	[unroll] for (int i = 0; i < 8; ++i)
	{
		float2 nvsf_uvDelta = nvsf_Poisson8[i] * nvsf_cb.nvsf_ShadowFilterRadius;
		float2 nvsf_uvSample = nvsf_uvzShadow.xy + nvsf_uvDelta;
		float nvsf_zShadowMap = nvsf_texDepth.Sample(nvsf_ss, nvsf_uvSample);
		nvsf_sampleSum += max(0, nvsf_linearDepth - nvsf_LinearizePerspectiveDepth(nvsf_cb, nvsf_zShadowMap));
	}

	return nvsf_sampleSum * (1.0 / 8.0);
}

float GFSDK_FaceWorks_EstimateThicknessFromParallelShadowPoisson32(
	GFSDK_FaceWorks_CBData nvsf_opaqueData,
	Texture2D<float> nvsf_texDepth,
	SamplerState nvsf_ss,
	float3 nvsf_uvzShadow)
{
	nvsf_CBData nvsf_cb = nvsf_UnpackCBData(nvsf_opaqueData);

	float nvsf_sampleSum = 0.0;
	[unroll] for (int i = 0; i < 32; ++i)
	{
		float2 nvsf_uvDelta = nvsf_Poisson32[i] * nvsf_cb.nvsf_ShadowFilterRadius;
		float2 nvsf_uvSample = nvsf_uvzShadow.xy + nvsf_uvDelta;
		float nvsf_zShadowMap = nvsf_texDepth.Sample(nvsf_ss, nvsf_uvSample);
		nvsf_sampleSum += max(0, nvsf_uvzShadow.z - nvsf_zShadowMap);
	}

	return nvsf_cb.nvsf_DecodeDepthScale * nvsf_sampleSum * (1.0 / 32.0);
}

float GFSDK_FaceWorks_EstimateThicknessFromPerspectiveShadowPoisson32(
	GFSDK_FaceWorks_CBData nvsf_opaqueData,
	Texture2D<float> nvsf_texDepth,
	SamplerState nvsf_ss,
	float3 nvsf_uvzShadow)
{
	nvsf_CBData nvsf_cb = nvsf_UnpackCBData(nvsf_opaqueData);

	float nvsf_sampleSum = 0.0;
	float nvsf_linearDepth = nvsf_LinearizePerspectiveDepth(nvsf_cb, nvsf_uvzShadow.z);
	[unroll] for (int i = 0; i < 32; ++i)
	{
		float2 nvsf_uvDelta = nvsf_Poisson32[i] * nvsf_cb.nvsf_ShadowFilterRadius;
		float2 nvsf_uvSample = nvsf_uvzShadow.xy + nvsf_uvDelta;
		float nvsf_zShadowMap = nvsf_texDepth.Sample(nvsf_ss, nvsf_uvSample);
		nvsf_sampleSum += max(0, nvsf_linearDepth - nvsf_LinearizePerspectiveDepth(nvsf_cb, nvsf_zShadowMap));
	}

	return nvsf_sampleSum * (1.0 / 32.0);
}

float GFSDK_FaceWorks_EvaluateDeepScatterDirectLight(
	GFSDK_FaceWorks_CBData nvsf_opaqueData,
	float3 nvsf_normalBlurred,
	float3 nvsf_vecToLight,
	float nvsf_thickness)
{
	nvsf_CBData nvsf_cb = nvsf_UnpackCBData(nvsf_opaqueData);

	float nvsf_transmittance = exp2(nvsf_cb.nvsf_DeepScatterFalloff * nvsf_thickness * nvsf_thickness);
	float nvsf_minusNDotL = -dot(nvsf_normalBlurred, nvsf_vecToLight);
	return nvsf_transmittance * saturate(nvsf_minusNDotL + 0.3);
}

#endif // GFSDK_FACEWORKS_HLSLI
