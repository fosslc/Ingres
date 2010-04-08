/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**
**  Name: nanbtn.c
**
**  Description:
**
**  History:
**	18-feb-2000 (somsa01)
**	    Generisize include of resource.h . Also, properly include
**	    windowsx.h and include windows.h .
**	29-Jun-2001 (hanje04)
**	    Removed include of catobal.h as it has been removed. 
*/

#ifdef WIN32
    #define SYS_WIN32
    #define WIN32_LEAN_AND_MEAN
#else
    #define SYS_WIN16
#endif
#define STRICT
#define NANACT_DEV
#include <windows.h>
#include <windowsx.h>
#include "aspen.h"
#include "nanerr.e"
#include "nanmem.e"

#include "global.h"
#include <assert.h>
#include <memory.h>

#include "nanact.e"

#include "port.h"

/* Modified Emb 1. Feb. 95 since we have several dummy items */
/* old code: #include "main.h" */

#include <resource.h>

#ifdef WIN32
#if	0
    #define __export
    #define _fmemset memset

    #define LONG2POINT(l,pt)\
                ((pt).x=(SHORT)LOWORD(l),(pt).y=(SHORT)HIWORD(l))

    __inline unsigned long GetTextExtent(HDC hdc, LPSTR lpstr, size_t cb);
    __inline unsigned long GetTextExtent(HDC hdc, LPSTR lpstr, size_t cb)
    {
        SIZE size;
        GetTextExtentPoint32(hdc, lpstr, cb, &size);
        return (DWORD)MAKELONG(size.cx, size.cy);
    }
#endif
#else
    #define SYS_WIN16
#endif

/* INGRES - Emb 6/1/95 */
#undef  MALLOC
#undef  FREE
#undef  MALLOCSTR
#include <malloc.h>
#define MALLOCSTR(x) (malloc(x+1))
#define MALLOC(x) (malloc(x))
#define FREE(x) (free(x))

#define NANBAR_BARBORDER        (2)
#define NANBAR_WNDOFFSET    (6)     /* 3 times NANBAR_BARBORDER  */
#define NANBAR_PENWIDTH     (1)

#define     STATUS_OVERHEAD             (7)
#define     STATUS_OFFSET               (1)
#define     STATUS_MAXLINELEN           (255)

LRESULT WINAPI EXPORT NanBarWndProc(HWND hwnd, UINT message,WPARAM wParam, LONG lParam);
LRESULT WINAPI EXPORT NanStatusLineProc(HWND hwnd, UINT message,WPARAM wParam, LONG lParam);
static LRESULT NEAR PASCAL HandleNanUserMessages(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static BOOL NanOnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
static void NanOnDestroy(HWND hwnd);
static void NanOnSize(HWND hwnd, UINT state, int cx, int cy);
static void NanOnPaint(HWND hwnd);
static void NanOnMouseMove(HWND hwnd, int x, int y, UINT keyFlags);
static void NanOnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
static void NanOnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags);


static BOOL StatusOnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
static void StatusOnPaint(HWND hwnd);
static void StatusOnDestroy(HWND hwnd);
static LRESULT HandleStatusUserMessages(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static BOOL NanOnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);

static  char *achNanBarClass   = "NantucketButtonClass";
static  char *achNanStatusClass= "NANStatusLine";

HANDLE  _DLL_hInstance; /*  accessed from anybody in this little DLL */
#ifdef RC_EXTERNAL
HANDLE	hResource;
#endif

typedef struct      _NanBtnList
{
  struct _NanBtnList  FAR *   pNext;
  NANBTNDATA              nanBtnData;
  RECT                    rectPos;
  BOOL                    fEnabled:1;
  BOOL                    fPressed:1;
  BOOL                    fNotified:1;
  BOOL                    fChanged:1;
  BOOL                    fBreak:1;
  BOOL                    fCheckMark:1;
} NANBTNLIST, FAR * LPNANBTNLIST;

#define NEXT(p)     ((p)->pNext)
#define SKIP(p)     ((p) = (p)->pNext)
#define BMP(p)      ((p)->nanBtnData.hBitMap)
#define ID(p)       ((p)->nanBtnData.wButtonID)
#define STRINGID(p) ((p)->nanBtnData.wNotifyStringID)
#define PRESSED(p)  ((p)->fPressed)
#define RECT(p)     ((p)->rectPos)
#define NOTIFIED(p) ((p)->fNotified)
#define CHANGED(p)  ((p)->fChanged)
#define BREAK(p)    ((p)->fBreak)
#define ENABLED(p)  ((p)->fEnabled)
#define CHECKMARK(p)((p)->fCheckMark)


typedef struct  _NanBarDATA
{
  HWND            hwnd;
  HWND            hwndParent;
  HANDLE          hInstance;
  LPNANBTNLIST    pNanBtnRoot;
  LPNANBTNLIST    pNanBtnLast;
  USHORT          usBtnHeight;
  USHORT          usBtnWidth;
  INT             iWidth;
  BOOL            bCapture; 
  UINT            uMsgNotify;
}   NANBARDATA, FAR * LPNANBARDATA;


#define NANBAR_SELF()           ((pNanBar)->hwnd)
#define NANBAR_PARENT()         ((pNanBar)->hwndParent)
#define NANBAR_INSTANCE()       ((pNanBar)->hInstance)
#define NANBAR_ROOT()           ((pNanBar)->pNanBtnRoot)
#define NANBAR_LAST()           ((pNanBar)->pNanBtnLast)
#define NANBAR_BTNW()           ((pNanBar)->usBtnWidth)
#define NANBAR_BTNH()           ((pNanBar)->usBtnHeight)
#define NANBAR_WIDTH()          ((pNanBar)->iWidth)
#define NANBAR_CAPTURE()        ((pNanBar)->bCapture)
#define NANBAR_MSGNOTIFY()      ((pNanBar)->uMsgNotify)

/* */
/*      status line window */
/* */

typedef struct _STATUSWNDDATA
{
  PSZ         pszMsgText;
  INT         iLen;
  HFONT       hFont;
  USHORT      usHeigth;
  HPEN        hPenBlack;
  HPEN        hPenWhite;
  HPEN        hPenGray;
  HBRUSH      hBrush;
  COLORREF    colorRef;

} STATUSWNDDATA, FAR * PSTATUSWNDDATA;

#define     STATUS_TEXT()           ((pStatusWndData)->pszMsgText)
#define     STATUS_FONT()           ((pStatusWndData)->hFont)
#define     STATUS_HEIGTH()         ((pStatusWndData)->usHeigth)
#define     STATUS_BLACKPEN()       ((pStatusWndData)->hPenBlack)
#define     STATUS_WHITEPEN()       ((pStatusWndData)->hPenWhite)
#define     STATUS_GRAYPEN()        ((pStatusWndData)->hPenGray)
#define     STATUS_BRUSH()          ((pStatusWndData)->hBrush)
#define     STATUS_BKCOLOR()        ((pStatusWndData)->colorRef)
#define     STATUS_TEXTLEN()        ((pStatusWndData)->iLen)

#ifdef DLL
#ifdef WIN32
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved);
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:

#ifdef RC_EXTERNAL
			hResource = hInstance;
#endif
            _DLL_hInstance = hInstance;
            NanStatusWndClassRegister(hInstance);
            NanBarWndClassRegister(hInstance);
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;
    }

    return TRUE;
}
#else       // WIN32
/****************************************************************************
   FUNCTION: LibMain(HANDLE, WORD, WORD, LPSTR)

   PURPOSE:  Is called by LibEntry.  LibEntry is called by Windows when
       the DLL is loaded.  The LibEntry routine is provided in
       the LIBENTRY.OBJ in the SDK Link Libraries disk.  (The
       source LIBENTRY.ASM is also provided.)  

       LibEntry initializes the DLL's heap, if a HEAPSIZE value is
       specified in the DLL's DEF file.  Then LibEntry calls
       LibMain.  The LibMain function below satisfies that call.
       
       The LibMain function should perform additional initialization
       tasks required by the DLL.  In this example, no initialization
       tasks are required.  LibMain should return a value of 1 if
       the initialization is successful.
       
*******************************************************************************/
int CALLBACK LibMain(HANDLE hModule, WORD wDataSeg, WORD cbHeapSize, LPSTR lpszCmdLine)
{

  _DLL_hInstance = hModule;

#ifdef RC_EXTERNAL
  hResource = hModule;
#endif

  NanStatusWndClassRegister(hModule);
  NanBarWndClassRegister(hModule);

  return (TRUE);
}

/****************************************************************************
  FUNCTION:  WEP(int)

  PURPOSE:  Performs cleanup tasks when the DLL is unloaded.  WEP() is
        called automatically by Windows when the DLL is unloaded (no
        remaining tasks still have the DLL loaded).  It is strongly
        recommended that a DLL have a WEP() function, even if it does
        nothing but returns success (1), as in this example.

        in our case WEP() frees the occupied memory for the error lists....
        ErrExit() does nothing

*******************************************************************************/
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
#endif      // WIN32
#endif      /*  ifdef DLL */

/********************************** 
  BOOL NanBarWndClassRegister()
  Description:
    Register Nantucket Button Class
  Access to Globals:
**********************************/
NANACT_LIBAPI BOOL FAR PASCAL NanBarWndClassRegister(HANDLE hInstance)
{
  WNDCLASS wc;

  wc.style = CS_DBLCLKS;   

  wc.lpfnWndProc      = NanBarWndProc ;
  wc.cbClsExtra       = 0 ;
  wc.cbWndExtra       = sizeof(LPNANBARDATA);
  wc.hInstance        = hInstance ;
  wc.hIcon            = NOTHING;
  wc.hCursor          = LoadCursor (NOTHING, IDC_ARROW) ;
  wc.hbrBackground    = (HBRUSH) 0; /* COLOR_WINDOW+1; */
  wc.lpszMenuName     = NULL;
  wc.lpszClassName    = achNanBarClass ;

  #ifdef LIB
  _DLL_hInstance = hInstance;
  #endif

  return (RegisterClass(&wc));

}        /* End of BOOL NanBarWndClassRegister() */

/********************************** 
  BOOL NanStatusWndClassRegister()
  Description:
    Register Nantucket Button Class
  Access to Globals:
**********************************/
NANACT_LIBAPI BOOL FAR PASCAL NanStatusWndClassRegister(HANDLE hInstance)
{

  WNDCLASS wc;

  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = NanStatusLineProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = sizeof(PSTATUSWNDDATA);
  wc.hInstance = hInstance;           /* Application that owns the class.   */
  wc.hIcon = 0;
  wc.hCursor = LoadCursor(NOTHING, IDC_ARROW);
  wc.hbrBackground = NULL;
  wc.lpszMenuName = NULL;             /* Name of menu resource in .RC file. */
  wc.lpszClassName = achNanStatusClass;   /* Name used in call to CreateWindow. */

  #ifdef LIB
  _DLL_hInstance = hInstance;
  #endif

  return (RegisterClass(&wc));
}

/********************************** 
  HWND CreateNanStatusWnd()
  Description:
    Creates a Nantucket Button Window 
  Access to Globals:
    
**********************************/ 
NANACT_LIBAPI HWND FAR PASCAL CreateNanStatusWnd(HWND hwndParent, HANDLE hInstance)
{
  HWND            hwndChild;

  hwndChild = CreateWindowEx(WS_EX_NOPARENTNOTIFY,
    achNanStatusClass,
    NOTHING,
    WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS ,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    hwndParent,
    NOTHING,
    _DLL_hInstance,
    NOTHING);

  return (hwndChild);

}        /* End of HWND CreateNanStatusWnd */

/********************************** 
  HWND CreateNanBarWnd()
  Description:
    Creates a Nantucket Button Window 
  Access to Globals:
    
**********************************/ 
NANACT_LIBAPI HWND FAR PASCAL CreateNanBarWnd(HWND hwndParent, HANDLE hInstance, UINT * lpumsg)
{
  HWND            hwndChild;

#ifdef RC_EXTERNAL
  hResource=hInstance;
#endif

  /*  MS0001 - took off WS_CLIPSIBLINGS style     */
  /* Emb 2/2/95 : put back WS_CLIPSIBLINGS style */
  hwndChild = CreateWindowEx(WS_EX_NOPARENTNOTIFY,
    achNanBarClass,
    NOTHING,
    WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, 
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    hwndParent,
    NOTHING,
    _DLL_hInstance,
    (LPSTR) 0);
    
  *lpumsg = (UINT)NULL;
  if (hwndChild)
  {
     LPNANBARDATA pNanBar = (LPNANBARDATA) GetWindowLong(hwndChild, GWL_USERDATA);
     *lpumsg = NANBAR_MSGNOTIFY();
  }

  return (hwndChild);
}        /* End of HWND CreaeNanBarWnd  */


static VOID NEAR PASCAL _paintNanBar(HDC hDCGiven, LPNANBARDATA pNanBar, BOOL fAll)
{
  RECT           rect;
  HDC            hMemoryDC;
  LPNANBTNLIST   pAct;
  HDC            hDC;
  HPEN           hOldPen;
  HPEN           hGrayPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNFACE));


  if (hDCGiven)
    hDC = hDCGiven;
  else
    hDC = GetDC(NANBAR_SELF());

  hOldPen=SelectObject(hDC,hGrayPen);

  GetClientRect(NANBAR_SELF(), &rect);

  hMemoryDC = CreateCompatibleDC(hDC);
  pAct = NANBAR_ROOT();

  rect.top+=  NANBAR_BARBORDER;
  rect.left+= 2 * NANBAR_BARBORDER;

  while (pAct)
  {
    if (fAll || CHANGED(pAct))
    {
      SHORT sLeft, sTop, sWidth, sHeight;
      rect.bottom   = rect.top + NANBAR_BTNH() + NANBAR_PENWIDTH;
      rect.right    = rect.left + NANBAR_BTNW() + NANBAR_PENWIDTH;

      sLeft   = rect.left + NANBAR_PENWIDTH;
      sTop    = rect.top + NANBAR_PENWIDTH;
      sWidth  = NANBAR_BTNW() - NANBAR_PENWIDTH;
      sHeight = NANBAR_BTNH() - NANBAR_PENWIDTH;

       /* Modified Emb 1. Feb. 95 since we have several dummy items */
       /* Old Code : if (ID(pAct) != IDM_EB_FILTER) */
       if (ID(pAct)!=IDM_DUMMY1
         && ID(pAct)!=IDM_DUMMY2
         && ID(pAct)!=IDM_DUMMY3)
      {
        BITMAP Bitmap;

        SelectObject(hMemoryDC, BMP(pAct));
        GetObject(BMP(pAct), sizeof(BITMAP), (LPSTR) &Bitmap);

          if (sWidth == Bitmap.bmWidth &&  sHeight == Bitmap.bmHeight)
            BitBlt(hDC,sLeft,sTop,sWidth, sHeight,hMemoryDC,0,0,ENABLED(pAct) ? SRCCOPY : NOTSRCCOPY);
          else
            StretchBlt(hDC,sLeft,sTop,sWidth, sHeight,hMemoryDC, 0, 0,Bitmap.bmWidth,Bitmap.bmHeight,ENABLED(pAct) ? SRCCOPY : NOTSRCCOPY);

        SelectObject(hDC, hGrayPen);
        MoveToEx(hDC, rect.right-1, rect.top, NULL);
        LineTo(hDC, rect.right-1, rect.bottom-1);
        LineTo(hDC, rect.left, rect.bottom-1);


        if (PRESSED(pAct))
        {
          SelectObject(hDC, GetStockObject(BLACK_PEN));
          MoveToEx(hDC, rect.left, rect.bottom, NULL);
          LineTo(hDC, rect.left, rect.top);
          LineTo(hDC, rect.right, rect.top);
          SelectObject(hDC, GetStockObject(WHITE_PEN));
          LineTo(hDC, rect.right, rect.bottom);
          LineTo(hDC, rect.left, rect.bottom);
        }
        else
        {
          SelectObject(hDC, GetStockObject(WHITE_PEN));
          MoveToEx(hDC, rect.left, rect.bottom, NULL);
          LineTo(hDC, rect.left, rect.top);
          LineTo(hDC, rect.right, rect.top);
          SelectObject(hDC, GetStockObject(BLACK_PEN));
          LineTo(hDC, rect.right, rect.bottom);
          LineTo(hDC, rect.left, rect.bottom);
        }
      }
      if (fAll)
        ExcludeClipRect(hDC,rect.left,rect.top,rect.right+1,rect.bottom+1);
      RECT(pAct).left = sLeft;
      RECT(pAct).top = sTop;
      RECT(pAct).right = sLeft+sWidth;
      RECT(pAct).bottom = sTop+sHeight;
    }

    CHANGED(pAct) = FALSE;
    rect.left+=   NANBAR_BTNW() + 2 * NANBAR_BARBORDER;
    if (BREAK(pAct))
      rect.left += NANBAR_BTNW() / 2;
    SKIP(pAct);
  }

  if (fAll)
  {
    char c=' ';
    GetClientRect(NANBAR_SELF(), &rect);
    rect.bottom-=2;
    SelectObject(hDC,GetStockObject(BLACK_PEN));
    SetBkColor(hDC, GetSysColor(COLOR_BTNFACE));
    ExtTextOut(hDC,rect.left,rect.top,ETO_OPAQUE|ETO_CLIPPED,&rect,&c,1,NULL);

    SetBkColor(hDC, RGB(0,0,0));
    rect.top=rect.bottom;
    rect.bottom+=2;
    ExtTextOut(hDC,rect.left,rect.top,ETO_OPAQUE|ETO_CLIPPED,&rect,&c,1,NULL);
  }

  SelectObject(hDC,hOldPen);
  DeleteObject(hGrayPen);

  DeleteDC(hMemoryDC);
  if (!hDCGiven)
    ReleaseDC(NANBAR_SELF(), hDC);
}


static LRESULT NEAR PASCAL HandleNanUserMessages(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  LPNANBTNLIST pNew;
  BOOL fDone;

  LPNANBARDATA pNanBar = (LPNANBARDATA) GetWindowLong(hwnd, GWL_USERDATA);

  if (!pNanBar)
    return FALSE;

  switch (message)
  {
    case NANBAR_GETHEIGHT:
        return (NANBAR_BTNH() + NANBAR_WNDOFFSET + NANBAR_PENWIDTH);

    case NANBAR_CREATEBTN:
    {
      LPNANBTNDATA  pData = (LPNANBTNDATA) lParam;
      pNew = (LPNANBTNLIST) MALLOC(sizeof(NANBTNLIST));
      assert(pNew);
      BMP(pNew)       = pData->hBitMap;
      ID(pNew)        = pData->wButtonID;
      STRINGID(pNew)  = pData->wNotifyStringID;
      BREAK(pNew)     = pData->fBreak;
      CHECKMARK(pNew) = pData->fCheckMark;

      PRESSED(pNew)   = FALSE;
      NOTIFIED(pNew)  = FALSE;
      NEXT(pNew)      = NULL;
      CHANGED(pNew)   = TRUE;
      ENABLED(pNew)   = TRUE;

      if (NANBAR_ROOT())
      {
        NEXT(NANBAR_LAST()) = pNew;
        NANBAR_LAST() = pNew;
      }
      else
      {
        NANBAR_ROOT() = pNew;
        NANBAR_LAST() = pNew;
      }
    }
    return (TRUE);

  case NANBAR_DISABLE:
    pNew = NANBAR_ROOT();
    fDone = FALSE;
    while (pNew && !fDone)
    {
      if (ID(pNew) == (WORD) lParam)
      {
        fDone = ENABLED(pNew);
        ENABLED(pNew) = FALSE;
        if (fDone)
        {
          CHANGED(pNew) = TRUE;
          //_paintNanBar((HDC) 0, pNanBar, FALSE);
          InvalidateRect(NANBAR_SELF(),&RECT(pNew),FALSE);
        }
        fDone = TRUE;
      }
      SKIP(pNew);

    }
    return (fDone);

  case NANBAR_ISDISABLED:
    for (pNew = NANBAR_ROOT();pNew;SKIP(pNew))
    {
      if (STRINGID(pNew) == (WORD) lParam)
      {
        return (!ENABLED(pNew));
      }
    }
    return FALSE;

  case NANBAR_BTNSTATUS:
    for (pNew = NANBAR_ROOT();pNew;SKIP(pNew))
    {
      if (ID(pNew) == (WORD) lParam)
      {
        return (ENABLED(pNew));
      }
    }
    return FALSE;

  case NANBAR_ENABLE:
    pNew = NANBAR_ROOT();
    fDone = FALSE;
    while (pNew && !fDone)
    {
      if (ID(pNew) == (WORD) lParam)
      {
        fDone = ENABLED(pNew);
        ENABLED(pNew) = TRUE;
        if (!fDone)
        {
          CHANGED(pNew) = TRUE;
          //_paintNanBar((HDC) 0, pNanBar, FALSE);
          InvalidateRect(NANBAR_SELF(),&RECT(pNew),FALSE);
        }
        fDone = TRUE;
      }
      SKIP(pNew);
    }
    return (fDone);

  case NANBAR_SETCHECK:
  case NANBAR_RELEASECHECK:
    pNew = NANBAR_ROOT();
    fDone = FALSE;
    while (pNew && !fDone)
    {
      if (ID(pNew) == (WORD) lParam)
      {
        fDone = PRESSED(pNew);
        PRESSED(pNew) = (message == NANBAR_SETCHECK) ? TRUE : FALSE;
        if (fDone != PRESSED(pNew))
        {
          CHANGED(pNew) =TRUE;
          InvalidateRect(NANBAR_SELF(),&RECT(pNew),FALSE);
          //_paintNanBar((HDC) 0, pNanBar, FALSE);
        }
        fDone = TRUE;
      }
      SKIP(pNew);
    }
    return (fDone);
  }
  return (FALSE);
}


LRESULT WINAPI EXPORT NanBarWndProc (HWND hwnd, UINT message,WPARAM wParam, LONG lParam)
{
    switch (message)
    {
        HANDLE_MSG(hwnd, WM_CREATE, NanOnCreate);
        HANDLE_MSG(hwnd, WM_DESTROY, NanOnDestroy);
        HANDLE_MSG(hwnd, WM_SIZE, NanOnSize);
        HANDLE_MSG(hwnd, WM_PAINT, NanOnPaint);
        HANDLE_MSG(hwnd, WM_MOUSEMOVE, NanOnMouseMove);
        HANDLE_MSG(hwnd, WM_LBUTTONDOWN, NanOnLButtonDown);
        HANDLE_MSG(hwnd, WM_LBUTTONUP, NanOnLButtonUp);

        default:
        {
            if (message >= WM_USER)
            {
                return HandleNanUserMessages(hwnd, message, wParam, lParam);
            }
        }
    }

    return (DefWindowProc(hwnd, message, wParam, lParam));
}       


static BOOL NanOnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    LPNANBARDATA pNanBar = (LPNANBARDATA) MALLOC(sizeof(NANBARDATA));

    if (!pNanBar)
      return FALSE;

    SetWindowLong(hwnd, GWL_USERDATA, (LONG) pNanBar);

    NANBAR_SELF()       = hwnd;
    NANBAR_PARENT()     = lpCreateStruct->hwndParent;
    NANBAR_INSTANCE()   = lpCreateStruct->hInstance;
    NANBAR_ROOT()       = NULL;
    NANBAR_LAST()       = NULL;
    NANBAR_WIDTH()      = 0;

    NANBAR_BTNH() = 17;
    NANBAR_BTNW() = 21;                        

    NANBAR_MSGNOTIFY() = RegisterWindowMessage("msgNanBarNotify");

    return TRUE;
}

static void NanOnDestroy(HWND hwnd)
{
    LPNANBARDATA pNanBar = (LPNANBARDATA) GetWindowLong(hwnd, GWL_USERDATA);

    if (pNanBar)
    {
        LPNANBTNLIST pAct= NANBAR_ROOT();

        while (pAct)
        {
            LPNANBTNLIST  pKill=pAct;
            NanBar_NotifyFreeBmp(NANBAR_PARENT(), NANBAR_MSGNOTIFY(), BMP(pAct));
            SKIP(pAct);
            FREE(pKill);
        }

        FREE(pNanBar);
        SetWindowLong( hwnd, GWL_USERDATA, 0L );
    }
}

static void NanOnSize(HWND hwnd, UINT state, int cx, int cy)
{
    LPNANBARDATA pNanBar = (LPNANBARDATA) GetWindowLong(hwnd, GWL_USERDATA);

    if (pNanBar)
    {
        RECT    rect;

        if (NANBAR_WIDTH() <= cx)
        {
              rect.left = NANBAR_WIDTH() - (4*NANBAR_BARBORDER);
              rect.top = 0;
              rect.right = cx;
              rect.bottom = cy;
              InvalidateRect(hwnd, &rect, FALSE);
        }

        InvalidateRect(hwnd, NULL, FALSE);
        NANBAR_WIDTH() = cx;
    }
}

static void NanOnPaint(HWND hwnd)
{
    PAINTSTRUCT  PaintStruct;
    LPNANBARDATA pNanBar = (LPNANBARDATA) GetWindowLong(hwnd, GWL_USERDATA);

    BeginPaint(hwnd, &PaintStruct);
    if (pNanBar)
        _paintNanBar(PaintStruct.hdc, pNanBar, TRUE);
    EndPaint(hwnd, &PaintStruct);
}

static void NanOnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
{
    LPNANBTNLIST pAct;
    BOOL fDone;
    LPNANBARDATA pNanBar = (LPNANBARDATA) GetWindowLong(hwnd, GWL_USERDATA);

    if (!pNanBar)
        return;

    pAct = NANBAR_ROOT();
    fDone = FALSE;

    while (pAct && !fDone)
    {
        POINT pt;

        pt.x = x;
        pt.y = y;

        if (PtInRect(&RECT(pAct), pt))
        {
            if (!NOTIFIED(pAct))
            {
                RECT rc;
                POINT pt;
                char ach[256];
                HINSTANCE hInst=GetWindowInstance(GetParent(hwnd));

#ifdef RC_EXTERNAL
				if (hResource!=_DLL_hInstance)
					hInst=hResource;
#endif
                LoadString(hInst,STRINGID(pAct),ach,sizeof(ach));
                rc=RECT(pAct);
                ClientToScreen(hwnd,(LPPOINT) &rc.left);
                ClientToScreen(hwnd,(LPPOINT) &rc.right);
                pt.x=(rc.left+rc.right)/2;
                pt.y=rc.top+NANBAR_BARBORDER;

                // Keep notification msg and balloon in parallel
                NanBar_NotifyMsg(NANBAR_PARENT(), NANBAR_MSGNOTIFY(), STRINGID(pAct));
                BalloonUp(ach,pt,&rc);

                NOTIFIED(pAct) = TRUE;
            }        
            fDone = TRUE;
        }
        else
        {
            NOTIFIED(pAct) = FALSE;
        }
        SKIP (pAct);
    }

    if (!fDone)
    {
        // Keep notification msg and balloon in parallel
        NanBar_NotifyMsg(NANBAR_PARENT(), NANBAR_MSGNOTIFY(), 0);
        BalloonDown();
    }
}

static void NanOnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
    LPNANBTNLIST pAct;
    BOOL fDone;
    LPNANBARDATA pNanBar = (LPNANBARDATA) GetWindowLong(hwnd, GWL_USERDATA);

    if (!pNanBar)
        return;

    pAct = NANBAR_ROOT();
    fDone = FALSE;

    while (pAct && !fDone)
    {
        if (ENABLED(pAct))
        {
            POINT pt;

            pt.x = x;
            pt.y = y;

            if (PtInRect(&RECT(pAct), pt))
            {
                PRESSED(pAct) = !PRESSED(pAct);
                CHANGED(pAct) = TRUE;
                SetCapture(hwnd);
                NANBAR_CAPTURE() = TRUE;
                fDone = TRUE;
                _paintNanBar((HDC) 0, pNanBar, FALSE);
                if (CHECKMARK(pAct))
                {
                    SendMessage(NANBAR_PARENT(), WM_COMMAND, ID(pAct), (LPARAM) PRESSED(pAct)); 
                }
            }
        }

        SKIP (pAct);
    }
}

static void NanOnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
{
    LPNANBTNLIST pAct;
    BOOL fDone;
    LPNANBARDATA pNanBar = (LPNANBARDATA) GetWindowLong(hwnd, GWL_USERDATA);

    if (!pNanBar)
        return;

    if (NANBAR_CAPTURE())
    {
        ReleaseCapture();

        pAct = NANBAR_ROOT();
        fDone = FALSE;
        while (pAct)
        {
            POINT pt;

            pt.x = x;
            pt.y = y;

            if (PtInRect(&RECT(pAct), pt))
            {
                if (PRESSED(pAct) &&  !CHECKMARK(pAct))
                {
                    PRESSED(pAct) = FALSE;
                    CHANGED(pAct) = TRUE;
                    fDone = TRUE;
                    break;
                }
            }
            else
            {
                if (PRESSED(pAct) && !CHECKMARK(pAct))
                {
                    PRESSED(pAct) = FALSE;
                    CHANGED(pAct) = TRUE;
                    fDone = TRUE;
                }
            }

            SKIP (pAct);
        }

        if (fDone)
        {
            _paintNanBar((HDC) 0, pNanBar, FALSE);
            if (pAct)
            {
                PostMessage(NANBAR_PARENT(), WM_COMMAND, ID(pAct), 0L); 
            }
        }
    }
}

/* */
/*      NanStatusLine */

static VOID NEAR PASCAL StatusMsgInvalidate(HWND hwnd,PSTATUSWNDDATA pStatusWndData)
{
  RECT rect;
  GetClientRect(hwnd, &rect);
  rect.left+=   STATUS_OFFSET+3;
  rect.top+=    STATUS_OFFSET+2;
  rect.bottom-= STATUS_OFFSET+1;
  rect.right-=  STATUS_OFFSET+1;
  InvalidateRect(hwnd,&rect,FALSE);
  UpdateWindow(hwnd);
}

/********************************** 
  static VOID StatusMsgPaint()
  Description:
    puts a message in the status line
  Access to Globals:
**********************************/ 
static VOID NEAR PASCAL StatusMsgPaint(HWND hwnd,HDC hDC,PSTATUSWNDDATA pStatusWndData)
{
  RECT  rect;
  HFONT hOldFont;
  HPEN  hOldPen;

  GetClientRect(hwnd, &rect);
  hOldFont=SelectObject(hDC, STATUS_FONT());
  SetBkColor(hDC, STATUS_BKCOLOR());

  ExtTextOut(hDC,STATUS_OFFSET+3,STATUS_OFFSET+2,ETO_OPAQUE,&rect,STATUS_TEXT(),STATUS_TEXTLEN(),NULL);

  hOldPen=SelectObject(hDC, STATUS_GRAYPEN());
  MoveToEx(hDC, STATUS_OFFSET, rect.bottom-STATUS_OFFSET-1, NULL);
  LineTo(hDC, STATUS_OFFSET, rect.top+STATUS_OFFSET);
  LineTo(hDC, rect.right-STATUS_OFFSET-1, rect.top+STATUS_OFFSET);

  SelectObject(hDC, STATUS_WHITEPEN());
  LineTo(hDC, rect.right-STATUS_OFFSET-1, rect.bottom-STATUS_OFFSET-1);
  LineTo(hDC, STATUS_OFFSET, rect.bottom-STATUS_OFFSET-1);
  SelectObject(hDC, hOldFont);
  SelectObject(hDC, hOldPen);
}

static VOID NEAR PASCAL StatusGetHeigth(HWND hwnd,PSTATUSWNDDATA pStatusWndData)
{
  char  c='A';
  HFONT hOldFont;
  HDC hDC=GetDC(hwnd);
  hOldFont=SelectObject(hDC, STATUS_FONT());
  STATUS_HEIGTH() = HIWORD(GetTextExtent(hDC,&c,1))+STATUS_OVERHEAD;
  SelectObject(hDC,hOldFont);
  ReleaseDC(hwnd,hDC);
}

/********************************** 
  LONG EXPORT NanStatusLineProc()
  Description:
    Window function for nantucket editor controll
  Access to Globals:
**********************************/ 
LRESULT WINAPI EXPORT NanStatusLineProc(HWND hwnd, UINT message,WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
    HANDLE_MSG(hwnd, WM_CREATE, StatusOnCreate);
    HANDLE_MSG(hwnd, WM_PAINT, StatusOnPaint);
    HANDLE_MSG(hwnd, WM_DESTROY, StatusOnDestroy);

    default:
    {
        if (message >= WM_USER)
        {
            return HandleStatusUserMessages(hwnd, message, wParam, lParam);
        }
    }
  }
  return DefWindowProc(hwnd, message,wParam, lParam);
}        /* End of LONG EXPORT NEDStatusLineProc */

static LRESULT HandleStatusUserMessages(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case STATUS_SETMSG:
        {
            PSTATUSWNDDATA pStatusWndData = (PSTATUSWNDDATA) GetWindowLong(hwnd, GWL_USERDATA);

            LPSTR lpstrTemp = (LPSTR)lParam;
            if (lpstrTemp && pStatusWndData)
            {
                INT i;

                STATUS_TEXTLEN() = lstrlen(lpstrTemp);
                if (STATUS_TEXTLEN() > STATUS_MAXLINELEN)
                { 
                    STATUS_TEXTLEN() = STATUS_MAXLINELEN;
                }
                _fmemset(STATUS_TEXT(), 0, STATUS_MAXLINELEN+1);    
                for (i=0; *(lpstrTemp+i) != '\r' && i<STATUS_TEXTLEN(); i++)
                {
                    if (*(lpstrTemp+i) != '\t')
                    {
                        *(STATUS_TEXT()+i) = *(lpstrTemp+i);
                    }
                    else
                    {
                        *(STATUS_TEXT()+i) = ' ';
                    }
                }
                STATUS_TEXTLEN() = i;
                StatusMsgInvalidate(hwnd,pStatusWndData);
            }
            break;
        }

        case STATUS_GETHEIGHT:
        {
            PSTATUSWNDDATA pStatusWndData = (PSTATUSWNDDATA) GetWindowLong(hwnd, GWL_USERDATA);
            return (pStatusWndData ? STATUS_HEIGTH() : 1);
        }

        case STATUS_FONTCHG:
        {
            PSTATUSWNDDATA pStatusWndData = (PSTATUSWNDDATA) GetWindowLong(hwnd, GWL_USERDATA);
            if (pStatusWndData)
            {
                STATUS_FONT() = (HFONT) lParam;
                StatusGetHeigth(hwnd,pStatusWndData);
            }
            return (TRUE);
        }
    }

    return 0;
}

static BOOL StatusOnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
  PSTATUSWNDDATA pStatusWndData = (PSTATUSWNDDATA) MALLOC(sizeof(STATUSWNDDATA));

  if (pStatusWndData)
  {
    LOGBRUSH       logBrush;
    STATUS_FONT() =(HFONT) (LONG) (lpCreateStruct->lpCreateParams);

    if (!STATUS_FONT())
      STATUS_FONT() = GetStockObject(SYSTEM_FONT);

    StatusGetHeigth(hwnd,pStatusWndData);

    STATUS_GRAYPEN()    = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNSHADOW));
    STATUS_BLACKPEN()   = GetStockObject(BLACK_PEN);
    STATUS_WHITEPEN()   = GetStockObject(WHITE_PEN);
    STATUS_BRUSH()      = GetStockObject(LTGRAY_BRUSH);

    GetObject(STATUS_BRUSH(), sizeof(LOGBRUSH), (LPSTR) &logBrush);
    STATUS_BKCOLOR()        = logBrush.lbColor;

    STATUS_TEXT()  = (PSZ) MALLOCSTR(STATUS_MAXLINELEN);
    assert(STATUS_TEXT());
    _fmemset(STATUS_TEXT(), 0, STATUS_MAXLINELEN+1);
    STATUS_TEXTLEN() = 0;

    ERR_MEMWALK(" NEDStatusLineProc");
    SetWindowLong(hwnd, GWL_USERDATA, (LONG) pStatusWndData);
    return TRUE;
  }
  return FALSE;
}


static void StatusOnPaint(HWND hwnd)
{
  PAINTSTRUCT paintStruct;
  BeginPaint(hwnd, &paintStruct);
  StatusMsgPaint(hwnd, paintStruct.hdc, (PSTATUSWNDDATA) GetWindowLong(hwnd, GWL_USERDATA));
  EndPaint(hwnd, &paintStruct);
}


static void StatusOnDestroy(HWND hwnd)
{
  PSTATUSWNDDATA pStatusWndData = (PSTATUSWNDDATA) GetWindowLong(hwnd, GWL_USERDATA);
  if (pStatusWndData)
  {
    DeleteObject(STATUS_GRAYPEN());
    FREE(STATUS_TEXT());
    FREE(pStatusWndData);
    SetWindowLong( hwnd, GWL_USERDATA, 0L );
  }
}
