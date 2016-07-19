#ifndef __SKIPMENU_H
#define __SKIPMENU_H

//phases used in EventHandler
enum inputPhases {
	_initialState,
	_menuActive,
	_keyMenuActive,
};

int _phase;

//menu options - these should match the menu lines in the menu
enum menuOptions {
	_optSkipMode,
	_optColourKeys,
	_optSkipKeys,
//	_optKey1,
	_optKey1Len,
//	_optKey2,
	_optKey2Len,
//	_optKey3,
	_optKey3Len,
//	_optKey4,
	_optKey4Len,
	_optReserveKeys,
	_optDisplayPBar,
	_optMenuKey,
	_optTrace,
	_optPressAndHold,
	_optQuadRecordings,
	_optExitTAP
};

dword menuactive_handler ( dword param1 );
dword keymenuactive_handler ( dword param1 );
void ShowMenu();

#endif
