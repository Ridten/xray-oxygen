//----------------------------------------------------
// file: stdafx.h
//----------------------------------------------------
#pragma once

#define smart_cast dynamic_cast

#define DIRECTINPUT_VERSION 0x0800

#define         R_R1    1
#define         R_R2    2
#define         RENDER  R_R1

// Std C++ headers
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <process.h>
#include <functional>


#define fmodf fmod


#ifdef	_ECOREB
    #define ECORE_API		__declspec(dllexport)
    #define ENGINE_API		__declspec(dllexport)
#else
    #define ECORE_API		__declspec(dllimport)
    #define ENGINE_API		__declspec(dllimport)
#endif

#define DLL_API			__declspec(dllimport)
#define PropertyGP(a,b)	__declspec( property( get=a, put=b ) )
#define THROW			FATAL("THROW");
#define THROW2(a)		FATAL(a);
#define NO_XRC_STATS

#define clMsg 			Msg

// core
#include <xrCore/xrCore.h>
#include "../xrRenderCommons/xrRenderCommons.h"

#ifdef _EDITOR
	class PropValue;
	class PropItem;
	DEFINE_VECTOR(PropItem*,PropItemVec,PropItemIt);

	class ListItem;
	DEFINE_VECTOR(ListItem*,ListItemsVec,ListItemsIt);
#endif

//#include "..\..\xrCDB\xrCDB.h"
//#include "..\..\xrSound\Sound.h"
//#include "..\..\xrEngine\PSystem.h"

//just types at this moments
#include <d3d9types.h>

// DirectX headers
//#include <d3d9.h>
//#include <d3dx9.h>
//#include "..\..\Layers\xrRender\xrD3dDefs.h"

#include <dinput.h>
//#include <dsound.h>

// some user components
//#include "..\..\xrEngine\fmesh.h"
//#include "..\..\xrEngine\_d3d_extensions.h"

//#include "D3DX_Wrapper.h"

DEFINE_VECTOR		(AnsiString,AStringVec,AStringIt);
DEFINE_VECTOR		(AnsiString*,LPAStringVec,LPAStringIt);

//#include "..\..\..\xrServerEntities\xrEProps.h"
#include "xrCore/Log.h"
#include "editor\engine.h"
//#include "..\..\xrEngine\defines.h"

//#include "../../xrphysics/xrphysics.h"

struct str_pred
{
    IC bool operator()(LPCSTR x, LPCSTR y) const
    {
        return strcmp(x, y) < 0;
    }
};

struct astr_pred
{
    IC bool operator()(const AnsiString& x, const AnsiString& y) const
    {
        return x < y;
    }
};

#ifdef _EDITOR
	#include "editor\device.h"
	#include "..\..\xrEngine\properties.h"
	#include "editor\render.h"
	DEFINE_VECTOR(FVF::L,FLvertexVec,FLvertexIt);
	DEFINE_VECTOR(FVF::TL,FTLvertexVec,FTLvertexIt);
	DEFINE_VECTOR(FVF::LIT,FLITvertexVec,FLITvertexIt);
	DEFINE_VECTOR(shared_str,RStrVec,RStrVecIt);

	#include "EditorPreferences.h"
#endif

#ifdef _LEVEL_EDITOR                
	#include "net_utils.h"
#endif

#define INI_NAME(buf) 		{FS.update_path(buf,"$local_root$",EFS.ChangeFileExt(UI->EditorName(),".ini").c_str());}
//#define INI_NAME(buf) 		{buf = buf+xr_string(Core.WorkingPath)+xr_string("\\")+EFS.ChangeFileExt(UI->EditorName(),".ini");}
#define DEFINE_INI(storage)	{string_path buf; INI_NAME(buf); storage->IniFileName=buf;}
#define NONE_CAPTION "<none>"
#define MULTIPLESEL_CAPTION "<multiple selection>"

// path definition
#define _server_root_		"$server_root$"
#define _server_data_root_	"$server_data_root$"
#define _local_root_		"$local_root$"
#define _import_			"$import$"
#define _sounds_			"$sounds$"
#define _textures_			"$textures$"
#define _objects_			"$objects$"
#define _maps_				"$maps$"
#define _groups_			"$groups$"
#define _temp_				"$temp$"
#define _omotion_			"$omotion$"
#define _omotions_			"$omotions$"
#define _smotion_			"$smotion$"
#define _detail_objects_	"$detail_objects$"

#define		TEX_POINT_ATT	"internal\\internal_light_attpoint"
#define		TEX_SPOT_ATT	"internal\\internal_light_attclip"


