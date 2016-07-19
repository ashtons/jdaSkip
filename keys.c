#define _TMS_

#include <tap.h>
#include "keys.h"
#include "libFireBird.h"

RemoteKey remoteKeyNames[] = 
{
	{ RKEY_ChUp,					"P+" },
	{ RKEY_ChDown,					"P-" },
	{ RKEY_VolUp,					"V+" },
	{ RKEY_VolDown,					"V-" },
	{ RKEY_Prev,					"Prev" },
	{ RKEY_Next,					"Next" },
//	{ RKEY_MyRed,					"Red" },
	{ RKEY_NewF1,					"Red" },
	{ RKEY_F2,						"Green" },
	{ RKEY_F3,						"Yellow" },
	{ RKEY_F4,						"Blue" },
	{ RKEY_Sat,						"Sat" },
//	{ RKEY_MyWhite,					"White" },
	{ RKEY_Ab,					    "White" },
	{ RKEY_Recall,					"Recall" },
	{ RKEY_TvRadio,					"TV Radio" },
	{ RKEY_Up,						"Up" },
	{ RKEY_Down,					"Down" },
	{ RKEY_Left,					"Left" },
 	{ RKEY_Right,					"Right" },
 	{ RKEY_AudioTrk,				"AudioTrk" },
 	{ RKEY_Info,					"Info" },
 	{ RKEY_Ok,						"Ok" },
 	{ RKEY_Fav,						"Fav" },
 	{ RKEY_Subt,					"Subtitle" },
 	{ RKEY_TvSat,					"TvSat" },
 	{ RKEY_Teletext,				"Teletext" },
 	{ RKEY_Unmapped,				"TMSCommander" },
	{ 0,							NULL }
};

const int numRemoteKeys = 27;

const char* GetKeyName( dword keyId )
{
	static const char *unknown = "?";
	RemoteKey* p;
	for ( p = remoteKeyNames; p->name != 0; ++p )
		if ( p->id == keyId )
			return (const char *)(p->name);
	return unknown;
}

dword GetNextKey( dword keyId, bool allowUnmapped )
{
	RemoteKey* p;
	for ( p = remoteKeyNames; p->name != 0; ++p )
		if ( p->id == keyId )
		{
			RemoteKey* q = ++p;
			if ((q->name == 0) || ((q->id == RKEY_Unmapped) && !allowUnmapped))
				q = remoteKeyNames;  //if at end, or unmapped and not allowed, cycle around
			return (q->id);
		}
	return remoteKeyNames[0].id;  //if not found return the first key
}

dword GetPriorKey( dword keyId, bool allowUnmapped )
{
	int i;
	for ( i=0; i < numRemoteKeys; i++ )
		if ( remoteKeyNames[i].id == keyId )
		{
			if (i > 0)
				return remoteKeyNames[--i].id;
			else // i == 0  cycle around
			{
				if (allowUnmapped)
					return remoteKeyNames[numRemoteKeys-2].id;
				else
					return remoteKeyNames[numRemoteKeys-3].id;
			}
		}
	return remoteKeyNames[0].id;  //if not found return the first key
}
