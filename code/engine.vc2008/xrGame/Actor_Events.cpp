#include "stdafx.h"
#include "Actor.h"
#include "ActorCondition.h"
#include "items/Weapon.h"
#include "items/Artefact.h"
#include "items/Scope.h"
#include "items/Silencer.h"
#include "items/GrenadeLauncher.h"
#include "inventory.h"
#include "Level.h"
#include "..\xrEngine\xr_level_controller.h"
#include "..\xrEngine\CameraBase.h"
#include "items/FoodItem.h"
#include "items/Grenade.h"

#include "holder_custom.h"
#include "game_base.h"
#ifdef DEBUG
#include "PHDebug.h"
#endif

void CActor::OnEvent(NET_Packet& P, u16 type)
{
	inherited::OnEvent			(P,type);
	CInventoryOwner::OnEvent	(P,type);

	u16 id;
	switch (type)
	{
	case GE_TRADE_BUY:
	case GE_OWNERSHIP_TAKE:
	{
		P.r_u16(id);
		CObject* Obj = Level().Objects.net_Find(id);
		if (!Obj) {
			Msg("! GE_OWNERSHIP_TAKE: Object not found. object_id = [%d]", id);
			break;
		}

		CGameObject* _GO = smart_cast<CGameObject*>(Obj);
		if (inventory().CanTakeItem(smart_cast<CInventoryItem*>(_GO)))
		{
			Obj->H_SetParent(smart_cast<CObject*>(this));
			inventory().Take(_GO, false, true);
		}
		else
		{
			NET_Packet		P;
			u_EventGen(P, GE_OWNERSHIP_REJECT, ID());
			P.w_u16(u16(Obj->ID()));
			u_EventSend(P);
		}
	}
		break;
	case GE_TRADE_SELL:
	case GE_OWNERSHIP_REJECT:
		{
			P.r_u16							(id);
			CObject* Obj					= Level().Objects.net_Find	(id);

			if ( !Obj ) {
				Msg                 ( "! GE_OWNERSHIP_REJECT: Object not found, id = %d", id );
				break;
			}

			bool just_before_destroy		= !P.r_eof() && P.r_u8();
			bool dont_create_shell			= (type==GE_TRADE_SELL) || just_before_destroy;
			Obj->SetTmpPreDestroy			(just_before_destroy);
			
			CGameObject * GO = smart_cast<CGameObject*>(Obj);
			
			VERIFY( GO->H_Parent() );
			if ( !GO->H_Parent() )
			{
				Msg("! ERROR: Actor [%d][%s] tries to reject item [%d][%s] that has no parent", 
					ID(), Name(), GO->ID(), GO->cNameSect().c_str());
				break;
			}
			
			VERIFY_FORMAT( GO->H_Parent()->ID() == ID(), 
				"actor [%d][%s] tries to drop not own object [%d][%s]",
					ID(), Name(), GO->ID(), GO->cNameSect().c_str());

			if ( GO->H_Parent()->ID() != ID() )
			{
				CActor* real_parent = smart_cast<CActor*>(GO->H_Parent());
				Msg("! ERROR: Actor [%d][%s] tries to drop not own item [%d][%s], his parent is [%d][%s]",
					ID(), Name(), GO->ID(), GO->cNameSect().c_str(), real_parent->ID(), real_parent->Name());
				break;
			}

			if (!Obj->getDestroy() && inventory().DropItem(GO, just_before_destroy, dont_create_shell)) 
			{
				Level().m_feel_deny.feel_touch_deny(Obj, 1000);

				// [12.11.07] Alexander Maniluk: extended GE_OWNERSHIP_REJECT packet for drop item to selected position
				Fvector dropPosition;
				if (!P.r_eof())
				{
					P.r_vec3(dropPosition);
					GO->MoveTo(dropPosition);
				}
			}
		}
		break;
	case GE_INV_ACTION:
		{
			u16 cmd;
			P.r_u16		(cmd);
			u32 flags;
			P.r_u32		(flags);
			s32 ZoomRndSeed = P.r_s32();
			s32 ShotRndSeed = P.r_s32();
									
			if (flags & CMD_START)
			{
				if (cmd == kWPN_ZOOM)
					SetZoomRndSeed(ZoomRndSeed);
				if (cmd == kWPN_FIRE)
					SetShotRndSeed(ShotRndSeed);
				IR_OnKeyboardPress(cmd);
			}
			else
				IR_OnKeyboardRelease(cmd);
		}
		break;
	case GEG_PLAYER_ITEM2SLOT:
	case GEG_PLAYER_ITEM2BELT:
	case GEG_PLAYER_ITEM2RUCK:
	case GEG_PLAYER_ITEM_EAT:
	case GEG_PLAYER_ACTIVATEARTEFACT:
{
			P.r_u16		(id);
			CObject* Obj	= Level().Objects.net_Find	(id);

			VERIFY_FORMAT  ( Obj, "GEG_PLAYER_ITEM_EAT(use): Object not found. object_id = [%d]", id);
			
			if (!Obj)
				break;

			VERIFY_FORMAT( !Obj->getDestroy(), "GEG_PLAYER_ITEM_EAT(use): Object is destroying. object_id = [%d]", id);
			if (Obj->getDestroy())
				break;
			

			if (type == GEG_PLAYER_ACTIVATEARTEFACT)
			{
				CArtefact* pArtefact = smart_cast<CArtefact*>(Obj);
				VERIFY_FORMAT(pArtefact, "GEG_PLAYER_ACTIVATEARTEFACT: Artefact not found. artefact_id = [%d]", id);
				if (!pArtefact) {
					Msg("! GEG_PLAYER_ACTIVATEARTEFACT: Artefact not found. artefact_id = [%d]", id);
					break;//1
				}

				pArtefact->ActivateArtefact();
				break;//1
			}
			
			PIItem iitem = smart_cast<CInventoryItem*>(Obj);
			R_ASSERT( iitem );

			switch (type)
			{
			case GEG_PLAYER_ITEM2SLOT:
			{
				u16 slot_id = P.r_u16();
				inventory().Slot(slot_id, iitem);
			}break;//2
			case GEG_PLAYER_ITEM2BELT:
				inventory().Belt(iitem);
				break;//2
			case GEG_PLAYER_ITEM2RUCK:
				inventory().Ruck(iitem);
				break;//2
			case GEG_PLAYER_ITEM_EAT:
				inventory().Eat(iitem);
				break;//2
			}//switch

		}break;//1
	case GEG_PLAYER_ACTIVATE_SLOT:
		{
			u16							slot_id;
			P.r_u16						(slot_id);

			inventory().Activate		(slot_id);
								  
		}break;

	case GEG_PLAYER_DISABLE_SPRINT:
		{
			s8 cmd				= P.r_s8();
			m_block_sprint_counter = m_block_sprint_counter+cmd;
			Msg("m_block_sprint_counter=%d",m_block_sprint_counter);
			if(m_block_sprint_counter>0)
			{
				mstate_wishful	&=~mcSprint;
			}
		}break;

	case GEG_PLAYER_WEAPON_HIDE_STATE:
		{
			u16 State		= P.r_u16();
			BOOL	Set		= !!P.r_u8();
			inventory().SetSlotsBlocked	(State, !!Set);
		}break;
	case GE_MOVE_ACTOR:
		{
			Fvector NewPos, NewRot;
			P.r_vec3(NewPos);
			P.r_vec3(NewRot);
			
			MoveActor(NewPos, NewRot);
		}break;
	case GE_ACTOR_MAX_POWER:
		{
			conditions().MaxPower();
			conditions().ClearWounds();
			ClearBloodWounds();
		}break;
	case GE_ACTOR_MAX_HEALTH:
		{
			SetfHealth(GetMaxHealth());
		}break;
	case GEG_PLAYER_ATTACH_HOLDER:
		{
			u16 id = P.r_u16();
			CObject* O	= Level().Objects.net_Find	(id);
			if (!O){
				Msg("! Error: No object to attach holder [%d]", id);
				break;
			}
			VERIFY(m_holder==nullptr);
			CHolderCustom*	holder = smart_cast<CHolderCustom*>(O);
			if(!holder->Engaged())	use_Holder		(holder);

		}break;
	case GEG_PLAYER_DETACH_HOLDER:
		{
			if			(!m_holder)	break;
			u16 id			= P.r_u16();
			CGameObject*	GO	= smart_cast<CGameObject*>(m_holder);
			VERIFY			(id==GO->ID());
			use_Holder		(nullptr);
		}break;
	}
}

void CActor::MoveActor(Fvector NewPos, Fvector NewDir)
{
	Fmatrix	M = XFORM();
	M.translate(NewPos);
	r_model_yaw				= NewDir.y;
	r_torso.yaw				= NewDir.y;
	r_torso.pitch			= -NewDir.x;
	unaffected_r_torso.yaw	= r_torso.yaw;
	unaffected_r_torso.pitch= r_torso.pitch;
	unaffected_r_torso.roll	= 0;

	r_torso_tgt_roll		= 0;
	cam_Active()->Set		(-unaffected_r_torso.yaw,unaffected_r_torso.pitch,unaffected_r_torso.roll);
	ForceTransform(M);
}
