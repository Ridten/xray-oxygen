#include "stdafx.h"

static void set_viewport(ID3D10Device *dev, u32 w, u32 h)
{
	static D3D10_VIEWPORT viewport[1] =
	{
		0, 0, (UINT)w, (UINT)h, 0.f, 1.f
	};
	dev->RSSetViewports(1, viewport);
}

void CRenderTarget::phase_ssao	()
{
	u32	Offset	= 0;

	FLOAT ColorRGBA[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	HW.pDevice->ClearRenderTargetView(rt_ssao_temp->pRT, ColorRGBA);
	
	// low/hi RTs
	if( !RImplementation.o.dx10_msaa )
	{
		u_setrt				( rt_ssao_temp,0,0,0/*HW.pBaseZB*/ );
	}
	else
	{
		u_setrt				( rt_ssao_temp, 0, 0, 0/*RImplementation.Target->rt_MSAADepth->pZRT*/ );
	}

	RCache.set_Stencil	(FALSE);

	// Compute params
	float		fSSAONoise = 2.0f;
	fSSAONoise *= tan(deg2rad(67.5f));
	fSSAONoise /= tan(deg2rad(Device.fFOV));

	float		fSSAOKernelSize = 150.0f;
	fSSAOKernelSize *= tan(deg2rad(67.5f));
	fSSAOKernelSize /= tan(deg2rad(Device.fFOV));

	// Fill VB
	float	scale_X				= float(Device.dwWidth)	* 0.5f / float(TEX_jitter);
	float	scale_Y				= float(Device.dwHeight) * 0.5f / float(TEX_jitter);

	u32 _w = Device.dwWidth/2;
	u32 _h = Device.dwHeight/2;

	set_viewport(HW.pDevice, _w, _h);

	// Fill vertex buffer
	FVF::TL* pv					= (FVF::TL*)	RCache.Vertex.Lock	(4,g_combine->vb_stride,Offset);
	pv->set						( -1,  1, 0, 1, 0,		0,	scale_Y	);	pv++;
	pv->set						( -1, -1, 0, 0, 0,		0,		  0	);	pv++;
	pv->set						(  1,  1, 1, 1, 0, scale_X,	scale_Y	);	pv++;
	pv->set						(  1, -1, 1, 0, 0, scale_X,		  0	);	pv++;
	RCache.Vertex.Unlock		(4,g_combine->vb_stride);

	// Draw
	RCache.set_Element			(s_ssao->E[0]	);
	RCache.set_Geometry			(g_combine		);

	RCache.set_c				("ssao_noise_tile_factor",	fSSAONoise	);
	RCache.set_c				("ssao_kernel_size",		fSSAOKernelSize	);
	RCache.set_c				("resolution", float(_w), float(_h), 1.0f / float(_w), 1.0f / float(_h) );


	if( !RImplementation.o.dx10_msaa )
		RCache.Render				(D3DPT_TRIANGLELIST,Offset,0,4,0,2);
	else
	{
		RCache.Render				(D3DPT_TRIANGLELIST,Offset,0,4,0,2);
	}  

	set_viewport(HW.pDevice, Device.dwWidth, Device.dwHeight);

	RCache.set_Stencil	(FALSE);
}


void CRenderTarget::phase_downsamp	()
{
	u32			Offset = 0;

    u_setrt( rt_half_depth,0,0,0);
   	FLOAT ColorRGBA[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    HW.pContext->ClearRenderTargetView(rt_half_depth->pRT, ColorRGBA);
	u32 w = Device.dwWidth;
	u32 h = Device.dwHeight;

	if (RImplementation.o.ssao_half_data)
	{
		set_viewport(HW.pDevice, Device.dwWidth/2, Device.dwHeight/2);
		w /= 2;
		h /= 2;
	}

	RCache.set_Stencil	(FALSE);

	{
		// Fill VB
		float	scale_X				= float(w)	/ float(TEX_jitter);
		float	scale_Y				= float(h)  / float(TEX_jitter);

		// Fill vertex buffer
		FVF::TL* pv					= (FVF::TL*)	RCache.Vertex.Lock	(4,g_combine->vb_stride,Offset);
		pv->set						( -1,  1, 0, 1, 0,		0,	scale_Y	);	pv++;
		pv->set						( -1, -1, 0, 0, 0,		0,		  0	);	pv++;
		pv->set						(  1,  1, 1, 1, 0, scale_X,	scale_Y	);	pv++;
		pv->set						(  1, -1, 1, 0, 0, scale_X,		  0	);	pv++;
		RCache.Vertex.Unlock		(4,g_combine->vb_stride);

		// Draw
		RCache.set_Element			(s_ssao->E[1]	);
		RCache.set_Geometry			(g_combine		);

		RCache.Render				(D3DPT_TRIANGLELIST,Offset,0,4,0,2);
	}

	if (RImplementation.o.ssao_half_data)
		set_viewport(HW.pDevice, Device.dwWidth, Device.dwHeight);
}