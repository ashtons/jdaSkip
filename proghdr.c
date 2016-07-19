// Version: 
//			1.0: - initial release. Config done by setting params in .ini file
//			1.1: - fix bug where blue key couldn't be used while cutting
//			1.2: - allow use of Green key. Also if any key is missing from ini, or 0, then don't override it
//			1.3: - use dialogs to set options
//			1.4: - fix skip not working in Timeshift mode - need to use a default skip as no duration is available
//					- add Mode 1 stuff, operate while PB is up
//					- add Mode 2, any key support
//			1.5: - separate playback code into separate file
//					- add Mode 3: couloured keys only, don't show PB, don't operate if PB is up
//			1.5-1: - fix bug where you can't change the menu key when in Mode 3
//			1.6: - fix bug where keys were actioned when they shouldn't be; 
//					- new menu ala 2400; 
//					- fix going to timeshift from live
//			1.7: - use /ProgramFiles/Settings as ini location
//				 - replace MODE with 2 options:
//					1. "reserve standard keys for progress bar"  Yes/No          ie. Red/Green/Yellow/Blue
//					2. "display progressbar when skipping"         yes/No
//			1.7-1: - change ini loader to inhouse. and fix bug where jdaSkip.ini folder was created
//			1.7-2: - bug where left/right/up/down were captured during file cut dialog
//			2.0b1: - Add binary skip mode settings (no code yet)
//				   - Add press-and-hold
//			2.0b2: - press-and-hold: don't go back before 0, show % on the progress
//			2.1: - support TMSCommander, calling Menu and Stopping
//					 fix bug where if you change channel and then try to timeshift straight away it crashes the box (ie timeshift not available)
//				    add option to tell whether the firmware support quad recording
//			2.2b1: - support media files
//			2.3:   - increment in 1 sec lots if less than 10 seconds, otherwise 5 seconds
//			2.4:   - press-and-hold - show new position asap after releasing 
//				   - fix skip back from live not working
//				   - fix incorrect percentage when time shifting a recording
//			2.4a:  - fix incorrect percentage
//			2.4b:  - added logging/tracing to file
//				   - fix where skipping doesn't work when SHOWPROGBAR is set
//			2.4c:  - fix bug where it skips even when reserve keys is set and PB displays
//			2.4d:  - fix logic bugs with timeshift processing (thanks to Tango)
//			2.4e:  - fix bug where if you use Left/Right as a skip key it can sometimes be captured when a dialog is onscreen
//			2.4f:  - fix bug where if you use |< or >| as a skip key then they can't be used on trim. User needs to use RESERVEKEYS to allow this
//				   - allow RED key to be used even on RESERVEKEYS
//			2.4g:  - fix bug where if press-and-hold pass the end of a recording and the recording ends while you're pressing, it will reboot
//			2.4h:  - support new remotes where Red key is seen as RKEY_F1 and not RKEY_NewF1
//			2.5:   - generic jdaSkip - config any remote by asking for key input
//			2.6:   - (future)start on binary skip
//
// author:	deangelj

#define INI_FILE "jdaSkip.ini"
#define PGM_NAME "jdaSkip"
#define PGM_VERS "V2.5"
#define PGM_FULL PGM_NAME " " PGM_VERS
