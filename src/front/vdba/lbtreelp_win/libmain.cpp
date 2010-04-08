//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
//
// PAN/LCM source control information:
//
//  Archive name    ^[Archive:j:\vo4clip\lbtree\arch\libmain.cpp]
//  Modifier        ^[Author:rumew01(1600)]
//  Base rev        ^[BaseRev:None]
//  Latest rev      ^[Revision:1.0 ]
//  Latest rev time ^[Timestamp:Thu Aug 18 16:12:10 1994]
//  Check in time   ^[Date:Thu Aug 18 16:51:44 1994]
//  Change log      ^[Mods:// ]
// 
// Thu Aug 18 16:12:10 1994, Rev 1.0, rumew01(1600)
//  init
// 
// Fri Apr 29 16:41:16 1994, Rev 26.0, sauje01(0500)
//  Add LM_DISABLEEXPANDALL message
// 
// Fri Jan 07 17:31:06 1994, Rev 25.0, sauje01(0500)
// 
// Wed Jan 05 14:40:14 1994, Rev 24.0, sauje01(0500)
// 
// Tue Jan 04 17:09:36 1994, Rev 23.0, sauje01(0500)
// 
// Mon Jan 03 16:36:56 1994, Rev 22.0, sauje01(0500)
// 
// Sat Oct 30 13:40:56 1993, Rev 21.0, sauje01(0500)
// 
// 27-Sep-2000 (schph01)
//    bug #102758 . add cast (HBRUSH) before GetStockObject() function.
//////////////////////////////////////////////////////////////////////////////
#include "lbtree.h"

extern "C"
{
#include <windows.h>
#include "winlink.h"
}

#ifdef WIN32
    #define LBTREE_LIBAPI __declspec(dllexport)
#else
    #define LBTREE_LIBAPI
#endif  // WIN32

#define EXTRA_BYTES sizeof(LPVOID)

// Globals 
HINSTANCE     ghDllInstance           =0;
static LPSTR  pClassName              ="JEFTREE_WINDLL";
LPSTR         pszGlobalDefaultString  ="????";
//

#define  MESS(mess) {LinkedListWindow * ll=(LinkedListWindow *)GetWindowLong(hWnd,0); if (ll) return(ll->ON_##mess(wParam,lParam));}

extern "C" LRESULT CALLBACK __export WLLWndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
  switch (message)
    {
    case WM_ERASEBKGND:
      return(TRUE);
      break;

    case WM_CREATE:
      {
      LinkedListWindow * ll =new LinkedListWindow(hWnd);
      if (ll)
        {
        SetWindowLong(hWnd,0,(LPARAM) (LinkedListWindow FAR *) ll);
        return (ll->ON_CREATE (wParam,lParam));
        }
      return -1;
      }
      break;

    case WM_PAINT:
      MESS(PAINT);
      break;

    case WM_KEYDOWN:
      MESS(KEYDOWN);
      break;

    case WM_USER:
      MESS(USER);
      break;

    case LM_GETSTRING:
      MESS(GETSTRING);
      break;

    case LM_SETSTRING:
      MESS(SETSTRING);
      break;

    case WM_GETMINMAXINFO:
      MESS(GETMINMAXINFOS);
      break;

    case WM_CHAR:
      MESS(CHAR);
      break;

    case LM_SETFONT:
      MESS(SETFONT);
      break;

    case WM_COMMAND:
      MESS(COMMAND);
      break;
      
    case LM_GETFONT:
      MESS(GETFONT);
      break;

    case WM_MOUSEMOVE:
      MESS(MOUSEMOVE);
      break;

    case WM_TIMER:
      MESS(TIMER);
      break;

    case WM_RBUTTONDOWN:
      if (GetCapture()!=hWnd)
        MESS(RBUTTONDOWN);
      break;

    case WM_RBUTTONDBLCLK:
      if (GetCapture()!=hWnd)
        MESS(RBUTTONDBLCLK);
      break;

    case WM_LBUTTONDOWN:
      if (GetCapture()!=hWnd)
        {
        SetCapture(hWnd);
        SetFocus(hWnd);     // Added Emb 3/2/95 because of controls in nanbar
        MESS(LBUTTONDOWN);
        }
      break;

    case WM_LBUTTONDBLCLK:
      if (GetCapture()!=hWnd)
        MESS(LBUTTONDBLCLK);
      break;

    case WM_LBUTTONUP:
      ReleaseCapture();
      MESS(LBUTTONUP);
      break;

    case WM_SETFOCUS:
      MESS(SETFOCUS);
      break;

    case WM_KILLFOCUS:
      MESS(KILLFOCUS);
      break;

    case WM_HSCROLL:
      MESS(HSCROLL);
      break;

    case WM_VSCROLL:
      MESS(VSCROLL);
      break;

    case WM_DESTROY:
        {
        // UKS adds this line
        LinkedListWindow * ll=(LinkedListWindow *)GetWindowLong(hWnd,0); 

        MESS(DESTROY);
        // UKS adds this line, and use delete operator
        // Do not use <delete this> in the destructor !!!
        if (ll) 
            delete ll;
        }

      break;

    case WM_SIZE:
      MESS(SIZE);
      break;

    case LM_ADDRECORD:
      MESS(ADDRECORD);
      break;

    case LM_GETFIRSTCHILD:
      MESS(GETFIRSTCHILD);
      break;

    case LM_DELETEALL:
      MESS(DELETEALL);
      break;
      

    case LM_DELETERECORD:
      MESS(DELETERECORD);
      break;

    case LM_SETSEL:
      MESS(SETSEL);
      break;

    case LM_GETSEL:
      MESS(GETSEL);
      break;

    case LM_EXPAND:
      MESS(EXPAND);
      break;

      
    case LM_GETITEMDATA:
      MESS(GETITEMDATA);
      break;

    case LM_SETITEMDATA:
      MESS(SETITEMDATA);
      break;

    case LM_GETEDITHANDLE:
      MESS(GETEDITHANDLE);
      break;

    case LM_ITEMFROMPOINT:
      MESS(ITEMFROMPOINT);
      break;
      

    case LM_COLLAPSE:
      MESS(COLLAPSE);
      break;

    case LM_EXPANDBRANCH:
      MESS(EXPANDBRANCH);
      break;

    case LM_COLLAPSEBRANCH:
      MESS(COLLAPSEBRANCH);
      break;

    case LM_GETFIRST:
      MESS(GETFIRST);
      break;

    case LM_GETNEXT:
      MESS(GETNEXT);
      break;

    case LM_GETFIRSTPRESENT:
      MESS(GETFIRSTPRESENT);
      break;

    case LM_GETNEXTPRESENT:
      MESS(GETNEXTPRESENT);
      break;

    case LM_GETPARENT:
      MESS(GETPARENT);
      break;

    case LM_GETLEVEL:
      MESS(GETLEVEL);
      break;

    case LM_EXPANDALL:
      MESS(EXPANDALL);
      break;

    case LM_COLLAPSEALL:
      MESS(COLLAPSEALL);
      break;

    case LM_EXISTSTRING:
      MESS(EXISTSTRING);
      break;


    case LM_GETMAXEXTENT:
      MESS(GETMAXEXTENT);
      break;


    case LM_FINDFROMITEMDATA :
      MESS(FINDFROMITEMDATA);
      break;

    case LM_GETSTATE:
      MESS(GETSTATE);
      break;

    // Emb 13/2/95
    case LM_GETVISSTATE:
      MESS(GETVISSTATE);
      break;

    case LM_GETTOPITEM:
      MESS(GETTOPITEM);
      break;
      
    case LM_SETTOPITEM:
      MESS(SETTOPITEM);
      break;


    case LM_FINDFIRST:
      MESS(FINDFIRST);
      break;

    case LM_FINDNEXT:
      MESS(FINDNEXT);
      break;

    case LM_SETBKCOLOR:
      MESS(SETBKCOLOR);
      break;

    case LM_GETSEARCHSTRING:
      MESS(GETSEARCHSTRING);
      break;


    case LM_GETITEMRECT:
      MESS(GETITEMRECT);
      break;


    case LM_SETTABSTOP:
      MESS(SETTABSTOP);
      break;

    case LM_GETSTYLE:
      MESS(GETSTYLE);
      break;


    case LM_SETSTYLE:
      MESS(SETSTYLE);
      break;

    case LM_GETPREV:
      MESS(GETPREV);
      break;

    case LM_SETPRINTFLAGS:
      MESS(SETPRINTFLAGS);
      break;

    case LM_GETCOUNT:
      MESS(GETCOUNT);
      break;

    case LM_SETICONSPACING:
      MESS(SETICONSPACING);
      break;

    case LM_ADDRECORDBEFORE:
      MESS(ADDRECORDBEFORE);
      break;

    case LM_DISABLEEXPANDALL:
      MESS(DISABLEEXPANDALL);
      break;
      
    case LM_GETEDITCHANGES:
      MESS(GETEDITCHANGES);
      break;

    // Emb 18/4/95 for Drag/Drop :
    // returns the item behind the mouse, or 0L if none
    case LM_GETMOUSEITEM:
      MESS(GETMOUSEITEM);
      break;

    // Emb 19/4/95 for Drag/Drop :
    // cancels the dragdrop mode in progress
    case LM_CANCELDRAGDROP:
      MESS(CANCELDRAGDROP);
      break;

    // Debug Emb 26/6/95 : performance measurement data
    case LM_RESET_DEBUG_EMBCOUNTS:
      MESS(RESET_DEBUG_EMBCOUNTS);
      break;

    case WM_USER+500:
      {
      LinkedListWindow * ll=(LinkedListWindow *)GetWindowLong(hWnd,0);
      if (ll)
        ll->bPaintSelectOnFocus=TRUE;
      }
      break;
    }
  return (DefWindowProc(hWnd, message, wParam, lParam));
}

#ifdef WIN32
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved);
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            if (hInstance && !ghDllInstance)
            {
                WNDCLASS  wc;
                ghDllInstance=hInstance;
                if (!GetClassInfo(hInstance,pClassName, &wc))
                {
                    wc.style          = CS_DBLCLKS;
                    wc.lpfnWndProc    = WLLWndProc;       
                    wc.cbClsExtra     = 0;
                    wc.cbWndExtra     = EXTRA_BYTES;
                    wc.hInstance      = hInstance;           
                    wc.hIcon          = 0;
                    wc.hCursor        = LoadCursor(0, IDC_ARROW);
                    wc.hbrBackground  = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
                    wc.lpszMenuName   = 0;   
                    wc.lpszClassName  = pClassName;
                    if (!RegisterClass(&wc))
                        return (FALSE);
                }
            }
            break;
        }

        case DLL_PROCESS_DETACH:
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;
    }

    return TRUE;
}
#else
extern "C" int CALLBACK LibMain(HINSTANCE hInst,WORD wds,WORD cHs,LPSTR lpCmdLine)
{
  if (hInst && !ghDllInstance)
    {
    WNDCLASS  wc;
    ghDllInstance=hInst;
    if (!GetClassInfo(hInst,pClassName, &wc))
      {
      wc.style          = CS_DBLCLKS;
      wc.lpfnWndProc    = WLLWndProc;       
      wc.cbClsExtra     = 0;
      wc.cbWndExtra     = EXTRA_BYTES;
      wc.hInstance      = hInst;           
      wc.hIcon          = 0;
      wc.hCursor        = LoadCursor(0, IDC_ARROW);
      wc.hbrBackground  = GetStockObject(LTGRAY_BRUSH);
      wc.lpszMenuName   = 0;   
      wc.lpszClassName  = pClassName;
      if (!RegisterClass(&wc))
        return (FALSE);
      }
    }
  return(hInst ? TRUE :FALSE);
}
#endif

extern "C" int CALLBACK WEP(int nExitType)
{
  return(1);
}


extern "C" LBTREE_LIBAPI HWND CALLBACK __export LRCreateWindowPrint (HWND hOwner,LPRECT rcLocation,DWORD dwStyle,BOOL bVODrawStyle)
{
  HWND hWnd;
  hWnd=CreateWindow(pClassName,
                    0,
                    WS_CHILD|WS_VSCROLL|WS_HSCROLL|WS_CLIPSIBLINGS|WS_CLIPCHILDREN,
                    rcLocation->left,
                    rcLocation->top,
                    rcLocation->right-rcLocation->left,
                    rcLocation->bottom-rcLocation->top,
                    hOwner,
                    0,
                    ghDllInstance,
                    (void FAR*) (DWORD) bVODrawStyle);
  return hWnd;
}

#if 0
extern "C" LBTREE_LIBAPI HWND CALLBACK __export LRCreateWindow (HWND hOwner,LPRECT rcLocation,DWORD dwStyle,BOOL bVODrawStyle)
{
  HWND hWnd;
  hWnd=CreateWindow(pClassName,
                    0,
                    WS_CHILD|WS_VSCROLL|WS_HSCROLL|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|dwStyle,
                    rcLocation->left,
                    rcLocation->top,
                    rcLocation->right-rcLocation->left,
                    rcLocation->bottom-rcLocation->top,
                    hOwner,
                    0,
                    ghDllInstance,
                    (void FAR*) (DWORD) bVODrawStyle);
  return hWnd;
}
#endif