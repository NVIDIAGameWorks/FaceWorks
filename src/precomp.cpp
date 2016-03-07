//----------------------------------------------------------------------------------
// File:        FaceWorks/src/precomp.cpp
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

#include <cstdio>
#include <vector>



// Versioning

GFSDK_FACEWORKS_API int GFSDK_FACEWORKS_CALLCONV GFSDK_FaceWorks_GetBinaryVersion()
{
	// Capture the header version at time of compilation
	return GFSDK_FaceWorks_HeaderVersion;
}

GFSDK_FACEWORKS_API const char * GFSDK_FACEWORKS_CALLCONV GFSDK_FaceWorks_GetBuildInfo()
{
#define STRINGIZE2(x) #x
#define STRINGIZE(x) STRINGIZE2(x)

	return 
		"GFSDK_FaceWorks_HeaderVersion: " STRINGIZE(GFSDK_FaceWorks_HeaderVersion) "\n"
		"Built on: " __DATE__ " " __TIME__ "\n"

#if defined(_MSC_VER)
		"Compiler: Microsoft Visual C++\n"
		"_MSC_VER: " STRINGIZE(_MSC_VER) "\n"
#else
		"Compiler: unknown\n"
#endif

#if defined(_WIN64)
		"Platform: Win64\n"
#elif defined(_WIN32)
		"Platform: Win32\n"
#else
		"Platform: unknown\n"
#endif

#if defined(_DEBUG)
		"Configuration: Debug\n"
#else
		"Configuration: Release\n"
#endif
		;

#undef STRINGIZE
#undef STRINGIZE2
}

static const float pi = 3.141592654f;

// Initialization

GFSDK_FACEWORKS_API GFSDK_FaceWorks_Result GFSDK_FACEWORKS_CALLCONV GFSDK_FaceWorks_Init_Internal(int headerVersion)
{
	if (headerVersion != GFSDK_FaceWorks_GetBinaryVersion())
		return GFSDK_FaceWorks_VersionMismatch;

	return GFSDK_FaceWorks_OK;
}



// Error blob helper functions

void BlobPrintf(GFSDK_FaceWorks_ErrorBlob * pBlob, const char * fmt, ...)
{
	if (!pBlob)
		return;

	// Printf the message - just use a fixed-size buffer to simplify things
	char newMsg[256];
	va_list args;
	va_start(args, fmt);
	_vsnprintf_s(newMsg, dim(newMsg), _TRUNCATE, fmt, args);
	size_t newLen = strlen(newMsg);

	// Append the message to the blob
	if (pBlob->m_msg)
	{
		size_t curLen = strlen(pBlob->m_msg);
		size_t bytes = curLen + newLen + 1;
		char * concat = static_cast<char *>(FaceWorks_Malloc(bytes, pBlob->m_allocator));
		if (!concat)
		{
			// Out of memory while generating an error message - just give up
			return;
		}
		memcpy(concat, pBlob->m_msg, curLen);
		memcpy(concat + curLen, newMsg, newLen + 1);
		FaceWorks_Free(pBlob->m_msg, pBlob->m_allocator);
		pBlob->m_msg = concat;
	}
	else
	{
		size_t bytes = newLen + 1;
		pBlob->m_msg = static_cast<char *>(FaceWorks_Malloc(bytes, pBlob->m_allocator));
		if (!pBlob->m_msg)
		{
			// Out of memory while generating an error message - just give up
			return;
		}
		memcpy(pBlob->m_msg, newMsg, bytes);
	}
}

GFSDK_FACEWORKS_API void GFSDK_FACEWORKS_CALLCONV GFSDK_FaceWorks_FreeErrorBlob(
	GFSDK_FaceWorks_ErrorBlob * pBlob)
{
	if (!pBlob)
		return;

	FaceWorks_Free(pBlob->m_msg, pBlob->m_allocator);
	pBlob->m_msg = nullptr;
}



GFSDK_FACEWORKS_API size_t GFSDK_FACEWORKS_CALLCONV GFSDK_FaceWorks_CalculateCurvatureSizeBytes(int vertexCount)
{
	return sizeof(float) * max(0, vertexCount);
}

GFSDK_FACEWORKS_API GFSDK_FaceWorks_Result GFSDK_FACEWORKS_CALLCONV GFSDK_FaceWorks_CalculateMeshCurvature(
	int vertexCount,
	const void * pPositions,
	int positionStrideBytes,
	const void * pNormals,
	int normalStrideBytes,
	int indexCount,
	const int * pIndices,
	int smoothingPassCount,
	void * pCurvaturesOut,
	int curvatureStrideBytes,
	GFSDK_FaceWorks_ErrorBlob * pErrorBlobOut,
	gfsdk_new_delete_t * pAllocator /*= 0*/)
{
	// Validate parameters
	if (vertexCount < 1)
	{
		ErrPrintf("vertexCount is %d; should be at least 1\n", vertexCount);
		return GFSDK_FaceWorks_InvalidArgument;
	}
	if (!pPositions)
	{
		ErrPrintf("pPositions is null\n");
		return GFSDK_FaceWorks_InvalidArgument;
	}
	if (positionStrideBytes < 3 * int(sizeof(float)))
	{
		ErrPrintf("positionStrideBytes is %d; should be at least %d\n",
			positionStrideBytes, 3 * sizeof(float));
		return GFSDK_FaceWorks_InvalidArgument;
	}
	if (!pNormals)
	{
		ErrPrintf("pNormals is null\n");
		return GFSDK_FaceWorks_InvalidArgument;
	}
	if (normalStrideBytes < 3 * int(sizeof(float)))
	{
		ErrPrintf("normalStrideBytes is %d; should be at least %d\n",
			normalStrideBytes, 3 * sizeof(float));
		return GFSDK_FaceWorks_InvalidArgument;
	}
	if (indexCount < 3)
	{
		ErrPrintf("indexCount is %d; should be at least 3\n", indexCount);
		return GFSDK_FaceWorks_InvalidArgument;
	}
	if (!pIndices)
	{
		ErrPrintf("pIndices is null\n");
		return GFSDK_FaceWorks_InvalidArgument;
	}
	if (smoothingPassCount < 0)
	{
		ErrPrintf("smoothingPassCount is %d; should be at least 0\n", smoothingPassCount);
		return GFSDK_FaceWorks_InvalidArgument;
	}
	if (!pCurvaturesOut)
	{
		ErrPrintf("pCurvaturesOut is null\n");
		return GFSDK_FaceWorks_InvalidArgument;
	}
	if (curvatureStrideBytes < int(sizeof(float)))
	{
		ErrPrintf("curvatureStrideBytes is %d; should be at least %d\n",
			curvatureStrideBytes, sizeof(float));
		return GFSDK_FaceWorks_InvalidArgument;
	}

	// Calculate per-vertex curvature.  We do this by estimating the curvature along each
	// edge using the change in normals between its vertices; then we set each vertex's
	// curvature to the midpoint of the minimum and maximum over all the edges touching it.

	int triCount = indexCount / 3;

	// Catch out-of-memory exceptions
	try
	{
		FaceWorks_Allocator<float> allocFloat(pAllocator);
		std::vector<float, FaceWorks_Allocator<float>> curvatureMin(vertexCount, FLT_MAX, allocFloat);
		std::vector<float, FaceWorks_Allocator<float>> curvatureMax(vertexCount, 0.0f, allocFloat);

		// !!!UNDONE: SIMD-ize or GPU-ize all this math

		for (int iTri = 0; iTri < triCount; ++iTri)
		{
			int indices[] =
			{
				pIndices[3*iTri],
				pIndices[3*iTri + 1],
				pIndices[3*iTri + 2],
			};

			float * pos[] =
			{
				reinterpret_cast<float *>((char *)pPositions + indices[0] * positionStrideBytes),
				reinterpret_cast<float *>((char *)pPositions + indices[1] * positionStrideBytes),
				reinterpret_cast<float *>((char *)pPositions + indices[2] * positionStrideBytes),
			};

			float * normal[] =
			{
				reinterpret_cast<float *>((char *)pNormals + indices[0] * normalStrideBytes),
				reinterpret_cast<float *>((char *)pNormals + indices[1] * normalStrideBytes),
				reinterpret_cast<float *>((char *)pNormals + indices[2] * normalStrideBytes),
			};

			// Calculate each edge's curvature - most edges will be calculated twice this
			// way, but it's hard to fix that while still making sure to handle boundary edges.

			float dPx = pos[1][0] - pos[0][0];
			float dPy = pos[1][1] - pos[0][1];
			float dPz = pos[1][2] - pos[0][2];
			float dNx = normal[1][0] - normal[0][0];
			float dNy = normal[1][1] - normal[0][1];
			float dNz = normal[1][2] - normal[0][2];
			float curvature = sqrtf((dNx*dNx + dNy*dNy + dNz*dNz) / (dPx*dPx + dPy*dPy + dPz*dPz));
			curvatureMin[indices[0]] = min(curvatureMin[indices[0]], curvature);
			curvatureMin[indices[1]] = min(curvatureMin[indices[1]], curvature);
			curvatureMax[indices[0]] = max(curvatureMax[indices[0]], curvature);
			curvatureMax[indices[1]] = max(curvatureMax[indices[1]], curvature);

			dPx = pos[2][0] - pos[1][0];
			dPy = pos[2][1] - pos[1][1];
			dPz = pos[2][2] - pos[1][2];
			dNx = normal[2][0] - normal[1][0];
			dNy = normal[2][1] - normal[1][1];
			dNz = normal[2][2] - normal[1][2];
			curvature = sqrtf((dNx*dNx + dNy*dNy + dNz*dNz) / (dPx*dPx + dPy*dPy + dPz*dPz));
			curvatureMin[indices[1]] = min(curvatureMin[indices[1]], curvature);
			curvatureMin[indices[2]] = min(curvatureMin[indices[2]], curvature);
			curvatureMax[indices[1]] = max(curvatureMax[indices[1]], curvature);
			curvatureMax[indices[2]] = max(curvatureMax[indices[2]], curvature);

			dPx = pos[0][0] - pos[2][0];
			dPy = pos[0][1] - pos[2][1];
			dPz = pos[0][2] - pos[2][2];
			dNx = normal[0][0] - normal[2][0];
			dNy = normal[0][1] - normal[2][1];
			dNz = normal[0][2] - normal[2][2];
			curvature = sqrtf((dNx*dNx + dNy*dNy + dNz*dNz) / (dPx*dPx + dPy*dPy + dPz*dPz));
			curvatureMin[indices[2]] = min(curvatureMin[indices[2]], curvature);
			curvatureMin[indices[0]] = min(curvatureMin[indices[0]], curvature);
			curvatureMax[indices[2]] = max(curvatureMax[indices[2]], curvature);
			curvatureMax[indices[0]] = max(curvatureMax[indices[0]], curvature);
		}

		for (int i = 0; i < vertexCount; ++i)
		{
			float * pCurvature = reinterpret_cast<float *>((char *)pCurvaturesOut + i * curvatureStrideBytes);
			*pCurvature = 0.5f * (curvatureMin[i] + curvatureMax[i]);
		}
	}
	catch (std::bad_alloc)
	{
		return GFSDK_FaceWorks_OutOfMemory;
	}

	if (smoothingPassCount > 0)
	{
		// Catch out-of-memory exceptions
		try
		{
			FaceWorks_Allocator<float> allocFloat(pAllocator);
			std::vector<float, FaceWorks_Allocator<float>> curvatureSum(allocFloat);
			curvatureSum.resize(vertexCount);

			FaceWorks_Allocator<int> allocInt(pAllocator);
			std::vector<int, FaceWorks_Allocator<int>> curvatureCount(allocInt);
			curvatureCount.resize(vertexCount);

			// Run a couple of smoothing passes, replacing each vert's curvature
			// by the average of its neighbors'

			for (int iPass = 0; iPass < smoothingPassCount; ++iPass)
			{
				for (int i = 0; i < vertexCount; ++i)
				{
					curvatureSum[i] = 0.0f;
					curvatureCount[i] = 0;
				}

				for (int iTri = 0; iTri < triCount; ++iTri)
				{
					int indices[] =
					{
						pIndices[3*iTri],
						pIndices[3*iTri + 1],
						pIndices[3*iTri + 2],
					};

					float curvature0 = *reinterpret_cast<float *>((char *)pCurvaturesOut + indices[0] * curvatureStrideBytes);
					float curvature1 = *reinterpret_cast<float *>((char *)pCurvaturesOut + indices[1] * curvatureStrideBytes);
					float curvature2 = *reinterpret_cast<float *>((char *)pCurvaturesOut + indices[2] * curvatureStrideBytes);

					curvatureSum[indices[0]] += curvature1 + curvature2;
					curvatureCount[indices[0]] += 2;

					curvatureSum[indices[1]] += curvature2 + curvature0;
					curvatureCount[indices[1]] += 2;

					curvatureSum[indices[2]] += curvature0 + curvature1;
					curvatureCount[indices[2]] += 2;
				}

				for (int i = 0; i < vertexCount; ++i)
				{
					float * pCurvature = reinterpret_cast<float *>((char *)pCurvaturesOut + i * curvatureStrideBytes);
					*pCurvature = curvatureSum[i] / float(max(1, curvatureCount[i]));
				}
			}
		}
		catch (std::bad_alloc)
		{
			return GFSDK_FaceWorks_OutOfMemory;
		}
	}

	return GFSDK_FaceWorks_OK;
}



GFSDK_FACEWORKS_API GFSDK_FaceWorks_Result GFSDK_FACEWORKS_CALLCONV GFSDK_FaceWorks_CalculateMeshUVScale(
	int vertexCount,
	const void * pPositions,
	int positionStrideBytes,
	const void * pUVs,
	int uvStrideBytes,
	int indexCount,
	const int * pIndices,
	float * pAverageUVScaleOut,
	GFSDK_FaceWorks_ErrorBlob * pErrorBlobOut)
{
	// Validate parameters
	if (vertexCount < 1)
	{
		ErrPrintf("vertexCount is %d; should be at least 1\n", vertexCount);
		return GFSDK_FaceWorks_InvalidArgument;
	}
	if (!pPositions)
	{
		ErrPrintf("pPositions is null\n");
		return GFSDK_FaceWorks_InvalidArgument;
	}
	if (positionStrideBytes < 3 * int(sizeof(float)))
	{
		ErrPrintf("positionStrideBytes is %d; should be at least %d\n",
			positionStrideBytes, 3 * sizeof(float));
		return GFSDK_FaceWorks_InvalidArgument;
	}
	if (!pUVs)
	{
		ErrPrintf("pUVs is null\n");
		return GFSDK_FaceWorks_InvalidArgument;
	}
	if (uvStrideBytes < 2 * int(sizeof(float)))
	{
		ErrPrintf("uvStrideBytes is %d; should be at least %d\n",
			uvStrideBytes, 2 * sizeof(float));
		return GFSDK_FaceWorks_InvalidArgument;
	}
	if (indexCount < 3)
	{
		ErrPrintf("indexCount is %d; should be at least 3\n", indexCount);
		return GFSDK_FaceWorks_InvalidArgument;
	}
	if (indexCount % 3 != 0)
	{
		ErrPrintf("indexCount is %d; should be a multiple of 3\n", indexCount);
		return GFSDK_FaceWorks_InvalidArgument;
	}
	if (!pIndices)
	{
		ErrPrintf("pIndices is null\n");
		return GFSDK_FaceWorks_InvalidArgument;
	}
	if (!pAverageUVScaleOut)
	{
		ErrPrintf("pAverageUVScaleOut is null\n");
		return GFSDK_FaceWorks_InvalidArgument;
	}

	// Calculate average UV scale, as a geometric mean of scale for each triangle

	float logUvScaleSum = 0.0f;
	int logUvScaleCount = 0;

	// !!!UNDONE: SIMD-ize or GPU-ize all this math

	for (int iIndex = 0; iIndex < indexCount; iIndex += 3)
	{
		int indices[] =
		{
			pIndices[iIndex],
			pIndices[iIndex + 1],
			pIndices[iIndex + 2],
		};

		float * pos[] =
		{
			reinterpret_cast<float *>((char *)pPositions + indices[0] * positionStrideBytes),
			reinterpret_cast<float *>((char *)pPositions + indices[1] * positionStrideBytes),
			reinterpret_cast<float *>((char *)pPositions + indices[2] * positionStrideBytes),
		};

		float * uv[] =
		{
			reinterpret_cast<float *>((char *)pUVs + indices[0] * uvStrideBytes),
			reinterpret_cast<float *>((char *)pUVs + indices[1] * uvStrideBytes),
			reinterpret_cast<float *>((char *)pUVs + indices[2] * uvStrideBytes),
		};

		// Find longest edge length in local space
		float dP0x = pos[1][0] - pos[0][0];
		float dP0y = pos[1][1] - pos[0][1];
		float dP0z = pos[1][2] - pos[0][2];
		float dP1x = pos[2][0] - pos[1][0];
		float dP1y = pos[2][1] - pos[1][1];
		float dP1z = pos[2][2] - pos[1][2];
		float dP2x = pos[0][0] - pos[2][0];
		float dP2y = pos[0][1] - pos[2][1];
		float dP2z = pos[0][2] - pos[2][2];
		float diameter = sqrtf(max(dP0x*dP0x + dP0y*dP0y + dP0z*dP0z, 
							   max(dP1x*dP1x + dP1y*dP1y + dP1z*dP1z,
								   dP2x*dP2x + dP2y*dP2y + dP2z*dP2z)));

		// Find longest edge length in UV space
		float dUV0x = uv[1][0] - uv[0][0];
		float dUV0y = uv[1][1] - uv[0][1];
		float dUV1x = uv[2][0] - uv[1][0];
		float dUV1y = uv[2][1] - uv[1][1];
		float dUV2x = uv[0][0] - uv[2][0];
		float dUV2y = uv[0][1] - uv[2][1];
		float uvDiameter = sqrtf(max(dUV0x*dUV0x + dUV0y*dUV0y, 
								 max(dUV1x*dUV1x + dUV1y*dUV1y,
									 dUV2x*dUV2x + dUV2y*dUV2y)));

		// Skip degenerate triangles
		if (diameter < 1e-6f || uvDiameter < 1e-6f)
			continue;

		float triUvScale = diameter / uvDiameter;
		logUvScaleSum += logf(triUvScale);
		++logUvScaleCount;
	}

	*pAverageUVScaleOut = expf(logUvScaleSum / float(logUvScaleCount));

	return GFSDK_FaceWorks_OK;
}



// Diffusion profile from GPU Gems 3 - mixture of 6 Gaussians with RGB weights
// NOTE: could switch to a LUT generated using one of the Donner and Jensen papers

static const float diffusionSigmas[] = { 0.080f, 0.220f, 0.432f, 0.753f, 1.411f, 2.722f };
static const float diffusionWeightsR[] = { 0.233f, 0.100f, 0.118f, 0.113f, 0.358f, 0.078f };
static const float diffusionWeightsG[] = { 0.455f, 0.336f, 0.198f, 0.007f, 0.004f, 0.000f };
static const float diffusionWeightsB[] = { 0.649f, 0.344f, 0.000f, 0.007f, 0.000f, 0.000f };


static_assert(dim(diffusionWeightsR) == dim(diffusionSigmas), "dimension mismatch between array diffusionWeightsR and diffusionSigmas");
static_assert(dim(diffusionWeightsG) == dim(diffusionSigmas), "dimension mismatch between array diffusionWeightsG and diffusionSigmas");
static_assert(dim(diffusionWeightsB) == dim(diffusionSigmas), "dimension mismatch between array diffusionWeightsB and diffusionSigmas");

inline float Gaussian(float sigma, float x)
{
	static const float rsqrtTwoPi = 0.39894228f;
	return (rsqrtTwoPi / sigma) * expf(-0.5f * (x*x) / (sigma*sigma));
}

static void EvaluateDiffusionProfile(float x, float rgb[3])	// x in millimeters
{
	rgb[0] = 0.0f;
	rgb[1] = 0.0f;
	rgb[2] = 0.0f;

	for (int i = 0; i < dim(diffusionSigmas); ++i)
	{
		static const float rsqrtTwoPi = 0.39894228f;
		float sigma = diffusionSigmas[i];
		float gaussian = (rsqrtTwoPi / sigma) * expf(-0.5f * (x*x) / (sigma*sigma));

		rgb[0] += diffusionWeightsR[i] * gaussian;
		rgb[1] += diffusionWeightsG[i] * gaussian;
		rgb[2] += diffusionWeightsB[i] * gaussian;
	}
}

GFSDK_FACEWORKS_API size_t GFSDK_FACEWORKS_CALLCONV GFSDK_FaceWorks_CalculateCurvatureLUTSizeBytes(
	const GFSDK_FaceWorks_CurvatureLUTConfig * pConfig)
{
	if (!pConfig)
		return 0;

	return 4 * pConfig->m_texWidth * pConfig->m_texHeight;
}

GFSDK_FACEWORKS_API GFSDK_FaceWorks_Result GFSDK_FACEWORKS_CALLCONV GFSDK_FaceWorks_GenerateCurvatureLUT(
	const GFSDK_FaceWorks_CurvatureLUTConfig * pConfig,
	void * pCurvatureLUTOut,
	GFSDK_FaceWorks_ErrorBlob * pErrorBlobOut)
{
	// Validate parameters
	if (!pConfig)
	{
		ErrPrintf("pConfig is null\n");
		return GFSDK_FaceWorks_InvalidArgument;
	}
	if (!pCurvatureLUTOut)
	{
		ErrPrintf("pCurvatureLUTOut is null\n");
		return GFSDK_FaceWorks_InvalidArgument;
	}
	if (pConfig->m_diffusionRadius <= 0.0f)
	{
		ErrPrintf("m_diffusionRadius is %g; should be greater than 0\n",
			pConfig->m_diffusionRadius);
		return GFSDK_FaceWorks_InvalidArgument;
	}
	if (pConfig->m_texWidth < 1)
	{
		ErrPrintf("m_texWidth is %d; should be at least 1\n",
			pConfig->m_texWidth);
		return GFSDK_FaceWorks_InvalidArgument;
	}
	if (pConfig->m_texHeight < 1)
	{
		ErrPrintf("m_texHeight is %d; should be at least 1\n",
			pConfig->m_texHeight);
		return GFSDK_FaceWorks_InvalidArgument;
	}
	if (pConfig->m_curvatureRadiusMin <= 0.0f)
	{
		ErrPrintf("m_curvatureRadiusMin is %g; should be greater than 0\n",
			pConfig->m_curvatureRadiusMin);
		return GFSDK_FaceWorks_InvalidArgument;
	}
	if (pConfig->m_curvatureRadiusMax <= 0.0f)
	{
		ErrPrintf("m_curvatureRadiusMax is %g; should be greater than 0\n",
			pConfig->m_curvatureRadiusMax);
		return GFSDK_FaceWorks_InvalidArgument;
	}
	if (pConfig->m_curvatureRadiusMax < pConfig->m_curvatureRadiusMin)
	{
		ErrPrintf("m_curvatureRadiusMin is %g and m_curvatureRadiusMax is %g; max should be greater than min\n",
			pConfig->m_curvatureRadiusMin, pConfig->m_curvatureRadiusMax);
		return GFSDK_FaceWorks_InvalidArgument;
	}
	
	// The diffusion profile is built assuming a (standard human skin) radius
	// of 2.7 mm, so the curvatures and shadow widths need to be scaled to generate
	// a LUT for the user's desired diffusion radius.
	float diffusionRadiusFactor = pConfig->m_diffusionRadius / 2.7f;

	float curvatureMin = diffusionRadiusFactor / pConfig->m_curvatureRadiusMax;
	float curvatureMax = diffusionRadiusFactor / pConfig->m_curvatureRadiusMin;
	float curvatureScale = (curvatureMax - curvatureMin) / float(pConfig->m_texHeight);
	float curvatureBias = curvatureMin + 0.5f * curvatureScale;

	float NdotLScale = 2.0f / float(pConfig->m_texWidth);
	float NdotLBias = -1.0f + 0.5f * NdotLScale;

	unsigned char * pPx = static_cast<unsigned char *>(pCurvatureLUTOut);

	// !!!UNDONE: SIMD-ize or GPU-ize all this math

	for (int iY = 0; iY < pConfig->m_texHeight; ++iY)
	{
		for (int iX = 0; iX < pConfig->m_texWidth; ++iX)
		{
			float NdotL = float(iX) * NdotLScale + NdotLBias;
			float theta = acosf(NdotL);

			float curvature = float(iY) * curvatureScale + curvatureBias;
			float radius = 1.0f / curvature;

			// Sample points around a ring, and Monte-Carlo-integrate the
			// scattered lighting using the diffusion profile

			static const int cIter = 200;
			float rgb[3] = { 0.0f, 0.0f, 0.0f };

			// Set integration bounds in arc-length in mm on the sphere
			float lowerBound = max(-pi*radius, -10.0f);
			float upperBound = min(pi*radius, 10.0f);

			float iterScale = (upperBound - lowerBound) / float(cIter);
			float iterBias = lowerBound + 0.5f * iterScale;

			for (int iIter = 0; iIter < cIter; ++iIter)
			{
				float delta = float(iIter) * iterScale + iterBias;
				float rgbDiffusion[3];
				EvaluateDiffusionProfile(delta, rgbDiffusion);

				float NdotLDelta = max(0.0f, cosf(theta - delta * curvature));
				rgb[0] += NdotLDelta * rgbDiffusion[0];
				rgb[1] += NdotLDelta * rgbDiffusion[1];
				rgb[2] += NdotLDelta * rgbDiffusion[2];
			}

			// Scale sum of samples to get value of integral
			float scale = (upperBound - lowerBound) / float(cIter);
			rgb[0] *= scale;
			rgb[1] *= scale;
			rgb[2] *= scale;

			// Calculate delta from standard diffuse lighting (saturate(N.L)) to
			// scattered result, remapped from [-.25, .25] to [0, 1].
			float rgbAdjust = -max(0.0f, NdotL) * 2.0f + 0.5f;
			rgb[0] = rgb[0] * 2.0f + rgbAdjust;
			rgb[1] = rgb[1] * 2.0f + rgbAdjust;
			rgb[2] = rgb[2] * 2.0f + rgbAdjust;

			// Clamp to [0, 1]
			rgb[0] = min(max(rgb[0], 0.0f), 1.0f);
			rgb[1] = min(max(rgb[1], 0.0f), 1.0f);
			rgb[2] = min(max(rgb[2], 0.0f), 1.0f);

			// Convert to integer format (linear RGB space)
			*(pPx++) = static_cast<unsigned char>(255.0f * rgb[0] + 0.5f);
			*(pPx++) = static_cast<unsigned char>(255.0f * rgb[1] + 0.5f);
			*(pPx++) = static_cast<unsigned char>(255.0f * rgb[2] + 0.5f);
			*(pPx++) = 255;
		}
	}

	return GFSDK_FaceWorks_OK;
}

GFSDK_FACEWORKS_API size_t GFSDK_FACEWORKS_CALLCONV GFSDK_FaceWorks_CalculateShadowLUTSizeBytes(
	const GFSDK_FaceWorks_ShadowLUTConfig * pConfig)
{
	if (!pConfig)
		return 0;

	return 4 * pConfig->m_texWidth * pConfig->m_texHeight;
}

GFSDK_FACEWORKS_API GFSDK_FaceWorks_Result GFSDK_FACEWORKS_CALLCONV GFSDK_FaceWorks_GenerateShadowLUT(
	const GFSDK_FaceWorks_ShadowLUTConfig * pConfig,
	void * pShadowLUTOut,
	GFSDK_FaceWorks_ErrorBlob * pErrorBlobOut)
{
	if (!pConfig)
	{
		ErrPrintf("pConfig is null\n");
		return GFSDK_FaceWorks_InvalidArgument;
	}
	if (!pShadowLUTOut)
	{
		ErrPrintf("pShadowLUTOut is null\n");
		return GFSDK_FaceWorks_InvalidArgument;
	}
	if (pConfig->m_diffusionRadius <= 0.0f)
	{
		ErrPrintf("m_diffusionRadius is %g; should be greater than 0\n",
			pConfig->m_diffusionRadius);
		return GFSDK_FaceWorks_InvalidArgument;
	}
	if (pConfig->m_texWidth < 1)
	{
		ErrPrintf("m_texWidth is %d; should be at least 1\n",
			pConfig->m_texWidth);
		return GFSDK_FaceWorks_InvalidArgument;
	}
	if (pConfig->m_texHeight < 1)
	{
		ErrPrintf("m_texHeight is %d; should be at least 1\n",
			pConfig->m_texHeight);
		return GFSDK_FaceWorks_InvalidArgument;
	}
	if (pConfig->m_shadowWidthMin <= 0.0f)
	{
		ErrPrintf("m_shadowWidthMin is %g; should be greater than 0\n",
			pConfig->m_shadowWidthMin);
		return GFSDK_FaceWorks_InvalidArgument;
	}
	if (pConfig->m_shadowWidthMax <= 0.0f)
	{
		ErrPrintf("m_shadowWidthMax is %g; should be greater than 0\n",
			pConfig->m_shadowWidthMax);
		return GFSDK_FaceWorks_InvalidArgument;
	}
	if (pConfig->m_shadowWidthMax < pConfig->m_shadowWidthMin)
	{
		ErrPrintf("m_shadowWidthMin is %g and m_shadowWidthMax is %g; max should be greater than min\n",
			pConfig->m_shadowWidthMin, pConfig->m_shadowWidthMax);
		return GFSDK_FaceWorks_InvalidArgument;
	}
	if (pConfig->m_shadowSharpening < 1.0f)
	{
		ErrPrintf("m_shadowSharpening is %g; should be at least 1.0\n",
			pConfig->m_shadowSharpening);
		return GFSDK_FaceWorks_InvalidArgument;
	}

	// The diffusion profile is built assuming a (standard human skin) radius
	// of 2.7 mm, so the curvatures and shadow widths need to be scaled to generate
	// a LUT for the user's desired diffusion radius.
	float diffusionRadiusFactor = pConfig->m_diffusionRadius / 2.7f;

	float shadowRcpWidthMin = diffusionRadiusFactor / pConfig->m_shadowWidthMax;
	float shadowRcpWidthMax = diffusionRadiusFactor / pConfig->m_shadowWidthMin;
	float shadowScale = (shadowRcpWidthMax - shadowRcpWidthMin) / float(pConfig->m_texHeight);
	float shadowBias = shadowRcpWidthMin + 0.5f * shadowScale;

	unsigned char * pPx = static_cast<unsigned char *>(pShadowLUTOut);

	// !!!UNDONE: SIMD-ize or GPU-ize all this math

	for (int iY = 0; iY < pConfig->m_texHeight; ++iY)
	{
		for (int iX = 0; iX < pConfig->m_texWidth; ++iX)
		{
			// Calculate input position relative to the shadow edge, by approximately
			// inverting the transfer function of a disc or Gaussian filter.
			float u = (iX + 0.5f) / float(pConfig->m_texWidth);
			float inputPos = (sqrtf(u) - sqrtf(1.0f - u)) * 0.5f + 0.5f;

			float rcpWidth = float(iY) * shadowScale + shadowBias;

			// Sample points along a line perpendicular to the shadow edge, and
			// Monte-Carlo-integrate the scattered lighting using the diffusion profile

			static const int cIter = 200;
			float rgb[3] = { 0.0f, 0.0f, 0.0f };

			float iterScale = 20.0f / float(cIter);
			float iterBias = -10.0f + 0.5f * iterScale;

			for (int iIter = 0; iIter < cIter; ++iIter)
			{
				float delta = float(iIter) * iterScale + iterBias;
				float rgbDiffusion[3];
				EvaluateDiffusionProfile(delta, rgbDiffusion);

				// Use smoothstep as an approximation of the transfer function of a
				// disc or Gaussian filter.
				float newPos = (inputPos + delta * rcpWidth) * pConfig->m_shadowSharpening +
											(-0.5f * pConfig->m_shadowSharpening + 0.5f);
				float newPosClamped = min(max(newPos, 0.0f), 1.0f);
				float newShadow = (3.0f - 2.0f * newPosClamped) * newPosClamped * newPosClamped;

				rgb[0] += newShadow * rgbDiffusion[0];
				rgb[1] += newShadow * rgbDiffusion[1];
				rgb[2] += newShadow * rgbDiffusion[2];
			}

			// Scale sum of samples to get value of integral.  Also hack in a
			// fade to ensure the left edge of the image goes strictly to zero.
			float scale = 20.0f / float(cIter);
			if (iX * 25 < pConfig->m_texWidth)
			{
				scale *= min(25.0f * float(iX) / float(pConfig->m_texWidth), 1.0f);
			}
			rgb[0] *= scale;
			rgb[1] *= scale;
			rgb[2] *= scale;

			// Clamp to [0, 1]
			rgb[0] = min(max(rgb[0], 0.0f), 1.0f);
			rgb[1] = min(max(rgb[1], 0.0f), 1.0f);
			rgb[2] = min(max(rgb[2], 0.0f), 1.0f);

			// Convert linear to sRGB
			rgb[0] = (rgb[0] < 0.0031308f) ? (12.92f * rgb[0]) : (1.055f * powf(rgb[0], 1.0f / 2.4f) - 0.055f);
			rgb[1] = (rgb[1] < 0.0031308f) ? (12.92f * rgb[1]) : (1.055f * powf(rgb[1], 1.0f / 2.4f) - 0.055f);
			rgb[2] = (rgb[2] < 0.0031308f) ? (12.92f * rgb[2]) : (1.055f * powf(rgb[2], 1.0f / 2.4f) - 0.055f);

			// Convert to integer format
			*(pPx++) = static_cast<unsigned char>(255.0f * rgb[0] + 0.5f);
			*(pPx++) = static_cast<unsigned char>(255.0f * rgb[1] + 0.5f);
			*(pPx++) = static_cast<unsigned char>(255.0f * rgb[2] + 0.5f);
			*(pPx++) = 255;
		}
	}

	return GFSDK_FaceWorks_OK;
}
