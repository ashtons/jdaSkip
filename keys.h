#ifndef __KEYS_H
#define __KEYS_H


#define RKEY_Unmapped 0x20000

//#ifdef __7100PLUS__
//#define RKEY_MyWhite      RKEY_Ab
//#else
//#define RKEY_MyWhite      0x10049
//#endif

//#ifdef __7260__
//#define RKEY_MyRed        RKEY_F1
//#else
//#define RKEY_MyRed        RKEY_NewF1
//#endif

typedef struct _RemoteKey
{
	dword id;
	const char* name;
} RemoteKey;

extern RemoteKey remoteKeyNames[];

dword GetNextKey( dword keyId, bool allowUnmapped );
dword GetPriorKey( dword keyId, bool allowUnmapped );
const char* GetKeyName( dword keyId );

#endif
