/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project : Ingres Visual DBA
**
**    Source : settings.h
**    application-global settings
**
**   History after 01-Jan-2000:
**
**   25-Jan-2000 (noifr01)
**   (bug 100104) added prototypes of new functions allowing to map specific
**   object types only, for the background refresh settings  dialog ,
**   according to the way VDBA possibly has been "invoked in the context" 
**   27-Mar-2001 (noifr01)
**   (sir 104270) removal of code for managing Ingres/Desktop
**   08-Apr-2003 (noifr01)
**   (sir 107523) management of sequences
**************************************************************************/


#ifndef SETTINGS_HEADER
#define SETTINGS_HEADER

#include "dbatime.h"

// Global status bar preferences
typedef struct _tagStatusBarPreferences {
  BOOL bVisible;        // IDD_SBPREF_VISIBLE
  BOOL Date;            // IDD_SBPREF_DATE
  BOOL Time;            // IDD_SBPREF_TIME
  BOOL ServerCount;     // IDD_SBPREF_SERVERCOUNT
  BOOL CurrentServer;   // IDD_SBPREF_CURRENTSERVER
  BOOL ObjectsCount;    // IDD_SBPREF_OBJECTSCOUNT
  BOOL Environment;     // IDD_SBPREF_ENVIRONMENT
  HFONT hFont;          // IDD_SBPREF_CHOOSEFONT
} STATUSBARPREF, FAR *LPSTATUSBARPREF;


// Global tool bar preferences
typedef struct _tagToolBarPreferences {
  BOOL bVisible;    // is the global tool bar visible ?
} TOOLBARPREF, FAR *LPTOOLBARPREF;

// DOM documents preferences
typedef struct _tagDomPreferences {
  BOOL b3d;         // tree lines of 3d type?
  HFONT hFont;      // font used for tree lines - Dialog box only
} DOMPREF, FAR *LPDOMPREF;

// Sessions preferences
typedef struct tagSESSION
{
   long    number;   // must be in [SessionMin; SessionMax]
   long    time_out; // (second)

} SESSIONPARAMS, FAR * LPSESSIONPARAMS;

// preferences - global structures
extern STATUSBARPREF StatusBarPref; // Preferences for global status bar
extern TOOLBARPREF   ToolBarPref;   // Preferences for global tool bar
extern DOMPREF       DomPref;       // Preferences for dom documents


#define MAXREFRESHOBJTYPES   27

extern int ObjToBeRefreshed[MAXREFRESHOBJTYPES];

extern BOOL        DBAGetDefaultSettings(HWND hWnd);
extern int         GetRefreshSettingPos(int iobjecttype);
extern LPFREQUENCY GetSettingsFrequencyParams(int iobjecttype);

#define MAX_OBJ_TYPE_LEN 50
typedef struct tagOBJECT_STRING
{
   int     number;
   char    szString [MAX_OBJ_TYPE_LEN];
} OBJECT_STRING;

#define MAX_UNIT_STRING  7
typedef struct tagUNIT_STRING
{
   int     number;
   char    szString [MAX_OBJ_TYPE_LEN];
} UNIT_STRING;

#define REFRESH_WINDOW_POPUP   1
#define REFRESH_STATUSBAR      2
#define REFRESH_NONE           3

extern int             RefreshMessageOption;
extern BOOL            RefreshSyncAmongObject;
extern FREQUENCY       FreqSettings [MAXREFRESHOBJTYPES]; // Global;
extern OBJECT_STRING   RefreshSettingsObjectTypeString [MAXREFRESHOBJTYPES];
extern UNIT_STRING     UnitString [MAX_UNIT_STRING];

extern SESSIONPARAMS  GlobalSessionParams;
extern BOOL           bBkRefreshOn;         // activate/deactivate bk refresh

void InitObjectType ();
void InitUnit ();
void LoadBkRefreshDefaultSettingsFromIni (LPFREQUENCY lpFreqSettings);
BOOL SaveBkTaskEnableState();

BOOL SettingLoadFlagSaveAsDefault();
void SettingSaveFlagSaveAsDefault(BOOL bSaveAsDefault);

BOOL SettingLoadFlagRefreshSyncAmongObject();
void SettingSaveFlagRefreshSyncAmongObject(BOOL bRefreshSyncAmongObject);

int  SettingLoadFlagRefreshMessageOption();
void SettingSaveFlagRefreshMessageOption(int nRefreshMessageOption);

// Jfs Added some INI file functions

extern VOID   LoadOtherSettingsFromIni(HWND hwnd);
extern int    ReadIniString(UINT uiSection,UINT uiEntry,LPSTR lpBuffer,UINT cbMax);
extern BOOL   WriteIniString(UINT uiSection,UINT uiEntry,LPSTR lpBuffer);
extern HFONT  ReadFontFromIni(UINT uiSection,UINT uiEntry);
extern BOOL   WriteFontToIni(UINT uiSection ,UINT uiEntry,HFONT hFont);

int GetMaxRefreshObjecttypes();
int RefObjTypePosInArray(int);

#endif
