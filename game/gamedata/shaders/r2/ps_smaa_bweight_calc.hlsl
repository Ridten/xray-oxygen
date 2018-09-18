#include "common.h"

#define SMAA_HLSL_3 1
#define SMAA_RT_METRICS screen_res.zwxy
/*
#if !defined(AA_QUALITY)
	#define	SMAA_PRESET_LOW 1
#elif AA_QUALITY==1		// Low
	#define	SMAA_PRESET_LOW 1
#elif AA_QUALITY==2		// Medium
	#define	SMAA_PRESET_MEDIUM 1
#elif AA_QUALITY==3		// High
	#define	SMAA_PRESET_HIGH 1
#elif AA_QUALITY==4		// Extreme
*/
	#define	SMAA_PRESET_ULTRA 1
//#endif

#include "smaa.h"

struct _in
{
	float2	tc0: TEXCOORD0;
	float4	tc1: TEXCOORD1;
	float4	tc2: TEXCOORD2;
	float4	tc3: TEXCOORD3;
	float4	tc4: TEXCOORD4;
};

uniform sampler2D s_edgetex;
uniform sampler2D s_areatex;
uniform sampler2D s_searchtex;

float4 main(_in I) : COLOR0
{
	float4 offset[3] = {I.tc2, I.tc3, I.tc4};
	return SMAABlendingWeightCalculationPS(I.tc0, I.tc1, offset, s_edgetex, s_areatex, s_searchtex, 0);
};