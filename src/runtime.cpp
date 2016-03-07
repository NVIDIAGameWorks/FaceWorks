//----------------------------------------------------------------------------------
// File:        FaceWorks/src/runtime.cpp
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

#include "internal.h"

#include <cassert>

// ======================================================================================
//     Runtime API for SSS
// ======================================================================================

GFSDK_FaceWorks_Result ValidateSSSConfig(
	const GFSDK_FaceWorks_SSSConfig * pConfig,
	GFSDK_FaceWorks_ErrorBlob * pErrorBlobOut)
{
	if (!pConfig)
	{
		ErrPrintf("pConfig is null\n");
		return GFSDK_FaceWorks_InvalidArgument;
	}

	if (pConfig->m_diffusionRadius <= 0.0f)
	{
		WarnPrintf(
			"pConfig->m_diffusionRadius is %g; should be greater than 0.0\n",
			pConfig->m_diffusionRadius);
	}
	if (pConfig->m_diffusionRadiusLUT <= 0.0f)
	{
		WarnPrintf(
			"pConfig->m_diffusionRadiusLUT is %g; should be greater than 0.0\n",
			pConfig->m_diffusionRadius);
	}
	if (pConfig->m_curvatureRadiusMinLUT <= 0.0f)
	{
		WarnPrintf(
			"pConfig->m_curvatureRadiusMinLUT is %g; should be greater than 0.0\n",
			pConfig->m_curvatureRadiusMinLUT);
	}
	if (pConfig->m_curvatureRadiusMaxLUT <= 0.0f)
	{
		WarnPrintf(
			"pConfig->m_curvatureRadiusMaxLUT is %g; should be greater than 0.0\n",
			pConfig->m_curvatureRadiusMaxLUT);
	}
	if (pConfig->m_curvatureRadiusMaxLUT < pConfig->m_curvatureRadiusMinLUT)
	{
		WarnPrintf(
			"pConfig->m_curvatureRadiusMaxLUT (%g) is less than pConfig->m_curvatureRadiusMinLUT (%g)\n",
			pConfig->m_curvatureRadiusMaxLUT, pConfig->m_curvatureRadiusMinLUT);
	}
	if (pConfig->m_shadowWidthMinLUT <= 0.0f)
	{
		WarnPrintf(
			"pConfig->m_shadowWidthMinLUT is %g; should be greater than 0.0\n",
			pConfig->m_shadowWidthMinLUT);
	}
	if (pConfig->m_shadowWidthMaxLUT <= 0.0f)
	{
		WarnPrintf(
			"pConfig->m_shadowWidthMaxLUT is %g; should be greater than 0.0\n",
			pConfig->m_shadowWidthMaxLUT);
	}
	if (pConfig->m_shadowWidthMaxLUT < pConfig->m_shadowWidthMinLUT)
	{
		WarnPrintf(
			"pConfig->m_shadowWidthMaxLUT (%g) is less than pConfig->m_shadowWidthMinLUT (%g)\n",
			pConfig->m_shadowWidthMaxLUT, pConfig->m_shadowWidthMinLUT);
	}
	if (pConfig->m_shadowFilterWidth <= 0.0f)
	{
		WarnPrintf(
			"pConfig->m_shadowFilterWidth is %g; should be greater than 0.0\n",
			pConfig->m_shadowFilterWidth);
	}
	if (pConfig->m_normalMapSize < 0)
	{
		WarnPrintf(
			"pConfig->m_normalMapSize is %d; should be at least 0\n",
			pConfig->m_normalMapSize);
	}
	if (pConfig->m_averageUVScale <= 0.0f)
	{
		WarnPrintf(
			"pConfig->m_averageUVScale is %g; should be greater than 0.0\n",
			pConfig->m_averageUVScale);
	}

	return GFSDK_FaceWorks_OK;
}

GFSDK_FACEWORKS_API GFSDK_FaceWorks_Result GFSDK_FACEWORKS_CALLCONV GFSDK_FaceWorks_WriteCBDataForSSS(
	const GFSDK_FaceWorks_SSSConfig * pConfig,
	GFSDK_FaceWorks_CBData * pCBDataOut,
	GFSDK_FaceWorks_ErrorBlob * pErrorBlobOut)
{
	// Validate params

	GFSDK_FaceWorks_Result res = ValidateSSSConfig(pConfig, pErrorBlobOut);
	if (res != GFSDK_FaceWorks_OK)
		return res;

	if (!pCBDataOut)
	{
		ErrPrintf("GFSDK_FaceWorks_WriteCBDataForSSS: pCBDataOut is null\n");
		return GFSDK_FaceWorks_InvalidArgument;
	}

	// The LUTs are built assuming a particular scattering radius, so the
	// curvatures and penumbra widths need to be scaled to match the
	// scattering radius set at runtime.
	float diffusionRadiusFactor = pConfig->m_diffusionRadiusLUT /
									pConfig->m_diffusionRadius;

	// Set up the constant buffer

	float curvatureMin = diffusionRadiusFactor / pConfig->m_curvatureRadiusMaxLUT;
	float curvatureMax = diffusionRadiusFactor / pConfig->m_curvatureRadiusMinLUT;
	float curvatureScale = 1.0f / (curvatureMax - curvatureMin);
	float curvatureBias = -curvatureMin * curvatureScale;

	float shadowRcpWidthMin = diffusionRadiusFactor / pConfig->m_shadowWidthMaxLUT;
	float shadowRcpWidthMax = diffusionRadiusFactor / pConfig->m_shadowWidthMinLUT;
	float shadowScale = 1.0f / (shadowRcpWidthMax - shadowRcpWidthMin);
	float shadowBias = -shadowRcpWidthMin * shadowScale;

	float minLevelForBlurredNormal = log2(max(
										pConfig->m_normalMapSize * pConfig->m_diffusionRadius,
										pConfig->m_averageUVScale)
										/ pConfig->m_averageUVScale);

	// Output to user buffer
	pCBDataOut->data[0].x = curvatureScale;
	pCBDataOut->data[0].y = curvatureBias;
	pCBDataOut->data[0].z = shadowScale / pConfig->m_shadowFilterWidth;
	pCBDataOut->data[0].w = shadowBias;
	pCBDataOut->data[1].x = minLevelForBlurredNormal;

	return GFSDK_FaceWorks_OK;
}



// ======================================================================================
//     Runtime API for deep scatter
// ======================================================================================

GFSDK_FaceWorks_Result ValidateDeepScatterConfig(
	const GFSDK_FaceWorks_DeepScatterConfig * pConfig,
	GFSDK_FaceWorks_ErrorBlob * pErrorBlobOut)
{
	if (!pConfig)
	{
		ErrPrintf("pConfig is null\n");
		return GFSDK_FaceWorks_InvalidArgument;
	}

	if (pConfig->m_radius <= 0.0f)
	{
		WarnPrintf(
			"pConfig->m_radius is %g; should be greater than 0.0\n",
			pConfig->m_radius);
	}
	if (pConfig->m_shadowProjType != GFSDK_FaceWorks_NoProjection &&
		pConfig->m_shadowProjType != GFSDK_FaceWorks_ParallelProjection &&
		pConfig->m_shadowProjType != GFSDK_FaceWorks_PerspectiveProjection)
	{
		ErrPrintf(
			"pConfig->m_shadowProjType is %d; not a valid GFSDK_FaceWorks_ProjectionType enum value\n",
			pConfig->m_shadowProjType);
		return GFSDK_FaceWorks_InvalidArgument;
	}
	if (pConfig->m_shadowProjType != GFSDK_FaceWorks_NoProjection)
	{
		// Error checking for shadow parameters is done only if enabled by the projection type
		if (pConfig->m_shadowFilterRadius < 0.0f)
		{
			WarnPrintf(
				"pConfig->m_shadowFilterRadius is %g; should be at least 0.0\n",
				pConfig->m_shadowFilterRadius);
		}

		// Should we check that m_shadowProjMatrix is of the expected form
		// (e.g. has zeros in the expected places)?
	}

	return GFSDK_FaceWorks_OK;
}

GFSDK_FACEWORKS_API GFSDK_FaceWorks_Result GFSDK_FACEWORKS_CALLCONV GFSDK_FaceWorks_WriteCBDataForDeepScatter(
	const GFSDK_FaceWorks_DeepScatterConfig * pConfig,
	GFSDK_FaceWorks_CBData * pCBDataOut,
	GFSDK_FaceWorks_ErrorBlob * pErrorBlobOut)
{
	// Validate params

	GFSDK_FaceWorks_Result res = ValidateDeepScatterConfig(pConfig, pErrorBlobOut);
	if (res != GFSDK_FaceWorks_OK)
		return res;

	if (!pCBDataOut)
	{
		ErrPrintf("pCBDataOut is null\n");
		return GFSDK_FaceWorks_InvalidArgument;
	}

	// Set up the constant buffer

	// -0.7213475f = -1 / (2 * ln(2)) - this conversion lets us write this in the shader:
	//     exp2(deepScatterFalloff * thickness^2)
	// instead of this:
	//     exp(-thickness^2 / (2 * radius^2))
	float deepScatterFalloff = -0.7213475f / (pConfig->m_radius * pConfig->m_radius);

	// Calculate depth-decoding parameters from the projection matrix
	float decodeDepthScale = 0.0f;
	float decodeDepthBias = 0.0f;
	switch (pConfig->m_shadowProjType)
	{
	case GFSDK_FaceWorks_NoProjection:
		break;	// Nothing to do

	case GFSDK_FaceWorks_ParallelProjection:
		decodeDepthScale = -1.0f / pConfig->m_shadowProjMatrix._33;
		break;

	case GFSDK_FaceWorks_PerspectiveProjection:
		decodeDepthScale = pConfig->m_shadowProjMatrix._34 / pConfig->m_shadowProjMatrix._43;
		decodeDepthBias = -pConfig->m_shadowProjMatrix._33 / pConfig->m_shadowProjMatrix._43;
		break;

	default:
		assert(false);
		return GFSDK_FaceWorks_InvalidArgument;
	}

	// Output to user buffer
	pCBDataOut->data[1].y = deepScatterFalloff;
	pCBDataOut->data[1].z = pConfig->m_shadowFilterRadius;
	pCBDataOut->data[1].w = decodeDepthScale;
	pCBDataOut->data[2].x = decodeDepthBias;

	return GFSDK_FaceWorks_OK;
}
