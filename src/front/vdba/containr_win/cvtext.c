/****************************************************************************
 * Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * CVTEXT.C
 *
 * Contains message handler for the TEXT view format.
 ****************************************************************************/

#include <windows.h>
#include <windowsx.h>  // Need this for the memory macros
#include <stdlib.h>
#include <malloc.h>
#include "cntdll.h"
#include "cntdrag.h"

#define ASYNC_KEY_DOWN      0x8000   // flag indicating key is pressed

// Prototypes for Text view msg cracker functions.
BOOL Txt_OnNCCreate( HWND hWnd, LPCREATESTRUCT lpCreateStruct );
VOID Txt_OnCommand( HWND hWnd, int id, HWND hwndCtl, UINT codeNotify );
VOID Txt_OnChildActivate( HWND hWnd );
VOID Txt_OnDestroy( HWND hWnd );
BOOL Txt_OnEraseBkgnd( HWND hWnd, HDC hDC );
VOID Txt_OnPaint( HWND hWnd );
VOID Txt_OnEnable( HWND hWnd, BOOL fEnable );
VOID Txt_OnShowWindow( HWND hWnd, BOOL fShow, UINT status );
VOID Txt_OnCancelMode( HWND hWnd );
UINT Txt_OnGetDlgCode( HWND hWnd, LPMSG lpmsg );

// Prototypes for local functions
BOOL WINAPI Get1stColXPos( LPCONTAINER lpCnt, LPINT lpxStr, LPINT lpxEnd, int bDir );
int GetRecXPos( LPCONTAINER lpCnt, LPRECORDCORE lpRec, LPINT lpxStr, LPINT lpxEnd );
LPFLOWCOLINFO Get1stColInView( HWND hWnd, LPCONTAINER lpCnt );
LPFLOWCOLINFO GetLastColInView( HWND hWnd, LPCONTAINER lpCnt );
BOOL GetColXPos( LPCONTAINER lpCnt, LPFLOWCOLINFO lpCol, LPINT lpxStr, LPINT lpxEnd );
BOOL NEAR TxtTrackSelection( HWND hWnd, LPCONTAINER lpCnt, LPRECORDCORE lpRec, LPFLOWCOLINFO lpCol, 
                             LPPOINT lpPt, LPINT lpSelRecs, LPINT lpSelCols, BOOL bTrackCols );
BOOL NEAR TxtExtSelect( HWND hWnd, LPCONTAINER lpCnt, LPRECORDCORE lpRec, int nSelRecs,
                        LPRECORDCORE FAR *lpRecLast );
BOOL SetFlowColStruct( LPCONTAINER lpCnt, LPRECORDCORE lpTopRec, int nWidth, int nRecs, 
                       LPINT nMaxWidth );
VOID FreeFlowColStruct( LPCONTAINER lpCnt );

/****************************************************************************
 * TextViewWndProc
 *
 * Purpose:
 *  Handles all window messages for a container in TEXT view format.
 *  Any message not processed here should go to DefWindowProc.
 *
 * Parameters:
 *  Standard for WndProc
 *
 * Return Value:
 *  LRESULT       Varies with the message.
 ****************************************************************************/

LRESULT WINAPI TextViewWndProc( HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam )
    {
    switch( iMsg )
      {
      HANDLE_MSG( hWnd, WM_NCCREATE,       Txt_OnNCCreate      );
      HANDLE_MSG( hWnd, WM_CREATE,         Txt_OnCreate        );
      HANDLE_MSG( hWnd, WM_COMMAND,        Txt_OnCommand       );
      HANDLE_MSG( hWnd, WM_SIZE,           Txt_OnSize          );
      HANDLE_MSG( hWnd, WM_CHILDACTIVATE,  Txt_OnChildActivate );
      HANDLE_MSG( hWnd, WM_DESTROY,        Txt_OnDestroy       );
      HANDLE_MSG( hWnd, WM_ERASEBKGND,     Txt_OnEraseBkgnd    );
      HANDLE_MSG( hWnd, WM_SETFONT,        Txt_OnSetFont       );
      HANDLE_MSG( hWnd, WM_PAINT,          Txt_OnPaint         );
      HANDLE_MSG( hWnd, WM_ENABLE,         Txt_OnEnable        );
      HANDLE_MSG( hWnd, WM_SETFOCUS,       Txt_OnSetFocus      );
      HANDLE_MSG( hWnd, WM_KILLFOCUS,      Txt_OnKillFocus     );
      HANDLE_MSG( hWnd, WM_SHOWWINDOW,     Txt_OnShowWindow    );
      HANDLE_MSG( hWnd, WM_CANCELMODE,     Txt_OnCancelMode    );
      HANDLE_MSG( hWnd, WM_VSCROLL,        Txt_OnVScroll       );
      HANDLE_MSG( hWnd, WM_HSCROLL,        Txt_OnHScroll       );
      HANDLE_MSG( hWnd, WM_CHAR,           Txt_OnChar          );
      HANDLE_MSG( hWnd, WM_KEYDOWN,        Txt_OnKey           );
      HANDLE_MSG( hWnd, WM_RBUTTONDBLCLK,  Txt_OnRButtonDown   );
      HANDLE_MSG( hWnd, WM_RBUTTONDOWN,    Txt_OnRButtonDown   );
      HANDLE_MSG( hWnd, WM_GETDLGCODE,     Txt_OnGetDlgCode    );
      HANDLE_MSG( hWnd, WM_LBUTTONDBLCLK,  Txt_OnLButtonDown   );
      HANDLE_MSG( hWnd, WM_LBUTTONDOWN,    Txt_OnLButtonDown   );
      HANDLE_MSG( hWnd, WM_LBUTTONUP,      Txt_OnLButtonUp     );
      HANDLE_MSG( hWnd, WM_MOUSEMOVE,      Txt_OnMouseMove     );
      HANDLE_MSG( hWnd, WM_NCLBUTTONDOWN,  Txt_OnNCLButtonDown );
      HANDLE_MSG( hWnd, WM_NCMBUTTONDOWN,  Txt_OnNCMButtonDown );
      HANDLE_MSG( hWnd, WM_NCRBUTTONDOWN,  Txt_OnNCRButtonDown );

      default:
        return (Process_DefaultMsg(hWnd,iMsg,wParam,lParam));
      }

    return 0L;
    }

/****************************************************************************
 * TextViewWndProc Message Cracker Callback Functions
 *
 * Purpose:
 *  Functions to process each message received by the Container WndProc.
 *  This is a Win16/Win32 portable method for message handling.
 *
 * Return Value:
 *  Varies according to the message being processed.
 ****************************************************************************/

BOOL Txt_OnNCCreate( HWND hWnd, LPCREATESTRUCT lpCreateStruct )
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

BOOL Txt_OnCreate( HWND hWnd, LPCREATESTRUCT lpCreateStruct )
{
    LPCONTAINER lpCnt = GETCNT(hWnd);

    // Init flag indicating whether or not to send focus chging notifications.
    lpCnt->lpCI->bSendFocusMsg = TRUE;

    // Init flag indicating whether or not to send keyboard events.
    lpCnt->lpCI->bSendKBEvents = TRUE;

    // Init flag indicating whether or not to send mouse events.
    lpCnt->lpCI->bSendMouseEvents = TRUE;

    // Init flag indicating whether or not to draw the focust rect.
    lpCnt->lpCI->bDrawFocusRect = TRUE;

    lpCnt->lpCI->lpFocusRec = NULL;
    lpCnt->lpCI->lpFocusFld = NULL;

    // Initialize the character metrics.
    InitTextMetrics( hWnd, lpCnt );

    // NOTE: When using a msg cracker for WM_CREATE we must
    //       return TRUE if all is well instead of 0.
    return TRUE;
}

VOID Txt_OnCommand( HWND hWnd, int id, HWND hwndCtl, UINT codeNotify )
{
    // same as detailed view
    Dtl_OnCommand( hWnd, id, hwndCtl, codeNotify );
    
    return;
}

VOID Txt_OnSize( HWND hWnd, UINT state, int cx, int cy )
{
    LPCONTAINER lpCnt = GETCNT(hWnd);
//    LPCONTAINER lpCntFrame;
    LPRECORDCORE /*lpRec,*/ lpOldTopRec;
    RECT rect;
    BOOL bHasVscroll, bHasHscroll, bRepaint, bRowChange = FALSE;
    int  cxOldClient, nOldHscrollPos;
    int  cxClient, cyClient;
    int  cyRecordArea, nMaxWidth, OldnRecs;
//    int  nVscrollInc;

    if( !lpCnt )
      return;

    // Remember the old client width. If this changes we need to repaint
    // the title area under certain conditions.
    cxOldClient = lpCnt->cxClient;

    // Remember the old top record. If this changes we need to repaint
    // the entire record area. This can happen if the vertical position
    // is at or near the max and the user sizes the container higher.
    lpOldTopRec = lpCnt->lpCI->lpTopRec;

    // Remember the old horz scroll position. If this changes we need to
    // repaint the entire field area. This can happen if the horz position
    // is at or near the max and the user sizes the container wider.
    nOldHscrollPos = lpCnt->nHscrollPos;

    // remember the new container client area
    cxClient  = lpCnt->cxClient = cx;
    cyClient  = lpCnt->cyClient = cy;

    // set the nbr of records which are in view
    cyRecordArea = cyClient - lpCnt->lpCI->cyTitleArea;
    OldnRecs = lpCnt->lpCI->nRecsDisplayed;
    lpCnt->lpCI->nRecsDisplayed = (cyRecordArea-1) / lpCnt->lpCI->cyRow + 1;
    if (lpCnt->iView & CV_FLOW && lpCnt->lpCI->nRecsDisplayed > 1) // if flowed, don't want partial records at the bottome
       lpCnt->lpCI->nRecsDisplayed -= 1;

    // if flowed, and number of rows changed, must recalc column structure
    if (lpCnt->lpCI->nRecsDisplayed != OldnRecs)// && lpCnt->iView & CV_FLOW)
    {
      CreateFlowColStruct(hWnd, lpCnt);
      bRowChange = TRUE;
    }

    nMaxWidth   = lpCnt->lpCI->nMaxWidth;
    bHasVscroll =  lpCnt->dwStyle & CTS_VERTSCROLL ? TRUE : FALSE;
    bHasHscroll =  lpCnt->dwStyle & CTS_HORZSCROLL ? TRUE : FALSE;

    if( bHasHscroll )
    {
      if( nMaxWidth - cxClient > 0 )
        lpCnt->nHscrollMax = max(0, (nMaxWidth - cxClient - 1) / lpCnt->lpCI->cxChar + 1);
      else
        lpCnt->nHscrollMax = 0;

      lpCnt->nHscrollPos = min(lpCnt->nHscrollPos, lpCnt->nHscrollMax);

      SetScrollPos  ( hWnd, SB_HORZ,    lpCnt->nHscrollPos, FALSE  );
      SetScrollRange( hWnd, SB_HORZ, 0, lpCnt->nHscrollMax, TRUE );
    }

    // There is nothing to the right of this value.
    lpCnt->xLastCol = lpCnt->lpCI->nMaxWidth - lpCnt->nHscrollPos * lpCnt->lpCI->cxChar;

    if( bHasVscroll ) // Set up the vertical scrollbar.
    {
      AdjustVertScroll( hWnd, lpCnt, TRUE );
    }

#if 0
    // If using Delta scrolling model we must check to see if we exposed
    // new records which are beyond the current delta range.
    lpRec = CntRecHeadGet( hWnd );
    if( lpCnt->lDelta && lpRec && hWnd == lpCnt->hWnd1stChild )
    {
      // Issue the QueryDelta if we exposed records beyond current delta
      // or the position backed up behind the delta pos. 
      if( (lpCnt->nVscrollPos && lpCnt->nVscrollPos < lpCnt->lDeltaPos) ||
          lpCnt->nVscrollPos + lpCnt->lpCI->nRecsDisplayed > lpCnt->lDeltaPos + lpCnt->lDelta )
      {
        // If the position backed up behind the delta position send a negative
        // increment with the query delta msg. This can happen if the vertical
        // position is at or near max and the user sizes the container larger.
        if( lpCnt->nVscrollPos < lpCnt->lDeltaPos )
          nVscrollInc = lpCnt->nVscrollPos - (int)lpCnt->lDeltaPos;
        else
          nVscrollInc = 0;

        lpCntFrame->bDoingQueryDelta = TRUE;
        NotifyAssocWnd( hWnd, lpCnt, CN_QUERYDELTA, 0, NULL, NULL, nVscrollInc, 0, 0, NULL );
        lpCntFrame->bDoingQueryDelta = FALSE;
      }
    }
#endif

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
    // We must repaint the entire record area if the top record changed.
    if( lpOldTopRec && lpOldTopRec != lpCnt->lpCI->lpTopRec || bRowChange)
    {
      // Calc the rect for the record area.
      rect.left   = 0;
      rect.top    = lpCnt->lpCI->cyTitleArea;
      rect.right  = cx;
      rect.bottom = cy;
      InvalidateRect( hWnd, &rect, FALSE );
    }

    // New repaint code necessary because of removing redraw-on-size styles.
    // We must repaint the entire record area and the field title area
    // if the horizontal scroll position changed.
    if( nOldHscrollPos != lpCnt->nHscrollPos )
    {
      // Calc the rect for the fld title area and the record area.
      rect.left   = 0;
      rect.top    = lpCnt->lpCI->cyTitleArea;
      rect.right  = cx;
      rect.bottom = cy;
      InvalidateRect( hWnd, &rect, FALSE );
    }

    // New repaint code necessary because of removing redraw-on-size styles.
    // We must repaint the focus record if we are using a wide focus
    // rectangle and the client width is changing.
    if( lpCnt->bWideFocusRect && cxOldClient != lpCnt->cxClient )
    {
      if( lpCnt->lpCI->lpFocusRec )
        InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 1 );
    }

    return;
}  

VOID Txt_OnSetFont( HWND hWnd, HFONT hfont, BOOL fRedraw )
{
    LPCONTAINER lpCnt = GETCNT(hWnd);

    // set the font into the container info struct
    if( hfont )
    {
      lpCnt->lpCI->hFont = hfont;

      // Initialize the character metrics for this font.
      InitTextMetrics( hWnd, lpCnt );

      // Force repaint since we changed a state.
      UpdateContainer( hWnd, NULL, TRUE );
    }

    return;
}  

VOID Txt_OnChildActivate( HWND hWnd )
{
    // same as Detialed View
    Dtl_OnChildActivate( hWnd );

    return;
}  

VOID Txt_OnDestroy( HWND hWnd )
{
    // same as Detialed View
    Dtl_OnDestroy( hWnd );

    return;
}  

BOOL Txt_OnEraseBkgnd( HWND hWnd, HDC hDC )
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

VOID Txt_OnPaint( HWND hWnd )
{
    LPCONTAINER  lpCnt = GETCNT(hWnd);
    PAINTSTRUCT ps;
    HDC         hDC;

    hDC = BeginPaint( hWnd, &ps );
    
    PaintTextView( hWnd, lpCnt, hDC, (LPPAINTSTRUCT)&ps );
    
    EndPaint( hWnd, &ps );

    return;
}  

VOID Txt_OnEnable( HWND hWnd, BOOL fEnable )
{
    // same as Detailed View
    Dtl_OnEnable( hWnd, fEnable );
    return;
}  

VOID Txt_OnSetFocus( HWND hWnd, HWND hwndOldFocus )
{
    LPCONTAINER lpCnt = GETCNT(hWnd);

    if( !lpCnt )
      return;

    StateSet( lpCnt, CNTSTATE_HASFOCUS );

    // Display the focus rect.
    if( lpCnt->lpCI && lpCnt->lpCI->lpFocusRec )
      InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );

    // Bring the container to the top if its getting the focus.
    BringWindowToTop( hWnd );

    // Tell the assoc wnd about change in focus state.
    if( lpCnt->lpCI && lpCnt->lpCI->bSendFocusMsg )
      NotifyAssocWnd( hWnd, lpCnt, CN_SETFOCUS, 0, NULL, NULL, 0, 0, 0, (LPVOID)hwndOldFocus );
    return;
}

VOID Txt_OnKillFocus( HWND hWnd, HWND hwndNewFocus )
{
    LPCONTAINER lpCnt = GETCNT(hWnd);

    if( !lpCnt )
      return;

    StateClear( lpCnt, CNTSTATE_HASFOCUS );

    // Clear the focus rect.
    if( lpCnt->lpCI && lpCnt->lpCI->lpFocusRec )
      InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );

    // Tell the assoc wnd about change in focus state.
    if( lpCnt->lpCI && lpCnt->lpCI->bSendFocusMsg )
      NotifyAssocWnd( hWnd, lpCnt, CN_KILLFOCUS, 0, NULL, NULL, 0, 0, 0, (LPVOID)hwndNewFocus );

    return;
}  

VOID Txt_OnShowWindow( HWND hWnd, BOOL fShow, UINT status )
{
    // same as Detailed View
    Dtl_OnShowWindow( hWnd, fShow, status );
    return;
}  

VOID Txt_OnCancelMode( HWND hWnd )
{
    // same as Detailed view
    Dtl_OnCancelMode( hWnd );
    return;
}  

VOID Txt_OnVScroll( HWND hWnd, HWND hwndCtl, UINT code, int pos )
{
    LPCONTAINER lpCnt = GETCNT(hWnd);
    LPCONTAINER lpCntFrame;
    RECT       rect;
    UINT       wScrollEvent;
    BOOL       bSentQueryDelta=FALSE;
    LONG       lNewPos;
    int        nVscrollInc;
    int        cyRecordArea;

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
        nVscrollInc  = -1;
        if( lpCnt->lpCI->bFocusRecLocked )
          wScrollEvent = CN_LK_VS_LINEUP;
        else
          wScrollEvent = CN_VSCROLL_LINEUP;
        break;

      case SB_LINEDOWN:
        nVscrollInc  = 1;
        if( lpCnt->lpCI->bFocusRecLocked )
          wScrollEvent = CN_LK_VS_LINEDOWN;
        else
          wScrollEvent = CN_VSCROLL_LINEDOWN;
        break;

      case SB_PAGEUP:
        // Calc the area being used for record data.
        cyRecordArea = lpCnt->cyClient - lpCnt->lpCI->cyTitleArea;
        nVscrollInc  = min(-1, -cyRecordArea / lpCnt->lpCI->cyRow);
        if( lpCnt->lpCI->bFocusRecLocked )
          wScrollEvent = CN_LK_VS_PAGEUP;
        else
          wScrollEvent = CN_VSCROLL_PAGEUP;
        break;

      case SB_PAGEDOWN:
        // Calc the area being used for record data.
        cyRecordArea = lpCnt->cyClient - lpCnt->lpCI->cyTitleArea;
        nVscrollInc  = max(1, cyRecordArea / lpCnt->lpCI->cyRow);
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
        if( lpCnt->lpCI->flCntAttr & CA_WANTVTHUMBTRK ||
            lpCnt->lpCI->flCntAttr & CA_DYNAMICVSCROLL )
        {
          // If we have a 32 bit range max, calc the thumb position.
          if( StateTest( lpCnt, CNTSTATE_VSCROLL32 ) )
          {
            pos = CalcVThumbpos( hWnd, lpCnt );
          }

          wScrollEvent = CN_VSCROLL_THUMBTRK;

          // If both WantVThumbtrk and DynmanicVScroll are enabled
          // the dynamic scroll will take precedence.
          if( lpCnt->lpCI->flCntAttr & CA_DYNAMICVSCROLL )
          {
            // Forget it if using ownervscroll or delta model. Also
            // forget it if the focus record is locked. It means they are
            // doing validation which cannot be done with dynamic scrolling.
            if( (lpCnt->lpCI->flCntAttr & CA_OWNERVSCROLL) || 
                 lpCnt->lDelta || lpCnt->lpCI->bFocusRecLocked )
              return;

            nVscrollInc  = pos - lpCnt->nVscrollPos;
            break;
          }
          else
          {
            NotifyAssocWnd( hWnd, lpCnt, wScrollEvent, 0, NULL, NULL, pos, 0, 0, NULL );
          }
        }
        return;

      default:
        nVscrollInc = 0;
      }

    // Scroll the record area vertically (if owner vscroll not enabled).
    if( !(lpCnt->lpCI->flCntAttr & CA_OWNERVSCROLL) && !lpCnt->lpCI->bFocusRecLocked )
    {
      if( nVscrollInc = max(-lpCnt->nVscrollPos, min(nVscrollInc, lpCnt->nVscrollMax - lpCnt->nVscrollPos)) )
      {
        // If using Delta scrolling model check our position.
        if( lpCnt->lDelta )
        {
          // This is the target position.
          lNewPos = lpCnt->nVscrollPos + nVscrollInc;

          // Issue the QueryDelta if the target is outside the delta range.
          if( (lNewPos < lpCnt->lDeltaPos) ||
             ((lNewPos + lpCnt->lpCI->nRecsDisplayed) > lpCnt->lDeltaPos + lpCnt->lDelta) )
          {
            bSentQueryDelta = TRUE;
            lpCntFrame = GETCNT( lpCnt->hWndFrame );
            lpCntFrame->bDoingQueryDelta = TRUE;
            NotifyAssocWnd( hWnd, lpCnt, CN_QUERYDELTA, 0, NULL, NULL, nVscrollInc, 0, 0, NULL );
            lpCntFrame->bDoingQueryDelta = FALSE;
          }
        }

        // Set the new vertical position.
        lpCnt->nVscrollPos += nVscrollInc;

        // Set the scrolling rect and execute the scroll.
        rect.left   = 0;
        rect.top    = lpCnt->lpCI->cyTitleArea;
        rect.right  = min( lpCnt->cxClient, lpCnt->xLastCol);
        rect.bottom = lpCnt->cyClient;
        ScrollWindow( hWnd, 0, -lpCnt->lpCI->cyRow * nVscrollInc, NULL, &rect );

        // Now reposition the scroll thumb.
        SetScrollPos( hWnd, SB_VERT, lpCnt->nVscrollPos, TRUE );
  
        // Scroll the record list also so we always have a pointer to the 
        // record currently displayed at the top of the container wnd. 
        // If a CN_QUERYDELTA was sent we want to scroll the records from
        // the head taking into account the delta position.
//        if( !bSentQueryDelta )
          lpCnt->lpCI->lpTopRec = CntScrollRecList( hWnd, lpCnt->lpCI->lpTopRec, nVscrollInc );
//        else
//          lpCnt->lpCI->lpTopRec = CntScrollRecList( hWnd, lpCnt->lpCI->lpRecHead, lpCnt->nVscrollPos-lpCnt->lDeltaPos );
  
        UpdateWindow( hWnd );
      }
    }

    // Don't notify the assoc if dynamic scrolling is on
    // and this was a thumbtrack scroll event.
    // movement or always if owner vscroll is enabled.
    if( wScrollEvent != CN_VSCROLL_THUMBTRK )
    {
      // Notify the assoc of the scrolling event if there was
      // movement or always if owner vscroll is enabled.
      if( nVscrollInc || lpCnt->lpCI->flCntAttr & CA_OWNERVSCROLL )
        NotifyAssocWnd( hWnd, lpCnt, wScrollEvent, 0, NULL, NULL, nVscrollInc, 0, 0, NULL );
    }
    return;
}  

VOID Txt_OnHScroll( HWND hWnd, HWND hwndCtl, UINT code, int pos )
{
    LPCONTAINER lpCnt = GETCNT(hWnd);
    LPFLOWCOLINFO lpCol;
    RECT        rect;
    UINT        wScrollEvent;
    int         nHscrollInc;
    int         xStrCol, xEndCol;

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
#if 0
      case SB_LINEUP:
        if( !(lpCnt->lpCI->flCntAttr & CA_HSCROLLBYCHAR) && (lpCnt->iView & CV_FLOW))
        {
          // Get the starting xcoord of the 1st field which starts
          // to the left of the viewing area .
          Get1stColXPos( lpCnt, &xStrCol, &xEndCol, 0 );

          nHscrollInc  = xStrCol / lpCnt->lpCI->cxChar;
        }
        else
        {
          // Scroll left 1 character unit if scrolling by character.
          nHscrollInc = -1;
        }

        if( lpCnt->lpCI->bFocusFldLocked )
          wScrollEvent = CN_LK_HS_LINEUP;
        else
          wScrollEvent = CN_HSCROLL_LINEUP;
        break;

      case SB_LINEDOWN:
        if( !(lpCnt->lpCI->flCntAttr & CA_HSCROLLBYCHAR) && (lpCnt->iView & CV_FLOW))
        {
          // Get the ending xcoord of the 1st field which appears in the viewing area
          Get1stColXPos( lpCnt, &xStrCol, &xEndCol, 1 );

          nHscrollInc  = xEndCol / lpCnt->lpCI->cxChar;
        }
        else
        {
          // Scroll right 1 character unit if scrolling by character.
          nHscrollInc = 1;
        }

        if( lpCnt->lpCI->bFocusFldLocked )
          wScrollEvent = CN_LK_HS_LINEDOWN;
        else
          wScrollEvent = CN_HSCROLL_LINEDOWN;
        break;
#endif
      case SB_LINEUP:
        if( !(lpCnt->lpCI->flCntAttr & CA_HSCROLLBYCHAR) )
        {
          // Get the starting xcoord of the 1st column which starts
          // to the left of the viewing area .
          lpCol = Get1stColInView( hWnd, lpCnt );
          GetColXPos( lpCnt, lpCol, &xStrCol, &xEndCol );
          if( xStrCol >= 0 )
          {
            if (lpCol->lpPrev)
              lpCol = lpCol->lpPrev;

            GetColXPos( lpCnt, lpCol, &xStrCol, &xEndCol );
          }
          nHscrollInc  = xStrCol / lpCnt->lpCI->cxChar;
        }
        else
        {
          // Scroll left 1 character unit if scrolling by character.
          nHscrollInc = -1;
        }

        if( lpCnt->lpCI->bFocusFldLocked )
          wScrollEvent = CN_LK_HS_LINEUP;
        else
          wScrollEvent = CN_HSCROLL_LINEUP;
        break;

      case SB_LINEDOWN:
        if( !(lpCnt->lpCI->flCntAttr & CA_HSCROLLBYCHAR) )
        {
          // Get the starting xcoord of the 1st field which starts
          // to the left of the viewing area .
          lpCol = Get1stColInView( hWnd, lpCnt );
          GetColXPos( lpCnt, lpCol, &xStrCol, &xEndCol );

          if( xStrCol >= 0 )
          {
            if (lpCol->lpNext)
              lpCol = lpCol->lpNext;
            GetColXPos( lpCnt, lpCol, &xStrCol, &xEndCol );
            nHscrollInc = xStrCol / lpCnt->lpCI->cxChar;

            // This can happen if the last field is wider than
            // the width of the container. Treat like a partial field.
            if( !nHscrollInc )
              nHscrollInc = xEndCol / lpCnt->lpCI->cxChar;
          }
          else
          {
            // We are scrolling a partial field.
            nHscrollInc = xEndCol / lpCnt->lpCI->cxChar;
          }
        }
        else
        {
          // Scroll right 1 character unit if scrolling by character.
          nHscrollInc = 1;
        }

        if( lpCnt->lpCI->bFocusFldLocked )
          wScrollEvent = CN_LK_HS_LINEDOWN;
        else
          wScrollEvent = CN_HSCROLL_LINEDOWN;
        break;

      case SB_PAGEUP:
        nHscrollInc  = min(-1, -lpCnt->cxClient / lpCnt->lpCI->cxChar);
        if( lpCnt->lpCI->bFocusFldLocked )
          wScrollEvent = CN_LK_HS_PAGEUP;
        else
          wScrollEvent = CN_HSCROLL_PAGEUP;
        break;

      case SB_PAGEDOWN:
        nHscrollInc  = max(1, lpCnt->cxClient / lpCnt->lpCI->cxChar);
        if( lpCnt->lpCI->bFocusFldLocked )
          wScrollEvent = CN_LK_HS_PAGEDOWN;
        else
          wScrollEvent = CN_HSCROLL_PAGEDOWN;
        break;

      case SB_THUMBPOSITION:
        nHscrollInc  = pos - lpCnt->nHscrollPos;
        if( lpCnt->lpCI->bFocusFldLocked )
          wScrollEvent = CN_LK_HS_THUMBPOS;
        else
          wScrollEvent = CN_HSCROLL_THUMBPOS;
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

        // There is nothing to the right of this value.
        lpCnt->xLastCol = lpCnt->lpCI->nMaxWidth - lpCnt->nHscrollPos * lpCnt->lpCI->cxChar;
  
        ScrollWindow( hWnd, -lpCnt->lpCI->cxChar * nHscrollInc, 0, NULL, &rect );
        SetScrollPos( hWnd, SB_HORZ, lpCnt->nHscrollPos, TRUE );

        // If using wide focus rect and not scrolling by chars repaint the
        // focus record so we don't leave focus cell artifacts around.
//        if( !(lpCnt->lpCI->flCntAttr & CA_HSCROLLBYCHAR) )
//        {
//          if( lpCnt->lpCI->flCntAttr & CA_WIDEFOCUSRECT )
//            InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 1 );
//        }

        UpdateWindow( hWnd );
      }
    }

    // Notify the assoc of the scrolling event if there was movement.
    if( nHscrollInc )
      NotifyAssocWnd( hWnd, lpCnt, wScrollEvent, 0, NULL, NULL, nHscrollInc, 0, 0, NULL );

    return;
}  

VOID Txt_OnChar( HWND hWnd, UINT ch, int cRepeat )
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
    if( !TxtIsFocusInView( hWnd, lpCnt ) ) 
      TxtScrollToFocus( hWnd, lpCnt, 0 );

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

VOID Txt_OnKey( HWND hWnd, UINT vk, BOOL fDown, int cRepeat, UINT flags )
{
    LPCONTAINER   lpCnt = GETCNT(hWnd);
    LPCONTAINER   lpCntFrame;
    LPFLOWCOLINFO lpCol;
    LPRECORDCORE  lpOldFocusRec, lpTempRec;
    BOOL          bShiftKey, bCtrlKey, bRecSelected, bFound, bSel;
    int           xStrCol, xEndCol;
    int           nInc, nRecPos, nDir, nRecs, i;
    int           cyRecordArea, nFullRecsDisplayed;

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
      case VK_PRIOR:
      case VK_NEXT:
        // If focus cell is not in view, scroll to it.
        if( !TxtIsFocusInView( hWnd, lpCnt ) )
          TxtScrollToFocus( hWnd, lpCnt, vk == VK_PRIOR ? 0 : 1 );

        // NOTICE: Falling thru here!

      case VK_HOME:
      case VK_END:
        bRecSelected = FALSE;

        if( !lpCnt->lpCI->bFocusRecLocked )
        {
          // If selection is active we will chg the selected record.
          if( lpCnt->dwStyle & CTS_SINGLESEL && lpCnt->lpCI->lpSelRec )
          {
            // Unselect the previously selected record and invalidate
            // it but don't repaint it yet.
            lpCnt->lpCI->lpSelRec->flRecAttr &= ~CRA_SELECTED;
            InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpSelRec, NULL, 1 );
            NotifyAssocWnd( hWnd, lpCnt, CN_RECUNSELECTED, 0, lpCnt->lpCI->lpSelRec, NULL, 0, bShiftKey, bCtrlKey, NULL );
          }
          else if( lpCnt->dwStyle & CTS_EXTENDEDSEL )
          {
            // Clear the selection list if shift key not pressed.
            if( !bShiftKey || (lpCnt->lpCI->flCntAttr & CA_OWNERVSCROLL) )
            {
              ClearExtSelectList( hWnd, lpCnt );
              lpCnt->lpRecAnchor = NULL;
            }
          }

          lpOldFocusRec = lpCnt->lpCI->lpFocusRec;

          // Unfocus the old focus record.
          if( lpCnt->lpCI->lpFocusRec )
          {
            lpCnt->lpCI->lpFocusRec->flRecAttr &= ~CRA_CURSORED;
            InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
          }

          if( (lpCnt->dwStyle & CTS_ASYNCNOTIFY) && 
              (lpCnt->lpCI->flCntAttr & CA_OWNERVSCROLL) )
          {
            // There is a special case when ASYNC and OWNERVSCROLL are on.
            // We cannot be assured that scroll has actually been 
            // executed when the SendMessage returns. Therefore we will
            // simply tell the assoc to move the focus after the scroll.

            // Execute the scroll.
            if( vk == VK_HOME )
              (lpCnt->iView & CV_FLOW) ? FORWARD_WM_HSCROLL( hWnd, 0, SB_TOP, 0, SendMessage )
                                       : FORWARD_WM_VSCROLL( hWnd, 0, SB_TOP, 0, SendMessage );
            else if( vk == VK_END )
              (lpCnt->iView & CV_FLOW) ? FORWARD_WM_HSCROLL( hWnd, 0, SB_BOTTOM, 0, SendMessage )
                                       : FORWARD_WM_VSCROLL( hWnd, 0, SB_BOTTOM, 0, SendMessage );
            else if( vk == VK_PRIOR )
              (lpCnt->iView & CV_FLOW) ? FORWARD_WM_HSCROLL( hWnd, 0, SB_PAGEUP, 0, SendMessage )
                                       : FORWARD_WM_VSCROLL( hWnd, 0, SB_PAGEUP, 0, SendMessage );
            else
              (lpCnt->iView & CV_FLOW) ? FORWARD_WM_HSCROLL( hWnd, 0, SB_PAGEDOWN, 0, SendMessage )
                                       : FORWARD_WM_VSCROLL( hWnd, 0, SB_PAGEDOWN, 0, SendMessage );

            // Now tell the associate to set the focus.
            if( vk == VK_HOME || vk == VK_PRIOR )
              NotifyAssocWnd( hWnd, lpCnt, CN_OWNERSETFOCUSTOP, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
            else
              NotifyAssocWnd( hWnd, lpCnt, CN_OWNERSETFOCUSBOT, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
          }
          else
          {
            // Execute the scroll.
            if( vk == VK_HOME )
              (lpCnt->iView & CV_FLOW) ? FORWARD_WM_HSCROLL( hWnd, 0, SB_TOP, 0, SendMessage )
                                       : FORWARD_WM_VSCROLL( hWnd, 0, SB_TOP, 0, SendMessage );
            else if( vk == VK_END )
              (lpCnt->iView & CV_FLOW) ? FORWARD_WM_HSCROLL( hWnd, 0, SB_BOTTOM, 0, SendMessage )
                                       : FORWARD_WM_VSCROLL( hWnd, 0, SB_BOTTOM, 0, SendMessage );
            else if( vk == VK_PRIOR )
              (lpCnt->iView & CV_FLOW) ? FORWARD_WM_HSCROLL( hWnd, 0, SB_PAGEUP, 0, SendMessage )
                                       : FORWARD_WM_VSCROLL( hWnd, 0, SB_PAGEUP, 0, SendMessage );
            else
              (lpCnt->iView & CV_FLOW) ? FORWARD_WM_HSCROLL( hWnd, 0, SB_PAGEDOWN, 0, SendMessage )
                                       : FORWARD_WM_VSCROLL( hWnd, 0, SB_PAGEDOWN, 0, SendMessage );

            if( lpCnt->lpCI->lpFocusRec )
            {
              // If the end key was pressed give the last record the focus
              // else make the top record in display area the focus record.
              if( vk == VK_NEXT )
              {
                // Calc how many full records are in view. We want to scroll
                // the container up if the focus lands on the last record
                // and it is only partially in view. The only reason I am
                // adding 1 to the record area height is to accomadate apps
                // that have tried to size the container to show full records
                // but are off by one (the Simply products did this).
                cyRecordArea = lpCnt->cyClient - lpCnt->lpCI->cyTitleArea + 1;
                nFullRecsDisplayed = cyRecordArea / lpCnt->lpCI->cyRow;

                // If we are at the max scroll position (or there is no scroll
                // bar) use the number of records displayed for the calculation.
                // This insures that we can move the focus to the last record
                // even if it is not fully in view.
                if( lpCnt->nVscrollPos == lpCnt->nVscrollMax )
                  nRecs = lpCnt->lpCI->nRecsDisplayed;
                else
                  nRecs = nFullRecsDisplayed;

                if (lpCnt->iView & CV_FLOW)
                {
                  // just set the focus to the bottom right record
                  lpCol = GetLastColInView(hWnd, lpCnt);
                  lpCnt->lpCI->lpFocusRec = lpCol->lpTopRec;
                  lpCnt->lpCI->lpFocusRec = CntScrollRecList( hWnd, lpCol->lpTopRec, lpCol->nRecs );
                }
                else
                  lpCnt->lpCI->lpFocusRec = CntScrollRecList( hWnd, lpCnt->lpCI->lpTopRec, nRecs-1 );
              }
              else if( vk == VK_END )
              {
                if (lpCnt->iView & CV_FLOW)
                {
                  // get the last column and set the focus to the bottom record, i.e. the
                  // last record in the list.
                  lpCol = GetLastColInView( hWnd, lpCnt );
                  lpCnt->lpCI->lpFocusRec = CntScrollRecList( hWnd, lpCol->lpTopRec, lpCol->nRecs );
                }
                else
                  lpCnt->lpCI->lpFocusRec = CntScrollRecList( hWnd, lpCnt->lpCI->lpTopRec, lpCnt->lpCI->nRecsDisplayed-1 );
              }
              else
              {
                if (lpCnt->iView & CV_FLOW)
                {
                  // just set the focus to the top left record
                  lpCol = Get1stColInView(hWnd, lpCnt);
                  lpCnt->lpCI->lpFocusRec = lpCol->lpTopRec;
                }
                else
                  lpCnt->lpCI->lpFocusRec = lpCnt->lpCI->lpTopRec;
              }

              lpCnt->lpCI->lpFocusRec->flRecAttr |= CRA_CURSORED;

              // Now repaint the top record so the focus rect will appear.
              if( lpCnt->dwStyle & CTS_SINGLESEL )
              {
                // Repaint the entire new selected record.
                lpCnt->lpCI->lpSelRec = lpCnt->lpCI->lpFocusRec;
                lpCnt->lpCI->lpSelRec->flRecAttr |= CRA_SELECTED;
                InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
                bRecSelected = TRUE;
              }
              else if( lpCnt->dwStyle & CTS_EXTENDEDSEL )
              {
                // The selection moves with the focus record.
                if( !bShiftKey || (lpCnt->lpCI->flCntAttr & CA_OWNERVSCROLL) )
                {
                  lpCnt->lpCI->lpFocusRec->flRecAttr |= CRA_SELECTED;
                  InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
                  bRecSelected = TRUE;
                  if (lpCnt->lpRecAnchor==NULL)
                    lpCnt->lpRecAnchor = lpCnt->lpCI->lpFocusRec;
                }
                else
                {
                  // set the anchor for shift selection
                  // this points to the first record clicked on when beginning shift select
                  if (lpCnt->lpRecAnchor==NULL || !bShiftKey) // set new anchor
                    lpCnt->lpRecAnchor = lpCnt->lpCI->lpFocusRec;

                  if( vk == VK_PRIOR || vk == VK_HOME )
                    nDir = -1;
                  else
                    nDir = 1;

//                  BatchSelect( hWnd, lpCnt, lpOldFocusRec, lpCnt->lpCI->lpFocusRec, nDir );

                  // see if recanchor is before or after the old focus rec to determine
                  // whether we are selecting or deselecting
                  lpTempRec = lpOldFocusRec;
                  bFound = FALSE;
                  if (lpTempRec==lpCnt->lpRecAnchor)
                    bSel = FALSE;
                  else
                  {
                    while( lpTempRec )
                    {
                      if( lpTempRec == lpCnt->lpRecAnchor )
                      {
                        bFound = TRUE;
                        break;
                      }
                      lpTempRec = lpTempRec->lpPrev;
                    }
                    if ( (bFound && (vk==VK_PRIOR || vk==VK_HOME)) || (!bFound && (vk==VK_NEXT || vk==VK_END)) )
                      bSel = FALSE;
                    else
                      bSel = TRUE;
                  }

                  // Select or unselect the records from old focus to new record clicked on
                  lpTempRec = lpOldFocusRec;
                  while( lpTempRec )
                  {
                    if (lpTempRec==lpCnt->lpRecAnchor)
                    {
                      // toggle the selection state
                      bSel = !bSel;
                    }
                    else
                    {
                      // select records.
                      if( !(lpTempRec->flRecAttr & CRA_SELECTED) && bSel)
                      {
                        // Repaint the selected record and notify the parent.
                        lpTempRec->flRecAttr |= CRA_SELECTED;
                        InvalidateCntRecord( hWnd, lpCnt, lpTempRec, NULL, 1 );
                        NotifyAssocWnd( hWnd, lpCnt, CN_RECSELECTED, 0, lpTempRec, NULL, 0, 0, 0, NULL );
                      }
                      else 
                      {
                        if (!bSel && lpTempRec != lpCnt->lpCI->lpFocusRec)
                        {
                          lpTempRec->flRecAttr &= ~CRA_SELECTED;
                          NotifyAssocWnd( hWnd, lpCnt, CN_RECUNSELECTED, 0, lpTempRec, NULL, 0, 0, 0, NULL );
                        }
                        InvalidateCntRecord( hWnd, lpCnt, lpTempRec, NULL, 1 );
                      }
                    }

                    // Break out after we have done the end record.
                    if( lpTempRec == lpCnt->lpCI->lpFocusRec )
                      break;

                    // Get the next or previous record.
                    if( nDir == 1 )
                      lpTempRec = CntNextRec( lpTempRec );
                    else
                      lpTempRec = CntPrevRec( lpTempRec );
                  }
                }
              }
              else
              {
                // Repaint only the new focus cell.
                InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
              }

              // Tell associate wnd about the new focus record.
              NotifyAssocWnd( hWnd, lpCnt, CN_NEWFOCUSREC, 0, lpCnt->lpCI->lpFocusRec, NULL, 0, bShiftKey, bCtrlKey, NULL );

              // Tell associate wnd about the new selected record.
              if( bRecSelected )
                NotifyAssocWnd( hWnd, lpCnt, CN_RECSELECTED, 0, lpCnt->lpCI->lpFocusRec, NULL, 0, bShiftKey, bCtrlKey, NULL );
            }
          }
        }
        else
        {
          // User has locked the focus record so we will just send
          // the keystroke notification but NOT move the focus.
          if( vk == VK_HOME )
            NotifyAssocWnd( hWnd, lpCnt, CN_LK_HOME, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
          else if( vk == VK_END )
            NotifyAssocWnd( hWnd, lpCnt, CN_LK_END, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
          else if( vk == VK_PRIOR )
            NotifyAssocWnd( hWnd, lpCnt, CN_LK_PAGEUP, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
          else
            NotifyAssocWnd( hWnd, lpCnt, CN_LK_PAGEDOWN, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
        }
        break;

      case VK_UP:
        if( bCtrlKey )  
        {
          FORWARD_WM_KEYDOWN( hWnd, VK_HOME, 0, 0, SendMessage );
          break;
        }

        bRecSelected = FALSE;

        // If focus cell is not in view, scroll to it. This should
        // put the focus cell in view 1 row down from the top.
        if( !TxtIsFocusInView( hWnd, lpCnt ) )
          TxtScrollToFocus( hWnd, lpCnt, 0 );

        if( lpCntFrame->bScrollFocusRect && lpCnt->lpCI->lpFocusRec )
        {
          if( !lpCnt->lpCI->bFocusRecLocked )
          {
            // Get the position of the focus in the display area.
            // Remember that with mode=0 nothing happens in this call.
            nRecPos = InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 0 );

            // For single, extended, or block select we will chg the selection.
            if( lpCnt->dwStyle & CTS_SINGLESEL && lpCnt->lpCI->lpSelRec )
            {
              lpCnt->lpCI->lpSelRec->flRecAttr &= ~CRA_SELECTED;
              InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpSelRec, NULL, 1 );
              NotifyAssocWnd( hWnd, lpCnt, CN_RECUNSELECTED, 0, lpCnt->lpCI->lpSelRec, NULL, 0, bShiftKey, bCtrlKey, NULL );
            }
            else if( lpCnt->dwStyle & CTS_EXTENDEDSEL )
            {
              // Start a new selection list if shift key not pressed.
              if( !bShiftKey )
              {
                ClearExtSelectList( hWnd, lpCnt );
                lpCnt->lpRecAnchor = NULL;
                bFound = TRUE;
              }
              else
              {
                // if the anchor record is after the focus record, then select, else
                // deselect the record  
                if (lpCnt->lpRecAnchor)
                {
                  lpTempRec = lpCnt->lpCI->lpFocusRec;
                  bFound = FALSE;
                  while( lpTempRec )
                  {
                    if( lpTempRec == lpCnt->lpRecAnchor )
                    {
                      bFound = TRUE;
                      break;
                    }
                    lpTempRec = lpTempRec->lpNext;
                  }
                  if (!bFound && nRecPos > 0)
                  {
                    lpCnt->lpCI->lpFocusRec->flRecAttr &= ~CRA_SELECTED;
                    InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
                    NotifyAssocWnd( hWnd, lpCnt, CN_RECUNSELECTED, 0, lpCnt->lpCI->lpFocusRec, NULL, 0, bShiftKey, bCtrlKey, NULL );
                  }
                }
                else
                  bFound = TRUE;
              }
            }

            // Unfocus the old focus record.
            lpCnt->lpCI->lpFocusRec->flRecAttr &= ~CRA_CURSORED;
            InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );

            if( nRecPos <= 0 && (lpCnt->dwStyle & CTS_ASYNCNOTIFY) && 
                        (lpCnt->lpCI->flCntAttr & CA_OWNERVSCROLL) &&
                        (!(lpCnt->iView & CV_FLOW)) )
            {
              // When the focus cell moves out of the display area and
              // it is necessary to scroll before moving the focus there
              // is a special case when ASYNC and OWNERVSCROLL are on.
              // We cannot be assured that scroll has actually been 
              // executed when the SendMessage returns. Therefore we will
              // simply tell the assoc to move the focus after the scroll.
              FORWARD_WM_VSCROLL( hWnd, 0, SB_LINEUP, 0, SendMessage );
              NotifyAssocWnd( hWnd, lpCnt, CN_OWNERSETFOCUSTOP, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
            }
            else
            {
              if( nRecPos > 0 )
              {
                // Focus rect is in the display area but has not hit the top
                // yet. No scroll is necessary. Focus rect just moves up.
                lpCnt->lpCI->lpFocusRec = CntScrollRecList( hWnd, lpCnt->lpCI->lpFocusRec, -1 );
              }
              else if (!(lpCnt->iView & CV_FLOW))
              {
                // Focus rect moved out of the display area so we will do a
                // line up scroll and then make the top record the focus.
                // if flowed, then focus rect stays the same
                FORWARD_WM_VSCROLL( hWnd, 0, SB_LINEUP, 0, SendMessage );
                lpCnt->lpCI->lpFocusRec = lpCnt->lpCI->lpTopRec;
              }

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
                // set the anchor for shift selection
                // this points to the first record clicked on when beginning shift select
                if (lpCnt->lpRecAnchor==NULL || !bShiftKey) // set new anchor
                  lpCnt->lpRecAnchor = lpCnt->lpCI->lpFocusRec;

                if( !(lpCnt->lpCI->lpFocusRec->flRecAttr & CRA_SELECTED) && bFound )
                {
                  // The selection moves with the focus record.
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
            NotifyAssocWnd( hWnd, lpCnt, CN_LK_ARROW_UP, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
          }
        }
        else
        {
          if (!(lpCnt->iView & CV_FLOW))
            FORWARD_WM_VSCROLL( hWnd, 0, SB_LINEUP, 0, SendMessage );
        }
        break;

      case VK_DOWN:
        if( bCtrlKey )  
        {
          FORWARD_WM_KEYDOWN( hWnd, VK_END, 0, 0, SendMessage );
          break;
        }

        bRecSelected = FALSE;

        // If focus cell is not in view, scroll to it. This should
        // put the focus cell in view 1 row up from the bottom.
        if( !TxtIsFocusInView( hWnd, lpCnt ) )
          TxtScrollToFocus( hWnd, lpCnt, 1 );

        if( lpCntFrame->bScrollFocusRect && lpCnt->lpCI->lpFocusRec )
        {
          if( !lpCnt->lpCI->bFocusRecLocked )
          {
            // Get the position of the focus in the display area.
            // Remember that with mode=0 nothing happens in this call.
            nRecPos = InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 0 );

            // For single or extended select we will chg the selected record.
            if( lpCnt->dwStyle & CTS_SINGLESEL && lpCnt->lpCI->lpSelRec )
            {
              lpCnt->lpCI->lpSelRec->flRecAttr &= ~CRA_SELECTED;
              InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpSelRec, NULL, 1 );
              NotifyAssocWnd( hWnd, lpCnt, CN_RECUNSELECTED, 0, lpCnt->lpCI->lpSelRec, NULL, 0, bShiftKey, bCtrlKey, NULL );
            }
            else if( lpCnt->dwStyle & CTS_EXTENDEDSEL )
            {
              // Start a new selection list if shift key not pressed.
              if( !bShiftKey )
              {
                ClearExtSelectList( hWnd, lpCnt );
                lpCnt->lpRecAnchor = NULL;
                bFound = TRUE;
              }
              else 
              {
                // if the anchor record is before the new focus record, then select,
                // else deselect.
                if (lpCnt->lpRecAnchor)
                {
                  lpTempRec = lpCnt->lpCI->lpFocusRec;
                  bFound = FALSE;
                  while( lpTempRec )
                  {
                    if( lpTempRec == lpCnt->lpRecAnchor )
                    {
                      bFound = TRUE;
                      break;
                    }
                    lpTempRec = lpTempRec->lpPrev;
                  }
                  if (!bFound && nRecPos < lpCnt->lpCI->nRecsDisplayed - 1)
                  {
                    lpCnt->lpCI->lpFocusRec->flRecAttr &= ~CRA_SELECTED;
                    InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
                    NotifyAssocWnd( hWnd, lpCnt, CN_RECUNSELECTED, 0, lpCnt->lpCI->lpFocusRec, NULL, 0, bShiftKey, bCtrlKey, NULL );
                  }
                }
                else
                  bFound = TRUE;
              }
            }

            // Unfocus the old focus record.
            lpCnt->lpCI->lpFocusRec->flRecAttr &= ~CRA_CURSORED;
            InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );

            if( !(nRecPos >= 0 && nRecPos < lpCnt->lpCI->nRecsDisplayed - 1 ) &&
                (lpCnt->dwStyle & CTS_ASYNCNOTIFY) && (lpCnt->lpCI->flCntAttr & CA_OWNERVSCROLL) )
            {
              // When the focus cell moves out of the display area and
              // it is necessary to scroll before moving the focus there
              // is a special case when ASYNC and OWNERVSCROLL are on.
              // We cannot be assured that scroll has actually been 
              // executed when the SendMessage returns. Therefore we will
              // simply tell the assoc to move the focus after the scroll.
              if (!(lpCnt->iView & CV_FLOW))
              {
                FORWARD_WM_VSCROLL( hWnd, 0, SB_LINEDOWN, 0, SendMessage );
                NotifyAssocWnd( hWnd, lpCnt, CN_OWNERSETFOCUSBOT, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
              }
            }
            else
            {
              // Calc how many full records are in view. We want to scroll
              // the container up if the focus lands on the last record
              // and it is only partially in view. The only reason I am
              // adding 1 to the record area height is to accomadate apps
              // that have tried to size the container to show full records
              // but are off by one (the Simply products did this).
              cyRecordArea = lpCnt->cyClient - lpCnt->lpCI->cyTitleArea - lpCnt->lpCI->cyColTitleArea + 1;
              nFullRecsDisplayed = cyRecordArea / lpCnt->lpCI->cyRow;

              // If we are at the max scroll position (or there is no scroll
              // bar) use the number of records displayed for the calculation.
              // This insures that we can move the focus to the last record
              // even if it is not fully in view.
              if( lpCnt->nVscrollPos == lpCnt->nVscrollMax || (lpCnt->iView & CV_FLOW)) 
                nRecs = lpCnt->lpCI->nRecsDisplayed;
              else
                nRecs = nFullRecsDisplayed;

              if( nRecPos >= 0 && nRecPos < nRecs - 1 )
              {
                // Focus rect is in display area but will not hit the bottom
                // yet. No scroll is necessary. Focus rect just moves down.
                lpCnt->lpCI->lpFocusRec = CntScrollRecList( hWnd, lpCnt->lpCI->lpFocusRec, 1 );
              }
              else if (!(lpCnt->iView & CV_FLOW))
              {
                // Focus rect moved out of the display area so we will do a
                // line down scroll and then make the bottom record the focus.
                FORWARD_WM_VSCROLL( hWnd, 0, SB_LINEDOWN, 0, SendMessage );
                lpCnt->lpCI->lpFocusRec = CntScrollRecList( hWnd, lpCnt->lpCI->lpTopRec, nRecs-1 );
              }

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
                // set the anchor for shift selection
                // this points to the first record clicked on when beginning shift select
                if (lpCnt->lpRecAnchor==NULL || !bShiftKey) // set new anchor
                  lpCnt->lpRecAnchor = lpCnt->lpCI->lpFocusRec;

                if( !(lpCnt->lpCI->lpFocusRec->flRecAttr & CRA_SELECTED) && bFound )
                {
                  // The selection moves with the focus record.
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
            // the keystroke notification but not move the focus.
            NotifyAssocWnd( hWnd, lpCnt, CN_LK_ARROW_DOWN, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
          }
        }
        else
        {
          if (!(lpCnt->iView & CV_FLOW))
            FORWARD_WM_VSCROLL( hWnd, 0, SB_LINEDOWN, 0, SendMessage );
        }
        break;

      case VK_LEFT:
        if( bCtrlKey )  
        {
          FORWARD_WM_KEYDOWN( hWnd, VK_HOME, 0, 0, SendMessage );
          break;
        }

        bRecSelected = FALSE;

        // If focus cell is not in view, scroll to it. This should
        // put the focus cell in view 1 row down from the top.
        if( !TxtIsFocusInView( hWnd, lpCnt ) )
          TxtScrollToFocus( hWnd, lpCnt, 0 );

        if( lpCntFrame->bScrollFocusRect && lpCnt->lpCI->lpFocusRec && (lpCnt->iView & CV_FLOW) )
        {
          if( !lpCnt->lpCI->bFocusRecLocked )
          {
            // For single or extended select we will chg the selected record.
            if( lpCnt->dwStyle & CTS_SINGLESEL && lpCnt->lpCI->lpSelRec )
            {
              lpCnt->lpCI->lpSelRec->flRecAttr &= ~CRA_SELECTED;
              InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpSelRec, NULL, 1 );
              NotifyAssocWnd( hWnd, lpCnt, CN_RECUNSELECTED, 0, lpCnt->lpCI->lpSelRec, NULL, 0, bShiftKey, bCtrlKey, NULL );
            }
            else if( lpCnt->dwStyle & CTS_EXTENDEDSEL )
            {
              if( !bShiftKey )
              {
                ClearExtSelectList( hWnd, lpCnt );
                lpCnt->lpRecAnchor = NULL;
              }
            }

            lpOldFocusRec = lpCnt->lpCI->lpFocusRec;

            // Unfocus the old focus record and get its position.
            lpCnt->lpCI->lpFocusRec->flRecAttr &= ~CRA_CURSORED;
            nRecPos = InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );

            // Get a pointer to the head or the previous column
            // if focus rec not in first column, move left, otherwise don't change focus
            lpTempRec = lpCnt->lpColHead->lpTopRec;
            bFound    = FALSE;
            for (i=0; i<lpCnt->lpColHead->nRecs && lpTempRec; i++)
            {
              if (lpTempRec==lpCnt->lpCI->lpFocusRec)
                bFound = TRUE;
              lpTempRec = lpTempRec->lpNext;
            }
            if (!bFound)  
              lpCnt->lpCI->lpFocusRec = CntScrollRecList( hWnd, lpCnt->lpCI->lpFocusRec, -lpCnt->lpCI->nRecsDisplayed );

            // If focus column is hidden find the 1st non-hidden before it.
            nRecPos = GetRecXPos( lpCnt, lpCnt->lpCI->lpFocusRec, &xStrCol, &xEndCol );
            if( xEndCol < 0 || xStrCol > lpCnt->cxClient )
            {
              if (xEndCol < 0)
                lpCol = Get1stColInView(hWnd, lpCnt);
              else
                lpCol = GetLastColInView(hWnd, lpCnt);
            
              // set the focus to the top of the column and scroll down to its vertical position
              lpCnt->lpCI->lpFocusRec = CntScrollRecList( hWnd, lpCol->lpTopRec, min(lpCol->nRecs,nRecPos) );

              // Get the starting xcoord of the new focus field.
              nRecPos = GetRecXPos( lpCnt, lpCnt->lpCI->lpFocusRec, &xStrCol, &xEndCol );
            }

            if( xStrCol < 0 || xStrCol > lpCnt->cxClient )
            {
              // Focus rect was not in display area or it is now moving
              // out of the display area so we will do a scroll.
              nInc = xStrCol / lpCnt->lpCI->cxChar;
              nInc += lpCnt->nHscrollPos;
              FORWARD_WM_HSCROLL( hWnd, 0, SB_THUMBPOSITION, nInc, SendMessage );
            }

            // Now repaint the new focus field.
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
              if( !bShiftKey )
              {
                // The selection moves with the focus record.
                lpCnt->lpCI->lpFocusRec->flRecAttr |= CRA_SELECTED;
                InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
                bRecSelected = TRUE;
                if (lpCnt->lpRecAnchor==NULL)
                  lpCnt->lpRecAnchor = lpCnt->lpCI->lpFocusRec;
              }
              else
              {
//                BatchSelect( hWnd, lpCnt, lpOldFocusRec, lpCnt->lpCI->lpFocusRec, -1 );

                // see if recanchor is before or after the old focus rec to determine
                // whether we are selecting or deselecting
                lpTempRec = lpOldFocusRec;
                bFound = FALSE;
                if (lpTempRec==lpCnt->lpRecAnchor)
                  bSel = FALSE;
                else
                {
                  while( lpTempRec )
                  {
                    if( lpTempRec == lpCnt->lpRecAnchor )
                    {
                      bFound = TRUE;
                      break;
                    }
                    lpTempRec = lpTempRec->lpPrev;
                  }
                  if ( bFound )
                    bSel = FALSE;
                  else
                    bSel = TRUE;
                }

                // Select or unselect the records from old focus to new record clicked on
                lpTempRec = lpOldFocusRec;
                while( lpTempRec )
                {
                  if (lpTempRec==lpCnt->lpRecAnchor)
                  {
                    // toggle the selection state
                    bSel = !bSel;
                  }
                  else
                  {
                    // select records.
                    if( !(lpTempRec->flRecAttr & CRA_SELECTED) && bSel)
                    {
                      // Repaint the selected record and notify the parent.
                      lpTempRec->flRecAttr |= CRA_SELECTED;
                      InvalidateCntRecord( hWnd, lpCnt, lpTempRec, NULL, 1 );
                      NotifyAssocWnd( hWnd, lpCnt, CN_RECSELECTED, 0, lpTempRec, NULL, 0, 0, 0, NULL );
                    }
                    else 
                    {
                      if (!bSel && lpTempRec != lpCnt->lpCI->lpFocusRec)
                      {
                        lpTempRec->flRecAttr &= ~CRA_SELECTED;
                        NotifyAssocWnd( hWnd, lpCnt, CN_RECUNSELECTED, 0, lpTempRec, NULL, 0, 0, 0, NULL );
                      }
                      InvalidateCntRecord( hWnd, lpCnt, lpTempRec, NULL, 1 );
                    }
                  }

                  // Break out after we have done the end record.
                  if( lpTempRec == lpCnt->lpCI->lpFocusRec )
                    break;

                  // Get the previous record.
                  lpTempRec = CntPrevRec( lpTempRec );
                }
              }
            }
            else
            {
              InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
            }

            // Tell associate wnd about the keystroke and new focus record.
            NotifyAssocWnd( hWnd, lpCnt, CN_NEWFOCUSREC, 0, lpCnt->lpCI->lpFocusRec, NULL, 0, bShiftKey, bCtrlKey, NULL );
            if( bRecSelected )
              NotifyAssocWnd( hWnd, lpCnt, CN_RECSELECTED, 0, lpCnt->lpCI->lpFocusRec, NULL, 0, bShiftKey, bCtrlKey, NULL );
          }
          else
          {
            // User has locked the focus record so we will just send
            // the keystroke notification but not move the focus.
            NotifyAssocWnd( hWnd, lpCnt, CN_LK_ARROW_LEFT, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
          }
        }
        else
        {
          FORWARD_WM_HSCROLL( hWnd, 0, SB_LINEUP, 0, SendMessage );
        }
        break;

      case VK_RIGHT:
        if( bCtrlKey )  
        {
          FORWARD_WM_KEYDOWN( hWnd, VK_END, 0, 0, SendMessage );
          break;
        }

        bRecSelected = FALSE;

        // If focus cell is not in view, scroll to it. This should
        // put the focus cell in view 1 row down from the top.
        if( !TxtIsFocusInView( hWnd, lpCnt ) )
          TxtScrollToFocus( hWnd, lpCnt, 0 );

        if( lpCntFrame->bScrollFocusRect && lpCnt->lpCI->lpFocusRec && (lpCnt->iView & CV_FLOW) )
        {
          if( !lpCnt->lpCI->bFocusFldLocked )
          {
            // For single or extended select we will chg the selected record.
            if( lpCnt->dwStyle & CTS_SINGLESEL && lpCnt->lpCI->lpSelRec )
            {
              lpCnt->lpCI->lpSelRec->flRecAttr &= ~CRA_SELECTED;
              InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpSelRec, NULL, 1 );
              NotifyAssocWnd( hWnd, lpCnt, CN_RECUNSELECTED, 0, lpCnt->lpCI->lpSelRec, NULL, 0, bShiftKey, bCtrlKey, NULL );
            }
            else if( lpCnt->dwStyle & CTS_EXTENDEDSEL )
            {
              if( !bShiftKey )
              {
                ClearExtSelectList( hWnd, lpCnt );
                lpCnt->lpRecAnchor = NULL;
              }
            }

            lpOldFocusRec = lpCnt->lpCI->lpFocusRec;

            // Unfocus the old focus record and get its position.
            lpCnt->lpCI->lpFocusRec->flRecAttr &= ~CRA_CURSORED;
            nRecPos = InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );

            // Get a pointer to the tail or the next field.
            // if focus rec not in last column, move right, otherwise don't change focus
            lpTempRec = lpCnt->lpColTail->lpTopRec;
            bFound    = FALSE;
            for (i=0; i<lpCnt->lpColTail->nRecs && lpTempRec; i++)
            {
              if (lpTempRec==lpCnt->lpCI->lpFocusRec)
                bFound = TRUE;
              lpTempRec = lpTempRec->lpNext;
            }
            if (!bFound)  
              lpCnt->lpCI->lpFocusRec = CntScrollRecList( hWnd, lpCnt->lpCI->lpFocusRec, lpCnt->lpCI->nRecsDisplayed );

            // If field is hidden find the 1st non-hidden after it.
            nRecPos = GetRecXPos( lpCnt, lpCnt->lpCI->lpFocusRec, &xStrCol, &xEndCol );
            if( xEndCol < 0 || xStrCol > lpCnt->cxClient )
            {
              if (xEndCol < 0)
                lpCol = Get1stColInView(hWnd, lpCnt);
              else
                lpCol = GetLastColInView(hWnd, lpCnt);
            
              // set the focus to the top of the column and scroll down to its vertical position
              lpCnt->lpCI->lpFocusRec = CntScrollRecList( hWnd, lpCol->lpTopRec, min(lpCol->nRecs,nRecPos) );

              // Get the starting xcoord of the new focus field.
              nRecPos = GetRecXPos( lpCnt, lpCnt->lpCI->lpFocusRec, &xStrCol, &xEndCol );
            }

            if( xEndCol < 0 || xEndCol > lpCnt->cxClient )
            {
              // Focus rect was not in display area or it is now moving
              // out of the display area so we will do a scroll.
              nInc = (xEndCol - lpCnt->cxClient - 1) / lpCnt->lpCI->cxChar + 1;
              nInc += lpCnt->nHscrollPos;
              FORWARD_WM_HSCROLL( hWnd, 0, SB_THUMBPOSITION, nInc, SendMessage );
            }

            // Now repaint the new focus field.
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
              if( !bShiftKey )
              {
                // The selection moves with the focus record.
                lpCnt->lpCI->lpFocusRec->flRecAttr |= CRA_SELECTED;
                InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
                bRecSelected = TRUE;
                if (lpCnt->lpRecAnchor==NULL)
                  lpCnt->lpRecAnchor = lpCnt->lpCI->lpFocusRec;
              }
              else
              {
//                BatchSelect( hWnd, lpCnt, lpOldFocusRec, lpCnt->lpCI->lpFocusRec, 1 );

                // see if recanchor is before or after the old focus rec to determine
                // whether we are selecting or deselecting
                lpTempRec = lpOldFocusRec;
                bFound = FALSE;
                if (lpTempRec==lpCnt->lpRecAnchor)
                  bSel = FALSE;
                else
                {
                  while( lpTempRec )
                  {
                    if( lpTempRec == lpCnt->lpRecAnchor )
                    {
                      bFound = TRUE;
                      break;
                    }
                    lpTempRec = lpTempRec->lpPrev;
                  }
                  if ( bFound )
                    bSel = TRUE;
                  else
                    bSel = FALSE;
                }

                // Select or unselect the records from old focus to new record clicked on
                lpTempRec = lpOldFocusRec;
                while( lpTempRec )
                {
                  if (lpTempRec==lpCnt->lpRecAnchor)
                  {
                    // toggle the selection state
                    bSel = !bSel;
                  }
                  else
                  {
                    // select records.
                    if( !(lpTempRec->flRecAttr & CRA_SELECTED) && bSel)
                    {
                      // Repaint the selected record and notify the parent.
                      lpTempRec->flRecAttr |= CRA_SELECTED;
                      InvalidateCntRecord( hWnd, lpCnt, lpTempRec, NULL, 1 );
                      NotifyAssocWnd( hWnd, lpCnt, CN_RECSELECTED, 0, lpTempRec, NULL, 0, 0, 0, NULL );
                    }
                    else 
                    {
                      if (!bSel && lpTempRec != lpCnt->lpCI->lpFocusRec)
                      {
                        lpTempRec->flRecAttr &= ~CRA_SELECTED;
                        NotifyAssocWnd( hWnd, lpCnt, CN_RECUNSELECTED, 0, lpTempRec, NULL, 0, 0, 0, NULL );
                      }
                      InvalidateCntRecord( hWnd, lpCnt, lpTempRec, NULL, 1 );
                    }
                  }

                  // Break out after we have done the end record.
                  if( lpTempRec == lpCnt->lpCI->lpFocusRec )
                    break;

                  // Get the previous record.
                  lpTempRec = CntNextRec( lpTempRec );
                }
              }
            }
            else
            {
              InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
            }

            // Tell associate wnd about the keystroke and new focus record.
            NotifyAssocWnd( hWnd, lpCnt, CN_NEWFOCUSREC, 0, lpCnt->lpCI->lpFocusRec, NULL, 0, bShiftKey, bCtrlKey, NULL );
            if( bRecSelected )
              NotifyAssocWnd( hWnd, lpCnt, CN_RECSELECTED, 0, lpCnt->lpCI->lpFocusRec, NULL, 0, bShiftKey, bCtrlKey, NULL );
          }
          else
          {
            // User has locked the focus record so we will just send
            // the keystroke notification but not move the focus.
            NotifyAssocWnd( hWnd, lpCnt, CN_LK_ARROW_RIGHT, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
          }
        }
        else
        {
          FORWARD_WM_HSCROLL( hWnd, 0, SB_LINEDOWN, 0, SendMessage );
        }
        break;

      case VK_RETURN:
        // If focus cell is not in view, scroll to it. This should
        // put the focus cell in view 1 row down from the top.
        if( !TxtIsFocusInView( hWnd, lpCnt ) )
          TxtScrollToFocus( hWnd, lpCnt, 0 );

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
            if( !TxtIsFocusInView( hWnd, lpCnt ) )
              TxtScrollToFocus( hWnd, lpCnt, 0 );

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
            if( !TxtIsFocusInView( hWnd, lpCnt ) )
              TxtScrollToFocus( hWnd, lpCnt, 0 );

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
            // Clear the wide focus rect if the app didn't have it originally.
//            if( !lpCnt->bWideFocusRect )
//              lpCnt->lpCI->flCntAttr &= ~CA_WIDEFOCUSRECT;
            lpCnt->dwStyle &= ~CTS_MULTIPLESEL;
            lpCnt->dwStyle |=  CTS_EXTENDEDSEL;
            lpCnt->bSimulateMulSel = FALSE;  
          }
          else
          {
            lpCnt->dwStyle &= ~CTS_EXTENDEDSEL;
            lpCnt->dwStyle |=  CTS_MULTIPLESEL;
//            lpCnt->lpCI->flCntAttr |= CA_WIDEFOCUSRECT;
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

    // reset the id
    lpCnt->iInputID = -1;

    return;
}

VOID Txt_OnRButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags )
{
    LPCONTAINER lpCnt = GETCNT(hWnd);
    POINT       pt;

    // If mouse input is disabled just beep and return.
    if( !lpCnt->lpCI->bSendMouseEvents )
    {
      MessageBeep(0);
      return;
    }

    // notify user of event
    if( !fDoubleClick )
    {
      pt.x = x;
      pt.y = y;
      NotifyAssocWnd( hWnd, lpCnt, CN_RBTNCLK, 0, NULL, NULL, 0, 0, 0, (LPVOID)&pt );
    }

    return;
}

VOID Txt_OnLButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags )
{
    LPCONTAINER  lpCnt = GETCNT(hWnd);
    LPFLOWCOLINFO lpCol;
    LPRECORDCORE lpRec, lpOldSelRec;
    LPRECORDCORE lpRec2;
    RECT         rect, rectTitle, rectRecs;
    RECT         rectTitleBtn;
    POINT        pt;
    BOOL         bShiftKey, bCtrlKey;
    BOOL         bRecSelected, bRecUnSelected;
    BOOL         bClickedTitle, bClickedRecord;
    BOOL         bNewRecList;
    BOOL         bNewFocusRec, bNewRecSelected;
    BOOL         bClickedTitleBtn;
    int          xStrCol, xEndCol, i;
    int          cxPxlWidth;
    int          nRec, nSelRecs, nSelCols;

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

    bClickedTitle = bClickedRecord = bClickedTitleBtn = FALSE;

    // Get the bottom yCoord of the last visible record.
    lpCnt->yLastRec = lpCnt->lpCI->cyTitleArea;
    if( lpCnt->lpCI->lpTopRec )
    {
      lpRec = lpCnt->lpCI->lpTopRec;
      for( i=0; i < lpCnt->lpCI->nRecsDisplayed; i++ )
      {
        lpCnt->yLastRec += lpCnt->lpCI->cyRow; // add a row height
        if( lpRec->lpNext )           // break if we hit the last record
          lpRec = lpRec->lpNext;
        else
          break;
      }
    }

    // Set up rects to determine where the user clicked.
    rect.left   = 0;
    rect.top    = 0;
    rect.right  = lpCnt->cxClient;
    rect.bottom = min(lpCnt->cyClient, lpCnt->yLastRec);

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
    rectRecs.right = min(rect.right, lpCnt->xLastCol);
    rectRecs.top   = rectTitle.bottom;

    // Set the flag for the object the user clicked on.
    if( PtInRect(&rectTitle, pt) )
      bClickedTitle = TRUE;
    else if( PtInRect(&rectTitleBtn, pt) )
      bClickedTitleBtn = TRUE;
    else if( PtInRect(&rectRecs, pt) )
      bClickedRecord = TRUE;

    // If user clicked in a column find out which one it was.
    if( bClickedRecord )
    {
      lpCol = TxtGetColAtPt( lpCnt, &pt, &xStrCol, &xEndCol );
      if( !lpCol )   // this shouldn't ever happen, but...
        return;  
      
      // reset bClickedRecord if in flowed view and user clicked on the last column
      // but below the last displayed record
      if (lpCnt->iView & CV_FLOW && !lpCol->lpNext)
      {
        nRec = (pt.y - rectRecs.top) / lpCnt->lpCI->cyRow + 1;
        if (nRec > lpCol->nRecs)
          bClickedRecord = FALSE;
      }
    }

    // If user clicked in a record find out which one it was.
    if( bClickedRecord )
    {
      // Calc which record the user clicked on.
      nRec = (pt.y - rectRecs.top) / lpCnt->lpCI->cyRow;
      
      // save the rect of this record/field cell.
      lpCnt->rectCursorFld.top    = rectRecs.top + (nRec * lpCnt->lpCI->cyRow);
      lpCnt->rectCursorFld.bottom = lpCnt->rectCursorFld.top + lpCnt->lpCI->cyRow;
//      if( lpCnt->dwStyle & CTS_BLOCKSEL )
//      {
        lpCnt->rectCursorFld.left  = xStrCol;
        lpCnt->rectCursorFld.right = xEndCol;
//      }
//      else
//      {
//        lpCnt->rectCursorFld.left  = rectRecs.left;
//        lpCnt->rectCursorFld.right = rectRecs.right;
//      }

      // Save the record rect so we can track mouse movement against it.
      CopyRect( &lpCnt->rectRecArea, &rectRecs );
      lpCnt->nCurRec = nRec;

      // Scroll the record list to the clicked-on record.
      if( lpCnt->lpCI->lpTopRec )
      {
        if (lpCnt->iView & CV_FLOW)
          lpRec = CntScrollRecList( hWnd, lpCol->lpTopRec, nRec );
        else
          lpRec = CntScrollRecList( hWnd, lpCnt->lpCI->lpTopRec, nRec );
      }
      else
        bClickedRecord = FALSE;
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
      else if( bClickedRecord && lpRec )
      {
        // End special mode for extended select - simulate multiple select.
        if( (lpCnt->dwStyle & CTS_MULTIPLESEL) && lpCnt->bSimulateMulSel )
        {
//          if( !lpCnt->bWideFocusRect )
//            lpCnt->lpCI->flCntAttr &= ~CA_WIDEFOCUSRECT;
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
            // record if only the field selection changed.
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
            // Get the state of the shift and control keys.
            bShiftKey = GetAsyncKeyState( VK_SHIFT ) & ASYNC_KEY_DOWN;
            bCtrlKey  = GetAsyncKeyState( VK_CONTROL ) & ASYNC_KEY_DOWN;

            // clear and set the anchor for shift selection
            // this points to the first record clicked on when beginning shift select
            if (!bShiftKey)
              lpCnt->lpRecAnchor = NULL;
            if (lpCnt->lpRecAnchor==NULL || !bShiftKey) // set new anchor
              lpCnt->lpRecAnchor = lpRec;
            
            // If the control key is not pressed start a new selection list.
            if( !bCtrlKey )
            {
              ClearExtSelectList( hWnd, lpCnt );

              // Force an update of the clicked-on record.
              InvalidateCntRecord( hWnd, lpCnt, lpRec, NULL, 2 );
            }

            // Tell the assoc wnd we are starting a record selection.
            // They should not swap records out of the record list
            // until done with the selection.
            NotifyAssocWnd( hWnd, lpCnt, CN_BGNRECSELECTION, 0, NULL, NULL, 0, 0, 0, NULL );

            if( bShiftKey )
            {
              LPRECORDCORE lpRec2=lpRec;
              BOOL         bFound=FALSE;

              // Find the anchor and select all the records
              // between the clicked on record and the anchor.
              if( !lpCnt->lpRecAnchor || lpCnt->lpRecAnchor == lpRec )
                nSelRecs = 0;
              else
              {
                nSelRecs = 0;
                // First look forward in the list for the anchor
                while( lpRec2 )
                {
                  if( lpRec2 == lpCnt->lpRecAnchor )
                  {
                    bFound = TRUE;
                    break;
                  }
                  lpRec2 = lpRec2->lpNext;
                  nSelRecs++;
                }

                // If not found forward in the list look backwards.
                if( !bFound )
                {
                  lpRec2 = lpRec;
                  nSelRecs = 0;
                  while( lpRec2 )
                  {
                    if( lpRec2 == lpCnt->lpRecAnchor )
                    {
                      bFound = TRUE;
                      break;
                    }
                    lpRec2 = lpRec2->lpPrev;
                    nSelRecs--;
                  }
                }

                // If not found it must be currently swapped off the record list. (this shouldn't happen)
                if( !bFound )
                  nSelRecs = 0;
              }
              // returns a 0-based distance - adjust
              nSelRecs = nSelRecs < 0 ? --nSelRecs : ++nSelRecs;
#if 0
              // Find where the focus record is and select all the records
              // between the clicked on record and the focus record.
              nSelRecs = FindFocusRec( hWnd, lpCnt, lpRec );

              // FindFocusRec returns a 0-based distance.
              nSelRecs = nSelRecs < 0 ? --nSelRecs : ++nSelRecs;
#endif
            }
            else
            {
              // Track the user's record selection with the mouse.
              if (lpCnt->iView & CV_FLOW)
                TxtTrackSelection( hWnd, lpCnt, lpRec, lpCol, &pt, &nSelRecs, &nSelCols, TRUE );
              else
                TxtTrackSelection( hWnd, lpCnt, lpRec, lpCol, &pt, &nSelRecs, &nSelCols, FALSE );
            
              // find out total number of records selected
              if( nSelCols < 0 )
                nSelRecs = -(-nSelCols*lpCnt->lpCI->nRecsDisplayed - nSelRecs + 1);
              else if( nSelCols > 0 )
                nSelRecs = nSelCols*lpCnt->lpCI->nRecsDisplayed + nSelRecs + 1;
              else
              {
                if (nSelRecs < 0)
                  nSelRecs--;
                else
                  nSelRecs++;
              }
            }

            // Now do the actual record selection. 
            TxtExtSelect( hWnd, lpCnt, lpRec, nSelRecs, &lpRec2 );

            // The new focus rec will be the last one selected unless the
            // user was using the shift key. If he used the shift key the
            // new focus will be the record he clicked on.
            if( !bShiftKey && lpRec2 )
              lpRec = lpRec2;

            // Tell the assoc wnd we are done with the record selection.
            NotifyAssocWnd( hWnd, lpCnt, CN_ENDRECSELECTION, 0, NULL, NULL, 0, 0, 0, NULL );
          }

          // Unfocus the previously focused record/field and repaint it.
          if( lpCnt->lpCI->lpFocusRec )
          {
            lpCnt->lpCI->lpFocusRec->flRecAttr &= ~CRA_CURSORED;
            InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
          }

          // Remember if the field or record changed.
          bNewFocusRec = lpCnt->lpCI->lpFocusRec != lpRec ? TRUE : FALSE;

          // This is the new focus record
          lpCnt->lpCI->lpFocusRec = lpRec;
          lpRec->flRecAttr |= CRA_CURSORED;

          // Invalidate the clicked-on record and repaint.
          // If there is no record selection enabled just invalidate
          // the new focus field instead of the entire record.
          if( lpCnt->dwStyle & CTS_SINGLESEL && bNewRecSelected )
            InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
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
      
      // now check to see if were in drag mode
      if( (!bClickedTitleBtn) && (!bClickedTitle) &&
         (FALSE /*lpCnt->dwStyle & CTS_DRAGDROP  || lpCnt->dwStyle & CTS_DRAGONLY*/) )
         DoDragTracking(hWnd); 
    }

    // reset the id
    lpCnt->iInputID = -1;

    return;
}

VOID Txt_OnLButtonUp( HWND hWnd, int x, int y, UINT keyFlags )
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
    lpCnt->lp1stSelRec  = NULL;
    SetRectEmpty( &lpCnt->rectPressedBtn );
    SetRectEmpty( &lpCnt->rectCursorFld  );
    StateClear( lpCnt, CNTSTATE_MOUSEOUT );
    StateClear( lpCnt, CNTSTATE_CLKEDTTLBTN );

    return;
}  

UINT Txt_OnGetDlgCode( HWND hWnd, LPMSG lpmsg )
{
    UINT       uRet;
    
    uRet = Dtl_OnGetDlgCode( hWnd, lpmsg );

    return uRet;
}

VOID Txt_OnMouseMove( HWND hWnd, int x, int y, UINT keyFlags )
{
    LPCONTAINER  lpCnt = GETCNT(hWnd);
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
      rectRecs.right = min(rect.right, lpCnt->xLastCol);
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

VOID Txt_OnNCLButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest )
{
    LPCONTAINER lpCnt = GETCNT(hWnd);

    // Set the focus to the container child on an NC buttondown
    // if the focus is on some other control.
    if( GetFocus() != hWnd )
    {
      SetFocus( hWnd );
      if( !StateTest( lpCnt, CNTSTATE_HASFOCUS ) )
      {
        // The container should have gotten a WM_SETFOCUS msg which sets
        // the HASFOCUS state but in some cases it is not sent. If the
        // container didn't get the WM_SETFOCUS msg we will send it now
        // to ensure that the focus rect is painted correctly.
        FORWARD_WM_SETFOCUS( hWnd, NULL, SendMessage );
      }
    }

    // If we have a 32 bit range max and the user started a vertical
    // scroll action, init the values for tracking the thumb position.
    if( StateTest( lpCnt, CNTSTATE_VSCROLL32 ) )
    {
      if( codeHitTest == HTVSCROLL )
        InitVThumbpos( hWnd, lpCnt );
    }

    // Forward the NC msg to the def proc.
    FORWARD_WM_NCLBUTTONDOWN( hWnd, fDoubleClick, x, y, codeHitTest, CNT_DEFWNDPROC );
    return;
}

VOID Txt_OnNCMButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest )
{
    LPCONTAINER lpCnt = GETCNT(hWnd);

    // Set the focus to the container child on an NC buttondown
    // if the focus is on some other control.
    if( GetFocus() != hWnd )
    {
      SetFocus( hWnd );
      if( !StateTest( lpCnt, CNTSTATE_HASFOCUS ) )
      {
        // The container should have gotten a WM_SETFOCUS msg which sets
        // the HASFOCUS state but in some cases it is not sent. If the
        // container didn't get the WM_SETFOCUS msg we will send it now
        // to ensure that the focus rect is painted correctly.
        FORWARD_WM_SETFOCUS( hWnd, NULL, SendMessage );
      }
    }

    // Send the NC msg to the def proc.
    FORWARD_WM_NCMBUTTONDOWN( hWnd, fDoubleClick, x, y, codeHitTest, CNT_DEFWNDPROC );
    return;
}

VOID Txt_OnNCRButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest )
{
    LPCONTAINER lpCnt = GETCNT(hWnd);

    // Set the focus to the container child on an NC buttondown
    // if the focus is on some other control.
    if( GetFocus() != hWnd )
    {
      SetFocus( hWnd );
      if( !StateTest( lpCnt, CNTSTATE_HASFOCUS ) )
      {
        // The container should have gotten a WM_SETFOCUS msg which sets
        // the HASFOCUS state but in some cases it is not sent. If the
        // container didn't get the WM_SETFOCUS msg we will send it now
        // to ensure that the focus rect is painted correctly.
        FORWARD_WM_SETFOCUS( hWnd, NULL, SendMessage );
      }
    }

    // Send the NC msg to the def proc.
    FORWARD_WM_NCRBUTTONDOWN( hWnd, fDoubleClick, x, y, codeHitTest, CNT_DEFWNDPROC );
    return;
}

/****************************************************************************
 * Get1stColXPos
 *
 * Purpose:
 *  Gets the horz starting and ending position of the column farthest left
 *  Also sets the new top record for flowed view
 *
 * Parameters:
 *  LPCONTAINER   lpCnt       - pointer to existing CONTAINER structure.
 *  LPINT         lpxStr      - returned starting position of the field
 *  LPINT         lpxEnd      - returned ending position of the field
 *  BOOL          bDir        - direction we are scrolling - 0 = Left, 1 = Right
 *
 * Return Value:
 *  BOOL          TRUE if function is successful; else FALSE
 ****************************************************************************/
BOOL WINAPI Get1stColXPos( LPCONTAINER lpCnt, LPINT lpxStr, LPINT lpxEnd, BOOL bDir )
{
    int xStrCol=0, xEndCol=-1;
    int nRows, ctr, nWidth = 0;
    LPFLOWCOLINFO lpCol;

    lpCol   = lpCnt->lpColHead;
    xStrCol = -lpCnt->nHscrollPos * lpCnt->lpCI->cxChar;

    nRows = (lpCnt->cyClient - lpCnt->lpCI->cyTitleArea - 1) / lpCnt->lpCI->cyRow + 1; // number of rows
    ctr   = 0;

    // get the column widths and break when find right column
    while (lpCol)
    {
      xEndCol = xStrCol + lpCol->cxPxlWidth;
        
      if (bDir == 0)
      {
        if (xEndCol >= 0)
          break;
      }
      else
      {
        if (xEndCol > 0)
          break;
      }

      xStrCol += lpCol->cxPxlWidth;
      ctr     += nRows;
    
      // Advance to the next record.
      lpCol = lpCol->lpNext;
    }
          
    // set new top record
    lpCnt->lpCI->lpTopRec = lpCol->lpTopRec; // top record in flowed view is top record in leftmost column
    
    *lpxStr = xStrCol;
    *lpxEnd = xEndCol;
    return TRUE;
}

/****************************************************************************
 * CreateFlowColStruct
 *
 * Purpose:
 *  Fill structure used for handling the columns in flowed view.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container
 *  LPCONTAINER   lpCnt       - pointer to existing CONTAINER structure.
 *
 * Return Value:
 *  BOOL          TRUE if function is successful; else FALSE
 ****************************************************************************/
BOOL CreateFlowColStruct( HWND hWnd, LPCONTAINER lpCnt )
{
  LPRECORDCORE  lpRec = CntRecHeadGet( hWnd );
  LPRECORDCORE  lpTempTopRec;
  int nRows, ctr, nWidth = 0;
  BOOL bTopRecSet = FALSE;
  int nMaxWidth=0;

  if (lpCnt->lpColHead) // reinitialize the structure
  {
    FreeFlowColStruct( lpCnt );
  }

  nRows = lpCnt->lpCI->nRecsDisplayed;
  ctr   = 1;
  if (nRows > 0)
  {
    // get the widest name in each column
    while (lpRec)
    {
      if (!bTopRecSet) // set the toprec for this column
      {
        lpTempTopRec = lpRec;
        bTopRecSet = TRUE;
      }
      if (lpCnt->iView & CV_TEXT)
      {
        // see if text string is widest so far
        if (lpRec->lpszText)
        {
          if ((lstrlen(lpRec->lpszText)) > nWidth)
            nWidth = lstrlen(lpRec->lpszText);
        }
      }
      else // Name View
      {
        // see if name string is widest so far
        if (lpRec->lpszName)
        {
          if ((lstrlen(lpRec->lpszName)) > nWidth)
            nWidth = lstrlen(lpRec->lpszName);
        }
      }
      if ((ctr % nRows == 0) && lpCnt->iView & CV_FLOW) 
      {
        // add a new column to linked list 
        SetFlowColStruct( lpCnt, lpTempTopRec, nWidth, ctr, &nMaxWidth );

        // reset vars
        bTopRecSet = FALSE;
        nWidth     = 0;
        ctr        = 0;
      }
      ctr += 1;
    
      // Advance to the next record.
      lpRec = lpRec->lpNext;
    }
    if (nWidth != 0) // last column - not a complete column so upper code didn't hit.(or not CV_FLOW)
    {
      // add a new column to linked list 
      SetFlowColStruct( lpCnt, lpTempTopRec, nWidth, ctr-1, &nMaxWidth );
    }

    // set the new maxwidth
    lpCnt->lpCI->nMaxWidth = nMaxWidth;
  }
    
  return TRUE;
}

/****************************************************************************
 * SetFlowColStruct
 *
 * Purpose:
 *  initializes a column for text and name view
 *
 * Parameters:
 *  LPCONTAINER  lpCnt      - pointer to existing CONTAINER structure.
 *  LPRECORDCORE lpTopRec   - Top record in the column
 *  int          nWidth     - width of widest item in column in characters
 *  int          nRecs      - number of records in the column
 *  LPINT        nMaxWidth  - adds on to the maxwidth of the container.
 *
 * Return Value:
 *  BOOL          TRUE if function is successful; else FALSE
 ****************************************************************************/
BOOL SetFlowColStruct( LPCONTAINER lpCnt, LPRECORDCORE lpTopRec, int nWidth, int nRecs, 
                       LPINT nMaxWidth )
{
  LPFLOWCOLINFO lpCol;
  
  lpCol = (LPFLOWCOLINFO) calloc(1, LEN_FLOWCOLINFO);
   
  // set the column widths
  lpCol->cxWidth    = nWidth;
  if (lpCnt->iView & CV_TEXT)
    lpCol->cxPxlWidth = lpCol->cxWidth * lpCnt->lpCI->cxChar;
//          lpCol->cxPxlWidth = lpCol->cxWidth * lpCnt->lpCI->cxChar + lpCnt->lpCI->cxChar2;
  else // name view
    lpCol->cxPxlWidth = (lpCol->cxWidth * lpCnt->lpCI->cxChar) +
                         lpCnt->lpCI->ptBmpOrIcon.x + lpCnt->lpCI->cxChar;

  // add flowed spacing if there is any
  if (lpCnt->lpCI->cxFlowSpacing > 0) // spacing in characters
    lpCol->cxPxlWidth += lpCnt->lpCI->cxFlowSpacing * lpCnt->lpCI->cxChar;
  else if (lpCnt->lpCI->cxFlowSpacing < 0) // spacing in pixels
    lpCol->cxPxlWidth -= lpCnt->lpCI->cxFlowSpacing;

  // in flowed view, the pxlWidth of the column must be divisible by cxChar for
  // horizontal scrolling purposes
  if (lpCol->cxPxlWidth % lpCnt->lpCI->cxChar)
    lpCol->cxPxlWidth += (lpCnt->lpCI->cxChar - (lpCol->cxPxlWidth % lpCnt->lpCI->cxChar));
  
  *nMaxWidth        += lpCol->cxPxlWidth;
  lpCol->nRecs      = nRecs;
  lpCol->lpTopRec   = lpTopRec;

  // add to linked list
  lpCnt->lpColTail = (LPFLOWCOLINFO) LLAddTail( (LPLINKNODE) lpCnt->lpColTail, 
                                                (LPLINKNODE) lpCol);

  // Update head pointer, if necessary
  if (!lpCnt->lpColHead)
    lpCnt->lpColHead = lpCnt->lpColTail;
  
  return TRUE;
}

/****************************************************************************
 * FreeFlowColStruct
 *
 * Purpose:
 *  frees the flowed column linked list 
 *
 * Parameters:
 *  LPCONTAINER  lpCnt      - pointer to existing CONTAINER structure.
 *
 * Return Value:
 *  none.
 ****************************************************************************/
VOID FreeFlowColStruct( LPCONTAINER lpCnt )
{
  LPFLOWCOLINFO lpCol, lpCol1;

  lpCol = lpCnt->lpColHead;

  while (lpCol)
  {
    lpCol1 = lpCol;
    lpCol  = lpCol->lpNext;
    free(lpCol1);
  }

  lpCnt->lpColHead = NULL;
  lpCnt->lpColTail = NULL;

  return;
}

/****************************************************************************
 * TxtGetColAtPt
 *
 * Purpose:
 *  Gets the column under the given point
 *
 * Parameters:
 *  LPCONTAINER   lpCnt       - pointer to existing CONTAINER structure.
 *  LPPOINT       lpPt        - the point of interest.
 *  LPINT         lpXstr      - return the Xcoord of the start of the field
 *  LPINT         lpXend      - return the Xcoord of the end of the field
 *
 * Return Value:
 *  LPFIELDINFO   pointer to the field
 ****************************************************************************/
LPFLOWCOLINFO WINAPI TxtGetColAtPt( LPCONTAINER lpCnt, LPPOINT lpPt, LPINT lpXstr, LPINT lpXend )
{
    LPFLOWCOLINFO lpCol;
    int           cxCol, xStrCol=0, xEndCol=0;

    xStrCol = -lpCnt->nHscrollPos * lpCnt->lpCI->cxChar;
    lpCol   = lpCnt->lpColHead;  // point at 1st column
    while( lpCol )
    {
      cxCol   = lpCol->cxPxlWidth;
      xEndCol = xStrCol + cxCol;

      // break out when we find the column
      if( lpPt->x >= xStrCol && lpPt->x <= xEndCol )
        break;  

      xStrCol += cxCol;

      lpCol = lpCol->lpNext;       // advance to next column
    }

    *lpXstr = xStrCol;
    *lpXend = xEndCol;

    return lpCol;
}

/****************************************************************************
 * TxtTrackSelection
 *
 * Purpose:
 *  Tracks record selection for Extended selection in Text view.
 *  Does all the necessary mouse tracking and screen inversion.
 *  The record and column return value signs indicate whether selection was
 *  forward (positive) or backward (negative) in the list.
 *
 * Parameters:
 *  HWND          hWnd        - Container child window handle 
 *  LPCONTAINER   lpCnt       - pointer to window's CONTAINER structure.
 *  LPRECORDCORE  lpRec       - pointer to starting record to be selected
 *  LPFLOWCOLINFO lpCol       - pointer to starting column
 *  LPPOINT       lpPt        - selection starting point
 *  LPINT         lpSelRecs   - returned number of selected records
 *  LPINT         lpSelCols   - returned number of selected columns
 *  BOOL          bTrackCols  - flag for tracking columns
 *
 * Return Value:
 *  BOOL          TRUE  if selection was tracked with no errors
 *                FALSE if selection could not be tracked
 ****************************************************************************/
BOOL NEAR TxtTrackSelection( HWND hWnd, LPCONTAINER lpCnt, LPRECORDCORE lpRec, LPFLOWCOLINFO lpCol, 
                             LPPOINT lpPt, LPINT lpSelRecs, LPINT lpSelCols, BOOL bTrackCols )
{
    LPRECORDCORE  lpTopRec;
    LPRECORDCORE  lpBotRec;
    LPFLOWCOLINFO lpCurCol=lpCol;
    LPFLOWCOLINFO lpCol2, lpLastCol;
    HDC     hDC;
    MSG     msgModal;
    HCURSOR hOldCursor;
    RECT    rectOrig, rect;
    POINT   pt, ptPrev, ptLast;
    BOOL    bTracking=TRUE;
    BOOL    bDoInvert;
    int     i, nWidth, nHeight, nLastRecBottom, nLastColBottom;
    int     nSelectedRecs=0;
    int     nSelectedCols=0;
    int     nHscrollPos, cxScrolledPxls;
#ifdef WIN32
    POINTS  pts, ptLasts;
#endif

    if( !lpRec || !lpCol )
      return FALSE;

    // Set up a constraining rect for the cursor.
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

    // Remember the starting record and column
//    lpCnt->lp1stSelRec = lpRec; // not used?
//    lpCnt->lp1stSelFld = lpFld; // not used?

    // Calc the where the bottom of the last record will be if
    // it gets scrolled into view. After every vertical scroll we will have
    // to check to see if the last record has been hit. If it has we
    // have to adjust the bottom of the record area.
    nLastRecBottom = lpCnt->lpCI->cyTitleArea;
    nLastRecBottom += (lpCnt->lpCI->nRecsDisplayed-1) * lpCnt->lpCI->cyRow;

    // Get a pointer to the current bottom record.
//    if (lpCnt->iView & CV_FLOW)
//      lpBotRec = lpCurCol->lpTopRec; // not sure if I need since this is used for vert. scrolling (flow has no vscroll)
//    else
      lpBotRec = lpCnt->lpCI->lpTopRec;
    for( i=0; i < lpCnt->lpCI->nRecsDisplayed-1 && lpBotRec; i++ )
    {
      if( lpBotRec->lpNext )
        lpBotRec = lpBotRec->lpNext;
      else
        break;
    }

    // Set the last column and the bottom of the last column - need these to check to see
    // if the mouse is in the last column in flowed view but below the last record.
    lpLastCol = lpCurCol;
    nLastColBottom = lpCnt->lpCI->cyTitleArea;
    if (lpCnt->iView & CV_FLOW)
    {
      while (lpLastCol->lpNext)
        lpLastCol = lpLastCol->lpNext;
    
      nLastColBottom += lpLastCol->nRecs * lpCnt->lpCI->cyRow;
    }
    else
      nLastColBottom = min(lpCnt->lpCI->nRecsDisplayed * lpCnt->lpCI->cyRow + lpCnt->lpCI->cyTitleArea,
                           lpCnt->rectRecArea.bottom);

    // Set the starting point for the inverted rect.
    pt.x = ptPrev.x = lpCnt->rectCursorFld.left;
    pt.y = ptPrev.y = lpCnt->rectCursorFld.top;

    // Set initial height and width of the inverted rect.
    nHeight = lpCnt->rectCursorFld.bottom - lpCnt->rectCursorFld.top;
    nWidth  = lpCnt->rectCursorFld.right  - lpCnt->rectCursorFld.left;

    // Remember the original cell.
    CopyRect( &rectOrig, &lpCnt->rectCursorFld );
    CopyRect( &rect,     &lpCnt->rectCursorFld );
    // if were in drag mode we dont have swipe extended select so
    // just figure out where we are and skip the rest
    if( FALSE /*pCnt->dwStyle & CTS_DRAGDROP || lpCnt->dwStyle & CTS_DRAGONLY */)
      {
        bTracking = FALSE;
        GetCursorPos(&pt);
        ScreenToClient(hWnd,&pt);
      }
    else
      {

        // Turn on the timer so we can generate WM_MOUSEMOVE msgs if
        // the user drags the cursor out of the record area. The generated
        // MOUSEMOVE msgs will be sent here so we can generate scroll msgs.
        SetTimer( hWnd, IDT_AUTOSCROLL, CTICKS_AUTOSCROLL, (TIMERPROC)CntTimerProc );

        hDC = GetDC( hWnd );
        SetCapture( hWnd );
        hOldCursor = SetCursor( LoadCursor( NULL, IDC_ARROW ) );
        StateSet( lpCnt, CNTSTATE_CLICKED );

        // Draw the initial inverted record.
        PatBlt( hDC, lpCnt->rectCursorFld.left,    lpCnt->rectCursorFld.top, 
                     lpCnt->rectCursorFld.right  - lpCnt->rectCursorFld.left,
                     lpCnt->rectCursorFld.bottom - lpCnt->rectCursorFld.top, DSTINVERT );
      }
    // While the cursor is being dragged get mouse msgs.
    while( bTracking )
    {
      while( !PeekMessage( &msgModal, NULL, 0, 0, PM_REMOVE ) )
        WaitMessage();

      switch( msgModal.message )
      {
        case WM_MOUSEMOVE:
          // Get new mouse position and invert new area.
#ifdef WIN32
          pts = MAKEPOINTS(msgModal.lParam); 
          pt.x = pts.x;
          pt.y = pts.y;
#else
          pt.x = LOWORD(msgModal.lParam);
          pt.y = HIWORD(msgModal.lParam);
#endif         
          
          // save the old cell position
          CopyRect( &rect, &lpCnt->rectCursorFld );

          if( PtInRect( &lpCnt->rectRecArea, pt ) )
          {
            // The cursor is in the record area so select
            // or unselect records as cursor passes over them.
            bDoInvert = FALSE;

            if( pt.y < ptPrev.y )   // the cursor is moving upward
            {
              if( (pt.y < rectOrig.top || lpCnt->rectCursorFld.left < rectOrig.left) &&
                   lpCnt->rectCursorFld.right <= rectOrig.right )
              {
                if( pt.y < lpCnt->rectCursorFld.top )
                {
                  // If the cursor is moving upward and is above the top
                  // of the original record or if it is to the left of the
                  // original record, it means we are selecting.

                  // Clean up any inversion left below the original bottom.
                  // If the cursor is above the original top it means the
                  // bottom of the inversion block should be the bottom of
                  // the original record clicked on. This situation will
                  // occur if the user does a very fast upward stroke with
                  // the mouse. We will get a MOUSEMOVE for a point below the
                  // original bottom and the next MOUSEMOVE point will be well
                  // above the bottom. Limitations of the mouse hardware.
                  if( lpCnt->rectCursorFld.bottom > rectOrig.bottom &&
                      lpCnt->rectCursorFld.left  >= rectOrig.left && 
                      lpCnt->rectCursorFld.right <= rectOrig.right)
                  {
                    rect.top    = rectOrig.bottom;
                    rect.bottom = lpCnt->rectCursorFld.bottom;
                    PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                 rect.bottom - rect.top, DSTINVERT );
                    lpCnt->rectCursorFld.bottom = rectOrig.bottom;
                  }

                  // Expand the selected record block upward.
                  rect.bottom = lpCnt->rectCursorFld.top;
                  while( pt.y < lpCnt->rectCursorFld.top )
                    lpCnt->rectCursorFld.top -= lpCnt->lpCI->cyRow;
                  rect.top = max(lpCnt->rectCursorFld.top, lpCnt->rectRecArea.top);
                  bDoInvert = TRUE;
                }
              }
              else
              {
                if( pt.y < (lpCnt->rectCursorFld.bottom - lpCnt->lpCI->cyRow) )
                {
                  // If the cursor is moving upwards and is below the bottom
                  // of the original record it means we are unselecting.

                  // Decrease the selected record block upward.
                  rect.bottom = lpCnt->rectCursorFld.bottom;
                  while( pt.y < (lpCnt->rectCursorFld.bottom - lpCnt->lpCI->cyRow) )
                    lpCnt->rectCursorFld.bottom -= lpCnt->lpCI->cyRow;
                  rect.top = lpCnt->rectCursorFld.bottom;
                  bDoInvert = TRUE;
                }
              }
            }
            else    // the cursor is moving downward
            {            
              if( (pt.y > rectOrig.bottom || lpCnt->rectCursorFld.right > rectOrig.right) &&
                   lpCnt->rectCursorFld.left >= rectOrig.left )
              {
                if( pt.y > lpCnt->rectCursorFld.bottom )
                {
                  // If the cursor is moving downward and is below the bottom
                  // of the original record or to the right of the original
                  // record, it means we are selecting.

                  // Clean up any inversion left above the original top.
                  // If the cursor is below the original bottom it means
                  // the top of the inversion block should be the top of
                  // the original record clicked on. This situation will
                  // occur if the user does a very fast downward stroke with
                  // the mouse. We will get a MOUSEMOVE for a point above
                  // the original top and the next MOUSEMOVE point will be
                  // well below the top. Limitations of the mouse hardware.
                  if( lpCnt->rectCursorFld.top < rectOrig.top  &&
                      lpCnt->rectCursorFld.left  >= rectOrig.left && 
                      lpCnt->rectCursorFld.right <= rectOrig.right)
                  {
                    rect.top    = lpCnt->rectCursorFld.top;
                    rect.bottom = rectOrig.top;
                    PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                 rect.bottom - rect.top, DSTINVERT );
                    lpCnt->rectCursorFld.top = rectOrig.top;
                  }

                  // Expand the selected record block downward.
                  rect.top    = lpCnt->rectCursorFld.bottom;
                  while( lpCnt->rectCursorFld.bottom < ((lpCurCol==lpLastCol) ? 
                         min(nLastColBottom, pt.y) : pt.y) )
                    lpCnt->rectCursorFld.bottom += lpCnt->lpCI->cyRow;
                  rect.bottom = min(lpCnt->rectCursorFld.bottom, lpCnt->rectRecArea.bottom);
                  bDoInvert = TRUE;
                }
              }
              else
              {
                if( pt.y > (lpCnt->rectCursorFld.top + lpCnt->lpCI->cyRow) )
                {
                  // If the cursor is moving downward and is above the top
                  // of the original record or to the left of it, it means we are unselecting.

                  // Decrease the selected record block downward.
                  rect.top = lpCnt->rectCursorFld.top;
                  while( pt.y > (lpCnt->rectCursorFld.top + lpCnt->lpCI->cyRow) )
                    lpCnt->rectCursorFld.top += lpCnt->lpCI->cyRow;
                  rect.bottom = lpCnt->rectCursorFld.top;
                  bDoInvert = TRUE;
                }
              }
            }

            // Do the inversion if the cursor moved to a new record.
            if( bDoInvert )
            {
//              if( bTrackCols )
//              {
//                rect.left  = min(rectOrig.left,  lpCnt->rectCursorFld.left);
//                rect.right = max(rectOrig.right, lpCnt->rectCursorFld.right);
//              }

              PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                           rect.bottom - rect.top, DSTINVERT );
            }

            // Track the horizontal movement if tracking columns.
            if( bTrackCols )
            {
              bDoInvert = FALSE;

              if( pt.x < ptPrev.x )   // the cursor is moving leftward
              {
                if( pt.x < rectOrig.left )
                {
                  if( pt.x < lpCnt->rectCursorFld.left )
                  {
                    // If the cursor is moving leftward and is to the left
                    // of the original field it means we are selecting.

                    // Clean up any inversion to the right of the original fld.
                    // If the cursor is left of the original field it means
                    // the right edge of the inversion block should be the
                    // right edge of the original field clicked on. This 
                    // situation will occur if the user does a very fast 
                    // leftward stroke with the mouse. We will get a MOUSEMOVE
                    // for a point to the right of the original right and the
                    // next MOUSEMOVE point will be well to the left of the 
                    // original right. Limitations of the mouse hardware.
                    if( lpCnt->rectCursorFld.right > rectOrig.right )
                    {
                      // first deselect from the cursor field to the top
                      rect.left   = lpCnt->rectCursorFld.left;
                      rect.right  = lpCnt->rectCursorFld.right;
                      rect.bottom = lpCnt->rectCursorFld.bottom;
                      rect.top    = lpCnt->rectRecArea.top;
                      PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                   rect.bottom - rect.top, DSTINVERT );

                      // now deselect the record back to the original record
                      while (lpCnt->rectCursorFld.right > rectOrig.right && lpCurCol->lpPrev)
                      {
                        lpCurCol = lpCurCol->lpPrev;
                        lpCnt->rectCursorFld.right = lpCnt->rectCursorFld.left;
                        lpCnt->rectCursorFld.left -= lpCurCol->cxPxlWidth;
                        if (lpCnt->rectCursorFld.right > rectOrig.right)
                        {
                          // deselect the whole column
                          rect.left   = lpCnt->rectCursorFld.left;
                          rect.right  = lpCnt->rectCursorFld.right;
                          rect.bottom = lpCnt->rectRecArea.bottom;
                          rect.top    = lpCnt->rectRecArea.top;
                          PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                       rect.bottom - rect.top, DSTINVERT );
                        }
                      }

                      // define and select working column (note this is original rect column)
                      rect.left   = lpCnt->rectCursorFld.left;
                      rect.right  = lpCnt->rectCursorFld.right;
                      if (lpCnt->rectCursorFld.bottom >= rectOrig.bottom)
                      {
                        // deselect up to bottom of cursorrect
                        rect.top    = lpCnt->rectCursorFld.bottom;
                        rect.bottom = lpCnt->rectRecArea.bottom;
                        PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                     rect.bottom - rect.top, DSTINVERT );
                        lpCnt->rectCursorFld.top = rectOrig.top;
                      }
                      else
                      {
                        // deselect up to bottom of origrect
                        rect.top    = rectOrig.bottom;
                        rect.bottom = lpCnt->rectRecArea.bottom;
                        PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                     rect.bottom - rect.top, DSTINVERT );
                        
                        // select from origrect to cursor
                        lpCnt->rectCursorFld.bottom = rectOrig.bottom;
                        lpCnt->rectCursorFld.top    = rectOrig.top;
                        while (lpCnt->rectCursorFld.top > pt.y)
                          lpCnt->rectCursorFld.top -= lpCnt->lpCI->cyRow;
                        rect.bottom = rectOrig.top;
                        PatBlt( hDC, lpCnt->rectCursorFld.left, lpCnt->rectCursorFld.top, 
                                lpCnt->rectCursorFld.right - lpCnt->rectCursorFld.left,
                                rect.bottom - lpCnt->rectCursorFld.top, DSTINVERT );
                      }
                    }
 
                    // Expand the selected field block leftward.
                    // select the first column
                    rect.left   = lpCnt->rectCursorFld.left;
                    rect.right  = lpCnt->rectCursorFld.right;
                    if (lpCnt->rectCursorFld.left == rectOrig.left)
                    {
                      if (lpCnt->rectCursorFld.bottom > rectOrig.bottom)
                      {
                        // deselect up to the bottom of the rect orig
                        rect.top    = rectOrig.bottom;
                        rect.bottom = lpCnt->rectCursorFld.bottom;
                        PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                     rect.bottom - rect.top, DSTINVERT );

                        // select from top of orig rect to top of comtainer
                        rect.top    = lpCnt->rectRecArea.top;
                        rect.bottom = rectOrig.top;
                        PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                     rect.bottom - rect.top, DSTINVERT );
                      }
                      else
                      {
                        // select from top of cursor rect to top of comtainer
                        rect.top    = lpCnt->rectRecArea.top;
                        rect.bottom = lpCnt->rectCursorFld.top;
                        PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                     rect.bottom - rect.top, DSTINVERT );
                      }
                    }
                    else
                    {
                      // select from top of cursor rect to top
                      rect.top    = lpCnt->rectRecArea.top;
                      rect.bottom = lpCnt->rectCursorFld.top;
                      PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                   rect.bottom - rect.top, DSTINVERT );
                    }

                    // continue going left until we reach the new working column
                    rect.top    = lpCnt->rectRecArea.top;
                    rect.bottom = lpCnt->rectRecArea.bottom;
                    while( pt.x < lpCnt->rectCursorFld.left && lpCurCol->lpPrev )
                    {
                      lpCurCol = lpCurCol->lpPrev;
                      lpCnt->rectCursorFld.right = lpCnt->rectCursorFld.left;
                      lpCnt->rectCursorFld.left -= lpCurCol->cxPxlWidth;
                      if (pt.x < lpCnt->rectCursorFld.left)
                      {
                        // select the whole column
                        rect.right = lpCnt->rectCursorFld.right;
                        rect.left  = lpCnt->rectCursorFld.left;
                        PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                     rect.bottom - rect.top, DSTINVERT );
                      }
                    }

                    // define the new cursor rect and select the working column 
                    // from the bottom up to the cursor rect
                    lpCnt->rectCursorFld.bottom = lpCnt->rectRecArea.bottom;
                    lpCnt->rectCursorFld.top = lpCnt->rectCursorFld.bottom - lpCnt->lpCI->cyRow;
                    while (lpCnt->rectCursorFld.top > pt.y)
                      lpCnt->rectCursorFld.top -= lpCnt->lpCI->cyRow;
                    rect.right = lpCnt->rectCursorFld.right;
                    rect.left  = lpCnt->rectCursorFld.left;
                    rect.top   = lpCnt->rectCursorFld.top;
                    PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                 rect.bottom - rect.top, DSTINVERT );

                    bDoInvert = TRUE;
                  }
                }
                else
                {
                  if( pt.x < (lpCnt->rectCursorFld.right - (int)lpCurCol->cxPxlWidth) )
                  {
                    // If the cursor is moving leftwards and is to the right
                    // of the original field it means we are unselecting.

                    // Decrease the selected field block leftward.
                    // deselect rightmost column first
                    rect.left   = lpCnt->rectCursorFld.left;
                    rect.right  = lpCnt->rectCursorFld.right;
                    rect.top    = lpCnt->rectRecArea.top;
                    rect.bottom = lpCnt->rectCursorFld.bottom;
                    PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                 rect.bottom - rect.top, DSTINVERT );

                    // deselect whole columns until we get to the new working column
                    rect.bottom = lpCnt->rectRecArea.bottom;
                    while( pt.x < lpCnt->rectCursorFld.left && lpCurCol->lpPrev )
                    {
                      lpCurCol = lpCurCol->lpPrev;
                      lpCnt->rectCursorFld.right = lpCnt->rectCursorFld.left;
                      lpCnt->rectCursorFld.left -= lpCurCol->cxPxlWidth;
                      if (pt.x < lpCnt->rectCursorFld.left)
                      {
                        // select the whole column
                        rect.right = lpCnt->rectCursorFld.right;
                        rect.left  = lpCnt->rectCursorFld.left;
                        PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                     rect.bottom - rect.top, DSTINVERT );
                      }
                    }

                    // reselect the working column
                    rect.left   = lpCnt->rectCursorFld.left;
                    rect.right  = lpCnt->rectCursorFld.right;
                    if (lpCnt->rectCursorFld.left == rectOrig.left) // in column of original rect
                    {
                      // deselect from bottom of container to bottom of rectcursor or origrect
                      rect.top = max(lpCnt->rectCursorFld.bottom, rectOrig.bottom);
                      PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                   rect.bottom - rect.top, DSTINVERT );

                      if (lpCnt->rectCursorFld.bottom < rectOrig.bottom)
                      {
                        // select from top of origrect to cursor
                        lpCnt->rectCursorFld.top    = rectOrig.top;
                        while (lpCnt->rectCursorFld.top > pt.y)
                          lpCnt->rectCursorFld.top -= lpCnt->lpCI->cyRow;
                        rect.top    = lpCnt->rectCursorFld.top;
                        rect.bottom = rectOrig.top;
                        PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                     rect.bottom - rect.top, DSTINVERT );
                        lpCnt->rectCursorFld.bottom = rectOrig.bottom;
                      }
                      else
                      {
                        lpCnt->rectCursorFld.top    = rectOrig.top;
                      }
                    }
                    else
                    {
                       rect.top    = lpCnt->rectCursorFld.bottom;
                       rect.bottom = lpCnt->rectRecArea.bottom;
                       PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                    rect.bottom - rect.top, DSTINVERT );
                    }

                    bDoInvert = TRUE;
                  }
                }
              }
              else    // the cursor is moving rightward
              {            
                if( pt.x > rectOrig.right )
                {
                  if( pt.x > lpCnt->rectCursorFld.right )
                  {
                    // If the cursor is moving rightward and is to the right
                    // of the original field it means we are selecting.

                    // Clean up any inversion to the left of the original left.
                    // If the cursor is to the right of the original field it
                    // means the left edge of the inversion should be
                    // the left edge of the original field clicked on. This
                    // situation will occur if the user does a very fast 
                    // rightward stroke with the mouse. We will get a MOUSEMOVE
                    // for a point to the left of the original field and the
                    // next MOUSEMOVE point will be well to the right of the 
                    // left edge of the original field. This is due to the
                    // limitations of the mouse hardware.
                    if( lpCnt->rectCursorFld.left < rectOrig.left )
                    {
                      // first deselect from the cursor field to the bottom
                      rect.left   = lpCnt->rectCursorFld.left;
                      rect.right  = lpCnt->rectCursorFld.right;
                      rect.top    = lpCnt->rectCursorFld.top;
                      rect.bottom = lpCnt->rectRecArea.bottom;
                      PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                   rect.bottom - rect.top, DSTINVERT );

                      // now deselect the record back to the original record
                      while (lpCnt->rectCursorFld.left < rectOrig.left && lpCurCol->lpNext)
                      {
                        lpCurCol = lpCurCol->lpNext;
                        lpCnt->rectCursorFld.left   = lpCnt->rectCursorFld.right;
                        lpCnt->rectCursorFld.right += lpCurCol->cxPxlWidth;
                        if (lpCnt->rectCursorFld.left < rectOrig.left)
                        {
                          // deselect the whole column
                          rect.left   = lpCnt->rectCursorFld.left;
                          rect.right  = lpCnt->rectCursorFld.right;
                          rect.bottom = lpCnt->rectRecArea.bottom;
                          rect.top    = lpCnt->rectRecArea.top;
                          PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                       rect.bottom - rect.top, DSTINVERT );
                        }
                      }

                      // define and select working column
                      if (lpCnt->rectCursorFld.top <= rectOrig.top)
                      {
                        // deselect down to top of rectcursor
                        rect.left   = lpCnt->rectCursorFld.left;
                        rect.right  = lpCnt->rectCursorFld.right;
                        rect.top    = lpCnt->rectRecArea.top;
                        rect.bottom = lpCnt->rectCursorFld.top;
                        PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                     rect.bottom - rect.top, DSTINVERT );
                        lpCnt->rectCursorFld.bottom = rectOrig.bottom;
                      }
                      else
                      {
                        // deselect down to top of origrect
                        rect.left   = lpCnt->rectCursorFld.left;
                        rect.right  = lpCnt->rectCursorFld.right;
                        rect.top    = lpCnt->rectRecArea.top;
                        rect.bottom = rectOrig.top;
                        PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                     rect.bottom - rect.top, DSTINVERT );
                        
                        // select from origrect to cursor
                        lpCnt->rectCursorFld.top    = rectOrig.top;
                        lpCnt->rectCursorFld.bottom = rectOrig.bottom;
                        while( lpCnt->rectCursorFld.bottom < ((lpCurCol==lpLastCol) ? 
                               min(nLastColBottom, pt.y) : pt.y) )
                          lpCnt->rectCursorFld.bottom += lpCnt->lpCI->cyRow;
                        PatBlt( hDC, lpCnt->rectCursorFld.left, lpCnt->rectCursorFld.top, 
                                lpCnt->rectCursorFld.right - lpCnt->rectCursorFld.left,
                                lpCnt->rectCursorFld.bottom - lpCnt->rectCursorFld.top, DSTINVERT );
                      }
                    }

                    // Expand the selected field block rightward.
                    // select the first column
                    rect.left   = lpCnt->rectCursorFld.left;
                    rect.right  = lpCnt->rectCursorFld.right;
                    if (lpCnt->rectCursorFld.left == rectOrig.left)
                    {
                      if (lpCnt->rectCursorFld.top < rectOrig.top)
                      {
                        // deselect down to the top of the orig rect
                        rect.top    = lpCnt->rectCursorFld.top;
                        rect.bottom = rectOrig.top;
                        PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                     rect.bottom - rect.top, DSTINVERT );

                        // select from bottom of orig rect to bottom of container
                        rect.top    = rectOrig.bottom;
                        rect.bottom = lpCnt->rectRecArea.bottom;
                        PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                     rect.bottom - rect.top, DSTINVERT );
                      }
                      else
                      {
                        // select from bottom of cursor rect to bottom of container
                        rect.top    = lpCnt->rectCursorFld.bottom;
                        rect.bottom = lpCnt->rectRecArea.bottom;
                        PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                     rect.bottom - rect.top, DSTINVERT );
                      }
                    }
                    else
                    {
                      // select from bottom of cursor rect to bottom
                      rect.top    = lpCnt->rectCursorFld.bottom;
                      rect.bottom = lpCnt->rectRecArea.bottom;
                      PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                   rect.bottom - rect.top, DSTINVERT );
                    }

                    // continue going right until we reach the new working column
                    rect.top    = lpCnt->rectRecArea.top;
                    rect.bottom = lpCnt->rectRecArea.bottom;
                    while( pt.x > lpCnt->rectCursorFld.right && lpCurCol->lpNext )
                    {
                      lpCurCol = lpCurCol->lpNext;
                      lpCnt->rectCursorFld.left   = lpCnt->rectCursorFld.right;
                      lpCnt->rectCursorFld.right += lpCurCol->cxPxlWidth;
                      if (pt.x > lpCnt->rectCursorFld.right)
                      {
                        // select the whole column
                        rect.right = lpCnt->rectCursorFld.right;
                        rect.left  = lpCnt->rectCursorFld.left;
                        PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                     rect.bottom - rect.top, DSTINVERT );
                      }
                    }

                    // define the new cursor rect and select the working column 
                    // from the top down to the cursor rect
                    lpCnt->rectCursorFld.top    = lpCnt->rectRecArea.top;
                    lpCnt->rectCursorFld.bottom = lpCnt->rectCursorFld.top + lpCnt->lpCI->cyRow;
                    while( lpCnt->rectCursorFld.bottom < ((lpCurCol==lpLastCol) ? 
                           min(nLastColBottom, pt.y) : pt.y) )
                      lpCnt->rectCursorFld.bottom += lpCnt->lpCI->cyRow;
                    rect.right  = min(lpCnt->rectCursorFld.right, lpCnt->rectRecArea.right);
                    rect.left   = lpCnt->rectCursorFld.left;
                    rect.bottom = lpCnt->rectCursorFld.bottom;
                    PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                 rect.bottom - rect.top, DSTINVERT );

                    bDoInvert = TRUE;
                  }
                }
                else
                {
                  if( pt.x > (lpCnt->rectCursorFld.left + (int)lpCurCol->cxPxlWidth) )
                  {
                    // If the cursor is moving rightward and is to the left
                    // of the original field it means we are unselecting.

                    // Decrease the selected field block rightward.
                    // deselect leftmost column first
                    rect.left   = lpCnt->rectCursorFld.left;
                    rect.right  = lpCnt->rectCursorFld.right;
                    rect.top    = lpCnt->rectCursorFld.top;
                    rect.bottom = lpCnt->rectRecArea.bottom;
                    PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                 rect.bottom - rect.top, DSTINVERT );

                    // deselect whole columns until we get to the new working column
                    rect.top = lpCnt->rectRecArea.top;
                    while( pt.x > lpCnt->rectCursorFld.right && lpCurCol->lpNext )
                    {
                      lpCurCol = lpCurCol->lpNext;
                      lpCnt->rectCursorFld.left   = lpCnt->rectCursorFld.right;
                      lpCnt->rectCursorFld.right += lpCurCol->cxPxlWidth;
                      if (pt.x > lpCnt->rectCursorFld.right)
                      {
                        // select the whole column
                        rect.right = lpCnt->rectCursorFld.right;
                        rect.left  = lpCnt->rectCursorFld.left;
                        PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                     rect.bottom - rect.top, DSTINVERT );
                      }
                    }

                    // reselect the working column
                    rect.left   = lpCnt->rectCursorFld.left;
                    rect.right  = lpCnt->rectCursorFld.right;
                    if (lpCnt->rectCursorFld.left == rectOrig.left) // in column of original rect
                    {
                      // deselect from top of container to top of rectcursor or origrect
                      rect.bottom = min(lpCnt->rectCursorFld.top, rectOrig.top);
                      PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                   rect.bottom - rect.top, DSTINVERT );

                      if (lpCnt->rectCursorFld.top > rectOrig.top)
                      {
                        // select from bottom of origrect to top of cursor
                        lpCnt->rectCursorFld.bottom    = rectOrig.bottom;
                        while( lpCnt->rectCursorFld.bottom < ((lpCurCol==lpLastCol) ? 
                               min(nLastColBottom, pt.y) : pt.y) )
                          lpCnt->rectCursorFld.bottom += lpCnt->lpCI->cyRow;
                        rect.top    = rectOrig.bottom;
                        rect.bottom = lpCnt->rectCursorFld.bottom;
                        PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                     rect.bottom - rect.top, DSTINVERT );
                        lpCnt->rectCursorFld.top = rectOrig.top;
                      }
                      else
                      {
                        lpCnt->rectCursorFld.bottom = rectOrig.bottom;
                      }
                    }
                    else
                    {
                       rect.top    = lpCnt->rectRecArea.top;
                       rect.bottom = lpCnt->rectCursorFld.top;
                       PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                    rect.bottom - rect.top, DSTINVERT );
                    }

                    bDoInvert = TRUE;
                  }
                }
              }

              // Do the inversion if the cursor moved to a new column
//              if( bDoInvert )
//              {
//                rect.top    = max(lpCnt->rectRecArea.top ,min(rectOrig.top, lpCnt->rectCursorFld.top));
//                rect.bottom = max(rectOrig.bottom, lpCnt->rectCursorFld.bottom);

//                PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
//                             rect.bottom - rect.top, DSTINVERT );
//              }
            }
          }
          else
          {
            // Cursor is out of the record area so do an auto-scroll.
            if( pt.y < lpCnt->rectRecArea.top )
            {
//              if( bTrackCols )
//              {
//                rect.left  = min(rectOrig.left,  lpCnt->rectCursorFld.left);
//                rect.right = max(rectOrig.right, lpCnt->rectCursorFld.right);
//              }

              if( rectOrig.bottom < lpCnt->rectRecArea.top )
              {
                // Clean up any non-inverted recs left below the record area 
                // top. If the cursor is at the top of the record area and the
                // original bottom is above the top of the record area then
                // the bottom of the inversion block should be the top of the
                // record area. In other words there should be no visible 
                // inverted records.
                if( lpCnt->rectCursorFld.bottom > lpCnt->rectRecArea.top )
                {
                  rect.top    = lpCnt->rectRecArea.top;
                  rect.bottom = lpCnt->rectCursorFld.bottom;
                  PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                               rect.bottom - rect.top, DSTINVERT );

                  while( lpCnt->rectCursorFld.bottom > lpCnt->rectRecArea.top )
                    lpCnt->rectCursorFld.bottom -= lpCnt->lpCI->cyRow;
                }
              }
              else
              {
                // Clean up any inversion left below the original bottom.
                // If the cursor is above the original top it means the
                // bottom of the inversion block should be the bottom of
                // the original record clicked on (if we are in flowed the
                // cursor also has to be in the column of the original rect).
                // This situation will occur if the user does a very fast upward 
                // stroke with the mouse. We will get a MOUSEMOVE for a point 
                // below the original bottom and the next MOUSEMOVE point will 
                // be well above the bottom. Limitations of the mouse hardware.
                if( lpCnt->rectCursorFld.bottom > rectOrig.bottom &&
                    lpCnt->rectCursorFld.right == rectOrig.right)
                {
                  rect.top    = rectOrig.bottom;
                  rect.bottom = lpCnt->rectCursorFld.bottom;
                  PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                               rect.bottom - rect.top, DSTINVERT );
                  lpCnt->rectCursorFld.bottom = rectOrig.bottom;
                }

                // Clean up any non-inverted records between the top of
                // the inverted rect and the record area top (only if we are
                // in the same column as the original rect. If the cursor
                // is above the record area top it means the top of the
                // inverted rect should be the record area top.
                // This situation could occur if the user does a very fast
                // upward stroke with the mouse. We will get a MOUSEMOVE for
                // a point within the record area and the next MOUSEMOVE point
                // could be above the record area. Mouse hardware limitations.
                if( lpCnt->rectCursorFld.top > lpCnt->rectRecArea.top)
                {
                  rect.top    = lpCnt->rectRecArea.top;
                  rect.bottom = lpCnt->rectCursorFld.top;
                  PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                               rect.bottom - rect.top, DSTINVERT );
                  while( lpCnt->rectCursorFld.top > lpCnt->rectRecArea.top )
                    lpCnt->rectCursorFld.top -= lpCnt->lpCI->cyRow;
                }
              
                // if we are to the right of the orig. rect and the cursor is at 
                // the top of the record area, then for this column the bottom of
                // the inversion block should be the top of the record area. In 
                // other words there should be no visible inverted records.
                if (lpCnt->rectCursorFld.right > rectOrig.right &&
                    lpCnt->rectCursorFld.bottom > lpCnt->rectRecArea.top)
                {
                  rect.top    = lpCnt->rectRecArea.top;
                  rect.bottom = lpCnt->rectCursorFld.bottom;
                  PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                               rect.bottom - rect.top, DSTINVERT );

                  while( lpCnt->rectCursorFld.bottom > lpCnt->rectRecArea.top )
                    lpCnt->rectCursorFld.bottom -= lpCnt->lpCI->cyRow;
                }
              }

              if( lpCnt->dwStyle & CTS_VERTSCROLL && !(lpCnt->iView & CV_FLOW) )
              {
                // Scroll the record area down 1 record.
                lpTopRec = lpCnt->lpCI->lpTopRec;
                FORWARD_WM_VSCROLL( hWnd, 0, SB_LINEUP, 0, SendMessage );

                // If the top record changed it means there were more records.
                if( lpTopRec != lpCnt->lpCI->lpTopRec )
                {
                  // Move the bottom record up one also (if not on last rec)
                  if( lpBotRec->lpPrev && lpCnt->rectRecArea.bottom == lpCnt->cyClient )
                    lpBotRec = lpBotRec->lpPrev;

                  // After an upward scroll the last record can never be
                  // totally exposed so the record bottom has to be the
                  // bottom of the container.
                  lpCnt->rectRecArea.bottom = lpCnt->cyClient;

                  // If the orig top is above the top record keep the inverted
                  // rect top in sync with it.
                  if( rectOrig.top < lpCnt->rectRecArea.top )
                    lpCnt->rectCursorFld.top += lpCnt->lpCI->cyRow;

                  // Move the original rect down to account for the scroll.
                  rectOrig.top    += lpCnt->lpCI->cyRow;
                  rectOrig.bottom += lpCnt->lpCI->cyRow;

                  // If the orig bottom is below the cursor we are selecting.
                  if( rectOrig.bottom > lpCnt->rectRecArea.top )
                  {
                    rect.top    = lpCnt->rectRecArea.top;
                    rect.bottom = lpCnt->rectRecArea.top + lpCnt->lpCI->cyRow;

                    PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                 rect.bottom - rect.top, DSTINVERT );

                    // Move the inverted rect bottom also.
                    lpCnt->rectCursorFld.bottom += lpCnt->lpCI->cyRow;
                  }
                }
              }
            }
            else if( pt.y > lpCnt->rectRecArea.bottom )
            {
//              if( bTrackCols )
//              {
//                rect.left  = min(rectOrig.left,  lpCnt->rectCursorFld.left);
//                rect.right = max(rectOrig.right, lpCnt->rectCursorFld.right);
//              }

              if( rectOrig.top > lpCnt->rectRecArea.bottom )
              {
                // Clean up any non-inverted recs left above the record area 
                // bottom. If the cursor is at the bottom of the record area
                // and the original top is beneath the bottom of the record area
                // then the top of the inversion block should be the bottom of
                // the record area. In other words there should be no visible 
                // inverted records.
                if( lpCnt->rectCursorFld.top < lpCnt->rectRecArea.bottom )
                {
                  rect.top    = lpCnt->rectCursorFld.top;
                  rect.bottom = lpCnt->rectRecArea.bottom;
                  PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                               rect.bottom - rect.top, DSTINVERT );

                  while( lpCnt->rectCursorFld.top < lpCnt->rectRecArea.bottom )
                    lpCnt->rectCursorFld.top += lpCnt->lpCI->cyRow;
                }
              }
              else
              {
                // Clean up any inversion left above the original top.
                // If the cursor is below the original bottom it means
                // the top of the inversion block should be the top of
                // the original record clicked on (if we are in flowed we
                // must also be in the original rect column). This situation will
                // occur if the user does a very fast downward stroke with
                // the mouse. We will get a MOUSEMOVE for a point above
                // the original top and the next MOUSEMOVE point will be
                // well below the top. Limitations of the mouse hardware.
                if( lpCnt->rectCursorFld.top < rectOrig.top &&
                    lpCnt->rectCursorFld.right == rectOrig.right)
                {
                  rect.top    = lpCnt->rectCursorFld.top;
                  rect.bottom = rectOrig.top;
                  PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                               rect.bottom - rect.top, DSTINVERT );
                  lpCnt->rectCursorFld.top = rectOrig.top;
                }

                // Clean up any non-inverted records between the bottom of
                // the inverted rect and the record area bottom. If the cursor
                // is below the record area bottom it means the bottom of
                // the inverted rect should be the record area bottom.
                // This situation could occur if the user does a very fast
                // downward stroke with the mouse. We will get a MOUSEMOVE for
                // a point within the record area and the next MOUSEMOVE point
                // could be below the record area. Mouse hardware limitations.
                if( lpCnt->rectCursorFld.bottom < lpCnt->rectRecArea.bottom)
                {
                  rect.top    = lpCnt->rectCursorFld.bottom;
                  if (lpCurCol==lpLastCol)
                    rect.bottom = nLastColBottom;
                  else
                    rect.bottom = lpCnt->rectRecArea.bottom;
                  PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                               rect.bottom - rect.top, DSTINVERT );
                  while( lpCnt->rectCursorFld.bottom < ((lpCurCol==lpLastCol) ? 
                         nLastColBottom : lpCnt->rectRecArea.bottom) )
                    lpCnt->rectCursorFld.bottom += lpCnt->lpCI->cyRow;
                }

                // if we are to the left of the orig. rect and the cursor is at 
                // the bottom of the record area, then for this column the top of
                // the inversion block should be the bottom of the record area. In 
                // other words the whole column is deselected.
                if (lpCnt->rectCursorFld.left < rectOrig.left &&
                    lpCnt->rectCursorFld.top < lpCnt->rectRecArea.bottom)
                {
                  rect.top    = lpCnt->rectCursorFld.top;
                  rect.bottom = lpCnt->rectRecArea.bottom;
                  PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                               rect.bottom - rect.top, DSTINVERT );

                  while( lpCnt->rectCursorFld.top < lpCnt->rectRecArea.bottom )
                    lpCnt->rectCursorFld.top += lpCnt->lpCI->cyRow;
                }
              }

              if( lpCnt->dwStyle & CTS_VERTSCROLL && !(lpCnt->iView & CV_FLOW) )
              {
                // Scroll the record area down 1 record.
                lpTopRec = lpCnt->lpCI->lpTopRec;
                FORWARD_WM_VSCROLL( hWnd, 0, SB_LINEDOWN, 0, SendMessage );

                // If the top record changed it means there were more records.
                if( lpTopRec != lpCnt->lpCI->lpTopRec )
                {
                  // Move the bottom record down one also.
                  if( lpBotRec->lpNext )
                  {
                    lpBotRec = lpBotRec->lpNext;
                    lpCnt->rectRecArea.bottom = lpCnt->cyClient;
                  }
                  else
                  {
                    // If we just exposed the white space beneath the last
                    // record adjust the record bottom area.
                    lpCnt->rectRecArea.bottom   = nLastRecBottom;
                    lpCnt->rectCursorFld.bottom = nLastRecBottom;
                  }

                  // Move the original rect up to account for the scroll.
                  rectOrig.top    -= lpCnt->lpCI->cyRow;
                  rectOrig.bottom -= lpCnt->lpCI->cyRow;

                  // If the orig top is above the cursor we are selecting.
                  if( rectOrig.top < lpCnt->rectRecArea.bottom )
                  {
                    rect.top    = max(rectOrig.top, lpCnt->cyClient - lpCnt->lpCI->cyRow);
                    rect.bottom = lpCnt->rectRecArea.bottom;

                    PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                 rect.bottom - rect.top, DSTINVERT );

                    // Move the inverted rect top also.
                    lpCnt->rectCursorFld.top -= lpCnt->lpCI->cyRow;
                  }
                }
              }
            }

            // Track the horizontal movement if tracking fields.
            if( bTrackCols )
            {
              if( pt.x < lpCnt->rectRecArea.left )
              {
//                rect.top    = max(lpCnt->rectRecArea.top, min(rectOrig.top, lpCnt->rectCursorFld.top));
//                rect.bottom = max(rectOrig.bottom, lpCnt->rectCursorFld.bottom);

                if( rectOrig.right < lpCnt->rectRecArea.left )
                {
                  // Clean up any non-inverted flds right of the orig field.
                  // If the cursor is at the left of the record area and the
                  // original right is still left, the right edge of the 
                  // inversion block should be the left edge of the record
                  // area. In other words there should be no visible 
                  // inverted fields unless it is a half record - we will
                  // invert this when we do a scroll.
                  if( lpCnt->rectCursorFld.left > lpCnt->rectRecArea.left )
                  {
                    // deselect working column
                    rect.top    = lpCnt->rectRecArea.top;
                    rect.bottom = lpCnt->rectCursorFld.bottom;
                    rect.left   = lpCnt->rectCursorFld.left;
                    rect.right  = min(lpCnt->rectRecArea.right, lpCnt->rectCursorFld.right);
                    PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                 rect.bottom - rect.top, DSTINVERT );

                    // deselect whole columns until we get to the left edge
                    rect.bottom = lpCnt->rectRecArea.bottom;
                    while( lpCnt->rectCursorFld.left > lpCnt->rectRecArea.left && lpCurCol->lpPrev )
                    {
                      lpCurCol = lpCurCol->lpPrev;
                      lpCnt->rectCursorFld.right = lpCnt->rectCursorFld.left;
                      lpCnt->rectCursorFld.left -= lpCurCol->cxPxlWidth;
                      if (lpCnt->rectCursorFld.left > lpCnt->rectRecArea.left)
                      {
                        rect.right = lpCnt->rectCursorFld.right;
                        rect.left  = lpCnt->rectCursorFld.left;
                        PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                     rect.bottom - rect.top, DSTINVERT );
                      }
                    }

                    // deselect to bottom of cursor rect
                    rect.left  = lpCnt->rectRecArea.left;
                    rect.right = lpCnt->rectCursorFld.right;
                    rect.top   = lpCnt->rectCursorFld.bottom;
                    PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                 rect.bottom - rect.top, DSTINVERT );
                  }
                }
                else
                {
                  // Clean up any inversion right of the original field.
                  // If the cursor is left of the original field it means the
                  // right edge of the inversion block should be the right
                  // edge of the original field clicked on. This situation will
                  // occur if the user does a very fast leftward stroke with
                  // the mouse. We will get a MOUSEMOVE for a point right or the
                  // original field and the next MOUSEMOVE point will be well
                  // to the left of the orginal field. Mouse limitations.
                  if( lpCnt->rectCursorFld.right > rectOrig.right )
                  {
                    // deselect working column
                    PatBlt( hDC, lpCnt->rectCursorFld.left, lpCnt->rectCursorFld.top, 
                            lpCnt->rectCursorFld.right - lpCnt->rectCursorFld.left,
                            lpCnt->rectCursorFld.bottom - lpCnt->rectCursorFld.top, DSTINVERT );

                    // deselect whole columns until we reach the original column
                    rect.top    = lpCnt->rectRecArea.top;
                    rect.bottom = lpCnt->rectRecArea.bottom;
                    while (lpCnt->rectCursorFld.right > rectOrig.right && lpCurCol->lpPrev)
                    {
                      lpCurCol = lpCurCol->lpPrev;
                      lpCnt->rectCursorFld.right = lpCnt->rectCursorFld.left;
                      lpCnt->rectCursorFld.left -= lpCurCol->cxPxlWidth;
                      if (lpCnt->rectCursorFld.right > rectOrig.right)
                      {
                        rect.right = lpCnt->rectCursorFld.right;
                        rect.left  = lpCnt->rectCursorFld.left;
                        PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                     rect.bottom - rect.top, DSTINVERT );
                      }
                    }

                    // deselect from bottom of container to the bottom of original rect
                    rect.top    = rectOrig.bottom;
                    rect.bottom = lpCnt->rectRecArea.bottom;
                    rect.right  = lpCnt->rectCursorFld.right;
                    rect.left   = lpCnt->rectCursorFld.left;
                    PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                 rect.bottom - rect.top, DSTINVERT );
                    lpCnt->rectCursorFld.top    = rectOrig.top;
                    lpCnt->rectCursorFld.bottom = rectOrig.bottom;
                  }

                  // Clean up any non-inverted fields between the left edge of
                  // the inverted rect and the record area left. If the cursor
                  // is left or the record area it means the left edge of the
                  // inverted rect should be the record area left.
                  // This situation could occur if the user does a very fast
                  // leftward stroke with the mouse. We will get a MOUSEMOVE for
                  // a point within the record area and the next MOUSEMOVE point
                  // could be left of the record area. Mouse hardware limitations.
                  if( lpCnt->rectCursorFld.left > lpCnt->rectRecArea.left )
                  {
                    rect.left   = lpCnt->rectCursorFld.left;
                    rect.right  = lpCnt->rectCursorFld.right;

                    // select old working column to the top
                    if (lpCnt->rectCursorFld.right == rectOrig.right)
                    {
                      if (lpCnt->rectCursorFld.bottom > rectOrig.bottom)
                      {
                        // deselect up to bottom of original rect
                        rect.top    = rectOrig.bottom;
                        rect.bottom = lpCnt->rectCursorFld.bottom;
                        PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                     rect.bottom - rect.top, DSTINVERT );
                      }
  
                      // select up to the top
                      rect.top    = lpCnt->rectRecArea.top;
                      rect.bottom = min(rectOrig.top, lpCnt->rectCursorFld.top);
                    }
                    else
                    {
                      rect.top    = lpCnt->rectRecArea.top;
                      rect.bottom = lpCnt->rectCursorFld.top;
                    }
                    PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                 rect.bottom - rect.top, DSTINVERT );

                    // select whole columns until we get to the far left column
                    rect.bottom = lpCnt->rectRecArea.bottom;
                    while( lpCnt->rectCursorFld.left > lpCnt->rectRecArea.left && lpCurCol->lpPrev )
                    {
                      lpCurCol = lpCurCol->lpPrev;
                      lpCnt->rectCursorFld.right = lpCnt->rectCursorFld.left;
                      lpCnt->rectCursorFld.left -= lpCurCol->cxPxlWidth;
                      if (lpCnt->rectCursorFld.left > lpCnt->rectRecArea.left)
                      {
                        rect.right = lpCnt->rectCursorFld.right;
                        rect.left  = lpCnt->rectCursorFld.left;
                        PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                     rect.bottom - rect.top, DSTINVERT );
                      }
                    }

                    // Set it here to avoid going negative. This should be 0.
//                    lpCnt->rectCursorFld.left = lpCnt->rectRecArea.left;

                    // select working column
                    rect.left = max(lpCnt->rectCursorFld.left, lpCnt->rectRecArea.left);
                    lpCnt->rectCursorFld.bottom = lpCnt->rectRecArea.bottom;
                    lpCnt->rectCursorFld.top    = lpCnt->rectCursorFld.bottom - lpCnt->lpCI->cyRow;
                    while (lpCnt->rectCursorFld.top > max(pt.y, lpCnt->rectRecArea.top + 1))
                      lpCnt->rectCursorFld.top -= lpCnt->lpCI->cyRow;
                    PatBlt( hDC, rect.left, lpCnt->rectCursorFld.top, 
                            lpCnt->rectCursorFld.right - rect.left,
                            lpCnt->rectCursorFld.bottom - lpCnt->rectCursorFld.top, DSTINVERT );
                  }
                }

                if( lpCnt->dwStyle & CTS_HORZSCROLL )
                {
                  // Scroll the record area left by 1 field.
                  nHscrollPos = lpCnt->nHscrollPos;
                  FORWARD_WM_HSCROLL( hWnd, 0, SB_LINEUP, 0, SendMessage );

                  // If the horz pos changed it means there were more fields.
                  if( nHscrollPos != lpCnt->nHscrollPos )
                  {
                    // Get how many pixels were actually scrolled.
                    cxScrolledPxls = (nHscrollPos - lpCnt->nHscrollPos) * lpCnt->lpCI->cxChar;

                    // save old first col to see if new fields were exposed
                    lpCol2 = lpCurCol;
                    
                    // Get the new 1st column after the scroll.
                    lpCurCol = Get1stColInView( hWnd, lpCnt );

                    // After an leftward scroll the last field can never be
                    // totally exposed so the record area right has to be the
                    // client width of the container.
                    lpCnt->rectRecArea.right = lpCnt->cxClient;

                    // Move the original rect right to account for the scroll.
                    rectOrig.left  += cxScrolledPxls;
                    rectOrig.right += cxScrolledPxls;

                    // Move the working rect right to account for the scroll
                    lpCnt->rectCursorFld.left  += cxScrolledPxls;
                    lpCnt->rectCursorFld.right += cxScrolledPxls;
                    
                    // expand the working rect leftward
                    rect.left   = max(lpCnt->rectCursorFld.left, lpCnt->rectRecArea.left);
                    rect.right  = lpCnt->rectRecArea.left + cxScrolledPxls;
                    rect.top    = lpCnt->rectCursorFld.top;
                    rect.bottom = lpCnt->rectCursorFld.bottom;
                    PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                 rect.bottom - rect.top, DSTINVERT );
                    
                    // if new column scrolled into view
                    if (lpCol2 != lpCurCol)
                    {
                      if (rectOrig.right < lpCnt->rectRecArea.left)
                      {
                        // original rect still to the right of the container, so deselect
                        PatBlt( hDC, lpCnt->rectCursorFld.left, lpCnt->rectCursorFld.top, 
                                lpCnt->rectCursorFld.right - lpCnt->rectCursorFld.left,
                                lpCnt->rectCursorFld.bottom - lpCnt->rectCursorFld.top, DSTINVERT );

                        // define and select new column
                        lpCnt->rectCursorFld.right = lpCnt->rectCursorFld.left;
                        lpCnt->rectCursorFld.left -= lpCurCol->cxPxlWidth;
                        rect.left = max(lpCnt->rectCursorFld.left, lpCnt->rectRecArea.left);
                        PatBlt( hDC, rect.left, lpCnt->rectCursorFld.top, 
                                lpCnt->rectCursorFld.right - rect.left,
                                lpCnt->rectCursorFld.bottom - lpCnt->rectCursorFld.top, DSTINVERT );
                      }
                      else
                      {
                        if (lpCnt->rectCursorFld.right > rectOrig.right)
                        {
                          // original rect just scrolled into view
                          // deselect old working column
                          PatBlt( hDC, lpCnt->rectCursorFld.left, lpCnt->rectCursorFld.top, 
                                  lpCnt->rectCursorFld.right - lpCnt->rectCursorFld.left,
                                  lpCnt->rectCursorFld.bottom - lpCnt->rectCursorFld.top, DSTINVERT );

                          // select new working column (note that this is the original rect column)
                          lpCnt->rectCursorFld.right = lpCnt->rectCursorFld.left;
                          lpCnt->rectCursorFld.left -= lpCurCol->cxPxlWidth;
                          if (lpCnt->rectCursorFld.bottom > rectOrig.bottom)
                            lpCnt->rectCursorFld.top = rectOrig.top;
                          else
                          {
                            lpCnt->rectCursorFld.top    = rectOrig.top;
                            lpCnt->rectCursorFld.bottom = rectOrig.bottom;
                            while (lpCnt->rectCursorFld.top > max(pt.y, lpCnt->rectRecArea.top + 1))
                              lpCnt->rectCursorFld.top -= lpCnt->lpCI->cyRow;
                          }
                          rect.left = max(lpCnt->rectCursorFld.left, lpCnt->rectRecArea.left);
                          PatBlt( hDC, rect.left, lpCnt->rectCursorFld.top, 
                                  lpCnt->rectCursorFld.right - rect.left,
                                  lpCnt->rectCursorFld.bottom - lpCnt->rectCursorFld.top, DSTINVERT );
                        }
                        else // we are selecting
                        {
                          if (lpCnt->rectCursorFld.right == rectOrig.right &&
                              lpCnt->rectCursorFld.bottom > rectOrig.bottom)
                          {
                            // old working column was the original rect so deselect
                            // if any selection below the original rect
                            rect.top = rectOrig.bottom;
                            PatBlt( hDC, lpCnt->rectCursorFld.left, rect.top, 
                                    lpCnt->rectCursorFld.right - lpCnt->rectCursorFld.left,
                                    lpCnt->rectCursorFld.bottom - rect.top, DSTINVERT );
                            lpCnt->rectCursorFld.top = rectOrig.top;
                          }

                          // select up to the top
                          rect.top    = lpCnt->rectRecArea.top;
                          rect.bottom = lpCnt->rectCursorFld.top;
                          PatBlt( hDC, lpCnt->rectCursorFld.left, rect.top, 
                                  lpCnt->rectCursorFld.right - lpCnt->rectCursorFld.left,
                                  rect.bottom - rect.top, DSTINVERT );\
                          
                          // define and select new working column
                          lpCnt->rectCursorFld.right  = lpCnt->rectCursorFld.left;
                          lpCnt->rectCursorFld.left  -= lpCurCol->cxPxlWidth;
                          lpCnt->rectCursorFld.bottom = lpCnt->rectRecArea.bottom;
                          lpCnt->rectCursorFld.top    = lpCnt->rectRecArea.bottom - lpCnt->lpCI->cyRow;
                          while (lpCnt->rectCursorFld.top > max(pt.y, lpCnt->rectRecArea.top + 1))
                            lpCnt->rectCursorFld.top -= lpCnt->lpCI->cyRow;
                          PatBlt( hDC, lpCnt->rectCursorFld.left, lpCnt->rectCursorFld.top, 
                                  lpCnt->rectCursorFld.right - lpCnt->rectCursorFld.left,
                                  lpCnt->rectCursorFld.bottom - lpCnt->rectCursorFld.top, DSTINVERT );
                        }
                      }
                    }
                  }
                }
              }
              else if( pt.x > lpCnt->rectRecArea.right )
              {
//                rect.top    = max(lpCnt->rectRecArea.top, min(rectOrig.top, lpCnt->rectCursorFld.top));
//                rect.bottom = max(rectOrig.bottom, lpCnt->rectCursorFld.bottom);

                if( rectOrig.left > lpCnt->rectRecArea.right )
                {
                  // Clean up any non-inverted recs left of the the record area 
                  // right edge. If the cursor is at the right edge of the 
                  // record area and the orig field is still to the right then
                  // the left edge of the inversion block should be the right
                  // edge of the record area. In other words there should be
                  // no visible inverted fields unless it is a half record - we
                  // will invert this if we do a scroll.  This code should only
                  // hit if a very fast mousemove has occurred.
                  if( lpCnt->rectCursorFld.right < lpCnt->rectRecArea.right )
                  {
                    // deselect working column
                    rect.top    = lpCnt->rectCursorFld.top;
                    rect.bottom = lpCnt->rectRecArea.bottom;
                    rect.left   = max(lpCnt->rectRecArea.left,  lpCnt->rectCursorFld.left);
                    rect.right  = lpCnt->rectCursorFld.right;
                    PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                 rect.bottom - rect.top, DSTINVERT );

                    // deselect whole columns until we get to the right edge
                    rect.top = lpCnt->rectRecArea.top;
                    while( lpCnt->rectCursorFld.right < lpCnt->rectRecArea.right && lpCurCol->lpNext )
                    {
                      lpCurCol = lpCurCol->lpNext;
                      lpCnt->rectCursorFld.left = lpCnt->rectCursorFld.right;
                      lpCnt->rectCursorFld.right += lpCurCol->cxPxlWidth;
                      if (lpCnt->rectCursorFld.right < lpCnt->rectRecArea.right)
                      {
                        rect.right = lpCnt->rectCursorFld.right;
                        rect.left  = lpCnt->rectCursorFld.left;
                        PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                     rect.bottom - rect.top, DSTINVERT );
                      }
                    }

                    // deselect to top of cursor rect
                    rect.left   = lpCnt->rectCursorFld.left;
                    rect.right  = lpCnt->rectRecArea.right;
                    rect.bottom = lpCnt->rectCursorFld.top;
                    PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                 rect.bottom - rect.top, DSTINVERT );
                  }
                }
                else
                {
                  // Clean up any inversion to the left of the orig field.
                  // If the cursor is right of the orig field it means
                  // the left edge of the inversion block should be the left
                  // edge of the orig field clicked on. This situation will
                  // occur if the user does a very fast rightward stroke with
                  // the mouse. We will get a MOUSEMOVE for a point left
                  // the original field and the next MOUSEMOVE point will be
                  // well right of the field. Limitations of the mouse hardware.
                  if( lpCnt->rectCursorFld.left < rectOrig.left )
                  {
                    // deselect working column
                    PatBlt( hDC, lpCnt->rectCursorFld.left, lpCnt->rectCursorFld.top, 
                            lpCnt->rectCursorFld.right - lpCnt->rectCursorFld.left,
                            lpCnt->rectCursorFld.bottom - lpCnt->rectCursorFld.top, DSTINVERT );

                    // deselect whole columns until we reach the original column
                    rect.top    = lpCnt->rectRecArea.top;
                    rect.bottom = lpCnt->rectRecArea.bottom;
                    while (lpCnt->rectCursorFld.left < rectOrig.left && lpCurCol->lpNext)
                    {
                      lpCurCol = lpCurCol->lpNext;
                      lpCnt->rectCursorFld.left   = lpCnt->rectCursorFld.right;
                      lpCnt->rectCursorFld.right += lpCurCol->cxPxlWidth;
                      if (lpCnt->rectCursorFld.left < rectOrig.left)
                      {
                        rect.right = lpCnt->rectCursorFld.right;
                        rect.left  = lpCnt->rectCursorFld.left;
                        PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                     rect.bottom - rect.top, DSTINVERT );
                      }
                    }

                    // deselect from top of container to the top of original rect
                    rect.top    = lpCnt->rectRecArea.top;
                    rect.bottom = rectOrig.top;
                    rect.right  = lpCnt->rectCursorFld.right;
                    rect.left   = lpCnt->rectCursorFld.left;
                    PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                 rect.bottom - rect.top, DSTINVERT );
                    lpCnt->rectCursorFld.top    = rectOrig.top;
                    lpCnt->rectCursorFld.bottom = rectOrig.bottom;
                  }

                  // Clean up any non-inverted fields between the right edge of
                  // the inverted rect and the record area right edge. If the
                  // cursor is right of the record area it means the right edge
                  // of the inverted rect should be the record area right edge.
                  // This situation could occur if the user does a very fast
                  // rightward stroke with the mouse. We will get a MOUSEMOVE for
                  // a point within the record area and the next MOUSEMOVE point
                  // could be right of the record area. Mouse hardware limitations.
                  if( lpCnt->rectCursorFld.right < lpCnt->rectRecArea.right )
                  {
                    rect.left   = lpCnt->rectCursorFld.left;
                    rect.right  = lpCnt->rectCursorFld.right;

                    // select old working column to the bottom
                    if (lpCnt->rectCursorFld.right == rectOrig.right)
                    {
                      if (lpCnt->rectCursorFld.top < rectOrig.top)
                      {
                        // deselect down to top of original rect
                        rect.top    = lpCnt->rectCursorFld.top;
                        rect.bottom = rectOrig.top;
                        PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                     rect.bottom - rect.top, DSTINVERT );
                      }
  
                      // select down to the bottom
                      rect.top    = max(rectOrig.bottom, lpCnt->rectCursorFld.bottom);
                      rect.bottom = lpCnt->rectRecArea.bottom;
                    }
                    else
                    {
                      rect.top    = lpCnt->rectCursorFld.bottom;
                      rect.bottom = lpCnt->rectRecArea.bottom;
                    }
                    PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                 rect.bottom - rect.top, DSTINVERT );

                    // select whole columns until we get to the far right column
                    rect.top = lpCnt->rectRecArea.top;
                    while( lpCnt->rectCursorFld.right < lpCnt->rectRecArea.right && lpCurCol->lpNext )
                    {
                      lpCurCol = lpCurCol->lpNext;
                      lpCnt->rectCursorFld.left   = lpCnt->rectCursorFld.right;
                      lpCnt->rectCursorFld.right += lpCurCol->cxPxlWidth;
                      if (lpCnt->rectCursorFld.right < lpCnt->rectRecArea.right)
                      {
                        rect.right = lpCnt->rectCursorFld.right;
                        rect.left  = lpCnt->rectCursorFld.left;
                        PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                     rect.bottom - rect.top, DSTINVERT );
                      }
                    }

                    // select working column
                    {
                      int nBottom;

                      lpCnt->rectCursorFld.top    = lpCnt->rectRecArea.top;
                      lpCnt->rectCursorFld.bottom = lpCnt->rectRecArea.top + lpCnt->lpCI->cyRow;
                      nBottom = (lpCurCol==lpLastCol) ? min(nLastColBottom, pt.y) :
                                                        min(lpCnt->rectRecArea.bottom-1, pt.y);
                      while (lpCnt->rectCursorFld.bottom < nBottom)
                        lpCnt->rectCursorFld.bottom += lpCnt->lpCI->cyRow;
                      rect.right = min(lpCnt->rectCursorFld.right, lpCnt->rectRecArea.right);
                      PatBlt( hDC, lpCnt->rectCursorFld.left, lpCnt->rectCursorFld.top, 
                              rect.right - lpCnt->rectCursorFld.left,
                              lpCnt->rectCursorFld.bottom - lpCnt->rectCursorFld.top, DSTINVERT );
                    }
                  }
                }
                if( lpCnt->dwStyle & CTS_HORZSCROLL )
                {
                  // Scroll the record area right 1 field.
                  nHscrollPos = lpCnt->nHscrollPos;
                  FORWARD_WM_HSCROLL( hWnd, 0, SB_LINEDOWN, 0, SendMessage );

                  // If the field pos changed it means there were more fields.
                  if( nHscrollPos != lpCnt->nHscrollPos )
                  {
                    int oldRight;

                    // Get how many pixels were actually scrolled.
                    cxScrolledPxls = (lpCnt->nHscrollPos - nHscrollPos) * lpCnt->lpCI->cxChar;

                    lpCol2   = lpCurCol; // save old last col to see if new fields were exposed

                    // Get the new last field after the scroll.
                    lpCurCol = GetLastColInView( hWnd, lpCnt );

                    // the right of inverted rect
                    oldRight = lpCnt->rectRecArea.right - cxScrolledPxls; 

                    lpCnt->rectRecArea.right = min(lpCnt->xLastCol, lpCnt->cxClient);

                    // Move the original rect left to account for the scroll.
                    rectOrig.left  -= cxScrolledPxls;
                    rectOrig.right -= cxScrolledPxls;

                    // Move the working rect left to account for the scroll.
                    lpCnt->rectCursorFld.left  -= cxScrolledPxls;
                    lpCnt->rectCursorFld.right -= cxScrolledPxls;

                    // expand working rect rightward
                    rect.left   = oldRight;
                    rect.right  = min(lpCnt->rectCursorFld.right, lpCnt->rectRecArea.right);
                    rect.top    = lpCnt->rectCursorFld.top;
                    rect.bottom = lpCnt->rectCursorFld.bottom;
                    PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                 rect.bottom - rect.top, DSTINVERT );
                    
                    // if new column scrolled into view
                    if (lpCol2 != lpCurCol)
                    {
                      // set this for the later check of how many new columns scrolled into 
                      // view.  If just one new column, this should now equal lpCurCol
                      lpCol2 = lpCol2->lpNext;

                      if (rectOrig.left > lpCnt->rectRecArea.right)
                      {
                        // original rect still to the left of the container so deselect
                        PatBlt( hDC, lpCnt->rectCursorFld.left, lpCnt->rectCursorFld.top, 
                                     lpCnt->rectCursorFld.right - lpCnt->rectCursorFld.left,
                                     lpCnt->rectCursorFld.bottom - lpCnt->rectCursorFld.top, DSTINVERT );

                        // increase rectCursorFld by appropriate widths until we reach
                        // the new working column
                        while (lpCol2 != lpCurCol)
                        {
                          lpCnt->rectCursorFld.left   = lpCnt->rectCursorFld.right;
                          lpCnt->rectCursorFld.right += lpCol2->cxPxlWidth;
                          lpCol2 = lpCol2->lpNext;
                        }

                        // define and select new column
                        lpCnt->rectCursorFld.left   = lpCnt->rectCursorFld.right;
                        lpCnt->rectCursorFld.right += lpCurCol->cxPxlWidth;
                        rect.right = min(lpCnt->rectCursorFld.right, lpCnt->rectRecArea.right);
                        PatBlt( hDC, lpCnt->rectCursorFld.left, lpCnt->rectCursorFld.top, 
                                     rect.right - lpCnt->rectCursorFld.left,
                                     lpCnt->rectCursorFld.bottom - lpCnt->rectCursorFld.top, DSTINVERT );
                      }
                      else
                      {
                        if (lpCnt->rectCursorFld.left < rectOrig.left) 
                        {
                          // original rect just scrolled into view
                          BOOL bOrigSelected = FALSE;
                          int nBottom;

                          // deselect old column
                          PatBlt( hDC, lpCnt->rectCursorFld.left, lpCnt->rectCursorFld.top, 
                                       lpCnt->rectCursorFld.right - lpCnt->rectCursorFld.left,
                                       lpCnt->rectCursorFld.bottom - lpCnt->rectCursorFld.top, DSTINVERT );

                          if (lpCol2 != lpCurCol) // more than one column scrolled into view
                          {
                            // select columns until we get to lpCurCol
                            while (lpCol2 != lpCurCol)
                            {
                              lpCnt->rectCursorFld.left   = lpCnt->rectCursorFld.right;
                              lpCnt->rectCursorFld.right += lpCol2->cxPxlWidth;
                              if (lpCnt->rectCursorFld.left == rectOrig.left)
                              {
                                // select from top of origrect to bottom
                                rect.top    = rectOrig.top;
                                rect.bottom = lpCnt->rectRecArea.bottom;
                                PatBlt( hDC, lpCnt->rectCursorFld.left, rect.top, 
                                             lpCnt->rectCursorFld.right - lpCnt->rectCursorFld.left,
                                             rect.bottom - rect.top, DSTINVERT );
                                bOrigSelected = TRUE;
                              }
                              else if (lpCnt->rectCursorFld.left > rectOrig.left)
                              {
                                // select the column
                                rect.top    = lpCnt->rectRecArea.top;
                                rect.bottom = lpCnt->rectRecArea.bottom;
                                PatBlt( hDC, lpCnt->rectCursorFld.left, rect.top, 
                                             lpCnt->rectCursorFld.right - lpCnt->rectCursorFld.left,
                                             rect.bottom - rect.top, DSTINVERT );
                              }
                              lpCol2 = lpCol2->lpNext;
                            }
                          }

                          // select working column 
                          lpCnt->rectCursorFld.left   = lpCnt->rectCursorFld.right;
                          lpCnt->rectCursorFld.right += lpCurCol->cxPxlWidth;
                          if (bOrigSelected)
                          {
                            // original rect already passed by and has been selected
                            lpCnt->rectCursorFld.top    = lpCnt->rectRecArea.top;
                            lpCnt->rectCursorFld.bottom = lpCnt->rectRecArea.top + lpCnt->lpCI->cyRow;
                            nBottom = (lpCurCol==lpLastCol) ? min(nLastColBottom, pt.y) :
                                                              min(lpCnt->rectRecArea.bottom -1, pt.y);
                            while (lpCnt->rectCursorFld.bottom < nBottom)
                              lpCnt->rectCursorFld.bottom += lpCnt->lpCI->cyRow;
                          }
                          else
                          {
                            // select new working column (note this is the column of the original rect
                            if (lpCnt->rectCursorFld.top < rectOrig.top)
                              lpCnt->rectCursorFld.bottom = rectOrig.bottom;
                            else
                            {
                              lpCnt->rectCursorFld.top = rectOrig.top;
                              lpCnt->rectCursorFld.bottom = rectOrig.bottom;
                              nBottom = (lpCurCol==lpLastCol) ? min(nLastColBottom, pt.y) :
                                                                min(lpCnt->rectRecArea.bottom -1, pt.y);
                              while (lpCnt->rectCursorFld.bottom < nBottom)
                                lpCnt->rectCursorFld.bottom += lpCnt->lpCI->cyRow;
                            }
                          }
                          rect.right = min(lpCnt->rectCursorFld.right, lpCnt->rectRecArea.right);
                          PatBlt( hDC, lpCnt->rectCursorFld.left, lpCnt->rectCursorFld.top, 
                                       rect.right - lpCnt->rectCursorFld.left,
                                       lpCnt->rectCursorFld.bottom - lpCnt->rectCursorFld.top, DSTINVERT );
                        }
                        else // we are selecting
                        {
                          if (lpCnt->rectCursorFld.left == rectOrig.left && 
                               lpCnt->rectCursorFld.top < rectOrig.top)
                          {
                            // old working column was the original rect column so deselect
                            // if any selection above the original rect
                            rect.top    = lpCnt->rectCursorFld.top;
                            rect.bottom = rectOrig.top;
                            PatBlt( hDC, lpCnt->rectCursorFld.left, rect.top, 
                                         lpCnt->rectCursorFld.right - lpCnt->rectCursorFld.left,
                                         rect.bottom - rect.top, DSTINVERT );
                            lpCnt->rectCursorFld.bottom = rectOrig.bottom;
                          }

                          // select down to the bottom
                          rect.top    = lpCnt->rectCursorFld.bottom;
                          rect.bottom = lpCnt->rectRecArea.bottom;
                          rect.left   = lpCnt->rectCursorFld.left;
                          rect.right  = lpCnt->rectCursorFld.right;
                          PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                       rect.bottom - rect.top, DSTINVERT );

                          // select whole columns until we reach the working rect
                          while (lpCol2 != lpCurCol)
                          {
                            lpCnt->rectCursorFld.left   = lpCnt->rectCursorFld.right;
                            lpCnt->rectCursorFld.right += lpCol2->cxPxlWidth;
                            rect.top    = lpCnt->rectRecArea.top;
                            rect.bottom = lpCnt->rectRecArea.bottom;
                            PatBlt( hDC, lpCnt->rectCursorFld.left, rect.top, 
                                         lpCnt->rectCursorFld.right - lpCnt->rectCursorFld.left,
                                         rect.bottom - rect.top, DSTINVERT );
                            lpCol2 = lpCol2->lpNext;
                          }
                          
                          // define and select new working column
                          {
                            int nBottom;

                            lpCnt->rectCursorFld.left   = lpCnt->rectCursorFld.right;
                            lpCnt->rectCursorFld.right += lpCurCol->cxPxlWidth;
                            lpCnt->rectCursorFld.top    = lpCnt->rectRecArea.top;
                            lpCnt->rectCursorFld.bottom = lpCnt->rectRecArea.top + lpCnt->lpCI->cyRow;
                            nBottom = (lpCurCol==lpLastCol) ? min(nLastColBottom, pt.y) :
                                                              min(lpCnt->rectRecArea.bottom -1, pt.y);
                            while (lpCnt->rectCursorFld.bottom < nBottom)
                              lpCnt->rectCursorFld.bottom += lpCnt->lpCI->cyRow;
                            PatBlt( hDC, lpCnt->rectCursorFld.left, lpCnt->rectCursorFld.top, 
                                         lpCnt->rectCursorFld.right - lpCnt->rectCursorFld.left,
                                         lpCnt->rectCursorFld.bottom - lpCnt->rectCursorFld.top, DSTINVERT );
                          }
                        }
                      }
                    }
                  }
                } // end if horizontal scroll
              }
            } // end if track columns
          } // end if cursor out of record area

          // Save the new cursor position.
          ptPrev.x = pt.x;
          ptPrev.y = pt.y;

          // Don't dispatch this msg since we just processed it.
          continue;
          break;

        case WM_LBUTTONUP:              // End of selection drag          
          // Get the final position of the cursor.
#ifdef WIN32
          ptLasts  = MAKEPOINTS(msgModal.lParam); 
          ptLast.x = ptLasts.x;
          ptLast.y = ptLasts.y;
#else
          ptLast.x = LOWORD(msgModal.lParam);
          ptLast.y = HIWORD(msgModal.lParam);
#endif

          // If the user released the cursor outside the
          // record area we have to adjust the last point.
          if(!PtInRect( &lpCnt->rectRecArea, ptLast ))
          {
            if( ptLast.y < lpCnt->rectRecArea.top)
            {
              ptLast.y = lpCnt->rectRecArea.top + 1;
              if( lpCnt->rectCursorFld.left < rectOrig.left )
              {
                ptLast.y = lpCnt->rectRecArea.top + 1;
                ptLast.x = lpCnt->rectCursorFld.left+1;
              }
              else if( lpCnt->rectCursorFld.right > rectOrig.right )
              {
                lpCurCol = lpCurCol->lpPrev;
                ptLast.y = lpCnt->rectRecArea.bottom - 1;
                ptLast.x = lpCnt->rectCursorFld.left - 1;
              }
              else
              {
                ptLast.y = lpCnt->rectRecArea.top + 1;
                ptLast.x = lpCnt->rectCursorFld.left+1;
              }
            }
            else if ( ptLast.y > lpCnt->rectRecArea.bottom )
            {
              if( lpCnt->rectCursorFld.left < rectOrig.left )
              {
                lpCurCol = lpCurCol->lpNext;
                ptLast.y = lpCnt->rectRecArea.top + 1;
                ptLast.x = lpCnt->rectCursorFld.right+1;
              }
              else if( lpCnt->rectCursorFld.right > rectOrig.right )
              {
                ptLast.y = lpCnt->rectRecArea.bottom - 1;
                ptLast.x = lpCnt->rectCursorFld.right-1;
              }
              else
              {
                ptLast.y = lpCnt->rectRecArea.bottom - 1;
                ptLast.x = lpCnt->rectCursorFld.left+1;
              }
            }
            if( ptLast.x < lpCnt->rectRecArea.left || ptLast.x > lpCnt->rectRecArea.right )
            {
              if ( ptLast.x < lpCnt->rectRecArea.left )
                ptLast.x = lpCnt->rectCursorFld.left + 1;
              if ( ptLast.x > lpCnt->rectRecArea.right )
                ptLast.x = lpCnt->rectCursorFld.right - 1;

              if( lpCnt->rectCursorFld.left < rectOrig.left )
                ptLast.y = lpCnt->rectCursorFld.top+1;
              else if( lpCnt->rectCursorFld.right > rectOrig.right )
                ptLast.y = lpCnt->rectCursorFld.bottom-1;
              else if( lpCnt->rectCursorFld.top < rectOrig.top ) // in original rect column
                ptLast.y = lpCnt->rectCursorFld.top+1;
              else if( lpCnt->rectCursorFld.bottom > rectOrig.bottom ) // in original rect column
                ptLast.y = lpCnt->rectCursorFld.bottom-1;
              else
                ptLast.y = lpCnt->rectCursorFld.top+1;
            }
          }

          pt.x = ptLast.x;
          pt.y = ptLast.y;

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

    // Calc how many records the user has selected. A positive value
    // means the user selected forward in the record list while a 
    // negative value means they selected backward.
    if( pt.y > rectOrig.top )
      nSelectedRecs = (pt.y - rectOrig.top) / lpCnt->lpCI->cyRow;
    else
      nSelectedRecs = (pt.y - rectOrig.bottom) / lpCnt->lpCI->cyRow;

    // Calc how many fields the user has selected. A positive value
    // means the user selected forward in the field list while a 
    // negative value means they selected backward.
    if( bTrackCols )
    {
      lpCol2 = lpCol;
      nSelectedCols = 0;
      if( pt.x > rectOrig.left )
      {
        while( lpCol2 != lpCurCol && lpCol2->lpNext )
        {
          lpCol2 = lpCol2->lpNext;
          nSelectedCols++;
        }
      }
      else
      {
        while( lpCol2 != lpCurCol && lpCol2->lpPrev )
        {
          lpCol2 = lpCol2->lpPrev;
          nSelectedCols--;
        }
      }
    }

    // Return the selected count.
    *lpSelRecs = nSelectedRecs;
    *lpSelCols = nSelectedCols;
 
    return TRUE;
}

/****************************************************************************
 * Get1stColInView
 *
 * Purpose:
 *  Gets the 1st column which is viewable in the display area.  Also set the
 *  top record for flowed view.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the control window.
 *  LPCONTAINER   lpCnt       - pointer to existing CONTAINER structure.
 *
 * Return Value:
 *  LPFLOWCOLINFO   pointer to the 1st column viewable in the display area
 ****************************************************************************/
LPFLOWCOLINFO Get1stColInView( HWND hWnd, LPCONTAINER lpCnt )
{
    LPFLOWCOLINFO  lpCol;
    int            cxCol, xStrCol, xEndCol;

    xStrCol = -lpCnt->nHscrollPos * lpCnt->lpCI->cxChar;

    lpCol = lpCnt->lpColHead;  // point at 1st column
    while( lpCol )
    {
      cxCol   = lpCol->cxPxlWidth;
      xEndCol = xStrCol + cxCol;
    
      // break when we find the 1st column which ends in the display area
      if( xEndCol > 0 )
        break;  

      xStrCol += cxCol;
      lpCol = lpCol->lpNext;       // advance to next column
    }

    // set new top record for flowed view - not sure if I am using this anymore
    // for flowed view, but just in case...
    if (lpCnt->iView & CV_FLOW)
      lpCnt->lpCI->lpTopRec = lpCol->lpTopRec; // top record in flowed view is top record in leftmost column

    return lpCol;
}

/****************************************************************************
 * GetLastColInView
 *
 * Purpose:
 *  Gets the last column which is viewable in the display area.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the control window.
 *  LPCONTAINER   lpCnt       - pointer to existing CONTAINER structure.
 *
 * Return Value:
 *  LPFLOWCOLINFO   pointer to the last field viewable in the display area
 ****************************************************************************/
LPFLOWCOLINFO GetLastColInView( HWND hWnd, LPCONTAINER lpCnt )
{
    LPFLOWCOLINFO  lpCol;
    int          xStrCol, xEndCol;

    xStrCol = -lpCnt->nHscrollPos * lpCnt->lpCI->cxChar;

    lpCol = lpCnt->lpColHead;  // point at 1st column
    while( lpCol )
    {
      xEndCol = xStrCol + lpCol->cxPxlWidth;
    
      // Break when we find the last column which ends past
      // the display area or we hit the last column
      if( xEndCol > lpCnt->cxClient || !lpCol->lpNext )
        break;  

      xStrCol += lpCol->cxPxlWidth;
      lpCol = lpCol->lpNext;       // advance to next column
    }

    return lpCol;
}

/****************************************************************************
 * InvalidateCntRecTxt
 *
 * Purpose:
 *  Invalidates and repaints (maybe) a container record in text and name views
 *  based on the record and column pointer passed in.  Detail view uses 
 *  InvalidateCntRec() in cvdetail.c
 *
 *  NOTE: For now, the Column Ptr sent in is always NULL since I found no reason
 *        to ever use it.  If the need arises, you can add the code to invalidate
 *        a column easily.
 *
 *      Record Ptr   Column Ptr   Action
 *      ----------   ---------    ------------------------------------
 *        valid        NULL       repaints a container record
 *        NULL         NULL       repaints entire container record area
 *
 * Parameters:
 *  HWND          hWnd        - handle to the control window.
 *  LPCONTAINER   lpCnt       - pointer to existing CONTAINER structure.
 *  LPRECORDCORE  lpRecord    - pointer to record to be repainted
 *  LPFLOWCOLINFO lpFld       - pointer to the column to be repainted
 *  UINT          fMode       - indicating what type of action to take:
 *                              0: no action (just see if the rec is in display area)
 *                              1: invalidate target but DO NOT update
 *                              2: invalidate target and update immediately
 *
 * Return Value:
 *  int           >= 0 position of record in the display area
 *                -1   record was NOT found in the display area
 ****************************************************************************/
int WINAPI InvalidateCntRecTxt( HWND hWnd, LPCONTAINER lpCnt, LPRECORDCORE lpRecord,
                                LPFLOWCOLINFO lpCol, UINT fMode )
{
    LPRECORDCORE  lpRec;
    LPFLOWCOLINFO lpCol2;
    RECT          rect;
    BOOL          bFound=FALSE;
    int           i, yRecAreaTop;
    int           xStrCol, xEndCol;

    // Make sure we have a window and container struct.
    if( !hWnd || !lpCnt )
      return -1;

    lpCol=lpCol; // disable warning message
    
    if( lpRecord )
    {
      // if CV_FLOW, check all records in the display area by going thru columns
      if (lpCnt->iView & CV_FLOW)
      {
        // this will be the start of the first column in the display area
        xStrCol = -lpCnt->nHscrollPos * lpCnt->lpCI->cxChar;

        // find 1st column in the display area
        lpCol2 = lpCnt->lpColHead;
        while (lpCol2)
        {
          xEndCol = xStrCol + lpCol2->cxPxlWidth;

          if (xEndCol > 0)
            break;

          xStrCol += lpCol2->cxPxlWidth;
          lpCol2   = lpCol2->lpNext;
        }
      }
      else
      {
        lpCol2  = lpCnt->lpColHead;
        xStrCol = -lpCnt->nHscrollPos * lpCnt->lpCI->cxChar;
        xEndCol = xStrCol + lpCol2->cxPxlWidth;
      }
      
      while (lpCol2 && !bFound) // find the record (see if it is in display area)
      {
        // If this record is in the display area invalidate it.
        if (lpCnt->iView & CV_FLOW)
          lpRec = lpCol2->lpTopRec;
        else
          lpRec = lpCnt->lpCI->lpTopRec;

        for( i=0; i < lpCnt->lpCI->nRecsDisplayed && lpRec; i++ )
        {
          if( lpRec == lpRecord )
          {
            bFound = TRUE;
            break;
          }
          // advance to next data record
          lpRec = lpRec->lpNext;
        }

        if (bFound == TRUE || !(lpCnt->iView & CV_FLOW))
          break;

        xStrCol += lpCol2->cxPxlWidth;

        if (xStrCol > lpCnt->cxClient) // out of display area
          break;

        // advance to next column
        lpCol2 = lpCol2->lpNext;

        if (lpCol2)
          xEndCol = xStrCol + lpCol2->cxPxlWidth;
      }

      if( bFound && fMode )
      {
        rect.left  = xStrCol;
        rect.right = xEndCol;

        yRecAreaTop = lpCnt->lpCI->cyTitleArea;
        rect.top    = yRecAreaTop + i * lpCnt->lpCI->cyRow;
        rect.bottom = rect.top + lpCnt->lpCI->cyRow;

        if( fMode == 1 )
          InvalidateRect( hWnd, &rect, FALSE );
        else if( fMode == 2 )
          UpdateContainer( hWnd, &rect, TRUE );
      }
    }
    else
    {
       // Set to the entire width of the container.
       rect.left  = 0;
       rect.right = min(lpCnt->cxClient, lpCnt->xLastCol);

       // Set the top and bottom of the record area.
       rect.top    = lpCnt->lpCI->cyTitleArea;
       rect.bottom = lpCnt->cyClient;

       if( fMode == 1 )
         InvalidateRect( hWnd, &rect, FALSE );
       else if( fMode == 2 )
         UpdateContainer( hWnd, &rect, TRUE );
    }

    return bFound ? i : -1;
}

/****************************************************************************
 * TxtExtSelect
 *
 * Purpose:
 *  Selects the specified records after dragging the mouse in extended selection.
 *
 * Parameters:
 *  HWND          hWnd        - Container child window handle 
 *  LPCONTAINER   lpCnt       - pointer to window's CONTAINER structure.
 *  LPRECORDCORE  lpRec       - pointer to starting record to be selected
 *  LPFLOWCOLINFO lpCol       - pointer to starting column
 *  int           nSelRecs    - number of records to select
 *  LPRECORDCORE  lpRecLast   - pointer to final record that was selected
 *                              
 * Return Value:
 *  BOOL          TRUE  if selection was set with no errors
 *                FALSE if selection could not be set
 ****************************************************************************/
BOOL NEAR TxtExtSelect( HWND hWnd, LPCONTAINER lpCnt, LPRECORDCORE lpRec,
                        int nSelRecs, LPRECORDCORE FAR *lpRecLast )
{
    LPRECORDCORE  lpRec2;
    int  i;
    int  nSelectedRecs=nSelRecs;
    BOOL bdir; // 0 means select backwards, 1 means select forwards

    if( !lpRec )
      return FALSE;

    if (nSelRecs < 0)
    {
      nSelectedRecs = -nSelectedRecs;
      bdir = 0;
    }
    else
      bdir = 1;
    
    // select the records
    lpRec2 = *lpRecLast = lpRec;
    for( i=0; lpRec2 && i<nSelectedRecs; i++ )
    {
      // Toggle the selection state of this record.
      if( lpRec2->flRecAttr & CRA_SELECTED )
      {
        lpRec2->flRecAttr &= ~CRA_SELECTED;
        NotifyAssocWnd( hWnd, lpCnt, CN_RECUNSELECTED, 0, lpRec2, NULL, 0, 0, 0, NULL );
      }
      else
      {
        lpRec2->flRecAttr |= CRA_SELECTED;
        NotifyAssocWnd( hWnd, lpCnt, CN_RECSELECTED, 0, lpRec2, NULL, 0, 0, 0, NULL );
      }

      InvalidateCntRecord( hWnd, lpCnt, lpRec2, NULL, 1 );

      // Update the return record pointer.
      *lpRecLast = lpRec2;

      // Get the next or previous record.
      if( bdir > 0 )
        lpRec2 = CntNextRec( lpRec2 );
      else
        lpRec2 = CntPrevRec( lpRec2 );
    }

    return TRUE;
}

/****************************************************************************
 * TxtIsFocusInView
 *
 * Purpose:
 *  Determines whether the focus cell is viewable in the display area.  This
 *  is for text and name views.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *  LPCONTAINER   lpCnt       - pointer to container structure
 *
 * Return Value:
 *  BOOL          TRUE if any part of the focus cell is viewable
 *                FALSE if no part of the focus cell is viewable
 ****************************************************************************/
BOOL WINAPI TxtIsFocusInView( HWND hWnd, LPCONTAINER lpCnt )
{
    BOOL bIsInView=FALSE;
    int  i;

    if( lpCnt->lpCI->lpFocusRec )
    {
      // If not flowed, call regular checking function
  	  if ( !(lpCnt->iView & CV_FLOW) )
        bIsInView = IsRecInView( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec );
  	  else
	    {
        LPFLOWCOLINFO lp1stCol, lpLastCol;
        LPRECORDCORE lpRec;

        lp1stCol  = Get1stColInView(hWnd, lpCnt);
        lpLastCol = GetLastColInView(hWnd, lpCnt);

        // loop thru each column looking for the focus record
        while (lp1stCol != lpLastCol)
        {
          lpRec = lp1stCol->lpTopRec;
          for( i=0; i < lpCnt->lpCI->nRecsDisplayed && lpRec; i++ )
          {
            if( lpRec == lpCnt->lpCI->lpFocusRec )
            {
              bIsInView = TRUE;
              break;
            }
            // advance to next data record
            lpRec = lpRec->lpNext;
          }
          if (bIsInView) break;
          
          // advance to next column
          lp1stCol = lp1stCol->lpNext;
        }

        if (!bIsInView)
        {
          // check last column
          lpRec = lp1stCol->lpTopRec;
          for( i=0; i < lpCnt->lpCI->nRecsDisplayed && lpRec; i++ )
          {
            if( lpRec == lpCnt->lpCI->lpFocusRec )
            {
              bIsInView = TRUE;
              break;
            }
            // advance to next data record
            lpRec = lpRec->lpNext;
          }
        }
      }
    }

    return bIsInView;
}

/****************************************************************************
 * TxtScrollToFocus
 *
 * Purpose:
 *  Scrolls the focus record into view. Is called if the focus record is not
 *  in view and a keyboard char is pressed. The scrolling should occur
 *  prior to the desired keyboard action.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *  LPCONTAINER   lpCnt       - pointer to container structure
 *  UINT          wVertDir    - vert direction user is scrolling: 0=UP; 1=DOWN
 *
 * Return Value:
 *  VOID
 ****************************************************************************/
VOID WINAPI TxtScrollToFocus( HWND hWnd, LPCONTAINER lpCnt, UINT wVertDir )
{
    LPCONTAINER  lpCntFrame = GETCNT( lpCnt->hWndFrame );
    LPRECORDCORE lpRec;
    BOOL         bFound=FALSE;
    int          nInc=0;
    int          xStrCol, xEndCol;

    // If the scroll-to-focus behavior is disabled just return.
    if( !lpCntFrame->bScrollFocusRect )
      return;

    if (lpCnt->iView & CV_FLOW)
    {
      // Scroll horizontally to the focus record 
      if( lpCnt->lpCI->lpFocusRec )
      {
        GetRecXPos( lpCnt, lpCnt->lpCI->lpFocusRec, &xStrCol, &xEndCol );

        if( xStrCol < 0 || xStrCol > lpCnt->cxClient )
        {
          // Focus rect was not in display area or it is now moving
          // out of the display area so we will do a scroll.
          nInc = xStrCol / lpCnt->lpCI->cxChar;
          nInc += lpCnt->nHscrollPos;
          FORWARD_WM_HSCROLL( hWnd, 0, SB_THUMBPOSITION, nInc, SendMessage );
        }
      }
    }
    else
    {
      // First look for the focus record in the list.
      if( lpCnt->lpCI->lpFocusRec )
      {
        // If app is handling the vertical scroll, don't look for it.
        if( !(lpCnt->lpCI->flCntAttr & CA_OWNERVSCROLL ) )
        {
          lpRec = CntRecHeadGet( hWnd );
          while( lpRec )
          {
            if( lpRec == lpCnt->lpCI->lpFocusRec )
            {
              bFound = TRUE;
              break;
            }
    
            lpRec = lpRec->lpNext;
            nInc++;
          }
        }
      }

      // Scroll vertically to the focus record if it was found.
      if( bFound )
      {
        // NOTE: This never happens if CA_OWNERVSCROLL is active!
        nInc -= wVertDir ? (lpCnt->lpCI->nRecsDisplayed-2) : 1;

        // Note that for 32 bit scrolling we are passing the position in the
        // unused hwndCtl parm slot. When the msg is received that value will
        // be used instead of trying to calculate the position.
        if( StateTest( lpCnt, CNTSTATE_VSCROLL32 ) )
          FORWARD_WM_VSCROLL( hWnd, nInc, SB_THUMBPOSITION, nInc, SendMessage );
        else
          FORWARD_WM_VSCROLL( hWnd, 0, SB_THUMBPOSITION, nInc, SendMessage );
      }
      else
      {
        // If not found it means CA_OWNERVSCROLL is active or we are in a
        // DELTA model and the focus record is not currently in memory.
        // In either case the app knows where the focus record is and will
        // build a new list with the focus record at the head.
        NotifyAssocWnd( hWnd, lpCnt, CN_QUERYFOCUS, 0, NULL, NULL, 0, 0, 0, NULL );
      }
    }

    return;
}

/****************************************************************************
 * GetRecXPos
 *
 * Purpose:
 *  Gets the horz starting and ending position of the specified record
 *
 * Parameters:
 *  LPCONTAINER   lpCnt       - pointer to existing CONTAINER structure.
 *  LPRECORDCORE  lpRec       - pointer to the record
 *  LPINT         lpxStr      - returned starting position of the record
 *  LPINT         lpxEnd      - returned ending position of the record
 *
 * Return Value:
 *  BOOL          TRUE if function is successful; else FALSE
 ****************************************************************************/
int GetRecXPos( LPCONTAINER lpCnt, LPRECORDCORE lpRec, LPINT lpxStr, LPINT lpxEnd )
{
    LPFLOWCOLINFO lpCol;
    LPRECORDCORE  lpRec1;
    int  xStrCol=0, xEndCol=0, i;
    BOOL bFound=FALSE;

    if( lpRec )
    {
      xStrCol = -lpCnt->nHscrollPos * lpCnt->lpCI->cxChar;

      lpCol = lpCnt->lpColHead;  // point at 1st column
      while( lpCol )
      {
        xEndCol = xStrCol + lpCol->cxPxlWidth;
    
        // break out when we find the column
        lpRec1 = lpCol->lpTopRec;
        for (i=0; i< lpCnt->lpCI->nRecsDisplayed && lpRec1; i++)
        {
          if (lpRec1==lpRec)
          {
            bFound = TRUE;
            break;
          }
          
          lpRec1 = lpRec1->lpNext; // advance to next record
        }
        if (bFound) break;

        xStrCol += lpCol->cxPxlWidth;
        lpCol    = lpCol->lpNext;    // advance to next column
      }
    }

    *lpxStr = xStrCol;
    *lpxEnd = xEndCol;
    return (bFound) ? i : -1;
}

/****************************************************************************
 * GetColXPos
 *
 * Purpose:
 *  Gets the horz starting and ending position of a specified column
 *
 * Parameters:
 *  LPCONTAINER   lpCnt       - pointer to existing CONTAINER structure.
 *  LPFLOWCOLNFO  lpCol       - pointer to the column
 *  LPINT         lpxStr      - returned starting position of the column
 *  LPINT         lpxEnd      - returned ending position of the column
 *
 * Return Value:
 *  BOOL          TRUE if function is successful; else FALSE
 ****************************************************************************/
BOOL GetColXPos( LPCONTAINER lpCnt, LPFLOWCOLINFO lpCol, LPINT lpxStr, LPINT lpxEnd )
{
    LPFLOWCOLINFO lpCol1;
    int           cxCol, xStrCol=0, xEndCol=0;

    if( lpCol )
    {
      xStrCol = -lpCnt->nHscrollPos * lpCnt->lpCI->cxChar;

      lpCol1 = lpCnt->lpColHead;  // point at 1st column
      while( lpCol1 )
      {
        cxCol   = lpCol1->cxPxlWidth;
        xEndCol = xStrCol + cxCol;
    
        // break out when we find the column
        if( lpCol1 == lpCol )
          break;  

        xStrCol += cxCol;
        lpCol1 = lpCol1->lpNext;       // advance to next column
      }
    }

    *lpxStr = xStrCol;
    *lpxEnd = xEndCol;
    return TRUE;
}
