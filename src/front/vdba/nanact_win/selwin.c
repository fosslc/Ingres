#include <windows.h>
#include <windowsx.h>
#include <string.h>
#include <malloc.h>

#include "port.h"

#ifdef WIN32
    #define SELWIN_LIBAPI __declspec(dllexport)
#else
    #define SELWIN_LIBAPI
#endif

static HANDLE _DLL_hInstance;
#ifdef RC_EXTERNAL
HANDLE hResource;
#endif

static BOOL OnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam);
static void OnDestroy(HWND hwnd);
static void OnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify);

typedef struct tagENUMDATA
{
    HWND hDlg;
    HWND hmdi;
} ENUMDATA, FAR * LPENUMDATA;

// Emb Oct 13, 94 : added 2 functions for dll

#ifdef WIN32
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved);
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            _DLL_hInstance = hInstance;
#ifdef RC_EXTERNAL
			if ((hResource=LoadLibrary("NANACT.LNG"))==NULL)
				hResource=hInstance;
#endif
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_THREAD_ATTACH:
            _DLL_hInstance = hInstance;
            break;

        case DLL_THREAD_DETACH:
            break;
    }

    return TRUE;
}
#else
int FAR PASCAL LibMain(HANDLE hModule, WORD wDataSeg, WORD cbHeapSize, LPSTR lpszCmdLine)
{
  _DLL_hInstance = hModule;
#ifdef RC_EXTERNAL
			if ((hResource=LoadLibrary("NANACT.LNG"))==NULL)
				hResource=hModule;
#endif
    return TRUE;
}
#endif

#ifndef WIN32
int FAR PASCAL WEP (int bSystemExit)
{
    switch (bSystemExit)
    {
        case WEP_SYSTEM_EXIT:
            break;
        case WEP_FREE_DLL:
            break;
        default:
            break;
    }
    return (1);
}
#endif

// Global property name for the dialog box
static LPSTR     gsPropName   ="SW";
static DWORD     dwLastLBUTTONDOWNTime=0L;

typedef struct _OLD_GLOBALS
{
  HWND      hWnd;
  FARPROC   lpfnOldFarProc;
  struct _OLD_GLOBALS FAR *pNext;
} OLD_GLOBALS,FAR *LPOLDGLOBALS;

LPOLDGLOBALS lpFirstOldGlobals=0;

static LPOLDGLOBALS FindGlobal(HWND hWnd)
{
  LPOLDGLOBALS lp;
  for (lp=lpFirstOldGlobals;lp;lp=lp->pNext)
    {
    if (lp->hWnd==hWnd)
      return lp;
    }
  return NULL;
}

static LPOLDGLOBALS NewGlobal(HWND hWnd)
{
  LPOLDGLOBALS lp=(LPOLDGLOBALS)GlobalAllocPtr(GHND, sizeof(OLD_GLOBALS));
  if (lp)
    {
    lp->hWnd=hWnd;
    lp->lpfnOldFarProc=NULL;
    lp->pNext=NULL;
    if (!lpFirstOldGlobals)
      {
      lpFirstOldGlobals=lp;
      }
    else
      {
      LPOLDGLOBALS lp2;
      for (lp2=lpFirstOldGlobals;lp2;lp2=lp2->pNext)
        {
        if (!lp2->pNext)
          {
          lp2->pNext=lp;
          break;
          }
        }
      }
    }
  return lp;
}

static VOID FreeGlobal(HWND hWnd)
{
  LPOLDGLOBALS lp=FindGlobal(hWnd);
  if (lp)
    {
    if (lp==lpFirstOldGlobals)
      lpFirstOldGlobals=lp->pNext;
    else
      {
      LPOLDGLOBALS lp2;
      for (lp2=lpFirstOldGlobals;lp2;lp2=lp2->pNext)
        {
        if (lp2->pNext==lp)
          {
          lp2->pNext=lp->pNext;
          break;
          }
        }
      }
    GlobalFreePtr(lp);
    }
}

static void PASCAL CenterWindowOnCursor(HWND hWnd)
{
  POINT pt;
  RECT  rcWin;
  int   cxScreen=GetSystemMetrics(SM_CXSCREEN);
  int   cyScreen=GetSystemMetrics(SM_CYSCREEN);
  int   x,y;

  GetWindowRect(hWnd,&rcWin);
  OffsetRect   (&rcWin,-rcWin.left,-rcWin.top);

  GetCursorPos(&pt);
  x=pt.x-(rcWin.right/2);
  y=pt.y-(GetSystemMetrics(SM_CYDLGFRAME)+GetSystemMetrics(SM_CYCAPTION)+HIWORD(GetDialogBaseUnits()));

  if (x<0)
    x=0;
  else if (x+rcWin.right>cxScreen)
    x=cxScreen-rcWin.right;

  if (y<0)
    y=0;
  else if (y+rcWin.bottom>cyScreen)
    y=cyScreen-rcWin.bottom;

  MoveWindow(hWnd,x,y,rcWin.right,rcWin.bottom,FALSE);
}

BOOL FAR PASCAL EXPORT EnumMDIChild(HWND hChild, LPARAM lParam)
{
  unsigned char work[120];
  short index;
  HWND  hDlg;
  LPENUMDATA lpdata = (LPENUMDATA)lParam;
  hDlg=lpdata->hDlg;
  GetWindowText(hChild,work,sizeof(work));
  if (work[0] && GetParent(hChild) == lpdata->hmdi)
    {
    HWND hwndList = GetDlgItem(hDlg, 100);
    index=(short)ListBox_AddString(hwndList, work);
    ListBox_SetItemData(hwndList, index, (LPARAM)hChild);
    }
  return(TRUE);
}

static void PASCAL FillListBox(HWND hDlg,HWND hMdi)
{
  FARPROC lpEnum;
  ENUMDATA data;
  data.hDlg = hDlg;
  data.hmdi = GetProp(hDlg,gsPropName);
  lpEnum=MakeProcInstance ((FARPROC)EnumMDIChild,GetWindowInstance(hDlg));
  EnumChildWindows(hMdi,lpEnum,(LPARAM)&data);
  FreeProcInstance (lpEnum);
}

static void PASCAL PostToAll(HWND hDlg,HWND hMdi,BOOL bIconic,WORD param)
{
  int i;
  int count= ListBox_GetCount(GetDlgItem(hDlg, 100));
  for (i=0;i<count;i++)
    {
    HWND hWnd = (HWND)ListBox_GetItemData(GetDlgItem(hDlg, 100), i);
    if (IsWindow(hWnd))
      {
      if (bIconic)
        ShowWindow(hWnd,param);
      else if (!(param==WM_MDIDESTROY && (GetWindowStyle(hWnd)&CS_NOCLOSE)))
        SendMessage(hMdi,param,(WPARAM)hWnd,0L);
      }
    }
}

static void PASCAL PostMessageToMdi(HWND hDlg,WORD param)
{
  HWND hMdi=GetProp(hDlg,gsPropName);
  HCURSOR hOldCursor=SetCursor(LoadCursor(NULL,IDC_WAIT));
  switch (param)
    {
    case IDOK:  // show current window
      {
      HWND hwndList = GetDlgItem(hDlg, 100);
      int curSel = ListBox_GetCurSel(hwndList);
      if (curSel!=LB_ERR)
        {
        HWND hWnd = (HWND)ListBox_GetItemData(hwndList, curSel);
        if (IsIconic(hWnd))
          SendMessage(hMdi,WM_MDIRESTORE,(WPARAM)hWnd,0L);
        SendMessage(hMdi,WM_MDIACTIVATE,(WPARAM)hWnd,0L);
        }
      }
      break;

    case 13: // iconize all
      PostToAll(hDlg,hMdi,TRUE,SW_SHOWMINNOACTIVE);
      break;

    case 14: // restore all
      PostToAll(hDlg,hMdi,FALSE,WM_MDIRESTORE);
      break;

    case 15: // close all
      PostToAll(hDlg,hMdi,FALSE,WM_MDIDESTROY);
      break;
    }
  SetCursor(hOldCursor);
}

BOOL EXPORT FAR PASCAL SelWinDlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
    {
        HANDLE_MSG(hDlg, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hDlg, WM_DESTROY, OnDestroy);
        HANDLE_MSG(hDlg, WM_COMMAND, OnCommand);
    }
  return(FALSE);
}

static BOOL OnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam)
{
  HWND hwndIn = (HWND)lParam;
  HWND hwndList = GetDlgItem(hDlg, 100);
  SetProp(hDlg,gsPropName,(HANDLE)hwndIn);
  CenterWindowOnCursor(hDlg);
  FillListBox(hDlg,hwndIn);
  ListBox_SetCurSel(hwndList, 0);
  FORWARD_WM_NEXTDLGCTL(hDlg, hwndList, TRUE, PostMessage);
  return FALSE;
}

static void OnDestroy(HWND hDlg)
{
    RemoveProp(hDlg,gsPropName);
}

static void OnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{
  switch (id)
    {
    case IDCANCEL:
      EndDialog (hDlg,FALSE);
      break;

    case 10: //tile
      PostMessage (GetProp(hDlg,gsPropName),WM_MDITILE,0,0L);
      EndDialog (hDlg,TRUE);
      break;

    case 16: //tile horz
      PostMessage (GetProp(hDlg,gsPropName),WM_MDITILE,1,0L);
      EndDialog (hDlg,TRUE);
      break;

    case 11://cascade
      PostMessage (GetProp(hDlg,gsPropName),WM_MDICASCADE,0,0L);
      EndDialog (hDlg,TRUE);
      break;

    case 12://arrange
      PostMessage (GetProp(hDlg,gsPropName),WM_MDIICONARRANGE,0,0L);
      EndDialog (hDlg,TRUE);
      break;

    case IDOK:
    case 13:// iconize all
    case 14:// restore all
    case 15:// close all

      // Emb Oct 13, 94 : mask next instruction for issue 1327969
      //ShowWindow(hDlg,SW_HIDE);

      PostMessageToMdi(hDlg,(WORD)id);
      EndDialog (hDlg,TRUE);
      break;

    case 100: 
      if (codeNotify==LBN_DBLCLK)
        {
        PostMessageToMdi(hDlg,IDOK);
        EndDialog (hDlg,TRUE);
        }
      break;
    }
}

static BOOL BoxSelectWindow(LPOLDGLOBALS lp)
{
  FARPROC lpfn;
  BOOL    bResult;
  lpfn = MakeProcInstance ((FARPROC)SelWinDlgProc, _DLL_hInstance);
#ifdef RC_EXTERNAL
  bResult=(BOOL)DialogBoxParam(hResource,MAKEINTRESOURCE(5000), GetParent(lp->hWnd), lpfn, (LPARAM)lp->hWnd);
#else
  bResult=(BOOL)DialogBoxParam(_DLL_hInstance,MAKEINTRESOURCE(5000), GetParent(lp->hWnd), lpfn, (LPARAM)lp->hWnd);
#endif
  FreeProcInstance (lpfn);
  return(bResult);
}

// MDI client subclassing
LRESULT CALLBACK EXPORT CallBack(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  LPOLDGLOBALS lp=FindGlobal(hWnd);

  if (!lp)
    return 0L;

  switch (message)
    {
    case WM_LBUTTONDOWN:
      {
      DWORD dwThisLBUTTONDOWNTime;
      dwThisLBUTTONDOWNTime=GetMessageTime();
      if ( (dwThisLBUTTONDOWNTime-dwLastLBUTTONDOWNTime<=(DWORD)GetDoubleClickTime()))
        {
        dwLastLBUTTONDOWNTime=0;
        BoxSelectWindow(lp);
        return 0;
        }
      else
        dwLastLBUTTONDOWNTime=dwThisLBUTTONDOWNTime;
      }
    }
  return CallWindowProc(lp->lpfnOldFarProc,hWnd,message,wParam,lParam);
}


static void SubClassMDIBegin (LPOLDGLOBALS lp)
{
 FARPROC  lpfnNewFarProc;
 HANDLE   hInst;
 hInst = GetWindowInstance(GetParent (lp->hWnd));
 lpfnNewFarProc = MakeProcInstance ((FARPROC) CallBack, hInst);
 lp->lpfnOldFarProc = (FARPROC) SetWindowLong (lp->hWnd,GWL_WNDPROC,(long) lpfnNewFarProc);
}

void SubClassMDIEnd(LPOLDGLOBALS lp)
{
  FARPROC lpfnNewFarProc;
  if (lp->lpfnOldFarProc)
    {
    lpfnNewFarProc= (FARPROC) SetWindowLong (lp->hWnd,GWL_WNDPROC,(long) lp->lpfnOldFarProc);
    FreeProcInstance(lpfnNewFarProc);
    }
}

SELWIN_LIBAPI VOID WINAPI EXPORT MdiSelWinInit(HWND hWnd)
{
  LPOLDGLOBALS lp=NewGlobal(hWnd);
  if (lp)
    SubClassMDIBegin (lp);
}

SELWIN_LIBAPI VOID WINAPI EXPORT MdiSelWinEnd(HWND hWnd)
{
  LPOLDGLOBALS lp=FindGlobal(hWnd);
  if (lp)
    {
    SubClassMDIEnd(lp);
    }
  FreeGlobal(hWnd);
}

SELWIN_LIBAPI VOID WINAPI EXPORT MdiSelWinBox (HWND hMdiClientWnd)
{
  LPOLDGLOBALS lp=FindGlobal(hMdiClientWnd);
  if (lp)
    {
    BoxSelectWindow(lp);
    }
}
