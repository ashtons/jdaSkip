//-------------------------------------------------------------------------------------------------------
// playback.c - playback code
//-------------------------------------------------------------------------------------------------------

#define _TMS_

#include "keys.h"
#include "libFireBird.h"
#include "settings.h"
#include "skipmenu.h"
#include "tap.h"

//#include "images/prgbar.gd"

//not defined in tap.h so define them here
#define PLAYMODE_AVI_DIVX 7
#define PLAYMODE_TS_MP4_MKV 8

//------------------------------ Statics --------------------------------------
//

//------------------------------ Types --------------------------------------
//

//------------------------------ Global Variables --------------------------------------
//

// Current playback info
TYPE_PlayInfo _playInfo;
TYPE_RecInfo _recInfo;

// Current state within event handler
extern dword _subState;

word inforegion; //where the press-and-hold graphic is displayed

bool Repeating = FALSE; //whether we are in press-and-hold mode
int SkipSecs = 0; //# of seconds to skip (+ or -)
int SkipTotalSecs = 0; //Total number of seconds in skip
dword SkipBPS = 0; //# of blocks-per-second in playback
bool ShowingGraphic = FALSE; //whether we're showing the final repeat mode graphic
dword ShowingGraphicTimeout; //when to remove graphic if on the screen

typedef enum {
    SKIPDIR_NONE,
    SKIPDIR_FWD,
    SKIPDIR_BACK
} SkipDirection;

//For binary skip, which direction are we currently going
SkipDirection LastSkipDirection = SKIPDIR_NONE;
int LastSkipAmount = 0; //absolute amount of seconds to skip

//------------------------------ Function prototypes --------------------------------------
//
static dword getBPS(void);
static void UpdateGraphic();
static void ShowGraphic();
static void FinalGraphicPart1(dword target);
static void FinalGraphicPart2();
static void UpdateSkipData(dword k);
static dword CalculateSkip(bool haveTimeshifted);
static int getRepeatState(dword param1);
static bool IsSkippableMode(dword key);
static bool SetTimeshift();
static void SkipToTarget(dword targetBlock);

//------------------------------ Start of Subroutines --------------------------------------
//
extern void WriteLog(char* buff);
char LogPuffer[256];
#define DEBUG(...)                       \
    if (TRC) {                           \
        sprintf(LogPuffer, __VA_ARGS__); \
        WriteLog(LogPuffer);             \
    }
//#define DEBUG      if (TRC) TAP_PrintNet

//-------------------------------------------------------------------------------------------------------
// Press-and-hold graphics functions
//-------------------------------------------------------------------------------------------------------

//UpdateGraphic - update the progress of press-and-hold
//-------------------------------------------------------------------------------------------------------
static void UpdateGraphic()
{
    char Text[50];
    int width = 200;
    //int width = 350;
    int hrs, min, sec, total;

    if (PRESSANDHOLD == PRESSANDHOLD_TIME || PRESSANDHOLD == PRESSANDHOLD_TIMEPERCENT) {
        total = SkipTotalSecs > 0 ? SkipTotalSecs : -SkipTotalSecs;
        hrs = total / 3600;
        min = (total % 3600) / 60;
        sec = total - ((hrs * 3600) + (min * 60));
        //DEBUG("TotalSecs: %d\n", SkipTotalSecs);
        TAP_SPrint(Text, "%c%02d:%02d:%02d", SkipTotalSecs > 0 ? ' ' : '-', hrs, min, sec);
        //Align left to leave room for final stats
        TAP_Osd_PutS(inforegion, 10, 6, width - 10, Text, RGB(255, 255, 224), RGB(24, 24, 24), 0, FNT_Size_1419, FALSE, ALIGN_LEFT);
        //TAP_Osd_PutS(inforegion, 10, 6, width-10, Text, RGB(255,255,224), 0xff, 0, FNT_Size_1419, FALSE, ALIGN_LEFT);
    }

    //Can we update the skip %?
    if ((PRESSANDHOLD == PRESSANDHOLD_PERCENT || PRESSANDHOLD == PRESSANDHOLD_TIMEPERCENT) && (_playInfo.playMode == PLAYMODE_Playing || _playInfo.playMode == PLAYMODE_AVI_DIVX || _playInfo.playMode == PLAYMODE_TS_MP4_MKV)) {
        dword target;
        if ((SkipTotalSecs < 0) && ((SkipBPS * -SkipTotalSecs) > _playInfo.currentBlock))
            target = 500; //about 10 seconds from start
        else
            target = (SkipBPS * SkipTotalSecs) + _playInfo.currentBlock;

        TAP_SPrint(Text, "%.1f%%", (((float)target) / ((float)_playInfo.totalBlock)) * 100);
        TAP_Osd_PutS(inforegion, 100, 6, width - 10, Text, RGB(255, 255, 224), RGB(24, 24, 24), 0, FNT_Size_1419, FALSE, ALIGN_RIGHT);
        //TAP_Osd_PutS(inforegion, 100, 6, width-10, Text, RGB(255,255,224), 0xff, 0, FNT_Size_1419, FALSE, ALIGN_RIGHT);
    }

    TAP_Osd_Sync();
}

//ShowGraphic - set up the OSD that will display the progress of press-and-hold
//-------------------------------------------------------------------------------------------------------
static void ShowGraphic()
{
    int width = 200;
    int height = 30;
    int x_pos = 460;
    int y_pos = 65;

    /*
	int width = 350;
	int height = 140;
	int x_pos = 360;
	int y_pos = 65;
	*/

    if (ShowingGraphic) {
        //Delete the graphic
        TAP_Osd_Delete(inforegion);
        //TAP_SysOsdControl(SYSOSD_PvrInfo, TRUE);
        //TAP_Osd_Sync();
        ShowingGraphic = FALSE;
    }

    // Disable screen info
    TAP_SysOsdControl(SYSOSD_PvrInfo, FALSE);

    // Create the OSD
    inforegion = TAP_Osd_Create(x_pos, y_pos, width, height, 0, OSD_Flag_PlaneSubt);

    // Draw a box - this will be filled by the time updates
    TAP_Osd_Draw3dBoxFill(inforegion, 0, 0, 200, 30, RGB(127, 127, 127), RGB(127, 127, 127), RGB(24, 24, 24));

    UpdateGraphic();

    // Enable screen info??
    //TAP_SysOsdControl(SYSOSD_PvrInfo, TRUE);
}

//FinalGraphic - show the final progress of press-and-hold, including the stats of where we are, then delete the graphic
//-------------------------------------------------------------------------------------------------------
static void FinalGraphicPart1(dword target)
{
    int width = 200;
    char Text[50];

    switch (_playInfo.playMode) {
    case PLAYMODE_None:
        TAP_SPrint(Text, "No skip");
        break;
    case PLAYMODE_Playing: //normal playback
    case PLAYMODE_TempPlaying: //timeshift
    case PLAYMODE_AVI_DIVX:
    case PLAYMODE_TS_MP4_MKV:
        TAP_SPrint(Text, "%.1f%%", ((float)(target * 100)) / ((float)_playInfo.totalBlock));
        break;
    case PLAYMODE_RecPlaying: //timeshifting in a recording
        DEBUG("Target: %u, Duration: %u, TotalBlock: %u, RecordedSec: %u\n", target, _playInfo.duration * 60 + _playInfo.durationSec, _playInfo.totalBlock, _recInfo.recordedSec);
        TAP_SPrint(Text, "%.1f%%", ((float)(target * 100)) / ((float)((_playInfo.duration * 60 + _playInfo.durationSec) * (_playInfo.totalBlock / _recInfo.recordedSec))));
        break;
    default:
        TAP_SPrint(Text, "No data");
        break;
    }

    //Show final stats then delete graphic
    TAP_Osd_PutS(inforegion, 100, 6, width - 10, Text, RGB(255, 255, 224), RGB(24, 24, 24), 0, FNT_Size_1419, FALSE, ALIGN_RIGHT);
    //TAP_Osd_PutS(inforegion, 100, 6, width-10, Text, RGB(255,255,224), 0xff, 0, FNT_Size_1419, FALSE, ALIGN_RIGHT);

    //TAP_Osd_PutGd(inforegion, 0, 0, &_prgbarGd, FALSE );

    TAP_Osd_Sync();

    ShowingGraphic = TRUE;
    ShowingGraphicTimeout = TAP_GetTick() + 200; //now + 2 seconds
}

//FinalGraphic Part2 - delete the graphic
//-------------------------------------------------------------------------------------------------------
static void FinalGraphicPart2()
{
    //Delete the graphic
    TAP_Osd_Delete(inforegion);
    TAP_SysOsdControl(SYSOSD_PvrInfo, TRUE);
    TAP_Osd_Sync();
}

//-------------------------------------------------------------------------------------------------------
// UpdateSkipData - set the counters depending on whether we're repeating or not
//-------------------------------------------------------------------------------------------------------

static void UpdateSkipData(dword k)
{

    if (Repeating) {
        SkipTotalSecs += SkipSecs;
    }
    else //not repeating
    {

        int secs = 0;

        if (MODE == SKIPMODE_BINARY) {
            //LastSkipDirection = (fwd?SKIPDIR_FWD:SKIPDIR_BACK);
            secs = LastSkipAmount;
            //do something here!!!!
        }
        else {

            if (k == key1)
                secs = key1Len;
            else if (k == key2)
                secs = key2Len;
            else if (k == key3)
                secs = key3Len;
            else if (k == key4)
                secs = key4Len;
        }

        SkipSecs = secs;
        SkipTotalSecs = secs;
    }
    DEBUG("UpdateSkipData: SkipSecs: %d, SkipTotalSecs: %d\n", SkipSecs, SkipTotalSecs);
}

//-------------------------------------------------------------------------------------------------------
// Returns whether current channel is recording and sets the recinfo structure accordingly
//-------------------------------------------------------------------------------------------------------

static bool IsCurrentChannelRecording(TYPE_RecInfo* recInfo)
{
    int svcType, svcNum, i;

    DEBUG("------------------ IsCurrentChannelRecording --------------------------\n");

    // Get current channel
    TAP_Channel_GetCurrent(&svcType, &svcNum);
    if (svcType != SVC_TYPE_Tv)
        return FALSE;

    // Hack to for "no signal" on channel causing crash (eg selecting Analog in with nothing there)
    if ((TAP_Channel_GetTotalAudioTrack() == 0) && (TAP_Channel_GetTotalSubtitleTrack() == 0)) {
        DEBUG("IsCurrentChannelRecording found no Audio or Subtitle tracks. Therefore no skipping allowed");
        return FALSE;
    }

    //Is the current channel currently recording or timeshifting?
    for (i = 0; i < (QUADRECORDINGSSUPPORTED ? 5 : 3); i++) {
        TAP_Hdd_GetRecInfo(i, recInfo);

        //if (recInfo->svcNum == svcNum)
        if ((recInfo->recType != RECTYPE_None) && (recInfo->svcNum == svcNum)) {
            DEBUG("IsCurrentChannelRecording: Found match on Recinfo slot: %d, Recinfo svcNum: %d, Channel svcNum: %d\n", i, recInfo->svcNum, svcNum);
            return TRUE;
        }
    }

    return FALSE;
}

//-------------------------------------------------------------------------------------------------------
// Returns whether we're in the correct mode for skipping back
// ie. we must be playing back a video, timeshifting, or capable of timeshifting.
//-------------------------------------------------------------------------------------------------------

static bool IsSkippableMode(dword key)
{
    //TYPE_PlayInfo playInfo;
    int svcType, svcNum, stat;

    DEBUG("IsSkippableMode subState=%d\n", _subState);

    //can't skip if we're in some non-std mode except when the search bar is up
    if ((_subState != SUBSTATE_Normal) && (_subState != SUBSTATE_PvrTimeSearch) && (_subState != SUBSTATE_PvrPlayingSearch) && (_subState != SUBSTATE_PvrRecSearch))
        return FALSE;

    // Check we're in TV, not radio, mode
    TAP_Channel_GetCurrent(&svcType, &svcNum);
    if (svcType != SVC_TYPE_Tv)
        return FALSE;

    // Now check we're watching a video being played back or timeshifting
    TAP_Hdd_GetPlayInfo(&_playInfo);
    DEBUG("IsSkippableMode: Set _playinfo...playmode=%d\n", _playInfo.playMode);

    switch (_playInfo.playMode) {
    case PLAYMODE_Playing:
    case PLAYMODE_RecPlaying:
    case PLAYMODE_TempPlaying:
    case PLAYMODE_AVI_DIVX:
    case PLAYMODE_TS_MP4_MKV:
        return TRUE;
        break;
    case PLAYMODE_None: //if no playback only allow skipback
        if ((key == key1 && key1Len < 0) || (key == key2 && key2Len < 0) || (key == key3 && key3Len < 0) || (key == key4 && key4Len < 0)) {
            return TRUE;
        }
        else
            return FALSE;
        break;
    default:
        return FALSE;
        break;
    }
}

//-------------------------------------------------------------------------------------------------------
// getBPS - get Blocks per second rate from the current playback
//
// blocksize is 9024 (48 * 188). Divide playinfo.totalBlock by (.duration*60 + .durationSec) to get a time based factor
// _playinfo structure must be already set prior to calling this routine.
// Side effect is that _recInfo structure is set for timeshift or recplay modes
//-------------------------------------------------------------------------------------------------------

static dword getBPS(void) // Blocks Per Seconds
{

    DEBUG("(getBPS) Playmode: %d, totalBlock: %u, duration: %u, currentBlock: %u\n", _playInfo.playMode, _playInfo.totalBlock, (_playInfo.duration * 60) + _playInfo.durationSec, _playInfo.currentBlock);

    switch (_playInfo.playMode) {
    case PLAYMODE_None:
        return 0;
        break;
    case PLAYMODE_Playing: //2
    case PLAYMODE_AVI_DIVX:
    case PLAYMODE_TS_MP4_MKV:
        return (_playInfo.totalBlock / ((_playInfo.duration * 60) + _playInfo.durationSec));
        break;
    case PLAYMODE_TempPlaying: //3
        TAP_Hdd_GetRecInfo((QUADRECORDINGSSUPPORTED ? 4 : 2), &_recInfo);
        if (_recInfo.recType == RECTYPE_Timeshift && _recInfo.recordedSec > 0) {
            DEBUG("(getBPS) Playmode: %d, ttlBlk: %u, recordedSec: %u\n", _playInfo.playMode, _playInfo.totalBlock, _recInfo.recordedSec);
            return _playInfo.totalBlock / _recInfo.recordedSec;
        }
        else {
            DEBUG("(getBPS) No duration available - use default.. 150\n");
            return 150; //no duration - take a guess
        }
        break;
    case PLAYMODE_RecPlaying: //4
        if (IsCurrentChannelRecording(&_recInfo) && (_recInfo.recType != RECTYPE_None)) {
            return _playInfo.totalBlock / _recInfo.recordedSec;
        }
        else {
            DEBUG("(getBPS) No duration available - use default.. 150\n");
            return 150; //no duration available - take a guess...
        }

        break;
    }
    return 0;
}

//-------------------------------------------------------------------------------------------------------
// CalculateSkip - calculate skip based on how many secs we've supposed to move.
//-------------------------------------------------------------------------------------------------------

static dword CalculateSkip(bool haveTimeshifted)
{
    // convert seconds to blocks and add/subtract from current
    dword targetBlock = 0;
    dword blocks;
    bool fwd;
    int secs;

    secs = SkipTotalSecs;
    if (secs < 0) {
        if (!haveTimeshifted && _playInfo.currentBlock == 0xFFFFFFFF)
            return 0; //at start of buffer

        fwd = FALSE;
        secs = -secs;
    }
    else if (secs > 0) {
        fwd = TRUE;
    }
    else // secs == 0 - ignore this key
    {
        return 0;
    }

    //do we need to adjust secs based on binary skip change of direction?
    if (MODE == SKIPMODE_BINARY) {
        if (LastSkipDirection != SKIPDIR_NONE) {
            if ((LastSkipDirection == SKIPDIR_FWD && !fwd) || (LastSkipDirection == SKIPDIR_BACK && fwd)) {
                secs = LastSkipAmount / 2;
            }
        }
        LastSkipDirection = (fwd ? SKIPDIR_FWD : SKIPDIR_BACK);
        LastSkipAmount = secs;
    }

    //find te target block based on the type of playback, timeshift etc - sets _recInfo if timeshift or recplaying
    blocks = getBPS() * secs;

    DEBUG("-------CalculateSkip---------------------\n");
    DEBUG("Skipping %d seconds\n", secs);
    DEBUG("Playinfo: currBlk: %u, ttlBlk: %u, blocks to move: %u, %s\n", _playInfo.currentBlock, _playInfo.totalBlock, blocks, fwd ? "Forward" : "Back");

    if (fwd) // skipping forward, make sure we don't try and go past the end
    {
        if (blocks == 0) //usually live tv, so if skipping forward ignore it
            return 0;

        if (_playInfo.currentBlock != 0xFFFFFFFF)
            targetBlock = _playInfo.currentBlock + blocks;
        else
            targetBlock = blocks;

        if (targetBlock > _playInfo.totalBlock - 500) // don't go past end - 500, about 10 seconds
            targetBlock = _playInfo.totalBlock - 500;
    }
    else {
        //check for currentBlock == -1 in case of chaseplaying and haven't timeshifted yet
        if (blocks == 0) // usually live tv, so if skipping back set timeshift
        {
            DEBUG("********* Going back and blocks 0!!! Setting blocks to 1000\n");
            blocks = 1000;
        }

        if (_playInfo.currentBlock == 0xFFFFFFFF) //just set timeshift - no currentblock - assume skip from end
        {
            DEBUG("********* Going back and currentBlock = -1\n");
            if (blocks > _playInfo.totalBlock)
                targetBlock = 0; //min blocks to skip back
            else
                targetBlock = _playInfo.totalBlock - blocks;
        }
        else if (blocks > _playInfo.currentBlock) // back too far
            targetBlock = 500; //min blocks
        else if (_playInfo.currentBlock > _playInfo.totalBlock) //wierdness from timeshift in code
            targetBlock = _playInfo.totalBlock - blocks;
        else
            targetBlock = _playInfo.currentBlock - blocks;
        DEBUG("targetBlock: %u\n", targetBlock);
    }

    return targetBlock;
}

static void SkipToTarget(dword targetBlock)
{
    DEBUG("Skipping to block %u\n", targetBlock);

    //if progressbar is on screen don't take it off...

    // 2.4f: fix if progbar already on screen - thanks Tango
    // Allow ProgressBar to stay on screen if it is already on (only possible for if "display ProgBar" setting = NO
    // and RED is not considered reserved)
    if ((SHOWPROGBAR) || (_subState == SUBSTATE_PvrPlayingSearch)) {
        TAP_Hdd_ChangePlaybackPos(targetBlock);
    }
    else {
        // Disable OSD
        TAP_SysOsdControl(SYSOSD_PvrInfo, FALSE);
        // Disable OSD (do not show PVR progress bar)
        TAP_ExitNormal();
        TAP_Hdd_ChangePlaybackPos(targetBlock);
        // Enable OSD
        TAP_EnterNormal();
        TAP_SysOsdControl(SYSOSD_PvrInfo, TRUE);
        //TAP_SystemProc();
    }
}

//-------------------------------------------------------------------------------------------------------
// getRepeatState -
//-------------------------------------------------------------------------------------------------------

static int getRepeatState(dword param1)
{
    //static dword last_key = 0;
    //static int repeatState = -1;   //-1: not repeating, 0: startofrepeat; 1: endofrepeat

    //ignore start repeat/end repeat keys (2400 gives 2 key events for a key stroke, eg.
    // 0x001003C
    // 0x201003C   <-- end of repeat
    //-or-
    // 0x001003C
    // 0x101003C	 <-- start of repeat
    // 0x001003C
    // 0x001003C
    // 0x001003C
    // 0x001003C
    // 0x201003C   <-- end of repeat

    if (param1 > 0x11000) //repeat key
    {
        if (param1 == (param1 | 0x1000000)) {
            //start of repeat
            DEBUG("getRepeatState...start of repeat\n");
            return 0;
        }
        else if (param1 == (param1 | 0x2000000)) {
            DEBUG("getRepeatState...end of repeat\n");
            return 1;
        }
        else {
            //different key and a repeat key????
            DEBUG("getRepeatState...ignored unexpected high value key: %u\n", param1);
            return -1;
        }
    }
    else {
        return -1; //not repeating
    }
}

//-------------------------------------------------------------------------------------------------------
// SetTimeshift - go back from live tv
//-------------------------------------------------------------------------------------------------------

static bool SetTimeshift()
{
    TYPE_RecInfo recInfo;
    //int svcType, svcNum, i;
    //bool foundSvc = FALSE;

    DEBUG("------------------ SetTimeShift --------------------------\n");

    /*
	// Get current channel
	TAP_Channel_GetCurrent( &svcType, &svcNum );
	if (svcType != SVC_TYPE_Tv) return FALSE;

	//Is the current channel currently recording or timeshifting?
	for (i=0; i<QUADRECORDINGSSUPPORTED?4:2; i++)
	{
		if (TAP_Hdd_GetRecInfo(i, &recInfo)) 
		{
			DEBUG("Set timeshift... Recinfo slot: %d, type: %d, secs: %d\n", i, recInfo.recType, recInfo.recordedSec);
		}
		else
			return FALSE;

		DEBUG("Set timeshift... Recinfo slot: %d, Recinfo svcNum: %d, Channel svcNum: %d\n", i, recInfo.svcNum, svcNum);

		if (recInfo.svcNum == svcNum)
		{
			foundSvc = TRUE;
			break;
		}
	}
	if (!foundSvc) return FALSE;
	*/

    if (!IsCurrentChannelRecording(&recInfo))
        return FALSE;

    //check if timeshift is currently recording - what about if we're going from live into a timer recording?
    //Fix bug not going back from live??
    if (recInfo.recType != RECTYPE_Timeshift && recInfo.recType != RECTYPE_Normal) {
        DEBUG("Set timeshift... can't yet as Timeshift not set yet\n");
        return FALSE;
    }

    DEBUG("Set timeshift... recordedSec: %d\n", recInfo.recordedSec);
    if (recInfo.recordedSec < 10) {
        DEBUG("Set timeshift... can't yet as Timeshift recording not ready\n");
        return FALSE;
    }
    DEBUG("Set timeshift... goback to 0 blocks\n");
    TAP_Hdd_ChangePlaybackPos(0); //crashes if just changed to a new channel and Timeshift mode not available yet
    //then get the new playmode data
    DEBUG("Get _playinfo..\n");
    TAP_Hdd_GetPlayInfo(&_playInfo);
    DEBUG("Set _playinfo...playmode now=%d\n", _playInfo.playMode);
    DEBUG("New Playinfo: currBlk: %u, ttlBlk: %u\n", _playInfo.currentBlock, _playInfo.totalBlock); //currentBlock is probably incorrect
    return TRUE;
}

//-------------------------------------------------------------------------------------------------------
// handle idle event
//-------------------------------------------------------------------------------------------------------

dword idle_handler(dword param1)
{
    if (ShowingGraphic && (TAP_GetTick() > ShowingGraphicTimeout)) {
        FinalGraphicPart2();
        ShowingGraphic = FALSE;
    }
    return param1;
}

//-------------------------------------------------------------------------------------------------------
// handle_non_menu_keys
//-------------------------------------------------------------------------------------------------------

dword handle_non_menu_keys(dword param1)
{

    const dword MASK = 0x1FFFF; //used to mask off high bits that contain repeat stuff
    dword tKey = param1 & MASK; //use low word only for comparisons
    dword target = 0;

    DEBUG("handle_non_menu_keys: Key: %u, tKey: %u, Repeating: %s\n", param1, tKey, Repeating ? "TRUE" : "FALSE");

    //Only check if not repeating. If we're repeating then we can only get the same key or an end code.
    if (!Repeating) {
        //ignore if its not one of our keys
        switch (MODE) {
        case SKIPMODE_STD:
            if ((tKey != key1) && (tKey != key2) && (tKey != key3) && (tKey != key4)) {
                DEBUG("skip mode std but key is not one of ours: Key: %u, tKey: %u, k1: %u, k2: %u, k3: %u, k4: %u\n", param1, tKey, key1, key2, key3, key4);
                return param1;
            }
            else
                break;
        case SKIPMODE_BINARY:
            if ((tKey != key1) && (tKey != key2)) {
                DEBUG("skip mode binary but key is not one of ours: Key: %u, tKey: %u\n", param1, tKey);
                return param1;
            }
            else
                break;
        default: //shouldn't get here
            DEBUG("skip mode unknown\n");
            return param1;
        }

        //check invalid skip modes
        //2.4f change: allow RED key even on RESERVEKEYS, and don't capture |< or >| on RESERVEKEYS as they could be needed
        //Don't capture GREEN/YELLOW/BLUE if progress bar is up and RESERVEKEYS is set and SHOWPROGBAR is not set
        //if ((_subState == SUBSTATE_PvrTimeSearch || _subState == SUBSTATE_PvrPlayingSearch || _subState == SUBSTATE_PvrRecSearch) &&
        //2.5 allow user to set Red/Green/Yellow/Blue
        //	RESERVEKEYS && (tKey == RKEY_F2 || tKey == RKEY_F3 || tKey == RKEY_F4 || tKey == RKEY_Prev || tKey == RKEY_Next))
        if ((_subState == SUBSTATE_PvrTimeSearch || _subState == SUBSTATE_PvrPlayingSearch || _subState == SUBSTATE_PvrRecSearch) && RESERVEKEYS && (tKey == GREENKEY || tKey == YELLOWKEY || tKey == BLUEKEY || tKey == RKEY_Prev || tKey == RKEY_Next)) {
            DEBUG("Key ignored - RESERVEKEYS and PB up and one of the reseved keys was pressed\n");
            return param1;
        }

        //Don't capture Left/Right/Up/Down when the progress bar is up
        if ((_subState == SUBSTATE_PvrTimeSearch || _subState == SUBSTATE_PvrPlayingSearch || _subState == SUBSTATE_PvrRecSearch) && (tKey == RKEY_Left || tKey == RKEY_Right || tKey == RKEY_Up || tKey == RKEY_Down)) {
            DEBUG("Key ignored - PB up and one of L/R/U/D is pressed\n");
            return param1;
        }

        //ignore if we're not in a skippable mode
        if (!IsSkippableMode(tKey)) //side-effect - sets _playinfo structure
        {
            DEBUG("Key ignored - not in skippable state\n");
            return param1;
        }

        //If we're using Left/Right, handle special case where if you scrollback in timeshift and press record - you get a prompt
        //whether to save or not - there is no indication we are not in normal state
        if (tKey == RKEY_Left || tKey == RKEY_Right) {
            if (isAnyOSDVisibleEx(0, 0, 768, 572, BASE_PLANE)) {
                DEBUG("Key ignored - OSD on screen and key is Left or Right\n");
                return param1;
            }
        }
    }

    //DEBUG("handle_non_menu_keys: substate: %d\n", _subState);
    //looks like its one of ours

    DEBUG("Check repeat state\n");

    switch (getRepeatState(param1)) {
    case -1: //no repeat code in key

        UpdateSkipData(param1);

        if (Repeating) {
            //use existing skipdata to update the graphic
            UpdateGraphic();
        }
        //else do nothing - wait for the end code

        break;
    case 0: //start repeat
        //Setup the graphic with current skip data
        if ((PRESSANDHOLD == PRESSANDHOLD_PERCENT || PRESSANDHOLD == PRESSANDHOLD_TIMEPERCENT) && (_playInfo.playMode == PLAYMODE_Playing || _playInfo.playMode == PLAYMODE_AVI_DIVX || _playInfo.playMode == PLAYMODE_TS_MP4_MKV))
            SkipBPS = _playInfo.totalBlock / ((_playInfo.duration * 60) + _playInfo.durationSec);

        if (PRESSANDHOLD == PRESSANDHOLD_NONE) //user doesn't want repeating
            break;

        ShowGraphic();

        Repeating = TRUE;
        break;
    case 1: //end of repeat

        if (_playInfo.playMode == PLAYMODE_None && SkipTotalSecs < 0) //going back from live - set timeshift first
        {
            if (SetTimeshift())
                target = CalculateSkip(TRUE);
            else
                target = 0; //no skip
        }
        else if (IsSkippableMode(tKey)) //has the mode changed while we were skipping (fix in 2.4g)
        {
            target = CalculateSkip(FALSE);
        }
        else
            target = 0;

        if (Repeating || MODE == SKIPMODE_BINARY)
            FinalGraphicPart1(target);

        if (target > 0 && target < _playInfo.totalBlock)
            SkipToTarget(target); //goto where we're supposed to be

        //Do in idle event
        //if (Repeating)
        //	FinalGraphicPart2();

        SkipSecs = 0; //# of seconds to skip (+ or -)
        SkipTotalSecs = 0; //Total number of seconds in skip
        SkipBPS = 0;
        Repeating = FALSE;
        break;
    }
    return 0;
}
