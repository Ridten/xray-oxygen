#include "stdafx.h"

void	CRenderTarget::phase_smap_direct		(light* L, u32 sub_phase)
{
	// Targets
	if (RImplementation.o.HW_smap)		u_setrt	(rt_smap_surf, NULL, NULL, rt_smap_depth->pRT);
	else								u_setrt	(rt_smap_surf, NULL, NULL, rt_smap_ZB);

	// Clear
	if (SE_SUN_NEAR==sub_phase)			{
		// optimized clear
		D3DRECT		R;
		R.x1		= L->X.D.minX;
		R.x2		= L->X.D.maxX;
		R.y1		= L->X.D.minY;
		R.y2		= L->X.D.maxY;
		RCache.Clear					(1L, &R, D3DCLEAR_ZBUFFER, 0xFFFFFFFF, 1.0f, 0L);
	} else {
		// full-clear
		RCache.Clear					(0L, nullptr, D3DCLEAR_ZBUFFER, 0xFFFFFFFF, 1.0f, 0L);
	}

	// Stencil	- disable
	RCache.set_Stencil					( FALSE );

	//	Cull always CCW. If you want to revert to previouse solution, please, revert bias setup/
	RCache.set_CullMode			( CULL_CCW	);	// near
	if (RImplementation.o.HW_smap)		RCache.set_ColorWriteEnable	( FALSE		);
	else								RCache.set_ColorWriteEnable	( );
}

void	CRenderTarget::phase_smap_direct_tsh	(light* L, u32 sub_phase)
{
	VERIFY								(RImplementation.o.Tshadows);
	u32		_clr						= 0xffffffff;
	RCache.set_ColorWriteEnable			();
	RCache.Clear						(0L, nullptr, D3DCLEAR_TARGET, _clr, 1.0f, 0L);
}
