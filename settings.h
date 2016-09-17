
#ifndef _SETTINGS_H
#define _SETTINGS_H 1

void LoadSettings();
bool SaveSettings();
void ChangeDirSettings();

dword key1;
dword key2;
dword key3;
dword key4;
int key1Len;
int key2Len;
int key3Len;
int key4Len;

typedef enum {
    SKIPMODE_STD,
    SKIPMODE_BINARY
} SkipMode;

//Which mode we're in
SkipMode MODE; //legacy - 1.6 and earlier

typedef enum {
    PRESSANDHOLD_TIME,
    PRESSANDHOLD_PERCENT,
    PRESSANDHOLD_TIMEPERCENT,
    PRESSANDHOLD_NONE
} PHMODE;

//Which press-and-hold mode we're in
PHMODE PRESSANDHOLD;

//Trace mode - true means write to Telnet window
bool TRC;
//Which key brings up the config menu
dword MENUKEY;
//Actual values of R/G/Y/B according to remote
dword REDKEY;
dword GREENKEY;
dword YELLOWKEY;
dword BLUEKEY;

//from 1.7 on
bool RESERVEKEYS; //"reserve standard keys for progress bar"  Yes/No          ie. Red/Green/Yellow/Blue
bool SHOWPROGBAR; //"display progressbar when skipping"         yes/No
bool QUADRECORDINGSSUPPORTED; //2 or 4 depending on firmware

#endif /* _SETTINGS_H */
