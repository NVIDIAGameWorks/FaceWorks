//----------------------------------------------------------------------------------
// File:        FaceWorks/samples/sample_d3d11/util.cpp
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

#include "util.h"
#include "shader.h"

#include <algorithm>

#include <DirectXMath.h>

#include <DXUT/Core/DXUT.h>
#include <DXUT/Core/DXUTmisc.h>
#include <DXUT/Core/WICTextureLoader.h>
#include <DXUT/Core/DDSTextureLoader.h>

#include <GFSDK_FaceWorks.h>

#if defined(_MSC_VER) && (_MSC_VER < 1900)
#pragma warning(disable: 4351)	// Before VS2015 the warning "New behavior: array elements will be default-initialized" is produced when an array is construct-initialized with ().
#endif

using namespace std;
using namespace DirectX;

static wstring StrVprintf(const wchar_t * fmt, va_list args)
{
	int cChAlloc = _vscwprintf(fmt, args) + 1;
	wstring result = wstring(cChAlloc, L'\0');
	_vsnwprintf_s(&result[0], cChAlloc, _TRUNCATE, fmt, args);
	return result;
}

wstring StrPrintf(const wchar_t * fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	return StrVprintf(fmt, args);
}

#if defined(_DEBUG)
void DebugPrintf(const wchar_t * fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	OutputDebugString(StrVprintf(fmt, args).c_str());
}
#else
void DebugPrintf(const wchar_t * /*fmt*/, ...) {}
#endif

HRESULT LoadFile(const wchar_t * strFilename, vector<char> * pData, bool bText)
{
	FILE * pFile = nullptr;
	if (_wfopen_s(&pFile, strFilename, bText ? L"rt" : L"rb") != 0 || !pFile)
		return E_FAIL;
	assert(pFile);

	// Determine file size
	fseek(pFile, 0, SEEK_END);
	size_t size = ftell(pFile);

	// Read the whole file into memory
	assert(pData);
	pData->resize(bText ? size+1 : size);
	rewind(pFile);
	size_t sizeRead = fread(&(*pData)[0], sizeof(char), size, pFile);

	// Size can be smaller for text files, due to newline conversion
	assert(sizeRead == size || (bText && sizeRead < size));
	fclose(pFile);

	// Automatically null-terminate text files so string functions can be used
	if (bText)
	{
		pData->resize(sizeRead + 1);
		(*pData)[sizeRead] = 0;
	}

	return S_OK;
}



const wchar_t * BaseFilename(const wchar_t * strFilename)
{
	if (const wchar_t * strBase = wcsrchr(strFilename, L'\\'))
		return strBase + 1;
	else
		return strFilename;
}

#if defined(_DEBUG)
void SetDebugName(ID3D11DeviceChild * pD3DObject, const char * strName)
{
	HRESULT hr = pD3DObject->SetPrivateData(WKPDID_D3DDebugObjectName, UINT(strlen(strName)), strName);
	assert(SUCCEEDED(hr));
}

void SetDebugName(ID3D11DeviceChild * pD3DObject, const wchar_t * strName)
{
	// D3D only supports narrow strings for debug names; convert to UTF-8
	int cCh = WideCharToMultiByte(CP_UTF8, 0, strName, -1, nullptr, 0, nullptr, nullptr);
	string strUTF8(cCh, 0);
	WideCharToMultiByte(CP_UTF8, 0, strName, -1, &strUTF8[0], cCh, nullptr, nullptr);
	HRESULT hr = pD3DObject->SetPrivateData(WKPDID_D3DDebugObjectName, cCh, strUTF8.c_str());
	assert(SUCCEEDED(hr));
}
#else
void SetDebugName(ID3D11DeviceChild * /*pD3DObject*/, const char * /*strName*/) {}
void SetDebugName(ID3D11DeviceChild * /*pD3DObject*/, const wchar_t * /*strName*/) {}
#endif

// CMesh implementation

CMesh::CMesh()
:	m_verts(),
	m_indices(),
	m_pVtxBuffer(nullptr),
	m_pIdxBuffer(nullptr),
	m_vtxStride(0),
	m_cIdx(0),
	m_primtopo(D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED),
	m_posMin(0.0f, 0.0f, 0.0f),
	m_posMax(0.0f, 0.0f, 0.0f),
	m_posCenter(0.0f, 0.0f, 0.0f),
	m_diameter(0.0f),
	m_uvScale(1.0f)
{
}

void CMesh::Draw(ID3D11DeviceContext * pCtx)
{
	UINT zero = 0;
	pCtx->IASetVertexBuffers(0, 1, &m_pVtxBuffer, &m_vtxStride, &zero);
	pCtx->IASetIndexBuffer(m_pIdxBuffer, DXGI_FORMAT_R32_UINT, 0);
	pCtx->DrawIndexed(m_cIdx, 0, 0);
}

void CMesh::Release()
{
	m_verts.clear();
	m_indices.clear();
	SAFE_RELEASE(m_pVtxBuffer);
	SAFE_RELEASE(m_pIdxBuffer);
}

HRESULT CreateFullscreenMesh(ID3D11Device * pDevice, CMesh * pMesh)
{
	HRESULT hr;

	// Positions are directly in clip space; normals aren't used.

	Vertex verts[] =
	{
		// pos                    normal         uv
		{ XMFLOAT3(-1, -1, 0), XMFLOAT3(), XMFLOAT2(0,  1) },
		{ XMFLOAT3( 3, -1, 0), XMFLOAT3(), XMFLOAT2(2,  1) },
		{ XMFLOAT3(-1,  3, 0), XMFLOAT3(), XMFLOAT2(0, -1) },
	};

	UINT indices[] = { 0, 1, 2 };

	pMesh->m_verts.assign(&verts[0], &verts[dim(verts)]);
	pMesh->m_indices.assign(&indices[0], &indices[dim(indices)]);

	D3D11_BUFFER_DESC vtxBufferDesc =
	{
		sizeof(Vertex) * dim(verts),
		D3D11_USAGE_IMMUTABLE,
		D3D11_BIND_VERTEX_BUFFER,
		0,	// no cpu access
		0,	// no misc flags
		0,	// structured buffer stride
	};
	D3D11_SUBRESOURCE_DATA vtxBufferData = { &verts[0], 0, 0 };

	V_RETURN(pDevice->CreateBuffer(&vtxBufferDesc, &vtxBufferData, &pMesh->m_pVtxBuffer));

	D3D11_BUFFER_DESC idxBufferDesc =
	{
		sizeof(UINT) * dim(indices),
		D3D11_USAGE_IMMUTABLE,
		D3D11_BIND_INDEX_BUFFER,
		0,	// no cpu access
		0,	// no misc flags
		0,	// structured buffer stride
	};
	D3D11_SUBRESOURCE_DATA idxBufferData = { &indices[0], 0, 0 };

	V_RETURN(pDevice->CreateBuffer(&idxBufferDesc, &idxBufferData, &pMesh->m_pIdxBuffer));

	pMesh->m_vtxStride = sizeof(Vertex);
	pMesh->m_cIdx = UINT(dim(indices));
	pMesh->m_primtopo = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	SetDebugName(pMesh->m_pVtxBuffer, "Fullscreen mesh VB");
	SetDebugName(pMesh->m_pIdxBuffer, "Fullscreen mesh IB");

	return S_OK;
}



// Mesh loading - helper functions

HRESULT LoadObjMeshRaw(
	const wchar_t * strFilename,
	vector<Vertex> * pVerts,
	vector<int> * pIndices,
	XMFLOAT3 * pPosMin,
	XMFLOAT3 * pPosMax)
{
	HRESULT hr;

	// Read the whole file into memory
	vector<char> data;
	V_RETURN(LoadFile(strFilename, &data, true));

	vector<XMFLOAT3> positions;
	vector<XMFLOAT3> normals;
	vector<XMFLOAT2> uvs;

	struct OBJVertex { int iPos, iNormal, iUv; };
	vector<OBJVertex> OBJverts;

	struct OBJFace { int iVertStart, iVertEnd; };
	vector<OBJFace> OBJfaces;


	XMVECTOR posMin = XMVectorReplicate(FLT_MAX);
	XMVECTOR posMax = XMVectorReplicate(-FLT_MAX);

	// Parse the OBJ format line-by-line
	char * pCtxLine = nullptr;
	for (char * pLine = strtok_s(&data[0], "\n", &pCtxLine);
		 pLine;
		 pLine = strtok_s(nullptr, "\n", &pCtxLine))
	{
		// Strip comments starting with #
		if (char * pChzComment = strchr(pLine, '#'))
			*pChzComment = 0;

		// Parse the line token-by-token
		char * pCtxToken = nullptr;
		char * pToken = strtok_s(pLine, " \t", &pCtxToken);

		// Ignore blank lines
		if (!pToken)
			continue;

		if (_stricmp(pToken, "v") == 0)
		{

			XMFLOAT3 pos;
			pos.x = float(atof(strtok_s(nullptr, " \t", &pCtxToken)));
			pos.y = float(atof(strtok_s(nullptr, " \t", &pCtxToken)));
			pos.z = float(atof(strtok_s(nullptr, " \t", &pCtxToken)));
			positions.push_back(pos);

			XMVECTOR pos4 = XMLoadFloat3(&pos);

			posMin = XMVectorMin(posMin, pos4);
			posMax = XMVectorMax(posMax, pos4);
		}
		else if (_stricmp(pToken, "vn") == 0)
		{
			// Add normal
			XMFLOAT3 normal;
			normal.x = float(atof(strtok_s(nullptr, " \t", &pCtxToken)));
			normal.y = float(atof(strtok_s(nullptr, " \t", &pCtxToken)));
			normal.z = float(atof(strtok_s(nullptr, " \t", &pCtxToken)));
			normals.push_back(normal);
		}
		else if (_stricmp(pToken, "vt") == 0)
		{
			// Add UV, flipping V-axis since OBJ is stored in the opposite convention
			XMFLOAT2 uv;
			uv.x = float(atof(strtok_s(nullptr, " \t", &pCtxToken)));
			uv.y = 1.0f - float(atof(strtok_s(nullptr, " \t", &pCtxToken)));
			uvs.push_back(uv);
		}
		else if (_stricmp(pToken, "f") == 0)
		{
			// Add face
			OBJFace face;
			face.iVertStart = int(OBJverts.size());

			while (char * pToken2 = strtok_s(nullptr, " \t", &pCtxToken))
			{
				// Parse vertex specification, with slashes separating position, UV, normal indices
				// Note: assuming all three components are present, OBJ format allows some to be missing.
				OBJVertex vert;
				sscanf_s(pToken2, "%d/%d/%d", &vert.iPos, &vert.iUv, &vert.iNormal);
				
				// OBJ format uses 1-based indices; correct them
				--vert.iPos;
				--vert.iNormal;
				--vert.iUv;

				OBJverts.push_back(vert);
			}

			face.iVertEnd = int(OBJverts.size());
			OBJfaces.push_back(face);
		}
		else
		{
			// Unknown command; just ignore
		}
	}

	// Convert to vertex buffer and index buffer

	pVerts->reserve(OBJverts.size());

	for (size_t iVert = 0, cVert = OBJverts.size(); iVert < cVert; ++iVert)
	{
		OBJVertex objv = OBJverts[iVert];
		Vertex v = {};
		v.m_pos = positions[objv.iPos];
		v.m_normal = normals[objv.iNormal];
		v.m_uv = uvs[objv.iUv];
		pVerts->push_back(v);
	}

	for (size_t iFace = 0, cFace = OBJfaces.size(); iFace < cFace; ++iFace)
	{
		OBJFace face = OBJfaces[iFace];

		int iVertBase = face.iVertStart;

		// Triangulate the face
		for (int iVert = face.iVertStart + 2; iVert < face.iVertEnd; ++iVert)
		{
			pIndices->push_back(iVertBase);
			pIndices->push_back(iVert - 1);
			pIndices->push_back(iVert);
		}
	}

	if (pPosMin) XMStoreFloat3(pPosMin, posMin);
	if (pPosMax) XMStoreFloat3(pPosMax, posMax);

	return S_OK;
}

void DeduplicateVerts(CMesh * pMesh)
{
	struct VertexHasher
	{
		hash<float> fh;
		size_t operator () (const Vertex & v) const
		{
			return fh(v.m_pos.x) ^ fh(v.m_pos.y) ^ fh(v.m_pos.z) ^
				   fh(v.m_normal.x) ^ fh(v.m_normal.y) ^ fh(v.m_normal.z) ^
				   fh(v.m_uv.x) ^ fh(v.m_uv.y) ^
				   fh(v.m_curvature);
		}
	};

	struct VertexEquator
	{
		bool operator () (const Vertex & u, const Vertex & v) const
		{
			return (u.m_pos.x == v.m_pos.x &&
					u.m_pos.y == v.m_pos.y &&
					u.m_pos.z == v.m_pos.z &&
					u.m_normal.x == v.m_normal.x &&
					u.m_normal.y == v.m_normal.y &&
					u.m_normal.z == v.m_normal.z &&
					u.m_uv.x == v.m_uv.x &&
					u.m_uv.y == v.m_uv.y &&
					u.m_curvature == v.m_curvature);
		}
	};

	vector<Vertex> vertsDeduplicated;
	vector<int> remappingTable;
	unordered_map<Vertex, int, VertexHasher, VertexEquator> mapVertToIndex;

	vertsDeduplicated.reserve(pMesh->m_verts.size());
	remappingTable.reserve(pMesh->m_verts.size());

	for (int i = 0, cVert = int(pMesh->m_verts.size()); i < cVert; ++i)
	{
		const Vertex & vert = pMesh->m_verts[i];
		unordered_map<Vertex, int, VertexHasher, VertexEquator>::iterator
			iter = mapVertToIndex.find(vert);
		if (iter == mapVertToIndex.end())
		{
			// Found a new vertex that's not in the map yet.
			int newIndex = int(vertsDeduplicated.size());
			vertsDeduplicated.push_back(vert);
			remappingTable.push_back(newIndex);
			mapVertToIndex[vert] = newIndex;
		}
		else
		{
			// It's already in the map; re-use the previous index
			int newIndex = iter->second;
			remappingTable.push_back(newIndex);
		}
	}

	assert(vertsDeduplicated.size() <= pMesh->m_verts.size());
	assert(remappingTable.size() == pMesh->m_verts.size());

	vector<int> indicesRemapped;
	indicesRemapped.reserve(pMesh->m_indices.size());

	for (int i = 0, cIndex = int(pMesh->m_indices.size()); i < cIndex; ++i)
	{
		indicesRemapped.push_back(remappingTable[pMesh->m_indices[i]]);
	}

	pMesh->m_verts.swap(vertsDeduplicated);
	pMesh->m_indices.swap(indicesRemapped);
}

// Custom allocator for FaceWorks - just logs the allocs and passes through to CRT

void * MallocForFaceWorks(size_t bytes)
{
	void * p = ::operator new(bytes);
	DebugPrintf(L"FaceWorks alloced %d bytes: %p\n", bytes, p);
	return p;
}

void FreeForFaceWorks(void * p)
{
	DebugPrintf(L"FaceWorks freed %p\n", p);
	::operator delete(p);
}

void CalculateCurvature(CMesh * pMesh)
{
	// Calculate mesh curvature - also demonstrate using a custom allocator

	GFSDK_FaceWorks_ErrorBlob errorBlob = {};
	gfsdk_new_delete_t allocator = { &MallocForFaceWorks, &FreeForFaceWorks };
	GFSDK_FaceWorks_Result result = GFSDK_FaceWorks_CalculateMeshCurvature(
										int(pMesh->m_verts.size()),
										&pMesh->m_verts[0].m_pos,
										sizeof(Vertex),
										&pMesh->m_verts[0].m_normal,
										sizeof(Vertex),
										int(pMesh->m_indices.size()),
										&pMesh->m_indices[0],
										2, // smoothing passes
										&pMesh->m_verts[0].m_curvature,
										sizeof(Vertex),
										&errorBlob,
										&allocator);

	if (result != GFSDK_FaceWorks_OK)
	{
#if defined(_DEBUG)
		wchar_t msg[512];
		_snwprintf_s(msg, dim(msg), _TRUNCATE, 
			L"GFSDK_FaceWorks_CalculateMeshCurvature() failed:\n%hs", errorBlob.m_msg);
		DXUTTrace(__FILE__, __LINE__, E_FAIL, msg, true);
#endif
		GFSDK_FaceWorks_FreeErrorBlob(&errorBlob);
		return;
	}

#if defined(_DEBUG) && 0
	// Report min, max, mean curvature over mesh
	float minCurvature = FLT_MAX;
	float maxCurvature = -FLT_MAX;
	float curvatureSum = 0.0f;
	for (int i = 0, cVert = int(pMesh->m_verts.size()); i < cVert; ++i)
	{
		minCurvature = min(minCurvature, pMesh->m_verts[i].m_curvature);
		maxCurvature = max(maxCurvature, pMesh->m_verts[i].m_curvature);
		curvatureSum += pMesh->m_verts[i].m_curvature;
	}
	float meanCurvature = curvatureSum / float(pMesh->m_verts.size());
	DebugPrintf(
		L"\tCurvature min = %0.2f cm^-1, max = %0.2f cm^-1, mean = %0.2f cm^-1\n",
		minCurvature, maxCurvature, meanCurvature);
#endif // defined(_DEBUG)
}

void CalculateUVScale(CMesh * pMesh)
{
	GFSDK_FaceWorks_ErrorBlob errorBlob = {};
	GFSDK_FaceWorks_Result result = GFSDK_FaceWorks_CalculateMeshUVScale(
										int(pMesh->m_verts.size()),
										&pMesh->m_verts[0].m_pos,
										sizeof(Vertex),
										&pMesh->m_verts[0].m_uv,
										sizeof(Vertex),
										int(pMesh->m_indices.size()),
										&pMesh->m_indices[0],
										&pMesh->m_uvScale,
										&errorBlob);
	if (result != GFSDK_FaceWorks_OK)
	{
#if defined(_DEBUG)
		wchar_t msg[512];
		_snwprintf_s(msg, dim(msg), _TRUNCATE,
			L"GFSDK_FaceWorks_CalculateMeshUVScale() failed:\n%hs", errorBlob.m_msg);
		DXUTTrace(__FILE__, __LINE__, E_FAIL, msg, true);
#endif
		GFSDK_FaceWorks_FreeErrorBlob(&errorBlob);
		return;
	}

#if 0
	DebugPrintf(L"\tUV scale %0.2f cm\n", pMesh->m_uvScale);
#endif
}

void CalculateTangents(CMesh * pMesh)
{
	assert(pMesh->m_indices.size() % 3 == 0);

	// Clear tangents to zero
	for (int i = 0, c = int(pMesh->m_verts.size()); i < c; ++i)
	{
		pMesh->m_verts[i].m_tangent = XMFLOAT3(0, 0, 0);
	}

	// Generate a tangent for each triangle, based on triangle's UV mapping,
	// and accumulate onto vertex
	for (int i = 0, c = int(pMesh->m_indices.size()); i < c; i += 3)
	{
		int indices[3] = { pMesh->m_indices[i], pMesh->m_indices[i+1], pMesh->m_indices[i+2] };

		// Gather positions for this triangle
		XMVECTOR facePositions[3] =
		{
			XMLoadFloat3(&pMesh->m_verts[indices[0]].m_pos),
			XMLoadFloat3(&pMesh->m_verts[indices[1]].m_pos),
			XMLoadFloat3(&pMesh->m_verts[indices[2]].m_pos)
		};

		// Calculate edge and normal vectors
		XMVECTOR edge0 = facePositions[1] - facePositions[0];
		XMVECTOR edge1 = facePositions[2] - facePositions[0];
		XMVECTOR normal = XMVector3Cross(edge0, edge1);

		// Calculate matrix from unit triangle to position space
		XMMATRIX matUnitToPosition(edge0, edge1, normal, XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f));

		// Gather UVs for this triangle
		XMFLOAT2 faceUVs[3] =
		{
			pMesh->m_verts[indices[0]].m_uv,
			pMesh->m_verts[indices[1]].m_uv,
			pMesh->m_verts[indices[2]].m_uv,
		};


#if 0
		// This code is no longer used because it caused to invert a rank-deficient matrix

		// Calculate UV space edge vectors
		// Calculate matrix from unit triangle to UV space
		XMMATRIX matUnitToUV(
			faceUVs[1].x - faceUVs[0].x, faceUVs[1].y - faceUVs[0].y, 0.0f, 0.0f,
			faceUVs[2].x - faceUVs[0].x, faceUVs[2].y - faceUVs[0].y, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);
		// Calculate matrix from UV space to position space
		XMMATRIX matUVToUnit = XMMatrixInverse(nullptr, matUnitToUV);
		XMMATRIX matUVToPosition = XMMatrixMultiply(matUVToUnit, matUnitToPosition);

		// The x-axis of that matrix is the tangent vector
		XMVECTOR tangent = XMVector3Normalize(matUVToPosition.r[0]);
#else
		XMMATRIX matUVToUnitDenormalized(
			faceUVs[2].y - faceUVs[0].y, faceUVs[0].y - faceUVs[1].y, 0.0f, 0.0f,
			faceUVs[0].x - faceUVs[2].x, faceUVs[1].x - faceUVs[0].x, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		);
		XMMATRIX matUVToPositionDenormalized = XMMatrixMultiply(matUVToUnitDenormalized, matUnitToPosition);
		
		XMVECTOR tangent = matUVToPositionDenormalized.r[0];
		if (XMVector3Equal(tangent, XMVectorZero()))
		{
			tangent = XMVector2Cross(matUVToPositionDenormalized.r[1], matUVToPositionDenormalized.r[2]);
			if (XMVector3Equal(tangent, XMVectorZero()))
			{
				tangent = edge0;
			}
		}

		tangent = XMVector3Normalize(tangent);
#endif
		// Accumulate onto vertices
		XMStoreFloat3(&pMesh->m_verts[indices[0]].m_tangent, XMLoadFloat3(&pMesh->m_verts[indices[0]].m_tangent) + tangent);
		XMStoreFloat3(&pMesh->m_verts[indices[1]].m_tangent, XMLoadFloat3(&pMesh->m_verts[indices[1]].m_tangent) + tangent);
		XMStoreFloat3(&pMesh->m_verts[indices[2]].m_tangent, XMLoadFloat3(&pMesh->m_verts[indices[2]].m_tangent) + tangent);
	}

	// Normalize summed tangents
	for (int i = 0, c = int(pMesh->m_verts.size()); i < c; ++i)
	{
		XMStoreFloat3(&pMesh->m_verts[i].m_tangent, XMVector3Normalize(XMLoadFloat3(&pMesh->m_verts[i].m_tangent)));
	}
}

HRESULT LoadObjMesh(
	const wchar_t * strFilename,
	ID3D11Device * pDevice,
	CMesh * pMesh)
{
	HRESULT hr;

	V_RETURN(LoadObjMeshRaw(
				strFilename, &pMesh->m_verts, &pMesh->m_indices,
				&pMesh->m_posMin, &pMesh->m_posMax));

	DeduplicateVerts(pMesh);

	DebugPrintf(
		L"Loaded %s, %d verts, %d indices\n",
		strFilename, int(pMesh->m_verts.size()), int(pMesh->m_indices.size()));

	// !!!UNDONE: vertex cache optimization?

	XMVECTOR posMin = XMLoadFloat3(&pMesh->m_posMin);
	XMVECTOR posMax = XMLoadFloat3(&pMesh->m_posMax);
	XMStoreFloat3(&pMesh->m_posCenter, 0.5f * (posMin + posMax));
	pMesh->m_diameter = XMVectorGetX(XMVector3Length(posMax - posMin));

	CalculateCurvature(pMesh);
	CalculateUVScale(pMesh);
	CalculateTangents(pMesh);

	D3D11_BUFFER_DESC vtxBufferDesc =
	{
		sizeof(Vertex) * UINT(pMesh->m_verts.size()),
		D3D11_USAGE_IMMUTABLE,
		D3D11_BIND_VERTEX_BUFFER,
		0,	// no cpu access
		0,	// no misc flags
		0,	// structured buffer stride
	};
	D3D11_SUBRESOURCE_DATA vtxBufferData = { &pMesh->m_verts[0], 0, 0 };

	V_RETURN(pDevice->CreateBuffer(&vtxBufferDesc, &vtxBufferData, &pMesh->m_pVtxBuffer));

	D3D11_BUFFER_DESC idxBufferDesc =
	{
		sizeof(int) * UINT(pMesh->m_indices.size()),
		D3D11_USAGE_IMMUTABLE,
		D3D11_BIND_INDEX_BUFFER,
		0,	// no cpu access
		0,	// no misc flags
		0,	// structured buffer stride
	};
	D3D11_SUBRESOURCE_DATA idxBufferData = { &pMesh->m_indices[0], 0, 0 };

	V_RETURN(pDevice->CreateBuffer(&idxBufferDesc, &idxBufferData, &pMesh->m_pIdxBuffer));

	pMesh->m_vtxStride = sizeof(Vertex);
	pMesh->m_cIdx = UINT(pMesh->m_indices.size());
	pMesh->m_primtopo = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	SetDebugName(pMesh->m_pVtxBuffer, StrPrintf(L"%s VB", BaseFilename(strFilename)).c_str());
	SetDebugName(pMesh->m_pIdxBuffer, StrPrintf(L"%s IB", BaseFilename(strFilename)).c_str());

	return S_OK;
}


bool WCStringEndsWith(const wchar_t* hay, const wchar_t* needle)
{
	size_t hay_lenght = wcslen(hay);
	size_t needle_length = wcslen(needle);
	if (hay_lenght < needle_length) return false;    
	return wcscmp(hay + hay_lenght - needle_length, needle) == 0;
}

HRESULT LoadTexture(
	const wchar_t * strFilename,
	ID3D11Device * pDevice,
	ID3D11ShaderResourceView ** ppSrv,
	int flags)
{
	return LoadTexture(strFilename, pDevice, nullptr, ppSrv, flags);
}

HRESULT LoadTexture(
	const wchar_t * strFilename,
	ID3D11Device * pDevice,
	ID3D11DeviceContext * pDeviceContext,
	ID3D11ShaderResourceView ** ppSrv,
	int flags)
{
	HRESULT hr;

	bool bMipmap = (flags & LT_Mipmap) != 0;
	bool bHDR = (flags & LT_HDR) != 0;
	bool bCubemap = (flags & LT_Cubemap) != 0;
	bool bLinear = (flags & LT_Linear) != 0;

	// Load the texture, generating mipmaps if requested

	if (WCStringEndsWith(strFilename, L".dds"))
	{
		bMipmap = false;
		V_RETURN(CreateDDSTextureFromFileEx(
			pDevice,
			bMipmap ? pDeviceContext : nullptr,
			strFilename,
			4096,
			bMipmap ? D3D11_USAGE_DEFAULT : D3D11_USAGE_IMMUTABLE,
			(D3D11_BIND_SHADER_RESOURCE | (bMipmap ? (D3D11_BIND_RENDER_TARGET) : 0)),
			0,
			(bCubemap ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0) | (bMipmap ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0),
			!(bHDR || bLinear),
			nullptr,
			ppSrv));

	}
	else
	{
		assert((!bMipmap) || (pDeviceContext != nullptr));
		V_RETURN(CreateWICTextureFromFileEx(
			pDevice,
			bMipmap ? pDeviceContext : nullptr,
			strFilename,
			4096,
			bMipmap ? D3D11_USAGE_DEFAULT : D3D11_USAGE_IMMUTABLE,
			(D3D11_BIND_SHADER_RESOURCE | (bMipmap ? (D3D11_BIND_RENDER_TARGET) : 0)),
			0,
			(bCubemap ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0) | (bMipmap ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0),
			!(bHDR || bLinear),
			nullptr,
			ppSrv));
	}

#if defined(_DEBUG)
	// Set the name of the texture
	SetDebugName(*ppSrv, BaseFilename(strFilename));

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	(*ppSrv)->GetDesc(&srvDesc);

	UINT cMipLevels = 1;
	const wchar_t * strDimension = L"other";
	switch (srvDesc.ViewDimension)
	{
	case D3D11_SRV_DIMENSION_TEXTURE2D:
		strDimension = L"2D";
		cMipLevels = srvDesc.Texture2D.MipLevels;
		break;

	case D3D11_SRV_DIMENSION_TEXTURECUBE:
		strDimension = L"cube";
		cMipLevels = srvDesc.TextureCube.MipLevels;
		break;
	
	default: assert(false); break;
	}

	DebugPrintf(
		L"Loaded %s, format %s, %s, %d mip levels\n",
		strFilename, 
		DXUTDXGIFormatToString(srvDesc.Format, false),
		strDimension,
		cMipLevels);
#endif

	return S_OK;
}



// CMayaStyleCamera implementation

CMayaStyleCamera::CMayaStyleCamera()
:	CBaseCamera(),
	m_fRadius(5.0f)
{
}

void CMayaStyleCamera::FrameMove(FLOAT /*fElapsedTime*/)
{
	if (IsKeyDown(m_aKeys[CAM_RESET]))
		Reset();

	// Get the mouse movement (if any) if the mouse button are down
	GetInput(m_bEnablePositionMovement, m_nCurrentButtonMask != 0, false);

	// If left mouse button is down, update yaw and pitch based on mouse
	if (m_nCurrentButtonMask & MOUSE_LEFT_BUTTON)
	{
		// Update the pitch & yaw angle based on mouse movement
		float fYawDelta = -m_vMouseDelta.x * m_fRotationScaler;
		float fPitchDelta = m_vMouseDelta.y * m_fRotationScaler;

		// Invert pitch if requested
		if (m_bInvertPitch)
			fPitchDelta = -fPitchDelta;

		m_fCameraPitchAngle += fPitchDelta;
		m_fCameraYawAngle += fYawDelta;

		// Limit pitch to straight up or straight down
		m_fCameraPitchAngle = max(-XM_PI / 2.0f, m_fCameraPitchAngle);
		m_fCameraPitchAngle = min(+XM_PI / 2.0f, m_fCameraPitchAngle);
	}

	// If mouse wheel is scrolled or right mouse button is down, update radius
	if (m_nMouseWheelDelta != 0)
		m_fRadius -= m_nMouseWheelDelta * m_fRadius * 0.1f / 120.0f;
	if (m_nCurrentButtonMask & MOUSE_RIGHT_BUTTON)
		m_fRadius += m_vMouseDelta.y * m_fRadius * 0.01f;
	m_fRadius = max(m_fRadius, 1.0f);
	m_nMouseWheelDelta = 0;

	// Make a rotation matrix based on the camera's yaw & pitch
	XMMATRIX mCameraRot = XMMatrixRotationRollPitchYaw(m_fCameraPitchAngle, m_fCameraYawAngle, 0.0f);

	XMVECTOR vRight(mCameraRot.r[0]);
	XMVECTOR vUp(mCameraRot.r[1]);
	XMVECTOR vForward(mCameraRot.r[2]);

	XMVECTOR vLookAt = XMLoadFloat3(&m_vLookAt);

	// If middle mouse button is down, update look-at point
	if (m_nCurrentButtonMask & MOUSE_MIDDLE_BUTTON)
	{
		vLookAt += vRight * (m_vMouseDelta.x * m_fRadius * 0.005f);
		vLookAt += vUp * (m_vMouseDelta.y * m_fRadius * 0.005f);
		XMStoreFloat3(&m_vLookAt, vLookAt);
	}

	// Update the eye point based on a radius away from the lookAt position
	XMVECTOR vEye = vLookAt - vForward * m_fRadius;
	XMStoreFloat3(&m_vEye, vEye);

	// Update the view matrix
	XMStoreFloat4x4(&m_mView, XMMatrixLookAtRH(vEye, vLookAt, vUp));
}

void CMayaStyleCamera::SetProjParams(FLOAT fFOV, FLOAT fAspect, FLOAT fNearPlane, FLOAT fFarPlane)
{
	// Set attributes for the projection matrix
	m_fFOV = fFOV;
	m_fAspect = fAspect;
	m_fNearPlane = fNearPlane;
	m_fFarPlane = fFarPlane;

	XMStoreFloat4x4(&m_mProj, XMMatrixPerspectiveFovRH(fFOV, fAspect, fNearPlane, fFarPlane));
}

void CMayaStyleCamera::SetViewParams(FXMVECTOR pvEyePt, FXMVECTOR pvLookatPt)
{
	CBaseCamera::SetViewParams(pvEyePt, pvLookatPt);

	XMVECTOR vecLook = pvLookatPt - pvEyePt;
	m_fRadius = XMVectorGetX(XMVector3Length(vecLook));
}



// CFirstPersonCameraRH implementation

void CFirstPersonCameraRH::FrameMove(FLOAT fElapsedTime)
{
	if (DXUTGetGlobalTimer()->IsStopped())
	{
		if (DXUTGetFPS() == 0.0f)
			fElapsedTime = 0;
		else
			fElapsedTime = 1.0f / DXUTGetFPS();
	}

	if (IsKeyDown(m_aKeys[CAM_RESET]))
		Reset();

	// Get keyboard/mouse/gamepad input
	GetInput(m_bEnablePositionMovement, (m_nActiveButtonMask & m_nCurrentButtonMask) || m_bRotateWithoutButtonDown,
			  true);

	// Get amount of velocity based on the keyboard input and drag (if any)
	UpdateVelocity(fElapsedTime);

	// Simple euler method to calculate position delta
	XMVECTOR vPosDelta = XMVectorSet(-m_vVelocity.x, m_vVelocity.y, m_vVelocity.z, 0.0f)*fElapsedTime;
	if (GetAsyncKeyState(VK_LSHIFT) != 0) {
		vPosDelta *= 2.0f;
	}

	// If rotating the camera 
	if ((m_nActiveButtonMask & m_nCurrentButtonMask) ||
		m_bRotateWithoutButtonDown ||
		m_vGamePadRightThumb.x != 0 ||
		m_vGamePadRightThumb.z != 0)
	{
		// Update the pitch & yaw angle based on mouse movement
		float fYawDelta = -m_vRotVelocity.x;
		float fPitchDelta = m_vRotVelocity.y;

		// Invert pitch if requested
		if( m_bInvertPitch )
			fPitchDelta = -fPitchDelta;

		m_fCameraPitchAngle += fPitchDelta;
		m_fCameraYawAngle += fYawDelta;

		// Limit pitch to straight up or straight down
		m_fCameraPitchAngle = max(-XM_PI / 2.0f, m_fCameraPitchAngle);
		m_fCameraPitchAngle = min(+XM_PI / 2.0f, m_fCameraPitchAngle);
	}

	// Make a rotation matrix based on the camera's yaw & pitch
	XMMATRIX mCameraRot = XMMatrixRotationRollPitchYaw(m_fCameraPitchAngle, m_fCameraYawAngle, 0.0f);

	// Transform vectors based on camera's rotation matrix
	XMVECTOR vLocalUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR vLocalAhead = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	XMVECTOR vWorldUp = XMVector3TransformCoord(vLocalUp, mCameraRot);
	XMVECTOR vWorldAhead = XMVector3TransformCoord(vLocalAhead, mCameraRot);

	// Transform the position delta by the camera's rotation 
	if (!m_bEnableYAxisMovement)
	{
		// If restricting Y movement, do not include pitch
		// when transforming position delta vector.
		mCameraRot = XMMatrixRotationRollPitchYaw(0.0f, m_fCameraYawAngle, 0.f);
	}
	XMVECTOR vPosDeltaWorld = XMVector3TransformCoord(vPosDelta, mCameraRot);

	// Move the eye position
	XMVECTOR vEye = XMLoadFloat3(&m_vEye) + vPosDeltaWorld;
	XMStoreFloat3(&m_vEye, vEye);

	if (m_bClipToBoundary)
		ConstrainToBoundary(vEye);

	// Update the lookAt position based on the eye position 
	XMVECTOR vLookAt = vEye + vWorldAhead;
	XMStoreFloat3(&m_vLookAt, vLookAt);

	// Update the view matrix
	XMMATRIX mView = XMMatrixLookAtRH(vEye, vLookAt, vWorldUp);
	XMStoreFloat4x4(&m_mView, mView);
	XMStoreFloat4x4(&m_mCameraWorld, XMMatrixInverse(nullptr, mView));
}

void CFirstPersonCameraRH::SetProjParams(FLOAT fFOV, FLOAT fAspect, FLOAT fNearPlane, FLOAT fFarPlane)
{
	// Set attributes for the projection matrix
	m_fFOV = fFOV;
	m_fAspect = fAspect;
	m_fNearPlane = fNearPlane;
	m_fFarPlane = fFarPlane;

	XMStoreFloat4x4(&m_mProj, XMMatrixPerspectiveFovRH(fFOV, fAspect, fNearPlane, fFarPlane));
}



// CShadowMap implementation

CShadowMap::CShadowMap()
:	m_pDsv(nullptr),
	m_pSrv(nullptr),
	m_size(0),
	m_vecLight(0.0f, 0.0f, 0.0f),
	m_posMinScene(FLT_MAX, FLT_MAX, FLT_MAX),
	m_posMaxScene(-FLT_MAX, -FLT_MAX, -FLT_MAX),
	m_matWorldToClip(),
	m_matWorldToUvzw(),
	m_vecDiam()
{
}

HRESULT CShadowMap::Init(unsigned int size, ID3D11Device * pDevice)
{
	HRESULT hr;

	// Create 2D texture for shadow map
	D3D11_TEXTURE2D_DESC texDesc =
	{
		size, size,						// width, height
		1, 1,							// mip levels, array size
		DXGI_FORMAT_R32_TYPELESS,
		{ 1, 0 },						// multisample count, quality
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL,
		0,								// no cpu access
		0,								// no misc flags
	};
	ID3D11Texture2D * pTex = nullptr;
	V_RETURN(pDevice->CreateTexture2D(&texDesc, nullptr, &pTex));

	// Create depth-stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc =
	{
		DXGI_FORMAT_D32_FLOAT,
		D3D11_DSV_DIMENSION_TEXTURE2D,
	};
	V_RETURN(pDevice->CreateDepthStencilView(pTex, &dsvDesc, &m_pDsv));

	// Create shader resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc =
	{
		DXGI_FORMAT_R32_FLOAT,
		D3D11_SRV_DIMENSION_TEXTURE2D,
	};
	srvDesc.Texture2D.MipLevels = 1;
	V_RETURN(pDevice->CreateShaderResourceView(pTex, &srvDesc, &m_pSrv));

	// The texture isn't needed any more
	SAFE_RELEASE(pTex);

	m_size = size;

	SetDebugName(m_pDsv, "Shadow map - DSV");
	SetDebugName(m_pSrv, "Shadow map - SRV");
	DebugPrintf(L"Created shadow map, format D32_FLOAT, %d x %d\n", size, size);

	return S_OK;
}

void CShadowMap::UpdateMatrix()
{
	// Calculate view matrix based on light direction

	XMVECTOR posEye = XMVectorZero();
	XMVECTOR posLookAt = posEye - XMLoadFloat3(&m_vecLight);
	XMVECTOR vecUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	// Handle light vector being straight up or down
	if (fabsf(m_vecLight.x) < 1e-4f && fabsf(m_vecLight.z) < 1e-4f) {
		vecUp = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	}

	XMMATRIX matView = XMMatrixLookAtRH(posEye, posLookAt, vecUp);

	// Transform scene AABB into view space and recalculate bounds

	XMVECTOR posMinView = XMVectorReplicate(FLT_MAX);
	XMVECTOR posMaxView = XMVectorReplicate(-FLT_MAX);

	for (int i = 0; i < 8; ++i)
	{
		XMVECTOR posWorld = XMVectorSet(
			(i & 1) ? m_posMaxScene.x : m_posMinScene.x,
			(i & 2) ? m_posMaxScene.y : m_posMinScene.y,
			(i & 4) ? m_posMaxScene.z : m_posMinScene.z,
			0.0f);

		XMVECTOR posView = XMVector3TransformCoord(posWorld, matView);

		posMinView = XMVectorMin(posMinView, posView);
		posMaxView = XMVectorMax(posMaxView, posView);
	}

	XMStoreFloat3(&m_vecDiam, posMaxView - posMinView);

#if 0
	DebugPrintf(
		L"Shadow map world-space diameter (cm): %0.2f %0.2f %0.2f\n",
		m_vecDiam.x, m_vecDiam.y, m_vecDiam.z);
#endif

	// Calculate orthogonal projection matrix to fit the scene bounds
	XMMATRIX matProj = XMMatrixOrthographicOffCenterRH(
		XMVectorGetX(posMinView), XMVectorGetX(posMaxView),
		XMVectorGetY(posMinView), XMVectorGetY(posMaxView),
		-XMVectorGetZ(posMaxView), -XMVectorGetZ(posMinView));
	XMStoreFloat4x4(&m_matProj, matProj);
	
	XMMATRIX matWorldToClip = matView * matProj;
	XMStoreFloat4x4(&m_matWorldToClip, matWorldToClip);

	// Calculate matrix that maps to [0, 1] UV space instead of [-1, 1] clip space

	XMMATRIX matClipToUvzw(
		0.5f,  0,    0, 0,
		0,    -0.5f, 0, 0,
		0,     0,    1, 0,
		0.5f,  0.5f, 0, 1);

	XMMATRIX matWorldToUvzw = matWorldToClip * matClipToUvzw;
	XMStoreFloat4x4(&m_matWorldToUvzw, matWorldToUvzw);

	// Calculate inverse transpose matrix for transforming normals

	XMMATRIX matWorldToUvzNormal = XMMatrixTranspose(XMMatrixInverse(nullptr, XMMATRIX(
		matWorldToUvzw.r[0],
		matWorldToUvzw.r[1],
		matWorldToUvzw.r[2],
		XMVectorSelect(matWorldToUvzw.r[3], g_XMZero, g_XMSelect1110)
	)));
	XMStoreFloat4x4(&m_matWorldToUvzNormal, matWorldToUvzNormal);
}

void CShadowMap::BindRenderTarget(ID3D11DeviceContext * pCtx)
{
	D3D11_VIEWPORT viewport =
	{
		0.0f, 0.0f,						// left, top
		float(m_size), float(m_size),	// width, height
		0.0f, 1.0f,						// min/max depth
	};

	pCtx->RSSetViewports(1, &viewport);
	pCtx->OMSetRenderTargets(0, nullptr, m_pDsv);
}

XMFLOAT3 CShadowMap::CalcFilterUVZScale(float filterRadius) const
{
	// Expand the filter in the Z direction (this controls how far
	// the filter can tilt before it starts contracting).
	// Tuned empirically.
	float zScale = 4.0f;

	return XMFLOAT3(
			filterRadius / m_vecDiam.x,
			filterRadius / m_vecDiam.y,
			zScale * filterRadius / m_vecDiam.z);
}



// CVarShadowMap implementation

extern CMesh g_meshFullscreen;

CVarShadowMap::CVarShadowMap()
:	m_pRtv(nullptr),
	m_pSrv(nullptr),
	m_pRtvTemp(nullptr),
	m_pSrvTemp(nullptr),
	m_size(0),
	m_blurRadius(1.0f)
{
}

HRESULT CVarShadowMap::Init(unsigned int size, ID3D11Device * pDevice)
{
	HRESULT hr;

	// Create 2D textures for VSM and temp VSM for blur
	D3D11_TEXTURE2D_DESC texDesc =
	{
		size, size,						// width, height
		1, 1,							// mip levels, array size
		DXGI_FORMAT_R32G32_FLOAT,
		{ 1, 0 },						// multisample count, quality
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
		0,								// no cpu access
		0,								// no misc flags
	};
	ID3D11Texture2D * pTex = nullptr, * pTexTemp = nullptr;
	V_RETURN(pDevice->CreateTexture2D(&texDesc, nullptr, &pTex));
	V_RETURN(pDevice->CreateTexture2D(&texDesc, nullptr, &pTexTemp));

	// Create render target views
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc =
	{
		DXGI_FORMAT_R32G32_FLOAT,
		D3D11_RTV_DIMENSION_TEXTURE2D,
	};
	V_RETURN(pDevice->CreateRenderTargetView(pTex, &rtvDesc, &m_pRtv));
	V_RETURN(pDevice->CreateRenderTargetView(pTexTemp, &rtvDesc, &m_pRtvTemp));

	// Create shader resource views
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc =
	{
		DXGI_FORMAT_R32G32_FLOAT,
		D3D11_SRV_DIMENSION_TEXTURE2D,
	};
	srvDesc.Texture2D.MipLevels = 1;
	V_RETURN(pDevice->CreateShaderResourceView(pTex, &srvDesc, &m_pSrv));
	V_RETURN(pDevice->CreateShaderResourceView(pTexTemp, &srvDesc, &m_pSrvTemp));

	// The textures aren't needed any more
	SAFE_RELEASE(pTex);
	SAFE_RELEASE(pTexTemp);

	m_size = size;

	SetDebugName(m_pRtv, "VSM - RTV");
	SetDebugName(m_pSrv, "VSM - SRV");
	SetDebugName(m_pRtvTemp, "VSM - temp RTV");
	SetDebugName(m_pSrvTemp, "VSM - temp SRV");
	DebugPrintf(L"Created VSM, format R32G32_FLOAT, %d x %d\n", size, size);

	return S_OK;
}

void CVarShadowMap::UpdateFromShadowMap(const CShadowMap & shadow, ID3D11DeviceContext * pCtx)
{
	D3D11_VIEWPORT viewport =
	{
		0.0f, 0.0f,						// left, top
		float(m_size), float(m_size),	// width, height
		0.0f, 1.0f,						// min/max depth
	};

	pCtx->RSSetViewports(1, &viewport);
	pCtx->OMSetRenderTargets(1, &m_pRtv, nullptr);

	g_shdmgr.BindCreateVSM(pCtx, shadow.m_pSrv);
	g_meshFullscreen.Draw(pCtx);
}

void CVarShadowMap::GaussianBlur(ID3D11DeviceContext * pCtx)
{
	D3D11_VIEWPORT viewport =
	{
		0.0f, 0.0f,						// left, top
		float(m_size), float(m_size),	// width, height
		0.0f, 1.0f,						// min/max depth
	};

	pCtx->RSSetViewports(1, &viewport);
	pCtx->OMSetRenderTargets(1, &m_pRtvTemp, nullptr);

	g_shdmgr.BindGaussian(pCtx, m_pSrv, m_blurRadius, 0.0f);
	g_meshFullscreen.Draw(pCtx);

	pCtx->OMSetRenderTargets(0, nullptr, nullptr);
	g_shdmgr.BindGaussian(pCtx, m_pSrvTemp, 0.0f, m_blurRadius);
	pCtx->OMSetRenderTargets(1, &m_pRtv, nullptr);
	g_meshFullscreen.Draw(pCtx);
}



// CGpuProfiler implementation

CGpuProfiler g_gpuProfiler;

CGpuProfiler::CGpuProfiler()
:	m_iFrameQuery(0),
	m_iFrameCollect(-1),
	m_apQueryTsDisjoint(),
	m_apQueryTs(),
	m_fTsUsed(),
	m_adT(),
	m_gpuFrameTime(0.0f),
	m_adTAvg(),
	m_gpuFrameTimeAvg(0.0f),
	m_adTTotal(),
	m_gpuFrameTimeTotal(0.0f),
	m_frameCount(0),
	m_tBeginAvg(0)
{
}

HRESULT CGpuProfiler::Init(ID3D11Device * pDevice)
{
	HRESULT hr;

	// Create all the queries we'll need

	D3D11_QUERY_DESC queryDesc = { D3D11_QUERY_TIMESTAMP_DISJOINT, 0 };
	V_RETURN(pDevice->CreateQuery(&queryDesc, &m_apQueryTsDisjoint[0]));
	V_RETURN(pDevice->CreateQuery(&queryDesc, &m_apQueryTsDisjoint[1]));

	queryDesc.Query = D3D11_QUERY_TIMESTAMP;
	for (GTS gts = GTS_BeginFrame; gts < GTS_Max; gts = GTS(gts + 1))
	{
		V_RETURN(pDevice->CreateQuery(&queryDesc, &m_apQueryTs[gts][0]));
		V_RETURN(pDevice->CreateQuery(&queryDesc, &m_apQueryTs[gts][1]));
	}

	timeBeginPeriod(1);

	return S_OK;
}

void CGpuProfiler::Release()
{
	SAFE_RELEASE(m_apQueryTsDisjoint[0]);
	SAFE_RELEASE(m_apQueryTsDisjoint[1]);

	for (GTS gts = GTS_BeginFrame; gts < GTS_Max; gts = GTS(gts + 1))
	{
		SAFE_RELEASE(m_apQueryTs[gts][0]);
		SAFE_RELEASE(m_apQueryTs[gts][1]);
	}

	timeEndPeriod(1);
}

void CGpuProfiler::BeginFrame(ID3D11DeviceContext * pCtx)
{
	pCtx->Begin(m_apQueryTsDisjoint[m_iFrameQuery]);
	Timestamp(pCtx, GTS_BeginFrame);
}

void CGpuProfiler::Timestamp(ID3D11DeviceContext * pCtx, GTS gts)
{
	pCtx->End(m_apQueryTs[gts][m_iFrameQuery]);
	m_fTsUsed[gts][m_iFrameQuery] = true;
}

void CGpuProfiler::EndFrame(ID3D11DeviceContext * pCtx)
{
	Timestamp(pCtx, GTS_EndFrame);
	pCtx->End(m_apQueryTsDisjoint[m_iFrameQuery]);

	++m_iFrameQuery &= 1;
}

void CGpuProfiler::WaitForDataAndUpdate(ID3D11DeviceContext * pCtx)
{
	if (m_iFrameCollect < 0)
	{
		// Haven't run enough frames yet to have data
		m_iFrameCollect = 0;
		return;
	}

	int iFrame = m_iFrameCollect;
	++m_iFrameCollect &= 1;

	if (!m_fTsUsed[GTS_BeginFrame][iFrame] ||
		!m_fTsUsed[GTS_EndFrame][iFrame])
	{
		DebugPrintf(L"Couldn't analyze timestamp data because begin and end frame timestamps weren't recorded\n");
		return;
	}

	// Wait for data
	while (pCtx->GetData(m_apQueryTsDisjoint[iFrame], nullptr, 0, 0) == S_FALSE)
	{
		Sleep(1);
	}

	D3D11_QUERY_DATA_TIMESTAMP_DISJOINT timestampDisjoint;
	if (pCtx->GetData(m_apQueryTsDisjoint[iFrame], &timestampDisjoint, sizeof(timestampDisjoint), 0) != S_OK)
	{
		DebugPrintf(L"Couldn't retrieve timestamp disjoint query data\n");
		return;
	}

	if (timestampDisjoint.Disjoint)
	{
		// Throw out this frame's data
		DebugPrintf(L"Timestamps disjoint\n");
		return;
	}

	// Wait for data
	while (pCtx->GetData(m_apQueryTs[GTS_BeginFrame][iFrame], nullptr, 0, 0) == S_FALSE)
	{
		Sleep(1);
	}

	UINT64 timestampBeginFrame;
	if (pCtx->GetData(m_apQueryTs[GTS_BeginFrame][iFrame], &timestampBeginFrame, sizeof(UINT64), 0) != S_OK)
	{
		DebugPrintf(L"Couldn't retrieve timestamp query data for GTS %d\n", GTS_BeginFrame);
		return;
	}

	UINT64 timestampPrev = timestampBeginFrame;
	UINT64 timestampEndFrame = 0;
	for (GTS gts = GTS(GTS_BeginFrame + 1); gts < GTS_Max; gts = GTS(gts + 1))
	{
		UINT64 timestamp;
		if (m_fTsUsed[gts][iFrame])
		{
			// Wait for data
			while (pCtx->GetData(m_apQueryTs[gts][iFrame], nullptr, 0, 0) == S_FALSE)
			{
				Sleep(1);
			}

			if (pCtx->GetData(m_apQueryTs[gts][iFrame], &timestamp, sizeof(UINT64), 0) != S_OK)
			{
				DebugPrintf(L"Couldn't retrieve timestamp query data for GTS %d\n", gts);
				return;
			}
		}
		else
		{
			// Timestamp wasn't used in the frame we're collecting;
			// just pretend it took zero time
			timestamp = timestampPrev;
		}

		if (gts == GTS_EndFrame)
			timestampEndFrame = timestamp;

		m_adT[gts] = float(timestamp - timestampPrev) / float(timestampDisjoint.Frequency);
		timestampPrev = timestamp;

		m_adTTotal[gts] += m_adT[gts];

		// Reset timestamp-used flags for new frame
		m_fTsUsed[gts][iFrame] = false;
	}

	m_gpuFrameTime = float(timestampEndFrame - timestampBeginFrame) / float(timestampDisjoint.Frequency);
	m_gpuFrameTimeTotal += m_gpuFrameTime;

	++m_frameCount;
	DWORD curTime = timeGetTime();
	if (curTime > m_tBeginAvg + 500)
	{
		for (GTS gts = GTS_BeginFrame; gts < GTS_Max; gts = GTS(gts + 1))
		{
			m_adTAvg[gts] = m_adTTotal[gts] / m_frameCount;
			m_adTTotal[gts] = 0.0f;
		}

		m_gpuFrameTimeAvg = m_gpuFrameTimeTotal / m_frameCount;
		m_gpuFrameTimeTotal = 0.0f;

		m_frameCount = 0;
		m_tBeginAvg = curTime;
	}
}
