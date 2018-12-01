﻿#include "stdafx.h"
#include "../../xrEngine/igame_persistent.h"
#include "../../xrEngine/DirectXMathExternal.h"
#include "../xrRender/FBasicVisual.h"
#include "../../xrEngine/customhud.h"
#include "../../xrEngine/xr_object.h"

#include "../xrRender/QueryHelper.h"

IC	bool	pred_sp_sort	(ISpatial*	_1, ISpatial* _2)
{
	float	d1		= _1->spatial.sphere.P.distance_to_sqr	(Device.vCameraPosition);
	float	d2		= _2->spatial.sphere.P.distance_to_sqr	(Device.vCameraPosition);
	return	d1<d2	;
}
//MatthewKush to all: I was completely wrong before
//I shouldn't smoke so much cheeba
//Current goal: deffered lighting > deffered shading > forward+ shading
void CRender::render_main	(Fmatrix&	m_ViewProjection, bool _fportals)
{
	PIX_EVENT(render_main);
	marker					++;

	// Calculate sector(s) and their objects
	if (pLastSector)	
	{
		//!!! BECAUSE OF PARALLEL HOM RENDERING TRY TO DELAY ACCESS TO HOM AS MUCH AS POSSIBLE
		{
			// Traverse object database
			g_SpatialSpace->q_frustum
				(
				lstRenderables,
				ISpatial_DB::O_ORDERED,
				STYPE_RENDERABLE + STYPE_LIGHTSOURCE,
				ViewBase
				);

			// (almost) Exact sorting order (front-to-back)
			std::sort			(lstRenderables.begin(),lstRenderables.end(),pred_sp_sort);

			// Determine visibility for dynamic part of scene
			set_Object							(nullptr);
			u32 uID_LTRACK						= 0xffffffff;
			if (phase==PHASE_NORMAL)			{
				uLastLTRACK	++;
				if (!lstRenderables.empty())		uID_LTRACK	= uLastLTRACK%lstRenderables.size();

				// update light-vis for current entity / actor
				CObject*	O					= g_pGameLevel->CurrentViewEntity();
				if (O)		{
					CROS_impl*	R					= (CROS_impl*) O->ROS();
					if (R)		R->update			(O);
				}

				// update light-vis for selected entity
				// track lighting environment
				if (!lstRenderables.empty())		{
					IRenderable*	renderable		= lstRenderables[uID_LTRACK]->dcast_Renderable	();
					if (renderable)	{
						CROS_impl*		T = (CROS_impl*)renderable->renderable_ROS	();
						if (T)			T->update	(renderable);
					}
				}
			}
		}

		// Traverse sector/portal structure
		PortalTraverser.traverse	
			(
			pLastSector,
			ViewBase,
			Device.vCameraPosition,
			m_ViewProjection,
			CPortalTraverser::VQ_HOM + CPortalTraverser::VQ_SSA + CPortalTraverser::VQ_FADE
			//. disabled scissoring (HW.Caps.bScissor?CPortalTraverser::VQ_SCISSOR:0)	// generate scissoring info
			);

		// Determine visibility for static geometry hierrarhy
		for (auto & r_sector : PortalTraverser.r_sectors)
		{
			CSector*	sector		= (CSector*)r_sector;
			dxRender_Visual*	root	= sector->root();
			for (auto & r_frustum : sector->r_frustums)	{
				set_Frustum			(&r_frustum);
				add_Geometry		(root);
			}
		}

		// Traverse frustums
		for (ISpatial*	spatial : lstRenderables)
		{
			spatial->spatial_updatesector	();
			CSector*	sector		= (CSector*)spatial->spatial.sector;
			if	(nullptr==sector)										continue;	// disassociated from S/P structure

			if (spatial->spatial.type & STYPE_LIGHTSOURCE)		{
				// lightsource
				light*			L				= (light*)	(spatial->dcast_Light());
				VERIFY							(L);
				float	lod		= L->get_LOD	();
				if (lod>EPS_L)	{
					vis_data&		vis		= L->get_homdata	( );
					if	(HOM.visible(vis))	Lights.add_light	(L);
				}
				continue					;
			}

			if	(PortalTraverser.i_marker != sector->r_marker)	continue;	// inactive (untouched) sector
			for (CFrustum&	view : sector->r_frustums)	{
				if (!view.testSphere_dirty(spatial->spatial.sphere.P,spatial->spatial.sphere.R))	continue;

				if (spatial->spatial.type & STYPE_RENDERABLE)
				{
					// renderable
					IRenderable* renderable	= spatial->dcast_Renderable();
					bool bSphere = view.testSphere_dirty(spatial->spatial.sphere.P, spatial->spatial.sphere.R);

					if (!renderable)
					{
						if (ps_r_flags.is(R_FLAG_GLOW_USE)) continue;
						CGlow* pGlow = dynamic_cast<CGlow*>(spatial);
						VERIFY2(pGlow, "Glow don't created!");

						if (bSphere)
							Glows->add(pGlow);
						else if(pGlow)
							pGlow->hide_glow();
					}
					else if (bSphere)
					{
						// Occlusion
						//	casting is faster then using getVis method
						vis_data&		v_orig			= ((dxRender_Visual*)renderable->renderable.visual)->vis;
						vis_data		v_copy			= v_orig;
						v_copy.box.xform				(renderable->renderable.xform);
						BOOL			bVisible		= HOM.visible(v_copy);
						v_orig.marker					= v_copy.marker;
						v_orig.accept_frame				= v_copy.accept_frame;
						v_orig.hom_frame				= v_copy.hom_frame;
						v_orig.hom_tested				= v_copy.hom_tested;
						if (!bVisible)					break;	// exit loop on frustums

						// Rendering
						set_Object						(renderable);
						renderable->renderable_Render	();
						set_Object						(nullptr);
					}
				}
				break;	// exit loop on frustums
			}
		}
	}
	else
	{
		set_Object(nullptr);
	}
	// Render HUD Mesh
	if (g_pGameLevel && g_hud && (phase == PHASE_NORMAL))
		g_hud->Render_Last();
}

void CRender::render_menu	()
{
	PIX_EVENT(render_menu);
	//	Globals
	RCache.set_CullMode				(CULL_CCW);
	RCache.set_Stencil				(FALSE);
	RCache.set_ColorWriteEnable();

	// Main Render
	{
		Target->u_setrt(Target->rt_Generic_0,nullptr,nullptr,HW.pBaseZB);		// LDR RT
		g_pGamePersistent->OnRenderPPUI_main()	;	// PP-UI
	}

	// Distort
	{
		FLOAT ColorRGBA[4] = {127.0f/255.0f, 127.0f/255.0f, 0.0f, 127.0f/255.0f};
		Target->u_setrt(Target->rt_Generic_1,nullptr,nullptr,HW.pBaseZB);		// Now RT is a distortion mask
		HW.pContext->ClearRenderTargetView(Target->rt_Generic_1->pRT, ColorRGBA);		
		g_pGamePersistent->OnRenderPPUI_PP	()	;	// PP-UI
	}

	// Actual Display
	Target->u_setrt					( Device.dwWidth,Device.dwHeight,HW.pBaseRT,nullptr,nullptr,HW.pBaseZB);
	RCache.set_Shader				( Target->s_menu	);
	RCache.set_Geometry				( Target->g_menu	);

	Fvector2						p0,p1;
	u32								Offset;
	u32		C						= color_rgba	(255,255,255,255);
	float	_w						= float(Device.dwWidth);
	float	_h						= float(Device.dwHeight);
	float	d_Z						= EPS_S;
	float	d_W						= 1.f;
	p0.set							(.5f/_w, .5f/_h);
	p1.set							((_w+.5f)/_w, (_h+.5f)/_h );

	FVF::TL* pv						= (FVF::TL*) RCache.Vertex.Lock	(4,Target->g_menu->vb_stride,Offset);
	pv->set							(EPS,			float(_h+EPS),	d_Z,	d_W, C, p0.x, p1.y);	pv++;
	pv->set							(EPS,			EPS,			d_Z,	d_W, C, p0.x, p0.y);	pv++;
	pv->set							(float(_w+EPS),	float(_h+EPS),	d_Z,	d_W, C, p1.x, p1.y);	pv++;
	pv->set							(float(_w+EPS),	EPS,			d_Z,	d_W, C, p1.x, p0.y);	pv++;
	RCache.Vertex.Unlock			(4,Target->g_menu->vb_stride);
	RCache.Render					(D3DPT_TRIANGLELIST,Offset,0,4,0,2);
}

void CRender::Render		()
{
	PIX_EVENT(CRender_Render);

	VERIFY					(0==mapDistort.size());

	rmNormal();

	bool	_menu_pp		= g_pGamePersistent?g_pGamePersistent->OnRenderPPUI_query():false;
	if (_menu_pp)			{
		render_menu			()	;
		return					;
	};

	IMainMenu*	pMainMenu = g_pGamePersistent?g_pGamePersistent->m_pMainMenu:nullptr;
	bool	bMenu = pMainMenu?pMainMenu->CanSkipSceneRendering():false;

	if( !(g_pGameLevel && g_hud)
		|| bMenu)	
	{
		Target->u_setrt				( Device.dwWidth,Device.dwHeight,HW.pBaseRT,nullptr,nullptr,HW.pBaseZB);
		return;
	}

	if( m_bFirstFrameAfterReset )
	{
		m_bFirstFrameAfterReset = false;
		return;
	}

	// Configure
	RImplementation.o.distortion				= FALSE;		// disable distorion
	Fcolor					sun_color			= ((light*)Lights.sun._get())->color;
	BOOL					bSUN				= ps_r_flags.test(R_FLAG_SUN) && (u_diffuse2s(sun_color.r,sun_color.g,sun_color.b)>EPS) && !strstr(Core.Params, "-render_for_weak_systems");

	// HOM
	ViewBase.CreateFromMatrix					(CastToGSCMatrix(Device.mFullTransform), FRUSTUM_P_LRTB + FRUSTUM_P_FAR);
	View										= nullptr;

	Target->phase_scene_prepare					();
    //RCache.set_ZB( RImplementation.Target->rt_Depth->pZRT ); //NOT EVEN a depth prepass :P
	//*******
	// Sync point
	Device.Statistic->RenderDUMP_Wait_S.Begin	();
		
	CTimer	T;							T.Start	();
	BOOL	result						= FALSE;
	HRESULT	hr							= S_FALSE;
	while	((hr=GetData (q_sync_point[q_sync_count], &result,sizeof(result)))==S_FALSE) 
	{
		if (!SwitchToThread())			Sleep(ps_r_wait_sleep);
		if (T.GetElapsed_ms() > 500)	{
			result	= FALSE;
			break;
		}
	}
	
	Device.Statistic->RenderDUMP_Wait_S.End		();
	q_sync_count								= (q_sync_count+1)%HW.Caps.iGPUNum;
	CHK_DX										(EndQuery(q_sync_point[q_sync_count]));

	//******* Main calc - DEFERRER RENDERER
	// Main calc
	Device.Statistic->RenderCALC.Begin			();
	r_pmask										(true,false,true);	// enable priority "0",+ capture wmarks
	if (bSUN)									set_Recorder	(&main_coarse_structure);
	else										set_Recorder	(nullptr);
	phase										= PHASE_NORMAL;
	render_main									(CastToGSCMatrix(Device.mFullTransform),true);
	set_Recorder								(nullptr);
	r_pmask										(true,false);	// disable priority "1"
	Device.Statistic->RenderCALC.End			();

	BOOL split_the_scene_to_minimize_wait = (ps_r_flags.test(R_FLAG_EXP_SPLIT_SCENE));

	{
		PIX_EVENT(DEFER_PART0_NO_SPLIT);

		// level, DO NOT SPLIT
		Target->phase_scene_begin				();
		r_dsgraph_render_hud					();
		r_dsgraph_render_graph					(0);
		r_dsgraph_render_lods					(true,true);
		if(Details)	Details->Render				();
		Target->phase_scene_end					();
	} 

	//******* Occlusion testing of volume-limited light-sources
	Target->phase_occq							();
	LP_normal.clear								();
	LP_pending.clear							();
   if( RImplementation.o.dx10_msaa )
      RCache.set_ZB( RImplementation.Target->rt_MSAADepth->pZRT );
	{
		PIX_EVENT(DEFER_TEST_LIGHT_VIS);

		// perform tests
		size_t	count		= 0;
		light_Package&	LP	= Lights.package;

		// stats
		stats.l_shadowed	= LP.v_shadowed.size();
		stats.l_unshadowed	= LP.v_point.size() + LP.v_spot.size();
		stats.l_total		= stats.l_shadowed + stats.l_unshadowed;

		// perform tests
		count				= std::max	(count,LP.v_point.size());
		count				= std::max	(count,LP.v_spot.size());
		count				= std::max	(count,LP.v_shadowed.size());
		for (u32 it=0; it<count; it++)	{
			if (it<LP.v_point.size())		{
				light*	L			= LP.v_point	[it];
				L->vis_prepare		();
				if (L->vis.pending)	LP_pending.v_point.push_back	(L);
				else				LP_normal.v_point.push_back		(L);
			}
			if (it<LP.v_spot.size())		{
				light*	L			= LP.v_spot		[it];
				L->vis_prepare		();
				if (L->vis.pending)	LP_pending.v_spot.push_back		(L);
				else				LP_normal.v_spot.push_back		(L);
			}
			if (it<LP.v_shadowed.size())	{
				light*	L			= LP.v_shadowed	[it];
				L->vis_prepare		();
				if (L->vis.pending)	LP_pending.v_shadowed.push_back	(L);
				else				LP_normal.v_shadowed.push_back	(L);
			}
		}
	}
	LP_normal.sort							();
	LP_pending.sort							();


	if (g_hud && g_hud->RenderActiveItemUIQuery())
	{
		Target->phase_wallmarks();
		r_dsgraph_render_hud_ui();
	}

	// Wall marks
	if (Wallmarks)	
	{
		PIX_EVENT(DEFER_WALLMARKS);
		Target->phase_wallmarks					();
		Wallmarks->Render						();				// wallmarks has priority as normal geometry
	}

	// Update incremental shadowmap-visibility solver
	{
		PIX_EVENT(DEFER_FLUSH_OCCLUSION);
		u32 it=0;
		for (it=0; it<Lights_LastFrame.size(); it++)	{
			if (0==Lights_LastFrame[it])	continue	;
			try {
				Lights_LastFrame[it]->svis.flushoccq()	;
			} catch (...)
			{
				Msg	("! Failed to flush-OCCq on light [%d] %X",it,*(u32*)(&Lights_LastFrame[it]));
			}
		}
		Lights_LastFrame.clear	();
	}

    // full screen pass to mark msaa-edge pixels in highest stencil bit
    if (RImplementation.o.dx10_msaa)
    {
        PIX_EVENT(MARK_MSAA_EDGES);
        Target->mark_msaa_edges();
    }

    //	TODO: DX10: Implement DX10 rain.
    if (ps_r3_flags.test(R3_FLAG_DYN_WET_SURF))
    {
        PIX_EVENT(DEFER_RAIN);
        render_rain();
    }

	// Directional light - fucking sun
	if (bSUN)	
	{
		PIX_EVENT(DEFER_SUN);
		RImplementation.stats.l_visible		++;
		if (!ps_r_flags.is(R_FLAG_SUN_OLD))
			render_sun_cascades					();
		else
		{
			render_sun_near						();
			render_sun							();
			render_sun_filtered					();
		}
		Target->accum_direct_blend			();
	}

	{
		PIX_EVENT(DEFER_SELF_ILLUM);
		Target->phase_accumulator			();

		// Render emissive geometry, stencil - write 0x0 at pixel pos
		RCache.set_xform_project			(CastToGSCMatrix(Device.mProject)); 
		RCache.set_xform_view				(CastToGSCMatrix(Device.mView));

		// Stencil - write 0x1 at pixel pos - 
      if( !RImplementation.o.dx10_msaa )
		   RCache.set_Stencil					( TRUE,D3DCMP_ALWAYS,0x01,0xff,0xff,D3DSTENCILOP_KEEP,D3DSTENCILOP_REPLACE,D3DSTENCILOP_KEEP);
      else
		   RCache.set_Stencil					( TRUE,D3DCMP_ALWAYS,0x01,0xff,0x7f,D3DSTENCILOP_KEEP,D3DSTENCILOP_REPLACE,D3DSTENCILOP_KEEP);
		RCache.set_CullMode					(CULL_CCW);
		RCache.set_ColorWriteEnable			();
		RImplementation.r_dsgraph_render_emissive();

		PIX_EVENT(DEFER_LIGHT_NO_OCCQ);        
		HOM.Disable								();
		render_lights							(LP_normal);  
        
		PIX_EVENT(DEFER_LIGHT_OCCQ);
		render_lights							(LP_pending);
	}
    
	// Postprocess
	{
		PIX_EVENT(DEFER_LIGHT_COMBINE);
		Target->phase_combine					();
	}

	VERIFY	(0==mapDistort.size());
}

void CRender::render_forward				()
{
	VERIFY	(0==mapDistort.size());
	RImplementation.o.distortion				= RImplementation.o.distortion_enabled;	// enable distorion

	//******* Main render - second order geometry (the one, that doesn't support deffering)
	//.todo: should be done inside "combine" with estimation of of luminance, tone-mapping, etc.
	{
		// level
		r_pmask									(false,true);			// enable priority "1"
		phase									= PHASE_NORMAL;
		render_main								(CastToGSCMatrix(Device.mFullTransform),false);//

		//	Igor: we don't want to render old lods on next frame.
		mapLOD.clear							();
		r_dsgraph_render_graph					(1)	;					// normal level, secondary priority
		PortalTraverser.fade_render				()	;					// faded-portals
		r_dsgraph_render_sorted					()	;					// strict-sorted geoms

		if (Glows && ps_r_flags.is(R_FLAG_GLOW_USE))
			Glows->Render();											// glows render

		Environment().RenderLast();					// rain/thunder-bolts
	}

	RImplementation.o.distortion				= FALSE;				// disable distorion
}

// Перед началом рендера мира +SecondVP+
void CRender::BeforeWorldRender() {}

// После рендера мира и пост-эффектов +SecondVP+
void CRender::AfterWorldRender()
{
	if (Device.m_SecondViewport.IsSVPFrame())
	{
		// Делает копию бэкбуфера (текущего экрана) в рендер-таргет второго вьюпорта
		ID3DTexture2D* pBuffer = nullptr;
		HW.m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBuffer);
		HW.pContext->CopyResource(Target->rt_secondVP->pSurface, pBuffer);
		pBuffer->Release(); // Корректно очищаем ссылку на бэкбуфер (иначе игра зависнет в опциях)
	}
}
