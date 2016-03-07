//----------------------------------------------------------------------------------
// File:        FaceWorks/src/internal.h
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

#ifndef GFSDK_FACEWORKS_INTERNAL_H
#define GFSDK_FACEWORKS_INTERNAL_H

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <memory>

#include <GFSDK_FaceWorks.h>

using std::min;
using std::max;

// Get the dimension of a static array
template <typename T, int N> char (&dim_helper(T (&)[N]))[N];
#define dim(x) (sizeof(dim_helper(x)))
#define dim_field(S, m) (dim(((S*)nullptr)->m))

// math fixup: log2 wasn't defined before VS2013
#if defined(_MSC_VER) && (_MSC_VER < 1800)
inline float log2(float x) { return 1.442695041f * logf(x); }
#endif



// Shared constant buffer data for SSS and deep scatter;
// matches nvsf_CBData in internal.hlsl
struct CBData
{
	// SSS constants
	float	m_curvatureScale, m_curvatureBias;
	float	m_shadowScale, m_shadowBias;
	float	m_minLevelForBlurredNormal;

	// Deep scatter constants
	float	m_deepScatterFalloff;
	float	m_shadowFilterRadius;
	float	m_decodeDepthScale, m_decodeDepthBias;
};



// Memory allocation helper functions

inline void * FaceWorks_Malloc(size_t bytes, const gfsdk_new_delete_t & allocator)
{
	if (allocator.new_)
		return allocator.new_(bytes);
	else
		return ::operator new(bytes);
}

inline void FaceWorks_Free(void * p, const gfsdk_new_delete_t & allocator)
{
	if (!p)
		return;

	if (allocator.delete_)
		allocator.delete_(p);
	else
		::operator delete(p);
}

// STL allocator that uses the preceding functions

template <typename T>
class FaceWorks_Allocator : public std::allocator<T>
{
public:
	gfsdk_new_delete_t	m_allocator;

	explicit FaceWorks_Allocator(gfsdk_new_delete_t * pAllocator)
	{
		if (pAllocator)
		{
			m_allocator.new_ = pAllocator->new_;
			m_allocator.delete_ = pAllocator->delete_;
		}
		else
		{
			m_allocator.new_ = nullptr;
			m_allocator.delete_ = nullptr;
		}
	}

	template <typename T1>
	FaceWorks_Allocator(FaceWorks_Allocator<T1> const & other)
		: m_allocator(other.m_allocator)
	{
	}

	template <typename T1>
	struct rebind
	{
		typedef FaceWorks_Allocator<T1> other;
	};

	pointer allocate(size_type n, const void * hint = nullptr)
	{
		(void)hint;
		pointer p = pointer(FaceWorks_Malloc(n * sizeof(T), m_allocator));
		// Note: exceptions, yuck, but this is how you handle out-of-memory with STL...
		// In FaceWorks, this is caught by the code using the STL container and converted
		// to a return code; the exception should never propagate out to the caller.
		if (!p)
			throw std::bad_alloc();
		return p;
	}

	void deallocate(pointer p, size_type n)
	{
		(void)n;
		FaceWorks_Free(p, m_allocator);
	}
};



// Error blob helper functions
void BlobPrintf(GFSDK_FaceWorks_ErrorBlob * pBlob, const char * fmt, ...);
#define ErrPrintf(...) BlobPrintf(pErrorBlobOut, "Error: " __VA_ARGS__)
#define WarnPrintf(...) BlobPrintf(pErrorBlobOut, "Warning: " __VA_ARGS__)

#endif // GFSDK_FACEWORKS_INTERNAL_H
