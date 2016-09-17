//-------------------------------------------------------------------------------------------------------
// jdaSkip.c - Simple skip TAP for the Topfield TRF-2400
//
// Mainline code in this module.
//-------------------------------------------------------------------------------------------------------

#define _TMS_

#include "TMSCommander.h"
#include "keys.h"
#include "libFireBird.h"
#include "settings.h"
#include "skipmenu.h"
#include "tap.h"

//------------------- -V-------  deangelj's TAPs have this set to 1
#define ID_JDASKIP 0x80100100

// program name/version - include in
#include "proghdr.c"

TAP_ID(ID_JDASKIP);
TAP_PROGRAM_NAME(PGM_FULL);
TAP_AUTHOR_NAME("deangelj");
TAP_DESCRIPTION("Skip manager");
TAP_ETCINFO(__DATE__);

//------------------------------ Statics --------------------------------------
//

//------------------------------ Types --------------------------------------
//

//------------------------------ Global Variables
//--------------------------------------
//

// Current playback info
TYPE_PlayInfo _playInfo;

// Current state within event handler
dword _mainState = 0;
dword _subState = 0;

// initial phase
int _phase = _initialState;

//------------------------------ Function prototypes
//--------------------------------------
//

// void TMS_Win_ShowMessageWin (char* title, char* lpMessage1, char* lpMessage2,
// dword dwDelay);
dword handle_non_menu_keys(dword param1);
dword idle_handler(dword param1);

//------------------------------ Start of Subroutines
//--------------------------------------
//
//-------------------------------------- DEBUGGING
#define LOGFILE PGM_NAME ".log"
#define LOGDIR "/ProgramFiles/Settings"

char LogPuffer[256];
#define DEBUG(...)                       \
    if (TRC) {                           \
        sprintf(LogPuffer, __VA_ARGS__); \
        WriteLog(LogPuffer);             \
    }
//#define DEBUG      if (TRC) TAP_PrintNet

//===================================================================================================
void WriteLog(char* s)
{
    HDD_TAP_PushDir();
    TAP_Hdd_ChangeDir(LOGDIR);
    LogEntry(LOGFILE, PGM_NAME, TRUE, TIMESTAMP_YMDHMS, s);
    HDD_TAP_PopDir();
}

//===================================================================================================
// ExitTAP
//
// First check that the infobox (telling the user we're quitting) has timed out
//===================================================================================================

void ExitTAP()
{
    DEBUG("Exiting TAP...");
    OSDMenuDestroy();
    TAP_Exit();
}

//--------------------------------------------------------------------------------------
//	initial_state_handler - handle keys when in the initial state
//--------------------------------------------------------------------------------------

static dword initial_state_handler(dword param1)
{
    dword ret;
    static bool doNotReEnter = FALSE; // not re-entrant

    if (doNotReEnter)
        return param1;

    doNotReEnter = TRUE;

    // Is it our menu key
    if (param1 == MENUKEY) {
        DEBUG("Menu key - calling menu\n");
        ShowMenu();
        ret = 0;
    } else {
        ret = handle_non_menu_keys(param1);
    }

    doNotReEnter = FALSE;
    return ret;
}


static dword key_handler(dword param1)
{
    DEBUG("key_handler...key: %d\n", param1);

    switch (_phase) {
    case _initialState:
        return initial_state_handler(param1);
        break;
    case _menuActive:
        return menuactive_handler(param1);
        break;
    // case _keyMenuActive:	return keymenuactive_handler ( param1 ); break;
    default:
        DEBUG("Invalid _phase found: %d\n", _phase);
        break;
    }

    return param1;
}

//-------------------------------------------------------------------------------------------------------
// TMSCommander_handler -
//-------------------------------------------------------------------------------------------------------

static dword
TMSCommander_handler(dword param1)
{
    DEBUG("TMSCommandler_handler...key: %d", param1);

    switch (param1) {
    case TMSCMDR_Capabilities:
        DEBUG("Got request for TMSCMDR_Capabilities\n");
        return (dword)(TMSCMDR_HaveMenu | TMSCMDR_CanBeStopped);
    case TMSCMDR_Stop:
        // call your TAP's exit function or
        ExitTAP();
        return TMSCMDR_OK;
    case TMSCMDR_Menu:
        // call your TAP's menu
        DEBUG("Got request for TMSCMDR_Menu\n");
        ShowMenu();
        return TMSCMDR_OK;
    case TMSCMDR_Exiting:
        // call your TAP's menu
        DEBUG("Got message that TMSCMDR is exiting\n");
        return TMSCMDR_OK;
    default:
        // unknown TMSCommander function
        return TMSCMDR_UnknownFunction;
    }
}

//-------------------------------------------------------------------------------------------------------
// TAP_EventHandler - the standard TAP event handler.
//-------------------------------------------------------------------------------------------------------

dword TAP_EventHandler(word event, dword param1, dword param2)
{
    

    UNUSED(param2);

    // Get State Information from TAP interface
    TAP_GetState(&_mainState, &_subState);

    if (_mainState != STATE_Normal)
        return param1;

    // Absorb all events if any of the dialogs are visible
    if (OSDMenuInfoBoxIsVisible() || OSDMenuMessageBoxIsVisible() || OSDMenuColorPickerIsVisible()) {
        // special case when capturing colour/skip keys
        if (OSDMenuInfoBoxIsVisible() && _phase == _menuActive) {
            if (event == EVT_KEY) {
                return key_handler(param1);
            }
            else {
                return 0;
            }
        }
        else {
            OSDMenuEvent(&event, &param1, &param2);
            return 0;
        }
    }

    // if OSD key event has been handled just exit (param1 is 0)
    if ((_subState == SUBSTATE_MainMenu) && OSDMenuEvent(&event, &param1, &param2)) {
        // choose right buttons for menu option
        if (_phase == _menuActive) {
            OSDMenuButtonsClear();
            OSDMenuButtonAdd(1, BI_Exit, NULL, "Exit");
            if (OSDMenuGetCurrentItem() == _optExitTAP) {
                OSDMenuButtonAdd(1, BI_Ok, NULL, "Stop TAP");
            } else {
                OSDMenuButtonAdd(1, BI_Select, NULL, "Select");
            }
        }
        OSDMenuUpdate(FALSE);
        return param1;
    }

    // Pass event to appropriate handler
    switch (event) {
    case EVT_KEY:
        return key_handler(param1);
        break;
    case EVT_IDLE:
        return idle_handler(param1);
        break;
    case EVT_UART:
    case EVT_RBACK:
    case EVT_SVCSTATUS:
    case EVT_DEMUX:
    case EVT_STOP:
    case EVT_VIDEO_CONV:
        return param1;
        break;
    case EVT_TMSCommander:
        return TMSCommander_handler(param1);
        break;

    default:
        DEBUG("Invalid event found: %d", event);
        break;
    }
    return param1;
}

//-------------------------------------------------------------------------------------------------------
// TAP_Main - TAP start entry point
//-------------------------------------------------------------------------------------------------------

int TAP_Main(void)
{
    _phase = _initialState;
    DEBUG("Starting jdaSkip...");
   LoadSettings();

    // if (GetUptime() > (5*60*100))   //more than 5 minutes after system start?
    // 1 tick = 10ms, 1 sec = 100 ticks
    //{
    // Show the user we're started and the settings
    //	sprintf(config, "%s: %d, %s: %d, %s: %d, %s: %d",
    //		GetKeyName(key1), key1Len, GetKeyName(key2), key2Len,
    // GetKeyName(key3), key3Len, GetKeyName(key4), key4Len);
    //	DEBUG("%s\n", config);
    //	TMS_Win_ShowMessageWin(PGM_FULL, "  jdaSkip TAP started  ", config,
    // 3000);
    //}
    return 1; // we're starting in TSR-mode
}
