//------------------------------ Information --------------------------------------
//
// skipmenu
//
//---------------------------------------------------------------------------------

#define _TMS_

#include "keys.h"
#include "libFireBird.h"
#include "proghdr.c"
#include "settings.h"
#include "skipmenu.h"
#include "stdlib.h" //atoi
#include "string.h"
#include "tap.h"

#define DEBUG \
    if (TRC)  \
    TAP_PrintNet

//------------------------------ Statics --------------------------------------
//

//------------------------------ Types --------------------------------------
//

//------------------------------ Global Variables --------------------------------------
//

//------------------------------ Function prototypes --------------------------------------
//

static void HideMenu();
static char* GetPHMode();
void ExitTAP();

//------------------------------ Start of Subroutines --------------------------------------
//

// OSD regions
word inforegion;
word listregion;
word dialogregion;

word _optrgn = 0, _keyoptrgn = 0;
dword _options = 0, _keyoptions = 0;

//===================================================================================================
static char* GetStringFromInt(int val)
{
    static char strval[100];
    TAP_SPrint(strval, "%d", val);
    return &strval[0];
}

//===================================================================================================
// ShowMenu
//
// Shows jdaSkip config menu
//===================================================================================================
extern TYPE_GrData _Button_ok_Gd;

void ShowMenu()
{
    //char showvalue[100];
    bool bSkipStdEnabled = (MODE == SKIPMODE_STD);

    DEBUG("ShowMenu()");

    OSDMenuDestroy();
    //void OSDMenuInitialize(bool AllowScrollingOfLongText, bool HasValueColumn, bool NumberedItems, bool ScrollLoop, char *TitleLeft, char *TitleRight)
    OSDMenuInitialize(FALSE, TRUE, FALSE, TRUE, "jdaSkip", "Settings");
    OSDMenuItemModifyValueXPos(400); //adjust the value column start xpos
    //bool OSDMenuItemAdd(char *Name, char *Value, TYPE_GrData *pNameIconGd, TYPE_GrData *pValueIconGd, bool Selectable, bool ValueArrows, dword ID)
    OSDMenuItemAdd("Skip Mode", bSkipStdEnabled ? "Standard" : "Binary", NULL, NULL, FALSE, TRUE, 0);
    //Show key values
    OSDMenuItemAdd("Set Red/Green/Yellow/Blue", "", NULL, &_Button_ok_Gd, TRUE, FALSE, 0);
    OSDMenuItemAdd("Set Skip Keys", "", NULL, &_Button_ok_Gd, TRUE, FALSE, 0);
    if (bSkipStdEnabled) {
        OSDMenuItemAdd("Skip Key 1 Seconds", GetStringFromInt(key1Len), NULL, NULL, TRUE, TRUE, 0);
    } else {
        OSDMenuItemAdd("Skip Seconds", GetStringFromInt(key1Len), NULL, NULL, TRUE, TRUE, 0);
    }
    OSDMenuItemAdd("Skip Key 2 Seconds", GetStringFromInt(key2Len), NULL, NULL, bSkipStdEnabled, TRUE, 0);
    OSDMenuItemAdd("Skip Key 3 Seconds", GetStringFromInt(key3Len), NULL, NULL, bSkipStdEnabled, TRUE, 0);
    OSDMenuItemAdd("Skip Key 4 Seconds", GetStringFromInt(key4Len), NULL, NULL, bSkipStdEnabled, TRUE, 0);
    OSDMenuItemAdd("Reserve standard keys for progress bar", RESERVEKEYS ? "Yes" : "No", NULL, NULL, TRUE, TRUE, 0);
    OSDMenuItemAdd("Display progress bar when skipping", SHOWPROGBAR ? "Yes" : "No", NULL, NULL, TRUE, TRUE, 0);
    OSDMenuItemAdd("Menu Key", (char*)GetKeyName(MENUKEY), NULL, NULL, TRUE, TRUE, 0);
    OSDMenuItemAdd("Trace", TRC ? "Yes" : "No", NULL, NULL, TRUE, TRUE, 0);
    OSDMenuItemAdd("Press-and-hold option", GetPHMode(), NULL, NULL, TRUE, TRUE, 0);
    OSDMenuItemAdd("Quad recording support", QUADRECORDINGSSUPPORTED ? "Yes" : "No", NULL, NULL, TRUE, TRUE, 0);

    OSDMenuItemAdd("Stop jdaSkip", NULL, NULL, NULL, TRUE, FALSE, 0);
    OSDMenuButtonAdd(1, BI_Exit, NULL, "Exit");
    OSDMenuButtonAdd(1, BI_Select, NULL, "Select");

    OSDMenuSelectItem(2); //set current item to be the first selectable

    OSDMenuUpdate(FALSE);
    //State = ST_Menu;
    _phase = _menuActive;
}

//--------------------------------------------------------------------------------------
//	GetPHMode - get a string value for the P&H mode
//--------------------------------------------------------------------------------------

static char* GetPHMode()
{

    switch (PRESSANDHOLD) {
    case PRESSANDHOLD_TIME:
        return "Time";
    case PRESSANDHOLD_PERCENT:
        return "Percent";
    case PRESSANDHOLD_TIMEPERCENT:
        return "Time & Percent";
    case PRESSANDHOLD_NONE:
        return "None";
    default:
        return "None";
    }
}

//--------------------------------------------------------------------------------------
//	HideMenu
//--------------------------------------------------------------------------------------

static void HideMenu()
{
    SaveSettings();
    OSDMenuDestroy();
    _phase = _initialState;
}

//--------------------------------------------------------------------------------------
//	SetNewKey - set new value for key when pressing Left/Right
//--------------------------------------------------------------------------------------

static void SetNewKey(dword* ckey, dword key)
{
    if (key == RKEY_Left)
        *ckey = GetPriorKey(*ckey, *ckey == MENUKEY);
    else
        *ckey = GetNextKey(*ckey, *ckey == MENUKEY);
}

//--------------------------------------------------------------------------------------
//	SetNewKeyIncrement - set new value for key lengths when pressing Left/Right
//--------------------------------------------------------------------------------------

static void SetNewKeyIncrement(int* ckey, /*const char *cstr,*/ dword key)
{

    //DEBUG("SetNewKeyIncrement...\n");

    if (key == RKEY_Left) {
        if ((*ckey < 11) && (*ckey > -10))
            *ckey -= 1;
        else
            *ckey -= 5;
    }
    else {
        if ((*ckey < 10) && (*ckey > -11))
            *ckey += 1;
        else
            *ckey += 5;
    }
}

//--------------------------------------------------------------------------------------
//	SetPressAndHold - set new value for the press-and-hold option
//--------------------------------------------------------------------------------------

static void SetPressAndHold(dword key)
{
    //Loop through the p&H values
    //PRESSANDHOLD_TIME,
    //PRESSANDHOLD_PERCENT,
    //PRESSANDHOLD_TIMEPERCENT,
    //PRESSANDHOLD_NONE
    if (key == RKEY_Right) {
        if (++PRESSANDHOLD > PRESSANDHOLD_NONE)
            PRESSANDHOLD = PRESSANDHOLD_TIME;
    }
    else if (key == RKEY_Left) {
        if (PRESSANDHOLD > PRESSANDHOLD_TIME)
            --PRESSANDHOLD;
        else
            PRESSANDHOLD = PRESSANDHOLD_NONE;
    }
}

//--------------------------------------------------------------------------------------
//	menuOK_handler - handle keys for OK key in menu
//--------------------------------------------------------------------------------------

char* ColourKeyNames[] = { "Red", "Green", "Yellow", "Blue" };
char* SkipKeyNames[] = { "1st", "2nd", "3rd", "4th" };

static int currentColourKey;
static int currentSkipKey;

static void menuOK_handler()
{
    int item = OSDMenuGetCurrentItem();

    switch (item) {
    case _optColourKeys:
        currentColourKey = 0;
        DEBUG("Prompting for first colour key = Red\n");
        OSDMenuInfoBoxShow("jdaSkip", "Press Red Key", 0);
        //set key counter to 1, type to Colour, and start prompting user for keys
        break;

    case _optSkipKeys:
        currentSkipKey = 0;
        DEBUG("Prompting for first skip key\n");
        OSDMenuInfoBoxShow("jdaSkip", "Press 1st Skip Key", 0);
        //set key counter to 1, type to Skip, and start prompting user for keys
        break;

    case _optExitTAP:
        ExitTAP();
        return;

    default:
        return;
        break;
    }
}

//--------------------------------------------------------------------------------------
//	menuLeftRight_handler - handle keys for Left/Right in menu
//--------------------------------------------------------------------------------------

static void menuLeftRight_handler(dword param1)
{
    int item = OSDMenuGetCurrentItem();
    char* skipvalText;
    int skipval;

    switch (item) {
    case _optSkipMode:
        MODE = (MODE == SKIPMODE_BINARY ? SKIPMODE_STD : SKIPMODE_BINARY);
        //changed mode - now adjust following rows to adjust entries
        if (MODE == SKIPMODE_STD) {
            OSDMenuItemModifyValue(item, "Standard");
            item += 2;
            //OSDMenuItemModifyName(++item, "Skip Key 1");
            OSDMenuItemModifyName(++item, "Skip Key 1 Seconds");
            //OSDMenuItemModifyName(++item, "Skip Key 2");
        }
        else {
            OSDMenuItemModifyValue(item, "Binary");
            item += 2;
            //OSDMenuItemModifyName(++item, "Skip Key Back");
            OSDMenuItemModifyName(++item, "Skip Seconds");
            skipvalText = OSDMenuItemGetValue(item);
            skipval = atoi(skipvalText);
            if (skipval <= 0) {
                key1Len = 60; //default
                OSDMenuItemModifyValue(item, GetStringFromInt(key1Len));
            }
            //OSDMenuItemModifyName(++item, "Skip Key Forward");
        }
        OSDMenuItemModifySelectable(++item, MODE == SKIPMODE_STD); //skip 2 secs;
        //OSDMenuItemModifySelectable(++item, MODE==SKIPMODE_STD);  //skip 3 key;
        OSDMenuItemModifySelectable(++item, MODE == SKIPMODE_STD); //skip 3 secs;
        //OSDMenuItemModifySelectable(++item, MODE==SKIPMODE_STD);  //skip 4 key;
        OSDMenuItemModifySelectable(++item, MODE == SKIPMODE_STD); //skip 4 secs;
        break;
    //		case _optKey1:
    //			SetNewKey(&key1, param1);
    //			OSDMenuItemModifyValue(item, (char *)GetKeyName(key1));
    //			break;
    case _optKey1Len:
        SetNewKeyIncrement(&key1Len, param1);
        OSDMenuItemModifyValue(item, GetStringFromInt(key1Len));
        break;
    //		case _optKey2:
    //			SetNewKey(&key2, param1);
    //			OSDMenuItemModifyValue(item, (char *)GetKeyName(key2));
    //			break;
    case _optKey2Len:
        SetNewKeyIncrement(&key2Len, param1);
        OSDMenuItemModifyValue(item, GetStringFromInt(key2Len));
        break;
    //		case _optKey3:
    //			SetNewKey(&key3, param1);
    //			OSDMenuItemModifyValue(item, (char *)GetKeyName(key3));
    //			break;
    case _optKey3Len:
        SetNewKeyIncrement(&key3Len, param1);
        OSDMenuItemModifyValue(item, GetStringFromInt(key3Len));
        break;
    //		case _optKey4:
    //			SetNewKey(&key4, param1);
    //			OSDMenuItemModifyValue(item, (char *)GetKeyName(key4));
    //			break;
    case _optKey4Len:
        SetNewKeyIncrement(&key4Len, param1);
        OSDMenuItemModifyValue(item, GetStringFromInt(key4Len));
        break;
    case _optReserveKeys:
        RESERVEKEYS = !RESERVEKEYS;
        OSDMenuItemModifyValue(item, RESERVEKEYS ? "Yes" : "No");
        break;
    case _optDisplayPBar:
        SHOWPROGBAR = !SHOWPROGBAR;
        OSDMenuItemModifyValue(item, SHOWPROGBAR ? "Yes" : "No");
        break;
    case _optTrace:
        TRC = !TRC;
        OSDMenuItemModifyValue(item, TRC ? "Yes" : "No");
        break;
    case _optPressAndHold:
        SetPressAndHold(param1);
        OSDMenuItemModifyValue(item, GetPHMode());
        break;
    case _optQuadRecordings:
        QUADRECORDINGSSUPPORTED = !QUADRECORDINGSSUPPORTED;
        OSDMenuItemModifyValue(item, QUADRECORDINGSSUPPORTED ? "Yes" : "No");
        break;
    case _optMenuKey:
        SetNewKey(&MENUKEY, param1);
        OSDMenuItemModifyValue(item, (char*)GetKeyName(MENUKEY));
        break;
    default:
        return;
        break;
    }
    OSDMenuUpdate(FALSE);
}

//--------------------------------------------------------------------------------------
//	menuactive_handler - handle keys when the menu is active
//--------------------------------------------------------------------------------------

extern int InfoBoxOSDRgn;

dword menuactive_handler(dword param1)
{
    //int item = OSDMenuGetCurrentItem();

    DEBUG("menuactive_handler...\n");

    //if prompting for red/green/yellow/blue...
    //if prompting for Key1/Key2/Key3/Key4...
    //then infobox will be showing...

    if (InfoBoxOSDRgn) {
        int item = OSDMenuGetCurrentItem();
        char str[30];

        if (param1 > 0x11000) //repeat/start/end sequence key
        {
            DEBUG("Ignoring high key value..\n");
            return 0;
        }

        DEBUG("Infobox showing... destroying..\n");
        OSDMenuInfoBoxDestroy();
        switch (item) {
        case _optColourKeys:
            if (currentColourKey == 0)
                REDKEY = param1;
            if (currentColourKey == 1)
                GREENKEY = param1;
            if (currentColourKey == 2)
                YELLOWKEY = param1;
            if (currentColourKey == 3)
                BLUEKEY = param1;
            DEBUG("Current colour key = %s\n", ColourKeyNames[currentColourKey]);

            if (++currentColourKey > 3)
                return 0;

            DEBUG("About to prompt for colour key = %s\n", ColourKeyNames[currentColourKey]);
            TAP_SPrint(str, "Press %s Key", ColourKeyNames[currentColourKey]);
            OSDMenuInfoBoxShow("jdaSkip", str, 0);
            break;

        case _optSkipKeys:
            //set key counter to 1, type to Skip, and start prompting user for keys
            if (currentSkipKey == 0)
                key1 = param1;
            if (currentSkipKey == 1)
                key2 = param1;
            if (currentSkipKey == 2)
                key3 = param1;
            if (currentSkipKey == 3)
                key4 = param1;
            DEBUG("Current skip key = %s\n", SkipKeyNames[currentSkipKey]);

            if (++currentSkipKey > 3)
                return 0;

            DEBUG("About to prompt for skip key = %s\n", SkipKeyNames[currentSkipKey]);
            TAP_SPrint(str, "Press %s Skip Key", SkipKeyNames[currentSkipKey]);
            OSDMenuInfoBoxShow("jdaSkip", str, 0);
            break;

        default:
            DEBUG("INTERNAL ERROR: Invalid item for Infobox to show");
            break;
        }
        return 0;
    }

    switch (param1) {
    case RKEY_Left:
    case RKEY_Right:
        menuLeftRight_handler(param1);
        return 0;
        break;
    case RKEY_Ok:
        menuOK_handler();
        return 0;
        break;
    case RKEY_Info:
        //menuInfo_handler();
        return 0;
        break;
    case RKEY_Exit:
        HideMenu();
        return 0;
        break;
    default:
        break;
    }

    //Hitting menu key while menu is up also removes the screen
    if (param1 == MENUKEY) {
        HideMenu();
    }
    return 0;
}
