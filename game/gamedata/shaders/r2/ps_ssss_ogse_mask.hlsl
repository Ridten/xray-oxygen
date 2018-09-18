#include "common.h"
#include "ogse_config.h"
#include "ogse_functions.h"

float4 main(p_screen I) : COLOR0
{
	float4 depth;
	depth.x = get_depth(I.tc0.xy + float2( 0.0f, 1.00f) * screen_res.zw);
	depth.y = get_depth(I.tc0.xy + float2( 1.0f, 0.65f) * screen_res.zw);
	depth.z = get_depth(I.tc0.xy + float2(-1.0f, 0.65f) * screen_res.zw);

	// The sky depth value is 0 since we're rendering it without z-test
	if (depth.x < 0.0001f) depth.x = SKY_DEPTH;
	if (depth.y < 0.0001f) depth.y = SKY_DEPTH;
	if (depth.z < 0.0001f) depth.z = SKY_DEPTH;
	
	float4 sceneDepth;
	sceneDepth.x = normalize_depth(depth.x)*is_not_sky(depth.x);
	sceneDepth.y = normalize_depth(depth.y)*is_not_sky(depth.y);
	sceneDepth.z = normalize_depth(depth.z)*is_not_sky(depth.z);
	sceneDepth.w = (sceneDepth.x + sceneDepth.y + sceneDepth.z) * 0.333f;
	
	depth.w = saturate(1.0f - sceneDepth.w*1000.0f);
	
	float4 Color = float4(depth.www, sceneDepth.w);	
	return Color;
}
