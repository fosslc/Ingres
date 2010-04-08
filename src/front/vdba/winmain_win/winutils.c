/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*    Project : CA/OpenIngres Visual DBA
**
**    Source : winutils.c
**    Set of utility functions specific for windows
**
**    Author : Emmanuel Blattes
**
**    Special thanks to Jean Fran‡ois Sauget who wrote
**    most of the functions in c++ for CA-Visual Objects
**
**  History:
**   10-Dec-2001 (noifr01)
**    (sir 99596) removal of obsolete code and resources
**    07-Jun-2002 (schph01)
**    SIR 107951 Format the date and time with the preferences that are set
**    at the OS level.
**    22-Sep-2002 (schph01)
**    bug 108645 in BitmapCacheFree() and IconCacheFree() functions can be
**    called before that the pointers (bitmapCache,iconCache) are allocated
**    17-Mar-2009 (drivi01)
**    In efforts to port to Visual Studio 2008, clean up some warnings.
********************************************************************/

#include "dba.h"
#include "winutils.h"
#include "catocntr.h"
#include "main.h"
#include "treelb.e"   // tree list dll (LM_SETFONT)
#include "tchar.h"
// from main.c
extern HWND GetVdbaDocumentHandle(HWND hwndDoc);

#define TREE_CLASS "WLL_CN_WIN"
//
// How to make a unique property string...
//
static void WinToPropString(HWND hDlg,LPSTR p)
{
  wsprintf(p,"_PR%04X_",hDlg);
}

//
// Simplified LoadString. No checks here so please verify your parameters
//
LPSTR LoadStr(UINT ui,LPSTR lp)
{
  LoadString(hResource,ui,lp,256);
  return lp;
}


//
//  Speaks for itself
//
void CenterDialog(HWND hDlg)
{
  RECT rc;

  GetWindowRect(hDlg, &rc);
  OffsetRect(&rc, -rc.left, -rc.top);

  MoveWindow(hDlg,
             ((((GetSystemMetrics(SM_CXSCREEN)-rc.right)/2)+4)&(~7)),
             (GetSystemMetrics(SM_CYSCREEN)-rc.bottom)/2,
             rc.right,
             rc.bottom,
             FALSE);
}

//
// To be called at dialog box creation time, on WM_INITDIALOG
// stores the lParam in a property
//
BOOL HDLGSetProp(HWND hDlg, LPVOID lpData)
{
  char    w[16];
  LPVOID *lp;
  HANDLE  hData;
  
  hData = GlobalAlloc(GHND, sizeof(LPVOID));
  if (!hData)
    return FALSE;

  lp = (LPVOID *) GlobalLock(hData);
  *lp = lpData;
  GlobalUnlock(hData);

  WinToPropString(hDlg, w);

  return SetProp(hDlg, w, hData);
}

//
//  Retrieves the lParam stored by HDLGSetProp
//
LPVOID HDLGGetProp(HWND hDlg)
{
  char    w[16];
  HANDLE  hData;
  LPVOID *lp;

  WinToPropString(hDlg, w);
  hData = GetProp(hDlg, w);
  if (hData) {
    lp =(LPVOID *)GlobalLock(hData);
    return (LPVOID *) *lp;
  }
  return 0;
}

// To be called if we have made a HDLGGetProp and we stay in the box
// for a while...
VOID HDLGUnlockProp(HWND hDlg)
{
  char    w[16];
  HANDLE  hData;

  WinToPropString(hDlg, w);
  hData = GetProp(hDlg, w);
  if (hData)
    GlobalUnlock(hData);
}

//
// To be called at dialog box exit time
// frees the memory and the property allocated by HDLGSetProp
//
HANDLE HDLGRemoveProp(HWND hDlg)
{
  char    w[16];
  HANDLE  hData;

  WinToPropString(hDlg, w);
  hData =GetProp(hDlg, w);
  if (hData) {
    while(GlobalUnlock(hData));   // Might have been locked several times...
    GlobalFree(hData);
  }
  return RemoveProp(hDlg,w);
}

//
// Hourglass functions
//
static int      glassCount;
static HCURSOR  hOldCursor;

void ShowHourGlass()
{
  HCURSOR hGlass;
  if (glassCount == 0) {
    hGlass = LoadCursor(NULL, IDC_WAIT);
    hOldCursor = SetCursor(hGlass);
  }
  glassCount++;
}

void RemoveHourGlass()
{
  if (glassCount>0) {
    glassCount--;
    if (glassCount==0) {
      SetCursor(hOldCursor);
      // DeleteObject(hGlass);
    }
  }
}

//
// Cache functions for icons
//

typedef struct _tagIconCache {
  HICON hIcon;
  WORD  idIcon;
} ICONCACHE, FAR *LPICONCACHE;

#define ICONCACHE_CHUNK 50

static  LPICONCACHE iconCache;
static  int         maxIcons;

BOOL IconCacheInit()
{
  iconCache = (LPICONCACHE)ESL_AllocMem(ICONCACHE_CHUNK * sizeof(ICONCACHE));
  if (iconCache) {
    maxIcons = ICONCACHE_CHUNK;
    return TRUE;
  }
  else
    return FALSE;
}


HICON IconCacheLoadIcon(WORD idIcon)
{
  int cpt;

  // pre-test in case low on memory ---> we load each time, making memory
  // lower and lower, but what else can we do?
  if (iconCache == 0)
    return LoadIcon(hResource, MAKEINTRESOURCE(idIcon));

  // search in the cache
  for (cpt=0; cpt < maxIcons; cpt++) {
    // test if cached in current line
    if (iconCache[cpt].idIcon == idIcon)
      return iconCache[cpt].hIcon;

    // test if no more icons to be scanned (free space in the cache)
    if (iconCache[cpt].idIcon == 0) {
      iconCache[cpt].hIcon = LoadIcon(hResource, MAKEINTRESOURCE(idIcon));
      iconCache[cpt].idIcon = idIcon;
      return iconCache[cpt].hIcon;
    }
  }

  // Make the cache bigger and load the icon
  iconCache = (LPICONCACHE)ESL_ReAllocMem((LPVOID)iconCache,
                            (maxIcons+ICONCACHE_CHUNK) * sizeof(ICONCACHE),
                            maxIcons * sizeof(ICONCACHE) );
  if (iconCache) {
    maxIcons += ICONCACHE_CHUNK;
    iconCache[cpt].hIcon = LoadIcon(hResource, MAKEINTRESOURCE(idIcon));
    iconCache[cpt].idIcon = idIcon;
    return iconCache[cpt].hIcon;
  }
  else {
    maxIcons = 0;   // we will try later to realloc
    return LoadIcon(hResource, MAKEINTRESOURCE(idIcon));
  }
}

VOID  IconCacheFree()
{
  int cpt;

  if (!iconCache)
    return;
  // scan the cache and free
  for (cpt=0; cpt < maxIcons; cpt++)
    if (iconCache[cpt].hIcon)
      DestroyIcon(iconCache[cpt].hIcon);
    else
      break;  // We stop on the first null since no holes can exist

  ESL_FreeMem((LPVOID)iconCache);
}

//
// Cache functions for bitmaps
//

typedef struct _tagBitmapCache {
  HBITMAP hBitmap;
  WORD    idBitmap;
} BITMAPCACHE, FAR *LPBITMAPCACHE;

#define BITMAPCACHE_CHUNK 50

static  LPBITMAPCACHE bitmapCache;
static  int           maxBitmaps;

BOOL BitmapCacheInit()
{
  bitmapCache = (LPBITMAPCACHE)ESL_AllocMem(BITMAPCACHE_CHUNK * sizeof(BITMAPCACHE));
  if (bitmapCache) {
    maxBitmaps = BITMAPCACHE_CHUNK;
    return TRUE;
  }
  else
    return FALSE;
}


HBITMAP BitmapCacheLoadBitmap(WORD idBitmap)
{
  int cpt;

  // pre-test in case low on memory ---> we load each time, making memory
  // lower and lower, but what else can we do?
  if (bitmapCache == 0)
    return LoadBitmap(hResource, MAKEINTRESOURCE(idBitmap));

  // search in the cache
  for (cpt=0; cpt < maxBitmaps; cpt++) {
    // test if cached in current line
    if (bitmapCache[cpt].idBitmap == idBitmap)
      return bitmapCache[cpt].hBitmap;

    // test if no more bitmaps to be scanned (free space in the cache)
    if (bitmapCache[cpt].idBitmap == 0) {
      bitmapCache[cpt].hBitmap = LoadBitmap(hResource, MAKEINTRESOURCE(idBitmap));
      bitmapCache[cpt].idBitmap = idBitmap;
      return bitmapCache[cpt].hBitmap;
    }
  }

  // Make the cache bigger and load the bitmap
  bitmapCache = (LPBITMAPCACHE)ESL_ReAllocMem((LPVOID)bitmapCache,
                            (maxBitmaps+BITMAPCACHE_CHUNK) * sizeof(BITMAPCACHE),
                            maxBitmaps * sizeof(BITMAPCACHE) );
  if (bitmapCache) {
    maxBitmaps += BITMAPCACHE_CHUNK;
    bitmapCache[cpt].hBitmap = LoadBitmap(hResource, MAKEINTRESOURCE(idBitmap));
    bitmapCache[cpt].idBitmap = idBitmap;
    return bitmapCache[cpt].hBitmap;
  }
  else {
    maxBitmaps = 0;   // we will try later to realloc
    return LoadBitmap(hResource, MAKEINTRESOURCE(idBitmap));
  }
}

VOID  BitmapCacheFree()
{
  int cpt;

  if (!bitmapCache)
    return;
  // scan the cache and free
  for (cpt=0; cpt < maxIcons; cpt++)
    if (bitmapCache[cpt].hBitmap)
      DeleteObject( (HGDIOBJ)(bitmapCache[cpt].hBitmap) );
    else
      break;  // We stop on the first null since no holes can exist

  ESL_FreeMem((LPVOID)bitmapCache);
}

VOID ErrorMsgBox(UINT msgId)
{
  char rsBuf[BUFSIZE];

  LoadString(hResource, msgId, rsBuf, sizeof(rsBuf)-1);
  TS_MessageBox(hwndFrame, rsBuf, NULL,
             MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
}


VOID  PostMessageToTrees(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
  HWND hChild=GetWindow(hwnd,GW_CHILD);

  while (hChild)
    {
    char achClassName[64];
    PostMessageToTrees(hChild,msg,wParam,lParam);
    GetClassName(hChild,achClassName,sizeof(achClassName));
    if (lstrcmpi(achClassName,TREE_CLASS)==0)
      {
      if (msg==WM_SETFONT)
        PostMessage(hChild, LM_SETFONT, wParam, 0L);
      else
        PostMessage(hChild, msg, wParam, lParam);
      }
    hChild=GetWindow(hChild,GW_HWNDNEXT);
    }             
}



//-------------------------------------------------------------------------
//
//  International functions
//
//-------------------------------------------------------------------------

//
// from visual objects nanact\nanpopup.c
//
//    Description:
//        copies date into the lpstrDate parameter,
//        takes care of current windows sShortDate setting
//
BOOL NanGetWindowsDate(LPSTR lpstrDate)
{
    time_t    lTime;
    struct tm *timeStruct;
    TCHAR tmpbuf[128];

    time (&lTime);
    timeStruct = localtime(&lTime);

    _tcsftime( tmpbuf, sizeof(tmpbuf), "%x", timeStruct );
    lstrcpy(lpstrDate, tmpbuf);
    return (TRUE);
}


//
// from visual objects nanact\nanpopup.c
//
//    Description:
//        copies time into the lpstrTime parameter,
//        takes care of current windows time settings
//
BOOL NanGetWindowsTime(LPSTR lpstrTime)
{
    time_t    lTime;
    struct tm *timeStruct;
    TCHAR tmpbuf[128];

    time (&lTime);
    timeStruct = localtime(&lTime);

    _tcsftime( tmpbuf, sizeof(tmpbuf), "%X", timeStruct );
    lstrcpy(lpstrTime, tmpbuf);
    return TRUE;
}

//
//  For Container : 
//  Returns the height in pixels necessary to display a combobox in a cell
//
int CalculateComboHeight(HWND hwndCntr)
{
  int   comboH    = 0;
  HWND  hwndCombo;
  RECT  rc;

  // create a combo on the fly
  hwndCombo = CreateWindowEx(0, "combobox",        // class name
                            NULL,             // no title
                            CBS_DROPDOWNLIST | WS_VSCROLL
                                             | CBS_DISABLENOSCROLL
                                             | WS_CHILD,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            hwndCntr,         // parent window
                            NULL,             // no menu
                            hInst,
                            0);               // no creation data
  if (hwndCombo) {
    ShowWindow(hwndCombo, SW_HIDE);
    //ShowWindow(hwndCombo, SW_SHOW);
    //BringWindowToTop(hwndCombo);
    MoveWindow(hwndCombo, 0, 0, 50, 2, FALSE);  // small height -> auto adjust
    GetWindowRect(hwndCombo, &rc);
    comboH = rc.bottom - rc.top;
    DestroyWindow(hwndCombo);
  }
  return (comboH - 4);
}

