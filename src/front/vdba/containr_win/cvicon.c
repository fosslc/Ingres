/****************************************************************************
 * Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * CVICON.C
 *
 * Contains message handler for the ICON view format.
 *
 * History:
 *	14-Feb-2000 (somsa01)
 *	    Corrected arguments to LLRemoveTail() and LLRemoveHead().
 *  05-Feb-2004 (noifr01)
 *      error in apparently unused code, could cause infinite loop if it was
 *      used (detected through a build warning)
 ****************************************************************************/

#include <windows.h>
#include <windowsx.h>  // Need this for the memory macros
#include <stdlib.h>
#include <malloc.h>
#include "cntdll.h"
#include "cntdrag.h"

#define ASYNC_KEY_DOWN      0x8000   // flag indicating key is pressed

// Prototypes for local functions.
VOID NEAR         SwipeSelection( HWND hWnd, LPCONTAINER lpCnt );
VOID              DoIconMove( HWND hWnd, LPCONTAINER lpCnt, POINT pt, LPRECORDCORE lpRec );
BOOL              SelectBitmapOrIcon( LPCONTAINER lpCnt, LPRECORDCORE lpRec, HBITMAP hbmPrev,
                                      HDC hdcMem, COLORREF *rgbBk );
VOID DrawMovingBitmap( HWND hWnd, HDC hDC, LPCONTAINER lpCnt, LPRECORDCORE lpRec, HBITMAP *hbmBkg,
                       int dx, int dy );
VOID DrawMovingIcon( HWND hWnd, HDC hDC, LPCONTAINER lpCnt, LPRECORDCORE lpRec, HBITMAP *hbmBkg,
                     int dx, int dy );
VOID DrawIconForMove( HDC hDC, LPCONTAINER lpCnt, LPRECORDCORE lpRec, int dx, int dy );

// Prototypes for Icon view msg cracker functions.
BOOL Icn_OnNCCreate( HWND hWnd, LPCREATESTRUCT lpCreateStruct );
BOOL Icn_OnCreate( HWND hWnd, LPCREATESTRUCT lpCreateStruct );
VOID Icn_OnCommand( HWND hWnd, int id, HWND hwndCtl, UINT codeNotify );
VOID Icn_OnSize( HWND hwnd, UINT state, int cx, int cy );
VOID Icn_OnSetFont( HWND hWnd, HFONT hfont, BOOL fRedraw );
VOID Icn_OnChildActivate( HWND hWnd );
VOID Icn_OnDestroy( HWND hWnd );
BOOL Icn_OnEraseBkgnd( HWND hWnd, HDC hDC );
VOID Icn_OnPaint( HWND hWnd );
VOID Icn_OnEnable( HWND hWnd, BOOL fEnable );
VOID Icn_OnSetFocus( HWND hWnd, HWND hwndOldFocus );
VOID Icn_OnKillFocus( HWND hWnd, HWND hwndNewFocus );
VOID Icn_OnShowWindow( HWND hWnd, BOOL fShow, UINT status );
VOID Icn_OnCancelMode( HWND hWnd );
VOID Icn_OnVScroll( HWND hWnd, HWND hwndCtl, UINT code, int pos );
VOID Icn_OnHScroll( HWND hWnd, HWND hwndCtl, UINT code, int pos );
VOID Icn_OnChar( HWND hWnd, UINT ch, int cRepeat );
VOID Icn_OnKey( HWND hWnd, UINT vk, BOOL fDown, int cRepeat, UINT flags );
VOID Icn_OnRButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags );
UINT Icn_OnGetDlgCode( HWND hWnd, LPMSG lpmsg );
VOID Icn_OnLButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags );
VOID Icn_OnLButtonUp( HWND hWnd, int x, int y, UINT keyFlags );
VOID Icn_OnMouseMove( HWND hWnd, int x, int y, UINT keyFlags );
VOID Icn_OnNCLButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest );
VOID Icn_OnNCMButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest );
VOID Icn_OnNCRButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest );

/****************************************************************************
 * IconViewWndProc
 *
 * Purpose:
 *  Handles all window messages for a container in ICON view format.
 *  Any message not processed here should go to DefWindowProc.
 *
 * Parameters:
 *  Standard for WndProc
 *
 * Return Value:
 *  LRESULT       Varies with the message.
 ****************************************************************************/

LRESULT WINAPI IconViewWndProc( HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam )
{
    switch( iMsg )
    {
      HANDLE_MSG( hWnd, WM_NCCREATE,       Icn_OnNCCreate      );
      HANDLE_MSG( hWnd, WM_CREATE,         Icn_OnCreate        );
      HANDLE_MSG( hWnd, WM_COMMAND,        Icn_OnCommand       );
      HANDLE_MSG( hWnd, WM_SIZE,           Icn_OnSize          );
      HANDLE_MSG( hWnd, WM_CHILDACTIVATE,  Icn_OnChildActivate );
      HANDLE_MSG( hWnd, WM_DESTROY,        Icn_OnDestroy       );
      HANDLE_MSG( hWnd, WM_ERASEBKGND,     Icn_OnEraseBkgnd    );
      HANDLE_MSG( hWnd, WM_SETFONT,        Icn_OnSetFont       );
      HANDLE_MSG( hWnd, WM_PAINT,          Icn_OnPaint         );
      HANDLE_MSG( hWnd, WM_ENABLE,         Icn_OnEnable        );
      HANDLE_MSG( hWnd, WM_SETFOCUS,       Icn_OnSetFocus      );
      HANDLE_MSG( hWnd, WM_KILLFOCUS,      Icn_OnKillFocus     );
      HANDLE_MSG( hWnd, WM_SHOWWINDOW,     Icn_OnShowWindow    );
      HANDLE_MSG( hWnd, WM_CANCELMODE,     Icn_OnCancelMode    );
      HANDLE_MSG( hWnd, WM_VSCROLL,        Icn_OnVScroll       );
      HANDLE_MSG( hWnd, WM_HSCROLL,        Icn_OnHScroll       );
      HANDLE_MSG( hWnd, WM_CHAR,           Icn_OnChar          );
      HANDLE_MSG( hWnd, WM_KEYDOWN,        Icn_OnKey           );
      HANDLE_MSG( hWnd, WM_RBUTTONDBLCLK,  Icn_OnRButtonDown   );
      HANDLE_MSG( hWnd, WM_RBUTTONDOWN,    Icn_OnRButtonDown   );
      HANDLE_MSG( hWnd, WM_GETDLGCODE,     Icn_OnGetDlgCode    );
      HANDLE_MSG( hWnd, WM_LBUTTONDBLCLK,  Icn_OnLButtonDown   );
      HANDLE_MSG( hWnd, WM_LBUTTONDOWN,    Icn_OnLButtonDown   );
      HANDLE_MSG( hWnd, WM_LBUTTONUP,      Icn_OnLButtonUp     );
      HANDLE_MSG( hWnd, WM_MOUSEMOVE,      Icn_OnMouseMove     );
      HANDLE_MSG( hWnd, WM_NCLBUTTONDOWN,  Icn_OnNCLButtonDown );
      HANDLE_MSG( hWnd, WM_NCMBUTTONDOWN,  Icn_OnNCMButtonDown );
      HANDLE_MSG( hWnd, WM_NCRBUTTONDOWN,  Icn_OnNCRButtonDown );

      default:
        return (Process_DefaultMsg(hWnd,iMsg,wParam,lParam));
    }

    return 0L;
}

/****************************************************************************
 * IconViewWndProc Message Cracker Callback Functions
 *
 * Purpose:
 *  Functions to process each message received by the Container WndProc.
 *  This is a Win16/Win32 portable method for message handling.
 *
 * Return Value:
 *  Varies according to the message being processed.
 ****************************************************************************/
BOOL Icn_OnNCCreate( HWND hWnd, LPCREATESTRUCT lpCreateStruct )
{
    LPCONTAINER  lpCntNew, lpCntOld;

    // Get the existing container struct being passed in the create params.
    lpCntOld = (LPCONTAINER) lpCreateStruct->lpCreateParams;

    // If we are creating a split child alloc a new struct and load it.
    // actually, this should never happen but I'll leave it here in case
    // in the future, they wish to add split text views (I don't know why they
    // would but you never know...)
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

BOOL Icn_OnCreate( HWND hWnd, LPCREATESTRUCT lpCreateStruct )
{
    // same as Text View
    Txt_OnCreate( hWnd, lpCreateStruct );

    // NOTE: When using a msg cracker for WM_CREATE we must
    //       return TRUE if all is well instead of 0.
    return TRUE;
}

VOID Icn_OnCommand( HWND hWnd, int id, HWND hwndCtl, UINT codeNotify )
{
    // same as detailed view
    Dtl_OnCommand( hWnd, id, hwndCtl, codeNotify );
    
    return;
}

VOID Icn_OnSize( HWND hWnd, UINT state, int cx, int cy )
{
    LPCONTAINER lpCnt = GETCNT(hWnd);
    RECT rect;
    BOOL bHasVscroll, bHasHscroll, bRepaint, bInvalidate = FALSE;
    int  cxOldClient, nOldHscrollPos, nOldVscrollPos;
    int  cxClient, cyClient;

    if( !lpCnt )
      return;

    // Remember the old client width. If this changes we need to repaint
    // the title area under certain conditions.
    cxOldClient = lpCnt->cxClient;

    // Remember the old vert scroll position. If this changes we need to
    // repaint the entire field area. This can happen if the vert position
    // is at or near the max and the user sizes the container higher.
    nOldVscrollPos = lpCnt->nVscrollPos;

    // Remember the old horz scroll position. If this changes we need to
    // repaint the entire field area. This can happen if the horz position
    // is at or near the max and the user sizes the container wider.
    nOldHscrollPos = lpCnt->nHscrollPos;

    // remember the new container client area
    cxClient  = lpCnt->cxClient = cx;
    cyClient  = lpCnt->cyClient = cy;

    // set the size of the viewspace
    lpCnt->lpCI->rcViewSpace.right  = lpCnt->lpCI->rcViewSpace.left + cx;
    lpCnt->lpCI->rcViewSpace.bottom = lpCnt->lpCI->rcViewSpace.top  + cy - lpCnt->lpCI->cyTitleArea;
    
    bHasVscroll =  lpCnt->dwStyle & CTS_VERTSCROLL ? TRUE : FALSE;
    bHasHscroll =  lpCnt->dwStyle & CTS_HORZSCROLL ? TRUE : FALSE;

    if( bHasHscroll )
    {
      IcnAdjustHorzScroll( hWnd, lpCnt );
    }

    // Set up the vertical scrollbar.
    if( bHasVscroll ) 
    {
      AdjustVertScroll( hWnd, lpCnt, TRUE );
    }

    // New repaint code necessary because of removing redraw-on-size styles.
    // We must repaint the title area under certain conditions if the client
    // width is changing. If items are centered or right justified in the title
    // area they must be realigned according to the new client width.
    if( lpCnt->lpCI->cyTitleArea && cxOldClient != lpCnt->cxClient )
    {
      bRepaint = FALSE;
    
      // Must repaint title area if any of the following 3 are true:
      //   - There is title string that is not left justified
      //   - There is title button that is not left justified
      //   - There is title bitmap that is not left justified
      if( lpCnt->lpCI->lpszTitle && !(lpCnt->lpCI->flTitleAlign & CA_TA_LEFT) )
        bRepaint = TRUE;
      if( lpCnt->lpCI->nTtlBtnWidth && lpCnt->lpCI->bTtlBtnAlignRt )
        bRepaint = TRUE;
      if( lpCnt->lpCI->hBmpTtl && !(lpCnt->lpCI->flTtlBmpAlign & CA_TA_LEFT) )
        bRepaint = TRUE;
    
      if( bRepaint )
      {
        rect.left   = 0;
        rect.top    = 0;
        rect.right  = cx;
        rect.bottom = lpCnt->lpCI->cyTitleArea;
        InvalidateRect( hWnd, &rect, FALSE );
      }
    }

    // New repaint code necessary because of removing redraw-on-size styles.
    // We must repaint the entire record area if the vertical scroll 
    // position changed.
    if( nOldVscrollPos != lpCnt->nVscrollPos )
    {
      // Calc the rect for the record area.
      rect.left   = 0;
      rect.top    = lpCnt->lpCI->cyTitleArea;
      rect.right  = cx;
      rect.bottom = cy;
      InvalidateRect( hWnd, &rect, FALSE );
      bInvalidate = TRUE;
    }

    // New repaint code necessary because of removing redraw-on-size styles.
    // We must repaint the entire record area if the horizontal scroll 
    // position changed.
    if( nOldHscrollPos != lpCnt->nHscrollPos && !bInvalidate )
    {
      // Calc the rect for the fld title area and the record area.
      rect.left   = 0;
      rect.top    = lpCnt->lpCI->cyTitleArea;
      rect.right  = cx;
      rect.bottom = cy;
      InvalidateRect( hWnd, &rect, FALSE );
    }

    return;
}  

VOID Icn_OnSetFont( HWND hWnd, HFONT hfont, BOOL fRedraw )
{
    // same as text view
    Txt_OnSetFont( hWnd, hfont, fRedraw );

    return;
}  

VOID Icn_OnChildActivate( HWND hWnd )
{
    // same as Detialed View
    Dtl_OnChildActivate( hWnd );

    return;
}  

VOID Icn_OnDestroy( HWND hWnd )
{
    // same as Detialed View
    Dtl_OnDestroy( hWnd );

    return;
}  

BOOL Icn_OnEraseBkgnd( HWND hWnd, HDC hDC )
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

VOID Icn_OnPaint( HWND hWnd )
{
    LPCONTAINER  lpCnt = GETCNT(hWnd);
    PAINTSTRUCT ps;
    HDC         hDC;

    hDC = BeginPaint( hWnd, &ps );
    
    PaintIconView( hWnd, lpCnt, hDC, (LPPAINTSTRUCT)&ps );
    
    EndPaint( hWnd, &ps );

    return;
}  

VOID Icn_OnEnable( HWND hWnd, BOOL fEnable )
{
    // same as Detailed View
    Dtl_OnEnable( hWnd, fEnable );

    return;
}  

VOID Icn_OnSetFocus( HWND hWnd, HWND hwndOldFocus )
{
    // same as text view
    Txt_OnSetFocus( hWnd, hwndOldFocus );

    return;
}  

VOID Icn_OnKillFocus( HWND hWnd, HWND hwndNewFocus )
{
    // same as text view
    Txt_OnKillFocus( hWnd, hwndNewFocus );

    return;
}  

VOID Icn_OnShowWindow( HWND hWnd, BOOL fShow, UINT status )
{
    // same as Detailed View
    Dtl_OnShowWindow( hWnd, fShow, status );

    return;
}  

VOID Icn_OnCancelMode( HWND hWnd )
{
    // same as Detailed view
    Dtl_OnCancelMode( hWnd );

    return;
}  

VOID Icn_OnVScroll( HWND hWnd, HWND hwndCtl, UINT code, int pos )
{
    LPCONTAINER lpCnt = GETCNT(hWnd);
    RECT       rect;
    UINT       wScrollEvent;
    BOOL       bSentQueryDelta=FALSE;
    int        nVscrollInc;

    switch( code )
    {
      case SB_TOP:
        nVscrollInc  = -lpCnt->nVscrollPos;
        if( lpCnt->lpCI->bFocusRecLocked )
          wScrollEvent = CN_LK_VS_TOP;
        else
          wScrollEvent = CN_VSCROLL_TOP;
        break;

      case SB_BOTTOM:
        nVscrollInc  = lpCnt->nVscrollMax - lpCnt->nVscrollPos;
        if( lpCnt->lpCI->bFocusRecLocked )
          wScrollEvent = CN_LK_VS_BOTTOM;
        else
          wScrollEvent = CN_VSCROLL_BOTTOM;
        break;

      case SB_LINEUP:
        // if icons have been arranged, scroll by the height of the arranged
        // parameters, otherwise scroll by the height of the icon
        if ( lpCnt->lpCI->bArranged )
          nVscrollInc  = -(lpCnt->lpCI->aiInfo.nHeight + lpCnt->lpCI->aiInfo.nVerSpacing);
        else
          nVscrollInc  = -lpCnt->lpCI->ptBmpOrIcon.y;

        if( lpCnt->lpCI->bFocusRecLocked )
          wScrollEvent = CN_LK_VS_LINEUP;
        else
          wScrollEvent = CN_VSCROLL_LINEUP;
        break;

      case SB_LINEDOWN:
        // if icons have been arranged, scroll by the height of the arranged
        // parameters, otherwise scroll by the height of the icon
        if ( lpCnt->lpCI->bArranged )
          nVscrollInc  = lpCnt->lpCI->aiInfo.nHeight + lpCnt->lpCI->aiInfo.nVerSpacing;
        else
          nVscrollInc  = lpCnt->lpCI->ptBmpOrIcon.y;

        if( lpCnt->lpCI->bFocusRecLocked )
          wScrollEvent = CN_LK_VS_LINEDOWN;
        else
          wScrollEvent = CN_VSCROLL_LINEDOWN;
        break;

      case SB_PAGEUP:
        nVscrollInc  = -(lpCnt->cyClient - lpCnt->lpCI->cyTitleArea);
        if( lpCnt->lpCI->bFocusRecLocked )
          wScrollEvent = CN_LK_VS_PAGEUP;
        else
          wScrollEvent = CN_VSCROLL_PAGEUP;
        break;

      case SB_PAGEDOWN:
        nVscrollInc  = lpCnt->cyClient - lpCnt->lpCI->cyTitleArea;
        if( lpCnt->lpCI->bFocusRecLocked )
          wScrollEvent = CN_LK_VS_PAGEDOWN;
        else
          wScrollEvent = CN_VSCROLL_PAGEDOWN;
        break;

      case SB_THUMBPOSITION:
        if( StateTest( lpCnt, CNTSTATE_VSCROLL32 ) )
        {
          // Do special processing for 32 bit scrolling ranges. We have
          // to do this for the SB_THUMBPOS (and SB_THUMBTRACK) msg because 
          // Windows only passes us a 16 bit value for the new position.
          // This means that if your scrolling maximum is greater than 32767
          // you will not get a valid position value in the msg. To support
          // ranges greater 32767 (32 bit) we have to calculate the position 
          // ourselves.

          // If there is a value in the hwndCtl parm it means we are
          // forwarding a SB_THUMBPOS msg to ourselves. I use the unused
          // (for the container) hwndCtl parm to pass the position value
          // to get around the 16 bit limitation. If hwndCtl is NULL then
          // we are getting a real scrolling msg and need to calculate the
          // new position. 
          if( hwndCtl )
            pos = (int) hwndCtl;  // this is really a position I am passing to myself
          else 
            pos = CalcVThumbpos( hWnd, lpCnt );
        }

        nVscrollInc  = pos - lpCnt->nVscrollPos;

        if( lpCnt->lpCI->bFocusRecLocked )
          wScrollEvent = CN_LK_VS_THUMBPOS;
        else
          wScrollEvent = CN_VSCROLL_THUMBPOS;
        break;

      case SB_THUMBTRACK:
        // Send the thumb tracking msg if the user wants it or actually
        // scroll if the dynamic scrolling is turned on.
//        if( lpCnt->lpCI->flCntAttr & CA_WANTVTHUMBTRK ||
//            lpCnt->lpCI->flCntAttr & CA_DYNAMICVSCROLL )
//        {
          // If we have a 32 bit range max, calc the thumb position.
          if( StateTest( lpCnt, CNTSTATE_VSCROLL32 ) )
          {
            pos = CalcVThumbpos( hWnd, lpCnt );
          }

          wScrollEvent = CN_VSCROLL_THUMBTRK;

          // If both WantVThumbtrk and DynmanicVScroll are enabled
          // the dynamic scroll will take precedence.
//          if( lpCnt->lpCI->flCntAttr & CA_DYNAMICVSCROLL )
//          {
            // Forget it if using ownervscroll or delta model. Also
            // forget it if the focus record is locked. It means they are
            // doing validation which cannot be done with dynamic scrolling.
            if( (lpCnt->lpCI->flCntAttr & CA_OWNERVSCROLL) || 
                 lpCnt->lpCI->bFocusRecLocked )
              return;

            nVscrollInc  = pos - lpCnt->nVscrollPos;
            break;
//          }
//          else
//          {
//            NotifyAssocWnd( hWnd, lpCnt, wScrollEvent, 0, NULL, NULL, pos, 0, 0, NULL );
//          }
//        }
        return;

      default:
        nVscrollInc = 0;
      }

    // Scroll the record area vertically (if owner vscroll not enabled).
    if( !(lpCnt->lpCI->flCntAttr & CA_OWNERVSCROLL) && !lpCnt->lpCI->bFocusRecLocked )
    {
      if( nVscrollInc = max(-lpCnt->nVscrollPos, min(nVscrollInc, lpCnt->nVscrollMax - lpCnt->nVscrollPos)) )
      {
        // Set the new vertical position.
        lpCnt->nVscrollPos += nVscrollInc;

        // reset the icon viewspace
        lpCnt->lpCI->rcViewSpace.top   += nVscrollInc;
        lpCnt->lpCI->rcViewSpace.bottom = lpCnt->lpCI->rcViewSpace.top + lpCnt->cyClient - lpCnt->lpCI->cyTitleArea;
        
        // Set the scrolling rect and execute the scroll.
        rect.left   = 0;
        rect.top    = lpCnt->lpCI->cyTitleArea;
        rect.right  = lpCnt->cxClient;
        rect.bottom = lpCnt->cyClient;
        ScrollWindow( hWnd, 0, -nVscrollInc, NULL, &rect );
                                                                                            
        // Now reposition the scroll thumb.
        SetScrollPos( hWnd, SB_VERT, lpCnt->nVscrollPos, TRUE );
  
        // set top record
        if (lpCnt->lpCI->lpRecOrderHead)
          lpCnt->lpCI->lpTopRec = lpCnt->lpCI->lpRecOrderHead->lpRec;
  
        UpdateWindow( hWnd );
      }
    }

    // Don't notify the assoc if dynamic scrolling is on and this was a thumbtrack 
    // scroll event.  Movement or always if owner vscroll is enabled.
    if( wScrollEvent != CN_VSCROLL_THUMBTRK )
    {
      // Notify the assoc of the scrolling event if there was
      // movement or always if owner vscroll is enabled.
      if( nVscrollInc || lpCnt->lpCI->flCntAttr & CA_OWNERVSCROLL )
        NotifyAssocWnd( hWnd, lpCnt, wScrollEvent, 0, NULL, NULL, nVscrollInc, 0, 0, NULL );
    }

    // check to see if scroll bar still visible - there are cases when all the icons
    // can scroll into view - when this happens, the scroll bar should disappear.
    // Since you cannot call SetScrollRange with a range of zero while processing
    // a scroll bar message (causes painting problems - this is doc'ed in Win32 docs)
    // I post a message in the queue at the end of the scroll to WM_SIZE.  WM_SIZE
    // adjusts the icon workspace and viewspace and checks for validity of scrollbars.
    if (lpCnt->lpCI->rcViewSpace.top    <= lpCnt->lpCI->rcWorkSpace.top &&
        lpCnt->lpCI->rcViewSpace.bottom >= lpCnt->lpCI->rcWorkSpace.bottom)
    {
      if (code==SB_ENDSCROLL)
        PostMessage( hWnd, WM_SIZE, 0, MAKELPARAM(lpCnt->cxClient, lpCnt->cyClient) );
    }

    return;
}  

VOID Icn_OnHScroll( HWND hWnd, HWND hwndCtl, UINT code, int pos )
{
    LPCONTAINER lpCnt = GETCNT(hWnd);
    RECT        rect;
    UINT        wScrollEvent;
    int         nHscrollInc;

    switch( code )
    {
      case SB_TOP:
        nHscrollInc  = -lpCnt->nHscrollPos;

        if( lpCnt->lpCI->bFocusRecLocked )
          wScrollEvent = CN_LK_HS_TOP;
        else
          wScrollEvent = CN_HSCROLL_TOP;
        break;

      case SB_BOTTOM:
        nHscrollInc  = lpCnt->nHscrollMax - lpCnt->nHscrollPos;

        if( lpCnt->lpCI->bFocusRecLocked )
          wScrollEvent = CN_LK_HS_BOTTOM;
        else
          wScrollEvent = CN_HSCROLL_BOTTOM;
        break;

      case SB_LINEUP:
        // if icons have been arranged, scroll by the width of the arranged
        // parameters, otherwise scroll by the width of the icon
        if ( lpCnt->lpCI->bArranged )
          nHscrollInc = -(lpCnt->lpCI->aiInfo.nWidth + lpCnt->lpCI->aiInfo.nHorSpacing);
        else
          nHscrollInc = -lpCnt->lpCI->ptBmpOrIcon.x;

        if( lpCnt->lpCI->bFocusRecLocked )
          wScrollEvent = CN_LK_HS_LINEUP;
        else
          wScrollEvent = CN_HSCROLL_LINEUP;
        break;

      case SB_LINEDOWN:
        // if icons have been arranged, scroll by the width of the arranged
        // parameters, otherwise scroll by the width of the icon
        if ( lpCnt->lpCI->bArranged )
          nHscrollInc = lpCnt->lpCI->aiInfo.nWidth + lpCnt->lpCI->aiInfo.nHorSpacing;
        else
          nHscrollInc = lpCnt->lpCI->ptBmpOrIcon.x;

        if( lpCnt->lpCI->bFocusRecLocked )
          wScrollEvent = CN_LK_HS_LINEDOWN;
        else
          wScrollEvent = CN_HSCROLL_LINEDOWN;
        break;

      case SB_PAGEUP:
        nHscrollInc  = -lpCnt->cxClient;
        if( lpCnt->lpCI->bFocusRecLocked )
          wScrollEvent = CN_LK_HS_PAGEUP;
        else
          wScrollEvent = CN_HSCROLL_PAGEUP;
        break;

      case SB_PAGEDOWN:
        nHscrollInc  = lpCnt->cxClient;
        if( lpCnt->lpCI->bFocusRecLocked )
          wScrollEvent = CN_LK_HS_PAGEDOWN;
        else
          wScrollEvent = CN_HSCROLL_PAGEDOWN;
        break;

      case SB_THUMBPOSITION:
        nHscrollInc  = pos - lpCnt->nHscrollPos;
        if( lpCnt->lpCI->bFocusRecLocked )
          wScrollEvent = CN_LK_HS_THUMBPOS;
        else
          wScrollEvent = CN_HSCROLL_THUMBPOS;
        break;

      case SB_THUMBTRACK:
        nHscrollInc  = pos - lpCnt->nHscrollPos;
        break;

      default:
        nHscrollInc = 0;
    }

    // Scroll the record area horizontally.
    if( !lpCnt->lpCI->bFocusFldLocked )
    {
      if( nHscrollInc = max(-lpCnt->nHscrollPos, min(nHscrollInc, lpCnt->nHscrollMax - lpCnt->nHscrollPos)) )
      {
        rect.left   = 0;
        rect.top    = lpCnt->lpCI->cyTitleArea;
        rect.right  = lpCnt->cxClient;
        rect.bottom = lpCnt->cyClient;
    
        lpCnt->nHscrollPos += nHscrollInc;

        // reset the icon viewspace
        lpCnt->lpCI->rcViewSpace.left += nHscrollInc;
        lpCnt->lpCI->rcViewSpace.right = lpCnt->lpCI->rcViewSpace.left + lpCnt->cxClient;
        
        ScrollWindow( hWnd, -nHscrollInc, 0, NULL, &rect );
        SetScrollPos( hWnd, SB_HORZ, lpCnt->nHscrollPos, TRUE );

        // set top record
        if (lpCnt->lpCI->lpRecOrderHead)
          lpCnt->lpCI->lpTopRec = lpCnt->lpCI->lpRecOrderHead->lpRec;

        UpdateWindow( hWnd );
      }
    }

    // Notify the assoc of the scrolling event if there was movement.
    if( nHscrollInc )
      NotifyAssocWnd( hWnd, lpCnt, wScrollEvent, 0, NULL, NULL, nHscrollInc, 0, 0, NULL );

    // check to see if scroll bar still visible - there are cases when all the icons
    // can scroll into view - when this happens, the scroll bar should disappear.
    // Since you cannot call SetScrollRange with a range of zero while processing
    // a scroll bar message (causes painting problems - this is doc'ed in Win32 docs)
    // I post a message in the queue at the end of the scroll to WM_SIZE.  WM_SIZE
    // adjusts the icon workspace and viewspace and checks for validity of scrollbars.
    if (lpCnt->lpCI->rcViewSpace.left  <= lpCnt->lpCI->rcWorkSpace.left &&
        lpCnt->lpCI->rcViewSpace.right >= lpCnt->lpCI->rcWorkSpace.right)
    {
      if (code==SB_ENDSCROLL)
        PostMessage( hWnd, WM_SIZE, 0, MAKELPARAM(lpCnt->cxClient, lpCnt->cyClient) );
    }

    return;
}  

VOID Icn_OnChar( HWND hWnd, UINT ch, int cRepeat )
{
    LPCONTAINER lpCnt = GETCNT(hWnd);
    BOOL        bShiftKey, bCtrlKey;

    // Ignore non-supported keys.
    if( ch < 32 )
    {
      // Check for clipboard message keyboard equivlents.
      if( ch == CTRL_X )
        NotifyAssocWnd( hWnd, lpCnt, CN_CUT, ch, NULL, NULL, 0, 0, 0, NULL );
      else if( ch == CTRL_C )
        NotifyAssocWnd( hWnd, lpCnt, CN_COPY, ch, NULL, NULL, 0, 0, 0, NULL );
      else if( ch == CTRL_V )
        NotifyAssocWnd( hWnd, lpCnt, CN_PASTE, ch, NULL, NULL, 0, 0, 0, NULL );
      else if( ch == CTRL_R )
        NotifyAssocWnd( hWnd, lpCnt, CN_CLEAR, ch, NULL, NULL, 0, 0, 0, NULL );
      else if( ch == CTRL_Z )
        NotifyAssocWnd( hWnd, lpCnt, CN_UNDO, ch, NULL, NULL, 0, 0, 0, NULL );
      return;
    }

    // If keyboard input is disabled just beep and return.
    if( !lpCnt->lpCI->bSendKBEvents )
    {
      MessageBeep(0);
      return;
    }

    // Get the state of the shift and control keys.
    bShiftKey = GetAsyncKeyState( VK_SHIFT ) & ASYNC_KEY_DOWN;
    bCtrlKey  = GetAsyncKeyState( VK_CONTROL ) & ASYNC_KEY_DOWN;

    // If focus cell is not in view scroll to it.
    if( !IcnIsFocusInView( hWnd, lpCnt ) ) 
      IcnScrollToFocus( hWnd, lpCnt );

    // If selection is active do special handling of spacebar hit.
    if( ch == VK_SPACE )
    {
      // If in multiple select the spacebar toggles record selection.
      if( (lpCnt->dwStyle & CTS_MULTIPLESEL) && lpCnt->lpCI->lpFocusRec )
      {
        if( lpCnt->lpCI->lpFocusRec->flRecAttr & CRA_SELECTED )
        {
          lpCnt->lpCI->lpFocusRec->flRecAttr &= ~CRA_SELECTED;
          InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
          NotifyAssocWnd( hWnd, lpCnt, CN_RECUNSELECTED, 0, lpCnt->lpCI->lpFocusRec, NULL, 0, 0, 0, NULL );
        }
        else
        {
          lpCnt->lpCI->lpFocusRec->flRecAttr |= CRA_SELECTED;
          InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
          NotifyAssocWnd( hWnd, lpCnt, CN_RECSELECTED, 0, lpCnt->lpCI->lpFocusRec, NULL, 0, 0, 0, NULL );
        }

        // Don't pass this space bar hit as a CHARHIT notification.
        return;
      }
      
      // If in extended select the spacebar clears the selection list
      // list if the SHIFT key is not pressed. If it is pressed it toggles
      // the selection status of the focus record.
      if( (lpCnt->dwStyle & CTS_EXTENDEDSEL) && lpCnt->lpCI->lpFocusRec )
      {
        // If the shift key is not pressed clear the selection list.
        // and then select the focus record.
        if( !bShiftKey )
        {
          ClearExtSelectList( hWnd, lpCnt );

          lpCnt->lpCI->lpFocusRec->flRecAttr |= CRA_SELECTED;
          InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
          NotifyAssocWnd( hWnd, lpCnt, CN_RECSELECTED, 0, lpCnt->lpCI->lpFocusRec, NULL, 0, 0, 0, NULL );
        }
        else
        {
          if( lpCnt->lpCI->lpFocusRec->flRecAttr & CRA_SELECTED )
          {
            lpCnt->lpCI->lpFocusRec->flRecAttr &= ~CRA_SELECTED;
            InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
            NotifyAssocWnd( hWnd, lpCnt, CN_RECUNSELECTED, 0, lpCnt->lpCI->lpFocusRec, NULL, 0, 0, 0, NULL );
          }
          else
          {
            lpCnt->lpCI->lpFocusRec->flRecAttr |= CRA_SELECTED;
            InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
            NotifyAssocWnd( hWnd, lpCnt, CN_RECSELECTED, 0, lpCnt->lpCI->lpFocusRec, NULL, 0, 0, 0, NULL );
          }
        }

        // Don't pass this space bar hit as a CHARHIT notification.
        return;
      }
    }

    // Notify the parent of the char hit.
    NotifyAssocWnd( hWnd, lpCnt, CN_CHARHIT, ch, lpCnt->lpCI->lpFocusRec, NULL, 0, bShiftKey, bCtrlKey, NULL );

    return;
}  

VOID Icn_OnKey( HWND hWnd, UINT vk, BOOL fDown, int cRepeat, UINT flags )
{
    LPCONTAINER   lpCnt = GETCNT(hWnd);
    LPCONTAINER   lpCntFrame;
    LPRECORDCORE  lpRec=NULL;
    BOOL          bShiftKey, bCtrlKey, bRecSelected;

    // We don't process key-up msgs. This shouldn't ever happen but
    // I will put it here anyway just in case somebody inadvertently
    // adds a HANDLE_MSG for key-up msgs.
    if( !fDown )
    {
      // Forward this to Windows for processing.
      FORWARD_WM_KEYUP( hWnd, vk, cRepeat, flags, CNT_DEFWNDPROC );
      return;
    }

    // set the message.  This can be queried by the user to see if messages sent were
    // sent by a key click (as opposed to a lbuttondown msg).
    lpCnt->iInputID = WM_KEYDOWN;

    // If keyboard input is disabled just beep and return.
    if( !lpCnt->lpCI->bSendKBEvents )
    {
      MessageBeep(0);
      return;
    }

    // If the container has focus but the state was not updated
    // send a WM_SETFOCUS to ensure that the focus rect is painted.
    // Focus is sometimes muddled when interacting with edit controls.
    if( hWnd == GetFocus() && !StateTest( lpCnt, CNTSTATE_HASFOCUS ) )
      FORWARD_WM_SETFOCUS( hWnd, NULL, SendMessage );

    // Get the state of the shift and control keys.
    bShiftKey = GetAsyncKeyState( VK_SHIFT ) & ASYNC_KEY_DOWN;
    bCtrlKey  = GetAsyncKeyState( VK_CONTROL ) & ASYNC_KEY_DOWN;

    // Set to simulated state if we are simulating a key press.
    if( lpCnt->bSimulateShftKey )
      bShiftKey = lpCnt->lpCI->bSimShiftKey;
    if( lpCnt->bSimulateCtrlKey )
      bCtrlKey = lpCnt->lpCI->bSimCtrlKey;

    lpCntFrame = GETCNT(lpCnt->hWndFrame);

    switch( vk )
    {
      case VK_END:
      case VK_NEXT:
      case VK_PRIOR:
      case VK_HOME:
        // If focus cell is not in view, scroll to it.
        if( !IcnIsFocusInView( hWnd, lpCnt ) )
          IcnScrollToFocus( hWnd, lpCnt );

        bRecSelected = FALSE;

        // scroll down
        if (vk==VK_END)
          FORWARD_WM_VSCROLL( hWnd, 0, SB_BOTTOM,   0, SendMessage );
        else if (vk==VK_NEXT)
          FORWARD_WM_VSCROLL( hWnd, 0, SB_PAGEDOWN, 0, SendMessage );
        else if (vk==VK_PRIOR)
          FORWARD_WM_VSCROLL( hWnd, 0, SB_PAGEUP,   0, SendMessage );
        else 
          FORWARD_WM_VSCROLL( hWnd, 0, SB_TOP,      0, SendMessage );

        // find the next record closest to the bottom or top depending on vk
        if (lpCnt->lpCI->lpFocusRec)
          lpRec = FindNextRecIcn( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, vk );

        if( lpCntFrame->bScrollFocusRect && lpRec != lpCnt->lpCI->lpFocusRec && lpRec)
        {
          if( !lpCnt->lpCI->bFocusRecLocked )
          {
            // there is a record to move towards 
            // For single, extended, or block select we will chg the selection.
            if( lpCnt->dwStyle & CTS_SINGLESEL && lpCnt->lpCI->lpSelRec )
            {
              lpCnt->lpCI->lpSelRec->flRecAttr &= ~CRA_SELECTED;
              InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpSelRec, NULL, 1 );
              NotifyAssocWnd( hWnd, lpCnt, CN_RECUNSELECTED, 0, lpCnt->lpCI->lpSelRec, NULL, 0, bShiftKey, bCtrlKey, NULL );
            }
            else if( lpCnt->dwStyle & CTS_EXTENDEDSEL )
            {
              // will clear the selection list always on these keystrokes
              ClearExtSelectList( hWnd, lpCnt );
            }

            // Unfocus the old focus record.
            lpCnt->lpCI->lpFocusRec->flRecAttr &= ~CRA_CURSORED;
            InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );

            // set the new focus record
            lpCnt->lpCI->lpFocusRec = lpRec;

            // Now repaint the new focus record.
            lpCnt->lpCI->lpFocusRec->flRecAttr |= CRA_CURSORED;

            if( lpCnt->dwStyle & CTS_SINGLESEL )
            {
              // This is the new selected record.
              lpCnt->lpCI->lpSelRec = lpCnt->lpCI->lpFocusRec;
              lpCnt->lpCI->lpSelRec->flRecAttr |= CRA_SELECTED;
              InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
              bRecSelected = TRUE;
            }
            else if( lpCnt->dwStyle & CTS_EXTENDEDSEL )
            {
              lpCnt->lpCI->lpFocusRec->flRecAttr |= CRA_SELECTED;
              bRecSelected = TRUE;
              InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
            }
            else
            {
              InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
            }

            // Tell associate wnd about the events.
            NotifyAssocWnd( hWnd, lpCnt, CN_NEWFOCUSREC, 0, lpCnt->lpCI->lpFocusRec, NULL, 0, bShiftKey, bCtrlKey, NULL );
            if( bRecSelected )
              NotifyAssocWnd( hWnd, lpCnt, CN_RECSELECTED, 0, lpCnt->lpCI->lpFocusRec, NULL, 0, bShiftKey, bCtrlKey, NULL );
          }
          else
          {
              // User has locked the focus record so we will just send
              // the keystroke notification but NOT move the focus.
              if (vk==VK_END)
                NotifyAssocWnd( hWnd, lpCnt, CN_LK_END,      0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
              else if (vk==VK_NEXT)
                NotifyAssocWnd( hWnd, lpCnt, CN_LK_PAGEDOWN, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
              else if (vk==VK_PRIOR)
                NotifyAssocWnd( hWnd, lpCnt, CN_LK_PAGEUP,   0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
              else if (vk==VK_HOME)
                NotifyAssocWnd( hWnd, lpCnt, CN_LK_HOME,     0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
          }
        }
        break;

      case VK_UP:
      case VK_DOWN:
      case VK_LEFT:
      case VK_RIGHT:
        if( bCtrlKey )  
        {
          FORWARD_WM_KEYDOWN( hWnd, VK_HOME, 0, 0, SendMessage );
          break;
        }

        bRecSelected = FALSE;

        // If focus cell is not in view, scroll to it. 
        if( !IcnIsFocusInView( hWnd, lpCnt ) )
          IcnScrollToFocus( hWnd, lpCnt );

        if( lpCntFrame->bScrollFocusRect && lpCnt->lpCI->lpFocusRec )
        {
          if( !lpCnt->lpCI->bFocusRecLocked )
          {
            // find the next record near the focus record depending on vk
            lpRec = FindNextRecIcn( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, vk );

            if (lpRec)
            {
              // there is a record to move towards 
              // For single or extended select we will chg the selection.
              if( lpCnt->dwStyle & CTS_SINGLESEL && lpCnt->lpCI->lpSelRec )
              {
                lpCnt->lpCI->lpSelRec->flRecAttr &= ~CRA_SELECTED;
                InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpSelRec, NULL, 1 );
                NotifyAssocWnd( hWnd, lpCnt, CN_RECUNSELECTED, 0, lpCnt->lpCI->lpSelRec, NULL, 0, bShiftKey, bCtrlKey, NULL );
              }
              else if( lpCnt->dwStyle & CTS_EXTENDEDSEL )
              {
                // Start a new selection list if shift key or ctrl key not pressed.
                if( !bShiftKey && !bCtrlKey )
                  ClearExtSelectList( hWnd, lpCnt );
              }

              // Unfocus the old focus record.
              lpCnt->lpCI->lpFocusRec->flRecAttr &= ~CRA_CURSORED;
              InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );

              // set the new focus record
              lpCnt->lpCI->lpFocusRec = lpRec;

              // if out of view area, scroll it into view
              if( !IcnIsFocusInView( hWnd, lpCnt ) )
                IcnScrollToFocus( hWnd, lpCnt );

              // Now repaint the new focus record.
              lpCnt->lpCI->lpFocusRec->flRecAttr |= CRA_CURSORED;

              if( lpCnt->dwStyle & CTS_SINGLESEL )
              {
                // This is the new selected record.
                lpCnt->lpCI->lpSelRec = lpCnt->lpCI->lpFocusRec;
                lpCnt->lpCI->lpSelRec->flRecAttr |= CRA_SELECTED;
                InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
                bRecSelected = TRUE;
              }
              else if( lpCnt->dwStyle & CTS_EXTENDEDSEL )
              {
                // toggle the selected flag for this record.
                if( lpCnt->lpCI->lpFocusRec->flRecAttr & CRA_SELECTED )
                {
                  lpCnt->lpCI->lpFocusRec->flRecAttr &= ~CRA_SELECTED;
                  NotifyAssocWnd( hWnd, lpCnt, CN_RECUNSELECTED, 0, lpCnt->lpCI->lpFocusRec, NULL, 0, bShiftKey, bCtrlKey, NULL );
                }
                else
                {
                  lpCnt->lpCI->lpFocusRec->flRecAttr |= CRA_SELECTED;
                  bRecSelected = TRUE;
                }
                
                InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
              }
              else
              {
                InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
              }

              // Tell associate wnd about the events.
              NotifyAssocWnd( hWnd, lpCnt, CN_NEWFOCUSREC, 0, lpCnt->lpCI->lpFocusRec, NULL, 0, bShiftKey, bCtrlKey, NULL );
              if( bRecSelected )
                NotifyAssocWnd( hWnd, lpCnt, CN_RECSELECTED, 0, lpCnt->lpCI->lpFocusRec, NULL, 0, bShiftKey, bCtrlKey, NULL );
            }
          }
          else
          {
            // User has locked the focus record so we will just send
            // the keystroke notification but NOT move the focus.
            if (vk==VK_UP)
              NotifyAssocWnd( hWnd, lpCnt, CN_LK_ARROW_UP, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
            else if (vk==VK_DOWN)
              NotifyAssocWnd( hWnd, lpCnt, CN_LK_ARROW_DOWN, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
            else if (vk==VK_LEFT)
              NotifyAssocWnd( hWnd, lpCnt, CN_LK_ARROW_LEFT, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
            else 
              NotifyAssocWnd( hWnd, lpCnt, CN_LK_ARROW_RIGHT, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
          }
        }
        else
        {
          if (vk==VK_UP)
            FORWARD_WM_VSCROLL( hWnd, 0, SB_LINEUP, 0, SendMessage );
          else if (vk==VK_DOWN)
            FORWARD_WM_VSCROLL( hWnd, 0, SB_LINEDOWN, 0, SendMessage );
          else if (vk==VK_LEFT)
            FORWARD_WM_HSCROLL( hWnd, 0, SB_LINEUP, 0, SendMessage );
          else 
            FORWARD_WM_HSCROLL( hWnd, 0, SB_LINEDOWN, 0, SendMessage );
        }
        break;

      case VK_RETURN:
        // If focus cell is not in view, scroll to it. This should
        // put the focus cell in view 1 row down from the top.
        if( !IcnIsFocusInView( hWnd, lpCnt ) )
          IcnScrollToFocus( hWnd, lpCnt );

        NotifyAssocWnd( hWnd, lpCnt, CN_ENTER, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
        break;

      case VK_INSERT:
        if( bShiftKey )
        {
          NotifyAssocWnd( hWnd, lpCnt, CN_PASTE, 0, NULL, NULL, 0, 0, 0, NULL );
        }
        else if( bCtrlKey )
        {
          NotifyAssocWnd( hWnd, lpCnt, CN_COPY, 0, NULL, NULL, 0, 0, 0, NULL );
        }
        else
        {
          if( !(lpCnt->dwStyle & CTS_READONLY) )
          {
            // If focus cell is not in view, scroll to it. This should
            // put the focus cell in view 1 row down from the top.
            if( !IcnIsFocusInView( hWnd, lpCnt ) )
              IcnScrollToFocus( hWnd, lpCnt );

            // For async associates do an auto-lock with inserts.
            if( lpCnt->dwStyle & CTS_ASYNCNOTIFY )
              CntFocusRecLock( hWnd );
            
            NotifyAssocWnd( hWnd, lpCnt, CN_INSERT, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
          }
        }
        break;

      case VK_DELETE:
        if( bShiftKey )
        {
          NotifyAssocWnd( hWnd, lpCnt, CN_CUT, 0, NULL, NULL, 0, 0, 0, NULL );
        }
        else
          {            
          if( !(lpCnt->dwStyle & CTS_READONLY) )
          {
            // If focus cell is not in view, scroll to it. This should
            // put the focus cell in view 1 row down from the top.
            if( !IcnIsFocusInView( hWnd, lpCnt ) )
              IcnScrollToFocus( hWnd, lpCnt );

            // For async associates do an auto-lock with inserts.
            if( lpCnt->dwStyle & CTS_ASYNCNOTIFY )
              CntFocusRecLock( hWnd );
            
            NotifyAssocWnd( hWnd, lpCnt, CN_DELETE, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
          }
        }
        break;

      case VK_ESCAPE:
        NotifyAssocWnd( hWnd, lpCnt, CN_ESCAPE, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
        break;

      case VK_TAB:
        NotifyAssocWnd( hWnd, lpCnt, CN_TAB, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
        break;

      case VK_F1:
        NotifyAssocWnd( hWnd, lpCnt, CN_F1, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
        break;
      case VK_F2:
        NotifyAssocWnd( hWnd, lpCnt, CN_F2, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
        break;
      case VK_F3:
        NotifyAssocWnd( hWnd, lpCnt, CN_F3, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
        break;
      case VK_F4:
        NotifyAssocWnd( hWnd, lpCnt, CN_F4, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
        break;
      case VK_F5:
        NotifyAssocWnd( hWnd, lpCnt, CN_F5, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
        break;
      case VK_F6:
        NotifyAssocWnd( hWnd, lpCnt, CN_F6, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
        break;
      case VK_F7:
        NotifyAssocWnd( hWnd, lpCnt, CN_F7, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
        break;
      case VK_F8:
        if( ((lpCnt->dwStyle & CTS_EXTENDEDSEL) && bShiftKey) || 
            ((lpCnt->dwStyle & CTS_MULTIPLESEL) && lpCnt->bSimulateMulSel) )
        {
          // Special mode for extended select - simulate multiple select.
          // When we start this mode we cue the user visually by setting a
          // wide focus rect and painting the focus rect double thickness.
          if( lpCnt->bSimulateMulSel )
          {
            lpCnt->dwStyle &= ~CTS_MULTIPLESEL;
            lpCnt->dwStyle |=  CTS_EXTENDEDSEL;
            lpCnt->bSimulateMulSel = FALSE;  
          }
          else
          {
            lpCnt->dwStyle &= ~CTS_EXTENDEDSEL;
            lpCnt->dwStyle |=  CTS_MULTIPLESEL;
            lpCnt->bSimulateMulSel = TRUE;  
          }
          InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
        }
        else
        {
          NotifyAssocWnd( hWnd, lpCnt, CN_F8, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
        }
        break;
      case VK_F9:
        NotifyAssocWnd( hWnd, lpCnt, CN_F9, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
        break;
      case VK_F10:
        NotifyAssocWnd( hWnd, lpCnt, CN_F10, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
        break;
      case VK_F11:
        NotifyAssocWnd( hWnd, lpCnt, CN_F11, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
        break;
      case VK_F12:
        NotifyAssocWnd( hWnd, lpCnt, CN_F12, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
        break;
      case VK_F13:
        NotifyAssocWnd( hWnd, lpCnt, CN_F13, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
        break;
      case VK_F14:
        NotifyAssocWnd( hWnd, lpCnt, CN_F14, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
        break;
      case VK_F15:
        NotifyAssocWnd( hWnd, lpCnt, CN_F15, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
        break;
      case VK_F16:
        NotifyAssocWnd( hWnd, lpCnt, CN_F16, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
        break;
      case VK_F17:
        NotifyAssocWnd( hWnd, lpCnt, CN_F17, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
        break;
      case VK_F18:
        NotifyAssocWnd( hWnd, lpCnt, CN_F18, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
        break;
      case VK_F19:
        NotifyAssocWnd( hWnd, lpCnt, CN_F19, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
        break;
      case VK_F20:
        NotifyAssocWnd( hWnd, lpCnt, CN_F20, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
        break;
      case VK_F21:
        NotifyAssocWnd( hWnd, lpCnt, CN_F21, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
        break;
      case VK_F22:
        NotifyAssocWnd( hWnd, lpCnt, CN_F22, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
        break;
      case VK_F23:
        NotifyAssocWnd( hWnd, lpCnt, CN_F23, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
        break;
      case VK_F24:
        NotifyAssocWnd( hWnd, lpCnt, CN_F24, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
        break;
      case VK_BACK:
        NotifyAssocWnd( hWnd, lpCnt, CN_BACK, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
        break;

      }

    // reset the message
    lpCnt->iInputID = -1;

    return;
}

VOID Icn_OnRButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags )
{
    // same as text view
    Txt_OnRButtonDown( hWnd, fDoubleClick, x, y, keyFlags );
    
    return;
}

UINT Icn_OnGetDlgCode( HWND hWnd, LPMSG lpmsg )
{
    UINT       uRet;
    
    uRet = Dtl_OnGetDlgCode( hWnd, lpmsg );

    return uRet;
}

VOID Icn_OnLButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags )
{
    LPCONTAINER  lpCnt = GETCNT(hWnd);
    LPZORDERREC  lpOrderRec;
    LPRECORDCORE lpRec, lpOldSelRec;
    RECT         rect, rectTitle, rectRecs;
    RECT         rectTitleBtn;
    POINT        pt;
    BOOL         bShiftKey, bCtrlKey;
    BOOL         bRecSelected, bRecUnSelected;
    BOOL         bClickedTitle, bClickedRecord, bClickedInRecordArea;
    BOOL         bNewRecList;
    BOOL         bNewFocusRec, bNewRecSelected;
    BOOL         bClickedTitleBtn;
    int          cxPxlWidth;

    // set the message.  This can be queried by the user to see if messages sent were
    // sent by a lbuttondown click (as opposed to a keydown msg).
    lpCnt->iInputID = WM_LBUTTONDOWN;

    // If mouse input is disabled just beep and return.
    if( !lpCnt->lpCI->bSendMouseEvents )
    {
      MessageBeep(0);
      return;
    }

    if( hWnd != GetFocus() )
    {
      // User clicked in the container from some other
      // control so set the focus on the container.
      SetFocus( hWnd );

      // The container should have gotten a WM_SETFOCUS msg which sets
      // the HASFOCUS state but in some cases it is not sent. If the
      // container didn't get the WM_SETFOCUS msg we will send it now
      // to ensure that the focus rect is painted correctly.
      if( !StateTest( lpCnt, CNTSTATE_HASFOCUS ) )
        FORWARD_WM_SETFOCUS( hWnd, NULL, SendMessage );
    }

    // Get the mouse coordinates.
    pt.x = x;
    pt.y = y;

    bClickedTitle = bClickedRecord = bClickedTitleBtn = bClickedInRecordArea = FALSE;

    // Set up rects to determine where the user clicked.
    rect.left   = 0;
    rect.top    = 0;
    rect.right  = lpCnt->cxClient;
    rect.bottom = lpCnt->cyClient;

    SetRectEmpty( &rectTitle    );
    SetRectEmpty( &rectTitleBtn );
    SetRectEmpty( &rectRecs     );

    // Setup the title rect if there is one.
    if( lpCnt->lpCI->cyTitleArea )
    {
      CopyRect( &rectTitle, &rect );
      rectTitle.bottom = lpCnt->lpCI->cyTitleArea;

      // Setup the title button rect if there is one.
      if( lpCnt->lpCI->nTtlBtnWidth )
      {
        CopyRect( &rectTitleBtn, &rectTitle );

        // Get the title button width in pixels.
        if( lpCnt->lpCI->nTtlBtnWidth > 0 )
          cxPxlWidth = lpCnt->lpCI->cxCharTtl * lpCnt->lpCI->nTtlBtnWidth;
        else
          cxPxlWidth = -lpCnt->lpCI->nTtlBtnWidth;

        // Setup the boundaries of the title button and adjust the title.
        if( lpCnt->lpCI->bTtlBtnAlignRt )
        {
          rectTitle.right  -= cxPxlWidth;
          rectTitleBtn.left = rectTitle.right + 1;
        }
        else
        {
          rectTitle.left    += cxPxlWidth;
          rectTitleBtn.right = rectTitle.left - 1;
        }
      }
    }

    // Setup the record data rect.
    CopyRect( &rectRecs, &rect );
    rectRecs.top = rectTitle.bottom;

    // Set the flag for the object the user clicked on.
    if( PtInRect(&rectTitle, pt) )
      bClickedTitle = TRUE;
    else if( PtInRect(&rectTitleBtn, pt) )
      bClickedTitleBtn = TRUE;
    else if( PtInRect(&rectRecs, pt) )
      bClickedInRecordArea = TRUE;

    // If user clicked in a record find out which one it was.
    if( bClickedInRecordArea )
    {
      // finds the record the user clicked on - if this returns NULL, it means that
      // the user clicked in the record area but not on a record
      lpOrderRec = FindClickedRecordIcn( hWnd, lpCnt, pt );

      // Save the record rect so we can track mouse movement against it either in 
      // swipe select or when moving an icon
      CopyRect( &lpCnt->rectRecArea, &rectRecs );

      // Clicked in record area but not on a record - may be starting swipe select
      // if in extended select selection mode
      if (!lpOrderRec)
      {
        bClickedRecord = FALSE;
      }
      else
      {
        LPZORDERREC lpTemp;

        // bring this record to the front of the recorder list
        if ( lpOrderRec != lpCnt->lpCI->lpRecOrderHead )
        {
          // remove the node from the list and then add it to the head
          if ( lpOrderRec == lpCnt->lpCI->lpRecOrderTail )
          {
            lpCnt->lpCI->lpRecOrderTail = (LPZORDERREC)
              LLRemoveTail( (LPLINKNODE) lpCnt->lpCI->lpRecOrderTail, ((LPLINKNODE FAR *)&lpTemp) );
          }
          else
          {
            lpTemp = (LPZORDERREC) LLRemoveNode( (LPLINKNODE) lpOrderRec );
          }
          
          // add the record to the head of the list
          CntAddRecOrderHead( hWnd, lpOrderRec );
        }

        // flag that the user clicked on a record and set the clicked on record
        bClickedRecord = TRUE;
        lpRec = lpOrderRec->lpRec;
      }

      // save the rect of this record/field cell.
//      lpCnt->rectCursorFld.top    = rectRecs.top + (nRec * lpCnt->lpCI->cyRow);
//      lpCnt->rectCursorFld.bottom = lpCnt->rectCursorFld.top + lpCnt->lpCI->cyRow;
//      lpCnt->rectCursorFld.left  = xStrCol;
//      lpCnt->rectCursorFld.right = xEndCol;

//      lpCnt->nCurRec = nRec;
    }

    if( fDoubleClick )
    {
      if( bClickedTitle )
      {
        if( !(lpCnt->dwStyle & CTS_READONLY) &&
            !(lpCnt->lpCI->flCntAttr & CA_TTLREADONLY) )
        {
          // Let the user edit the container title.
          NotifyAssocWnd( hWnd, lpCnt, CN_BEGTTLEDIT, 0, NULL, NULL, 0, 0, 0, NULL );
        }
        else
        {
          NotifyAssocWnd( hWnd, lpCnt, CN_ROTTLDBLCLK, 0, NULL, NULL, 0, 0, 0, NULL );
        }
      }

      if( bClickedRecord )
      {
        if( !(lpCnt->dwStyle & CTS_READONLY) &&
            !(lpRec->flRecAttr & CRA_RECREADONLY) )
        {
          // notify user of double click.
          NotifyAssocWnd( hWnd, lpCnt, CN_RECDBLCLK, 0, lpRec, NULL, 0, 0, 0, NULL );
        }
        else
        {
          NotifyAssocWnd( hWnd, lpCnt, CN_RORECDBLCLK, 0, lpRec, NULL, 0, 0, 0, NULL );
        }
      }
    }
    else
    {
      if( bClickedTitleBtn )
      {
        // set the state to indicate we pressed the title button
        StateSet( lpCnt, CNTSTATE_CLKEDTTLBTN );
        lpCnt->lpCI->flCntAttr |= CA_TTLBTNPRESSED;
        CopyRect( &lpCnt->rectPressedBtn, &rectTitleBtn );
        SetCapture( hWnd );
        UpdateContainer( hWnd, &lpCnt->rectPressedBtn, TRUE );
      }
      else if( bClickedInRecordArea )
      {
        if ( bClickedRecord )
        {
          // not sure what these next lines do - I'll keep them for now.
          // End special mode for extended select - simulate multiple select.
          if( (lpCnt->dwStyle & CTS_MULTIPLESEL) && lpCnt->bSimulateMulSel )
          {
            lpCnt->dwStyle &= ~CTS_MULTIPLESEL;
            lpCnt->dwStyle |=  CTS_EXTENDEDSEL;
            lpCnt->bSimulateMulSel = FALSE;  
            InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
          }

          // Set flags indicating if the focus field or record is changing.
          bNewFocusRec = lpCnt->lpCI->lpFocusRec != lpRec ? TRUE : FALSE;

          // If the record is locked and the user has clicked to a new
          // record or if the field is locked and the user has clicked to
          // a new field (or both of the above) we do not let them.
          // We just notify the associate and let it take care of it.
          if((lpCnt->lpCI->bFocusRecLocked && bNewFocusRec))
          {
            NotifyAssocWnd( hWnd, lpCnt, CN_LK_NEWFOCUSREC, 0, lpRec, NULL, 0, 0, 0, NULL );
          }
          else
          {
            bRecSelected = bRecUnSelected = bNewRecList = FALSE;

            // Set the record attribute according to the selection style.
            if( lpCnt->dwStyle & CTS_SINGLESEL )
            {
              // Set this so we don't invalidate the whole
              // record if the record selection didn't change.
              bNewRecSelected = lpCnt->lpCI->lpSelRec != lpRec ? TRUE : FALSE;

              // Unselect the previously selected record and invalidate it.
              if( lpCnt->lpCI->lpSelRec && bNewRecSelected )
              {
                lpCnt->lpCI->lpSelRec->flRecAttr &= ~CRA_SELECTED;
         
                bRecUnSelected = TRUE;
                // remember old selrec so can send CN_RECUNSELECTED msg with this record.
                lpOldSelRec = lpCnt->lpCI->lpSelRec;

                // If this record is in the display area invalidate
                // it but don't repaint it yet.
                InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpSelRec, NULL, 1 );
              }

              // This is the new selected record.
              lpCnt->lpCI->lpSelRec = lpRec;
              lpRec->flRecAttr |= CRA_SELECTED;
              bRecSelected = TRUE;
            }
            else if( lpCnt->dwStyle & CTS_MULTIPLESEL )
            {
              // For multiple select toggle the selected flag for this record.
              if( lpRec->flRecAttr & CRA_SELECTED )
              {
                lpRec->flRecAttr &= ~CRA_SELECTED;
                bRecUnSelected = TRUE;
              }
              else
              {
                lpRec->flRecAttr |= CRA_SELECTED;
                bRecSelected = TRUE;
              }
            }
            else if( lpCnt->dwStyle & CTS_EXTENDEDSEL )
            {
              // Get the state of the shift and control keys.- note that in icon
              // view, shift key behaves like ctrl key
              bShiftKey = GetAsyncKeyState( VK_SHIFT ) & ASYNC_KEY_DOWN;
              bCtrlKey  = GetAsyncKeyState( VK_CONTROL ) & ASYNC_KEY_DOWN;

              // If the control key is not pressed start a new selection list.
              if( !bCtrlKey && !bShiftKey )
              {
                ClearExtSelectList( hWnd, lpCnt );

                // Force an update of the clicked-on record.
//                InvalidateCntRecord( hWnd, lpCnt, lpRec, NULL, 2 );
              }

              // toggle the selected flag for this record.
              if( lpRec->flRecAttr & CRA_SELECTED )
              {
                lpRec->flRecAttr &= ~CRA_SELECTED;
                bRecUnSelected = TRUE;
              }
              else
              {
                lpRec->flRecAttr |= CRA_SELECTED;
                bRecSelected = TRUE;
              }
            }

            // Remember if the field or record changed.
            bNewFocusRec = lpCnt->lpCI->lpFocusRec != lpRec ? TRUE : FALSE;

            // Unfocus the previously focused record/field and repaint it.
            if( lpCnt->lpCI->lpFocusRec && bNewFocusRec )
            {
              lpCnt->lpCI->lpFocusRec->flRecAttr &= ~CRA_CURSORED;
              InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
            }

            // This is the new focus record
            lpCnt->lpCI->lpFocusRec = lpRec;
            lpRec->flRecAttr |= CRA_CURSORED;

            // Invalidate the clicked-on record and repaint.
            // If there is no record selection enabled just invalidate
            // the new focus field instead of the entire record.
            if( lpCnt->dwStyle & CTS_SINGLESEL )
            {
              if ( bNewRecSelected )
                InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
            }
            else if( lpCnt->dwStyle & CTS_MULTIPLESEL )
              InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
            else if( lpCnt->dwStyle & CTS_EXTENDEDSEL )
              InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
            else
              InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );

            // Tell the assoc wnd if a record was selected.
            if( bRecSelected )
              NotifyAssocWnd( hWnd, lpCnt, CN_RECSELECTED, 0, lpRec, NULL, 0, 0, 0, NULL );

            // Tell the assoc wnd if a record was unselected.
            if( bRecUnSelected )
            {
              if (lpCnt->dwStyle & CTS_SINGLESEL)
                NotifyAssocWnd( hWnd, lpCnt, CN_RECUNSELECTED, 0, lpOldSelRec, NULL, 0, 0, 0, NULL );
              else // multiple select
                NotifyAssocWnd( hWnd, lpCnt, CN_RECUNSELECTED, 0, lpRec, NULL, 0, 0, 0, NULL );
            }

            // Notify the assoc wnd if the record or field focus changed.
            if( bNewFocusRec )
            {
              NotifyAssocWnd( hWnd, lpCnt, CN_NEWFOCUS, 0, lpRec, NULL, 0, 0, 0, NULL );
              NotifyAssocWnd( hWnd, lpCnt, CN_NEWFOCUSREC, 0, lpRec, NULL, 0, 0, 0, NULL );
            }
          }
        }
        else
        {
          // user clicked in record area but not on a record - begin swipe selection
          if ( lpCnt->dwStyle & CTS_EXTENDEDSEL )
            SwipeSelection( hWnd, lpCnt );
        }
      }

      // now check to see if were in drag mode (or moving icon)
      if( bClickedRecord )
      {
        if (FALSE /*lpCnt->dwStyle & CTS_DRAGDROP || lpCnt->dwStyle & CTS_DRAGONLY */)
          DoDragTracking(hWnd);
        else if (lpCnt->dwStyle & CTS_ICONMOVEONLY)
          DoIconMove( hWnd, lpCnt, pt, lpRec );
      }
    }

    // reset the id
    lpCnt->iInputID = -1;

    return;
}  

VOID Icn_OnLButtonUp( HWND hWnd, int x, int y, UINT keyFlags )
{
    LPCONTAINER lpCnt = GETCNT(hWnd);
    POINT      pt;

    // Get the mouse coordinates at the point of the button up.
    pt.x = x;
    pt.y = y;

    ReleaseCapture();

    if( StateTest(lpCnt, CNTSTATE_CLKEDTTLBTN) )
    {
      // If we are releasing a title button reset the pressed flag,
      // repaint it in the up position, and notify the assoc wnd
      // (if mouse is still in the boundaries of the title button).
      if( !StateTest( lpCnt, CNTSTATE_MOUSEOUT ) )
      {
        lpCnt->lpCI->flCntAttr &= ~((DWORD)CA_TTLBTNPRESSED);
        StateClear( lpCnt, CNTSTATE_CLKEDTTLBTN );
        UpdateContainer( hWnd, &lpCnt->rectPressedBtn, TRUE );
        NotifyAssocWnd( hWnd, lpCnt, CN_TTLBTNCLK, 0, NULL, NULL, 0, 0, 0, NULL );
      }
    }

    // Clear out the various pointers and states.
    SetRectEmpty( &lpCnt->rectPressedBtn );
    StateClear( lpCnt, CNTSTATE_MOUSEOUT );
    StateClear( lpCnt, CNTSTATE_CLKEDTTLBTN );
//    lpCnt->lp1stSelRec  = NULL;
//    SetRectEmpty( &lpCnt->rectCursorFld  );

    return;
}  

VOID Icn_OnMouseMove( HWND hWnd, int x, int y, UINT keyFlags )
{
    LPCONTAINER lpCnt = GETCNT(hWnd);
    RECT        rect, rectTitle, rectRecs;
    POINT       pt;
    UINT        wState;

   /*-------------------------------------------------------------------*
    * On WM_MOUSEMOVE messages we want to know where the mouse has moved
    * so the appropriate action can be taken based on the current state.
    *-------------------------------------------------------------------*/

    // Get the mouse coord into a POINT.
    pt.x = x;
    pt.y = y;

    if( StateTest(lpCnt, CNTSTATE_CLKEDTTLBTN) )
    {
      // If we are down on a title button see if we went out of it
      // or if we were out of it, see if we came back into it.
      wState = lpCnt->wState;

      if( PtInRect(&lpCnt->rectPressedBtn, pt) )
      {
        StateClear( lpCnt, CNTSTATE_MOUSEOUT );
        lpCnt->lpCI->flCntAttr |= CA_TTLBTNPRESSED;
      }
      else
      {
        StateSet( lpCnt, CNTSTATE_MOUSEOUT );
        lpCnt->lpCI->flCntAttr &= ~((DWORD)CA_TTLBTNPRESSED);
      }

      // If state changed, repaint the title button.
      if( wState != lpCnt->wState )
        UpdateContainer( hWnd, &lpCnt->rectPressedBtn, TRUE );
    }
    else
    {
      // Set up rects to determine which cursor to display.
      rect.left   = 0;
      rect.top    = 0;
      rect.right  = lpCnt->cxClient;
      rect.bottom = lpCnt->cyClient;
    
      SetRectEmpty( &rectTitle    );
      SetRectEmpty( &rectRecs     );
    
      // Setup the title rect if there is one.
      if( lpCnt->lpCI->cyTitleArea )
      {
        CopyRect( &rectTitle, &rect );
        rectTitle.bottom = lpCnt->lpCI->cyTitleArea;
      }
    
      // Setup the record data rect.
      CopyRect( &rectRecs, &rect );
      rectRecs.top   = rectTitle.bottom;
    
      // Set the appropriate cursor.
      if( PtInRect( &rectRecs, pt ) )
      {
        SetCursor( lpCnt->lpCI->hCursor );
      }
      else if( PtInRect( &rectTitle, pt ) )
      {
        if( PtInTtlBtn( lpCnt, &pt ) )
          SetCursor( lpCnt->lpCI->hCurTtlBtn );
        else
          SetCursor( lpCnt->lpCI->hCurTtl ? lpCnt->lpCI->hCurTtl : lpCnt->lpCI->hCursor );
      }
      else
      {
        SetCursor( LoadCursor( NULL, IDC_ARROW ) );
      }
    }

    return;
}

VOID Icn_OnNCLButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest )
{
    // same as Text view
    Txt_OnNCLButtonDown( hWnd, fDoubleClick, x, y, codeHitTest );

    return;
}

VOID Icn_OnNCMButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest )
{
    // same as Text view
    Txt_OnNCMButtonDown( hWnd, fDoubleClick, x, y, codeHitTest );

    return;
}

VOID Icn_OnNCRButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest )
{
    // same as Text view
    Txt_OnNCRButtonDown( hWnd, fDoubleClick, x, y, codeHitTest );

    return;
}

/****************************************************************************
 * CntNewOrderRec
 *
 * Purpose:
 *  Allocates storage for the record order node structure type.
 *
 * Parameters:
 *  none
 *
 * Return Value:
 *  LPZORDERREC pointer to allocated record order node
 ****************************************************************************/
LPZORDERREC CntNewOrderRec( void )
{
    LPZORDERREC lpOR = (LPZORDERREC) calloc(1, LEN_ZORDERREC);

    return lpOR;
}

/****************************************************************************
 * CntFreeOrderRec
 *
 * Purpose:
 *  Frees storage for the record order node structure type.
 *
 * Parameters:
 *  LPZORDERREC lpOrderRec  - pointer to record order node
 *
 * Return Value:
 *  BOOL          TRUE if storage was released; FALSE if error
 ****************************************************************************/
BOOL CntFreeOrderRec( LPZORDERREC lpOrderRec )
{
    if( lpOrderRec )
    {
      free( lpOrderRec );
      return TRUE;
    }
    else
      return FALSE; 
}

/****************************************************************************
 * CntAddRecOrderHead
 * CntAddRecOrderTail
 *
 * Purpose:
 *  Adds a record order node to the beginning or end of the record 
 *  order list of the container
 *
 * Parameters:
 *  HWND        hCntWnd  - handle to the container
 *  LPZORDERREC lpNew    - record order node to add
 *
 * Return Value:
 *  BOOL          TRUE if head or tail successfully added
 ****************************************************************************/
BOOL CntAddRecOrderHead( HWND hCntWnd, LPZORDERREC lpNew )
{
    LPCNTINFO lpCI = (GETCNT(hCntWnd))->lpCI;

    lpCI->lpRecOrderHead = (LPZORDERREC) LLAddHead( (LPLINKNODE) lpCI->lpRecOrderHead, 
                                                    (LPLINKNODE) lpNew );
    // Update tail pointer, if necessary
    if( !lpCI->lpRecOrderTail )
      lpCI->lpRecOrderTail = lpCI->lpRecOrderHead;
 
    return lpCI->lpRecOrderHead ? TRUE : FALSE;
}
 
BOOL CntAddRecOrderTail( HWND hCntWnd, LPZORDERREC lpNew )
{
    LPCNTINFO lpCI = (GETCNT(hCntWnd))->lpCI;
 
    lpCI->lpRecOrderTail = (LPZORDERREC) LLAddTail( (LPLINKNODE) lpCI->lpRecOrderTail, 
                                                    (LPLINKNODE) lpNew );
    // Update head pointer, if necessary
    if( !lpCI->lpRecOrderHead )
      lpCI->lpRecOrderHead = lpCI->lpRecOrderTail;
 
    return lpCI->lpRecOrderTail ? TRUE : FALSE;
}
 
/****************************************************************************
 * CntFindOrderRec
 *
 * Purpose:
 *  Finds the record order node that contains the specified record.  I first
 *  check to see if it is the head or the tail because the majority of the
 *  time it will be in one of these two positions - this will save searching time.
 *
 * Parameters:
 *  HWND         hCntWnd  - handle to the container
 *  LPRECORDCORE lpRec    - record to find
 *
 * Return Value:
 *  the record order node, or NULL if not found
 ****************************************************************************/
LPZORDERREC CntFindOrderRec( HWND hCntWnd, LPRECORDCORE lpRec )
{
    LPCNTINFO   lpCI = (GETCNT(hCntWnd))->lpCI;
    LPZORDERREC lpOrderRec;

    if (lpRec == lpCI->lpRecOrderHead->lpRec) // at the head of the list
      return lpCI->lpRecOrderHead;
    else if (lpRec == lpCI->lpRecOrderTail->lpRec) // at the end of the list
      return lpCI->lpRecOrderTail;
    else
    {
      // find the node
      lpOrderRec = lpCI->lpRecOrderHead;
      while (lpOrderRec)
      {
        if (lpOrderRec->lpRec == lpRec)
          return lpOrderRec;

        lpOrderRec = lpOrderRec->lpNext;
      }
    }

    // node not found
    return NULL;
}

/****************************************************************************
 * CntRemoveOrderRec
 *
 * Purpose:
 *  Removes the record order node from the current linked list.
 *
 * Parameters:
 *  HWND        hCntWnd     - container window handle 
 *  LPZORDERREC lpOrderRec  - record order node.
 *
 * Return Value:
 *  LPZORDERREC  pointer to node that was removed
 ****************************************************************************/
LPZORDERREC CntRemoveOrderRec(HWND hCntWnd, LPZORDERREC lpOrderRec)
{
   LPCNTINFO   lpCI = (GETCNT(hCntWnd))->lpCI;
   LPZORDERREC lpTemp;

   if (!lpCI || !lpOrderRec)
      return (LPZORDERREC) NULL;

   // Need to check for three cases here:
   //   Removing a head node      - call LLRemoveHead
   //   Removing a tail node      - call LLRemoveTail
   //   Removing an internal node - call LLRemoveNode
   if (lpOrderRec == lpCI->lpRecOrderHead)      // Current node is head
   {
     lpCI->lpRecOrderHead = (LPZORDERREC) 
       LLRemoveHead( (LPLINKNODE) lpCI->lpRecOrderHead, ((LPLINKNODE FAR *)&lpTemp));
     if (!lpCI->lpRecOrderHead)
       lpCI->lpRecOrderTail = lpCI->lpRecOrderHead;
   }
   else if (lpOrderRec == lpCI->lpRecOrderTail) // Current node is tail
   {
     lpCI->lpRecOrderTail = (LPZORDERREC) 
       LLRemoveTail( (LPLINKNODE) lpCI->lpRecOrderTail, ((LPLINKNODE FAR *)&lpTemp));
     if (!lpCI->lpRecOrderTail)
       lpCI->lpRecOrderHead = lpCI->lpRecOrderTail;
   }
   else                                         // Current node is internal
   {
     lpTemp = (LPZORDERREC) LLRemoveNode( (LPLINKNODE) lpOrderRec);
   }

   return lpTemp;
}

/****************************************************************************
 * CreateRecOrderList
 *
 * Purpose:
 *  Creates the record order list.  Initially in the order of the records.
 *
 * Parameters:
 *  HWND hCntWnd - handle to the container
 *
 * Return Value:
 *  BOOL          TRUE if list was successfully destroyed
 ****************************************************************************/
BOOL CreateRecOrderList( HWND hCntWnd )
{
   LPCNTINFO    lpCI = (GETCNT(hCntWnd))->lpCI;
   LPRECORDCORE lpRec;
   LPZORDERREC  lpOrderRec;

   lpRec = lpCI->lpRecHead;

   // create the record order linked list 
   while (lpRec)
   {
     lpOrderRec = CntNewOrderRec();
     lpOrderRec->lpRec = lpRec;
     
     // add the node
     CntAddRecOrderTail( hCntWnd, lpOrderRec );

     // advance to the next record
     lpRec = lpRec->lpNext;
   }

   return TRUE;
}

/****************************************************************************
 * CntKillRecOrderLst
 *
 * Purpose:
 *  Destroys container's record order list.
 *  All memory for the list is also freed.
 *
 * Parameters:
 *  HWND hCntWnd - handle to the container
 *
 * Return Value:
 *  BOOL          TRUE if list was successfully destroyed
 ****************************************************************************/
BOOL CntKillRecOrderLst( HWND hCntWnd )
{
    LPZORDERREC lpTemp;
    LPCNTINFO   lpCI = (GETCNT(hCntWnd))->lpCI;

    while( lpCI->lpRecOrderHead )
    {
      lpCI->lpRecOrderHead = (LPZORDERREC) 
         LLRemoveHead( (LPLINKNODE) lpCI->lpRecOrderHead, ((LPLINKNODE FAR *)&lpTemp) );
      if( lpTemp )
        CntFreeOrderRec( lpTemp );
    }

    lpCI->lpRecOrderTail = NULL;

    return TRUE;
}

/****************************************************************************
 * CalculateIconWorkspace
 *
 * Purpose:
 *   Calculates the size of the icon workspace - this is the smallest rectangle
 *   that bounds all the icon records.  
 *
 * Parameters:
 *  LPCONTAINER lpCnt  - pointer to the container
 *
 * Return Value:
 *  TRUE if the workspace changed
 ****************************************************************************/
BOOL CalculateIconWorkspace( HWND hWnd )
{
    LPCONTAINER  lpCnt = GETCNT(hWnd);
    HDC          hDC;
    HFONT        hFontOld;
    LPRECORDCORE lpRec;
    RECT         rcText;
    int          hMin, hMax, iStart;
    int          vMin, vMax;
    BOOL         bChanged=FALSE;

    SetRectEmpty ( &rcText );

    // start at the head of the list - initialize the horizontal and vertical min and max
    lpRec = lpCnt->lpCI->lpRecHead;
    if (lpRec)
    {
      hMin = lpRec->ptIconOrg.x;
      vMin = lpRec->ptIconOrg.y;
      hMax = hMin + lpCnt->lpCI->ptBmpOrIcon.x;
      vMax = vMin + lpCnt->lpCI->ptBmpOrIcon.y;
    }
    else
    {
      hMin = 0;
      vMin = 0;
      hMax = 0;
      vMax = 0;
    }

    // get the DC for the window
    hDC = GetDC( hWnd );
    
    // select the font into the DC so metrics are calculated accurately
    if( lpCnt->lpCI->hFont )
      hFontOld = SelectObject( hDC, lpCnt->lpCI->hFont );
    else
      hFontOld = SelectObject( hDC, GetStockObject( lpCnt->nDefaultFont ) );

    // traverse the list
    while (lpRec)
    {
      // get the texts' bounding rectangle
      if (!(lpCnt->lpCI->aiInfo.aiType & CAI_FIXED))
      {
        if (lpRec->lpszIcon)
        {
          CreateIconTextRect( hDC, lpCnt, lpRec->ptIconOrg, lpRec->lpszIcon, &rcText );

          // inflate the rectangle by 1 to encompass focus rects and sel rects
          InflateRect( &rcText, 1, 1 );
        }
        else
        {
          rcText.bottom = rcText.top  + 1;
          rcText.right  = rcText.left + 1;
        }
      }
        
      // set the new horizontal and vertical min and max
      if (lpCnt->lpCI->aiInfo.aiType & CAI_FIXED)
      {
        // if fixed rectangle the size of the icon rect is defined - the text may
        // extend beyond these boundaries, yet the min and max are defined by the
        // user defined values, not the actual text rectangle.  iStart represents
        // the left edge of the rectangle
        iStart = lpRec->ptIconOrg.x + lpCnt->lpCI->ptBmpOrIcon.x / 2 - 
                                       lpCnt->lpCI->aiInfo.nWidth / 2;
        iStart = min( iStart, lpRec->ptIconOrg.x );

        if ( iStart < hMin )
          hMin = iStart;
        if ( iStart + max(lpCnt->lpCI->aiInfo.nWidth, lpCnt->lpCI->ptBmpOrIcon.x) > hMax )
          hMax = iStart + max(lpCnt->lpCI->aiInfo.nWidth, lpCnt->lpCI->ptBmpOrIcon.x);
        if ( lpRec->ptIconOrg.y < vMin )
          vMin = lpRec->ptIconOrg.y;
        if ( lpRec->ptIconOrg.y + max (lpCnt->lpCI->aiInfo.nHeight, lpCnt->lpCI->ptBmpOrIcon.y) > vMax )
          vMax = lpRec->ptIconOrg.y + max(lpCnt->lpCI->aiInfo.nHeight, lpCnt->lpCI->ptBmpOrIcon.y);
      }
      else if (lpCnt->lpCI->aiInfo.aiType & CAI_FIXEDWIDTH)
      {
        // width is determined by user defined value - height is determined by the 
        // height of the text rectangle
        iStart = lpRec->ptIconOrg.x + lpCnt->lpCI->ptBmpOrIcon.x / 2 - 
                                       lpCnt->lpCI->aiInfo.nWidth / 2;
        iStart = min( iStart, lpRec->ptIconOrg.x );

        if ( iStart < hMin )
          hMin = iStart;
        if ( iStart + max(lpCnt->lpCI->aiInfo.nWidth, lpCnt->lpCI->ptBmpOrIcon.x) > hMax )
          hMax = iStart + max(lpCnt->lpCI->aiInfo.nWidth, lpCnt->lpCI->ptBmpOrIcon.x);
        if ( lpRec->ptIconOrg.y < vMin )
          vMin = lpRec->ptIconOrg.y;
        if ( lpRec->ptIconOrg.y + lpCnt->lpCI->ptBmpOrIcon.y + (rcText.bottom - rcText.top) > vMax )
          vMax = lpRec->ptIconOrg.y + lpCnt->lpCI->ptBmpOrIcon.y + (rcText.bottom - rcText.top);
      }
      else // CAI_LARGEST
      {
        // min and maxs determined from the text rect
        if ( min(lpRec->ptIconOrg.x, rcText.left) < hMin )
          hMin = min(lpRec->ptIconOrg.x, rcText.left);
        if ( max(rcText.right, lpRec->ptIconOrg.x + lpCnt->lpCI->ptBmpOrIcon.x) > hMax )
          hMax = max(rcText.right, lpRec->ptIconOrg.x + lpCnt->lpCI->ptBmpOrIcon.x);
        if ( lpRec->ptIconOrg.y < vMin )
          vMin = lpRec->ptIconOrg.y;
        if ( lpRec->ptIconOrg.y + lpCnt->lpCI->ptBmpOrIcon.y + (rcText.bottom - rcText.top) > vMax )
          vMax = lpRec->ptIconOrg.y + lpCnt->lpCI->ptBmpOrIcon.y + (rcText.bottom - rcText.top);
      }
        
      // advance to the next record
      lpRec = lpRec->lpNext;
    }
      
    // release the DC
    SelectObject( hDC, hFontOld );
    ReleaseDC( hWnd, hDC );
    
    // determine if the workspace changed
    if (lpCnt->lpCI->rcWorkSpace.top    != vMin ||
        lpCnt->lpCI->rcWorkSpace.bottom != vMax ||
        lpCnt->lpCI->rcWorkSpace.left   != hMin ||
        lpCnt->lpCI->rcWorkSpace.right  != hMax)
         bChanged = TRUE;
    
    // set icon workspace rectangle
    lpCnt->lpCI->rcWorkSpace.top    = vMin;
    lpCnt->lpCI->rcWorkSpace.bottom = vMax;
    lpCnt->lpCI->rcWorkSpace.left   = hMin;
    lpCnt->lpCI->rcWorkSpace.right  = hMax;

    return bChanged;
}

/****************************************************************************
 * AdjustIconWorkspace
 *
 * Purpose:
 *   Adjusts the size of the icon workspace. If a record is added 
 *   this function is called to see if the workspace changed.
 *
 * Parameters:
 *  HWND         hWnd   - handle to the container window
 *  LPCONTAINER  lpCnt  - pointer to the container
 *  LPRECORDCORE lpRec  - pointer to the record being added 
 *
 * Return Value:
 *  TRUE if icon workspace changed, FALSE if not affected
 ****************************************************************************/
BOOL AdjustIconWorkspace( HWND hWnd, LPCONTAINER lpCnt, LPRECORDCORE lpRec )
{
  HDC   hDC;
  HFONT hFontOld;
  RECT  rcText;
  BOOL  bChanged = FALSE;

  // Get the DC
  hDC = GetDC( hWnd );
  
  // select the font into the DC so metrics are calculated accurately
  if( lpCnt->lpCI->hFont )
    hFontOld = SelectObject( hDC, lpCnt->lpCI->hFont );
  else
    hFontOld = SelectObject( hDC, GetStockObject( lpCnt->nDefaultFont ) );

  // create the text rectangle
  CreateIconTextRect( hDC, lpCnt, lpRec->ptIconOrg, lpRec->lpszIcon, &rcText );

  // see if this record extends the workspace rect
  if (lpRec->ptIconOrg.y < lpCnt->lpCI->rcWorkSpace.top)
  {
    lpCnt->lpCI->rcWorkSpace.top = lpRec->ptIconOrg.y;
    bChanged = TRUE;
  }
  if (rcText.bottom > lpCnt->lpCI->rcWorkSpace.bottom)
  {
    lpCnt->lpCI->rcWorkSpace.bottom = rcText.bottom;
    bChanged = TRUE;
  }
  if (min(rcText.left,lpRec->ptIconOrg.x) < lpCnt->lpCI->rcWorkSpace.left)
  {
    lpCnt->lpCI->rcWorkSpace.left = min(rcText.left,lpRec->ptIconOrg.x);
    bChanged = TRUE;
  }
  if (max(rcText.right,lpRec->ptIconOrg.x + lpCnt->lpCI->ptBmpOrIcon.x) > lpCnt->lpCI->rcWorkSpace.right)
  {
    lpCnt->lpCI->rcWorkSpace.right  = max(rcText.right,lpRec->ptIconOrg.x + lpCnt->lpCI->ptBmpOrIcon.x);
    bChanged = TRUE;
  }

  // release the DC
  SelectObject( hDC, hFontOld );
  ReleaseDC( hWnd, hDC );
  
  return bChanged;
}

/****************************************************************************
 * CreateIconTextRect
 *
 * Purpose:
 *  
 *
 * Parameters:
 *  LPCONTAINER lpCnt     - pointer to the container
 *  LPSTR       szText    - text string for the icon text
 *  POINT       ptIconOrg - pt of the records upper left corner of its icon
 *  LPSTR       szText    - records icon string
 *  RECT *      lprect    - bounding rectangle (returned)
 *
 * Return Value:
 *  TRUE if ok.
 ****************************************************************************/
BOOL CreateIconTextRect( HDC hDC, LPCONTAINER lpCnt, POINT ptIconOrg, LPSTR szText, RECT *lpRect )
{
  int nWidth;

  // setup the top and left of the rect - will adjust the left after width and height 
  // are calculated
  lpRect->left = ptIconOrg.x;
  lpRect->top  = ptIconOrg.y + lpCnt->lpCI->ptBmpOrIcon.y + 2;

  // DrawText with DT_CALCRECT calculates the width and height of the bounding rectangle
  if (lpCnt->lpCI->aiInfo.aiType & CAI_WORDWRAP)
  {
    // have to specify the width of the rectangle before calculating.
    lpRect->right = lpRect->left + lpCnt->lpCI->aiInfo.nWidth;

    DrawText( hDC, szText, lstrlen(szText), lpRect, 
              DT_NOPREFIX | DT_TOP | DT_CENTER | DT_WORDBREAK | DT_CALCRECT );
  }
  else
    DrawText( hDC, szText, lstrlen(szText), lpRect, 
              DT_NOPREFIX | DT_TOP | DT_CENTER | DT_CALCRECT );
  
  // center the rectangle underneath the icon.  Note that the rect passed in has the 
  // bottom left point of the icon as the upper left of the rect.
  nWidth = lpRect->right - lpRect->left;
  lpRect->left += (lpCnt->lpCI->ptBmpOrIcon.x / 2) - (nWidth / 2);
  lpRect->right = lpRect->left + nWidth;

  return TRUE;
}

/****************************************************************************
 * IcnAdjustHorizScroll
 *
 * Purpose:
 *  Adjust the horizontal scroll bar
 *
 * Parameters:
 *  HWND        hWnd   - handle to the container
 *  LPCONTAINER lpCnt  - pointer to the container
 *
 * Return Value:
 *  none.
 ****************************************************************************/
VOID IcnAdjustHorzScroll( HWND hWnd, LPCONTAINER lpCnt )
{
    int nLeftPxls, nRightPxls;

    // the max scrolling range is the number of pixels the icon workspace extends
    // past the viewspace to the left plus to the right
    if (lpCnt->lpCI->bArranged)
      nLeftPxls  = max(0, lpCnt->lpCI->rcViewSpace.left  - 
                         (lpCnt->lpCI->rcWorkSpace.left - lpCnt->lpCI->aiInfo.xIndent));
    else  
      nLeftPxls  = max(0, lpCnt->lpCI->rcViewSpace.left  - lpCnt->lpCI->rcWorkSpace.left);
    nRightPxls = max(0, lpCnt->lpCI->rcWorkSpace.right - lpCnt->lpCI->rcViewSpace.right);
    lpCnt->nHscrollMax = nLeftPxls + nRightPxls;
    lpCnt->nHscrollPos = nLeftPxls;

    SetScrollPos  ( hWnd, SB_HORZ,    lpCnt->nHscrollPos, FALSE  );
    SetScrollRange( hWnd, SB_HORZ, 0, lpCnt->nHscrollMax, TRUE );

    return;
}

/****************************************************************************
 * FindClickedRecordIcn
 *
 * Purpose:
 *  Find the record in the container that the user clicked on.
 *
 * Parameters:
 *  HWND        hWnd   - handle to the container
 *  LPCONTAINER lpCnt  - pointer to the container
 *  POINT       pt     - point in client coordinates where the user clicked
 *
 * Return Value:
 *  none.
 ****************************************************************************/
LPZORDERREC FindClickedRecordIcn( HWND hWnd, LPCONTAINER lpCnt, POINT pt )
{
  LPZORDERREC  lpOrderRec;
  HDC          hDC;
  HFONT        hFontOld;
  BOOL         bFound;
  RECT         rcIcon, rcText;
  int          nIconWidth, nIconHeight;
  int          xOff, yOff;

  // get the width and height of the icons being used
  nIconWidth  = lpCnt->lpCI->ptBmpOrIcon.x;
  nIconHeight = lpCnt->lpCI->ptBmpOrIcon.y;
  
  // Get the DC
  hDC = GetDC( hWnd );
  
  // select the font into the DC so metrics are calculated accurately
  if( lpCnt->lpCI->hFont )
    hFontOld = SelectObject( hDC, lpCnt->lpCI->hFont );
  else
    hFontOld = SelectObject( hDC, GetStockObject( lpCnt->nDefaultFont ) );

  // offset from viewspace
  xOff = -lpCnt->lpCI->rcViewSpace.left;
  yOff = -lpCnt->lpCI->rcViewSpace.top + lpCnt->lpCI->cyTitleArea;

  // search for the record clicked on - search in the order of the z-order so
  // the topmost record is found first
  lpOrderRec = lpCnt->lpCI->lpRecOrderHead;
  while (lpOrderRec)
  {
    // see if the user clicked on the records icon rectangle
    rcIcon.left   = xOff + lpOrderRec->lpRec->ptIconOrg.x;
    rcIcon.top    = yOff + lpOrderRec->lpRec->ptIconOrg.y;
    rcIcon.right  = rcIcon.left + nIconWidth;
    rcIcon.bottom = rcIcon.top  + nIconHeight + 2; // add 2 in case they click on space between the icon and the text
    if( PtInRect(&rcIcon, pt) )
    {
      bFound = TRUE;
      break;
    }

    // see if the user clicked on the record's text rectangle
    if (lpOrderRec->lpRec->lpszIcon)
    {
      // create the text rectangle
      CreateIconTextRect( hDC, lpCnt, lpOrderRec->lpRec->ptIconOrg, lpOrderRec->lpRec->lpszIcon, &rcText );

      // offset the rect
      OffsetRect( &rcText, xOff, yOff );
      
      // inflate the rectangle by 1 to encompass focus rects and sel rects
      InflateRect( &rcText, 1, 1 );

      if( PtInRect(&rcText, pt) )
      {
        bFound = TRUE;
        break;
      }
    }
    
    // advance to the next record
    lpOrderRec = lpOrderRec->lpNext;
  }

  // release the DC
  SelectObject( hDC, hFontOld );
  ReleaseDC( hWnd, hDC );
  
  // return the order record if found, otherwise return NULL
  if (bFound)
    return lpOrderRec;
  else
    return NULL;
}

/****************************************************************************
 * InvalidateCntRecIcon
 *
 * Purpose:
 *  Invalidates and repaints (maybe) a container record in icon view
 *
 *      Record Ptr   Action
 *      ----------   ------------------------------------
 *        valid      repaints a container record
 *        NULL       repaints entire container record area
 *
 * Parameters:
 *  HWND          hWnd        - handle to the control window.
 *  LPCONTAINER   lpCnt       - pointer to existing CONTAINER structure.
 *  LPRECORDCORE  lpRec       - pointer to record to be repainted
 *  UINT          fMode       - indicating what type of action to take:
 *                              0: no action (just see if the rec is in display area)
 *                              1: invalidate target but DO NOT update
 *                              2: invalidate target and update immediately
 *
 * Return Value:
 *   TRUE if everything OK
 ****************************************************************************/
int WINAPI InvalidateCntRecIcon( HWND hWnd, LPCONTAINER lpCnt, LPRECORDCORE lpRec, UINT fMode )
{
    RECT  rcIcon, rcIcona;
    RECT  rcText;
    BOOL  bIconFound = FALSE;
    BOOL  bTextFound = FALSE;
    HDC   hDC;
    HFONT hFontOld;
    int   xOff, yOff;
    LPZORDERREC lpOrderRec;

    // Make sure we have a window and container struct.
    if( !hWnd || !lpCnt )
      return -1;

    if( lpRec )
    {
      // get the DC
      hDC = GetDC( hWnd );

      // select the font into the DC so metrics are calculated accurately
      if( lpCnt->lpCI->hFont )
        hFontOld = SelectObject( hDC, lpCnt->lpCI->hFont );
      else
        hFontOld = SelectObject( hDC, GetStockObject( lpCnt->nDefaultFont ) );

      // offsets
      xOff = -lpCnt->lpCI->rcViewSpace.left;
      yOff = -lpCnt->lpCI->rcViewSpace.top + lpCnt->lpCI->cyTitleArea;

      // define the icon rect - we only invalidate the icon if it is covered by 
      // another icon or icon's text
      rcIcon.left   = lpRec->ptIconOrg.x;
      rcIcon.top    = lpRec->ptIconOrg.y;
      rcIcon.right  = rcIcon.left + lpCnt->lpCI->ptBmpOrIcon.x;
      rcIcon.bottom = rcIcon.top  + lpCnt->lpCI->ptBmpOrIcon.y;

      // determine if the icon is covered - only invalidate an icon if it is covered
      // and in the viewspace
      if ( WithinRect( &rcIcon, &lpCnt->lpCI->rcViewSpace ) )
      {
        lpOrderRec = lpCnt->lpCI->lpRecOrderHead;
        while ( lpOrderRec )
        {
          if ( lpOrderRec->lpRec != lpRec )
          {
            // define the icon rect
            rcIcona.left   = lpOrderRec->lpRec->ptIconOrg.x;
            rcIcona.top    = lpOrderRec->lpRec->ptIconOrg.y;
            rcIcona.right  = rcIcona.left + lpCnt->lpCI->ptBmpOrIcon.x;
            rcIcona.bottom = rcIcona.top  + lpCnt->lpCI->ptBmpOrIcon.y;
        
            // define the text rect
            CreateIconTextRect( hDC, lpCnt, lpOrderRec->lpRec->ptIconOrg, 
                                lpOrderRec->lpRec->lpszIcon, &rcText );
            InflateRect( &rcText, 1, 1 );

            // see if being covered and in the view area
            if ( WithinRect( &rcIcon, &rcIcona ) ||
                 WithinRect( &rcIcon, &rcText ) )
            {
              bIconFound = TRUE;

              // adjust the rectangle to client coords
              OffsetRect( &rcIcon, xOff, yOff );

              rcIcon.top = max( rcIcon.top, lpCnt->lpCI->cyTitleArea );

              break;
            }
          }

          // advance to the next record
          lpOrderRec = lpOrderRec->lpNext;
        }
      }

      // define the text rect and see if it is in the display area
      if (lpRec->lpszIcon)
      {
        CreateIconTextRect( hDC, lpCnt, lpRec->ptIconOrg, lpRec->lpszIcon, &rcText );

        // inflate the rectangle by 1 to encompass focus rects and sel rects
        InflateRect( &rcText, 1, 1 );

        if ( WithinRect( &rcText, &lpCnt->lpCI->rcViewSpace ) )
        {
          bTextFound = TRUE;
         
          // adjust the rectangle to client coords
          OffsetRect( &rcText, xOff, yOff );

          rcText.top = max( rcText.top, lpCnt->lpCI->cyTitleArea );
        }
      }
      else
        bTextFound = FALSE;

      if( fMode == 1 )
      {
        if ( bIconFound )
          InvalidateRect( hWnd, &rcIcon, FALSE );
        if ( bTextFound )
          InvalidateRect( hWnd, &rcText, FALSE );
      }
      else if( fMode == 2 )
      {
        if ( bIconFound )
          UpdateContainer( hWnd, &rcIcon, TRUE );
        if ( bTextFound )
          UpdateContainer( hWnd, &rcText, TRUE );
      }
    
      // release the DC
      SelectObject ( hDC, hFontOld );
      ReleaseDC( hWnd, hDC );
    }
    else
    {
       // Set to the entire width of the container.
       rcText.left  = 0;
       rcText.right = lpCnt->cxClient;

       // Set the top and bottom of the record area.
       rcText.top    = lpCnt->lpCI->cyTitleArea;
       rcText.bottom = lpCnt->cyClient;

       if( fMode == 1 )
         InvalidateRect( hWnd, &rcText, FALSE );
       else if( fMode == 2 )
         UpdateContainer( hWnd, &rcText, TRUE );
    }

    if ( bIconFound || bTextFound )
      return TRUE;
    else
      return  -1;
}

/****************************************************************************
 * SwipeSelection
 *
 * Purpose:
 *  Performs swipe record selection for Extended selection in Icon view.
 *  Does all the necessary mouse tracking and record selection.
 *
 * Parameters:
 *  HWND          hWnd        - Container child window handle 
 *  LPCONTAINER   lpCnt       - pointer to window's CONTAINER structure.
 *
 * Return Value:
 *  none.
 ****************************************************************************/
VOID NEAR SwipeSelection( HWND hWnd, LPCONTAINER lpCnt )
{
    LPZORDERREC  lpOrderRec;
    LPRECORDCORE lpCurRec=NULL;
    RECT         rect;
    HDC          hDC;
    MSG          msgModal;
    HCURSOR      hOldCursor;
    BOOL         bTracking=TRUE;
    POINT        pt;
#ifdef WIN32
    POINTS  pts;
#endif

    // Set up a constraining rect for the cursor - cursor cannot leave this rectangle
    pt.x = lpCnt->rectRecArea.left;
    pt.y = lpCnt->rectRecArea.top;
    ClientToScreen( hWnd, &pt );
    rect.left = pt.x - lpCnt->lpCI->cxChar2;
    rect.top  = pt.y - lpCnt->lpCI->cyRow2;
    pt.x = lpCnt->rectRecArea.right;
    pt.y = lpCnt->cyClient;
    ClientToScreen( hWnd, &pt );
    rect.right  = pt.x + lpCnt->lpCI->cxChar + lpCnt->lpCI->cxChar4;
    rect.bottom = pt.y + lpCnt->lpCI->cyRow2;
    ClipCursor( &rect );

    // Turn on the timer so we can generate WM_MOUSEMOVE msgs if
    // the user drags the cursor out of the record area. The generated
    // MOUSEMOVE msgs will be sent here so we can generate scroll msgs.
    SetTimer( hWnd, IDT_AUTOSCROLL, CTICKS_AUTOSCROLL, (TIMERPROC)CntTimerProc );

    hDC = GetDC( hWnd );
    SetCapture( hWnd );
    hOldCursor = SetCursor( LoadCursor( NULL, IDC_ARROW ) );
    StateSet( lpCnt, CNTSTATE_CLICKED );

    // While the cursor is being dragged get mouse msgs.
    while( bTracking )
    {
      while( !PeekMessage( &msgModal, NULL, 0, 0, PM_REMOVE ) )
        WaitMessage();

      switch( msgModal.message )
      {
        case WM_MOUSEMOVE:
          // Get new mouse position and see if it is on a record
#ifdef WIN32
          pts = MAKEPOINTS(msgModal.lParam); 
          pt.x = pts.x;
          pt.y = pts.y;
#else
          pt.x = LOWORD(msgModal.lParam);
          pt.y = HIWORD(msgModal.lParam);
#endif         
          
          // see if in the record area - if not, do a scroll
          if( PtInRect( &lpCnt->rectRecArea, pt ) )
          {
            // see if the mouse is over a record - if it is, this function will return
            // the record, otherwise it returns NULL
            lpOrderRec = FindClickedRecordIcn( hWnd, lpCnt, pt );
        
            if( lpOrderRec )
            {
              // we are over a record - if we are still over the same record we were
              // the last time we processed a mouse move, do nothing, otherwise select
              // or deselect the record
              if ( lpOrderRec->lpRec != lpCurRec )
              {
                // set our current record
                lpCurRec = lpOrderRec->lpRec;

                // toggle the selected flag for this record.
                if( lpCurRec->flRecAttr & CRA_SELECTED )
                {
                  lpCurRec->flRecAttr &= ~CRA_SELECTED;

                  // repaint the record
                  InvalidateCntRecord( hWnd, lpCnt, lpCurRec, NULL, 2 );

                  // Tell the assoc wnd if a record was unselected.
                  NotifyAssocWnd( hWnd, lpCnt, CN_RECUNSELECTED, 0, lpCurRec, NULL, 0, 0, 0, NULL );
                }
                else
                {
                  lpCurRec->flRecAttr |= CRA_SELECTED;

                  // repaint the record
                  InvalidateCntRecord( hWnd, lpCnt, lpCurRec, NULL, 2 );

                  // Tell the assoc wnd if a record selected.
                  NotifyAssocWnd( hWnd, lpCnt, CN_RECSELECTED, 0, lpCurRec, NULL, 0, 0, 0, NULL );
                }
              }
            }
            else
            {
              // set lpCurRec to NULL
              lpCurRec = NULL;
            }
          }
          else
          {
            // out of record area, so do an autoscroll
            if( pt.y < lpCnt->rectRecArea.top )
            {
              if( lpCnt->dwStyle & CTS_VERTSCROLL )
              {
                // Scroll up
                FORWARD_WM_VSCROLL( hWnd, 0, SB_LINEUP, 0, SendMessage );
              }
            }
            else if( pt.y > lpCnt->rectRecArea.bottom )
            {
              if( lpCnt->dwStyle & CTS_VERTSCROLL )
              {
                // Scroll down 
                FORWARD_WM_VSCROLL( hWnd, 0, SB_LINEDOWN, 0, SendMessage );
              }
            }

            if( pt.x < lpCnt->rectRecArea.left )
            {
              if( lpCnt->dwStyle & CTS_HORZSCROLL )
              {
                // Scroll left 
                FORWARD_WM_HSCROLL( hWnd, 0, SB_LINEUP, 0, SendMessage );
              }
            }
            else if( pt.x > lpCnt->rectRecArea.right )
            {
              if( lpCnt->dwStyle & CTS_HORZSCROLL )
              {
                // Scroll right
                FORWARD_WM_HSCROLL( hWnd, 0, SB_LINEDOWN, 0, SendMessage );
              }
            }
          }

          // Don't dispatch this msg since we just processed it.
          continue;
          break;

        case WM_LBUTTONUP:           // End of swipe select
          // Break out of tracking loop.
          bTracking = FALSE;  

          // Don't dispatch this msg since we just processed it.
          continue;
          break;
      }

      TranslateMessage( &msgModal );
      DispatchMessage( &msgModal );
    }

    // Restore all the system states.
    StateClear( lpCnt, CNTSTATE_CLICKED );
    ReleaseCapture();
    KillTimer( hWnd, IDT_AUTOSCROLL );
    ClipCursor( NULL );
    SetCursor( hOldCursor );
    ReleaseDC( hWnd, hDC );

    return;
}

/****************************************************************************
 * IcnIsFocusInView
 *
 * Purpose:
 *  Determines whether the focus cell is viewable in the display area.  This
 *  is for icon view.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *  LPCONTAINER   lpCnt       - pointer to container structure
 *
 * Return Value:
 *  BOOL          TRUE if any part of the focus cell is viewable
 *                FALSE if no part of the focus cell is viewable
 ****************************************************************************/
BOOL WINAPI IcnIsFocusInView( HWND hWnd, LPCONTAINER lpCnt )
{
    BOOL  bIsInView=FALSE;
    RECT  rcText;
    HDC   hDC;
    HFONT hFontOld;

    if( lpCnt->lpCI->lpFocusRec )
    {
      // get the DC
      hDC = GetDC( hWnd );

      // select the font into the DC so metrics are calculated accurately
      if( lpCnt->lpCI->hFont )
        hFontOld = SelectObject( hDC, lpCnt->lpCI->hFont );
      else
        hFontOld = SelectObject( hDC, GetStockObject( lpCnt->nDefaultFont ) );

      // create the text rectangle for the record
      CreateIconTextRect( hDC, lpCnt, lpCnt->lpCI->lpFocusRec->ptIconOrg, 
                          lpCnt->lpCI->lpFocusRec->lpszIcon, &rcText );

      // inflate the rectangle by 1 to encompass focus rects and sel rects
      InflateRect( &rcText, 1, 1 );

      // if within viewspace, then return true
      if ( WithinRect( &rcText, &lpCnt->lpCI->rcViewSpace ) )
        bIsInView = TRUE;

      // release the DC
      SelectObject ( hDC, hFontOld );
      ReleaseDC( hWnd, hDC );
    }

    return bIsInView;
}

/****************************************************************************
 * IcnScrollToFocus
 *
 * Purpose:
 *  Scrolls the focus record into view. Is called if the focus record is not
 *  in view and a keyboard char is pressed. The scrolling should occur
 *  prior to the desired keyboard action.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *  LPCONTAINER   lpCnt       - pointer to container structure
 *
 * Return Value:
 *  VOID
 ****************************************************************************/
VOID WINAPI IcnScrollToFocus( HWND hWnd, LPCONTAINER lpCnt )
{
    LPCONTAINER  lpCntFrame = GETCNT( lpCnt->hWndFrame );
    int          nInc=0;
    HDC          hDC;
    RECT         rcText;
    HFONT        hFontOld;

    // If the scroll-to-focus behavior is disabled just return.
    if( !lpCntFrame->bScrollFocusRect )
      return;

    if( lpCnt->lpCI->lpFocusRec )
    {
      // get the DC
      hDC = GetDC( hWnd );

      // select the font into the DC so metrics are calculated accurately
      if( lpCnt->lpCI->hFont )
        hFontOld = SelectObject( hDC, lpCnt->lpCI->hFont );
      else
        hFontOld = SelectObject( hDC, GetStockObject( lpCnt->nDefaultFont ) );

      // create the text rectangle for the record
      CreateIconTextRect( hDC, lpCnt, lpCnt->lpCI->lpFocusRec->ptIconOrg, 
                          lpCnt->lpCI->lpFocusRec->lpszIcon, &rcText );

      // inflate the rectangle by 1 to encompass focus rects and sel rects
      InflateRect( &rcText, 1, 1 );

      // get horizontal scrolling increment
      if ( rcText.left > lpCnt->lpCI->rcViewSpace.right )
      {
        // scroll to the right
        nInc = rcText.right - lpCnt->lpCI->rcViewSpace.right;
      }
      else if (rcText.right < lpCnt->lpCI->rcViewSpace.left )
      {
        // scroll to the left
        nInc = rcText.left - lpCnt->lpCI->rcViewSpace.left;
      }
      
      // scroll horizontally
      if ( nInc )
      {
        nInc += lpCnt->nHscrollPos;
        FORWARD_WM_HSCROLL( hWnd, 0, SB_THUMBPOSITION, nInc, SendMessage );
      }

      // get vertical scrolling increment
      nInc = 0;
      if ( rcText.bottom < lpCnt->lpCI->rcViewSpace.top )
      {
        // scroll up
        nInc = (rcText.top - lpCnt->lpCI->ptBmpOrIcon.y - 2) - lpCnt->lpCI->rcViewSpace.top;
      }
      else if ( rcText.top > lpCnt->lpCI->rcViewSpace.bottom )
      {
        // scroll down
        nInc = rcText.bottom - lpCnt->lpCI->rcViewSpace.bottom;
      }
      
      // scroll vertically
      if ( nInc )
      {
        nInc += lpCnt->nVscrollPos;
        if( StateTest( lpCnt, CNTSTATE_VSCROLL32 ) )
          FORWARD_WM_VSCROLL( hWnd, nInc, SB_THUMBPOSITION, nInc, SendMessage );
        else
          FORWARD_WM_VSCROLL( hWnd, 0, SB_THUMBPOSITION, nInc, SendMessage );
      }
    
      // release the DC
      SelectObject ( hDC, hFontOld );
      ReleaseDC( hWnd, hDC );
    }
    else
    {
      // If not found it means CA_OWNERVSCROLL is active or we are in a
      // DELTA model and the focus record is not currently in memory.
      // In either case the app knows where the focus record is and will
      // build a new list with the focus record at the head.
      NotifyAssocWnd( hWnd, lpCnt, CN_QUERYFOCUS, 0, NULL, NULL, 0, 0, 0, NULL );
    }

    return;
}

/****************************************************************************
 * FindNextRecIcn
 *
 * Purpose:
 *  Finds the closest record to the focus record depending on the key code:
 *   VK_UP    - closest record above that is within xDelta (xDelta is a value
 *              that specifies how close horizontally the record has to be to be
 *              considered)
 *   VK_DOWN  - closest record below that is within xDelta 
 *   VK_LEFT  - closest record left that is within yDelta 
 *   VK_RIGHT - closest record right that is within yDelta 
 *   VK_HOME  - closest record to the top left
 *   VK_END   - closest record to the bottom right
 *   VK_PRIOR - closest record to the top within xDelta
 *   VK_NEXT  - closest record to the bottom within xDelta
 *
 * Parameters:
 *  HWND         hWnd       - handle to the container
 *  LPCONTAINER  lpCnt      - pointer to container structure
 *  LPRECORDCORE lpFocusRec - pointer to the focus record
 *  UINT         vkCode     - virtual key code (key user pressed)
 *
 * Return Value:
 *  The closest record - NULL if no record was found
 ****************************************************************************/
LPRECORDCORE FindNextRecIcn( HWND hWnd, LPCONTAINER lpCnt, LPRECORDCORE lpFocusRec, UINT vkCode )
{
  LPRECORDCORE lpRec;
  LPRECORDCORE lpNextRec=NULL;
  int          xClosest, yClosest;
  int          xDist, yDist;
  int          xDelta, yDelta;
  HDC          hDC;
  RECT         rcText;
  HFONT        hFontOld;

  // these values represent how close an icon has to be to the record 
  // to be considered as a possible record to move to
  xDelta = 2 * lpCnt->lpCI->ptBmpOrIcon.x;
  yDelta = 2 * lpCnt->lpCI->ptBmpOrIcon.y;
  
  // initialize y closest so far - if moving sideways, only go to an icon within yDelta
  if (vkCode == VK_LEFT || vkCode == VK_RIGHT)
    yClosest = yDelta;
  else
    yClosest = lpCnt->cyClient;
  
  // initialize X closest so far - if moving up and down, only go to an icon within xDelta
  if (vkCode == VK_UP || vkCode == VK_DOWN || vkCode == VK_PRIOR || vkCode == VK_NEXT)
    xClosest = xDelta;
  else
    xClosest = lpCnt->cxClient;
  
  // if paging up or down, I am returning the closest record to either the top
  // or the bottom of the viewspace.  I have to check if the text rect is within the 
  // viewspace to do this, therefore I must get the container DC
  if (vkCode==VK_NEXT || vkCode==VK_PRIOR || vkCode==VK_HOME || vkCode==VK_END)
  {
    hDC = GetDC( hWnd );

    // select the font into the DC so metrics are calculated accurately
    if( lpCnt->lpCI->hFont )
      hFontOld = SelectObject( hDC, lpCnt->lpCI->hFont );
    else
      hFontOld = SelectObject( hDC, GetStockObject( lpCnt->nDefaultFont ) );
  }
  
  // traverse the list and find the closest record either up, down, left, or right
  // depending on the key code.
  lpRec = lpCnt->lpCI->lpRecHead;
  while (lpRec)
  {
    switch (vkCode)
    {
      case VK_UP:
      case VK_DOWN:
        // find the closest record above this record
        if ((lpRec->ptIconOrg.y < lpFocusRec->ptIconOrg.y && vkCode==VK_UP) ||
            (lpRec->ptIconOrg.y > lpFocusRec->ptIconOrg.y && vkCode==VK_DOWN))
        {
          // find out how far away vertically the record is
          yDist = abs(lpRec->ptIconOrg.y - lpFocusRec->ptIconOrg.y);
          
          // find out how horizontally away the record is
          xDist = abs(lpRec->ptIconOrg.x - lpFocusRec->ptIconOrg.x);

          // only consider those within the delta
          if ( xDist < xDelta )
          {
            if ( xDist + yDist < xClosest + yClosest )
            {
              // set the new next rec
              lpNextRec = lpRec;
              xClosest  = xDist;
              yClosest  = yDist;
            }
          }
        }
        break;

      case VK_LEFT:
      case VK_RIGHT:
        // find the closest record to the left of this record if going left
        // or closest to the right if moving right
        if ((lpRec->ptIconOrg.x < lpFocusRec->ptIconOrg.x && vkCode==VK_LEFT) ||
            (lpRec->ptIconOrg.x > lpFocusRec->ptIconOrg.x && vkCode==VK_RIGHT))
        {
          // find out how horizontally away the record is
          xDist = abs(lpRec->ptIconOrg.x - lpFocusRec->ptIconOrg.x);

          // find out how far away vertically the record is
          yDist = abs(lpRec->ptIconOrg.y - lpFocusRec->ptIconOrg.y);
          
          // only consider those within the delta
          if ( yDist < yDelta )
          {
            if ( xDist + yDist < xClosest + yClosest )
            {
              // set the new next rec
              lpNextRec = lpRec;
              xClosest  = xDist;
              yClosest  = yDist;
            }
          }
        }
        break;

      case VK_NEXT:
        // find record closest to the bottom that is horizontally close to focus
        // create icon text rect and see if it is in view
        CreateIconTextRect( hDC, lpCnt, lpRec->ptIconOrg, lpRec->lpszIcon, &rcText );

        // inflate the rectangle by 1 to encompass focus rects and sel rects
        InflateRect( &rcText, 1, 1 );

        if ( WithinRect( &rcText, &lpCnt->lpCI->rcViewSpace ) )
        {
          // check to see if this is closest so far
          // find out how horizontally away the record is
          xDist = abs(lpRec->ptIconOrg.x - lpFocusRec->ptIconOrg.x);

          // find out how far away vertically the record is
          yDist = abs(lpCnt->lpCI->rcViewSpace.bottom - lpRec->ptIconOrg.y);
          
          // only consider those within the delta
          if ( xDist < xDelta )
          {
            if ( xDist + yDist < xClosest + yClosest )
            {
              // set the new next rec
              lpNextRec = lpRec;
              xClosest  = xDist;
              yClosest  = yDist;
            }
          }
        }
        break;

      case VK_END:
        // find record closest to the bottom that is closest to the right
        // create icon text rect and see if it is in view
        CreateIconTextRect( hDC, lpCnt, lpRec->ptIconOrg, lpRec->lpszIcon, &rcText );

        // inflate the rectangle by 1 to encompass focus rects and sel rects
        InflateRect( &rcText, 1, 1 );

        if ( WithinRect( &rcText, &lpCnt->lpCI->rcViewSpace ) )
        {
          // check to see if this is closest so far
          // find out how horizontally away the record is
          xDist = abs(lpCnt->lpCI->rcViewSpace.right - lpRec->ptIconOrg.x);

          // find out how far away vertically the record is
          yDist = abs(lpCnt->lpCI->rcViewSpace.bottom - lpRec->ptIconOrg.y);
          
          // only consider those within the delta
          if ( (yDist < yClosest) ||
               (yDist == yClosest && xDist < xClosest) )
          {
            // set the new next rec
            lpNextRec = lpRec;
            xClosest  = xDist;
            yClosest  = yDist;
          }
        }
        break;

      case VK_PRIOR:
        // find record closest to the top that is horizontally close to focus
        // create icon text rect and see if it is in view
        CreateIconTextRect( hDC, lpCnt, lpRec->ptIconOrg, lpRec->lpszIcon, &rcText );

        // inflate the rectangle by 1 to encompass focus rects and sel rects
        InflateRect( &rcText, 1, 1 );

        if ( WithinRect( &rcText, &lpCnt->lpCI->rcViewSpace ) )
        {
          // check to see if this is closest so far
          // find out how horizontally away the record is
          xDist = abs(lpRec->ptIconOrg.x - lpFocusRec->ptIconOrg.x);

          // find out how far away vertically the record is - go from the bottom of the text rect
          yDist = abs((lpRec->ptIconOrg.y + lpCnt->lpCI->ptBmpOrIcon.y + 2 + (rcText.bottom - rcText.top)) - 
                       lpCnt->lpCI->rcViewSpace.top);
          
          // only consider those within the delta
          if ( xDist < xDelta )
          {
            if ( xDist + yDist < xClosest + yClosest )
            {
              // set the new next rec
              lpNextRec = lpRec;
              xClosest  = xDist;
              yClosest  = yDist;
            }
          }
        }
        break;

      case VK_HOME:
        // find record closest to the bottom that is closest to the right
        // create icon text rect and see if it is in view
        CreateIconTextRect( hDC, lpCnt, lpRec->ptIconOrg, lpRec->lpszIcon, &rcText );

        // inflate the rectangle by 1 to encompass focus rects and sel rects
        InflateRect( &rcText, 1, 1 );

        if ( WithinRect( &rcText, &lpCnt->lpCI->rcViewSpace ) )
        {
          // check to see if this is closest so far
          // find out how horizontally away the record is
          xDist = abs(lpRec->ptIconOrg.x - lpCnt->lpCI->rcViewSpace.left);

          // find out how far away vertically the record is - go from the bottom of the text rect
          yDist = abs((lpRec->ptIconOrg.y + lpCnt->lpCI->ptBmpOrIcon.y + 2 + (rcText.bottom - rcText.top)) - 
                       lpCnt->lpCI->rcViewSpace.top);
          
          // only consider those within the delta
          if ( (yDist < yClosest) ||
               (yDist == yClosest && xDist < xClosest) )
          {
            // set the new next rec
            lpNextRec = lpRec;
            xClosest  = xDist;
            yClosest  = yDist;
          }
        }
        break;
    }
    
    // advance to the next record
    lpRec = lpRec->lpNext;
  }

  if (vkCode==VK_NEXT || vkCode==VK_PRIOR || vkCode==VK_HOME || vkCode==VK_END)
  {
    // release the DC
    SelectObject( hDC, hFontOld );
    ReleaseDC( hWnd, hDC );
  }

  return lpNextRec;
}

VOID DoIconMove( HWND hWnd, LPCONTAINER lpCnt, POINT ptOrig, LPRECORDCORE lpRec )
{
    RECT         rect;
    HDC          hDC;
    MSG          msgModal;
    HCURSOR      hOldCursor;
    BOOL         bTracking=TRUE, bMoving=FALSE, bUseIcon, bChanged;
    POINT        pt, ptPrev;
    HBITMAP      hbmBkg, hbmPrev;
    int          cyTitleArea;
    HFONT        hFontOld;
    int          xOff, yOff;
    int          dx, dy;
#ifdef WIN32
    POINTS  pts;
#endif

    // determine whether we will be moving an icon or a bitmap (and if they exist)
    if( lpCnt->lpCI->flCntAttr & CA_DRAWBMP )
    {
      if (lpCnt->iView & CV_MINI)
      {
        if (!lpRec->hMiniBmp)
          return;
      }
      else
      {
        if (!lpRec->hBmp)
          return;
      }

      // using bitmap, so set to false
      bUseIcon = FALSE;
    }
    else if( lpCnt->lpCI->flCntAttr & CA_DRAWICON )
    {
      if (lpCnt->iView & CV_MINI)
      {
        if (!lpRec->hMiniIcon)
          return;
      }
      else
      {
        if (!lpRec->hIcon)
          return;
      }

      // using icon, so set to true
      bUseIcon = TRUE;
    }
    else
    {
      // check first to see if icon exists, then bitmap
      if (lpCnt->iView & CV_MINI)
      {
        if (lpRec->hMiniIcon)
          bUseIcon = TRUE;
        else if (lpRec->hMiniBmp)
          bUseIcon = FALSE;
        else
          return;
      }
      else
      {
        if (lpRec->hIcon)
          bUseIcon = TRUE;
        else if (lpRec->hBmp)
          bUseIcon = FALSE;
        else
          return;
      }
    }
    
    // Set up a constraining rect for the cursor - cursor cannot leave this rectangle
    pt.x = lpCnt->rectRecArea.left;
    pt.y = lpCnt->rectRecArea.top;
    ClientToScreen( hWnd, &pt );
    rect.left = pt.x - 1;
    rect.top  = pt.y - 1;
    pt.x = lpCnt->rectRecArea.right;
    pt.y = lpCnt->cyClient;
    ClientToScreen( hWnd, &pt );
    rect.right  = pt.x + 1;
    rect.bottom = pt.y + 1;
    ClipCursor( &rect );

    // Turn on the timer so we can generate WM_MOUSEMOVE msgs if
    // the user drags the cursor out of the record area. The generated
    // MOUSEMOVE msgs will be sent here so we can generate scroll msgs.
//    SetTimer( hWnd, IDT_AUTOSCROLL, CTICKS_AUTOSCROLL, (TIMERPROC)CntTimerProc );

    // get DC and set up some variables
    hDC = GetDC( hWnd );
    SetCapture( hWnd );
    hOldCursor = SetCursor( LoadCursor( NULL, IDC_ARROW ) );
    StateSet( lpCnt, CNTSTATE_CLICKED );
    ptPrev.x = ptOrig.x;
    ptPrev.y = ptOrig.y;
    cyTitleArea = lpCnt->lpCI->cyTitleArea;

    // set offsets
    xOff = -lpCnt->lpCI->rcViewSpace.left;
    yOff = -lpCnt->lpCI->rcViewSpace.top + lpCnt->lpCI->cyTitleArea;

    // select the font into the DC so metrics are calculated accurately
    if( lpCnt->lpCI->hFont )
      hFontOld = SelectObject( hDC, lpCnt->lpCI->hFont );
    else
      hFontOld = SelectObject( hDC, GetStockObject( lpCnt->nDefaultFont ) );

    // set up the beginning background bitmap - this must be the background of the icon
    // MINUS the icon itself.  To do this I call the paint routine with my own DC and
    // a state set that tells the paint routine not to paint the icon.  I then save
    // this bitmap in my beginning background bitmap
    {
      PAINTSTRUCT ps;
      HDC         hdcPaint, hdcTemp;
      HBITMAP     hbmPaint, hbmPaintPrev;

      hdcPaint = CreateCompatibleDC(hDC);
      hdcTemp  = CreateCompatibleDC(hDC);

      // Create and select a new bitmap to store our background
      hbmBkg  = CreateCompatibleBitmap(hDC, lpCnt->lpCI->ptBmpOrIcon.x,
                                            lpCnt->lpCI->ptBmpOrIcon.y);
      hbmPrev = SelectObject(hdcTemp, hbmBkg);

      // Create and select a new bitmap to use for the paint
      hbmPaint     = CreateCompatibleBitmap(hDC, lpCnt->cxClient, lpCnt->cyClient);
      hbmPaintPrev = SelectObject(hdcPaint, hbmPaint);

      // set up rectangle that we want painted (the icon rectangle)
      ps.rcPaint.left   = lpRec->ptIconOrg.x + xOff;
      ps.rcPaint.top    = lpRec->ptIconOrg.y + yOff;
      ps.rcPaint.right  = ps.rcPaint.left + lpCnt->lpCI->ptBmpOrIcon.x;
      ps.rcPaint.bottom = ps.rcPaint.top + lpCnt->lpCI->ptBmpOrIcon.y;

      // set state so when paint is called, everything in the rect is drawn but the icon
      StateSet(lpCnt, CNTSTATE_MOVINGFLD);
      PaintIconView( hWnd, lpCnt, hdcPaint, (LPPAINTSTRUCT)&ps );
    
      // Get the beginning background from the dc returned from paint
      pt.x = lpRec->ptIconOrg.x + xOff;
      pt.y = lpRec->ptIconOrg.y + yOff;
      BitBlt(hdcTemp, 0, 0, lpCnt->lpCI->ptBmpOrIcon.x, lpCnt->lpCI->ptBmpOrIcon.y, hdcPaint,
             pt.x, pt.y, SRCCOPY);

      // Tidy up
      SelectObject(hdcTemp, hbmPrev);
      SelectObject(hdcPaint, hbmPaintPrev);

      DeleteObject(hbmPaint);
      DeleteDC(hdcTemp);
      DeleteDC(hdcPaint);
    }

    // While the cursor is being dragged get mouse msgs.
    while( bTracking )
    {
      while( !PeekMessage( &msgModal, NULL, 0, 0, PM_REMOVE ) )
        WaitMessage();

      switch( msgModal.message )
      {
        case WM_MOUSEMOVE:
          // Get new mouse position 
#ifdef WIN32
          pts = MAKEPOINTS(msgModal.lParam); 
          pt.x = pts.x;
          pt.y = pts.y;
#else
          pt.x = LOWORD(msgModal.lParam);
          pt.y = HIWORD(msgModal.lParam);
#endif         
          
          if (!bMoving)
          {
            // dont start moving until we have moved three pixels in any way
            if(pt.x < ptOrig.x - 3 || 
               pt.y < ptOrig.y - 3 ||
             pt.x > ptOrig.x + 3 ||
               pt.y > ptOrig.y + 3)
            {
              // invalidate the text - this will cause it to disappear because the state
              // CNTSTATE_MOVINGFLD is Set
              CreateIconTextRect( hDC, lpCnt, lpRec->ptIconOrg, lpRec->lpszIcon, &rect );
              OffsetRect( &rect, xOff, yOff );
              InflateRect( &rect, 1, 1 );
              InvalidateRect( hWnd, &rect, TRUE );
              UpdateWindow( hWnd );

              // flag that the moving has begun
              bMoving = TRUE;
            }
            else
              continue;
          }

          // see if in the record area - if not, do a scroll - if so, 
          // move the icon (or bitmap as the case may be)
          if( PtInRect( &lpCnt->rectRecArea, pt ) )
          {
            // Calculate delta x and delta y
            dx = ptPrev.x - pt.x;
            dy = ptPrev.y - pt.y;

            // Save previous mouse position
            ptPrev.x = pt.x;
            ptPrev.y = pt.y;

            // Update icon's position
            lpRec->ptIconOrg.x -= dx;
            lpRec->ptIconOrg.y -= dy;

            if ( bUseIcon )
              DrawMovingIcon( hWnd, hDC, lpCnt, lpRec, &hbmBkg, dx, dy );
            else
              DrawMovingBitmap( hWnd, hDC, lpCnt, lpRec, &hbmBkg, dx, dy );
          }
#if 0
          else
          {
            int nVscrollInc;
            
            // Calculate delta x and delta y
            dx = ptPrev.x - pt.x;
            dy = ptPrev.y - pt.y;

            // Save previous mouse position
            ptPrev.x = pt.x;
            ptPrev.y = pt.y;

            // Update icon's position
            lpRec->ptIconOrg.x -= dx;
            lpRec->ptIconOrg.y -= dy;

            // out of record area, so do an autoscroll
            if( pt.y < lpCnt->rectRecArea.top )
            {
              if( lpCnt->dwStyle & CTS_VERTSCROLL )
              {
                // figure out how far the scroll will go 
                if ( lpCnt->lpCI->bArranged )
                  nVscrollInc  = -(lpCnt->lpCI->aiInfo.nHeight + lpCnt->lpCI->aiInfo.nVerSpacing);
                else
                  nVscrollInc  = -lpCnt->lpCI->ptBmpOrIcon.y;
                
                if( nVscrollInc = max(-lpCnt->nVscrollPos, min(nVscrollInc, lpCnt->nVscrollMax - lpCnt->nVscrollPos)) )
          
                // Scroll up
                if (nVscrollInc)
                {
                  FORWARD_WM_VSCROLL( hWnd, 0, SB_LINEUP, 0, SendMessage );

                  lpRec->ptIconOrg.y += nVscrollInc;
                }
              }
            }
            else if( pt.y > lpCnt->rectRecArea.bottom )
            {
              if( lpCnt->dwStyle & CTS_VERTSCROLL )
              {
                // Scroll down 
                FORWARD_WM_VSCROLL( hWnd, 0, SB_LINEDOWN, 0, SendMessage );
              }
            }

            if( pt.x < lpCnt->rectRecArea.left )
            {
              if( lpCnt->dwStyle & CTS_HORZSCROLL )
              {
                // Scroll left 
                FORWARD_WM_HSCROLL( hWnd, 0, SB_LINEUP, 0, SendMessage );
              }
            }
            else if( pt.x > lpCnt->rectRecArea.right )
            {
              if( lpCnt->dwStyle & CTS_HORZSCROLL )
              {
                // Scroll right
                FORWARD_WM_HSCROLL( hWnd, 0, SB_LINEDOWN, 0, SendMessage );
              }
            }
          }
#endif
          // Don't dispatch this msg since we just processed it.
          continue;
          break;

        case WM_LBUTTONUP: // End of move
          // Break out of tracking loop.
          bTracking = FALSE;  

          // clear state so the record will be drawn again
          StateClear( lpCnt, CNTSTATE_MOVINGFLD);

          // invalidate the record so it will draw the text
          if (bMoving)
            InvalidateCntRecord( hWnd, lpCnt, lpRec, NULL, 2 );
          
          // if icon over the title, invalidate the icon
          if (lpRec->ptIconOrg.y + yOff < cyTitleArea && bMoving)
          {
            rect.top    = lpRec->ptIconOrg.y + yOff;
            rect.left   = lpRec->ptIconOrg.x + xOff;
            rect.bottom = rect.top  + lpCnt->lpCI->ptBmpOrIcon.y;
            rect.right  = rect.left + lpCnt->lpCI->ptBmpOrIcon.x;
            InvalidateRect( hWnd, &rect, TRUE );
            UpdateWindow( hWnd );
          }

          // recalculate the icon workspace (see if changed)
          bChanged = CalculateIconWorkspace( hWnd );

          // adjust the vertical scroll bar if workspace changed
          if ( lpCnt->dwStyle & CTS_HORZSCROLL && bChanged )
            AdjustVertScroll( lpCnt->hWnd1stChild, lpCnt, FALSE );
         
          // adjust the horizontal scroll bar if workspace changed
          if ( lpCnt->dwStyle & CTS_HORZSCROLL && bChanged )
            IcnAdjustHorzScroll( lpCnt->hWnd1stChild, lpCnt );
         
          // notify user that an icon has moved
          if (bMoving)
            NotifyAssocWnd( hWnd, lpCnt, CN_ICONPOSCHANGE, 0, lpRec, NULL, 0, 0, 0, NULL );

          // Don't dispatch this msg since we just processed it.
          continue;
          break;
      }

      TranslateMessage( &msgModal );
      DispatchMessage( &msgModal );
    }

    // Restore all the system states.
    StateClear( lpCnt, CNTSTATE_CLICKED );
    ReleaseCapture();
//    KillTimer( hWnd, IDT_AUTOSCROLL );
    ClipCursor( NULL );
    SetCursor( hOldCursor );
    DeleteObject(hbmBkg);
    SelectObject( hDC, hFontOld );
    ReleaseDC( hWnd, hDC );

    return;
}

VOID DrawMovingBitmap( HWND hWnd, HDC hDC, LPCONTAINER lpCnt, LPRECORDCORE lpRec, HBITMAP *hbmBkg,
                       int dx, int dy )
{
    COLORREF rgbBk, cTransparentColor;
    HDC      hdcMem, hdcNewBkg, hdcOldBkg, hdcMask, hdcCache;
    HBITMAP  hbmNew, hbmNPrev, hbmOPrev, hbmPrev, hbmTemp, hbmMask,
             hbmPrevMask, hbmCache, hbmPrevCache;
    int      xOff, yOff;

    // set the offsets
    xOff = -lpCnt->lpCI->rcViewSpace.left;
    yOff = -lpCnt->lpCI->rcViewSpace.top + lpCnt->lpCI->cyTitleArea;
    
    hdcMem    = CreateCompatibleDC(hDC);
    hdcMask   = CreateCompatibleDC(hDC);
    hdcCache  = CreateCompatibleDC(hDC);
    hdcNewBkg = CreateCompatibleDC(hDC);
    hdcOldBkg = CreateCompatibleDC(hDC);

    // Create a temp bitmap for our new background
    hbmNew   = CreateCompatibleBitmap(hDC, lpCnt->lpCI->ptBmpOrIcon.x,
                                      lpCnt->lpCI->ptBmpOrIcon.y);
    hbmMask  = CreateCompatibleBitmap(hdcMask, lpCnt->lpCI->ptBmpOrIcon.x,
                                      lpCnt->lpCI->ptBmpOrIcon.y);
    hbmCache = CreateCompatibleBitmap(hDC, lpCnt->lpCI->ptBmpOrIcon.x,
                                      lpCnt->lpCI->ptBmpOrIcon.y);

    // Select the bitmap
    if (lpCnt->iView & CV_MINI)
    {
      // select mini bitmap
      hbmPrev = SelectObject(hdcMem, lpRec->hMiniBmp);
      if( lpRec->lpTransp && lpRec->lpTransp->fMiniBmpTransp )
      {
        // using transparent bitmap
        cTransparentColor = GetPixel( hdcMem, lpRec->lpTransp->ptMiniBmp.x,
                                              lpRec->lpTransp->ptMiniBmp.y );
        rgbBk = SetBkColor( hdcMem, cTransparentColor );
      }
    }
    else
    {
      // select normal sized bitmap
      hbmPrev = SelectObject(hdcMem, lpRec->hBmp);
      if( lpRec->lpTransp && lpRec->lpTransp->fBmpTransp )
      {
        // using transparent bitmap
        cTransparentColor = GetPixel( hdcMem, lpRec->lpTransp->ptBmp.x,
                                              lpRec->lpTransp->ptBmp.y );
        rgbBk = SetBkColor( hdcMem, cTransparentColor );
      }
    }

    hbmNPrev     = SelectObject(hdcNewBkg, hbmNew);
    hbmOPrev     = SelectObject(hdcOldBkg, *hbmBkg);
    hbmPrevMask  = SelectObject(hdcMask,   hbmMask);
    hbmPrevCache = SelectObject(hdcCache,  hbmCache);

    // Create the bitmap mask
    BitBlt(hdcMask, 0, 0, lpCnt->lpCI->ptBmpOrIcon.x, lpCnt->lpCI->ptBmpOrIcon.y,
           hdcMem, 0, 0, SRCCOPY);

    // Copy screen to new background
    BitBlt(hdcNewBkg, 0, 0, lpCnt->lpCI->ptBmpOrIcon.x, lpCnt->lpCI->ptBmpOrIcon.y,
           hDC, lpRec->ptIconOrg.x + xOff, lpRec->ptIconOrg.y + yOff, SRCCOPY);

    // Replace part of new bkg with old background
    BitBlt(hdcNewBkg, dx, dy, lpCnt->lpCI->ptBmpOrIcon.x, lpCnt->lpCI->ptBmpOrIcon.y,
           hdcOldBkg, 0, 0, SRCCOPY);

    // Copy sprite to old background
    BitBlt(hdcOldBkg, -dx, -dy, lpCnt->lpCI->ptBmpOrIcon.x, lpCnt->lpCI->ptBmpOrIcon.y,
           hdcMem, 0, 0, SRCINVERT);
    BitBlt(hdcOldBkg, -dx, -dy, lpCnt->lpCI->ptBmpOrIcon.x, lpCnt->lpCI->ptBmpOrIcon.y,
           hdcMask, 0, 0, SRCAND);
    BitBlt(hdcOldBkg, -dx, -dy, lpCnt->lpCI->ptBmpOrIcon.x, lpCnt->lpCI->ptBmpOrIcon.y,
           hdcMem, 0, 0, SRCINVERT);

    // Copy sprite to screen without flicker
    BitBlt(hdcCache, 0, 0, lpCnt->lpCI->ptBmpOrIcon.x, lpCnt->lpCI->ptBmpOrIcon.y,
           hdcNewBkg, 0, 0, SRCCOPY);
    BitBlt(hdcCache, 0, 0, lpCnt->lpCI->ptBmpOrIcon.x, lpCnt->lpCI->ptBmpOrIcon.y,
           hdcMem, 0, 0, SRCINVERT);
    BitBlt(hdcCache, 0, 0, lpCnt->lpCI->ptBmpOrIcon.x, lpCnt->lpCI->ptBmpOrIcon.y,
           hdcMask, 0, 0, SRCAND);
    BitBlt(hdcCache, 0, 0, lpCnt->lpCI->ptBmpOrIcon.x, lpCnt->lpCI->ptBmpOrIcon.y,
           hdcMem, 0, 0, SRCINVERT);
    BitBlt(hDC, lpRec->ptIconOrg.x + xOff, lpRec->ptIconOrg.y + yOff, 
           lpCnt->lpCI->ptBmpOrIcon.x, lpCnt->lpCI->ptBmpOrIcon.y,
           hdcCache, 0, 0, SRCCOPY);

    // Copy old background to screen
    BitBlt(hDC, lpRec->ptIconOrg.x + xOff + dx, lpRec->ptIconOrg.y + yOff + dy, 
           lpCnt->lpCI->ptBmpOrIcon.x, lpCnt->lpCI->ptBmpOrIcon.y, 
           hdcOldBkg, 0, 0, SRCCOPY);

    // Tidy up
    SelectObject(hdcMem,    hbmPrev);
    SelectObject(hdcNewBkg, hbmNPrev);
    SelectObject(hdcOldBkg, hbmOPrev);
    SelectObject(hdcMask,   hbmPrevMask);
    SelectObject(hdcCache,  hbmPrevCache);
    SetBkColor(hdcMem, rgbBk);

    // Swap old with new background
    hbmTemp = *hbmBkg;
    *hbmBkg  = hbmNew;
    hbmNew  = hbmTemp;

    // Free resources
    DeleteObject(hbmNew);
    DeleteObject(hbmMask);
    DeleteObject(hbmCache);

    // Tidy up some more
    DeleteDC(hdcMem);
    DeleteDC(hdcMask);
    DeleteDC(hdcCache);
    DeleteDC(hdcNewBkg);
    DeleteDC(hdcOldBkg);

    return;
}

VOID DrawMovingIcon( HWND hWnd, HDC hDC, LPCONTAINER lpCnt, LPRECORDCORE lpRec, HBITMAP *hbmBkg,
                     int dx, int dy )
{
    HDC      hdcNewBkg, hdcOldBkg, hdcCache;
    HBITMAP  hbmNew, hbmNPrev, hbmOPrev, hbmTemp, hbmCache, hbmPrevCache;
    int      xOff, yOff;

    // set the offsets
    xOff = -lpCnt->lpCI->rcViewSpace.left;
    yOff = -lpCnt->lpCI->rcViewSpace.top + lpCnt->lpCI->cyTitleArea;
    
    hdcCache  = CreateCompatibleDC(hDC);
    hdcNewBkg = CreateCompatibleDC(hDC);
    hdcOldBkg = CreateCompatibleDC(hDC);

    // Create a temp bitmap for our new background
    hbmNew   = CreateCompatibleBitmap(hDC, lpCnt->lpCI->ptBmpOrIcon.x,
                                           lpCnt->lpCI->ptBmpOrIcon.y);
    hbmCache = CreateCompatibleBitmap(hDC, lpCnt->lpCI->ptBmpOrIcon.x,
                                           lpCnt->lpCI->ptBmpOrIcon.y);

    hbmNPrev     = SelectObject(hdcNewBkg, hbmNew);
    hbmOPrev     = SelectObject(hdcOldBkg, *hbmBkg);
    hbmPrevCache = SelectObject(hdcCache,  hbmCache);

    // Copy screen to new background
    BitBlt(hdcNewBkg, 0, 0, lpCnt->lpCI->ptBmpOrIcon.x, lpCnt->lpCI->ptBmpOrIcon.y,
           hDC, lpRec->ptIconOrg.x + xOff, lpRec->ptIconOrg.y + yOff, SRCCOPY);

    // Replace part of new bkg with old background
    BitBlt(hdcNewBkg, dx, dy, lpCnt->lpCI->ptBmpOrIcon.x, lpCnt->lpCI->ptBmpOrIcon.y,
           hdcOldBkg, 0, 0, SRCCOPY);

    DrawIconForMove( hdcOldBkg, lpCnt, lpRec, -dx, -dy );

    // Copy sprite to screen without flicker
    BitBlt(hdcCache, 0, 0, lpCnt->lpCI->ptBmpOrIcon.x, lpCnt->lpCI->ptBmpOrIcon.y,
           hdcNewBkg, 0, 0, SRCCOPY);
    DrawIconForMove( hdcCache, lpCnt, lpRec, 0, 0 );
    BitBlt(hDC, lpRec->ptIconOrg.x + xOff, lpRec->ptIconOrg.y + yOff, 
           lpCnt->lpCI->ptBmpOrIcon.x, lpCnt->lpCI->ptBmpOrIcon.y,
           hdcCache, 0, 0, SRCCOPY);

    // Copy old background to screen
    BitBlt(hDC, lpRec->ptIconOrg.x + xOff + dx, lpRec->ptIconOrg.y + yOff + dy, 
           lpCnt->lpCI->ptBmpOrIcon.x, lpCnt->lpCI->ptBmpOrIcon.y, 
           hdcOldBkg, 0, 0, SRCCOPY);

    // Tidy up
    SelectObject(hdcNewBkg, hbmNPrev);
    SelectObject(hdcOldBkg, hbmOPrev);
    SelectObject(hdcCache,  hbmPrevCache);

    // Swap old with new background
    hbmTemp = *hbmBkg;
    *hbmBkg  = hbmNew;
    hbmNew  = hbmTemp;

    // Free resources
    DeleteObject(hbmCache);
    DeleteObject(hbmNew);

    // Tidy up some more
    DeleteDC(hdcCache);
    DeleteDC(hdcNewBkg);
    DeleteDC(hdcOldBkg);

    return;
}

VOID DrawIconForMove( HDC hDC, LPCONTAINER lpCnt, LPRECORDCORE lpRec, int dx, int dy )
{
    // Select the icon
    if (lpCnt->iView & CV_MINI)
    {
      // select mini icon
#ifdef WIN32
      DrawIconEx( hDC, dx, dy, lpRec->hMiniIcon, lpCnt->lpCI->ptBmpOrIcon.x,
                  lpCnt->lpCI->ptBmpOrIcon.y, 0, NULL, DI_NORMAL );
#else 
      DrawIcon( hDC, dx, dy, lpRec->hMiniIcon );
#endif
    }
    else
    {
      // select normal sized icon
#ifdef WIN32
      DrawIconEx( hDC, dx, dy, lpRec->hIcon, lpCnt->lpCI->ptBmpOrIcon.x,
                  lpCnt->lpCI->ptBmpOrIcon.y, 0, NULL, DI_NORMAL );
#else 
      DrawIcon( hDC, dx, dy, lpRec->hIcon );
#endif
    }

    return;
}
