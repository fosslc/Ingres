/**************************************************************************
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**  Source   : esltools.c
**
**  Project : Ingres Visual DBA
**  Author  : Francois Noirot-Nerin
**
**  environment-specific layer tools
**  History after 01-Dec-2000
**  20-Dec-2000 (noifr01)
**   (SIR 103548) use new function for determining whether Ingres Desktop
**   is running
**  26-Mar-2001 (noifr01)
**   (sir 104270) removal of code for managing Ingres/Desktop
**  10-Dec-2001 (noifr01)
**   (sir 99596) removal of obsolete code and resources
**  27-Mar-2003 (noifr01)
**   (sir 107523) management of sequences
****************************************************************************/

#include "dba.h"
#include "error.h"
#include "winutils.h"   // CenterDialog
#include "resource.h"   // For the Background task refresh dialog box
#include "nanact.e"   // nanbar and status bar dll
#include "msghandl.h"
#include "dll.h"
#include "dbaginfo.h"
#include "extccall.h"
#include "prodname.h"

int TS_MessageBox( HWND hWnd, LPCTSTR pTxt, LPCTSTR pTitle, UINT uType)
{
	if (CanBeInSecondaryThead()) {
		MESSAGEBOXPARAMS params;
		params.hWnd   = hWnd;
		params.pTxt   = pTxt;
		params.pTitle = pTitle;
		params.uType  = uType;
		return (int) Notify_MT_Action(ACTION_MESSAGEBOX, (LPARAM) &params);// for being executed in the main thread
	}
	else
		return MessageBox(hWnd,pTxt,pTitle,uType);
}
HWND  TS_GetFocus()
{
	if (CanBeInSecondaryThead()) {
		return (HWND ) Notify_MT_Action(ACTION_GETFOCUS, (LPARAM) 0);// for being executed in the main thread
	}
	else
		return GetFocus();
}

LRESULT TS_SendMessage( HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam )
{
	if (CanBeInSecondaryThead()) {
		SENDMESSAGEPARAMS params;
		params.hWnd = hWnd;
		params.Msg  = Msg;
		params.wParam = wParam;
		params.lParam = lParam;
		return (LRESULT) Notify_MT_Action(ACTION_SENDMESSAGE, (LPARAM) &params);// for being executed in the main thread
	}
	else 
		return SendMessage(hWnd, Msg, wParam, lParam);
}
 


#ifdef DEBUGMALLOC
#define MAXALLOCS 64000
//static void * AddressTable [MAXALLOCS];
static void ** AddressTable ;
int ESL_AllocCount ()
{
   int i;
   int nb_error = 0;

   if (!AddressTable) {
     myerror(ERR_FREE);
     return 0;
   }

   for (i=0; i<MAXALLOCS; i++)
   {
       if (AddressTable[i])
          nb_error ++;
   }

   free(AddressTable);
   AddressTable = NULL;

   return (nb_error);
}
#endif 

LPVOID ESL_AllocMem(uisize) /* specified to fill allocated area with zeroes */
UINT uisize;
{
  LPVOID lpv;

  // DEBUG 
#ifdef DEBUGMALLOC
  static int  cptCrashMalloc = 0;
  static BOOL bUseCrash;
  static int  cycle = 10;

  cptCrashMalloc++;
  cptCrashMalloc = cptCrashMalloc % cycle;
  if (!cptCrashMalloc)
    if (bUseCrash) {
      myerror(ERR_ALLOCMEM);
      return 0;
    }
#endif  // DEBUGMALLOC

   //LPVOID lpv = malloc(uisize);
   lpv = malloc(uisize);
   if (lpv)
   {
#ifdef DEBUGMALLOC      
      int i;
      if (!AddressTable) {
         AddressTable=malloc(MAXALLOCS * sizeof (void *));
         if (!AddressTable)
            TS_MessageBox( TS_GetFocus(), "error in allocating for DEBUGMALLOC",
                        NULL, MB_OK | MB_TASKMODAL);
         memset(AddressTable,'\0',MAXALLOCS * sizeof (void *));
      }
      for (i= 0; i <MAXALLOCS; i++)
      {
         if (!AddressTable[i])
         {
            AddressTable [i] = lpv;
            break;
         }
      }
      if (i>=MAXALLOCS)
         TS_MessageBox( TS_GetFocus(), "no more entry in DEBUGMALLOC", NULL,
                     MB_OK | MB_TASKMODAL);
      if (!uisize)
         TS_MessageBox( TS_GetFocus(), "try to allocate null size", NULL,
                     MB_OK | MB_TASKMODAL);
#endif
      memset(lpv,'\0',uisize);
   }
   else
      myerror(ERR_ALLOCMEM);
   return lpv;

}
void   ESL_FreeMem (lpArea)
LPVOID lpArea;
{
#ifdef DEBUGMALLOC
   int i;
   for (i= 0; i< MAXALLOCS; i++)
   {
       if (AddressTable[i] == lpArea) {
          AddressTable[i] = NULL;
          break;
       }
   }
   if (!lpArea)
      TS_MessageBox(TS_GetFocus(),"tries to free null pointed area",NULL,
                 MB_OK | MB_TASKMODAL);

   if (i>=MAXALLOCS)
      TS_MessageBox( TS_GetFocus(), "tries to free unallocated memory area",
                  NULL, MB_OK | MB_TASKMODAL);
#endif 
   free(lpArea);
}

LPVOID ESL_ReAllocMem(lpArea, uinewsize, uioldsize)
LPVOID lpArea;
UINT uinewsize;
UINT uioldsize;
{
   LPVOID lpv = realloc(lpArea, uinewsize);
#ifdef DEBUGMALLOC     
   int i;
   for (i=0; i<MAXALLOCS; i++) 
      if (AddressTable[i] == lpArea )
          break;
   if (!lpArea)
      TS_MessageBox(TS_GetFocus(),"tries to Reallocate null pointer",NULL,MB_OK | MB_TASKMODAL);
   if (i>=MAXALLOCS)
      TS_MessageBox(TS_GetFocus(),"tries to reallocate unalloc.ptr", NULL,MB_OK | MB_TASKMODAL);
   else 
      AddressTable [i] = lpv;
#endif
   if ( lpv && uioldsize>0 && (uinewsize>uioldsize) )
         memset((LPUCHAR)lpv + uioldsize, '\0', (uinewsize-uioldsize));
   return lpv;
}

time_t ESL_time()
{
   return time(NULL);
}

// "out of memory" error management flag
int  gMemErrFlag = MEMERR_STANDARD;   // initialized at standard value

int myerror(ierr)
int ierr;
{
   char buf[80];
   // system error %d

   // special management for memory error
   if (ierr == ERR_ALLOCMEM) {
      switch (gMemErrFlag) {
        case MEMERR_STANDARD:
          LoadString(hResource, IDS_E_MEMERR_STANDARD, buf, sizeof(buf));
          TS_MessageBox(TS_GetFocus(), buf, NULL, MB_OK | MB_ICONHAND | MB_TASKMODAL);
          break;
        case MEMERR_NOMESSAGE:
          gMemErrFlag = MEMERR_HAPPENED;
          break;
        case MEMERR_HAPPENED:
          break;
      }
   }
   else {
     wsprintf(buf,ResourceString ((UINT)IDS_E_SYSTEM_ERROR), ierr);
     TS_MessageBox( TS_GetFocus(), buf, NULL, MB_OK | MB_TASKMODAL);
   }
   return RES_ERR;
}

static BOOL bDispErrRefresh=TRUE;

int SetDispErr(BOOL b)
{
   bDispErrRefresh=b;
   return TRUE;
}

BOOL AskIfKeepOldState(iobjecttype,errtype,parentstrings)
int iobjecttype;
int errtype;
LPUCHAR * parentstrings;
{
   if (bDispErrRefresh) {
      TS_MessageBox( GetFocus(), ResourceString(IDS_ERR_SOME_INFO_NOT_REFRESHED),
                              NULL, MB_OK | MB_TASKMODAL);
   }
   return TRUE;   
}

static HWND lastFocus;
static HWND hwndRefreshOwner;

BOOL WINAPI RefreshBoxProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg) {
    case WM_INITDIALOG:
      //SetFocus(GetDlgItem(hDlg, IDD_TEXT));
      return FALSE;

    default:
      return FALSE;
  }
  return TRUE;
}

static HWND CachedHDlgRefresh = NULL;

//
// These two functions below are defined in MAINMFC: dgbkrefr.cpp:
extern HWND DlgBkRefresh_Create(HWND hwndParent);
extern void DlgBkRefresh_SetInfo (HWND hWndDlg, LPCTSTR lpszText);

//
// Keep old state when Open refresh box, and restore old state
// when close refresh box.
static BOOL bEnableAnimation_WithThread = FALSE;
extern void VDBA_EnableAnimate(BOOL bEnable, BOOL bLocked);
extern void VDBA_ReleaseAnimateMutex();
extern BOOL VDBA_IsAnimateEnabled();
DISPWND ESL_OpenRefreshBox()
{
	myerror(ERR_REFRESH_WIN);
	return NULL;

  bEnableAnimation_WithThread = VDBA_IsAnimateEnabled();
  VDBA_EnableAnimate(FALSE, TRUE);

  if (IsTheActiveApp())
    lastFocus = GetFocus();
  else
    lastFocus = GetStoredFocusWindow();

  // set the owner of the box
  hwndRefreshOwner =  hwndFrame;

  // Disable owner
  if (IsTheActiveApp())
    EnableWindow(hwndRefreshOwner, FALSE);

  // Create the box or use created one
  if (!CachedHDlgRefresh) {
    hDlgRefresh = DlgBkRefresh_Create(hwndRefreshOwner);
    CachedHDlgRefresh = hDlgRefresh;
  }
  else
    hDlgRefresh = CachedHDlgRefresh;

  if (hDlgRefresh) {
    CenterDialog(hDlgRefresh);
    if (IsTheActiveApp())
      ShowWindow(hDlgRefresh, SW_SHOW);
    else
      ShowWindow(hDlgRefresh, SW_SHOWNOACTIVATE);
    UpdateWindow(hDlgRefresh);
  }

  return (DISPWND)hDlgRefresh;
}

BOOL ESL_SetRefreshBoxTitle(dispwnd,lpstring)
DISPWND dispwnd;
LPUCHAR lpstring;
{
  HWND  hDlg;

	myerror(ERR_REFRESH_WIN);
	return FALSE;

	hDlg = dispwnd;
  SetWindowText(hDlg, lpstring);
  return TRUE;
}

BOOL ESL_SetRefreshBoxText (dispwnd,lpstring)
DISPWND dispwnd;
LPUCHAR lpstring;
{
  HWND  hDlg;

	myerror(ERR_REFRESH_WIN);
	return FALSE;

  hDlg = dispwnd;
  //SetDlgItemText(hDlg, IDD_TEXT, lpstring);
  DlgBkRefresh_SetInfo (hDlg, (LPCTSTR)lpstring);
  return TRUE;
}

BOOL ESL_CloseRefreshBox  (DISPWND dispwnd)
{
  HWND  hDlg;
	myerror(ERR_REFRESH_WIN);
	return FALSE;

  hDlg = dispwnd;
  VDBA_ReleaseAnimateMutex();
  VDBA_EnableAnimate(bEnableAnimation_WithThread, FALSE);

  // ASSERTIONS
  if (hDlg != CachedHDlgRefresh)
    return FALSE;
  if (hDlg != hDlgRefresh)
    return FALSE;

  // Enable owner of the box
  // Emb 16/06/97 : only if app is the active app,
  // and before hiding dialog box
  if (IsTheActiveApp())
    EnableWindow (hwndRefreshOwner, TRUE);
  hwndRefreshOwner = 0;

  // Fnn suggestion 16/06/97: don't destroy, but hide instead
  ShowWindow(hDlg, SW_HIDE);

  // Emb 16/06/97: disable dialog entry
  EnableWindow(hDlg, FALSE);

  if (lastFocus) {
    // Emb 16/06/97 : only if app is the active app
    if (IsTheActiveApp())
      SetFocus(lastFocus);
  }
  lastFocus = 0;

  // clear the dialog handle for main message loop
  hDlgRefresh = NULL;

  return TRUE;
}

static BOOL bRefreshGlobalStatus;
static BOOL bRefreshStatusBarOpened;

BOOL ESL_IsRefreshStatusBarOpened ()
{
  return bRefreshStatusBarOpened;
}

DISPWND ESL_OpenRefreshStatusBar()
{
  if (StatusBarPref.bVisible) {
    bRefreshStatusBarOpened = TRUE;
    bRefreshGlobalStatus = bGlobalStatus;
    bGlobalStatus = FALSE;
    BringWindowToTop(hwndStatusBar);
    Status_SetMsg(hwndStatusBar, "");
  }
  return hwndStatusBar;
}

BOOL ESL_SetRefreshStatusBarText(DISPWND dispwnd, LPUCHAR lpstring)
{
  if (StatusBarPref.bVisible)
     Status_SetMsg(dispwnd, lpstring);
  return TRUE;
}

BOOL ESL_CloseRefreshStatusBar (DISPWND dispwnd)
{
  bRefreshStatusBarOpened = FALSE;
  if (StatusBarPref.bVisible) {
    bGlobalStatus = bRefreshGlobalStatus;
    if (bGlobalStatus)
      BringWindowToTop(hwndGlobalStatus);
    else
      BringWindowToTop(hwndStatusBar);
    Status_SetMsg(dispwnd, "");
  }
  return TRUE;
}

// The following function applies to the object type names for refresh
// Some are grouped together (for example: replication objects).
LPUCHAR ObjectTypeString(int iobjecttype,BOOL bMultiple)
{

   static char buf[100];
   char * pchar;
   if (bMultiple) {
      switch (iobjecttype) { // TO BE FINISHED integrate strings into
                             // resources through generic function
                             // (for better portablility)
         case OT_VIRTNODE:
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_NODES);             break;
         case OT_DATABASE:
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_DATABASES);         break;
         case OT_USER:              
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_USERS  );             break;
         case OT_PROFILE:              
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_PROFILES );          break;
         case OT_GROUP:
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_GROUPS  );     break;
         case OT_GROUPUSER:
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_GROUPUSERS );    break;
         case OT_ROLE:
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_ROLES );        break;
         case OT_LOCATION:
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_LOCATIONS );  break;
         case OT_TABLE:
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_TABLES );  break;
         case OT_TABLELOCATION   :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_TABLE_LOCATIONS );break;
         case OT_VIEW            :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_VIEWS ); break;
         case OT_VIEWTABLE       :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_VIEWCOMPONENTS ); break;
         case OT_INDEX           :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_INDEXES ); break;
         case OT_INTEGRITY       :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_INTEGRITIES );break;
         case OT_PROCEDURE       :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_PROCEDURES );  break;
         case OT_RULE            :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_RULES );  break;
         case OT_SCHEMAUSER      :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_SCHEMAS );break;
         case OT_SYNONYM         :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_SYNONYMS );break;
         case OT_DBEVENT         :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_DBEVENTS); break;
         case OT_SEQUENCE        :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_SEQUENCES); break;
         case OTLL_SECURITYALARM :  
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_SECALARMS );break;
         case OT_ROLEGRANT_USER  :  
         case OTLL_GRANTEE       :  
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_GRANTEES );break;
         case OTLL_DBGRANTEE     :  
         case OTLL_OIDTDBGRANTEE :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_DBGRANTEES ); break;
         case OTLL_DBSECALARM    :  
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_DBSECALARMS ); break;
         case OTLL_GRANT         :  
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_GRANTS ); break;
         case OT_VIEWCOLUMN      : 
         case OT_COLUMN          : 
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_COLUMNS ); break;
         case OT_REPLIC_CONNECTION  :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_REP_OBJECTS ); break;
         case OTLL_REPLIC_CDDS      :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_REP_CDDS ); break;
         case OT_REPLIC_REGTABLE    :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_REP_REGTBLS );break;
         case OT_REPLIC_MAILUSER    :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_ERRLIST_USRNMS ); break;
         case OT_MON_ALL      :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_MON_DATA );  break;
         case OT_NODE       :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_NODE_DEF_DATA );  break;
         case OT_ICE_ROLE       :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_ICE_ROLES );  break;
         case OT_ICE_DBUSER       :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_ICE_DBUSERS );  break;
         case OT_ICE_DBCONNECTION       :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_ICE_DBCONNECTS );  break;
         case OT_ICE_WEBUSER       :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_ICE_WEBUSRS );  break;
         case OT_ICE_WEBUSER_ROLE       :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_ICE_WEBUSRROLES );  break;
         case OT_ICE_WEBUSER_CONNECTION       :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_ICE_WEBUSERDBCONNS );  break;
         case OT_ICE_PROFILE       :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_ICE_PROFILES );  break;
         case OT_ICE_PROFILE_ROLE       :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_ICE_PROF_ROLES );  break;
         case OT_ICE_PROFILE_CONNECTION       :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_ICE_PROF_CONNS );  break;
         case OT_ICE_BUNIT       :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_ICE_BUS );  break;
         case OT_ICE_BUNIT_SEC_ROLE       :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_ICE_BU_ROLES );  break;
         case OT_ICE_BUNIT_SEC_USER       :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_ICE_BU_USERS );  break;
         case OT_ICE_BUNIT_FACET       :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_ICE_BU_FACETS);  break;
         case OT_ICE_BUNIT_PAGE       :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_ICE_BU_PAGES);  break;
         case OT_ICE_BUNIT_LOCATION       :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_ICE_BU_LOCATIONS);  break;
         case OT_ICE_SERVER_APPLICATION       :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_ICE_SVR_B_GROUPS );  break;
         case OT_ICE_SERVER_LOCATION       :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_ICE_LOCATIONS );  break;
         case OT_ICE_SERVER_VARIABLE       :
                            pchar=ResourceString ( (UINT) IDS_OBJTYPES_ICE_VARIABLES );  break;

         case OT_ICE_GENERIC:
            pchar = ResourceString ( (UINT) IDS_OBJTYPES_ICE_OBJECT_DEFS );
            break;

         default:
          myerror(ERR_OBJECTNOEXISTS);
          buf[0] = '\0';
          return buf;
      }
      fstrncpy(buf,pchar,sizeof(buf));
   }
   else  {
      if (iobjecttype==OTLL_SECURITYALARM || iobjecttype==OTLL_DBSECALARM)
           x_strcpy (buf, "Security alarm");
      else
           LoadString(hResource, GetObjectTypeStringId(iobjecttype), buf, sizeof(buf));
   }
      return buf;
}

// For windows, we start/stop a timer
//
// timer id defined in main.h not to be confused with status refresh timer
//
//#define TIMEOUT 4000    // in milliseconds
#define TIMEOUT 3333    // in association with the new "anticipation" by 1 second 
						// (see DOMIsTimeElapsed(...) in dbatime.c)
						// this provides a compromise allowing an acceptable average, min and
						// max error in the frequency of the refresh, compared to the settings
						// old avg: +2  secs (+ refresh time) old min: 0  (+ r.t.) old max: 4 (+ r.t.)
						// new avg: .66 secs (+ refresh time) new min: -1 (+ r.t.) new max: 2.33(+ r.t.)
						// see also comments in dbatime.c (DOMIsTimeElapsed(...))
static BOOL bBkTaskInProgress;

void ActivateBkTask()
{
  char  rsBuf[BUFSIZE];

  // In case we forgot to stop
  KillTimer(hwndFrame, IDT_BKTASK);

  if (!SetTimer(hwndFrame, IDT_BKTASK, TIMEOUT, 0)) {
    LoadString(hResource, IDS_ERR_TIMER_BKTASK, rsBuf, sizeof(rsBuf));
    TS_MessageBox(hwndFrame, rsBuf, NULL, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
  }
  bBkTaskInProgress = FALSE;
}

void StopBkTask()
{
  KillTimer(hwndFrame, IDT_BKTASK);
  bBkTaskInProgress = TRUE;
}

BOOL IsBkTaskInProgress()
{
  return bBkTaskInProgress;
}

//
// Launch the dialog box used when a remote command is executing
//
extern int DlgExecuteRemoteCommand (LPRMCMDPARAMS lpParam);
int ESL_RemoteCmdBox(LPRMCMDPARAMS lpRmCmdParams)
{
    return DlgExecuteRemoteCommand(lpRmCmdParams);
}

static char buffilename[16];

LPUCHAR inifilename()
{
   LoadString(hResource, IDS_INIFILENAME, buffilename,sizeof(buffilename));
   return buffilename;
}

int ESL_GetTempFileName(LPUCHAR buffer, int buflen)
{
   UCHAR szFileId[_MAX_PATH];
#ifdef WIN32
   UCHAR szTempPath[_MAX_PATH];
   if (GetTempPath(sizeof(szTempPath), szTempPath) == 0)
     return RES_ERR;
   if (GetTempFileName(szTempPath, "dba", 0, szFileId) == 0)
     return RES_ERR;
#else
   GetTempFileName(0, "dba", 0, szFileId);
#endif
   if ( (int) x_strlen(szFileId) > buflen-1 )
     return RES_ERR;
   fstrncpy(buffer,szFileId,buflen);
   return RES_SUCCESS;
}
int ESL_FillNewFileFromBuffer(LPUCHAR filename,LPUCHAR buffer, int buflen, BOOL bAdd0D0A)
{
   HFILE hf;
   UINT uiresult;
   char bx[] = {0x0D, 0x0A, '\0'};
   int result = RES_SUCCESS;

   hf = _lcreat (filename, 0);
   if (hf == HFILE_ERROR)
      return RES_ERR;

   uiresult =_lwrite(hf, buffer, buflen);
   if ((int)uiresult!=buflen)
      result = RES_ERR;

   if (bAdd0D0A && result == RES_SUCCESS) {
     uiresult=_lwrite(hf, bx, 2);
     if ((int)uiresult!=2)
        result = RES_ERR;
   }
   
   hf =_lclose(hf);
   if (hf == HFILE_ERROR)
        result = RES_ERR;

   return result;
}

int ESL_FillBufferFromFile(LPUCHAR filename,LPUCHAR buffer,int buflen, int * pretlen, BOOL bAppend0)
{
   HFILE hf;
   UINT uiresult;
   int result = RES_SUCCESS;

   if (bAppend0) {
      buflen--;
      if (buflen<0)
         return RES_ERR;
   }

   hf = _lopen (filename, OF_READ);
   if (hf == HFILE_ERROR)
      return RES_ERR;

   uiresult =_lread(hf, buffer, buflen);
   if (uiresult==HFILE_ERROR)
      result = RES_ERR;

   *pretlen= (int) uiresult;

   if (bAppend0 && result == RES_SUCCESS)
      buffer[uiresult]='\0';
   
   hf =_lclose(hf);
   if (hf == HFILE_ERROR)
        result = RES_ERR;

   return result;
}

int ESL_RemoveFile(LPUCHAR filename)
{
   OFSTRUCT ofFileStruct;
   HFILE hf;
   hf = OpenFile((LPCSTR)filename, &ofFileStruct, OF_DELETE);
   if (hf==HFILE_ERROR)
      return RES_ERR;
   return RES_SUCCESS;
}
