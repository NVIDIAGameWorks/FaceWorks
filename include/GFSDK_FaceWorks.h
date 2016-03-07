//----------------------------------------------------------------------------------
// File:        FaceWorks/include/GFSDK_FaceWorks.h
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

/// \file GFSDK_FaceWorks.h
/// NVIDIA FaceWorks
/// ----------------
/// v1.00
///
/// FaceWorks is a middleware library for high-quality skin rendering, inspired by the
/// techniques used in NVIDIA's "Digital Ira" demo.
/// (http://www.nvidia.com/coolstuff/demos#!/lifelike-human-face-rendering)
///
/// Digital Ira is a tech demo, created by NVIDIA in collaboration with Dr. Paul Debevec's team at USC,
/// in which a high-resolution head scan and performance capture are rendered as realistically as
/// possible in real-time, using every rendering technique the demo team could think of.  FaceWorks, in
/// contrast, aims to be a reusable, production-ready library, with the goal of enabling game developers
/// to match the rendering quality of Digital Ira.
///
/// FaceWorks currently provides two main features:
///
/// * A high-quality, efficient subsurface scattering solution, which supports both direct and ambient
///   light.
/// * An implementation of deep scattering (translucency) for direct light, based on estimating
///   thickness from a shadow map.
/// 
/// Currently, Direct3D 11 is the only API supported by FaceWorks.
///
/// For detailed documentation please see doc/html/ in the archive.
/// A sample application can be found in the samples/ subdirectory.


#ifndef GFSDK_FACEWORKS_H
#define GFSDK_FACEWORKS_H

#include <stddef.h>
#include <stdint.h>

#pragma pack(push, 8) // Make sure we have consistent structure packings

// =================================================================================
//	Preliminaries
// =================================================================================

#ifndef __GFSDK_COMMON_AUTOCONFIG
#	define __GFSDK_COMMON_AUTOCONFIG
#	ifdef __GNUC__
#		define __GFSDK_CC_GNU__ 1
#		define __GFSDK_CC_MSVC__ 0
#	else
#		define __GFSDK_CC_GNU__ 0
#		define __GFSDK_CC_MSVC__ 1
#	endif
#endif

#ifndef __GFSDK_COMMON_TYPES
#	define __GFSDK_COMMON_TYPES

	typedef unsigned char gfsdk_U8;
	typedef unsigned short gfsdk_U16;
	typedef signed int gfsdk_S32;
	typedef signed long long gfsdk_S64;
	typedef unsigned int gfsdk_U32;
	typedef unsigned long long gfsdk_U64;
	typedef float gfsdk_F32;

	typedef struct {
		gfsdk_F32 x;
		gfsdk_F32 y;
	} gfsdk_float2;

	typedef struct {
		gfsdk_F32 x;
		gfsdk_F32 y;
		gfsdk_F32 z;
	} gfsdk_float3;

	typedef struct {
		gfsdk_F32 x;
		gfsdk_F32 y;
		gfsdk_F32 z;
		gfsdk_F32 w;
	} gfsdk_float4;

	// implicit row major
	typedef struct {
		gfsdk_F32 _11, _12, _13, _14;
		gfsdk_F32 _21, _22, _23, _24;
		gfsdk_F32 _31, _32, _33, _34;
		gfsdk_F32 _41, _42, _43, _44;
	} gfsdk_float4x4;

	typedef bool                 gfsdk_bool;
	typedef char                 gfsdk_char;
	typedef const gfsdk_char*    gfsdk_cstr;
	typedef double               gfsdk_F64;

#endif // __GFSDK_COMMON_TYPES

#ifndef __GFSDK_COMMON_MACROS
#	define __GFSDK_COMMON_MACROS
// GNU
#	if __GFSDK_CC_GNU__
#		define __GFSDK_ALIGN__(bytes) __attribute__((aligned (bytes)))
#		define __GFSDK_EXPECT__(exp,tf) __builtin_expect(exp, tf)
#		define __GFSDK_INLINE__ __attribute__((always_inline))
#		define __GFSDK_NOLINE__ __attribute__((noinline))
#		define __GFSDK_RESTRICT__ __restrict
#		define __GFSDK_CDECL__
#		define __GFSDK_IMPORT__
#		define __GFSDK_EXPORT__
#		define __GFSDK_STDCALL__
#	endif

// MSVC
#	if __GFSDK_CC_MSVC__
#		define __GFSDK_ALIGN__(bytes) __declspec(align(bytes))
#		define __GFSDK_EXPECT__(exp, tf) (exp)
#		define __GFSDK_INLINE__ __forceinline
#		define __GFSDK_NOINLINE__
#		define __GFSDK_RESTRICT__ __restrict
#		define __GFSDK_CDECL__ __cdecl
#		define __GFSDK_IMPORT__ __declspec(dllimport)
#		define __GFSDK_EXPORT__ __declspec(dllexport)
#		define __GFSDK_STDCALL__ __stdcall
#	endif
#endif // __GFSDK_COMMON_MACROS

// Custom heap allocator
#ifndef __GFSDK_COMMON_CUSTOMHEAP
#	define __GFSDK_COMMON_CUSTOMHEAP

	typedef struct {
		void* (*new_)(size_t);
		void (*delete_)(void*);
	} gfsdk_new_delete_t;

#endif // __GFSDK_COMMON_CUSTOMHEAP

#define GFSDK_FACEWORKS_CALLCONV __GFSDK_CDECL__
#if defined(GFSDK_FACEWORKS_EXPORTS)
#	define GFSDK_FACEWORKS_API extern "C" __GFSDK_EXPORT__
#else
#	define GFSDK_FACEWORKS_API extern "C" __GFSDK_IMPORT__
#endif



// =================================================================================
//	Versioning
// =================================================================================

/// Header version number - used to check that the header matches the DLL.
#define GFSDK_FaceWorks_HeaderVersion 100

/// Retrieves the version number of the .dll actually being used
/// \return the binary version as an integer (version is multiplied by 100)
GFSDK_FACEWORKS_API int GFSDK_FACEWORKS_CALLCONV GFSDK_FaceWorks_GetBinaryVersion();

/// Retrive baked-in build info string (branch, date/time, versions, etc.)
///
/// \return a null-terminated char string containing the build information
GFSDK_FACEWORKS_API const char * GFSDK_FACEWORKS_CALLCONV GFSDK_FaceWorks_GetBuildInfo();



// =================================================================================
//	Error handling
// =================================================================================

/// Error codes for return values.
typedef enum
{
	GFSDK_FaceWorks_OK,						///< Everything ok
	GFSDK_FaceWorks_InvalidArgument,		///< A required argument is NULL, or not in the valid range
	GFSDK_FaceWorks_OutOfMemory,			///< Couldn't allocate memory
	GFSDK_FaceWorks_VersionMismatch,		///< Header version doesn't match DLL version
} GFSDK_FaceWorks_Result;

/// \brief Error blob, for returning verbose error messages.
/// \details Functions that take/return an error blob will allocate storage for the error message if
/// necessary, using the custom allocator if present, and fill in m_msg.
typedef struct
{
	gfsdk_new_delete_t	m_allocator;		///< [in] Custom allocator (fill with nullptr if not used)
	char *				m_msg;				///< [out] Null-terminated error message
} GFSDK_FaceWorks_ErrorBlob;

/// Free the storage for the message in an error blob
///
/// \param pBlob the error blob object
GFSDK_FACEWORKS_API void GFSDK_FACEWORKS_CALLCONV GFSDK_FaceWorks_FreeErrorBlob(GFSDK_FaceWorks_ErrorBlob * pBlob);



// =================================================================================
//	Initialization
// =================================================================================

/// Internal initialization function. Should not be called directly.
GFSDK_FACEWORKS_API GFSDK_FaceWorks_Result GFSDK_FACEWORKS_CALLCONV GFSDK_FaceWorks_Init_Internal(int headerVersion);

/// Initialize FaceWorks, and checks that the header version matches the DLL version.
///
/// \return						GFSDK_FaceWorks_OK if binary and header version does not match and the library has been
///								initialized properly, or GFSDK_FaceWorks_Mismatch if versions does not match
inline GFSDK_FaceWorks_Result GFSDK_FaceWorks_Init() {
	return GFSDK_FaceWorks_Init_Internal(GFSDK_FaceWorks_HeaderVersion);
}



// =================================================================================
//	Building mesh data for SSS
// =================================================================================

/// Calculate size needed to store curvatures generated by GFSDK_FaceWorks_CalculateMeshCurvature
/// Assuming they're not interleaved with any other vertex components
///
/// \param vertexCount			[in] number of vertices
///
/// \return						the size to store curvatures generated by GFSDK_FaceWorks_CalculateMeshCurvature, or zero if vertexCount is negative
GFSDK_FACEWORKS_API size_t GFSDK_FACEWORKS_CALLCONV GFSDK_FaceWorks_CalculateCurvatureSizeBytes(int vertexCount);

/// Generate per-vertex curvature for SSS.
/// The positions and normals of the mesh are assumed to be in float3 format and the
/// curvature is written out as a single float per vertex.
/// Indices are assumed to be 32-bit ints.
/// One or more smoothing passes can also be done on the calculated curvatures.
///
/// \param vertexCount			[in] the vertex count
/// \param pPositions			[in] pointer to the positions (per-vertex)
/// \param positionStrideBytes	[in] distance, in bytes, between two positions in the pPosition buffer
/// \param pNormals				[in] pointer to the normals (per-vertex)
/// \param normalStrideBytes	[in] distance, in bytes, between two normals in the pNormal buffer
/// \param indexCount			[in] the index count
/// \param pIndices				[in] pointer to the indices buffer
/// \param smoothingPassCount	[in] number of smoothing passes applied to the curvatures
/// \param pCurvaturesOut		[out] pointer to the curvatures buffer (written by this function)
/// \param curvatureStrideBytes	[in] distance, in bytes, between two curvatures in the pCurvaturesOut
/// \param pErrorBlobOut		[in] buffer the error blob, where errors are stored.
///								Error messages will be stored in the error blob, if one is given;
///								if error messages are generated, use GFSDK_FaceWorks_FreeErrorBlob()
///								to free the storage.
///								The given allocator will be used to allocate temporary storage for
///								working data, if provided; if not, the standard CRT allocator will
///								be used.
///
/// \return						GFSDK_FaceWorks_OK if parameters are correct
/// 							GFSDK_FaceWorks_InvalidArgument if pConfig contains invalid values
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
												gfsdk_new_delete_t * pAllocator);

/// Calculate average UV scale.
/// The positions and UVs of the mesh are assumed to be in float3 and float2 format,
/// respectively.
/// Indices are assumed to be 32-bit ints.
///
/// \param vertexCount			[in] the vertex count
/// \param pPositions			[in] pointer to the positions (per-vertex)
/// \param positionStrideBytes	[in] distance, in bytes, between two positions in the pPosition buffer
/// \param pUVs					[in] pointer to the UV coordinates (per-vertex)
/// \param uvStrideBytes		[in] distance, in bytes, between two UV coordinates in the pUVs buffer
/// \param indexCount			[in] the index count
/// \param pIndices				[in] pointer to the indices buffer
/// \param pAverageUVScaleOut	[out] pointer to a float where the average UV scale will be stored
/// \param pErrorBlobOut		[in] buffer the error blob, where errors are stored.
///								Error messages will be stored in the error blob, if one is given;
///								if error messages are generated, use GFSDK_FaceWorks_FreeErrorBlob()
///								to free the storage.
///								The given allocator will be used to allocate temporary storage for
///								working data, if provided; if not, the standard CRT allocator will
///								be used.
///
/// \return						GFSDK_FaceWorks_OK if parameters are correct
/// 							GFSDK_FaceWorks_InvalidArgument if pConfig contains invalid values
GFSDK_FACEWORKS_API GFSDK_FaceWorks_Result GFSDK_FACEWORKS_CALLCONV GFSDK_FaceWorks_CalculateMeshUVScale(
												int vertexCount,
												const void * pPositions,
												int positionStrideBytes,
												const void * pUVs,
												int uvStrideBytes,
												int indexCount,
												const int * pIndices,
												float * pAverageUVScaleOut,
												GFSDK_FaceWorks_ErrorBlob * pErrorBlobOut);



// =================================================================================
//	Building lookup textures for SSS
// =================================================================================

/// \brief Parameters for building curvature lookup texture (LUT) for SSS.
typedef struct
{
	float		m_diffusionRadius;			///< Diffusion radius, in world units (= 2.7mm for human skin)
	int			m_texWidth;					///< Width of curvature LUT (typically 512)
	int			m_texHeight;				///< Height of curvature LUT (typically 512)
	float		m_curvatureRadiusMin;		///< Min radius of curvature used to build the LUT (typically ~0.1 cm min)
	float		m_curvatureRadiusMax;		///< Max radius of curvature used to build the LUT (typically ~10.0 cm max)
} GFSDK_FaceWorks_CurvatureLUTConfig;

/// Calculate size needed to store pixels of texture generated by GFSDK_FaceWorks_GenerateCurvatureLUT.
///
/// \param pConfig				[in] the parameters for building curvature lookup texture for SSS
///
/// \return						the size needed to store pixels of texture generated by
///								GFSDK_FaceWorks_GenerateCurvatureLUT
GFSDK_FACEWORKS_API size_t GFSDK_FACEWORKS_CALLCONV GFSDK_FaceWorks_CalculateCurvatureLUTSizeBytes(
								const GFSDK_FaceWorks_CurvatureLUTConfig * pConfig);

/// Generate curvature lookup texture for SSS shaders.
/// The image is stored in RGBA8 format to the given pointer, in left-to-right, top-to-bottom
/// order. The curvature LUT is in linear color space.
///
/// \param pConfig				[in] the parameters for building curvature lookup texture for SSS
/// \param pCurvatureLUTOut		[out] buffer where the curvature LUT is stored
/// \param pErrorBlobOut		[in] buffer the error blob, where errors are stored.
///								Error messages will be stored in the error blob, if one is given;
///								if error messages are generated, use GFSDK_FaceWorks_FreeErrorBlob()
///								to free the storage.
///								The given allocator will be used to allocate temporary storage for
///								working data, if provided; if not, the standard CRT allocator will
///								be used.
///
/// \return						GFSDK_FaceWorks_OK if parameters are correct
/// 							GFSDK_FaceWorks_InvalidArgument if pConfig contains invalid values
GFSDK_FACEWORKS_API GFSDK_FaceWorks_Result GFSDK_FACEWORKS_CALLCONV GFSDK_FaceWorks_GenerateCurvatureLUT(
												const GFSDK_FaceWorks_CurvatureLUTConfig * pConfig,
												void * pCurvatureLUTOut,
												GFSDK_FaceWorks_ErrorBlob * pErrorBlobOut);

/// \brief Parameters for building shadow lookup texture (LUT) for SSS.
typedef struct
{
	float		m_diffusionRadius;			///< Diffusion radius, in world units (= 2.7mm for human skin)
	int			m_texWidth;					///< Width of curvature LUT (typically 512)
	int			m_texHeight;				///< Height of curvature LUT (typically 512)
	float		m_shadowWidthMin;			///< Min world-space penumbra width used to build the LUT (typically ~0.8 cm)
	float		m_shadowWidthMax;			///< Max world-space penumbra width used to build the LUT (typically ~10.0 cm)
	float		m_shadowSharpening;			///< Ratio by which output shadow is sharpened (adjust to taste; typically 3.0 to 10.0)
} GFSDK_FaceWorks_ShadowLUTConfig;

/// Calculate size needed to store pixels of texture generated by GFSDK_FaceWorks_GenerateShadowLUT.
///
/// \param pConfig				[in] the parameters for building shadow lookup texture for SSS
///
/// \return						the size needed to store pixels of texture generated by GFSDK_FaceWorks_GenerateShadowLUT
GFSDK_FACEWORKS_API size_t GFSDK_FACEWORKS_CALLCONV GFSDK_FaceWorks_CalculateShadowLUTSizeBytes(
												const GFSDK_FaceWorks_ShadowLUTConfig * pConfig);

/// Generate shadow lookup texture for SSS shaders.
/// The image is stored in RGBA8 format to the given pointer, in left-to-right, top-to-bottom
/// order.
/// The shadow LUT is in sRGB color space.
///
/// \param pConfig				[in] the parameters for building shadow lookup texture for SSS
/// \param pShadowLUTOut		[out] buffer where the shadow LUT is stored
/// \param pErrorBlobOut		[in] buffer the error blob, where errors are stored.
///								Error messages will be stored in the error blob, if one is given;
///								if error messages are generated, use GFSDK_FaceWorks_FreeErrorBlob()
///								to free the storage.
///								The given allocator will be used to allocate temporary storage for
///								working data, if provided; if not, the standard CRT allocator will
///								be used.
///
/// \return						GFSDK_FaceWorks_OK if parameters are correct
/// 							GFSDK_FaceWorks_InvalidArgument if pConfig contains invalid values
GFSDK_FACEWORKS_API GFSDK_FaceWorks_Result GFSDK_FACEWORKS_CALLCONV GFSDK_FaceWorks_GenerateShadowLUT(
												const GFSDK_FaceWorks_ShadowLUTConfig * pConfig,
												void * pShadowLUTOut,
												GFSDK_FaceWorks_ErrorBlob * pErrorBlobOut);

/// \brief Shared constant buffer
/// Include this struct in your constant buffer; it provides data to the SSS and deep scatter APIs.
/// This structure matches the corresponding struct in GFSDK_FaceWorks.hlsli.
typedef struct
{
	gfsdk_float4 data[3];					///< The opaque data used to communicate with shaders
} GFSDK_FaceWorks_CBData;

/// \brief Runtime config struct for SSS.
typedef struct
{
	float		m_diffusionRadius;			///< Diffusion radius, in world units (= 2.7mm for human skin)
	float		m_diffusionRadiusLUT;		///< Diffusion radius used to build the LUTs
	float		m_curvatureRadiusMinLUT;	///< Min radius of curvature used to build the LUT
	float		m_curvatureRadiusMaxLUT;	///< Max radius of curvature used to build the LUT
	float		m_shadowWidthMinLUT;		///< Min world-space penumbra width used to build the LUT
	float		m_shadowWidthMaxLUT;		///< Max world-space penumbra width used to build the LUT
	float		m_shadowFilterWidth;		///< World-space width of shadow filter
	int			m_normalMapSize;			///< Pixel size of normal map
	float		m_averageUVScale;			///< Average UV scale of the mesh, i.e. world-space size of UV unit square
} GFSDK_FaceWorks_SSSConfig;

/// Write constant buffer data for SSS, using specified configuration options.
///
/// \param pConfig				[in] pointer to runtime config struct for SSS
/// \param pCBDataOut			[out] pointer to CBData struct in your constant buffer
/// \param pErrorBlobOut		[in] buffer the error blob, where errors are stored.
///								Error messages will be stored in the error blob, if one is given;
///								if error messages are generated, use GFSDK_FaceWorks_FreeErrorBlob()
///								to free the storage.
///								The given allocator will be used to allocate temporary storage for
///								working data, if provided; if not, the standard CRT allocator will
///								be used.
///
/// \return						GFSDK_FaceWorks_OK if parameters are correct
/// 							GFSDK_FaceWorks_InvalidArgument if pConfig contains invalid values
GFSDK_FACEWORKS_API GFSDK_FaceWorks_Result GFSDK_FACEWORKS_CALLCONV GFSDK_FaceWorks_WriteCBDataForSSS(
												const GFSDK_FaceWorks_SSSConfig * pConfig,
												GFSDK_FaceWorks_CBData * pCBDataOut,
												GFSDK_FaceWorks_ErrorBlob * pErrorBlobOut);

/// Enum for projection types.
typedef enum
{
	GFSDK_FaceWorks_NoProjection,			///< No Projection
	GFSDK_FaceWorks_ParallelProjection,		///< Parallel projection
	GFSDK_FaceWorks_PerspectiveProjection,  ///< Perspective Projection
} GFSDK_FaceWorks_ProjectionType;

/// \brief Runtime config struct for deep scatter.
/// \details Parameters m_shadow* are for shadow map thickness estimate: you only need to fill these in if
/// you're using the FaceWorks helper functions forgetting the thickness. Set m_shadowProjType to
/// GFSDK_FaceWorks_NoProjection if not using the helpers.
typedef struct
{
	float			m_radius;				///< Deep scatter radius, in world units
	GFSDK_FaceWorks_ProjectionType
					m_shadowProjType;		///< What type of projection this is
	gfsdk_float4x4	m_shadowProjMatrix;		///< Shadow map projection matrix (row-vector convention)
	float			m_shadowFilterRadius;   ///< Desired filter radius, in shadow texture UV space
} GFSDK_FaceWorks_DeepScatterConfig;

/// Write constant buffer data for deep scatter, using specified configuration options.
///
/// \param pConfig				[in] pointer to runtime config struct for Deep Scatter
/// \param pCBDataOut			[out] pointer to CBData struct in your constant buffer
/// \param pErrorBlobOut		[in] buffer the error blob, where errors are stored.
///								Error messages will be stored in the error blob, if one is given;
///								if error messages are generated, use GFSDK_FaceWorks_FreeErrorBlob()
///								to free the storage.
///								The given allocator will be used to allocate temporary storage for
///								working data, if provided; if not, the standard CRT allocator will
///								be used.
///
/// \return						GFSDK_FaceWorks_OK if parameters are correct
/// 							GFSDK_FaceWorks_InvalidArgument if pConfig contains invalid values
GFSDK_FACEWORKS_API GFSDK_FaceWorks_Result GFSDK_FACEWORKS_CALLCONV GFSDK_FaceWorks_WriteCBDataForDeepScatter(
												const GFSDK_FaceWorks_DeepScatterConfig * pConfig,
												GFSDK_FaceWorks_CBData * pCBDataOut,
												GFSDK_FaceWorks_ErrorBlob * pErrorBlobOut);

#pragma pack(pop)

#endif // GFSDK_FACEWORKS_H
