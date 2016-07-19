//-------------------------------------------------------------------------------------------------------
// settings.c - load settings for a TAP
//
// author:	deangelj
//
//-------------------------------------------------------------------------------------------------------

#define _TMS_

#include "string.h"
#include "tap.h"
#include "libFireBird.h"
#include "settings.h"
#include "keys.h"

#define SKIPINI "jdaSkip.ini"
#define SKIPFILESROOT "/ProgramFiles/Settings"

#define DEBUG      if (TRC) TAP_PrintNet

char inidir[255]; 

//Defaults
static const dword key1def = RKEY_NewF1;
//static const dword key1def = RKEY_MyRed;
static const int      key1lendef = 60;
static const dword key2def = RKEY_F2;
static const int      key2lendef = 0;
static const dword key3def = RKEY_F3;
static const int      key3lendef = 30;
static const dword key4def = RKEY_F4;
static const int      key4lendef = -10;

struct settingsStruct
{
	dword key1;
	int key1Len;
	dword key2;
	int key2Len;
	dword key3;
	int key3Len;
	dword key4;
	int key4Len;
	bool RESERVEKEYS;
	bool SHOWPROGBAR;
	dword MENUKEY;
	bool TRC;
	SkipMode MODE;
	PHMODE PRESSANDHOLD;
	bool QUADRECORDINGSSUPPORTED;
	dword REDKEY;
	dword GREENKEY;
	dword YELLOWKEY;
	dword BLUEKEY;
	char reserved[104];
} settings;


//-------------------------------------------------------------------------------------------------------
// ChangeDirSettings
//-------------------------------------------------------------------------------------------------------

void ChangeDirSettings()
{
	TAP_Hdd_ChangeDir("/ProgramFiles");
	if (!TAP_Hdd_Exist("Settings")) TAP_Hdd_Create("Settings", ATTR_FOLDER);

	TAP_Hdd_ChangeDir("Settings");

}

//-------------------------------------------------------------------------------------------------------
dword CheckKey(dword key)
{
	//if out of range return the '0' key (this is to catch issues when key hasn't been assigned yet)
	if (key < 0x10000) return 0x10000;
	if (key > 0x11000) return 0x10000;
	return key;
}

//-------------------------------------------------------------------------------------------------------
// SaveSettings
// save curent settings to file
//-------------------------------------------------------------------------------------------------------

bool SaveSettings ()
{
	TYPE_File *fp;
	bool status = FALSE;

	//TAP_Hdd_ChangeDir(SKIPFILESROOT);
	HDD_TAP_PushDir();
	ChangeDirSettings();

	if (!TAP_Hdd_Exist(SKIPINI)) TAP_Hdd_Create(SKIPINI, ATTR_NORMAL);
	if ((fp = TAP_Hdd_Fopen(SKIPINI)))
	{

		settings.key1 = key1;
		settings.key2 = key2;
		settings.key3 = key3;
		settings.key4 = key4;
		settings.key1Len = key1Len;
		settings.key2Len = key2Len;
		settings.key3Len = key3Len;
		settings.key4Len = key4Len;
		settings.RESERVEKEYS = RESERVEKEYS;
		settings.SHOWPROGBAR = SHOWPROGBAR;
		settings.MENUKEY = MENUKEY;
		settings.TRC = TRC;
		settings.MODE = MODE;
		settings.PRESSANDHOLD = PRESSANDHOLD;
		settings.QUADRECORDINGSSUPPORTED = QUADRECORDINGSSUPPORTED;
		settings.REDKEY = REDKEY;
		settings.GREENKEY = GREENKEY;
		settings.YELLOWKEY = YELLOWKEY;
		settings.BLUEKEY = BLUEKEY;

		(void) TAP_Hdd_Fwrite(&settings, sizeof settings, 1, fp);
		TAP_Hdd_Fclose(fp);
		status = TRUE;
	}
	HDD_TAP_PopDir();
	return status;
}

//-------------------------------------------------------------------------------------------------------
// LoadSettings
// load curent settings to memory
//-------------------------------------------------------------------------------------------------------

void LoadSettings ()
{
	TYPE_File *fp;

	HDD_TAP_PushDir();
	ChangeDirSettings();

	if ((fp = TAP_Hdd_Fopen(SKIPINI)))
	{
		dword len = TAP_Hdd_Flen( fp );
		memset( &settings.key1, 0, sizeof (settings));
		if (TAP_Hdd_Fread(&settings, len < sizeof settings?len:sizeof settings, 1, fp) > 0)
		{
			key1 = settings.key1;
			key2 = settings.key2;
			key3 = settings.key3;
			key4 = settings.key4;
			key1Len = settings.key1Len;
			key2Len = settings.key2Len;
			key3Len = settings.key3Len;
			key4Len = settings.key4Len;
			RESERVEKEYS = settings.RESERVEKEYS;
			SHOWPROGBAR = settings.SHOWPROGBAR;
			MENUKEY = settings.MENUKEY;
			TRC = settings.TRC;
			MODE = settings.MODE;
			PRESSANDHOLD = settings.PRESSANDHOLD;
			QUADRECORDINGSSUPPORTED = settings.QUADRECORDINGSSUPPORTED;
			REDKEY = CheckKey(settings.REDKEY);
			GREENKEY = CheckKey(settings.GREENKEY);
			YELLOWKEY = CheckKey(settings.YELLOWKEY);
			BLUEKEY = CheckKey(settings.BLUEKEY);
		}
		TAP_Hdd_Fclose(fp);
	}
	else
	{
		MODE = SKIPMODE_STD;

		key1  = key1def;
		key1Len  = key1lendef;
		key2  = key2def;
		key2Len  = key2lendef;
		key3  = key3def;
		key3Len  = key3lendef;
		key4  = key4def;
		key4Len  = key4lendef;

		//MODE = SKIPMODE_NOBLUE;
		RESERVEKEYS = TRUE; //by default reserve the std keys while pbar is up
		SHOWPROGBAR = FALSE; //by default don't show pbar while skipping
		MENUKEY = RKEY_Exit;  //exit key brings up menu
		TRC = FALSE;
		MODE = SKIPMODE_STD;
		PRESSANDHOLD = PRESSANDHOLD_TIME;		//By default don't show skip percent as it slows down the P&H
		QUADRECORDINGSSUPPORTED = TRUE;
		REDKEY = RKEY_NewF1;
		GREENKEY = RKEY_F2;
		YELLOWKEY = RKEY_F3;
		BLUEKEY = RKEY_F4;
	}
	HDD_TAP_PopDir();

}
