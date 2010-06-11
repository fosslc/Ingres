/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project : CA/OpenIngres Visual DBA
//
//    Source : statbar.c
//    ribbon bar management
//
//    Author : Rich Williams
//
// History:
// uk$so01 (20-Jan-2000, Bug #100063)
//         Eliminate the undesired compiler's warning
// 01-Jun-2010 (drivi01)
//	Update SB_MAXTEXT length to accomodate long ids.
********************************************************************/

#ifdef __BORLANDC__
 #pragma hdrfile windows.sym
 #include <windows.h>
 #include <windowsx.h>
 #pragma hdrstop
#else
 #include "dba.h"
// #include <windows.h>
 #include <windowsx.h>
#endif

#include <string.h>
#include "statbar.h"
#include "dbadlg1.h"        // ZEROINIT

#define SB_PENFRAME     0x00000001
#define SB_PENLIGHT     0x00000002
#define SB_PENSHADOW    0x00000004
#define SB_PENFACE      0x00000008
#define SB_BRUSHFACE    0x00000010


#define SB_CLASSNAME  "CASTATBAR"
#define SB_MAXTEXT    MAXOBJECTNAME*12  // Changed from 1500 by drivi01 June 1, 2010
			    		// Unexplained BUFFER OVERFLOW.

typedef struct tagstatfield
{
  WORD     wID;
  WORD     wType;
  RECT     boundRect;
  RECT     drawRect;
  COLORREF lTextColor;
  COLORREF lFaceColor;
  HFONT    hFont;
  WORD     wFormat;
  HBITMAP  hBitmap;
  WORD     nText;
  WORD     wTextOffset;
  BOOL     bVisible;
} STATFIELD, FAR *LPSTATFIELD;

typedef struct tagstatbar
{
  LONG        lStockObjFlag; /* indicates which objects are stock objects
				so that they will not be deleted */
  HPEN        hpenFrame;     /* pen for drawing field frame */
  HPEN        hpenLight;     /* pen for drawing field highlight */
  HPEN        hpenShadow;    /* pen for drawing field shadow */
  COLORREF    lBkgnColor;    /* background color */
  HPEN        hpenFace;      /* pen for drawing field interior */
  HPEN        hSavePen;      /* save the old pen */
  HBRUSH      hbrushFace;    /* brush for drawing field interior */
  HBRUSH      hbrushBgn;     /* brush for drawing window background */
  HBRUSH      hSaveBrush;
  HFONT       hFont;
  RECT        boundRect;
  RECT        drawRect;
  HFONT       hSaveFont;
  BOOL        bSuperFlag;    /* TRUE, if super field is truned on */
  WORD        wSuperIdx;     /* index of the super field in fieldList,
				the value is the actual index + 1.
				A zero means there is no super field. */
  WORD        nFields;
  char        szText[SB_MAXTEXT];
  STATFIELD   fieldList[1];
} STATBAR, FAR *LPSTATBAR;


LRESULT CALLBACK EXPORT StatBarWndProc( HWND hwnd, UINT message,
					WPARAM wParam, LPARAM lParam );

static void SB_EraseBackground( HWND hwnd, HDC hdc );
static void DrawStatBar( HWND hwnd, LPPAINTSTRUCT lpps );
static void SB_DrawField( LPPAINTSTRUCT lpps, LPSTATBAR lpStatBar, WORD wFieldIdx );
static void SB_SetField( HWND hwnd, WORD wID, LPSTR lpText );
static void DestroyStatBar( HWND hwnd );
static void SB_SetMode( HWND hwnd, WORD wMode, LONG lParam );
static void PASCAL SB_SetBarColor( HWND hwnd, COLORREF lColor );
static void PASCAL SB_SetFieldColor( HWND hwnd, WORD wID, COLORREF lColor, WORD wWhat );
static void PASCAL SB_ShowField( HWND hwnd, WORD wID, LONG lShowHide );
static void PASCAL SB_SizeField( HWND hwnd, WORD wID, LPRECT lpRect );
static void PASCAL SB_Size(HWND hWnd, UINT state, int cx, int cy);
static int InitStatusBar(HINSTANCE hinst);
static BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
static BOOL OnEraseBkgnd(HWND hwnd, HDC hdc);
static void OnPaint(HWND hwnd);
static void OnDestroy(HWND hwnd);
static void OnSize(HWND hwnd, UINT state, int cx, int cy);
static void OnSetText(HWND hwnd, LPCSTR lpszText);


#ifdef __DLL__
#ifdef WIN32
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved);
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
	    InitStatusBar(hInstance);
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
int FAR PASCAL LibMain( HANDLE hInstance, WORD wDataSeg, WORD wHeapSize,
			LPSTR lpszCmdLine )
{
  if (wHeapSize > 0)
    UnlockData( 0 );

  return InitStatusBar(hInstance);
}
#endif      // WIN32
#endif      // __DLL__


static int InitStatusBar(HINSTANCE hinst)
{
    WNDCLASS    wndclass;

    if (GetClassInfo(hinst, SB_CLASSNAME, &wndclass))
	return TRUE;

    ZEROINIT(wndclass);

    /* Register the status bar window class */
    wndclass.style         = CS_GLOBALCLASS | CS_HREDRAW;
    wndclass.lpfnWndProc   = StatBarWndProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = sizeof(long);     /* handle to custom data */
    wndclass.hInstance     = hinst;
    wndclass.hIcon         = NULL;
    wndclass.hCursor       = LoadCursor( NULL, IDC_ARROW );
    wndclass.hbrBackground = NULL;
    wndclass.lpszMenuName  = NULL;
    wndclass.lpszClassName = SB_CLASSNAME;

    return RegisterClass( &wndclass );
}


HWND NewStatusBar( HANDLE hInst, HWND hwndParent, LPSB_DEFINITION lpStatBarDef )
{
  HANDLE      hStatBar;
  LPSTATBAR   lpStatBar;
  HWND        hwndStatBar;
  LPSTATFIELD lpStatField;
  LPSB_FIELDDEF  lpFieldDef;
  UINT         i;
  WORD        wOffset;

#ifndef __DLL__
  if (!InitStatusBar(hInst))
    return (HWND)NULL;
#endif

  /* Create status bar window */
  hwndStatBar = CreateWindow( SB_CLASSNAME,
			      "",
			      WS_CHILD | WS_CLIPSIBLINGS,
			      lpStatBarDef->x,
			      lpStatBarDef->y,
			      lpStatBarDef->cx,
			      lpStatBarDef->cy,
			      hwndParent,
			      NULL,
			      hInst,
			      0L );
  if (hwndStatBar)
  {
    hStatBar = GlobalAlloc( GHND, sizeof(STATBAR) + (sizeof(STATFIELD) *
				     (lpStatBarDef->nFields-1)) );
    if (hStatBar == NULL)
    {
      DestroyWindow( hwndStatBar );
      return NULL;
    }
    lpStatBar = (LPSTATBAR) GlobalLock( hStatBar );


    lpStatBar->hpenFrame = CreatePen( PS_SOLID, 1, lpStatBarDef->lFrameColor );
    lpStatBar->hpenLight = CreatePen( PS_SOLID, 1, lpStatBarDef->lLightColor );
    lpStatBar->hpenShadow = CreatePen( PS_SOLID, 1, lpStatBarDef->lShadowColor );
    lpStatBar->lBkgnColor = lpStatBarDef->lBkgnColor;
    lpStatBar->hpenFace = CreatePen( PS_SOLID, 1, lpStatBar->lBkgnColor );
    lpStatBar->hbrushFace = CreateSolidBrush( lpStatBar->lBkgnColor );
    lpStatBar->hbrushBgn = CreateSolidBrush( lpStatBar->lBkgnColor );

    GetClientRect( hwndStatBar, &lpStatBar->boundRect );
    lpStatBar->drawRect = lpStatBar->boundRect;
    InsetRect( &lpStatBar->drawRect, 2, 2);

    lpStatBar->nFields = lpStatBarDef->nFields;


    for (i = 0, lpFieldDef = lpStatBarDef->fields,
	   lpStatField = lpStatBar->fieldList, wOffset = 1;
	 i < lpStatBarDef->nFields;
	 i++, lpFieldDef++, lpStatField++ )
    {
      lpStatField->wID         = lpFieldDef->wID;
      lpStatField->wType       = lpFieldDef->wType;
      lpStatField->boundRect   = lpFieldDef->boundRect;
      lpStatField->drawRect    = lpStatField->boundRect;
      if ((lpStatField->wType & SBT_3D) || (lpStatField->wType & SBT_FRAME))
	InsetRect( &lpStatField->drawRect, 1, 1);
      lpStatField->lTextColor  = lpFieldDef->lTextColor;
      lpStatField->lFaceColor  = lpFieldDef->lFaceColor;
      lpStatField->hFont       = lpFieldDef->hFont;
      lpStatField->wFormat     = lpFieldDef->wFormat;
      lpStatField->nText       = lpFieldDef->wLength;
      lpStatField->wTextOffset = wOffset;
      lpStatField->bVisible    = (lpFieldDef->wType & SBT_VISIBLE) != 0;

      wOffset += lpFieldDef->wLength+1;

      if (lpStatField->wType & SBT_SUPER)
      {
	if (lpStatBar->wSuperIdx == 0)
	  lpStatBar->wSuperIdx = i + 1;
      }
    }

    GlobalUnlock( hStatBar );
    SetWindowLong(  hwndStatBar, GWL_USERDATA, (LPARAM)hStatBar );
  }

  return hwndStatBar;
}


LRESULT CALLBACK EXPORT StatBarWndProc( HWND hwnd, UINT message,
					WPARAM wParam, LPARAM lParam )
{
  switch (message)
  {
    HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
    HANDLE_MSG(hwnd, WM_ERASEBKGND, OnEraseBkgnd);
    HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
    HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
    HANDLE_MSG(hwnd, WM_SIZE, OnSize);
    HANDLE_MSG(hwnd, WM_SETTEXT, OnSetText);

    case SBM_SETFIELD:
      SB_SetField( hwnd, (WORD)wParam, (LPSTR)lParam );
      break;

    case SBM_SETMODE:
      SB_SetMode( hwnd, (WORD)wParam, 0L );
      break;

    case SBM_SHOWFIELD:
      SB_ShowField( hwnd, (WORD)wParam, lParam );
      break;

    case SBM_SIZEFIELD:
      SB_SizeField( hwnd, (WORD)wParam, (LPRECT)lParam );
      break;

    case SBM_SETBARBKGCOLOR:
      SB_SetBarColor( hwnd, lParam );
      return 0;

    case SBM_SETFIELDFACECOLOR:
      /* wParam == 0xffff change all fields' color */
      SB_SetFieldColor( hwnd, (WORD)wParam, (COLORREF)lParam, 0 );
      return 0;

    case SBM_SETFIELDTEXTCOLOR:
      /* wParam == 0xffff change all fields' color */
      SB_SetFieldColor( hwnd, (WORD)wParam, (COLORREF)lParam, 1 );
      return 0;
  }

  return DefWindowProc( hwnd, message, wParam, lParam );
}

static BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    return TRUE;
}

static BOOL OnEraseBkgnd(HWND hwnd, HDC hdc)
{
    SB_EraseBackground( hwnd, hdc );
    return TRUE;
}

static void OnPaint(HWND hwnd)
{
    PAINTSTRUCT ps;

    BeginPaint( hwnd, &ps );
    DrawStatBar( hwnd, &ps );
    EndPaint( hwnd, &ps );
}

static void OnDestroy(HWND hwnd)
{
    DestroyStatBar( hwnd );
}


static void OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    SB_Size( hwnd, state, cx, cy );
}

static void OnSetText(HWND hwnd, LPCSTR lpszText)
{
      SB_SetField( hwnd, 1, (LPSTR)lpszText );
}

static void SB_EraseBackground( HWND hwnd, HDC hdc )
{
  HANDLE      hStatBar;
  LPSTATBAR   lpStatBar;
  RECT        rectClient;

  hStatBar = (HWND)GetWindowLong( hwnd, GWL_USERDATA );
  if (hStatBar == NULL)
    return;

  lpStatBar = (LPSTATBAR) GlobalLock( hStatBar );
  if (lpStatBar == NULL)
    return;

  UnrealizeObject( lpStatBar->hbrushBgn );
  GetClientRect( hwnd, &rectClient );

  FillRect( hdc, &rectClient, lpStatBar->hbrushBgn );
  MoveToEx( hdc, rectClient.left, rectClient.top, NULL );
  LineTo( hdc, rectClient.right, rectClient.top );

#if 0
  hSavePen = SelectObject( hdc, lpStatBar->hpenLight );
  MoveToEx( hdc, rectClient.left, rectClient.bottom,NULL );
  LineTo( hdc, rectClient.left, rectClient.top );
  LineTo( hdc, rectClient.right, rectClient.top );

  MoveToEx( hdc, rectClient.left+1, rectClient.bottom-1,NULL );
  LineTo( hdc, rectClient.left+1, rectClient.top+1 );
  LineTo( hdc, rectClient.right-1, rectClient.top+1 );

  SelectObject( hdc, lpStatBar->hpenShadow );
  MoveToEx( hdc, rectClient.left+1, rectClient.bottom,NULL );
  LineTo( hdc, rectClient.right, rectClient.bottom );
  LineTo( hdc, rectClient.right, rectClient.top+1 );

  MoveToEx( hdc, rectClient.left+2, rectClient.bottom-1,NULL );
  LineTo( hdc, rectClient.right-1, rectClient.bottom-1 );
  LineTo( hdc, rectClient.right-1, rectClient.top+2 );
  SelectObject( hdc, hSavePen );
#endif

  GlobalUnlock( hStatBar );
}



static void DrawStatBar( HWND hwnd, LPPAINTSTRUCT lpps )
{
  HANDLE      hStatBar;
  LPSTATBAR   lpStatBar;
  int         i;

  hStatBar = (HWND)GetWindowLong( hwnd, GWL_USERDATA );
  if (hStatBar == NULL)
    return;

  lpStatBar = (LPSTATBAR) GlobalLock( hStatBar );

  SetBkMode( lpps->hdc, TRANSPARENT );

  if (lpStatBar->bSuperFlag)
  {
    /* Draw the super field only */
    if (lpStatBar->wSuperIdx != 0)
      SB_DrawField( lpps, lpStatBar, (WORD)(lpStatBar->wSuperIdx-1) );
  }
  else
  {
    for (i = 0; i < lpStatBar->nFields; i++)
    {
      if ((lpStatBar->wSuperIdx - 1) != i )
	SB_DrawField( lpps, lpStatBar, (WORD)i );
    }
  }

  GlobalUnlock( hStatBar );
}


static void SB_DrawField( LPPAINTSTRUCT lpps, LPSTATBAR lpStatBar, WORD wFieldIdx )
{
	COLORREF    lSaveColor;
	HBITMAP         hbmMem;
	HBITMAP         hbmOld;
	HBRUSH      hbrushFace;
	HDC                     hdcMem;
	LPSTATFIELD lpStatField;
	LPSTR       lpText;

	lpStatField = &lpStatBar->fieldList[wFieldIdx];

	if (lpStatField->bVisible == FALSE || (lpStatBar->bSuperFlag &&
		(wFieldIdx+1) != lpStatBar->wSuperIdx))
	return;

	hdcMem=CreateCompatibleDC(lpps->hdc);
	if (hdcMem)
	{
		// Make the bitmap the size of the status bar 
		hbmMem=CreateCompatibleBitmap(lpps->hdc,
			lpStatBar->boundRect.right - lpStatBar->boundRect.left,
			lpStatBar->boundRect.bottom- lpStatBar->boundRect.top);
		hbmOld=SelectObject(hdcMem, hbmMem);

		hbrushFace = CreateSolidBrush( lpStatField->lFaceColor );

		lpStatBar->hSavePen = SelectObject( hdcMem, lpStatBar->hpenFrame );
		lpStatBar->hSaveBrush = SelectObject( hdcMem, hbrushFace );

		SetBkMode(hdcMem, TRANSPARENT);

		if ((lpStatField->wType & SBT_FRAME)/* ||
		(lpStatField->wType & SBT_3D)*/)
		Rectangle( hdcMem, lpStatField->boundRect.left,
					lpStatField->boundRect.top,
					lpStatField->boundRect.right,
					lpStatField->boundRect.bottom );
		else
			FillRect(hdcMem, &lpStatField->boundRect, hbrushFace);

		if (lpStatField->wType & SBT_3D)
		{
		/* Draw highlight */
		SelectObject( hdcMem, lpStatBar->hpenLight );
		MoveToEx( hdcMem, lpStatField->boundRect.left+1,
				lpStatField->boundRect.bottom-1, NULL );
		LineTo( hdcMem, lpStatField->boundRect.right-1,
				lpStatField->boundRect.bottom-1 );
		LineTo( hdcMem, lpStatField->boundRect.right-1,
				lpStatField->boundRect.top+1 );

		/* Draw shadow */
		SelectObject( hdcMem, lpStatBar->hpenShadow );
		LineTo( hdcMem, lpStatField->boundRect.left+1,
				lpStatField->boundRect.top+1 );
		LineTo( hdcMem, lpStatField->boundRect.left+1,
				lpStatField->boundRect.bottom-1 );
		}

		SelectObject( hdcMem, lpStatBar->hSavePen );
		SelectObject( hdcMem, lpStatBar->hSaveBrush );

		/* Draw text */
		if (lpStatField->wTextOffset != 0)
		{
		lSaveColor = SetTextColor( hdcMem, lpStatField->lTextColor );

		if (lpStatField->hFont != NULL)
		lpStatBar->hSaveFont = SelectObject( hdcMem, lpStatField->hFont );

		lpText = &lpStatBar->szText[lpStatField->wTextOffset];

			/* Adjust text so that it is not right up against the border */
		if (((lpStatField->wType & SBT_3D) ||
				(lpStatField->wType & SBT_FRAME))&&
				!(lpStatField->wFormat & (DT_CENTER | DT_RIGHT)))
			{
				lpStatField->drawRect.left+=3;
				lpStatField->drawRect.right-=3;
			}

		DrawText( hdcMem, lpText, lstrlen( lpText ),
			&lpStatField->drawRect, lpStatField->wFormat );

		if (((lpStatField->wType & SBT_3D) ||
				(lpStatField->wType & SBT_FRAME))&&
				!(lpStatField->wFormat & (DT_CENTER | DT_RIGHT)))
			{
				lpStatField->drawRect.left-= 3;
				lpStatField->drawRect.right+=3;
			}

		//
		// Blit the changes to the screen dc
		//
		BitBlt(lpps->hdc,
				lpStatField->boundRect.left,
				lpStatField->boundRect.top,
				lpStatField->boundRect.right - lpStatField->boundRect.left,
				lpStatField->boundRect.bottom- lpStatField->boundRect.top,
			hdcMem,
			lpStatField->boundRect.left, lpStatField->boundRect.top,
//                      0, 0,
			SRCCOPY);

		if (lpStatField->hFont != NULL)
		SelectObject( hdcMem, lpStatBar->hSaveFont );

		SetTextColor( hdcMem, lSaveColor );
		}

		DeleteObject( hbrushFace );
	    SelectObject(hdcMem, hbmOld);
	DeleteObject(hbmMem);
	DeleteDC(hdcMem);
	}
}


static void DestroyStatBar( HWND hwnd )
{
  HANDLE      hStatBar;
  LPSTATBAR   lpStatBar;

  hStatBar = (HWND)GetWindowLong( hwnd, GWL_USERDATA );
  if (hStatBar == NULL)
    return;

  if ((lpStatBar = (LPSTATBAR) GlobalLock( hStatBar )) != NULL)
  {
    if (lpStatBar->hpenFrame != NULL)
    {
      DeleteObject( lpStatBar->hpenFrame );
      lpStatBar->hpenFrame = NULL;
    }

    if (lpStatBar->hpenLight != NULL)
    {
      DeleteObject( lpStatBar->hpenLight );
      lpStatBar->hpenLight = NULL;
    }

    if (lpStatBar->hpenShadow != NULL)
    {
      DeleteObject( lpStatBar->hpenShadow );
      lpStatBar->hpenShadow = NULL;
    }


    if (lpStatBar->hpenFace != NULL)
    {
      DeleteObject( lpStatBar->hpenFace );
      lpStatBar->hpenFace = NULL;
    }

    if (lpStatBar->hbrushFace != NULL)
    {
      DeleteObject( lpStatBar->hbrushFace );
      lpStatBar->hbrushFace = NULL;
    }

    if (lpStatBar->hbrushBgn != NULL)
    {
      DeleteObject( lpStatBar->hbrushBgn );
      lpStatBar->hbrushBgn = NULL;
    }

    GlobalUnlock( hStatBar );
  }

  GlobalFree( hStatBar );
  SetWindowLong( hwnd, GWL_USERDATA, 0 );
}


static void SB_SetField( HWND hwnd, WORD wID, LPSTR lpText )
{
  HANDLE      hStatBar;
  LPSTATBAR   lpStatBar;
  LPSTATFIELD lpStatField;
  UINT        i;
  LPSTR       lpTemp;
  RECT            rcFld;

  hStatBar = (HWND)GetWindowLong( hwnd, GWL_USERDATA);
  if (hStatBar == NULL)
    return;

  if ((lpStatBar = (LPSTATBAR) GlobalLock( hStatBar )) == NULL)
    return;

  for (i = 0, lpStatField = lpStatBar->fieldList;
       i < lpStatBar->nFields; i++, lpStatField++)
  {
    if (lpStatField->wID == wID)
      break;
  }

  if (i < lpStatBar->nFields)
  {
    lpTemp = &lpStatBar->szText[lpStatField->wTextOffset];
    if (x_strcmp( lpTemp, lpText ) != 0)
    {
      x_strncpy( lpTemp, lpText, lpStatField->nText );
      *(lpTemp+lpStatField->nText) = '\0';
	  
	  /* New code - Set the rectangle and invalidate the new rectangle */
	  SetRect(&rcFld, lpStatField->drawRect.left+2,
		lpStatField->drawRect.top+2,
		lpStatField->drawRect.right-2,
		lpStatField->drawRect.bottom-2);
      InvalidateRect( hwnd, &lpStatField->drawRect, FALSE );
      //InvalidateRect( hwnd, &lpStatField->drawRect, FALSE );
      UpdateWindow( hwnd );
    }
  }

  GlobalUnlock( hStatBar );
}


static void SB_SetMode( HWND hwnd, WORD wMode, LONG lParam )
{
  HANDLE      hStatBar;
  LPSTATBAR   lpStatBar;
  RECT        clientRect;

  hStatBar = (HWND)GetWindowLong( hwnd, GWL_USERDATA );
  if (hStatBar == NULL)
    return;

  if ((lpStatBar = (LPSTATBAR) GlobalLock( hStatBar )) == NULL)
    return;

  if (lpStatBar->bSuperFlag != (BOOL)wMode)
  {
    lpStatBar->bSuperFlag = (BOOL)wMode;
    GetClientRect( hwnd, &clientRect );
    InvalidateRect( hwnd, &clientRect, TRUE );
    UpdateWindow( hwnd );
  }

  GlobalUnlock( hStatBar );
}



static void PASCAL SB_ShowField( HWND hwnd, WORD wID, LONG lShowHide )
{
  UINT        i;
  HANDLE      hStatBar;
  LPSTATBAR   lpStatBar;
  LPSTATFIELD lpStatField;

  hStatBar = (HWND)GetWindowLong( hwnd, GWL_USERDATA );
  if (hStatBar == NULL)
    return;

  if ((lpStatBar = (LPSTATBAR) GlobalLock( hStatBar )) == NULL)
    return;

  for (i = 0, lpStatField = lpStatBar->fieldList;
       i < lpStatBar->nFields; i++, lpStatField++)
  {
    if (lpStatField->wID == wID)
      break;
  }

  if (i < lpStatBar->nFields)
  {
    lpStatField->bVisible = (lShowHide != 0);
    InvalidateRect( hwnd, &lpStatField->boundRect, TRUE );
    UpdateWindow( hwnd );
  }

  GlobalUnlock( hStatBar );
}



static void PASCAL SB_SizeField( HWND hwnd, WORD wID, LPRECT lpRect )
{
  UINT        i;
  HANDLE      hStatBar;
  LPSTATBAR   lpStatBar;
  LPSTATFIELD lpStatField;
  RECT        oldRect;

  hStatBar = (HWND)GetWindowLong( hwnd, GWL_USERDATA );
  if (hStatBar == NULL)
    return;

  if ((lpStatBar = (LPSTATBAR) GlobalLock( hStatBar )) == NULL)
    return;

  for (i = 0, lpStatField = lpStatBar->fieldList;
       i < lpStatBar->nFields; i++, lpStatField++)
  {
    if (lpStatField->wID == wID)
      break;
  }

  if (i < lpStatBar->nFields)
  {
    oldRect = lpStatField->boundRect;
    lpStatField->boundRect = *lpRect;
    lpStatField->drawRect  = lpStatField->boundRect;
    if ((lpStatField->wType & SBT_3D) || (lpStatField->wType & SBT_FRAME))
      InsetRect( &lpStatField->drawRect, 1, 1);
	//
	// RPB - 12/29/93
	//
	// If we only painted the areas that are effected by the 
	// invalidaterect call, we would need to invalidate both the
	// old and the new rectanges.  But, since we redraw the entire 
	// line on each call, we can get by with only one call.
	//
    //InvalidateRect( hwnd, &oldRect, TRUE );
    InvalidateRect( hwnd, &lpStatField->boundRect, TRUE );
    UpdateWindow( hwnd );
  }

  GlobalUnlock( hStatBar );
}



static void PASCAL SB_SetBarColor( HWND hwnd, COLORREF lColor )
{
  UINT        i;
  HANDLE      hStatBar;
  LPSTATBAR   lpStatBar;
  LPSTATFIELD lpStatField;
  RECT        clientRect;

  hStatBar = (HWND)GetWindowLong( hwnd, GWL_USERDATA );
  if (hStatBar == NULL)
    return;

  if ((lpStatBar = (LPSTATBAR) GlobalLock( hStatBar )) == NULL)
    return;

  if (lpStatBar->hbrushBgn)
    DeleteObject( lpStatBar->hbrushBgn );

  lpStatBar->lBkgnColor = lColor;
  lpStatBar->hbrushBgn = CreateSolidBrush( lpStatBar->lBkgnColor );

  GetClientRect( hwnd, &clientRect );
  InvalidateRect( hwnd, &clientRect, TRUE );

  for (i = 0, lpStatField = lpStatBar->fieldList;
       i < lpStatBar->nFields; i++, lpStatField++)
  {
    ValidateRect( hwnd, &lpStatField->boundRect );
  }

  GlobalUnlock( hStatBar );
  UpdateWindow( hwnd );
}



static void PASCAL SB_SetFieldColor( HWND hwnd, WORD wID, COLORREF lColor, WORD wWhat )
{
  UINT        i;
  HANDLE      hStatBar;
  LPSTATBAR   lpStatBar;
  LPSTATFIELD lpStatField;

  hStatBar = (HWND)GetWindowLong( hwnd, GWL_USERDATA );
  if (hStatBar == NULL)
    return;

  if ((lpStatBar = (LPSTATBAR) GlobalLock( hStatBar )) == NULL)
    return;

  if (wID == 0xFFFF)
  {
    for (i = 0, lpStatField = lpStatBar->fieldList;
	 i < lpStatBar->nFields; i++, lpStatField++)
    {
      if (wWhat == 1)
      {
	lpStatField->lTextColor = lColor;
	InvalidateRect( hwnd, &lpStatField->drawRect, FALSE );
      }
      else
      {
	lpStatField->lFaceColor = lColor;
	InvalidateRect( hwnd, &lpStatField->boundRect, FALSE );
      }
    }
  }
  else
  {
    for (i = 0, lpStatField = lpStatBar->fieldList;
	 i < lpStatBar->nFields; i++, lpStatField++)
    {
      if (lpStatField->wID == wID)
	break;
    }

    if (i < lpStatBar->nFields)
    {
      if (wWhat == 1)
      {
	lpStatField->lTextColor = lColor;
	InvalidateRect( hwnd, &lpStatField->drawRect, FALSE );
      }
      else
      {
	lpStatField->lFaceColor = lColor;
	InvalidateRect( hwnd, &lpStatField->boundRect, FALSE );
      }
    }
  }

  GlobalUnlock( hStatBar );
  UpdateWindow( hwnd );
}


static void PASCAL SB_Size(HWND hWnd, UINT state, int cx, int cy)
{
  HANDLE      hStatBar;
  LPSTATBAR   lpStatBar;

  hStatBar = (HWND)GetWindowLong( hWnd, GWL_USERDATA );
  if (hStatBar == NULL)
    return;

  if ((lpStatBar = (LPSTATBAR) GlobalLock( hStatBar )) == NULL)
    return;

  lpStatBar->boundRect.right=cx;
  lpStatBar->boundRect.bottom=cy;

  GlobalUnlock( hStatBar );
}

