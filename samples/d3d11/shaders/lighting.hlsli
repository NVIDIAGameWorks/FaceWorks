//----------------------------------------------------------------------------------
// File:        FaceWorks/samples/sample_d3d11/shaders/lighting.hlsli
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

#ifndef LIGHTING_HLSLI
#define LIGHTING_HLSLI

#include "common.hlsli"
#include "tonemap.hlsli"
#include "GFSDK_FaceWorks.hlsli"



// Normal mapping

float3 UnpackNormal(
	float3 sample,
	float normalStrength)
{
	return lerp(float3(0, 0, 1),
				sample * 2.0 - 1.0,
				normalStrength);
}



// Shadow filtering, using variance shadow maps

float EvaluateShadowVSM(
	float4 uvzwShadow,
	float3 normalGeom)
{
	float3 uvzShadow = uvzwShadow.xyz / uvzwShadow.w;

	float2 vsmValue = g_texVSM.Sample(g_ssBilinearClamp, uvzShadow.xy);
	float mean = vsmValue.x;
	float variance = max(g_vsmMinVariance, vsmValue.y - mean*mean);

	return saturate(variance / (variance + square(uvzShadow.z - mean)));
}



// Diffuse lighting

float3 EvaluateDiffuseLight(
	float3 normalGeom,
	float3 normalShade,
	float shadow)
{
	// Directional light diffuse
	float NdotL = saturate(dot(normalShade, g_vecDirectionalLight));
	float3 rgbLightDiffuse = g_rgbDirectionalLight * (NdotL * shadow);

	// IBL diffuse
	rgbLightDiffuse += g_texCubeDiffuse.Sample(g_ssTrilinearRepeat, normalShade);

	return rgbLightDiffuse;
}

float3 EvaluateSSSDiffuseLight(
	float3 normalGeom,
	float3 normalShade,
	float3 normalBlurred,
	float shadow,
	float curvature,
	GFSDK_FaceWorks_CBData faceworksData)
{
	// Directional light diffuse
	float3 rgbSSS = GFSDK_FaceWorks_EvaluateSSSDirectLight(
						faceworksData,
						normalGeom, normalShade, normalBlurred,
						g_vecDirectionalLight, curvature,
						g_texCurvatureLUT, g_ssBilinearClamp);
	float3 rgbShadow = GFSDK_FaceWorks_EvaluateSSSShadow(
							faceworksData,
							normalGeom, g_vecDirectionalLight, shadow,
							g_texShadowLUT, g_ssBilinearClamp);
	float3 rgbLightDiffuse = g_rgbDirectionalLight * rgbSSS * rgbShadow;

	// IBL diffuse
	float3 normalAmbient0, normalAmbient1, normalAmbient2;
	GFSDK_FaceWorks_CalculateNormalsForAmbientLight(
		normalShade, normalBlurred,
		normalAmbient0, normalAmbient1, normalAmbient2);
	float3 rgbAmbient0 = g_texCubeDiffuse.Sample(g_ssTrilinearRepeat, normalAmbient0);
	float3 rgbAmbient1 = g_texCubeDiffuse.Sample(g_ssTrilinearRepeat, normalAmbient1);
	float3 rgbAmbient2 = g_texCubeDiffuse.Sample(g_ssTrilinearRepeat, normalAmbient2);
	rgbLightDiffuse += GFSDK_FaceWorks_EvaluateSSSAmbientLight(
							rgbAmbient0, rgbAmbient1, rgbAmbient2);

	return rgbLightDiffuse;
}



// Specular lighting

float3 EvaluateSpecularLight(
	float3 normalGeom,
	float3 normalShade,
	float3 vecCamera,
	float specReflectance,
	float gloss,
	float shadow)
{
	// Directional light spec

	float3 vecHalf = normalize(g_vecDirectionalLight + vecCamera);
	float NdotL = saturate(dot(normalShade, g_vecDirectionalLight));
	float NdotH = saturate(dot(normalShade, vecHalf));
	float LdotH = dot(g_vecDirectionalLight, vecHalf);
	float NdotV = saturate(dot(normalShade, vecCamera));
	float specPower = exp2(gloss * 13.0);

	// Evaluate NDF and visibility function:
	// Two-lobe Blinn-Phong, with double gloss on second lobe
	float specLobeBlend = 0.05;
	float specPower0 = specPower;
	float specPower1 = square(specPower);
	float ndf0 = pow(NdotH, specPower0) * (specPower0 + 2.0) * 0.5;
	float schlickSmithFactor0 = rsqrt(specPower0 * (3.14159 * 0.25) + (3.14159 * 0.5));
	float visibilityFn0 = 0.25 / (lerp(schlickSmithFactor0, 1, NdotL) *
									lerp(schlickSmithFactor0, 1, NdotV));
	float ndf1 = pow(NdotH, specPower1) * (specPower1 + 2.0) * 0.5;
	float schlickSmithFactor1 = rsqrt(specPower1 * (3.14159 * 0.25) + (3.14159 * 0.5));
	float visibilityFn1 = 0.25 / (lerp(schlickSmithFactor1, 1, NdotL) *
									lerp(schlickSmithFactor1, 1, NdotV));
	float ndfResult = lerp(ndf0 * visibilityFn0, ndf1 * visibilityFn1, specLobeBlend);

	float fresnel = lerp(specReflectance, 1.0, pow(1.0 - LdotH, 5.0));
	float specResult = ndfResult * fresnel;
	// Darken spec where the *geometric* NdotL gets too low -
	// avoids it showing up on bumps in shadowed areas
	float edgeDarken = saturate(5.0 * dot(normalGeom, g_vecDirectionalLight));
	float3 rgbLitSpecular = g_rgbDirectionalLight * (NdotL * edgeDarken * specResult * shadow);

	// IBL spec - again two-lobe
	float3 vecReflect = reflect(-vecCamera, normalShade);
	float gloss0 = gloss;
	float gloss1 = saturate(2.0 * gloss);
	float fresnelIBL0 = lerp(specReflectance, 1.0,
							pow(1.0 - NdotV, 5.0) / (-3.0 * gloss0 + 4.0));
	float mipLevel0 = -9.0 * gloss0 + 9.0;
	float3 iblSpec0 = fresnelIBL0 * g_texCubeSpec.SampleLevel(
										g_ssTrilinearRepeat, vecReflect, mipLevel0);
	float fresnelIBL1 = lerp(specReflectance, 1.0,
							pow(1.0 - NdotV, 5.0) / (-3.0 * gloss1 + 4.0));
	float mipLevel1 = -9.0 * gloss1 + 9.0;
	float3 iblSpec1 = fresnelIBL1 * g_texCubeSpec.SampleLevel(
										g_ssTrilinearRepeat, vecReflect, mipLevel1);
	rgbLitSpecular += lerp(iblSpec0, iblSpec1, specLobeBlend);

	return rgbLitSpecular;
}



// Master lighting routine

void LightingMegashader(
	in Vertex i_vtx,
	in float3 i_vecCamera,
	in float4 i_uvzwShadow,
	in float3 rgbDiffuse,
	in float3 normalTangent,
	in float3 normalTangentBlurred,
	in float specReflectance,
	in float gloss,
	in float3 rgbDeepScatter,
	in GFSDK_FaceWorks_CBData faceworksData,
	out float3 o_rgbLit,
	uniform bool useNormalMap,
	uniform bool useSSS,
	uniform bool useDeepScatter)
{
	float3 normalGeom = normalize(i_vtx.m_normal);
	float3 vecCamera = normalize(i_vecCamera);
	float2 uv = i_vtx.m_uv;

	float3 normalShade, normalBlurred;
	if (useNormalMap)
	{
		// Transform normal maps to world space

		float3x3 matTangentToWorld = float3x3(
										normalize(i_vtx.m_tangent),
										normalize(cross(normalGeom, i_vtx.m_tangent)),
										normalGeom);

		normalShade = normalize(mul(normalTangent, matTangentToWorld));

		if (useSSS || useDeepScatter)
		{
			normalBlurred = normalize(mul(normalTangentBlurred, matTangentToWorld));
		}
	}
	else
	{
		normalShade = normalGeom;
		normalBlurred = normalGeom;
	}

	// Evaluate shadow map
	float shadow = EvaluateShadowVSM(i_uvzwShadow, normalGeom);

	float3 rgbLitDiffuse;
	if (useSSS)
	{
		// Evaluate diffuse lighting
		float3 rgbDiffuseLight = EvaluateSSSDiffuseLight(
									normalGeom, normalShade, normalBlurred,
									shadow, i_vtx.m_curvature, faceworksData);
		rgbLitDiffuse = rgbDiffuseLight * rgbDiffuse;

		// Remap shadow to 1/3-as-wide penumbra to match shadow from LUT.
		shadow = GFSDK_FaceWorks_SharpenShadow(shadow, g_shadowSharpening);
	}
	else
	{
		// Remap shadow to 1/3-as-wide penumbra to match shadow in SSS case.
		shadow = GFSDK_FaceWorks_SharpenShadow(shadow, g_shadowSharpening);

		// Evaluate diffuse lighting
		float3 rgbDiffuseLight = EvaluateDiffuseLight(normalGeom, normalShade, shadow);
		rgbLitDiffuse = rgbDiffuseLight * rgbDiffuse;
	}

	// Evaluate specular lighting
	float3 rgbLitSpecular = EvaluateSpecularLight(
								normalGeom, normalShade, vecCamera,
								specReflectance, gloss,
								shadow);

	// Put it all together
	o_rgbLit = rgbLitDiffuse + rgbLitSpecular;

	if (useDeepScatter)
	{
		float3 uvzShadow = i_uvzwShadow.xyz / i_uvzwShadow.w;

		// Apply normal offset to avoid silhouette edge artifacts
		// !!!UNDONE: move this to vertex shader
		float3 normalShadow = mul(normalGeom, g_matWorldToUvzShadowNormal);
		uvzShadow += normalShadow * g_deepScatterNormalOffset;

		float thickness = GFSDK_FaceWorks_EstimateThicknessFromParallelShadowPoisson32(
							faceworksData,
							g_texShadowMap, g_ssBilinearClamp, uvzShadow);

		float deepScatterFactor = GFSDK_FaceWorks_EvaluateDeepScatterDirectLight(
										faceworksData,
										normalBlurred, g_vecDirectionalLight, thickness);
		rgbDeepScatter *= g_deepScatterIntensity;
		o_rgbLit += (g_deepScatterIntensity * deepScatterFactor) * rgbDeepScatter *
						rgbDiffuse * g_rgbDirectionalLight;
	}

	// Apply tonemapping to the result
	o_rgbLit = Tonemap(o_rgbLit);
}

#endif // LIGHTING_HLSLI
