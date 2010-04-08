/****************************************************************************
 * Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * CVNAME.C
 *
 * Contains message handler for the NAME view format.
 ****************************************************************************/

#include <windows.h>
#include <windowsx.h>  // Need this for the memory macros
#include <stdlib.h>
#include <malloc.h>
#include "cntdll.h"
#include "cntdrag.h"

#define ASYNC_KEY_DOWN      0x8000   // flag indicating key is pressed

// Prototypes for Name view msg cracker functions.
BOOL Nam_OnNCCreate( HWND hWnd, LPCREATESTRUCT lpCreateStruct );
BOOL Nam_OnCreate( HWND hWnd, LPCREATESTRUCT lpCreateStruct );
VOID Nam_OnCommand( HWND hWnd, int id, HWND hwndCtl, UINT codeNotify );
VOID Nam_OnSize( HWND hwnd, UINT state, int cx, int cy );
VOID Nam_OnSetFont( HWND hWnd, HFONT hfont, BOOL fRedraw );
VOID Nam_OnChildActivate( HWND hWnd );
VOID Nam_OnDestroy( HWND hWnd );
BOOL Nam_OnEraseBkgnd( HWND hWnd, HDC hDC );
VOID Nam_OnPaint( HWND hWnd );
VOID Nam_OnEnable( HWND hWnd, BOOL fEnable );
VOID Nam_OnSetFocus( HWND hWnd, HWND hwndOldFocus );
VOID Nam_OnKillFocus( HWND hWnd, HWND hwndNewFocus );
VOID Nam_OnShowWindow( HWND hWnd, BOOL fShow, UINT status );
VOID Nam_OnCancelMode( HWND hWnd );
VOID Nam_OnVScroll( HWND hWnd, HWND hwndCtl, UINT code, int pos );
VOID Nam_OnHScroll( HWND hWnd, HWND hwndCtl, UINT code, int pos );
VOID Nam_OnChar( HWND hWnd, UINT ch, int cRepeat );
VOID Nam_OnKey( HWND hWnd, UINT vk, BOOL fDown, int cRepeat, UINT flags );
VOID Nam_OnLButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags );
VOID Nam_OnLButtonUp( HWND hWnd, int x, int y, UINT keyFlags );
VOID Nam_OnMouseMove( HWND hWnd, int x, int y, UINT keyFlags );
VOID Nam_OnRButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags );
UINT Nam_OnGetDlgCode( HWND hWnd, LPMSG lpmsg );
VOID Nam_OnNCLButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest );
VOID Nam_OnNCMButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest );
VOID Nam_OnNCRButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest );

/****************************************************************************
 * NameViewWndProc
 *
 * Purpose:
 *  Handles all window messages for a container in NAME view format.
 *  Any message not processed here should go to DefWindowProc.
 *
 * Parameters:
 *  Standard for WndProc
 *
 * Return Value:
 *  LRESULT       Varies with the message.
 ****************************************************************************/

LRESULT WINAPI NameViewWndProc( HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam )
{
    switch( iMsg )
    {
      HANDLE_MSG( hWnd, WM_NCCREATE,       Nam_OnNCCreate      );
      HANDLE_MSG( hWnd, WM_CREATE,         Nam_OnCreate        );
      HANDLE_MSG( hWnd, WM_COMMAND,        Nam_OnCommand       );
      HANDLE_MSG( hWnd, WM_SIZE,           Nam_OnSize          );
      HANDLE_MSG( hWnd, WM_SETFONT,        Nam_OnSetFont       );
      HANDLE_MSG( hWnd, WM_CHILDACTIVATE,  Nam_OnChildActivate );
      HANDLE_MSG( hWnd, WM_DESTROY,        Nam_OnDestroy       );
      HANDLE_MSG( hWnd, WM_ERASEBKGND,     Nam_OnEraseBkgnd    );
      HANDLE_MSG( hWnd, WM_PAINT,          Nam_OnPaint         );
      HANDLE_MSG( hWnd, WM_ENABLE,         Nam_OnEnable        );
      HANDLE_MSG( hWnd, WM_SETFOCUS,       Nam_OnSetFocus      );
      HANDLE_MSG( hWnd, WM_KILLFOCUS,      Nam_OnKillFocus     );
      HANDLE_MSG( hWnd, WM_SHOWWINDOW,     Nam_OnShowWindow    );
      HANDLE_MSG( hWnd, WM_CANCELMODE,     Nam_OnCancelMode    );
      HANDLE_MSG( hWnd, WM_VSCROLL,        Nam_OnVScroll       );
      HANDLE_MSG( hWnd, WM_HSCROLL,        Nam_OnHScroll       );
      HANDLE_MSG( hWnd, WM_CHAR,           Nam_OnChar          );
      HANDLE_MSG( hWnd, WM_KEYDOWN,        Nam_OnKey           );
      HANDLE_MSG( hWnd, WM_RBUTTONDBLCLK,  Nam_OnRButtonDown   );
      HANDLE_MSG( hWnd, WM_RBUTTONDOWN,    Nam_OnRButtonDown   );
      HANDLE_MSG( hWnd, WM_GETDLGCODE,     Nam_OnGetDlgCode    );
      HANDLE_MSG( hWnd, WM_LBUTTONDBLCLK,  Nam_OnLButtonDown   );
      HANDLE_MSG( hWnd, WM_LBUTTONDOWN,    Nam_OnLButtonDown   );
      HANDLE_MSG( hWnd, WM_LBUTTONUP,      Nam_OnLButtonUp     );
      HANDLE_MSG( hWnd, WM_MOUSEMOVE,      Nam_OnMouseMove     );
      HANDLE_MSG( hWnd, WM_NCLBUTTONDOWN,  Nam_OnNCLButtonDown );
      HANDLE_MSG( hWnd, WM_NCMBUTTONDOWN,  Nam_OnNCMButtonDown );
      HANDLE_MSG( hWnd, WM_NCRBUTTONDOWN,  Nam_OnNCRButtonDown );

      default:
        return (Process_DefaultMsg(hWnd,iMsg,wParam,lParam));
    }

    return 0L;
}

/****************************************************************************
 * NameViewWndProc Message Cracker Callback Functions
 *
 * Purpose:
 *  Functions to process each message received by the Container WndProc.
 *  This is a Win16/Win32 portable method for message handling.
 *
 * Return Value:
 *  Varies according to the message being processed.
 ****************************************************************************/
BOOL Nam_OnNCCreate( HWND hWnd, LPCREATESTRUCT lpCreateStruct )
{
    LPCONTAINER  lpCntNew, lpCntOld;

    // Get the existing container struct being passed in the create params.
    lpCntOld = (LPCONTAINER) lpCreateStruct->lpCreateParams;

    // If we are creating a split child alloc a new struct and load it.
    if( StateTest( lpCntOld, CNTSTATE_CREATINGSB ) )
    {
      lpCntNew = (LPCONTAINER) calloc( 1, LEN_CONTAINER );
      if( lpCntNew == NULL )
        return 0L;

      // Save the container pointer in the window extra bytes.
      SetWindowLong( hWnd, GWL_CONTAINERHMEM, (LONG)lpCntNew );

      // Load up the new container struct with the already existing
      // info that is in the passed in container struct.
      *lpCntNew = *lpCntOld;

      // Flag that the container is now in a split state.
      lpCntNew->bIsSplit = TRUE;    

      // Clear out all initial states.
      StateClear( lpCntNew, CNTSTATE_ALL );

      // If the 1st child is deferring paints, set it for the split wnd.
      if( StateTest( lpCntOld, CNTSTATE_DEFERPAINT ) )
        StateSet( lpCntNew, CNTSTATE_DEFERPAINT );
    }
    else
    {
      // Do not alloc a new container struct since we are creating the
      // 1st child and it shares the same one as the frame window.
      // All other split children alloc their own container structs.

      // Save the old container pointer in the window extra bytes.
      SetWindowLong( hWnd, GWL_CONTAINERHMEM, (LONG)lpCntOld );
    }
 
    return FORWARD_WM_NCCREATE( hWnd, lpCreateStruct, CNT_DEFWNDPROC );
}

BOOL Nam_OnCreate( HWND hWnd, LPCREATESTRUCT lpCreateStruct )
{
    // same as Text View
    Txt_OnCreate( hWnd, lpCreateStruct );

    return TRUE;
}

VOID Nam_OnCommand( HWND hWnd, int id, HWND hwndCtl, UINT codeNotify )
{
    // same as detailed view
    Dtl_OnCommand( hWnd, id, hwndCtl, codeNotify );
    
    return;
}

VOID Nam_OnSize( HWND hWnd, UINT state, int cx, int cy )
{
    // same as Text View
    Txt_OnSize( hWnd, state, cx, cy );
    
    return;
}  

VOID Nam_OnSetFont( HWND hWnd, HFONT hfont, BOOL fRedraw )
{
    // same as Text View
    Txt_OnSetFont( hWnd, hfont, fRedraw );

    return;
}  

VOID Nam_OnChildActivate( HWND hWnd )
{
    // same as Detailed View
    Dtl_OnChildActivate( hWnd );

    return;
}  

VOID Nam_OnDestroy( HWND hWnd )
{
    // same as Detailed View
    Dtl_OnDestroy( hWnd );

    return;
}  

BOOL Nam_OnEraseBkgnd( HWND hWnd, HDC hDC )
{
   /*-------------------------------------------------------------------*
    * Eat this msg to avoid erasing portions that will be repainted
    * in WM_PAINT.  Part of a change-state-and-repaint strategy is to
    * rely on WM_PAINT to do anything visual, which includes erasing
    * invalid portions. Letting WM_ERASEBKGND erase bkgrnd is redundant.
    *-------------------------------------------------------------------*/

    // Uncommenting this line will let Windows repaint the background.
//    return FORWARD_WM_ERASEBKGND( hWnd, hDC, CNT_DEFWNDPROC );
    return TRUE;
}  

VOID Nam_OnPaint( HWND hWnd )
{
    LPCONTAINER  lpCnt = GETCNT(hWnd);
    PAINTSTRUCT ps;
    HDC         hDC;

    hDC = BeginPaint( hWnd, &ps );
    
    PaintNameView( hWnd, lpCnt, hDC, (LPPAINTSTRUCT)&ps );
    
    EndPaint( hWnd, &ps );

    return;
}  

VOID Nam_OnEnable( HWND hWnd, BOOL fEnable )
{
    // same as Detailed View
    Dtl_OnEnable( hWnd, fEnable );

    return;
}  

VOID Nam_OnSetFocus( HWND hWnd, HWND hwndOldFocus )
{
    // same as Text View
    Txt_OnSetFocus( hWnd, hwndOldFocus );

    return;
}  

VOID Nam_OnKillFocus( HWND hWnd, HWND hwndNewFocus )
{
    // same as Text View
    Txt_OnKillFocus( hWnd, hwndNewFocus );

    return;
}  

VOID Nam_OnShowWindow( HWND hWnd, BOOL fShow, UINT status )
{
    // same as Detailed View
    Dtl_OnShowWindow( hWnd, fShow, status );

    return;
}  

VOID Nam_OnCancelMode( HWND hWnd )
{
    // same as Detailed view
    Dtl_OnCancelMode( hWnd );

    return;
}  

VOID Nam_OnVScroll( HWND hWnd, HWND hwndCtl, UINT code, int pos )
{
    // same as Text View
    Txt_OnVScroll( hWnd, hwndCtl, code, pos );
    
    return;
}  

VOID Nam_OnHScroll( HWND hWnd, HWND hwndCtl, UINT code, int pos )
{
    // same as Text View
    Txt_OnHScroll( hWnd, hwndCtl, code, pos );

    return;
}  

VOID Nam_OnChar( HWND hWnd, UINT ch, int cRepeat )
{
    // same as Text View
    Txt_OnChar( hWnd, ch, cRepeat );
    return;
}  

VOID Nam_OnKey( HWND hWnd, UINT vk, BOOL fDown, int cRepeat, UINT flags )
{
    // same as Text View
    Txt_OnKey( hWnd, vk, fDown, cRepeat, flags );

    return;
}

VOID Nam_OnRButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags )
{
    // same as Text View
    Txt_OnRButtonDown( hWnd, fDoubleClick, x, y, keyFlags );

    return;
}

VOID Nam_OnLButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags )
{
    // same as Text View
    Txt_OnLButtonDown( hWnd, fDoubleClick, x, y, keyFlags );

    return;
}  

VOID Nam_OnLButtonUp( HWND hWnd, int x, int y, UINT keyFlags )
{
    // same as Text View
    Txt_OnLButtonUp( hWnd, x, y, keyFlags );

    return;
}  

UINT Nam_OnGetDlgCode( HWND hWnd, LPMSG lpmsg )
{
    UINT       uRet;
    
    // same as Detailed View
    uRet = Dtl_OnGetDlgCode( hWnd, lpmsg );

    return uRet;
}

VOID Nam_OnMouseMove( HWND hWnd, int x, int y, UINT keyFlags )
{
    // same as Text View
    Txt_OnMouseMove( hWnd, x, y, keyFlags );

    return;
}

VOID Nam_OnNCLButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest )
{
    // same as Text view
    Txt_OnNCLButtonDown( hWnd, fDoubleClick, x, y, codeHitTest );

    return;
}

VOID Nam_OnNCMButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest )
{
    // same as Text view
    Txt_OnNCMButtonDown( hWnd, fDoubleClick, x, y, codeHitTest );

    return;
}

VOID Nam_OnNCRButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest )
{
    // same as Text view
    Txt_OnNCRButtonDown( hWnd, fDoubleClick, x, y, codeHitTest );

    return;
}
