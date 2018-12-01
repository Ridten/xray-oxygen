////////////////////////////////////
// MatthewKush to X-Ray Oxygen Project
// All credit goes to the author of "WickedEngine"

#include "common.h"

float3 tonemap(float3 hdr, float k = 0.25)
{
    float3 reinhard = hdr / (hdr + k);
    return reinhard * reinhard;
}

float3 inverseTonemap(float3 sdr, float k = 0.25)
{
    return k * (sdr + sqrt(sdr)) / (1.0 - sdr);
}

//note: you might need to change s_depth to s_position.z
float2 GetVelocity(in int2 pixel)
{
//#ifdef DILATE_VELOCITY_BEST_3X3 // search best velocity in 3x3 neighborhood

	float bestDepth = 1;
	int2 bestPixel = int2(0, 0);

	[unroll]
	for (int i = -1; i <= 1; ++i)
	{
		[unroll]
		for (int j = -1; j <= 1; ++j)
		{
			int2 curPixel = pixel + int2(i, j);
			float depth = s_depth[curPixel].x;
			[flatten]
			if (depth < bestDepth)
			{
				bestDepth = depth;
				bestPixel = curPixel;
			}
		}
	}

	return bestPixel;
}

// This hack can improve bright areas:
#define HDR_CORRECTION
float4 main(p_screen I) : SV_Target
{
	float2 velocity = GetVelocity(I.hpos.xy);
	float2 prevTC = I.tc0 + velocity;

	float4 neighborhood[9];
	neighborhood[0] = s_image[I.hpos.xy + float2(-1, -1)];
	neighborhood[1] = s_image[I.hpos.xy + float2(0, -1)];
	neighborhood[2] = s_image[I.hpos.xy + float2(1, -1)];
	neighborhood[3] = s_image[I.hpos.xy + float2(-1, 0)];
	neighborhood[4] = s_image[I.hpos.xy + float2(0, 0)]; // center
	neighborhood[5] = s_image[I.hpos.xy + float2(1, 0)];
	neighborhood[6] = s_image[I.hpos.xy + float2(-1, 1)];
	neighborhood[7] = s_image[I.hpos.xy + float2(0, 1)];
	neighborhood[8] = s_image[I.hpos.xy + float2(1, 1)];
	float4 neighborhoodMin = neighborhood[0];
	float4 neighborhoodMax = neighborhood[0];
	[unroll]
	for (uint i = 1; i < 9; ++i)
	{
		neighborhoodMin = min(neighborhoodMin, neighborhood[i]);
		neighborhoodMax = max(neighborhoodMax, neighborhood[i]);
	}

	// we cannot avoid the linear filter here because point sampling could sample irrelevant pixels but we try to correct it later:
	float4 history = s_image.SampleLevel(smp_rtlinear, prevTC, 0);

	// simple correction of image signal incoherency (eg. moving shadows or lighting changes):
	history = clamp(history, neighborhoodMin, neighborhoodMax);

	// our currently rendered frame sample:
	float4 current = neighborhood[4];

	// the linear filtering can cause blurry image, try to account for that:
	float subpixelCorrection = frac(max(abs(velocity.x)*screen_res.x, abs(velocity.y)*screen_res.y)) * 0.5f;

	// compute a nice blend factor:
	float blendfactor = saturate(lerp(0.05f, 0.8f, subpixelCorrection));

	// if information can not be found on the screen, revert to aliased image:
	blendfactor = any(prevTC - saturate(prevTC)) ? 1.0f : blendfactor;

#ifdef HDR_CORRECTION
	history.rgb = tonemap(history.rgb);
	current.rgb = tonemap(current.rgb);
#endif

	// do the temporal super sampling by linearly accumulating previous samples with the current one:
	float4 resolved = float4(lerp(history.rgb, current.rgb, blendfactor), 1);

#ifdef HDR_CORRECTION
	resolved.rgb = inverseTonemap(resolved.rgb);
#endif

	return resolved;
}