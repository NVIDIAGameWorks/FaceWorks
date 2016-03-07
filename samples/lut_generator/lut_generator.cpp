//----------------------------------------------------------------------------------
// File:        FaceWorks/samples/lut_generator/lut_generator.cpp
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

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <ctime>
#include <vector>

#include <GFSDK_FaceWorks.h>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>



// Command-line processing

void PrintUsage()
{
	printf(
		"Usage: lut_generator [options]\n"
		"Options:\n"
		" -curvatureLUT FILENAME        Generate curvature LUT (.bmp format)\n"
		" -shadowLUT FILENAME           Generate shadow LUT (.bmp format)\n"
		"\n"
		"Curvature LUT options:\n"
		" -diffusionRadius FLOAT        Radius of diffusion profile in mm; default is 2.7\n"
		" -minCurvatureRadius FLOAT     Minimum radius of curvature in mm; default is 1.0\n"
		" -maxCurvatureRadius FLOAT     Maximum radius of curvature in mm; default is 100.0\n"
		" -width INT                    LUT width in pixels; default is 512\n"
		" -height INT                   LUT height in pixels; default is 512\n"
		"\n"
		"Shadow LUT options:\n"
		" -diffusionRadius FLOAT        Radius of diffusion profile in mm; default is 2.7\n"
		" -minShadowWidth FLOAT         Minimum penumbra width in mm; default is 8.0\n"
		" -maxShadowWidth FLOAT         Maximum penumbra width in mm; default is 100.0\n"
		" -shadowSharpening FLOAT       Shadow sharpening ratio; default is 10.0\n"
		" -width INT                    LUT width in pixels; default is 512\n"
		" -height INT                   LUT height in pixels; default is 512\n"
		"\n"
	);
}

bool ReadInt(
	const char ** argNameAndValue,
	int * pValue,
	int minValid = INT_MIN,
	int maxValid = INT_MAX)
{
	assert(minValid <= maxValid);

	const char * argName = argNameAndValue[0];
	const char * strValue = argNameAndValue[1];

	if (!strValue)
	{
		fprintf(stderr, "%s: missing argument; ignoring\n", argName);
		return false;
	}

	int value;
	if (!sscanf_s(strValue, "%d", &value))
	{
		fprintf(
			stderr,
			"%s: could not parse \"%s\" as an int; ignoring\n",
			argName,
			strValue);
		return false;
	}

	if (value < minValid)
	{
		fprintf(
			stderr,
			"%s: value must be at least %d; clamping\n",
			argName,
			minValid);
		value = minValid;
	}

	if (value > maxValid)
	{
		fprintf(
			stderr,
			"%s: value must be at most %d; clamping\n",
			argName,
			maxValid);
		value = maxValid;
	}

	*pValue = value;
	return true;
}

bool ReadFloat(
	const char ** argNameAndValue,
	float * pValue,
	float minValid = -FLT_MAX,
	float maxValid = FLT_MAX)
{
	assert(minValid <= maxValid);

	const char * argName = argNameAndValue[0];
	const char * strValue = argNameAndValue[1];

	if (!strValue)
	{
		fprintf(stderr, "%s: missing argument; ignoring\n", argName);
		return false;
	}

	float value;
	if (!sscanf_s(strValue, "%f", &value))
	{
		fprintf(
			stderr,
			"%s: could not parse \"%s\" as a float; ignoring\n",
			argName,
			strValue);
		return false;
	}

	if (value < minValid)
	{
		fprintf(
			stderr,
			"%s: value must be at least %.2g; clamping\n",
			argName,
			minValid);
		value = minValid;
	}

	if (value > maxValid)
	{
		fprintf(
			stderr,
			"%s: value must be at most %.2g; clamping\n",
			argName,
			maxValid);
		value = maxValid;
	}

	*pValue = value;
	return true;
}



int GenerateCurvatureLUT(
	const GFSDK_FaceWorks_CurvatureLUTConfig * pConfig,
	const char * strFilename);

int GenerateShadowLUT(
	const GFSDK_FaceWorks_ShadowLUTConfig * pConfig,
	const char * strFilename);



int main(int argc, const char ** argv)
{
	// Set default params
	GFSDK_FaceWorks_CurvatureLUTConfig curvatureConfig =
	{
		2.7f,				// m_diffusionRadius
		512, 512,			// m_texWidth, m_texHeight
		1.0f, 100.0f,		// m_curvatureRadiusMin, m_curvatureRadiusMax
	};
	GFSDK_FaceWorks_ShadowLUTConfig shadowConfig =
	{
		2.7f,				// m_diffusionRadius
		512, 512,			// m_texWidth, m_texHeight
		8.0f, 100.0f,		// m_shadowWidthMin, m_shadowWidthMax
		10.0f,				// m_shadowSharpening
	};
	const char * strCurvatureFilename = NULL;
	const char * strShadowFilename = NULL;

	// Parse command-line params
	for (int iArg = 1; iArg < argc; ++iArg)
	{
		if (_stricmp(argv[iArg], "-h") == 0 ||
			_stricmp(argv[iArg], "-help") == 0 ||
			_stricmp(argv[iArg], "--help") == 0 ||
			_stricmp(argv[iArg], "/?") == 0)
		{
			PrintUsage();
			return 0;
		}
		else if (_stricmp(argv[iArg], "-curvatureLUT") == 0)
		{
			strCurvatureFilename = argv[++iArg];
			if (!strCurvatureFilename)
				fprintf(stderr, "-curvatureLUT: filename expected\n");
		}
		else if (_stricmp(argv[iArg], "-shadowLUT") == 0)
		{
			strShadowFilename = argv[++iArg];
			if (!strShadowFilename)
				fprintf(stderr, "-shadowLUT: filename expected\n");
		}
		else if (_stricmp(argv[iArg], "-width") == 0)
		{
			int width;
			if (ReadInt(&argv[iArg++], &width, 1, 16384))
			{
				curvatureConfig.m_texWidth = width;
				shadowConfig.m_texWidth = width;
			}
		}
		else if (_stricmp(argv[iArg], "-height") == 0)
		{
			int height;
			if (ReadInt(&argv[iArg++], &height, 1, 16384))
			{
				curvatureConfig.m_texHeight = height;
				shadowConfig.m_texHeight = height;
			}
		}
		else if (_stricmp(argv[iArg], "-minCurvatureRadius") == 0)
		{
			ReadFloat(&argv[iArg++], &curvatureConfig.m_curvatureRadiusMin, FLT_MIN);
		}
		else if (_stricmp(argv[iArg], "-maxCurvatureRadius") == 0)
		{
			ReadFloat(&argv[iArg++], &curvatureConfig.m_curvatureRadiusMax, FLT_MIN);
		}
		else if (_stricmp(argv[iArg], "-minShadowWidth") == 0)
		{
			ReadFloat(&argv[iArg++], &shadowConfig.m_shadowWidthMin, FLT_MIN);
		}
		else if (_stricmp(argv[iArg], "-maxShadowWidth") == 0)
		{
			ReadFloat(&argv[iArg++], &shadowConfig.m_shadowWidthMax, FLT_MIN);
		}
		else if (_stricmp(argv[iArg], "-shadowSharpening") == 0)
		{
			ReadFloat(&argv[iArg++], &shadowConfig.m_shadowSharpening, 1.0f);
		}
		else
		{
			fprintf(
				stderr,
				"Warning: unrecognized command-line parameter \"%s\"; ignoring\n",
				argv[iArg]);
		}
	}

	if (!strCurvatureFilename && !strShadowFilename)
	{
		PrintUsage();
		return 0;
	}

	if (strCurvatureFilename)
	{
		if (curvatureConfig.m_curvatureRadiusMin > curvatureConfig.m_curvatureRadiusMax)
		{
			fprintf(stderr, "Warning: minCurvatureRadius > maxCurvatureRadius; swapping\n");
			std::swap(curvatureConfig.m_curvatureRadiusMin, curvatureConfig.m_curvatureRadiusMax);
		}

		int res = GenerateCurvatureLUT(&curvatureConfig, strCurvatureFilename);
		if (res != 0)
			return 1;
	}

	if (strShadowFilename)
	{
		if (shadowConfig.m_shadowWidthMin > shadowConfig.m_shadowWidthMax)
		{
			fprintf(stderr, "Warning: minShadowWidth > maxShadowWidth; swapping\n");
			std::swap(shadowConfig.m_shadowWidthMin, shadowConfig.m_shadowWidthMax);
		}

		int res = GenerateShadowLUT(&shadowConfig, strShadowFilename);
		if (res != 0)
			return 1;
	}

	return 0;
}



int WriteBMP(
	int width, int height,
	void * pPixelsRGBA,
	const char * strFilename)
{
	FILE * pFile;
	if (fopen_s(&pFile, strFilename, "wb") != 0 || !pFile)
	{
		fprintf(stderr, "Error: couldn't open %s for writing\n", strFilename);
		return 1;
	}

	BITMAPFILEHEADER bmpFileHeader =
	{
		0x4d42,
		0, 0, 0,
		sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER),
	};
	fwrite(&bmpFileHeader, sizeof(bmpFileHeader), 1, pFile);

	BITMAPINFOHEADER bmpInfoHeader =
	{
		sizeof(BITMAPINFOHEADER),
		width, -height,		// negative height means top-down BMP
		1,					// planes
		24,					// bits per pixel
	};
	fwrite(&bmpInfoHeader, sizeof(bmpInfoHeader), 1, pFile);

	// Convert RGBA to BGR pixel order
	unsigned char * pRGBA = static_cast<unsigned char *>(pPixelsRGBA);
	int pixelCount = width * height;
	for (int i = 0; i < pixelCount; ++i)
	{
		unsigned char r = *(pRGBA++);
		unsigned char g = *(pRGBA++);
		unsigned char b = *(pRGBA++);
		pRGBA++;	// skip alpha

		fputc(b, pFile);
		fputc(g, pFile);
		fputc(r, pFile);
	}

	fclose(pFile);
	printf("Wrote %s\n", strFilename);

	return 0;
}



int GenerateCurvatureLUT(
	const GFSDK_FaceWorks_CurvatureLUTConfig * pConfig,
	const char * strFilename)
{
	printf("Generating curvature LUT...\n");
	clock_t start = clock();

	std::vector<unsigned char> pixels;
	pixels.resize(GFSDK_FaceWorks_CalculateCurvatureLUTSizeBytes(pConfig));

	GFSDK_FaceWorks_ErrorBlob errorBlob = {};
	GFSDK_FaceWorks_Result res = GFSDK_FaceWorks_GenerateCurvatureLUT(pConfig, &pixels[0], &errorBlob);
	if (res != GFSDK_FaceWorks_OK)
	{
		fprintf(stderr, "GFSDK_FaceWorks_GenerateCurvatureLUT() failed:\n%s", errorBlob.m_msg);
		GFSDK_FaceWorks_FreeErrorBlob(&errorBlob);
		return 1;
	}

	clock_t clocks = clock() - start;
	printf("Done in %0.3f seconds\n", float(clocks) / float(CLOCKS_PER_SEC));

	return WriteBMP(
			pConfig->m_texWidth,
			pConfig->m_texHeight,
			&pixels[0],
			strFilename);
}



int GenerateShadowLUT(
	const GFSDK_FaceWorks_ShadowLUTConfig * pConfig,
	const char * strFilename)
{
	printf("Generating shadow LUT...\n");
	clock_t start = clock();

	std::vector<unsigned char> pixels;
	pixels.resize(GFSDK_FaceWorks_CalculateShadowLUTSizeBytes(pConfig));

	GFSDK_FaceWorks_ErrorBlob errorBlob = {};
	GFSDK_FaceWorks_Result res = GFSDK_FaceWorks_GenerateShadowLUT(pConfig, &pixels[0], &errorBlob);
	if (res != GFSDK_FaceWorks_OK)
	{
		fprintf(stderr, "GFSDK_FaceWorks_GenerateShadowLUT() failed:\n%s", errorBlob.m_msg);
		GFSDK_FaceWorks_FreeErrorBlob(&errorBlob);
		return 1;
	}

	clock_t clocks = clock() - start;
	printf("Done in %0.3f seconds\n", float(clocks) / float(CLOCKS_PER_SEC));

	return WriteBMP(
			pConfig->m_texWidth,
			pConfig->m_texHeight,
			&pixels[0],
			strFilename);
}
