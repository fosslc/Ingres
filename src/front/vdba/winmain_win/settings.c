/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project : Ingres Visual DBA
**
**    Source : settings.c
**    application-level settings
**
**    Authors : Francois Noirot-Nerin - Emmanuel Blattes
**
**   History after 01-Jan-2000:
**
**   25-Jan-2000 (noifr01)
**   (bug 100104) added functions allowing to map specific object
**   types only, for the background refresh settings  dialog ,
**   according to the way VDBA possibly has been "invoked in the context" 
**   27-Mar-2001 (noifr01)
**   (sir 104270) removal of code for managing Ingres/Desktop
**   10-Dec-2001 (noifr01)
**   (sir 99596) removal of obsolete code and resources
**   28-Mar-2002 (noifr01)
**   (sir 99596) removed additional unused resources/sources
**   18-Mar-2003 (schph01)
**   sir 107523 management of sequences
**************************************************************************/

#include "dba.h"
#include "settings.h"
#include "error.h"
#include "msghandl.h"
#include "winutils.h"
#include "resource.h"
#include "dll.h"
#include "dlgres.h"
#include "dbaginfo.h"

#ifdef WIN32
#endif  // WIN32


// Structures or variables for preferences
STATUSBARPREF StatusBarPref;    // Preferences for global status bar
TOOLBARPREF   ToolBarPref;      // Preferences for global tool bar
DOMPREF       DomPref;          // Preferences for dom documents
BOOL          bBkRefreshOn;     // bk refresh active yes/no

//
//  local functions prototypes
//
static void NEAR LoadBkTaskEnableState(void);


/* When you modify the list.
//
// Make sure that the object type and the string identifier are used in the
// function 'InitObjectType()', otherwise define the string ids in :
//     1) ..\include\msghandl.h
//     2) ..\dbadlg1\dbadlg1.rc
*/
int ObjToBeRefreshed[MAXREFRESHOBJTYPES]={ OT_DATABASE          ,
                                           OT_PROFILE           ,
                                           OT_USER              ,
                                           OT_GROUP             ,
                                           OT_GROUPUSER         ,
                                           OT_ROLE              ,
                                           OT_LOCATION          ,
                                           OT_TABLE             ,
                                           OT_TABLELOCATION     ,
                                           OT_VIEW              ,
                                           OT_VIEWTABLE         ,
                                           OT_INDEX             ,
                                           OT_INTEGRITY         ,
                                           OT_PROCEDURE         ,
                                           OT_SEQUENCE          ,
                                           OT_RULE              ,
                                           OT_SCHEMAUSER        ,
                                           OT_SYNONYM           ,
                                           OT_DBEVENT           ,
                                           OTLL_SECURITYALARM   ,
                                           OTLL_GRANTEE         ,
                                           OTLL_DBGRANTEE       ,
                                           OT_COLUMN            ,
                                           OT_REPLIC_CONNECTION ,
                                           OT_ICE_GENERIC       ,
	/* the 2 following ones must remain the last ones, and the order  */
	/* unchanged, otherwise the logic of the functionality using the  */
	/* GetMaxRefreshObjecttypes() function (and the function itself)  */
	/* must be updated                                                */
                                           OT_NODE              ,
                                           OT_MON_ALL              };

/*
// Global
*/

FREQUENCY      FreqSettings[MAXREFRESHOBJTYPES];
UNIT_STRING    UnitString   [MAX_UNIT_STRING];
OBJECT_STRING  RefreshSettingsObjectTypeString [MAXREFRESHOBJTYPES];
int            RefreshMessageOption = REFRESH_NONE;
BOOL           RefreshSyncAmongObject  = FALSE;

SESSIONPARAMS  GlobalSessionParams;

BOOL DBAGetDefaultSettings(HWND hwnd)
{
   if ( (sizeof(ObjToBeRefreshed)/sizeof(int)) != MAXREFRESHOBJTYPES) {
      myerror(ERR_SETTINGS);
      return FALSE;
   }

   LoadBkRefreshDefaultSettingsFromIni (FreqSettings);
   LoadOtherSettingsFromIni(hwnd);

   return TRUE;
}


void InitObjectType ()
{
   unsigned int i;
   char*    szObjectTypeStr;

   for (i = 0; i < MAXREFRESHOBJTYPES; i++) {
       switch (ObjToBeRefreshed [i])
       {
           case OT_VIRTNODE:
               szObjectTypeStr = ResourceString ((UINT)IDS_VIRTNODE);
               break;
           case OT_DATABASE:
               szObjectTypeStr = ResourceString ((UINT)IDS_I_DATABASE);
               break;
           case OT_USER:       
               szObjectTypeStr = ResourceString ((UINT)IDS_I_USER_I);
               break;
           case OT_PROFILE:    
               szObjectTypeStr = ResourceString ((UINT)IDS_I_PROFILE_I);
               break;
           case OT_GROUP:
                szObjectTypeStr= ResourceString ((UINT)IDS_I_GROUP_I);
               break;
           case OT_GROUPUSER:
               szObjectTypeStr = ResourceString ((UINT)IDS_I_GROUPUSER_I);
               break;
           case OT_ROLE:
               szObjectTypeStr = ResourceString ((UINT)IDS_I_ROLE_I);
               break;
           case OT_LOCATION:
               szObjectTypeStr = ResourceString ((UINT)IDS_I_LOCATION_I);
               break;
           case OT_TABLE:
               szObjectTypeStr = ResourceString ((UINT)IDS_I_TABLE);
               break;
           case OT_TABLELOCATION:
               szObjectTypeStr = ResourceString ((UINT)IDS_I_TABLE_LOCATION_I);
               break;
           case OT_VIEW:
               szObjectTypeStr = ResourceString ((UINT)IDS_I_VIEW);
               break;
           case OT_VIEWTABLE:
               szObjectTypeStr = ResourceString ((UINT)IDS_I_VIEWTABLE);
               break;
           case OT_INDEX:
               szObjectTypeStr = ResourceString ((UINT)IDS_I_INDEX);
               break;
           case OT_INTEGRITY:
               szObjectTypeStr = ResourceString ((UINT)IDS_I_INTEGRITY_I);
               break;
           case OT_PROCEDURE:
               szObjectTypeStr = ResourceString ((UINT)IDS_I_PROCEDURE);
               break;
           case OT_SEQUENCE:
               szObjectTypeStr = ResourceString ((UINT)IDS_I_SEQUENCE);
               break;
           case OT_RULE:
               szObjectTypeStr = ResourceString ((UINT)IDS_I_RULE_TRIGGER); // DIRTYOIDT
               break;
           case OT_SCHEMAUSER:
               szObjectTypeStr = ResourceString ((UINT)IDS_I_SCHEMA_I);
               break;
           case OT_MON_ALL:
               szObjectTypeStr = ResourceString ((UINT)IDS_I_MONITOR_DATA_I); 
               break;
           case OT_NODE:
               szObjectTypeStr = ResourceString ((UINT)IDS_I_NODE_DEFS); 
               break;
           case OT_SYNONYM:
               szObjectTypeStr = ResourceString ((UINT)IDS_I_SYNONYM);
               break;
           case OT_DBEVENT:
#ifdef ENGINE_OK
               szObjectTypeStr = ResourceString ((UINT) IDS_I_EVENT_DBEVENT );// DIRTYOIDT
#else
               szObjectTypeStr = ResourceString ((UINT) IDS_I_DBEVENT_I ); 
#endif
               break;
           case OT_ICE_GENERIC:  
               szObjectTypeStr = ResourceString ((UINT) IDS_I_ICE_OBJS ); 
               break;
           case OTLL_SECURITYALARM:  
               szObjectTypeStr = ResourceString ((UINT) IDS_I_SECALARM_I );
               break;
           case OT_ROLEGRANT_USER:  
               szObjectTypeStr = ResourceString ((UINT) IDS_I_ROLE_GRANTEES ); 
               break;
           case OTLL_GRANTEE:  
               szObjectTypeStr = ResourceString ((UINT)IDS_I_GRANTEE);
               break;
           case OTLL_DBGRANTEE:  
           case OTLL_OIDTDBGRANTEE:
               szObjectTypeStr = ResourceString ((UINT)IDS_I_DBGRANTEE);
               break;
           case OTLL_GRANT:  
               szObjectTypeStr = ResourceString ((UINT)IDS_I_GRANT);
               break;
           case OT_COLUMN: 
               szObjectTypeStr = ResourceString ((UINT)IDS_I_COLUMN);
               break;
           case OT_REPLIC_CONNECTION:
               szObjectTypeStr = ResourceString ((UINT) IDS_I_REPLIC_OBJECTS_I);
               break;
           case OTLL_REPLIC_CDDS:
               szObjectTypeStr = ResourceString ((UINT)IDS_I_CDDS);
               break;
           case OT_REPLIC_REGTABLE:
               szObjectTypeStr = ResourceString ((UINT)IDS_I_REGTABLE);
               break;
           case OT_REPLIC_MAILUSER:
               szObjectTypeStr = ResourceString ((UINT)IDS_I_MAILUSER);
               break;
           default:
               szObjectTypeStr = ResourceString ((UINT)IDS_I_UNKNOWN);
               break;
       }
       x_strcpy (RefreshSettingsObjectTypeString [i].szString, szObjectTypeStr);
       RefreshSettingsObjectTypeString [i].number = i;
   }
}



void InitUnit ()
{
   unsigned int i;
   char szUnit [50];
   UINT Order [MAX_UNIT_STRING] =
   {
       IDS_I_SECONDS, 
       IDS_I_MINUTES, 
       IDS_I_HOURS,   
       IDS_I_DAYS, 
       IDS_I_WEEKS,   
       IDS_I_MONTHS,  
       IDS_I_YEARS
   };

   for (i = 0; i < MAX_UNIT_STRING; i++) // 0, 6
   {
       LoadString (hResource, (UINT)Order [i], szUnit, sizeof (szUnit));
       x_strcpy (UnitString [i].szString, szUnit);
       UnitString [i].number = i+1;
   }
}



void LoadBkRefreshDefaultSettingsFromIni (LPFREQUENCY lpFreqSettings)
{
   int      i1, i2, i3, i4;
   unsigned int i;
//   char     szObjectTypeStr [50];

   char     privateString [30];
   char     defaultStr [2];

   InitObjectType ();
   InitUnit ();

   wsprintf (defaultStr, "%d", REFRESH_WINDOW_POPUP);
   GetPrivateProfileString (
       "BACKGROUND REFRESH SETTINGS",
       "RefreshMessageOption",
        defaultStr,
        privateString,
        sizeof (privateString),
       inifilename());
   sscanf (privateString, "%d", &RefreshMessageOption);

   GetPrivateProfileString (
       "BACKGROUND REFRESH SETTINGS",
       "RefreshSyncAmongObject",
       "0",
        privateString,
        sizeof (privateString),
       inifilename());
   sscanf (privateString, "%d", &RefreshSyncAmongObject);

   for (i = 0; i < MAXREFRESHOBJTYPES; i++)
   {
	   LPUCHAR defstr;
	   if (ObjToBeRefreshed [i]==OT_MON_ALL)
		   defstr="90 1 0 0";
	   else
		   defstr="1 4 0 0";
//       LoadString (hResource, (UINT)(IDS_I_DATABASE + i), szObjectTypeStr, sizeof (szObjectTypeStr));

       GetPrivateProfileString (
           "BACKGROUND REFRESH SETTINGS",
            RefreshSettingsObjectTypeString [i].szString,
            defstr,
            privateString,
            sizeof (privateString),
           inifilename());
       //
       //  privateString = "i1 i2 i3 i4"
       //  i1 = periode
       //  i2 = unit
       //  i3 = refresh on load
       //  i4 = sync on parent
       //

       sscanf (privateString, "%d %d %d %d", &i1, &i2, &i3, &i4);

       lpFreqSettings [i].count         = i1;
       lpFreqSettings [i].unit          = i2;
       lpFreqSettings [i].bOnLoad       = i3;
       lpFreqSettings [i].bSyncOnParent = i4;
   }
}



int GetRefreshSettingPos(int iobjecttype)
{
   int i;

   for (i=0;i<MAXREFRESHOBJTYPES;i++) {
      if (ObjToBeRefreshed[i]==iobjecttype)
         return i;
   }
   myerror(ERR_SETTINGS);
   return (-1);
}

LPFREQUENCY GetSettingsFrequencyParams(int iobjecttype)
{
  int i=GetRefreshSettingPos(iobjecttype);
  if (i>=0)
    return &(FreqSettings[i]);
  return &(FreqSettings[0]); // system error has already displayed a message
}





// jfs functions added

int ReadIniString(UINT uiSection,UINT uiEntry,LPSTR lpBuffer,UINT cbMax)
{
  char achInifile[16];
  char achSectionName[64];
  char achEntryName[64];
  char cDefault=0;

  return GetPrivateProfileString( LoadStr(uiSection,achSectionName),
                                  LoadStr(uiEntry,achEntryName),
                                  &cDefault,
                                  lpBuffer,
                                  cbMax,
                                  LoadStr(IDS_INIFILENAME,achInifile));
}

BOOL  WriteIniString(UINT uiSection,UINT uiEntry, LPSTR lpBuffer)
{
  char achInifile[16];
  char achSectionName[64];
  char achEntryName[64];

  return WritePrivateProfileString( LoadStr(uiSection,achSectionName),
                                  LoadStr(uiEntry,achEntryName),
                                  lpBuffer,
                                  LoadStr(IDS_INIFILENAME,achInifile));
}

HFONT  ReadFontFromIni(UINT uiSection,UINT uiEntry)
{
  char achFont[256];
  char achFormat[128];
  LOGFONT lf;
  int     cpt;
  char   *p;

#ifdef MAINWIN
  // TEMPORARY - TO BE FINISHED - SSCANF CRASHES
  // Absolute default: "" font
  _fmemset(&lf, 0, sizeof(lf));
  lf.lfHeight = 18;
  lf.lfWeight = FW_NORMAL;
  lstrcpy(lf.lfFaceName, "new century schoolbook");
  return CreateFontIndirect(&lf);
#endif

  if (!ReadIniString(uiSection,uiEntry,achFont,sizeof(achFont)))
    {
    // Absolute default: "MS Sans Serif 8" font
    _fmemset(&lf, 0, sizeof(lf));
    lf.lfHeight = 8;
    lf.lfWeight = FW_NORMAL;
    lstrcpy(lf.lfFaceName, "MS Sans Serif");
    return CreateFontIndirect(&lf);
    }

  sscanf(achFont,LoadStr(IDS_INIFONTFORMAT,achFormat),
                           &lf.lfHeight,
                           &lf.lfWidth,
                           &lf.lfEscapement,
                           &lf.lfOrientation,
                           &lf.lfWeight,
                           &lf.lfItalic,
                           &lf.lfUnderline,
                           &lf.lfStrikeOut,
                           &lf.lfCharSet,
                           &lf.lfOutPrecision,
                           &lf.lfClipPrecision,
                           &lf.lfQuality,
                           &lf.lfPitchAndFamily,
                           lf.lfFaceName);

  // Added Emb 19/5/95 : face name containing spaces
  p = achFont;
  for (cpt=0; cpt<13; cpt++)
    p = x_strchr(p, ' ')+1;
  x_strcpy(lf.lfFaceName, p);

  return CreateFontIndirect(&lf);
}

BOOL WriteFontToIni(UINT uiSection ,UINT uiEntry,HFONT hFont)
{
  char achFont[256];
  char achFormat[128];
  LOGFONT lf;

  GetObject(hFont,sizeof(lf),(void FAR *) &lf);

  wsprintf(achFont,LoadStr(IDS_INIFONTFORMAT,achFormat),
                           lf.lfHeight,
                           lf.lfWidth,
                           lf.lfEscapement,
                           lf.lfOrientation,
                           lf.lfWeight,
                           lf.lfItalic,
                           lf.lfUnderline,
                           lf.lfStrikeOut,
                           lf.lfCharSet,
                           lf.lfOutPrecision,
                           lf.lfClipPrecision,
                           lf.lfQuality,
                           lf.lfPitchAndFamily,
                           lf.lfFaceName);

  return WriteIniString(uiSection,uiEntry,achFont);
}


VOID LoadOtherSettingsFromIni(HWND hwnd)
{
  char          ach[256];
  char          achFormat[64];

  // Emb May 3, 95
  // load info about the Global status bar
  if (ReadIniString(IDS_INISETTINGS ,IDS_INISTATUSBAR, ach, sizeof(ach)))
    {
    sscanf(ach,LoadStr(IDS_INISTATUSBARFORMAT, achFormat),
      &StatusBarPref.bVisible,
      &StatusBarPref.Date,
      &StatusBarPref.Time,
      &StatusBarPref.ServerCount,
      &StatusBarPref.CurrentServer,
      &StatusBarPref.ObjectsCount,
      &StatusBarPref.Environment );
  }
  else
    {
    // absolute defaults if never stored
    StatusBarPref.bVisible        = TRUE;
    StatusBarPref.Date            = TRUE;
    StatusBarPref.Time            = TRUE;
    StatusBarPref.ServerCount     = TRUE;
    StatusBarPref.CurrentServer   = TRUE;
    StatusBarPref.ObjectsCount    = TRUE;
    StatusBarPref.Environment     = TRUE;
    }

  // Emb May 17, 95
  // load info about the Global tool bar
  if (ReadIniString(IDS_INISETTINGS ,IDS_INITOOLBAR, ach, sizeof(ach)))
    sscanf(ach, LoadStr(IDS_INITOOLBARFORMAT, achFormat),
      &ToolBarPref.bVisible);
  else
    // absolute defaults if never stored
    ToolBarPref.bVisible = TRUE;

  // Emb May 17, 95
  // load info about the dom documents
  if (ReadIniString(IDS_INISETTINGS ,IDS_INIDOM, ach, sizeof(ach)))
    sscanf(ach, LoadStr(IDS_INIDOMFORMAT, achFormat),
      &DomPref.b3d);
  else
    // absolute defaults if never stored
    DomPref.b3d = TRUE;

  // session settings 
  LoadSessionSettings (&GlobalSessionParams);

  // background task authorized yes/no
  // Emb June 10, 95
  LoadBkTaskEnableState();
}

//
//  Load the flag that says "bk task enabled yes/no"
//
static void NEAR LoadBkTaskEnableState()
{
  char  ach[256];
  char  achFormat[256];

  if (ReadIniString(IDS_INISETTINGS, IDS_INIBKTASK, ach, sizeof(ach)))
    sscanf(ach, LoadStr(IDS_INIBKTASKFORMAT, achFormat), &bBkRefreshOn);
  else
    // absolute default if never stored
    bBkRefreshOn = TRUE;
}

//
//  Store the flag that says "bk task enabled yes/no"
//
BOOL SaveBkTaskEnableState()
{
  char  ach[256];
  char  achFormat[256];

  wsprintf(ach, LoadStr(IDS_INIBKTASKFORMAT, achFormat), bBkRefreshOn);
  return WriteIniString(IDS_INISETTINGS, IDS_INIBKTASK, ach);
}


BOOL SettingLoadFlagSaveAsDefault()
{
	TCHAR privateString [10];
	//
	// Load the Boolean "Save as Default" check box:
	BOOL b = GetPrivateProfileString (
		"BACKGROUND REFRESH SETTINGS",
		"RefreshSaveAlsoAsDefaults",
		"1",
		privateString,
		sizeof (privateString),
		inifilename());
	return b? (BOOL)atoi (privateString): FALSE;
}

void SettingSaveFlagSaveAsDefault(BOOL bSaveAsDefault)
{
	TCHAR privateString [10];
	wsprintf (privateString, "%d", (int)bSaveAsDefault); 
	WritePrivateProfileString (
		"BACKGROUND REFRESH SETTINGS",
		"RefreshSaveAlsoAsDefaults",
		 privateString,
		inifilename());
}


BOOL SettingLoadFlagRefreshSyncAmongObject()
{
	TCHAR privateString [10];
	GetPrivateProfileString (
		"BACKGROUND REFRESH SETTINGS",
		"RefreshSyncAmongObject",
		"0",
		privateString,
		sizeof (privateString),
		inifilename());
	sscanf (privateString, "%d", &RefreshSyncAmongObject);
	return RefreshSyncAmongObject;
}

void SettingSaveFlagRefreshSyncAmongObject(BOOL bRefreshSyncAmongObject)
{
	TCHAR privateString [10];
	wsprintf (privateString, "%d", (int)bRefreshSyncAmongObject); 
	WritePrivateProfileString (
		"BACKGROUND REFRESH SETTINGS",
		"RefreshSyncAmongObject",
		privateString,
		inifilename());
}

int  SettingLoadFlagRefreshMessageOption()
{
	TCHAR privateString [10];
	TCHAR defaultStr    [10];
	BOOL b;
	wsprintf (defaultStr, "%d", REFRESH_WINDOW_POPUP);
	b = GetPrivateProfileString (
		"BACKGROUND REFRESH SETTINGS",
		"RefreshMessageOption",
		defaultStr,
		privateString,
		sizeof (privateString),
		inifilename());
	sscanf (privateString, "%d", &RefreshMessageOption);
	return b? RefreshMessageOption: REFRESH_NONE;
}

void SettingSaveFlagRefreshMessageOption(int nRefreshMessageOption)
{
	TCHAR privateString [10];
	wsprintf (privateString, "%d", nRefreshMessageOption);
	WritePrivateProfileString (
		"BACKGROUND REFRESH SETTINGS",
		"RefreshMessageOption",
		privateString,
		inifilename());
}

int GetMaxRefreshObjecttypes()
{
	switch ( OneWinExactlyGetWinType()) {
		case TYPEDOC_DOM:
			return MAXREFRESHOBJTYPES-2;
		case TYPEDOC_MONITOR:
			return 1;
	}
	return MAXREFRESHOBJTYPES;
}

int RefObjTypePosInArray(int i)
{
	if (i<0 || i>GetMaxRefreshObjecttypes()) {
       myerror(ERR_SETTINGS);
	   return 0;
	}
 	switch ( OneWinExactlyGetWinType()) {
		case TYPEDOC_DOM:
			return i;
		case TYPEDOC_MONITOR:
			return MAXREFRESHOBJTYPES-1; /* since there is only 1, the input vaule doesn't matter...*/
	}
	return i;
}

