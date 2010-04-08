/****************************************************************************
 * Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * CVDETAIL.C
 *
 * Contains message handler for the DETAIL view format.
 ****************************************************************************/

/*
**  History:
**  02-Jan-2002 (noifr01)
**      (SIR 99596) removed usage of unused or obsolete resources
*/

#include <windows.h>
#include <windowsx.h>   // Need this for the memory macros
#include <stdlib.h>
#include <malloc.h>
#include "cntdll.h"
#include "ca_prntf.h"   // ca_sprintf stuff for floating point
#include "cntdrag.h"

/*char szDebug[100];*/

// Prototypes for local helper functions.
BOOL NEAR IsFldMoveable( LPCONTAINER lpCnt, LPFIELDINFO lpFld );
int  NEAR FindFocusFld( HWND hWnd, LPCONTAINER lpCnt, LPFIELDINFO lpFld );
LPFIELDINFO NEAR GetLastFldInView( HWND hWnd, LPCONTAINER lpCnt );
BOOL NEAR BlkSelectFld( HWND hWnd, LPCONTAINER lpCnt, LPRECORDCORE lpRec,
                        LPFIELDINFO lpFld, int nSelectedRecs, int nSelectedFlds,
                        LPRECORDCORE FAR *lpRecLast, LPFIELDINFO FAR *lpFldLast );
BOOL NEAR TrackSelection( HWND hWnd, LPCONTAINER lpCnt, LPRECORDCORE lpRec, LPFIELDINFO lpFld, LPPOINT lpPt,
                          LPINT lpSelRecs, LPINT lpSelFlds, BOOL bTrackFlds );
BOOL NEAR BatchFldSelect( HWND hWnd, LPCONTAINER lpCnt, LPFIELDINFO lpFldStr,
                          LPFIELDINFO lpFldEnd, int nDir );

// Prototypes for Detail view msg cracker functions.
BOOL Dtl_OnNCCreate( HWND hWnd, LPCREATESTRUCT lpCreateStruct );
BOOL Dtl_OnCreate( HWND hWnd, LPCREATESTRUCT lpCreateStruct );
VOID Dtl_OnCommand( HWND hWnd, int id, HWND hwndCtl, UINT codeNotify );
VOID Dtl_OnSize( HWND hWnd, UINT state, int cx, int cy );
BOOL Dtl_OnEraseBkgnd( HWND hWnd, HDC hDC );
VOID Dtl_OnCut( HWND hWnd );
VOID Dtl_OnCopy( HWND hWnd );
VOID Dtl_OnPaste( HWND hWnd );
VOID Dtl_OnClear( HWND hWnd );
VOID Dtl_OnUndo( HWND hWnd );
VOID Dtl_OnPaint( HWND hWnd );
VOID Dtl_OnSetFocus( HWND hWnd, HWND hwndOldFocus );
VOID Dtl_OnKillFocus( HWND hWnd, HWND hwndNewFocus );
VOID Dtl_OnVScroll( HWND hWnd, HWND hwndCtl, UINT code, int pos );
VOID Dtl_OnHScroll( HWND hWnd, HWND hwndCtl, UINT code, int pos );
VOID Dtl_OnChar( HWND hWnd, UINT ch, int cRepeat );
VOID Dtl_OnKey( HWND hWnd, UINT vk, BOOL fDown, int cRepeat, UINT flags );
VOID Dtl_OnRButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags );
VOID Dtl_OnLButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags );
VOID Dtl_OnMouseMove( HWND hWnd, int x, int y, UINT keyFlags );
VOID Dtl_OnLButtonUp( HWND hWnd, int x, int y, UINT keyFlags );
VOID Dtl_OnNCLButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest );
VOID Dtl_OnNCMButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest );
VOID Dtl_OnNCRButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest );


/****************************************************************************
 * DetailViewWndProc
 *
 * Purpose:
 *  Handles all window messages for a container in DETAIL view format.
 *  Any message not processed here should go to DefWindowProc.
 *
 * Parameters:
 *  Standard for WndProc
 *
 * Return Value:
 *  LRESULT       varies with the message
 ****************************************************************************/

LRESULT WINAPI DetailViewWndProc( HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam )
    {
    switch( iMsg )
      {
      HANDLE_MSG( hWnd, WM_NCCREATE,       Dtl_OnNCCreate      );
      HANDLE_MSG( hWnd, WM_CREATE,         Dtl_OnCreate        );
      HANDLE_MSG( hWnd, WM_COMMAND,        Dtl_OnCommand       );
      HANDLE_MSG( hWnd, WM_SIZE,           Dtl_OnSize          );
      HANDLE_MSG( hWnd, WM_CHILDACTIVATE,  Dtl_OnChildActivate );
      HANDLE_MSG( hWnd, WM_DESTROY,        Dtl_OnDestroy       );
      HANDLE_MSG( hWnd, WM_ERASEBKGND,     Dtl_OnEraseBkgnd    );
      HANDLE_MSG( hWnd, WM_CUT,            Dtl_OnCut           );
      HANDLE_MSG( hWnd, WM_COPY,           Dtl_OnCopy          );
      HANDLE_MSG( hWnd, WM_PASTE,          Dtl_OnPaste         );
      HANDLE_MSG( hWnd, WM_CLEAR,          Dtl_OnClear         );
      HANDLE_MSG( hWnd, WM_UNDO,           Dtl_OnUndo          );
      HANDLE_MSG( hWnd, WM_PAINT,          Dtl_OnPaint         );
      HANDLE_MSG( hWnd, WM_ENABLE,         Dtl_OnEnable        );
      HANDLE_MSG( hWnd, WM_SETFOCUS,       Dtl_OnSetFocus      );
      HANDLE_MSG( hWnd, WM_KILLFOCUS,      Dtl_OnKillFocus     );
      HANDLE_MSG( hWnd, WM_SHOWWINDOW,     Dtl_OnShowWindow    );
      HANDLE_MSG( hWnd, WM_CANCELMODE,     Dtl_OnCancelMode    );
      HANDLE_MSG( hWnd, WM_VSCROLL,        Dtl_OnVScroll       );
      HANDLE_MSG( hWnd, WM_HSCROLL,        Dtl_OnHScroll       );
      HANDLE_MSG( hWnd, WM_CHAR,           Dtl_OnChar          );
      HANDLE_MSG( hWnd, WM_KEYDOWN,        Dtl_OnKey           );
      HANDLE_MSG( hWnd, WM_RBUTTONDBLCLK,  Dtl_OnRButtonDown   );
      HANDLE_MSG( hWnd, WM_RBUTTONDOWN,    Dtl_OnRButtonDown   );
      HANDLE_MSG( hWnd, WM_LBUTTONDBLCLK,  Dtl_OnLButtonDown   );
      HANDLE_MSG( hWnd, WM_LBUTTONDOWN,    Dtl_OnLButtonDown   );
      HANDLE_MSG( hWnd, WM_MOUSEMOVE,      Dtl_OnMouseMove     );
      HANDLE_MSG( hWnd, WM_LBUTTONUP,      Dtl_OnLButtonUp     );
      HANDLE_MSG( hWnd, WM_GETDLGCODE,     Dtl_OnGetDlgCode    );
      HANDLE_MSG( hWnd, WM_NCLBUTTONDOWN,  Dtl_OnNCLButtonDown );
      HANDLE_MSG( hWnd, WM_NCMBUTTONDOWN,  Dtl_OnNCMButtonDown );
      HANDLE_MSG( hWnd, WM_NCRBUTTONDOWN,  Dtl_OnNCRButtonDown );


      default:
        return (Process_DefaultMsg(hWnd,iMsg,wParam,lParam));
          
      }

    return 0L;
    }



/****************************************************************************
 * DetailViewWndProc Message Cracker Callback Functions
 *
 * Purpose:
 *  Functions to process each message received by the Container WndProc.
 *  This is a Win16/Win32 portable method for message handling.
 *
 * Return Value:
 *  Varies according to the message being processed.
 ****************************************************************************/

BOOL Dtl_OnNCCreate( HWND hWnd, LPCREATESTRUCT lpCreateStruct )
    {
    LPCONTAINER  lpCntNew, lpCntOld;

    // Get the existing container struct being passed in the create params.
    lpCntOld = (LPCONTAINER) lpCreateStruct->lpCreateParams;

    // If we are creating a split child alloc a new struct and load it.
    if( StateTest( lpCntOld, CNTSTATE_CREATINGSB ) )
      {
/*      lpCntNew = (LPCONTAINER) GlobalAllocPtr( GHND, LEN_CONTAINER );*/
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

BOOL Dtl_OnCreate( HWND hWnd, LPCREATESTRUCT lpCreateStruct )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);
    LPCONTAINER lpCntOld;

    // Init flag indicating whether or not to send focus chging notifications.
    lpCnt->lpCI->bSendFocusMsg = TRUE;

    // Init flag indicating whether or not to send keyboard events.
    lpCnt->lpCI->bSendKBEvents = TRUE;

    // Init flag indicating whether or not to send mouse events.
    lpCnt->lpCI->bSendMouseEvents = TRUE;

    // Init flag indicating whether or not to draw the focust rect.
    lpCnt->lpCI->bDrawFocusRect = TRUE;

    // Get the existing container struct being passed in the create params.
    lpCntOld = (LPCONTAINER) lpCreateStruct->lpCreateParams;

    // Init focus recs to NULL if not creating split child
    if( !(StateTest( lpCntOld, CNTSTATE_CREATINGSB )) )
    {
      lpCnt->lpCI->lpFocusRec = NULL;
      lpCnt->lpCI->lpFocusFld = NULL;
    }

    // Initialize the character metrics.
    InitTextMetrics( hWnd, lpCnt );

    // NOTE: When using a msg cracker for WM_CREATE we must
    //       return TRUE if all is well instead of 0.
    return TRUE;
    }

VOID Dtl_OnCommand( HWND hWnd, int id, HWND hwndCtl, UINT codeNotify )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);

    switch( id )
      {
      // All msgs from edit controls are simply passed to the assoc wnd.
      case CE_ID_EDIT1:
      case CE_ID_EDIT2:
      case CE_ID_EDIT3:
      case CE_ID_EDIT4:
      case CE_ID_EDIT5:
      case CE_ID_EDIT6:
      case CE_ID_EDIT7:
      case CE_ID_EDIT8:
      case CE_ID_EDIT9:
      case CE_ID_EDIT10:
      case CE_ID_EDIT11:
      case CE_ID_EDIT12:
        FORWARD_WM_COMMAND( lpCnt->hWndAssociate, id, hwndCtl, codeNotify, SendMessage );
        break;
      }

    return;
    }

VOID Dtl_OnSize( HWND hWnd, UINT state, int cx, int cy )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);
    LPCONTAINER lpCntFrame;
    LPFIELDINFO lpFld;
    LPRECORDCORE lpRec, lpOldTopRec;
    RECT rect;
    BOOL bHasVscroll, bHasHscroll, bRepaint;
    int  cxOldClient, xStrCol, xEndCol, nOldHscrollPos;
    int  cxClient, cyClient, nExpand;
    int  cyRecordArea, nMaxWidth, nVscrollInc;

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
    nMaxWidth = lpCnt->lpCI->nMaxWidth;

    // set the nbr of records which are in view
    cyRecordArea = cyClient - lpCnt->lpCI->cyTitleArea - lpCnt->lpCI->cyColTitleArea;
    lpCnt->lpCI->nRecsDisplayed = (cyRecordArea-1) / lpCnt->lpCI->cyRow + 1;

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

    // Expand the last field if necessary if the style is enabled.
    // (unless we are in the middle of dynamically moving a field)
    lpFld = CntFldTailGet( hWnd );
    lpCntFrame = GETCNT( lpCnt->hWndFrame );
    if( (lpCnt->dwStyle & CTS_EXPANDLASTFLD) && lpFld && !lpCntFrame->bMovingFld)
      {
      // Don't do anything if there is no unused background area.
      if( lpCnt->xLastCol < lpCnt->cxClient )
        {
        // Calc how much the field needs to expand.
        nExpand = (lpCnt->cxClient - lpCnt->xLastCol) / lpCnt->lpCI->cxChar;
    
        // Expand the field width.
        lpFld->cxWidth += nExpand;
        lpFld->cxPxlWidth = lpFld->cxWidth * lpCnt->lpCI->cxChar;

        // Increase the max width of all fields by the expansion amount.
        lpCnt->lpCI->nMaxWidth += nExpand * lpCnt->lpCI->cxChar;

        // Gotta check the Horz scrolling again since the max width changed.
        if( bHasHscroll )
          {
          if( lpCnt->lpCI->nMaxWidth - cxClient > 0 )
            lpCnt->nHscrollMax = max(0, (lpCnt->lpCI->nMaxWidth - cxClient - 1) / lpCnt->lpCI->cxChar + 1);
          else
            lpCnt->nHscrollMax = 0;

          lpCnt->nHscrollPos = min(lpCnt->nHscrollPos, lpCnt->nHscrollMax);

          SetScrollPos  ( hWnd, SB_HORZ,    lpCnt->nHscrollPos, FALSE  );
          SetScrollRange( hWnd, SB_HORZ, 0, lpCnt->nHscrollMax, TRUE );
          }

        // Reset the last X column.
        lpCnt->xLastCol = lpCnt->lpCI->nMaxWidth - lpCnt->nHscrollPos * lpCnt->lpCI->cxChar;

        // New repaint code necessary because of removing redraw-on-size styles.
        // Calc and invalidate the field title area and the record
        // area for this field so it will be entirely repainted.
        GetFieldXPos( lpCnt, lpFld, &xStrCol, &xEndCol );
        rect.left   = xStrCol;
        rect.top    = lpCnt->lpCI->cyTitleArea;
        rect.right  = cx;
        rect.bottom = cy;
        InvalidateRect( hWnd, &rect, FALSE );
        }
      }

    if( bHasVscroll )
      {
      // Set up the vertical scrollbar.
      AdjustVertScroll( hWnd, lpCnt, TRUE );
      }

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
    if( lpOldTopRec && lpOldTopRec != lpCnt->lpCI->lpTopRec )
      {
      // Calc the rect for the record area.
      rect.left   = 0;
      rect.top    = lpCnt->lpCI->cyTitleArea + lpCnt->lpCI->cyColTitleArea;
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

VOID Dtl_OnChildActivate( HWND hWnd )
    {
    // If this window is being activated, give it the focus.
    SetFocus( hWnd );
    return;
    }

VOID Dtl_OnDestroy( HWND hWnd )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);

    // Free all the memory for this container.
    ContainerDestroy( hWnd, lpCnt );
    return;
    }

BOOL Dtl_OnEraseBkgnd( HWND hWnd, HDC hDC )
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

VOID Dtl_OnCut( HWND hWnd )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);

    NotifyAssocWnd( hWnd, lpCnt, CN_CUT, 0, NULL, NULL, 0, 0, 0, NULL );
    return;
    }

VOID Dtl_OnCopy( HWND hWnd )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);

    NotifyAssocWnd( hWnd, lpCnt, CN_COPY, 0, NULL, NULL, 0, 0, 0, NULL );
    return;
    }

VOID Dtl_OnPaste( HWND hWnd )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);

    NotifyAssocWnd( hWnd, lpCnt, CN_PASTE, 0, NULL, NULL, 0, 0, 0, NULL );
    return;
    }

VOID Dtl_OnClear( HWND hWnd )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);

    NotifyAssocWnd( hWnd, lpCnt, CN_CLEAR, 0, NULL, NULL, 0, 0, 0, NULL );
    return;
    }

VOID Dtl_OnUndo( HWND hWnd )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);

    NotifyAssocWnd( hWnd, lpCnt, CN_UNDO, 0, NULL, NULL, 0, 0, 0, NULL );
    return;
    }

VOID Dtl_OnPaint( HWND hWnd )
    {
    LPCONTAINER  lpCnt = GETCNT(hWnd);
    PAINTSTRUCT ps;
    HDC         hDC;

    hDC = BeginPaint( hWnd, &ps );
    
    PaintDetailView( hWnd, lpCnt, hDC, (LPPAINTSTRUCT)&ps );

    EndPaint( hWnd, &ps );

    return;
    }

VOID Dtl_OnEnable( HWND hWnd, BOOL fEnable )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);

    if( fEnable )
      StateClear( lpCnt, CNTSTATE_GRAYED );
    else
      StateSet( lpCnt, CNTSTATE_GRAYED );

    return;
    }

VOID Dtl_OnSetFocus( HWND hWnd, HWND hwndOldFocus )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);
    LPCONTAINER lpCntFrame;

    if( !lpCnt )
      return;

    StateSet( lpCnt, CNTSTATE_HASFOCUS );

    // Display the focus rect.
    if( lpCnt->lpCI && lpCnt->lpCI->lpFocusRec && lpCnt->lpCI->lpFocusFld )
      {
      if( lpCnt->lpCI->flCntAttr & CA_WIDEFOCUSRECT )
        InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
      else
        InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec,
                                          lpCnt->lpCI->lpFocusFld, 2 );
      }

    // Bring the container to the top if its getting the focus.
    BringWindowToTop( hWnd );

    // Tell the assoc wnd about change in focus state.
    if( lpCnt->lpCI && lpCnt->lpCI->bSendFocusMsg )
      {
      if( lpCnt->bIsSplit )
        {
        // Don't send focus msg if we are simply moving between split children.
        lpCntFrame = GETCNT( lpCnt->hWndFrame );
        if( hwndOldFocus != lpCnt->hWnd1stChild &&
            hwndOldFocus != lpCntFrame->SplitBar.hWndLeft )
          NotifyAssocWnd( hWnd, lpCnt, CN_SETFOCUS, 0, NULL, NULL, 0, 0, 0, (LPVOID)hwndOldFocus );
        }
      else
        {
        NotifyAssocWnd( hWnd, lpCnt, CN_SETFOCUS, 0, NULL, NULL, 0, 0, 0, (LPVOID)hwndOldFocus );
        }
      }

    return;
    }

VOID Dtl_OnKillFocus( HWND hWnd, HWND hwndNewFocus )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);
    LPCONTAINER lpCntFrame;

    if( !lpCnt )
      return;

    StateClear( lpCnt, CNTSTATE_HASFOCUS );

    // Save this hWnd to indicate which split child had the focus last.
    // If the focus returns to the container thru the frame window it will
    // use this hWnd to decide which split child to return the focus to.
    lpCntFrame = GETCNT( lpCnt->hWndFrame );
    lpCntFrame->hWndLastFocus = hWnd;

    // Clear the focus rect.
    if( lpCnt->lpCI && lpCnt->lpCI->lpFocusRec && lpCnt->lpCI->lpFocusFld )
      {
      if( lpCnt->lpCI->flCntAttr & CA_WIDEFOCUSRECT )
        InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
      else
        InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec,
                                          lpCnt->lpCI->lpFocusFld, 2 );
      }

    // Tell the assoc wnd about change in focus state.
    if( lpCnt->lpCI && lpCnt->lpCI->bSendFocusMsg )
      {
      if( lpCnt->bIsSplit )
        {
        // Don't send focus msg if we are simply moving between split children.
        if( hwndNewFocus != lpCnt->hWnd1stChild &&
            hwndNewFocus != lpCntFrame->SplitBar.hWndLeft )
          NotifyAssocWnd( hWnd, lpCnt, CN_KILLFOCUS, 0, NULL, NULL, 0, 0, 0, (LPVOID)hwndNewFocus );
        }
      else
        {
        NotifyAssocWnd( hWnd, lpCnt, CN_KILLFOCUS, 0, NULL, NULL, 0, 0, 0, (LPVOID)hwndNewFocus );
        }
      }

    return;
    }

VOID Dtl_OnShowWindow( HWND hWnd, BOOL fShow, UINT status )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);

    if( fShow )
      StateClear(lpCnt, CNTSTATE_HIDDEN);
    else
      StateSet(lpCnt, CNTSTATE_HIDDEN);

    return;
    }

VOID Dtl_OnCancelMode( HWND hWnd )
{
    LPCONTAINER lpCnt = GETCNT(hWnd);

   /*-------------------------------------------------------------------*
    * IMPORTANT MESSAGE!  WM_CANCELMODE means that a dialog or some
    * other modal process has started. We must make sure that we cancel
    * any state we are in and release the capture.
    *-------------------------------------------------------------------*/ 
    StateClear( lpCnt, CNTSTATE_ALL );
   
    /*-------------------------------------------------------------------*
     * NOTE: we must forward this message to ensure proper handling of    
     * mouse input in the case were we have not explictly done a set capture
     * The affect of not forwarding this is that windows does not generate
     * messages for the scroll bars. So in a delta container that gets this
     * message after sending a CN_QUERYDELTA message caused by a single click
     * of the scroll arrow, windows will still assume the scroll bar is active
     * and when the cursor is left over the arrow the container will start 
     * scrolling.
     *--------------------------------------------------------------------*/
#ifdef WIN32
    FORWARD_WM_CANCELMODE(hWnd,CNT_DEFWNDPROC);
#endif

    ReleaseCapture();

    return;
}

VOID Dtl_OnVScroll( HWND hWnd, HWND hwndCtl, UINT code, int pos )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);
    LPCONTAINER lpCntFrame;
    LPCONTAINER lpCntSplit;
    HWND       hWndSplit;
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
        cyRecordArea = lpCnt->cyClient - lpCnt->lpCI->cyTitleArea - lpCnt->lpCI->cyColTitleArea;
        nVscrollInc  = min(-1, -cyRecordArea / lpCnt->lpCI->cyRow);
        if( lpCnt->lpCI->bFocusRecLocked )
          wScrollEvent = CN_LK_VS_PAGEUP;
        else
          wScrollEvent = CN_VSCROLL_PAGEUP;
        break;

      case SB_PAGEDOWN:
        // Calc the area being used for record data.
        cyRecordArea = lpCnt->cyClient - lpCnt->lpCI->cyTitleArea - lpCnt->lpCI->cyColTitleArea;
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
        rect.top    = lpCnt->lpCI->cyTitleArea + lpCnt->lpCI->cyColTitleArea;
        rect.right  = min( lpCnt->cxClient, lpCnt->xLastCol);
        rect.bottom = lpCnt->cyClient;
        ScrollWindow( hWnd, 0, -lpCnt->lpCI->cyRow * nVscrollInc, NULL, &rect );

        // Now reposition the scroll thumb.
        SetScrollPos( hWnd, SB_VERT, lpCnt->nVscrollPos, TRUE );
  
        // If container is split scroll the other split child also.
        hWndSplit = NULL;
        if( lpCnt->dwStyle & CTS_SPLITBAR && lpCnt->bIsSplit )
          {
          // Get the window handle of the other split child.
          lpCntFrame = GETCNT( lpCnt->hWndFrame );
          if( hWnd == lpCntFrame->SplitBar.hWndLeft )
            hWndSplit = lpCntFrame->SplitBar.hWndRight;
          else
            hWndSplit = lpCntFrame->SplitBar.hWndLeft;

          // Get the container struct of the other split child.
          lpCntSplit = GETCNT( hWndSplit );

          // calc the scrolling rect
          rect.right  = min( lpCntSplit->cxClient, lpCntSplit->xLastCol);
          rect.bottom = lpCntSplit->cyClient;
  
          lpCntSplit->nVscrollPos += nVscrollInc;
          ScrollWindow( hWndSplit, 0, -lpCntSplit->lpCI->cyRow * nVscrollInc, NULL, &rect );
          SetScrollPos( hWndSplit, SB_VERT, lpCntSplit->nVscrollPos, TRUE );
          }

        // Scroll the record list also so we always have a pointer to the 
        // record currently displayed at the top of the container wnd. 
        // If a CN_QUERYDELTA was sent we want to scroll the records from
        // the head taking into account the delta position.
        if( !bSentQueryDelta )
          lpCnt->lpCI->lpTopRec = CntScrollRecList( hWnd, lpCnt->lpCI->lpTopRec, nVscrollInc );
        else
          lpCnt->lpCI->lpTopRec = CntScrollRecList( hWnd, lpCnt->lpCI->lpRecHead, lpCnt->nVscrollPos-lpCnt->lDeltaPos );
  
        if( hWndSplit )
          UpdateWindow( hWndSplit );
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
      if( nVscrollInc || lpCnt->lpCI->flCntAttr & CA_OWNERVSCROLL ||
          wScrollEvent == CN_VSCROLL_THUMBPOS)
        NotifyAssocWnd( hWnd, lpCnt, wScrollEvent, 0, NULL, NULL, nVscrollInc, 0, 0, NULL );
      }
    return;
    }

VOID Dtl_OnHScroll( HWND hWnd, HWND hwndCtl, UINT code, int pos )
    {
    LPCONTAINER  lpCnt = GETCNT(hWnd);
    LPFIELDINFO lpCol;
    RECT        rect;
    UINT        wScrollEvent;
    int         nHscrollInc;
    int         xStrCol, xEndCol;

    switch( code )
      {
      case SB_LINEUP:
        if( !(lpCnt->lpCI->flCntAttr & CA_HSCROLLBYCHAR) )
        {
          // Get the starting xcoord of the 1st field which starts
          // to the left of the viewing area .
          lpCol    = Get1stFldInView( hWnd, lpCnt );
/*          dwFldPos = GetFieldXPos( lpCnt, lpCol );*/
/*          xStrCol  = LOWORD(dwFldPos);*/
          GetFieldXPos( lpCnt, lpCol, &xStrCol, &xEndCol );

          if( xStrCol >= 0 )
          {
            lpCol = CntScrollFldList( hWnd, lpCol, -1 );

            // If the prev field is hidden find the 1st non-hidden before it.
            if( !lpCol->cxWidth )
            {
              // break out when we find a col with some width
              while( lpCol->lpPrev )
              {
                lpCol = lpCol->lpPrev;
                if( lpCol->cxWidth )
                  break;  
              }
            }

/*            dwFldPos = GetFieldXPos( lpCnt, lpCol );*/
/*            xStrCol  = LOWORD(dwFldPos);*/
            GetFieldXPos( lpCnt, lpCol, &xStrCol, &xEndCol );
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
          lpCol    = Get1stFldInView( hWnd, lpCnt );
/*          dwFldPos = GetFieldXPos( lpCnt, lpCol );*/
/*          xStrCol  = LOWORD(dwFldPos);*/
/*          xEndCol  = HIWORD(dwFldPos);*/
          GetFieldXPos( lpCnt, lpCol, &xStrCol, &xEndCol );

          if( xStrCol >= 0 )
            {
            lpCol    = CntScrollFldList( hWnd, lpCol, 1 );
/*            dwFldPos = GetFieldXPos( lpCnt, lpCol );*/
/*            xStrCol  = LOWORD(dwFldPos);*/
            GetFieldXPos( lpCnt, lpCol, &xStrCol, &xEndCol );
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
        if( !(lpCnt->lpCI->flCntAttr & CA_HSCROLLBYCHAR) )
          {
          if( lpCnt->lpCI->flCntAttr & CA_WIDEFOCUSRECT )
            InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 1 );
          }

        UpdateWindow( hWnd );
        }
      }

    // Notify the assoc of the scrolling event if there was movement.
    if( nHscrollInc )
      NotifyAssocWnd( hWnd, lpCnt, wScrollEvent, 0, NULL, NULL, nHscrollInc, 0, 0, NULL );

    return;
    }

VOID Dtl_OnChar( HWND hWnd, UINT ch, int cRepeat )
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
    if( !IsFocusInView( hWnd, lpCnt ) )
      ScrollToFocus( hWnd, lpCnt, 0 );

    // If selection is active do special handling of spacebar hit.
    if( ch == VK_SPACE )
      {
      // If in single select ignore spacebars hits.
/*      if( lpCnt->dwStyle & CTS_SINGLESEL )*/
/*        return;*/

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

      // If in block select the spacebar clears the selection list
      // list if the SHIFT key is not pressed. If it is pressed it toggles
      // the selection status of the focus record.
      if( (lpCnt->dwStyle & CTS_BLOCKSEL) && lpCnt->lpCI->lpFocusRec && lpCnt->lpCI->lpFocusFld )
        {
        // If the shift key is not pressed clear the selection list.
        // and then select the focus cell.
        if( !bShiftKey )
          {
          ClearBlkSelectList( hWnd, lpCnt );
          CntSelectFld( hWnd, lpCnt->lpCI->lpFocusRec, lpCnt->lpCI->lpFocusFld );
          }
        else
          {
          // Toggle the selection state of this field.
          if( CntIsFldSelected( hWnd, lpCnt->lpCI->lpFocusRec, lpCnt->lpCI->lpFocusFld ) )
            CntUnSelectFld( hWnd, lpCnt->lpCI->lpFocusRec, lpCnt->lpCI->lpFocusFld );
          else
            CntSelectFld( hWnd, lpCnt->lpCI->lpFocusRec, lpCnt->lpCI->lpFocusFld );
          }

        // Don't pass this space bar hit as a CHARHIT notification.
        return;
        }
      }

#ifdef ZOT
    // Notify the parent if the focus cell is editable.
    if( lpCnt->lpCI->lpFocusRec && lpCnt->lpCI->lpFocusFld )
      {
      if( !(lpCnt->dwStyle & CTS_READONLY) &&
          !(lpCnt->lpCI->lpFocusFld->flColAttr & CFA_FLDREADONLY) &&
          !(lpCnt->lpCI->lpFocusRec->flRecAttr & CRA_RECREADONLY) )
        {
        NotifyAssocWnd( hWnd, lpCnt, CN_CHARHIT, ch, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
        }
      }
#endif

    // Notify the parent of the char hit.
    NotifyAssocWnd( hWnd, lpCnt, CN_CHARHIT, ch, lpCnt->lpCI->lpFocusRec, lpCnt->lpCI->lpFocusFld, 0, bShiftKey, bCtrlKey, NULL );

    return;
    }

VOID Dtl_OnKey( HWND hWnd, UINT vk, BOOL fDown, int cRepeat, UINT flags )
    {
    LPCONTAINER  lpCnt = GETCNT(hWnd);
    LPCONTAINER  lpCntFrame;
    LPFIELDINFO  lpCol, lpOldFocusFld;
    LPRECORDCORE lpOldFocusRec, lpRec2;
    BOOL        bShiftKey, bCtrlKey, bRecSelected, bFound, bSel;
    int         xStrCol, xEndCol;
    int         nInc, nRecPos, nDir, nRecs;
    int         cyRecordArea, nFullRecsDisplayed;

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
        if( !IsFocusInView( hWnd, lpCnt ) )
          ScrollToFocus( hWnd, lpCnt, vk == VK_PRIOR ? 0 : 1 );

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
            if( !bShiftKey || lpCnt->lDelta || (lpCnt->lpCI->flCntAttr & CA_OWNERVSCROLL) )
            {
              ClearExtSelectList( hWnd, lpCnt );
              lpCnt->lpRecAnchor = NULL;
            }
          }
          else if( lpCnt->dwStyle & CTS_BLOCKSEL )
            {
            // Clear the selection list if shift key not pressed.
            if( !bShiftKey || lpCnt->lDelta || (lpCnt->lpCI->flCntAttr & CA_OWNERVSCROLL) )
              ClearBlkSelectList( hWnd, lpCnt );
            }

          lpOldFocusRec = lpCnt->lpCI->lpFocusRec;
          lpOldFocusFld = lpCnt->lpCI->lpFocusFld;

          // Unfocus the old focus record.
          if( lpCnt->lpCI->lpFocusRec && lpCnt->lpCI->lpFocusFld )
            {
            lpCnt->lpCI->lpFocusRec->flRecAttr &= ~CRA_CURSORED;
            if( lpCnt->lpCI->flCntAttr & CA_WIDEFOCUSRECT )
              InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
            else
              InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec,
                                         lpCnt->lpCI->lpFocusFld, 2 );
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
              FORWARD_WM_VSCROLL( hWnd, 0, SB_TOP, 0, SendMessage );
            else if( vk == VK_END )
              FORWARD_WM_VSCROLL( hWnd, 0, SB_BOTTOM, 0, SendMessage );
            else if( vk == VK_PRIOR )
              FORWARD_WM_VSCROLL( hWnd, 0, SB_PAGEUP, 0, SendMessage );
            else
              FORWARD_WM_VSCROLL( hWnd, 0, SB_PAGEDOWN, 0, SendMessage );

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
              FORWARD_WM_VSCROLL( hWnd, 0, SB_TOP, 0, SendMessage );
            else if( vk == VK_END )
              FORWARD_WM_VSCROLL( hWnd, 0, SB_BOTTOM, 0, SendMessage );
            else if( vk == VK_PRIOR )
              FORWARD_WM_VSCROLL( hWnd, 0, SB_PAGEUP, 0, SendMessage );
            else
              FORWARD_WM_VSCROLL( hWnd, 0, SB_PAGEDOWN, 0, SendMessage );

            if( lpCnt->lpCI->lpFocusRec && lpCnt->lpCI->lpFocusFld )
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
                cyRecordArea = lpCnt->cyClient - lpCnt->lpCI->cyTitleArea - lpCnt->lpCI->cyColTitleArea + 1;
                nFullRecsDisplayed = cyRecordArea / lpCnt->lpCI->cyRow;

                // If we are at the max scroll position (or there is no scroll
                // bar) use the number of records displayed for the calculation.
                // This insures that we can move the focus to the last record
                // even if it is not fully in view.
                if( lpCnt->nVscrollPos == lpCnt->nVscrollMax )
                  nRecs = lpCnt->lpCI->nRecsDisplayed;
                else
                  nRecs = nFullRecsDisplayed;

                lpCnt->lpCI->lpFocusRec = CntScrollRecList( hWnd, lpCnt->lpCI->lpTopRec, nRecs-1 );
                }
              else if( vk == VK_END )
                {
                lpCnt->lpCI->lpFocusRec = CntScrollRecList( hWnd, lpCnt->lpCI->lpTopRec, lpCnt->lpCI->nRecsDisplayed-1 );
                }
              else
                {
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
                if( !bShiftKey || lpCnt->lDelta || (lpCnt->lpCI->flCntAttr & CA_OWNERVSCROLL) )
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
                  //BatchSelect( hWnd, lpCnt, lpOldFocusRec, lpCnt->lpCI->lpFocusRec, nDir );

                  // see if recanchor is before or after the old focus rec to determine
                  // whether we are selecting or deselecting
                  lpRec2 = lpOldFocusRec;
                  bFound = FALSE;
                  if (lpRec2==lpCnt->lpRecAnchor)
                    bSel = FALSE;
                  else
                  {
                    while( lpRec2 )
                    {
                      if( lpRec2 == lpCnt->lpRecAnchor )
                      {
                        bFound = TRUE;
                        break;
                      }
                      lpRec2 = lpRec2->lpPrev;
                    }
                    if ( (bFound && (vk==VK_PRIOR || vk==VK_HOME)) || (!bFound && (vk==VK_NEXT || vk==VK_END)) )
                      bSel = FALSE;
                    else
                      bSel = TRUE;
                  }

                  // Select or unselect the records from old focus to new record clicked on
                  lpRec2 = lpOldFocusRec;
                  while( lpRec2 )
                  {
                    if (lpRec2==lpCnt->lpRecAnchor)
                    {
                      // toggle the selection state
                      bSel = !bSel;
                    }
                    else
                    {
                      // select records.
                      if( !(lpRec2->flRecAttr & CRA_SELECTED) && bSel)
                      {
                        // Repaint the selected record and notify the parent.
                        lpRec2->flRecAttr |= CRA_SELECTED;
                        InvalidateCntRecord( hWnd, lpCnt, lpRec2, NULL, 1 );
                        NotifyAssocWnd( hWnd, lpCnt, CN_RECSELECTED, 0, lpRec2, NULL, 0, 0, 0, NULL );
                      }
                      else 
                      {
                        if (!bSel && lpRec2 != lpCnt->lpCI->lpFocusRec)
                        {
                          lpRec2->flRecAttr &= ~CRA_SELECTED;
                          NotifyAssocWnd( hWnd, lpCnt, CN_RECUNSELECTED, 0, lpRec2, NULL, 0, 0, 0, NULL );
                        }
                        InvalidateCntRecord( hWnd, lpCnt, lpRec2, NULL, 1 );
                      }
                    }

                    // Break out after we have done the end record.
                    if( lpRec2 == lpCnt->lpCI->lpFocusRec )
                      break;

                    // Get the next or previous record.
                    if( nDir == 1 )
                      lpRec2 = CntNextRec( lpRec2 );
                    else
                      lpRec2 = CntPrevRec( lpRec2 );
                  }
                }
              }
              else if( lpCnt->dwStyle & CTS_BLOCKSEL )
                {
                if( !bShiftKey || lpCnt->lDelta || (lpCnt->lpCI->flCntAttr & CA_OWNERVSCROLL) )
                  {
                  // The selection moves with the focus record.
                  CntSelectFld( hWnd, lpCnt->lpCI->lpFocusRec, lpCnt->lpCI->lpFocusFld );
                  }
                else
                  {
                  if( vk == VK_PRIOR || vk == VK_HOME )
                    nDir = -1;
                  else
                    nDir = 1;
                  BatchSelect( hWnd, lpCnt, lpOldFocusRec, lpCnt->lpCI->lpFocusRec, nDir );
                  }
                }
              else
                {
                // Repaint only the new focus cell.
                if( lpCnt->lpCI->flCntAttr & CA_WIDEFOCUSRECT )
                  InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
                else
                  InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec,
                                                    lpCnt->lpCI->lpFocusFld, 2 );
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
        if( !IsFocusInView( hWnd, lpCnt ) )
          ScrollToFocus( hWnd, lpCnt, 0 );

        if( lpCntFrame->bScrollFocusRect && lpCnt->lpCI->lpFocusRec && lpCnt->lpCI->lpFocusFld )
          {
          if( !lpCnt->lpCI->bFocusRecLocked )
            {
            // Get the position of the focus in the display area.
            // Remember that with mode=0 nothing happens in this call.
            nRecPos = InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec,
                                           lpCnt->lpCI->lpFocusFld, 0 );

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
                  lpRec2 = lpCnt->lpCI->lpFocusRec;
                  bFound = FALSE;
                  while( lpRec2 )
                  {
                    if( lpRec2 == lpCnt->lpRecAnchor )
                    {
                      bFound = TRUE;
                      break;
                    }
                    lpRec2 = lpRec2->lpNext;
                  }
                  if (!bFound)
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
            else if( lpCnt->dwStyle & CTS_BLOCKSEL )
              {
              // Start a new selection list if shift key not pressed.
              if( !bShiftKey )
                {
                ClearBlkSelectList( hWnd, lpCnt );
                }
              }

            // Unfocus the old focus record.
            lpCnt->lpCI->lpFocusRec->flRecAttr &= ~CRA_CURSORED;
            if( lpCnt->lpCI->flCntAttr & CA_WIDEFOCUSRECT )
              InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
            else
              InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec,
                                                  lpCnt->lpCI->lpFocusFld, 2 );

            if( nRecPos <= 0 && (lpCnt->dwStyle & CTS_ASYNCNOTIFY) && 
                        (lpCnt->lpCI->flCntAttr & CA_OWNERVSCROLL) )
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
              else
                {
                // Focus rect moved out of the display area so we will do a
                // line up scroll and then make the top record the focus.
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
              else if( lpCnt->dwStyle & CTS_BLOCKSEL )
                {
                // The selection moves with the focus record.
                if( !CntIsFldSelected( hWnd, lpCnt->lpCI->lpFocusRec, lpCnt->lpCI->lpFocusFld ) )
                  CntSelectFld( hWnd, lpCnt->lpCI->lpFocusRec, lpCnt->lpCI->lpFocusFld );
                else
                  InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec,
                                                    lpCnt->lpCI->lpFocusFld, 2 );
                }
              else
                {
                if( lpCnt->lpCI->flCntAttr & CA_WIDEFOCUSRECT )
                  InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
                else
                  InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec,
                                                    lpCnt->lpCI->lpFocusFld, 2 );
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
          FORWARD_WM_VSCROLL( hWnd, 0, SB_LINEUP, 0, SendMessage );
          }
        break;

      case VK_DOWN:
        if( bCtrlKey )  
          {
          FORWARD_WM_KEYDOWN( hWnd, VK_END, 0, 0, SendMessage );
          break;
          }

        bRecSelected   = FALSE;

        // If focus cell is not in view, scroll to it. This should
        // put the focus cell in view 1 row up from the bottom.
        if( !IsFocusInView( hWnd, lpCnt ) )
          ScrollToFocus( hWnd, lpCnt, 1 );

        if( lpCntFrame->bScrollFocusRect && lpCnt->lpCI->lpFocusRec && lpCnt->lpCI->lpFocusFld )
          {
          if( !lpCnt->lpCI->bFocusRecLocked )
            {
            // Get the position of the focus in the display area.
            // Remember that with mode=0 nothing happens in this call.
            nRecPos = InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec,
                                           lpCnt->lpCI->lpFocusFld, 0 );

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
                  lpRec2 = lpCnt->lpCI->lpFocusRec;
                  bFound = FALSE;
                  while( lpRec2 )
                  {
                    if( lpRec2 == lpCnt->lpRecAnchor )
                    {
                      bFound = TRUE;
                      break;
                    }
                    lpRec2 = lpRec2->lpPrev;
                  }
                  if (!bFound)
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
            else if( lpCnt->dwStyle & CTS_BLOCKSEL )
            {
              // Start a new selection list if shift key not pressed.
              if( !bShiftKey )
              {
                ClearBlkSelectList( hWnd, lpCnt );
              }
            }

            // Unfocus the old focus record.
            lpCnt->lpCI->lpFocusRec->flRecAttr &= ~CRA_CURSORED;
            if( lpCnt->lpCI->flCntAttr & CA_WIDEFOCUSRECT )
              InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
            else
              InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec,
                                                lpCnt->lpCI->lpFocusFld, 2 );

            if( !(nRecPos >= 0 && nRecPos < lpCnt->lpCI->nRecsDisplayed - 1 ) &&
                (lpCnt->dwStyle & CTS_ASYNCNOTIFY) && (lpCnt->lpCI->flCntAttr & CA_OWNERVSCROLL) )
              {
              // When the focus cell moves out of the display area and
              // it is necessary to scroll before moving the focus there
              // is a special case when ASYNC and OWNERVSCROLL are on.
              // We cannot be assured that scroll has actually been 
              // executed when the SendMessage returns. Therefore we will
              // simply tell the assoc to move the focus after the scroll.
              FORWARD_WM_VSCROLL( hWnd, 0, SB_LINEDOWN, 0, SendMessage );
              NotifyAssocWnd( hWnd, lpCnt, CN_OWNERSETFOCUSBOT, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL );
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
              if( lpCnt->nVscrollPos == lpCnt->nVscrollMax )
                nRecs = lpCnt->lpCI->nRecsDisplayed;
              else
                nRecs = nFullRecsDisplayed;

              if( nRecPos >= 0 && nRecPos < nRecs - 1 )
                {
                // Focus rect is in display area but will not hit the bottom
                // yet. No scroll is necessary. Focus rect just moves down.
                lpCnt->lpCI->lpFocusRec = CntScrollRecList( hWnd, lpCnt->lpCI->lpFocusRec, 1 );
                }
              else
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
              else if( lpCnt->dwStyle & CTS_BLOCKSEL )
              {
                // The selection moves with the focus record.
                if( !CntIsFldSelected( hWnd, lpCnt->lpCI->lpFocusRec, lpCnt->lpCI->lpFocusFld ) )
                  CntSelectFld( hWnd, lpCnt->lpCI->lpFocusRec, lpCnt->lpCI->lpFocusFld );
                else
                  InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec,
                                                    lpCnt->lpCI->lpFocusFld, 2 );
              }
              else
              {
                if( lpCnt->lpCI->flCntAttr & CA_WIDEFOCUSRECT )
                  InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
                else
                  InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec,
                                                    lpCnt->lpCI->lpFocusFld, 2 );
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
          FORWARD_WM_VSCROLL( hWnd, 0, SB_LINEDOWN, 0, SendMessage );
          }
        break;

      case VK_LEFT:
        // If focus cell is not in view, scroll to it. This should
        // put the focus cell in view 1 row down from the top.
        if( !IsFocusInView( hWnd, lpCnt ) )
          ScrollToFocus( hWnd, lpCnt, 0 );

/*        if( lpCntFrame->bScrollFocusRect && lpCnt->lpCI->lpFocusRec && lpCnt->lpCI->lpFocusFld )*/
        if( lpCntFrame->bScrollFocusRect && lpCnt->lpCI->lpFocusRec && lpCnt->lpCI->lpFocusFld && !lpCnt->bWideFocusRect )
          {
          if( !lpCnt->lpCI->bFocusFldLocked )
            {
            // For Block select start a new selection list if shift key not pressed.
            if( lpCnt->dwStyle & CTS_BLOCKSEL )
              {
              if( !bShiftKey )
                {
                ClearBlkSelectList( hWnd, lpCnt );
                }
              }

            lpOldFocusFld = lpCnt->lpCI->lpFocusFld;

            // Unfocus the old focus field and get its position.
            lpCnt->lpCI->lpFocusFld->flColAttr &= ~CFA_CURSORED;
            nRecPos = InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec,
                                        lpCnt->lpCI->lpFocusFld, 2 );

            // Get a pointer to the head or the previous field.
            if( bCtrlKey )  
              lpCnt->lpCI->lpFocusFld = CntFldHeadGet( hWnd );
            else
              lpCnt->lpCI->lpFocusFld = CntScrollFldList( hWnd, lpCnt->lpCI->lpFocusFld, -1 );

            // If field is hidden find the 1st non-hidden before it.
            if( !lpCnt->lpCI->lpFocusFld->cxWidth )
              {
              lpCol = lpCnt->lpCI->lpFocusFld;
              // break out when we find a col with some width
              while( lpCol->lpPrev )
                {
                lpCol = lpCol->lpPrev;
                if( lpCol->cxWidth )
                  break;  
                }

              // If no width found before it look after it.
              if( !lpCol->cxWidth )
                {                 
                while( lpCol->lpNext )
                 {
                  lpCol = lpCol->lpNext;
                  if( lpCol->cxWidth )
                    break;  
                  }
                }

              // Set the focus to a field which is not hidden.
              if( lpCol->cxWidth )
                lpCnt->lpCI->lpFocusFld = lpCol;
              }

            // Get the starting xcoord of the new focus field.
            GetFieldXPos( lpCnt, lpCnt->lpCI->lpFocusFld, &xStrCol, &xEndCol );

            if( xStrCol < 0 || xStrCol > lpCnt->cxClient )
              {
              // Focus rect was not in display area or it is now moving
              // out of the display area so we will do a scroll.
              nInc = xStrCol / lpCnt->lpCI->cxChar;
              nInc += lpCnt->nHscrollPos;
              FORWARD_WM_HSCROLL( hWnd, 0, SB_THUMBPOSITION, nInc, SendMessage );
              }

            // Now repaint the new focus field.
            lpCnt->lpCI->lpFocusFld->flColAttr |= CFA_CURSORED;

            if( lpCnt->dwStyle & CTS_BLOCKSEL )
              {
              if( !bShiftKey )
                {
                // The selection moves with the focus record.
                if( !CntIsFldSelected( hWnd, lpCnt->lpCI->lpFocusRec, lpCnt->lpCI->lpFocusFld ) )
                  CntSelectFld( hWnd, lpCnt->lpCI->lpFocusRec, lpCnt->lpCI->lpFocusFld );
                else
                  InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec,
                                       lpCnt->lpCI->lpFocusFld, 2 );
                }
              else
                {
                BatchFldSelect( hWnd, lpCnt, lpOldFocusFld, lpCnt->lpCI->lpFocusFld, -1 );
                }
              }
            else
              {
              InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec,
                                         lpCnt->lpCI->lpFocusFld, 2 );
              }

            // Tell associate wnd about the keystroke and new focus field.
            NotifyAssocWnd( hWnd, lpCnt, CN_NEWFOCUSFLD, 0, NULL, lpCnt->lpCI->lpFocusFld, 0, bShiftKey, bCtrlKey, NULL );
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
        // If focus cell is not in view, scroll to it. This should
        // put the focus cell in view 1 row down from the top.
        if( !IsFocusInView( hWnd, lpCnt ) )
          ScrollToFocus( hWnd, lpCnt, 0 );

/*        if( lpCntFrame->bScrollFocusRect && lpCnt->lpCI->lpFocusRec && lpCnt->lpCI->lpFocusFld )*/
        if( lpCntFrame->bScrollFocusRect && lpCnt->lpCI->lpFocusRec && lpCnt->lpCI->lpFocusFld && !lpCnt->bWideFocusRect )
          {
          if( !lpCnt->lpCI->bFocusFldLocked )
            {
            // For Block select start a new selection list if shift key not pressed.
            if( lpCnt->dwStyle & CTS_BLOCKSEL )
              {
              if( !bShiftKey )
                {
                ClearBlkSelectList( hWnd, lpCnt );
                }
              }

            lpOldFocusFld = lpCnt->lpCI->lpFocusFld;

            // Unfocus the old focus record and get its position.
            lpCnt->lpCI->lpFocusFld->flColAttr &= ~CFA_CURSORED;
            nRecPos = InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec,
                                        lpCnt->lpCI->lpFocusFld, 2 );

            // Get a pointer to the tail or the next field.
            if( bCtrlKey )  
              lpCnt->lpCI->lpFocusFld = CntFldTailGet( hWnd );
            else
              lpCnt->lpCI->lpFocusFld = CntScrollFldList( hWnd, lpCnt->lpCI->lpFocusFld, 1 );

            // If field is hidden find the 1st non-hidden after it.
            if( !lpCnt->lpCI->lpFocusFld->cxWidth )
              {
              lpCol = lpCnt->lpCI->lpFocusFld;
              // break out when we find a col with some width
              while( lpCol->lpNext )
                {
                lpCol = lpCol->lpNext;
                if( lpCol->cxWidth )
                  break;  
                }

              // If no width found after it look before it.
              if( !lpCol->cxWidth )
                {                 
                while( lpCol->lpPrev )
                 {
                  lpCol = lpCol->lpPrev;
                  if( lpCol->cxWidth )
                    break;  
                  }
                }

              // Set the focus to a field which is not hidden.
              if( lpCol->cxWidth )
                lpCnt->lpCI->lpFocusFld = lpCol;
              }

            // Get the ending xcoord of the new focus field.
            GetFieldXPos( lpCnt, lpCnt->lpCI->lpFocusFld, &xStrCol, &xEndCol );

            if( xEndCol < 0 || xEndCol > lpCnt->cxClient )
              {
              // Focus rect was not in display area or it is now moving
              // out of the display area so we will do a scroll.
              nInc = (xEndCol - lpCnt->cxClient - 1) / lpCnt->lpCI->cxChar + 1;
              nInc += lpCnt->nHscrollPos;
              FORWARD_WM_HSCROLL( hWnd, 0, SB_THUMBPOSITION, nInc, SendMessage );
              }

            // Now repaint the new focus field.
            lpCnt->lpCI->lpFocusFld->flColAttr |= CFA_CURSORED;

            if( lpCnt->dwStyle & CTS_BLOCKSEL )
              {
              if( !bShiftKey )
                {
                // The selection moves with the focus record.
                if( !CntIsFldSelected( hWnd, lpCnt->lpCI->lpFocusRec, lpCnt->lpCI->lpFocusFld ) )
                  CntSelectFld( hWnd, lpCnt->lpCI->lpFocusRec, lpCnt->lpCI->lpFocusFld );
                else
                  InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec,
                                       lpCnt->lpCI->lpFocusFld, 2 );
                }
              else
                {
                BatchFldSelect( hWnd, lpCnt, lpOldFocusFld, lpCnt->lpCI->lpFocusFld, 1 );
                }
              }
            else
              {
              InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec,
                                         lpCnt->lpCI->lpFocusFld, 2 );
              }

            // Tell associate wnd about the keystroke and new focus field.
            NotifyAssocWnd( hWnd, lpCnt, CN_NEWFOCUSFLD, 0, NULL, lpCnt->lpCI->lpFocusFld, 0, bShiftKey, bCtrlKey, NULL );
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
        if( !IsFocusInView( hWnd, lpCnt ) )
          ScrollToFocus( hWnd, lpCnt, 0 );

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
            if( !IsFocusInView( hWnd, lpCnt ) )
              ScrollToFocus( hWnd, lpCnt, 0 );

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
            if( !IsFocusInView( hWnd, lpCnt ) )
              ScrollToFocus( hWnd, lpCnt, 0 );

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
            if( !lpCnt->bWideFocusRect )
              lpCnt->lpCI->flCntAttr &= ~CA_WIDEFOCUSRECT;
            lpCnt->dwStyle &= ~CTS_MULTIPLESEL;
            lpCnt->dwStyle |=  CTS_EXTENDEDSEL;
            lpCnt->bSimulateMulSel = FALSE;  
            }
          else
            {
            lpCnt->dwStyle &= ~CTS_EXTENDEDSEL;
            lpCnt->dwStyle |=  CTS_MULTIPLESEL;
            lpCnt->lpCI->flCntAttr |= CA_WIDEFOCUSRECT;
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

VOID Dtl_OnRButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);
    POINT      pt;

    // If mouse input is disabled just beep and return.
    if( !lpCnt->lpCI->bSendMouseEvents )
      {
      MessageBeep(0);
      return;
      }

    // User wants to create a split bar.
    if( fDoubleClick )
      {
      if( lpCnt->dwStyle & CTS_SPLITBAR && lpCnt->hWndFrame )
        {
        if( !(lpCnt->lpCI->flCntAttr & CA_APPSPLITABLE) )
          {
          pt.x = x;
          pt.y = y;
          ClientToScreen( hWnd, &pt );
          ScreenToClient( lpCnt->hWndFrame, &pt );
          SplitBarManager( lpCnt->hWndFrame, lpCnt, SPLITBAR_CREATE, &pt );
          }
        }
      }
    else
      {
      pt.x = x;
      pt.y = y;
      NotifyAssocWnd( hWnd, lpCnt, CN_RBTNCLK, 0, NULL, NULL, 0, 0, 0, (LPVOID)&pt );
      }

    return;
    }

VOID Dtl_OnLButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags )
    {
    LPCONTAINER   lpCnt = GETCNT(hWnd);
    LPFIELDINFO  lpCol;
    LPFIELDINFO  lpCol2;
    LPRECORDCORE lpRec, lpOldSelRec;
    LPRECORDCORE lpRec2;
    RECT         rect, rectTitle, rectColTitle, rectRecs;
    RECT         rectTitleBtn, rectColTitleBtn;
    POINT        pt, ptDev;
    BOOL         bShiftKey, bCtrlKey;
    BOOL         bRecSelected, bRecUnSelected;
    BOOL         bClickedTitle, bClickedColTitle, bClickedRecord;
    BOOL         bFldSelected, bFldUnSelected;
    BOOL         bNewFldList,  bNewRecList;
    BOOL         bNewFocusFld, bNewFocusRec, bNewRecSelected;
    BOOL         bClickedTitleBtn;
    int          xStrCol, xEndCol, i;
    int          cxPxlWidth;
    int          xStartSizing;
    int          xLeft, xRight;
    int          nRec, nSelRecs, nSelFlds;

    // set the message.  This can be queried by the user to see if messages sent were
    // sent by a left mouse button click (as opposed to a keydown msg).
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

    bClickedTitle = bClickedColTitle = bClickedRecord = bClickedTitleBtn = FALSE;

    // Get the right xCoord of the last field.
/*    xLastCol = lpCnt->lpCI->nMaxWidth - lpCnt->nHscrollPos * lpCnt->lpCI->cxChar;*/

    // Get the bottom yCoord of the last visible record.
    lpCnt->yLastRec = lpCnt->lpCI->cyTitleArea + lpCnt->lpCI->cyColTitleArea;
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
    SetRectEmpty( &rectColTitle );
    SetRectEmpty( &rectColTitleBtn );
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

    // Setup the column titles rect if there are any.
    if( lpCnt->lpCI->cyColTitleArea )
      {
      CopyRect( &rectColTitle, &rect );
      rectColTitle.right  = min(rect.right, lpCnt->xLastCol);
      rectColTitle.top    = rectTitle.bottom;
      rectColTitle.bottom = rectColTitle.top + lpCnt->lpCI->cyColTitleArea;
      }

    // Setup the record data rect.
    CopyRect( &rectRecs, &rect );
    rectRecs.right = min(rect.right, lpCnt->xLastCol);
    if( lpCnt->lpCI->cyColTitleArea )
      rectRecs.top = rectColTitle.bottom;
    else
      rectRecs.top = rectTitle.bottom;

    // Set the flag for the object the user clicked on.
    if( PtInRect(&rectTitle, pt) )
      bClickedTitle = TRUE;
    else if( PtInRect(&rectTitleBtn, pt) )
      bClickedTitleBtn = TRUE;
    else if( PtInRect(&rectColTitle, pt) )
      bClickedColTitle = TRUE;
    else if( PtInRect(&rectRecs, pt) )
      bClickedRecord = TRUE;

    // If user clicked in a column find out which one it was.
    if( bClickedColTitle || bClickedRecord )
      {
      lpCol = GetFieldAtPt( lpCnt, &pt, &xStrCol, &xEndCol );
      if( !lpCol )   // this shouldn't ever happen, but...
        return;  
      }

    // If user clicked in a record find out which one it was.
    if( bClickedRecord )
      {
      // Calc which record the user clicked on.
      nRec = (pt.y - rectRecs.top) / lpCnt->lpCI->cyRow;
      
      // save the rect of this record/field cell.
      lpCnt->rectCursorFld.top    = rectRecs.top + (nRec * lpCnt->lpCI->cyRow);
      lpCnt->rectCursorFld.bottom = lpCnt->rectCursorFld.top + lpCnt->lpCI->cyRow;
      if( lpCnt->dwStyle & CTS_BLOCKSEL )
        {
        lpCnt->rectCursorFld.left  = xStrCol;
        lpCnt->rectCursorFld.right = xEndCol;
        }
      else
        {
        lpCnt->rectCursorFld.left  = rectRecs.left;
        lpCnt->rectCursorFld.right = rectRecs.right;
        }

      // Save the record rect so we can track mouse movement against it.
      CopyRect( &lpCnt->rectRecArea, &rectRecs );
      lpCnt->nCurRec = nRec;

      // Scroll the record list to the clicked-on record.
      if( lpCnt->lpCI->lpTopRec )
        lpRec = CntScrollRecList( hWnd, lpCnt->lpCI->lpTopRec, nRec );
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
/*          MessageBox( hWnd, lpCnt->lpCI->lpszTitle, "Title Edit", MB_OK );*/
/*          SetFocus( hWnd );*/
          }
        else
          {
          NotifyAssocWnd( hWnd, lpCnt, CN_ROTTLDBLCLK, 0, NULL, NULL, 0, 0, 0, NULL );
          }
        }

      if( bClickedColTitle )
        {
        if( !(lpCnt->dwStyle & CTS_READONLY) &&
            !(lpCol->flColAttr & CFA_FLDTTLREADONLY) )
          {
          // Let the user edit the field title.
          NotifyAssocWnd( hWnd, lpCnt, CN_BEGFLDTTLEDIT, 0, NULL, lpCol, 0, 0, 0, NULL );
/*          MessageBox( hWnd, lpCol->lpFTitle, "Field Title Edit", MB_OK );*/
/*          SetFocus( hWnd );*/
          }
        else
          {
          NotifyAssocWnd( hWnd, lpCnt, CN_ROFLDTTLDBLCLK, 0, NULL, lpCol, 0, 0, 0, NULL );
          }
        }

      if( bClickedRecord )
        {
        if( !(lpCnt->dwStyle & CTS_READONLY) &&
            !(lpRec->flRecAttr & CRA_RECREADONLY) &&
            !(lpCol->flColAttr & CFA_FLDREADONLY) )
          {
/*          char szString[80];*/

          // Let the user edit the field.
          NotifyAssocWnd( hWnd, lpCnt, CN_BEGFLDEDIT, 0, lpRec, lpCol, 0, 0, 0, NULL );
/*          wsprintf( szString, "Field: %s\nRecord: %s", */
/*                    (LPSTR)lpCol->lpFTitle,*/
/*                    (LPSTR)lpRec->lpRecData );*/
/*          MessageBox( hWnd, szString, "Record Edit", MB_OK );*/
/*          SetFocus( hWnd );*/
          }
        else
          {
          NotifyAssocWnd( hWnd, lpCnt, CN_ROFLDDBLCLK, 0, lpRec, lpCol, 0, 0, 0, NULL );
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
      else if( bClickedColTitle )
        {
        if( PtInFldSizeArea( lpCnt, lpCol, &pt ) )
          {
          // Get the field which is to be resized.
          lpCnt->lpSizMovFld = GetFldToBeSized( lpCnt, lpCol, &pt, &xStartSizing );
          lpCnt->xStartPos = xStartSizing-1;

          // Set the cursor right over the field border.
          ptDev.x = xStartSizing-1;
          ptDev.y = pt.y;
          ClientToScreen( hWnd, &ptDev );
          SetCursorPos( ptDev.x, ptDev.y );

          // Set the state to indicate we pressed a field sizing area.
          StateSet( lpCnt, CNTSTATE_SIZINGFLD );
          SetCapture( hWnd );
          }
        else if( PtInFldTtlBtn( lpCnt, lpCol, &pt, &xLeft, &xRight ) )
          {
          CopyRect( &rectColTitleBtn, &rectColTitle );

          // Setup the boundaries of the field title button.
          rectColTitleBtn.left  = xLeft;
          rectColTitleBtn.right = xRight;

          // set the state to indicate we pressed a field title button
          StateSet( lpCnt, CNTSTATE_CLKEDFLDBTN );
          lpCol->flColAttr |= CFA_FLDBTNPRESSED;
          lpCnt->lpPressedFld = lpCol;
          CopyRect( &lpCnt->rectPressedBtn, &rectColTitleBtn );
          SetCapture( hWnd );
          UpdateContainer( hWnd, &lpCnt->rectPressedBtn, TRUE );
          }
        else
          {
          if( IsFldMoveable( lpCnt, lpCol ) )
            {
            // Save the field which is to be moved.
            lpCnt->lpSizMovFld = lpCol;
            InitMovingRect( hWnd, lpCnt, lpCol, &pt );

            // Set the state to indicate we pressed in field moving area.
            StateSet( lpCnt, CNTSTATE_MOVINGFLD );
            SetCursor( lpCnt->lpCI->hCurFldMoving );
            SetCapture( hWnd );
            }
          }
        }
      else if( bClickedRecord && lpRec )
        {
        // End special mode for extended select - simulate multiple select.
        if( (lpCnt->dwStyle & CTS_MULTIPLESEL) && lpCnt->bSimulateMulSel )
          {
          if( !lpCnt->bWideFocusRect )
            lpCnt->lpCI->flCntAttr &= ~CA_WIDEFOCUSRECT;
          lpCnt->dwStyle &= ~CTS_MULTIPLESEL;
          lpCnt->dwStyle |=  CTS_EXTENDEDSEL;
          lpCnt->bSimulateMulSel = FALSE;  
          InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
          }

        // Set flags indicating if the focus field or record is changing.
        bNewFocusFld = lpCnt->lpCI->lpFocusFld != lpCol ? TRUE : FALSE;
        bNewFocusRec = lpCnt->lpCI->lpFocusRec != lpRec ? TRUE : FALSE;

        // If the record is locked and the user has clicked to a new
        // record or if the field is locked and the user has clicked to
        // a new field (or both of the above) we do not let them.
        // We just notify the associate and let it take care of it.
        if( (lpCnt->lpCI->bFocusRecLocked && bNewFocusRec) ||
            (lpCnt->lpCI->bFocusFldLocked && bNewFocusFld) )
          {
          if( bNewFocusRec && bNewFocusFld )
            {
            NotifyAssocWnd( hWnd, lpCnt, CN_LK_NEWFOCUS, 0, lpRec, lpCol, 0, 0, 0, NULL );
            }
          else
            {
            if( bNewFocusRec )
              NotifyAssocWnd( hWnd, lpCnt, CN_LK_NEWFOCUSREC, 0, lpRec, lpCol, 0, 0, 0, NULL );
            if( bNewFocusFld )
              NotifyAssocWnd( hWnd, lpCnt, CN_LK_NEWFOCUSFLD, 0, lpRec, lpCol, 0, 0, 0, NULL );
            }
          }
        else
          {
          bRecSelected = bRecUnSelected = bNewRecList = FALSE;
          bFldSelected = bFldUnSelected = bNewFldList = FALSE;

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
            NotifyAssocWnd( hWnd, lpCnt, CN_BGNRECSELECTION, 0, lpRec, NULL, 0, 0, 0, NULL );

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
            }
            else
              {
              // Track the user's record selection with the mouse.
              TrackSelection( hWnd, lpCnt, lpRec, lpCol, &pt, &nSelRecs, &nSelFlds, FALSE );
              }

            // Now do the actual record selection. 
            ExtSelectRec( hWnd, lpCnt, lpRec, nSelRecs, &lpRec2, bShiftKey );

            // The new focus rec will be the last one selected unless the
            // user was using the shift key. If he used the shift key the
            // new focus will be the record he clicked on.
            if( !bShiftKey && lpRec2 )
              lpRec = lpRec2;

            // Tell the assoc wnd we are done with the record selection.
            NotifyAssocWnd( hWnd, lpCnt, CN_ENDRECSELECTION, 0, lpRec2, NULL, 0, 0, 0, NULL );
            }
          else if( lpCnt->dwStyle & CTS_BLOCKSEL )
            {
            // Get the state of the shift and control keys.
            bShiftKey = GetAsyncKeyState( VK_SHIFT ) & ASYNC_KEY_DOWN;
            bCtrlKey  = GetAsyncKeyState( VK_CONTROL ) & ASYNC_KEY_DOWN;

            // If the control key is not pressed start a new selection list.
            if( !bCtrlKey )
              {
              ClearBlkSelectList( hWnd, lpCnt );

              // Force an update of the clicked-on field.
              InvalidateCntRecord( hWnd, lpCnt, lpRec, lpCol, 2 );
              }

            // Tell the assoc wnd we are starting a field selection.
            // They should not swap records out of the record list 
            // until done with the selection.
            NotifyAssocWnd( hWnd, lpCnt, CN_BGNFLDSELECTION, 0, NULL, NULL, 0, 0, 0, NULL );

            if( bShiftKey )
              {
              // Find where the focus cell is and select all the fields
              // between the clicked on field and the focus cell.
              nSelRecs = FindFocusRec( hWnd, lpCnt, lpRec );
              nSelFlds = FindFocusFld( hWnd, lpCnt, lpCol );

              // FindFocusRec/Fld return 0-based distances.
              nSelRecs = nSelRecs < 0 ? --nSelRecs : ++nSelRecs;
              nSelFlds = nSelFlds < 0 ? --nSelFlds : ++nSelFlds;
              }
            else
              {
              // Track the user's field selection with the mouse.
              TrackSelection( hWnd, lpCnt, lpRec, lpCol, &pt, &nSelRecs, &nSelFlds, TRUE );
              }

            // Now do the actual field selection.
            BlkSelectFld( hWnd, lpCnt, lpRec, lpCol, nSelRecs, nSelFlds, &lpRec2, &lpCol2 );

            // The new focus cell will be the last one selected unless
            // the user was using the shift key. If he used the shift
            // key the new focus will be the cell he clicked on.
            if( !bShiftKey )
              {
              if( lpRec2 )
                lpRec = lpRec2;
              if( lpCol2 )
                lpCol = lpCol2;
              }

            // Tell the assoc wnd we are done with the field selection.
            NotifyAssocWnd( hWnd, lpCnt, CN_ENDFLDSELECTION, 0, NULL, NULL, 0, 0, 0, NULL );
            }

          // Unfocus the previously focused record/field and repaint it.
          if( lpCnt->lpCI->lpFocusRec )
            lpCnt->lpCI->lpFocusRec->flRecAttr &= ~CRA_CURSORED;
          if( lpCnt->lpCI->lpFocusFld )
            lpCnt->lpCI->lpFocusFld->flColAttr &= ~CFA_CURSORED;
          if( lpCnt->lpCI->lpFocusRec && lpCnt->lpCI->lpFocusFld )
            {
            if( lpCnt->lpCI->flCntAttr & CA_WIDEFOCUSRECT )
              InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
            else
              InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec,
                                                lpCnt->lpCI->lpFocusFld, 2 );
            }

          // Remember if the field or record changed.
          bNewFocusFld = lpCnt->lpCI->lpFocusFld != lpCol ? TRUE : FALSE;
          bNewFocusRec = lpCnt->lpCI->lpFocusRec != lpRec ? TRUE : FALSE;

          // This is the new focus record/field.
          lpCnt->lpCI->lpFocusRec = lpRec;
          lpCnt->lpCI->lpFocusFld = lpCol;
          lpRec->flRecAttr |= CRA_CURSORED;
          lpCol->flColAttr |= CFA_CURSORED;

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
            {
            if( lpCnt->lpCI->flCntAttr & CA_WIDEFOCUSRECT )
              InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 2 );
            else
              InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, lpCnt->lpCI->lpFocusFld, 2 );
            }

          // Tell the assoc wnd if a record or field was selected.
          if( bRecSelected )
            NotifyAssocWnd( hWnd, lpCnt, CN_RECSELECTED, 0, lpRec, NULL, 0, 0, 0, NULL );
          if( bFldSelected )
            NotifyAssocWnd( hWnd, lpCnt, CN_FLDSELECTED, 0, lpRec, lpCol, 0, 0, 0, NULL );

          // Tell the assoc wnd if a record or field was unselected.
          if( bFldUnSelected )
            NotifyAssocWnd( hWnd, lpCnt, CN_FLDUNSELECTED, 0, lpRec, lpCol, 0, 0, 0, NULL );
          if( bRecUnSelected )
          {
            if (lpCnt->dwStyle & CTS_SINGLESEL)
              NotifyAssocWnd( hWnd, lpCnt, CN_RECUNSELECTED, 0, lpOldSelRec, NULL, 0, 0, 0, NULL );
            else // multiple select
              NotifyAssocWnd( hWnd, lpCnt, CN_RECUNSELECTED, 0, lpRec, NULL, 0, 0, 0, NULL );
          }

          // Notify the assoc wnd if the record or field focus changed.
          if( bNewFocusRec && bNewFocusFld )
            {
            NotifyAssocWnd( hWnd, lpCnt, CN_NEWFOCUS, 0, lpRec, lpCol, 0, 0, 0, NULL );
            }
          else
            {
            if( bNewFocusRec )
              NotifyAssocWnd( hWnd, lpCnt, CN_NEWFOCUSREC, 0, lpRec, lpCol, 0, 0, 0, NULL );
            if( bNewFocusFld )
              NotifyAssocWnd( hWnd, lpCnt, CN_NEWFOCUSFLD, 0, lpRec, lpCol, 0, 0, 0, NULL );
            }
          }
        }
      // now check to see if were in drag mode
      if( (!bClickedColTitle) && (!bClickedTitleBtn) && (!bClickedTitle) &&
         (FALSE /*lpCnt->dwStyle & CTS_DRAGDROP  || lpCnt->dwStyle & CTS_DRAGONLY*/))
          DoDragTracking(hWnd); 
      }

    // reset the message ID
    lpCnt->iInputID = -1;

    return;
    }

VOID Dtl_OnMouseMove( HWND hWnd, int x, int y, UINT keyFlags )
    {
    LPCONTAINER  lpCnt = GETCNT(hWnd);
    LPFIELDINFO lpCol;
    RECT        rect, rectTitle, rectColTitle, rectRecs;
    POINT       pt;
    UINT        wState;
    int         xStrCol, xEndCol;
    int         xLeft, xRight;

   /*-------------------------------------------------------------------*
    * On WM_MOUSEMOVE messages we want to know where the mouse has moved
    * so the appropriate action can be taken based on the current state.
    *-------------------------------------------------------------------*/

    // Get the mouse coord into a POINT.
    pt.x = x;
    pt.y = y;

    if( StateTest(lpCnt, CNTSTATE_SIZINGFLD) )
      {
      // If we are resizing a field, redraw the new field border.
      DrawSizingLine( hWnd, lpCnt, pt.x );
      }
    else if( StateTest(lpCnt, CNTSTATE_MOVINGFLD) )
      {
      // If we are moving a field, redraw the new field rect.
      DrawMovingRect( hWnd, lpCnt, &pt );
      }
    else if( StateTest(lpCnt, CNTSTATE_CLKEDTTLBTN) )
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
    else if( StateTest(lpCnt, CNTSTATE_CLKEDFLDBTN) )
      {
      // If we are down on a field button see if we went out of it
      // or if we were out of it, see if we came back into it.
      wState = lpCnt->wState;

      if( PtInRect(&lpCnt->rectPressedBtn, pt) )
        {
        StateClear( lpCnt, CNTSTATE_MOUSEOUT );
        lpCnt->lpPressedFld->flColAttr |= CFA_FLDBTNPRESSED;
        }
      else
        {
        StateSet( lpCnt, CNTSTATE_MOUSEOUT );
        lpCnt->lpPressedFld->flColAttr &= ~((DWORD)CFA_FLDBTNPRESSED);
        }

      // If state changed, repaint the field title button.
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
      SetRectEmpty( &rectColTitle );
      SetRectEmpty( &rectRecs     );
    
      // Setup the title rect if there is one.
      if( lpCnt->lpCI->cyTitleArea )
        {
        CopyRect( &rectTitle, &rect );
        rectTitle.bottom = lpCnt->lpCI->cyTitleArea;
        }
    
      // There is nothing to the right of this value.
/*      xLastCol = lpCnt->lpCI->nMaxWidth - lpCnt->nHscrollPos * lpCnt->lpCI->cxChar;*/
    
      // Setup the column titles rect if there are any.
      if( lpCnt->lpCI->cyColTitleArea )
        {
        CopyRect( &rectColTitle, &rect );
        rectColTitle.right  = min(rect.right, lpCnt->xLastCol);
        rectColTitle.top    = rectTitle.bottom;
        rectColTitle.bottom = rectColTitle.top + lpCnt->lpCI->cyColTitleArea;
        }
    
      // Setup the record data rect.
      CopyRect( &rectRecs, &rect );
      rectRecs.right = min(rect.right, lpCnt->xLastCol);
      rectRecs.top   = rectColTitle.bottom;
    
      // Set the appropriate cursor.
      if( PtInRect( &rectRecs, pt ) )
        {
        SetCursor( lpCnt->lpCI->hCursor );
        }
      else if( PtInRect( &rectColTitle, pt ) )
        {
        // Get the field the cursor is over.
        lpCol = GetFieldAtPt( lpCnt, &pt, &xStrCol, &xEndCol );
  
        if( PtInFldSizeArea( lpCnt, lpCol, &pt ) )
          {
          SetCursor( lpCnt->lpCI->hCurFldSizing );
          }
        else if( PtInFldTtlBtn( lpCnt, lpCol, &pt, &xLeft, &xRight ) )
          {
          SetCursor( lpCnt->lpCI->hCurFldTtlBtn );
          }
        else
          {
          SetCursor( lpCnt->lpCI->hCurFldTtl ? lpCnt->lpCI->hCurFldTtl : lpCnt->lpCI->hCursor );
          }
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

VOID Dtl_OnLButtonUp( HWND hWnd, int x, int y, UINT keyFlags )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);
    POINT      pt;

    // Get the mouse coordinates at the point of the button up.
    pt.x = x;
    pt.y = y;

    ReleaseCapture();
    // Take appropriate action according to the state.
    if( StateTest(lpCnt, CNTSTATE_SIZINGFLD) )
      {
      // If we are resizing a field, redraw the newly sized fields.
      StateClear( lpCnt, CNTSTATE_SIZINGFLD );
      DrawSizingLine( hWnd, lpCnt, 0 );
      ResizeCntFld( hWnd, lpCnt, pt.x );
      }
    else if( StateTest(lpCnt, CNTSTATE_MOVINGFLD) )
      {
      // If we are moving a field, redraw the fields in the new order.
      StateClear( lpCnt, CNTSTATE_MOVINGFLD );
      DrawMovingRect( hWnd, lpCnt, NULL );
      ReorderCntFld( hWnd, lpCnt, pt.x );
      }
    else if( StateTest(lpCnt, CNTSTATE_CLKEDTTLBTN) )
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
    else if( StateTest(lpCnt, CNTSTATE_CLKEDFLDBTN) )
      {
      // If we are releasing a field button reset the pressed flag,
      // repaint it in the up position, and notify the assoc wnd
      // (if mouse is still in the boundaries of the field button).
      if( !StateTest( lpCnt, CNTSTATE_MOUSEOUT ) )
        {
        lpCnt->lpPressedFld->flColAttr &= ~((DWORD)CFA_FLDBTNPRESSED);
        StateClear( lpCnt, CNTSTATE_CLKEDFLDBTN );
        UpdateContainer( hWnd, &lpCnt->rectPressedBtn, TRUE );
        NotifyAssocWnd( hWnd, lpCnt, CN_FLDTTLBTNCLK, 0, NULL, lpCnt->lpPressedFld, 0, 0, 0, NULL );
        }
      }

    // Clear out the various pointers and states.
    lpCnt->lp1stSelRec  = NULL;
    lpCnt->lp1stSelFld  = NULL;
    lpCnt->lpPressedFld = NULL;
    lpCnt->lpSizMovFld  = NULL;
    lpCnt->xPrevPos     = 0;
    lpCnt->yPrevPos     = 0;
    lpCnt->cxMoveFld    = 0;
    lpCnt->cyMoveFld    = 0;
    lpCnt->xOffMove     = 0;
    lpCnt->yOffMove     = 0;
    SetRectEmpty( &lpCnt->rectPressedBtn );
    SetRectEmpty( &lpCnt->rectCursorFld  );
    StateClear( lpCnt, CNTSTATE_MOUSEOUT );
    StateClear( lpCnt, CNTSTATE_CLKEDTTLBTN );
    StateClear( lpCnt, CNTSTATE_CLKEDFLDBTN );
    StateClear( lpCnt, CNTSTATE_SIZINGFLD );
    StateClear( lpCnt, CNTSTATE_MOVINGFLD );

    return;
    }

UINT Dtl_OnGetDlgCode( HWND hWnd, LPMSG lpmsg )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);
    UINT       uRet;

    // Processing of this message is only necessary for containers
    // within dialogs. The return value of DLGC_WANTMESSAGE tells the
    // dialog manager that we want ALL keystroke msgs to come to this
    // control and no further processing of the keystroke msg is to
    // take place. If this message is not processed in this manner the
    // container will not receive certain important keystroke messages
    // (like TAB, ENTER, and ESCAPE) while in a dialog.
    uRet = FORWARD_WM_GETDLGCODE( hWnd, lpmsg, CNT_DEFWNDPROC );
    uRet |= DLGC_WANTMESSAGE;

    return uRet;
    }

VOID Dtl_OnNCLButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);
    LPCONTAINER lpCntFrame;
    BOOL       bSetFocus=FALSE;

    // When a non-client hit occurs send the focus to the 1st child if
    // the container does not have the focus.
    if( lpCnt->bIsSplit )
      {
      lpCntFrame = GETCNT( lpCnt->hWndFrame );
      if( GetFocus() != lpCntFrame->SplitBar.hWndRight &&
          GetFocus() != lpCntFrame->SplitBar.hWndLeft )
        bSetFocus = TRUE;
      }
    else
      {
      if( GetFocus() != hWnd )
        bSetFocus = TRUE;
      }

    // Set the focus to the container child on an NC buttondown
    // if the focus is on some other control.
    if( bSetFocus )
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

VOID Dtl_OnNCMButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);
    LPCONTAINER lpCntFrame;
    BOOL       bSetFocus=FALSE;

    // When a non-client hit occurs send the focus to the 1st child if
    // the container does not have the focus.
    if( lpCnt->bIsSplit )
      {
      lpCntFrame = GETCNT( lpCnt->hWndFrame );
      if( GetFocus() != lpCntFrame->SplitBar.hWndRight &&
          GetFocus() != lpCntFrame->SplitBar.hWndLeft )
        bSetFocus = TRUE;
      }
    else
      {
      if( GetFocus() != hWnd )
        bSetFocus = TRUE;
      }

    // Set the focus to the container child on an NC buttondown
    // if the focus is on some other control.
    if( bSetFocus )
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

VOID Dtl_OnNCRButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);
    LPCONTAINER lpCntFrame;
    BOOL       bSetFocus=FALSE;

    // When a non-client hit occurs send the focus to the 1st child if
    // the container does not have the focus.
    if( lpCnt->bIsSplit )
      {
      lpCntFrame = GETCNT( lpCnt->hWndFrame );
      if( GetFocus() != lpCntFrame->SplitBar.hWndRight &&
          GetFocus() != lpCntFrame->SplitBar.hWndLeft )
        bSetFocus = TRUE;
      }
    else
      {
      if( GetFocus() != hWnd )
        bSetFocus = TRUE;
      }

    // Set the focus to the container child on an NC buttondown
    // if the focus is on some other control.
    if( bSetFocus )
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
 * GetRecordData
 *
 * Purpose:
 *  Extracts and formats record data for a specified field.
 *  Valid for DETAIL view only.
 *
 * Parameters:
 *  HWND          hWndCnt     - handle to container window.
 *  LPRECORDCORE  lpRec       - pointer to the record.
 *  LPFIELDINFO   lpField     - pointer to the field.
 *  UINT          wMaxLen     - max length of output string buffer including terminator
 *  LPSTR         lpStrData   - pointer to string buffer for converting data
 *
 * Return Value:
 *  UINT          length of converted data in string format
 ****************************************************************************/

UINT WINAPI GetRecordData( HWND hWndCnt, LPRECORDCORE lpRec, LPFIELDINFO lpField, UINT wMaxLen, LPSTR lpStrData )
    {
    LPSTR  lpData;
    LPSTR  lpStr;

    *lpStrData = 0;

    if( !lpField || !lpRec )
      return 0;
  
    lpData = (LPSTR)lpRec->lpRecData + lpField->wOffStruct;    
    if( !lpData )
      return 0;

    switch( lpField->wColType )
      {
      case CFT_STRING:                 // char data
        if( lpField->lpDescriptor && lpField->bDescEnabled && lstrlen(lpField->lpDescriptor) )
          wsprintf( lpStrData, lpField->lpDescriptor, lpData ); 
        else    
          wsprintf( lpStrData, "%.255s", lpData ); 
        break;

      case CFT_LPSTRING:               // far ptr to char data - could be NULL!
        lpStr = *(LPSTR FAR *)lpData; 
        if( lpStr )
          {
          if( lpField->lpDescriptor && lpField->bDescEnabled && lstrlen(lpField->lpDescriptor) )
            wsprintf( lpStrData, lpField->lpDescriptor, lpStr ); 
          else    
            wsprintf( lpStrData, "%.255s", lpStr); 
          }
        break;

      case CFT_WORD:                   // unsigned short
        if( lpField->lpDescriptor && lpField->bDescEnabled && lstrlen(lpField->lpDescriptor) )
          CAsprintf( lpStrData, lpField->lpDescriptor, *(LPWORD)lpData );
        else if( lpField->flColAttr & CFA_HEX )
          CAsprintf( lpStrData, "%04x", *(LPWORD)lpData ); 
        else if( lpField->flColAttr & CFA_OCTAL )
          CAsprintf( lpStrData, "%o", *(LPWORD)lpData ); 
        else if( lpField->flColAttr & CFA_BINARY )
          CAsprintf( lpStrData, "%b", *(LPWORD)lpData ); 
        else
          wsprintf( lpStrData, "%u", *(LPWORD)lpData ); 
        break;

      case CFT_UINT:                   // unsigned int
        if( lpField->lpDescriptor && lpField->bDescEnabled && lstrlen(lpField->lpDescriptor) )
          CAsprintf( lpStrData, lpField->lpDescriptor, *(UINT FAR *)lpData );
        else if( lpField->flColAttr & CFA_HEX )
          CAsprintf( lpStrData, "%04x", *(UINT FAR *)lpData ); 
        else if( lpField->flColAttr & CFA_OCTAL )
          CAsprintf( lpStrData, "%o", *(UINT FAR *)lpData ); 
        else if( lpField->flColAttr & CFA_BINARY )
          CAsprintf( lpStrData, "%b", *(UINT FAR *)lpData ); 
        else
          wsprintf( lpStrData, "%u", *(UINT FAR *)lpData ); 
        break;

      case CFT_INT:                    // signed int
        if( lpField->lpDescriptor && lpField->bDescEnabled && lstrlen(lpField->lpDescriptor) )
          CAsprintf( lpStrData, lpField->lpDescriptor, *(LPINT)lpData );
        else if( lpField->flColAttr & CFA_HEX )
          CAsprintf( lpStrData, "%04x", *(LPINT)lpData ); 
        else if( lpField->flColAttr & CFA_OCTAL )
          CAsprintf( lpStrData, "%o", *(LPINT)lpData ); 
        else if( lpField->flColAttr & CFA_BINARY )
          CAsprintf( lpStrData, "%b", *(LPINT)lpData ); 
        else
          wsprintf( lpStrData, "%i", *(LPINT)lpData ); 
        break;

      case CFT_DWORD:                  // unsigned long
        if( lpField->lpDescriptor && lpField->bDescEnabled && lstrlen(lpField->lpDescriptor) )
          {
          // CAsprintf doesn't support long unsigned conversions (%lu)
          // but wsprintf does.
          if( _fstrchr( lpField->lpDescriptor, 'u' ) )
            wsprintf( lpStrData, lpField->lpDescriptor, *(LPDWORD)lpData );
          else    
            CAsprintf( lpStrData, lpField->lpDescriptor, *(LPDWORD)lpData );
          }
        else if( lpField->flColAttr & CFA_HEX )
          wsprintf( lpStrData, "%#08lx", *(LPDWORD)lpData ); 
        else if( lpField->flColAttr & CFA_OCTAL )
          CAsprintf( lpStrData, "%lo", *(LPDWORD)lpData ); 
        else if( lpField->flColAttr & CFA_BINARY )
          CAsprintf( lpStrData, "%lb", *(LPDWORD)lpData ); 
        else
          wsprintf( lpStrData, "%lu", *(LPDWORD)lpData ); 
        break;

      case CFT_LONG:                   // signed long
        if( lpField->lpDescriptor && lpField->bDescEnabled && lstrlen(lpField->lpDescriptor) )
          CAsprintf( lpStrData, lpField->lpDescriptor, *(LPLONG)lpData );
        else if( lpField->flColAttr & CFA_HEX )
          wsprintf( lpStrData, "%#08lx", *(LPLONG)lpData ); 
        else if( lpField->flColAttr & CFA_OCTAL )
          CAsprintf( lpStrData, "%lo", *(LPLONG)lpData ); 
        else if( lpField->flColAttr & CFA_BINARY )
          CAsprintf( lpStrData, "%lb", *(LPLONG)lpData ); 
        else
          wsprintf( lpStrData, "%li", *(LPLONG)lpData ); 
        break;

      case CFT_FLOAT:                  // float
        if( lpField->lpDescriptor && lpField->bDescEnabled && lstrlen(lpField->lpDescriptor) )
          CAsprintf( lpStrData, lpField->lpDescriptor, *(float FAR *)lpData );
        else if( lpField->flColAttr & CFA_SCIENTIFIC )
          CAsprintf( lpStrData, "%e", *(float FAR *)lpData ); 
        else
          CAsprintf( lpStrData, "%f", *(float FAR *)lpData ); 
        break;

      case CFT_DOUBLE:                 // double
        if( lpField->lpDescriptor && lpField->bDescEnabled && lstrlen(lpField->lpDescriptor) )
          CAsprintf( lpStrData, lpField->lpDescriptor, *(double FAR *)lpData );
        else if( lpField->flColAttr & CFA_SCIENTIFIC )
          CAsprintf( lpStrData, "%le", *(double FAR *)lpData ); 
        else
          CAsprintf( lpStrData, "%lf", *(double FAR *)lpData ); 
        break;

      case CFT_BCD:                    // binary coded data
        break;

      case CFT_BMP:                    // bitmap data
        break;

      case CFT_ICON:                   // icon data
        break;

      case CFT_CUSTOM:                 // custom user type
        if( lpField->lpfnCvtToStr )
          {
          (*lpField->lpfnCvtToStr)( hWndCnt, lpField->lpDescriptor,
                                    lpData, wMaxLen, lpStrData ); 
          }
        break;

      case CFT_CHAR:                   // single character
        if( lpField->lpDescriptor && lpField->bDescEnabled && lstrlen(lpField->lpDescriptor) )
          wsprintf( lpStrData, lpField->lpDescriptor, lpData[0] ); 
        else    
          wsprintf( lpStrData, "%c", lpData[0] ); 
        break;
      }

    return lstrlen( lpStrData );
    }

/****************************************************************************
 * InvalidateCntRecord
 *
 * Purpose:
 *  Invalidates and repaints (maybe) a container record or cell.
 *  This layer exists because of the need to invalidate split children
 *  if they are active. If fMode=0 OR split bars are not active OR split
 *  bars are active but the container is not currently split the call goes
 *  straight thru to InvalidateCntRec. If the container is split we must
 *  update the record or cell of the other split child.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the control window.
 *  LPCONTAINER   lpCnt       - pointer to existing CONTAINER structure.
 *  LPRECORDCORE  lpRecord    - pointer to record to be repainted
 *  LPVOID        lpFldCol    - pointer to the field or column to be repainted.  If
 *                              Detail view, then field, otherwise column.
 *                              (if NULL then the entire record will be repainted)
 *  UINT          fMode       - indicating what type of action to take:
 *                              0: no action (just see if the rec is in display area)
 *                              1: invalidate record but DO NOT update
 *                              2: invalidate record and update immediately
 *
 * Return Value:
 *  int           >= 0  position of record in the display area
 *                -1    record was NOT found in the display area
 ****************************************************************************/

int WINAPI InvalidateCntRecord( HWND hWnd, LPCONTAINER lpCnt, LPRECORDCORE lpRecord,
                                LPVOID lpFldCol, UINT fMode )
{
    LPCONTAINER lpCntFrame;
    LPCONTAINER lpCntSplit;
    HWND hWndSplit;
    int  nRet;
    LPFIELDINFO   lpFld; // if detail view, use this
    LPFLOWCOLINFO lpCol; // text, name views, use this

   /*-----------------------------------------------------------------------*
    * cast over the column structure depending on the view.
    *-----------------------------------------------------------------------*/
    if (lpCnt->iView & CV_DETAIL)
      lpFld = (LPFIELDINFO)lpFldCol;
    else if (lpCnt->iView & CV_ICON)
      ;
    else
      lpCol = (LPFLOWCOLINFO)lpFldCol;

   /*-----------------------------------------------------------------------*
    * If only checking for position don't worry about splits.
    *-----------------------------------------------------------------------*/
    if( !fMode )
    {
      if (lpCnt->iView & CV_DETAIL)
        return InvalidateCntRec( hWnd, lpCnt, lpRecord, lpFld, fMode );
      else if (lpCnt->iView & CV_ICON)
        return InvalidateCntRecIcon( hWnd, lpCnt, lpRecord, fMode );
      else
        return InvalidateCntRecTxt( hWnd, lpCnt, lpRecord, lpCol, fMode );
    }

   /*-----------------------------------------------------------------------*
    * This covers no-splitbar-active and splitbar-active but not split. 
    *-----------------------------------------------------------------------*/
    if( !lpCnt->bIsSplit )
    {
      if (lpCnt->iView & CV_DETAIL)
        return InvalidateCntRec( hWnd, lpCnt, lpRecord, lpFld, fMode );
      else if (lpCnt->iView & CV_ICON)
        return InvalidateCntRecIcon( hWnd, lpCnt, lpRecord, fMode );
      else
        return InvalidateCntRecTxt( hWnd, lpCnt, lpRecord, lpCol, fMode );
    }

   /*-----------------------------------------------------------------------*
    * If it made it here it means split bars are active and we are split.
    *-----------------------------------------------------------------------*/

    if (lpCnt->iView & CV_DETAIL) // should only get here in this view but I'll put this here anyways
    {
      // Get the window handle of the other split child.
      lpCntFrame = GETCNT( lpCnt->hWndFrame );
      if( hWnd == lpCntFrame->SplitBar.hWndLeft )
        hWndSplit = lpCntFrame->SplitBar.hWndRight;
      else
        hWndSplit = lpCntFrame->SplitBar.hWndLeft;

      // Get the container struct of the other split child.
      lpCntSplit = GETCNT( hWndSplit );

     // Update the record in both split windows.
      nRet = InvalidateCntRec( hWnd, lpCnt, lpRecord, lpFld, fMode );
      InvalidateCntRec( hWndSplit, lpCntSplit, lpRecord, lpFld, fMode );
      return nRet;
    }
}

/****************************************************************************
 * InvalidateCntRec
 *
 * Purpose:
 *  Invalidates and repaints (maybe) a container cell, record, field, or
 *  entire record area based on the record and field pointer passed in.
 *
 *      Record Ptr   Field Ptr    Action
 *      ----------   ---------    ------------------------------------
 *        valid        valid      repaints a container cell
 *        valid        NULL       repaints a container record
 *        NULL         valid      repaints a container field
 *        NULL         NULL       repaints entire container record area
 *
 * Parameters:
 *  HWND          hWnd        - handle to the control window.
 *  LPCONTAINER   lpCnt       - pointer to existing CONTAINER structure.
 *  LPRECORDCORE  lpRecord    - pointer to record to be repainted
 *  LPFIELDINFO   lpFld       - pointer to the field to be repainted
 *  UINT          fMode       - indicating what type of action to take:
 *                              0: no action (just see if the rec is in display area)
 *                              1: invalidate taget but DO NOT update
 *                              2: invalidate target and update immediately
 *
 * Return Value:
 *  int           >= 0 position of record in the display area
 *                -1   record was NOT found in the display area
 ****************************************************************************/

int WINAPI InvalidateCntRec( HWND hWnd, LPCONTAINER lpCnt, LPRECORDCORE lpRecord,
                                 LPFIELDINFO lpFld, UINT fMode )
    {
    LPRECORDCORE lpRec;
    LPFIELDINFO  lpCol;
    RECT         rect;
    BOOL         bFound=FALSE;
    int          i, yRecAreaTop;
    int          cxCol, xStrCol, xEndCol;

    // Make sure we have a window and container struct.
    if( !hWnd || !lpCnt )
      return -1;

    // Make sure we have a record pointer.
//    if( !lpRecord )
//      return -1;

    if( lpRecord )
      {
      // If this record is in the display area invalidate it.
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

      if( bFound && fMode )
        {
        // If the record was found in the display area then find
        // the start and end of the field (if one was passed in).
        if( lpFld )
          {
          xStrCol = -lpCnt->nHscrollPos * lpCnt->lpCI->cxChar;

          lpCol = lpCnt->lpCI->lpFieldHead;  // point at 1st column
          while( lpCol )
            {
            cxCol   = lpCol->cxWidth * lpCnt->lpCI->cxChar;
            xEndCol = xStrCol + cxCol;
      
            // break out when we find the column
            if( lpCol == lpFld )
              break;  

            xStrCol += cxCol;
            lpCol = lpCol->lpNext;       // advance to next column
            }

          rect.left  = xStrCol;
          rect.right = xEndCol;
          }
        else
          {
          rect.left  = 0;
          if( lpCnt->lpCI->flCntAttr & CA_WIDEFOCUSRECT && 
              lpCnt->lpCI->flCntAttr & CA_HSCROLLBYCHAR )
            rect.right = lpCnt->xLastCol;
          else
            rect.right = min(lpCnt->cxClient, lpCnt->xLastCol);
          }

        yRecAreaTop = lpCnt->lpCI->cyTitleArea + lpCnt->lpCI->cyColTitleArea;
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
      if( fMode )
        {
        // Find the start and end of the field (if one was passed in).
        if( lpFld )
          {
          xStrCol = -lpCnt->nHscrollPos * lpCnt->lpCI->cxChar;

          lpCol = lpCnt->lpCI->lpFieldHead;  // point at 1st column
          while( lpCol )
            {
            cxCol   = lpCol->cxWidth * lpCnt->lpCI->cxChar;
            xEndCol = xStrCol + cxCol;
      
            // break out when we find the column
            if( lpCol == lpFld )
              break;  

            xStrCol += cxCol;
            lpCol = lpCol->lpNext;       // advance to next column
            }

          rect.left  = xStrCol;
          rect.right = xEndCol;
          }
        else
          {
          // Set to the entire width of the container.
          rect.left  = 0;
          if( lpCnt->lpCI->flCntAttr & CA_WIDEFOCUSRECT && 
              lpCnt->lpCI->flCntAttr & CA_HSCROLLBYCHAR )
            rect.right = lpCnt->xLastCol;
          else
            rect.right = min(lpCnt->cxClient, lpCnt->xLastCol);
          }

        // Set the top and bottom of the record area.
        rect.top    = lpCnt->lpCI->cyTitleArea + lpCnt->lpCI->cyColTitleArea;
        rect.bottom = lpCnt->cyClient;

        if( fMode == 1 )
          InvalidateRect( hWnd, &rect, FALSE );
        else if( fMode == 2 )
          UpdateContainer( hWnd, &rect, TRUE );
        }
      }

    return bFound ? i : -1;
    }

/****************************************************************************
 * GetFieldXPos
 *
 * Purpose:
 *  Gets the horz starting and ending position of a specified field.
 *
 * Parameters:
 *  LPCONTAINER   lpCnt       - pointer to existing CONTAINER structure.
 *  LPFIELDINFO   lpFld       - pointer to the field
 *  LPINT         lpxStr      - returned starting position of the field
 *  LPINT         lpxEnd      - returned ending position of the field
 *
 * Return Value:
 *  BOOL          TRUE if function is successful; else FALSE
 ****************************************************************************/

BOOL WINAPI GetFieldXPos( LPCONTAINER lpCnt, LPFIELDINFO lpFld, LPINT lpxStr, LPINT lpxEnd )
    {
    LPFIELDINFO lpCol;
    int         cxCol, xStrCol=0, xEndCol=0;

    if( lpFld )
      {
      xStrCol = -lpCnt->nHscrollPos * lpCnt->lpCI->cxChar;

      lpCol = lpCnt->lpCI->lpFieldHead;  // point at 1st column
      while( lpCol )
        {
/*        cxCol   = lpCol->cxWidth * lpCnt->lpCI->cxChar + lpCnt->lpCI->cxChar;*/
        cxCol   = lpCol->cxWidth * lpCnt->lpCI->cxChar;
        xEndCol = xStrCol + cxCol;
    
        // break out when we find the column
        if( lpCol == lpFld )
          break;  

        xStrCol += cxCol;
        lpCol = lpCol->lpNext;       // advance to next column
        }
      }

/*    return MAKELONG(xStrCol, xEndCol);*/
    *lpxStr = xStrCol;
    *lpxEnd = xEndCol;
    return TRUE;
    }

/****************************************************************************
 * PtInTtlBtn
 *
 * Purpose:
 *  Determines if a point is in a container title button.
 *
 * Parameters:
 *  LPCONTAINER   lpCnt       - pointer to existing CONTAINER structure.
 *  LPPOINT       lpPt        - the point of interest.
 *
 * Return Value:
 *  BOOL          TRUE  if point is in the title button rect
 *                FALSE if point is NOT in the title button rect
 ****************************************************************************/
BOOL WINAPI PtInTtlBtn( LPCONTAINER lpCnt, LPPOINT lpPt )
    {
    BOOL  bIsInBtn=FALSE;
    int   cxPxlWidth;
    int   xLeft=0;
    int   xRight=lpCnt->cxClient;

    // Simply return if no title button is defined.
    if( lpCnt->lpCI->nTtlBtnWidth )
      {
      // Get the title button width in pixels.
      if( lpCnt->lpCI->nTtlBtnWidth > 0 )
        cxPxlWidth = lpCnt->lpCI->cxCharTtl * lpCnt->lpCI->nTtlBtnWidth;
      else
        cxPxlWidth = -lpCnt->lpCI->nTtlBtnWidth;

      // Get the left and right boundaries of the title button.
      if( lpCnt->lpCI->bTtlBtnAlignRt )
        xLeft  = xRight - cxPxlWidth;
      else
        xRight = xLeft  + cxPxlWidth;

      // Set flag if X point is over the button.
      if( lpPt->x >= xLeft && lpPt->x <= xRight )
        bIsInBtn = TRUE;
      }

    return bIsInBtn;
    }

/****************************************************************************
 * PtInFldTtlBtn
 *
 * Purpose:
 *  Determines if a point is in a field title button.
 *
 * Parameters:
 *  LPCONTAINER   lpCnt       - pointer to existing CONTAINER structure.
 *  LPFIELDINFO   lpFld       - pointer to the field
 *  LPPOINT       lpPt        - the point of interest.
 *  LPINT         lpStr       - returned X coord start of field button
 *  LPINT         lpEnd       - returned X coord end of field button
 *
 * Return Value:
 *  BOOL          TRUE  if point is in the field button rect
 *                FALSE if point is NOT in the field button rect
 ****************************************************************************/
BOOL WINAPI PtInFldTtlBtn( LPCONTAINER lpCnt, LPFIELDINFO lpFld, LPPOINT lpPt, LPINT lpStr, LPINT lpEnd )
    {
    BOOL  bIsInBtn=FALSE;
    int   cxPxlWidth;
    int   xLeft=0, xRight=0;

    // Simply return if no field title button is defined.
    if( lpFld->nFldBtnWidth )
      {
      // Get the left and right boundaries of the field.
/*      dwEdges = GetFieldXPos( lpCnt, lpFld );*/
/*      xLeft  = (int) LOWORD(dwEdges);*/
/*      xRight = (int) HIWORD(dwEdges);*/
      GetFieldXPos( lpCnt, lpFld, &xLeft, &xRight );
        
      // Get the field button width in pixels.
      if( lpFld->nFldBtnWidth > 0 )
        cxPxlWidth = lpCnt->lpCI->cxCharFldTtl * lpFld->nFldBtnWidth;
      else
        cxPxlWidth = -lpFld->nFldBtnWidth;

      // Get the left and right boundaries of the field button.
      if( lpFld->bFldBtnAlignRt )
        xLeft  = xRight - cxPxlWidth;
      else
        xRight = xLeft  + cxPxlWidth;

      // Set flag if X point is over the button.
      if( lpPt->x >= xLeft && lpPt->x <= xRight )
        bIsInBtn = TRUE;
      }

    // Return the button boundaries.
    *lpStr = xLeft;
    *lpEnd = xRight;

    return bIsInBtn;
    }

/****************************************************************************
 * GetFieldAtPt
 *
 * Purpose:
 *  Gets the field under the given point
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
LPFIELDINFO WINAPI GetFieldAtPt( LPCONTAINER lpCnt, LPPOINT lpPt, LPINT lpXstr, LPINT lpXend )
    {
    LPFIELDINFO  lpCol;
    int          cxCol, xStrCol=0, xEndCol=0;

    xStrCol = -lpCnt->nHscrollPos * lpCnt->lpCI->cxChar;
    lpCol   = lpCnt->lpCI->lpFieldHead;  // point at 1st column
    while( lpCol )
      {
      cxCol   = lpCol->cxWidth * lpCnt->lpCI->cxChar;
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
 * PtInFldSizeArea
 *
 * Purpose:
 *  Determines if a point is in the sizing area around the field borders.
 *
 * Parameters:
 *  LPCONTAINER   lpCnt       - pointer to existing CONTAINER structure.
 *  LPFIELDINFO   lpFld       - pointer to the field
 *  LPPOINT       lpPt        - the point of interest.
 *
 * Return Value:
 *  BOOL          TRUE  if point is in the field sizing area
 *                FALSE if point is NOT in the field sizing area
 ****************************************************************************/
BOOL WINAPI PtInFldSizeArea( LPCONTAINER lpCnt, LPFIELDINFO lpFld, LPPOINT lpPt )
    {
    BOOL  bIsSizing=FALSE;
    BOOL  bChkFld=FALSE;
    LPFIELDINFO  lpCol=lpFld;
    int   xLeft=0, xRight=0;

#ifdef ZOT
    if( lpCnt->lpCI->flCntAttr & CA_SIZEABLEFLDS )
      {
/*      dwEdges = GetFieldXPos( lpCnt, lpFld );*/
/*      xLeft  = (int) LOWORD(dwEdges);*/
/*      xRight = (int) HIWORD(dwEdges);*/
      GetFieldXPos( lpCnt, lpFld, &xLeft, &xRight );

      if( lpPt->x > FLDSIZE_HIT &&
          lpPt->x <  xLeft  + FLDSIZE_HIT ||
          lpPt->x >= xRight - FLDSIZE_HIT )
        bIsSizing = TRUE;
      }
#endif

/*    dwEdges = GetFieldXPos( lpCnt, lpCol );*/
/*    xLeft  = (int) LOWORD(dwEdges);*/
/*    xRight = (int) HIWORD(dwEdges);*/
    GetFieldXPos( lpCnt, lpCol, &xLeft, &xRight );

    if( lpPt->x < xLeft + FLDSIZE_HIT )
      {
      // We are at the left border of this field so the
      // field which is to be sized is the previous one.
      if( lpCol->lpPrev )
        {
        lpCol = lpCol->lpPrev;
        bChkFld = TRUE;
        }
      }
    else if( lpPt->x >= xRight - FLDSIZE_HIT )
      {
      bChkFld = TRUE;
      }

    if( bChkFld )
      {
      // If we have more than 1 hidden field stacked up on a field
      // boundary we want the last hidden field to be the sizing target.
      while( lpCol->lpNext )
        {
        // Break out when next col has some width.
        if( lpCol->lpNext->cxWidth )
          break;  

        // Next field was a hidden field so make it the sizing target.
        lpCol = lpCol->lpNext;
        }

      // If the target field is hidden AND marked non-sizeable we have to find
      // the 1st sizeable hidden field before it or the 1st non-hidden field.
      if( !lpCol->cxWidth && (lpCol->flColAttr & CFA_NONSIZEABLEFLD) )
        {
        while( lpCol->lpPrev )
          {
          // Back up until we find a sizeable or non-hidden field.
          lpCol = lpCol->lpPrev;
          if( !(lpCol->flColAttr & CFA_NONSIZEABLEFLD) || lpCol->cxWidth )
            break;  
          }
        }

      // If all fields or this field is tagged sizeable return TRUE.
      if( !(lpCol->flColAttr & CFA_NONSIZEABLEFLD) )
        {
        if( lpCnt->lpCI->flCntAttr & CA_SIZEABLEFLDS ||
            lpCol->flColAttr & CFA_SIZEABLEFLD )
          {
          bIsSizing = TRUE;
          }
        }
      }

    return bIsSizing;
    }

/****************************************************************************
 * GetFldToBeSized
 *
 * Purpose:
 *  Determines which field is to be sized and returns its right border.
 *
 * Parameters:
 *  LPCONTAINER   lpCnt       - pointer to existing CONTAINER structure.
 *  LPFIELDINFO   lpFld       - pointer to the field
 *  LPPOINT       lpPt        - the point of interest.
 *  LPINT         lpXright    - x coord of right border of the sizing field
 *
 * Return Value:
 *  LPFIELDINFO   pointer to the field to be sized
 ****************************************************************************/
LPFIELDINFO WINAPI GetFldToBeSized( LPCONTAINER lpCnt, LPFIELDINFO lpFld, LPPOINT lpPt, LPINT lpXright )
    {
    LPFIELDINFO lpCol=lpFld;
    int         xLeft=0, xRight=0;

/*    dwEdges = GetFieldXPos( lpCnt, lpFld );*/
/*    xLeft  = (int) LOWORD(dwEdges);*/
/*    xRight = (int) HIWORD(dwEdges);*/
    GetFieldXPos( lpCnt, lpFld, &xLeft, &xRight );

    if( lpPt->x < xLeft + FLDSIZE_HIT )
      {
      // We are at the left border of this field so the
      // field which is to be sized is the previous one.
      if( lpCol->lpPrev )
        lpCol = lpCol->lpPrev;

/*      dwEdges = GetFieldXPos( lpCnt, lpCol );*/
/*      xLeft  = (int) LOWORD(dwEdges);*/
/*      xRight = (int) HIWORD(dwEdges);*/
      GetFieldXPos( lpCnt, lpCol, &xLeft, &xRight );

      // Pass back the x coord of the right border.
      *lpXright = xRight;
      }

    // If we have more than 1 hidden field stacked up on a field
    // boundary we want the last hidden field to be the sizing target.
    while( lpCol->lpNext )
      {
      // Break out when next col has some width.
      if( lpCol->lpNext->cxWidth )
        break;  

      // Next field was a hidden field so make it the sizing target.
      lpCol = lpCol->lpNext;
      }

    // If the target field is hidden AND marked non-sizeable we have to find
    // the 1st sizeable hidden field before it or the 1st non-hidden field.
    if( !lpCol->cxWidth && (lpCol->flColAttr & CFA_NONSIZEABLEFLD) )
      {
      while( lpCol->lpPrev )
        {
        // Back up until we find a sizeable or non-hidden field.
        lpCol = lpCol->lpPrev;
        if( !(lpCol->flColAttr & CFA_NONSIZEABLEFLD) || lpCol->cxWidth )
          break;  
        }
      }

    // Pass back the x coord of the right border.
    *lpXright = xRight;

    return lpCol;
    }

/****************************************************************************
 * DrawSizingLine
 *
 * Purpose:
 *  Draws the sizing line while a field is being dynamically sized.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window.
 *  LPCONTAINER   lpCnt       - pointer to existing CONTAINER structure.
 *  int           xNewRight   - the x coord of the new right border
 *                              (if NULL only the previous line is removed)
 * Return Value:
 *  VOID
 ****************************************************************************/
VOID WINAPI DrawSizingLine( HWND hWnd, LPCONTAINER lpCnt, int xNewRight )
    {
    HDC  hDC = GetDC(hWnd);
    HPEN hPen;
    int yTop = lpCnt->lpCI->cyTitleArea;
    int yBottom = lpCnt->cyClient;
    
    // Don't redraw if the mouse has not moved horizontally.
    if( lpCnt->xPrevPos != xNewRight )
      {
      // Erase the previous line
      if( lpCnt->xPrevPos )
        {
        // If this is the 1st move just redraw the field border (if enabled).
        if( lpCnt->xPrevPos == lpCnt->xStartPos &&
            ( lpCnt->lpCI->flCntAttr & CA_VERTFLDSEP ||
              lpCnt->lpSizMovFld->flColAttr & CFA_VERTSEPARATOR ) )
          {
          // Create a grid colored pen.
          hPen = CreatePen( PS_SOLID, 1, lpCnt->crColor[CNTCOLOR_GRID] );
          SelectObject( hDC, hPen );
          }
        else
          {
          SetROP2(hDC, R2_NOT);  
          }

        MoveToEx( hDC, lpCnt->xPrevPos, yTop, NULL );
        LineTo( hDC, lpCnt->xPrevPos, yBottom );

        if( lpCnt->xPrevPos == lpCnt->xStartPos )
          {
          SetROP2(hDC, R2_NOT);  

          SelectObject( hDC, GetStockObject( BLACK_PEN ) );
          DeleteObject( hPen );
          }
        }

      // Save the current mouse position.
      lpCnt->xPrevPos = xNewRight;

      // Draw the new line.
      if( xNewRight )
        {
        MoveToEx( hDC, lpCnt->xPrevPos, yTop, NULL );
        LineTo( hDC, lpCnt->xPrevPos, yBottom );
        }
      }

    ReleaseDC(hWnd, hDC);

    return;
    }

/****************************************************************************
 * ResizeCntFld
 *
 * Purpose:
 *  Resizes a field and updates the container.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window.
 *  LPCONTAINER   lpCnt       - pointer to existing CONTAINER structure.
 *  int           xNewRight   - the x coord of the new right border
 *
 * Return Value:
 *  VOID
 ****************************************************************************/
VOID WINAPI ResizeCntFld( HWND hWnd, LPCONTAINER lpCnt, int xNewRight )
    {
    HWND        hWndSplit;
    LPCONTAINER  lpCntSplit;
    LPCONTAINER  lpCntFrame;
    LPFIELDINFO lpSizFld=lpCnt->lpSizMovFld;
    LPFIELDINFO lpCol;
    RECT  rect;
    BOOL  bFoundWidth;
    int   xLeft, xRight;
    int   cxChar = lpCnt->lpCI->cxChar;
    int   nNewWidth;

    if( lpSizFld )
      {
      // Get the left and right coords of this field.
/*      dwEdges = GetFieldXPos( lpCnt, lpSizFld );*/
/*      xLeft   = (int) LOWORD(dwEdges);*/
/*      xRight  = (int) HIWORD(dwEdges);*/
      GetFieldXPos( lpCnt, lpSizFld, &xLeft, &xRight );

      // If user is shrinking this field and it is already
      // hidden find the 1st non-hidden field before it.
      if( xNewRight < xRight && !lpSizFld->cxWidth )
        {
        // break out when we find a previous col with some width
        while( lpSizFld->lpPrev )
          {
          lpSizFld = lpSizFld->lpPrev;
          if( lpSizFld->cxWidth )
            break;  
          }

        // Get the left and right coords of this new field.
/*        dwEdges = GetFieldXPos( lpCnt, lpSizFld );*/
/*        xLeft   = (int) LOWORD(dwEdges);*/
/*        xRight  = (int) HIWORD(dwEdges);*/
        GetFieldXPos( lpCnt, lpSizFld, &xLeft, &xRight );
        }

      if( xNewRight <= xLeft )
        {
        // User has hidden this field so do a quick check to 
        // make sure all fields are not hidden.
        bFoundWidth = FALSE;
        lpCol = CntFldHeadGet( hWnd );
        while( lpCol )
          {
          // break out when next col has some width
          if( lpCol != lpSizFld && lpCol->cxWidth )
            {
            bFoundWidth = TRUE;
            break;  
            }
          lpCol = lpCol->lpNext;
          }

        // If user is trying to hide all fields, set last one to 1.
        if( bFoundWidth )
          nNewWidth = 0;
        else
          nNewWidth = 1;
        }
      else if( xNewRight < xLeft + cxChar )
        {
        // new size is less than a char so set width to 1 char
        nNewWidth = 1;
        }
      else if( xNewRight < xRight )
        {
        // calc widths in integral multiples of char widths
        nNewWidth =  (xNewRight - xLeft + cxChar) / cxChar;
        }
      else
        {
        // set new width always in char width multiples
        nNewWidth =  (xNewRight - xLeft) / cxChar;
        }

      // Set new field width, re-calc container metrics, and repaint.
      if( nNewWidth != (int)lpSizFld->cxWidth )
        {
        lpSizFld->cxWidth = nNewWidth;
  
        // Notify now to allow assoc to check out the new size before painting.
        NotifyAssocWnd( hWnd, lpCnt, CN_FLDSIZED, 0, NULL, lpSizFld, 0, 0, 0, NULL );
  
        InitTextMetrics( hWnd, lpCnt );
  
        // Notify the user of the change in field size.
        NotifyAssocWnd( hWnd, lpCnt, CN_FLDSIZECHANGED, 0, NULL, lpSizFld, 0, 0, 0, NULL );

        rect.left   = 0;
        rect.top    = lpCnt->lpCI->cyTitleArea;
        rect.right  = lpCnt->cxClient;
        rect.bottom = lpCnt->cyClient;
        UpdateContainer( hWnd, &rect, TRUE );

        // If we are split update the other view's field list.
        if( lpCnt->dwStyle & CTS_SPLITBAR && lpCnt->bIsSplit )
          {
          // Get the split window handles from the frame's stuct.
          lpCntFrame = GETCNT( lpCnt->hWndFrame );
          hWndSplit = hWnd == lpCntFrame->SplitBar.hWndLeft ? lpCntFrame->SplitBar.hWndRight : lpCntFrame->SplitBar.hWndLeft;

          // Get the other view's client area.
          lpCntSplit = GETCNT( hWndSplit );
          rect.right  = lpCntSplit->cxClient;
          rect.bottom = lpCntSplit->cyClient;

          // Re-initialize the container metrics.
          InitTextMetrics( hWndSplit, lpCntSplit );

          UpdateContainer( hWndSplit, &rect, TRUE );
          }

        // If user has hidden the focus field move it to the right.
        if( lpSizFld == lpCnt->lpCI->lpFocusFld && !lpSizFld->cxWidth )
          FORWARD_WM_KEYDOWN( hWnd, VK_RIGHT, 0, 0, SendMessage );
        }
      }

    return;
    }

/****************************************************************************
 * InitMovingRect
 *
 * Purpose:
 *  Initializes the moving rect for a field about to be dynamically moved.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window.
 *  LPCONTAINER   lpCnt       - pointer to existing CONTAINER structure.
 *  LPFIELDINFO   lpCol       - pointer to the field about to be moved
 *  LPPOINT       lpPt        - the x/y coord of the initial mouse position
 *
 * Return Value:
 *  VOID
 ****************************************************************************/
VOID WINAPI InitMovingRect( HWND hWnd, LPCONTAINER lpCnt, LPFIELDINFO lpCol, LPPOINT lpPt )
    {
    int   xLeft, xRight;
    
    // Get the left and right coords of this field.
/*    dwEdges = GetFieldXPos( lpCnt, lpCol );*/
/*    xLeft   = (int) LOWORD(dwEdges);*/
/*    xRight  = (int) HIWORD(dwEdges);*/
    GetFieldXPos( lpCnt, lpCol, &xLeft, &xRight );

    // Get the right border of the last column.
/*    lpCnt->xLastCol = lpCnt->lpCI->nMaxWidth - lpCnt->nHscrollPos * lpCnt->lpCI->cxChar;*/

    // Set to true after 1st draw.
    lpCnt->bMoving = FALSE;

    // Save the origin, width, and length of this field.
    lpCnt->xStartPos = lpPt->x;
    lpCnt->yStartPos = lpPt->y;
    lpCnt->cxMoveFld = lpCol->cxWidth * lpCnt->lpCI->cxChar;
    lpCnt->cyMoveFld = lpCnt->cyClient - lpCnt->lpCI->cyTitleArea + 1;

    // Save the offset from the original mouse pos to the field origin.
    lpCnt->xOffMove = lpPt->x - xLeft + 1;
    lpCnt->yOffMove = lpPt->y - lpCnt->lpCI->cyTitleArea + 1;

    return;
    }

/****************************************************************************
 * DrawMovingRect
 *
 * Purpose:
 *  Draws the moving rect while a field is being dynamically moved.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window.
 *  LPCONTAINER   lpCnt       - pointer to existing CONTAINER structure.
 *  LPPOINT       lpPt        - the x/y coord of the new mouse position
 *                              (if NULL only the previous rect is removed)
 * Return Value:
 *  VOID
 ****************************************************************************/
VOID WINAPI DrawMovingRect( HWND hWnd, LPCONTAINER lpCnt, LPPOINT lpPt )
    {
    HDC  hDC = GetDC(hWnd);
    HPEN hPen;
    BOOL bOnVertGrid=FALSE;
    BOOL bOnHorzGrid=FALSE;
    
    // Erase the previous rectangle.
    if( lpCnt->bMoving )
      {
      // If we are on the original horz or vert col grid lines, set a flag.
      if( lpCnt->xPrevPos == lpCnt->xStartPos &&
         (lpCnt->lpCI->flCntAttr & CA_VERTFLDSEP ||
          lpCnt->lpSizMovFld->flColAttr & CFA_VERTSEPARATOR) )
        bOnVertGrid=TRUE;
      if( lpCnt->yPrevPos == lpCnt->yStartPos &&
          lpCnt->lpCI->flCntAttr & CA_TTLSEPARATOR )
        bOnHorzGrid=TRUE;

      // If we are on the field's grid line create a pen for redrawing it.
      if( bOnVertGrid || bOnHorzGrid )
        {
        // Create a grid colored pen.
        hPen = CreatePen( PS_SOLID, 1, lpCnt->crColor[CNTCOLOR_GRID] );
        }

      // If we are passing over the field's original horz grid
      // lines do a straight redraw instead of a pen inversion.
      if( bOnHorzGrid )
        SelectObject( hDC, hPen );
      else
        SetROP2(hDC, R2_NOT);  

      if( bOnHorzGrid && lpCnt->rectMoveFld.right > lpCnt->xLastCol-1 )
        {
        // Have to do part inverse, part straight draw.
        MoveToEx( hDC, lpCnt->rectMoveFld.left,  lpCnt->rectMoveFld.top, NULL );
        LineTo( hDC, lpCnt->xLastCol,          lpCnt->rectMoveFld.top );
        SetROP2(hDC, R2_NOT);  
        LineTo( hDC, lpCnt->rectMoveFld.right, lpCnt->rectMoveFld.top    );
        SetROP2( hDC, R2_COPYPEN );  

        if( lpCnt->lpCI->flCntAttr & CA_FLDSEPARATOR ||
            lpCnt->lpSizMovFld->flColAttr & CFA_HORZSEPARATOR )
          {
          MoveToEx( hDC, lpCnt->rectMoveFld.left+1, lpCnt->rectMoveFld.top + lpCnt->lpCI->cyColTitleArea, NULL );
          LineTo( hDC, lpCnt->xLastCol,           lpCnt->rectMoveFld.top + lpCnt->lpCI->cyColTitleArea );
          SetROP2(hDC, R2_NOT);  
          LineTo( hDC, lpCnt->rectMoveFld.right,  lpCnt->rectMoveFld.top + lpCnt->lpCI->cyColTitleArea );
          }

        MoveToEx( hDC, lpCnt->rectMoveFld.right, lpCnt->rectMoveFld.bottom, NULL );
        LineTo( hDC, lpCnt->xLastCol-1,        lpCnt->rectMoveFld.bottom );
        SetROP2( hDC, R2_COPYPEN );  
        LineTo( hDC, lpCnt->rectMoveFld.left,  lpCnt->rectMoveFld.bottom );
        }
      else
        {
        MoveToEx( hDC, lpCnt->rectMoveFld.left,  lpCnt->rectMoveFld.top, NULL );
        LineTo( hDC, lpCnt->rectMoveFld.right, lpCnt->rectMoveFld.top );

        if( lpCnt->lpCI->flCntAttr & CA_FLDSEPARATOR ||
            lpCnt->lpSizMovFld->flColAttr & CFA_HORZSEPARATOR )
          {
          MoveToEx( hDC, lpCnt->rectMoveFld.left+1, lpCnt->rectMoveFld.top + lpCnt->lpCI->cyColTitleArea, NULL );
          LineTo( hDC, lpCnt->rectMoveFld.right,  lpCnt->rectMoveFld.top + lpCnt->lpCI->cyColTitleArea );
          }

        MoveToEx( hDC, lpCnt->rectMoveFld.right, lpCnt->rectMoveFld.bottom, NULL );
        LineTo( hDC, lpCnt->rectMoveFld.left,  lpCnt->rectMoveFld.bottom );
        }

      // If we are passing over the field's original vert grid
      // lines do a straight redraw instead of a pen inversion.
      if( bOnVertGrid )
        {
        SelectObject( hDC, hPen );
        SetROP2( hDC, R2_COPYPEN );  

        if( lpCnt->lpCI->cyTitleArea &&
           (lpCnt->rectMoveFld.top < lpCnt->lpCI->cyTitleArea-1) )
          {        
          // Have to do part inverse, part straight draw.
          LineTo( hDC, lpCnt->rectMoveFld.left,  lpCnt->lpCI->cyTitleArea-2 );
          SetROP2(hDC, R2_NOT);  
          LineTo( hDC, lpCnt->rectMoveFld.left,  lpCnt->rectMoveFld.top    );
          MoveToEx( hDC, lpCnt->rectMoveFld.right, lpCnt->rectMoveFld.top, NULL );
          LineTo( hDC, lpCnt->rectMoveFld.right, lpCnt->lpCI->cyTitleArea-1 );
          SetROP2( hDC, R2_COPYPEN );  
          LineTo( hDC, lpCnt->rectMoveFld.right, lpCnt->rectMoveFld.bottom );
          }
        else
          {
          LineTo( hDC, lpCnt->rectMoveFld.left,  lpCnt->rectMoveFld.top    );
          MoveToEx( hDC, lpCnt->rectMoveFld.right, lpCnt->rectMoveFld.top, NULL );
          LineTo( hDC, lpCnt->rectMoveFld.right, lpCnt->rectMoveFld.bottom );
          }
        }
      else
        {
        SetROP2(hDC, R2_NOT);  

        LineTo( hDC, lpCnt->rectMoveFld.left,  lpCnt->rectMoveFld.top    );
        MoveToEx( hDC, lpCnt->rectMoveFld.right, lpCnt->rectMoveFld.top, NULL );
        LineTo( hDC, lpCnt->rectMoveFld.right, lpCnt->rectMoveFld.bottom );
        }

      if( bOnVertGrid || bOnHorzGrid )
        {
        SetROP2(hDC, R2_NOT);  

        SelectObject( hDC, GetStockObject( BLACK_PEN ) );
        DeleteObject( hPen );
        }
      }

    if( lpPt )
      {
      // Save the current mouse position and setup the moving rectangle.
      lpCnt->bMoving = TRUE;
      lpCnt->xPrevPos = lpPt->x;
      lpCnt->yPrevPos = lpPt->y;

      lpCnt->rectMoveFld.left   = lpCnt->xPrevPos - lpCnt->xOffMove;
      lpCnt->rectMoveFld.right  = lpCnt->rectMoveFld.left + lpCnt->cxMoveFld;
      lpCnt->rectMoveFld.top    = lpCnt->yPrevPos - lpCnt->yOffMove;
      lpCnt->rectMoveFld.bottom = lpCnt->rectMoveFld.top + lpCnt->cyMoveFld;

      // Draw the new rectangle.
      SetROP2(hDC, R2_NOT);  
      MoveToEx( hDC, lpCnt->rectMoveFld.left,  lpCnt->rectMoveFld.top, NULL );
      LineTo( hDC, lpCnt->rectMoveFld.right, lpCnt->rectMoveFld.top );

      if( lpCnt->lpCI->flCntAttr & CA_FLDSEPARATOR ||
          lpCnt->lpSizMovFld->flColAttr & CFA_HORZSEPARATOR )
        {
        MoveToEx( hDC, lpCnt->rectMoveFld.left+1, lpCnt->rectMoveFld.top + lpCnt->lpCI->cyColTitleArea, NULL );
        LineTo( hDC, lpCnt->rectMoveFld.right,  lpCnt->rectMoveFld.top + lpCnt->lpCI->cyColTitleArea );
        }

      MoveToEx( hDC, lpCnt->rectMoveFld.right, lpCnt->rectMoveFld.bottom, NULL );
      LineTo( hDC, lpCnt->rectMoveFld.left,  lpCnt->rectMoveFld.bottom );

      LineTo( hDC, lpCnt->rectMoveFld.left,  lpCnt->rectMoveFld.top    );
      MoveToEx( hDC, lpCnt->rectMoveFld.right, lpCnt->rectMoveFld.top, NULL );
      LineTo( hDC, lpCnt->rectMoveFld.right, lpCnt->rectMoveFld.bottom );
      }

    ReleaseDC(hWnd, hDC);

    return;
    }

/****************************************************************************
 * ReorderCntFld
 *
 * Purpose:
 *  Reorders the fields and updates the container.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window.
 *  LPCONTAINER   lpCnt       - pointer to existing CONTAINER structure.
 *  int           xNewPos     - the ending mouse x coord for the moved fld
 *                              (will add offset to get the new left border)
 * Return Value:
 *  VOID
 ****************************************************************************/
VOID WINAPI ReorderCntFld( HWND hWnd, LPCONTAINER lpCnt, int xNewPos )
    {
    HWND        hWndSplit;
    LPCONTAINER  lpCntSplit;
    LPCONTAINER  lpCntFrame;
    LPFIELDINFO lpCol;
    RECT  rect;
    POINT pt;
    int   xStr, xEnd;
    BOOL  bMovedFld=FALSE;
    BOOL  bMoveFocus=FALSE;
    BOOL  bInsBeforeHead=FALSE;

    if( lpCnt->lpSizMovFld )
      {
      lpCntFrame = GETCNT( lpCnt->hWndFrame );

      lpCntFrame->bMovingFld = TRUE;

      // Get the current field at this x coord. The field to be moved will
      // be inserted AFTER the field found at this position.
      pt.x = xNewPos - lpCnt->xOffMove;
      pt.y = 0;
      lpCol = GetFieldAtPt( lpCnt, &pt, &xStr, &xEnd );
  
      // If it wasn't found then it was dropped before 1st or after last col.
      if( !lpCol )
        {
        if( pt.x <= 0 )
          {
          lpCol = CntFldHeadGet( hWnd );

          // Can insert before the head only if it is moveable.
          if( IsFldMoveable( lpCnt, lpCol ) )
            bInsBeforeHead = TRUE;
          }
        else
          {
          lpCol = CntFldTailGet( hWnd );
          }
        }
  
      // If this isn't the tail and all fields are not moveable we have
      // to check for the next non-moveable field to insert after.
      if( lpCol->lpNext && lpCnt->lpSizMovFld != lpCol && lpCnt->lpSizMovFld != lpCol->lpNext )
        {
        while( lpCol->lpNext )
          {
          // Break when we find the next moveable column.
          if( IsFldMoveable( lpCnt, lpCol->lpNext ) )
            break;  
    
          lpCol = lpCol->lpNext;       // advance to next column
          }
        }

      // If it didn't end up on itself or just before itself,
      // move it after the field at xNewPos.
      if( lpCol && lpCnt->lpSizMovFld != lpCol )
        {
        // If we are moving the focus field we must reset it after
        // moving it. CntRemoveFld will move the focus off the field
        // being removed. We want the focus to stay with the moved field
        // if that field is the focus field.
        if( lpCnt->lpSizMovFld == lpCnt->lpCI->lpFocusFld )
          bMoveFocus = TRUE;

        if( bInsBeforeHead )
          {
          CntRemoveFld( hWnd, lpCnt->lpSizMovFld );
          CntAddFldHead( hWnd, lpCnt->lpSizMovFld );
          bMovedFld = TRUE;
          }
        else if( lpCnt->lpSizMovFld != lpCol->lpNext )
          {
          CntRemoveFld( hWnd, lpCnt->lpSizMovFld );
          CntInsFldAfter( hWnd, lpCol, lpCnt->lpSizMovFld );
          bMovedFld = TRUE;
          }
  
          // Repaint the container if the field was moved.
        if( bMovedFld )
          {
          if( bMoveFocus )
            {
            // Unfocus the old focus field.
            lpCnt->lpCI->lpFocusFld->flColAttr &= ~CFA_CURSORED;

            // Reset the focus on the moved field if it was the focus field.
            lpCnt->lpCI->lpFocusFld = lpCnt->lpSizMovFld;
            lpCnt->lpCI->lpFocusFld->flColAttr |= CFA_CURSORED;
            }

          // Notify now to allow assoc to check out new order before painting.
          NotifyAssocWnd( hWnd, lpCnt, CN_FLDMOVED, 0, NULL, lpCnt->lpSizMovFld, 0, 0, 0, NULL );
 
          rect.left   = 0;
          rect.top    = lpCnt->lpCI->cyTitleArea;
          rect.right  = lpCnt->cxClient;
          rect.bottom = lpCnt->cyClient;
          UpdateContainer( hWnd, &rect, TRUE );

          // If we are split update the other view's field list.
          if( lpCnt->dwStyle & CTS_SPLITBAR && lpCnt->bIsSplit )
            {
            // Get the split window handles from the frame's stuct.
            hWndSplit = hWnd == lpCntFrame->SplitBar.hWndLeft ? lpCntFrame->SplitBar.hWndRight : lpCntFrame->SplitBar.hWndLeft;

            // Get the other view's client area.
            lpCntSplit = GETCNT( hWndSplit );
            rect.right  = lpCntSplit->cxClient;
            rect.bottom = lpCntSplit->cyClient;
            UpdateContainer( hWndSplit, &rect, TRUE );
            }
          }
        }

      lpCntFrame->bMovingFld = FALSE;
      }

    return;
    }

/****************************************************************************
 * Get1stFldInView
 *
 * Purpose:
 *  Gets the 1st field which is viewable in the display area.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the control window.
 *  LPCONTAINER   lpCnt       - pointer to existing CONTAINER structure.
 *
 * Return Value:
 *  LPFIELDINFO   pointer to the 1st field viewable in the display area
 ****************************************************************************/

LPFIELDINFO WINAPI Get1stFldInView( HWND hWnd, LPCONTAINER lpCnt )
    {
    LPFIELDINFO  lpCol;
    int          cxCol, xStrCol, xEndCol;

    xStrCol = -lpCnt->nHscrollPos * lpCnt->lpCI->cxChar;

    lpCol = lpCnt->lpCI->lpFieldHead;  // point at 1st column
    while( lpCol )
      {
      cxCol   = lpCol->cxWidth * lpCnt->lpCI->cxChar;
      xEndCol = xStrCol + cxCol;
    
      // break when we find the 1st column which ends in the display area
      if( xEndCol > 0 )
        break;  

      xStrCol += cxCol;
      lpCol = lpCol->lpNext;       // advance to next column
      }

    return lpCol;
    }

/****************************************************************************
 * GetLastFldInView
 *
 * Purpose:
 *  Gets the last field which is viewable in the display area.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the control window.
 *  LPCONTAINER   lpCnt       - pointer to existing CONTAINER structure.
 *
 * Return Value:
 *  LPFIELDINFO   pointer to the last field viewable in the display area
 ****************************************************************************/

LPFIELDINFO NEAR GetLastFldInView( HWND hWnd, LPCONTAINER lpCnt )
    {
    LPFIELDINFO  lpCol;
    int          xStrCol, xEndCol;

    xStrCol = -lpCnt->nHscrollPos * lpCnt->lpCI->cxChar;

    lpCol = lpCnt->lpCI->lpFieldHead;  // point at 1st column
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
 * IsRecInView
 *
 * Purpose:
 *  Determines whether a record is viewable in the display area.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the control window.
 *  LPCONTAINER   lpCnt       - pointer to existing CONTAINER structure.
 *  LPRECORDCORE  lpRec       - pointer to the record
 *
 * Return Value:
 *  BOOL          TRUE if any part of the record is viewable
 *                FALSE if no part of the record is viewable
 ****************************************************************************/

BOOL WINAPI IsRecInView( HWND hWnd, LPCONTAINER lpCnt, LPRECORDCORE lpRecord )
    {
    LPRECORDCORE lpRec;
    BOOL         bIsInView=FALSE;
    int          i;

    // Make sure we have a record pointer.
    if( !lpRecord )
      return bIsInView;

    // Look for the record in the display area.
    lpRec = lpCnt->lpCI->lpTopRec;
    for( i=0; i < lpCnt->lpCI->nRecsDisplayed && lpRec; i++ )
      {
      if( lpRec == lpRecord )
        {
        bIsInView = TRUE;
        break;
        }
      // advance to next data record
      lpRec = lpRec->lpNext;
      }

    return bIsInView;
    }

/****************************************************************************
 * IsFldMoveable
 *
 * Purpose:
 *  Determines whether a field is dynamically moveable.
 *
 * Parameters:
 *  LPCONTAINER   lpCnt       - pointer to existing CONTAINER structure
 *  LPFIELDINFO   lpFld       - pointer to the field
 *
 * Return Value:
 *  BOOL          TRUE  if the field is moveable
 *                FALSE if the field is NOT moveable
 ****************************************************************************/

BOOL NEAR IsFldMoveable( LPCONTAINER lpCnt, LPFIELDINFO lpFld )
    {
    BOOL bIsMoveable=FALSE;

    if( lpFld )
      {
      if( !(lpFld->flColAttr & CFA_NONMOVEABLEFLD) )
        {
        if( lpCnt->lpCI->flCntAttr & CA_MOVEABLEFLDS ||
            lpFld->flColAttr & CFA_MOVEABLEFLD )
          bIsMoveable = TRUE;
        }
      }

    return bIsMoveable;
    }

/****************************************************************************
 * IsFldInView
 *
 * Purpose:
 *  Determines whether a field is viewable in the display area.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the control window.
 *  LPCONTAINER   lpCnt       - pointer to existing CONTAINER structure.
 *  LPFIELDINFO   lpFld       - pointer to the field
 *
 * Return Value:
 *  BOOL          TRUE if any part of the field is viewable
 *                FALSE if no part of the field is viewable
 ****************************************************************************/

BOOL WINAPI IsFldInView( HWND hWnd, LPCONTAINER lpCnt, LPFIELDINFO lpFld )
    {
    LPFIELDINFO  lpCol;
    BOOL         bIsInView=FALSE;
    int          cxCol, xStrCol, xEndCol;

    if( lpFld )
      {
      xStrCol = -lpCnt->nHscrollPos * lpCnt->lpCI->cxChar;

      lpCol = lpCnt->lpCI->lpFieldHead;  // point at 1st column
      while( lpCol )
        {
        cxCol   = lpCol->cxWidth * lpCnt->lpCI->cxChar;
        xEndCol = xStrCol + cxCol;
    
        // break out when we find the column
        if( lpCol == lpFld )
          break;  

        xStrCol += cxCol;
        lpCol = lpCol->lpNext;       // advance to next column
        }

      // If either of these is true the field is not viewable in the client.
      if( xStrCol >= lpCnt->cxClient || xEndCol <= 0 )
        bIsInView = FALSE;
      else
        bIsInView = TRUE;
      }

    return bIsInView;
    }

/****************************************************************************
 * BlkSelectFld
 *
 * Purpose:
 *  Selects the specified fields when block selection is active.
 *
 * Parameters:
 *  HWND          hWnd        - Container child window handle 
 *  LPCONTAINER   lpCnt       - pointer to window's CONTAINER structure.
 *  LPRECORDCORE  lpRec       - pointer to starting record to be selected
 *  LPFIELDINFO   lpFld       - pointer to starting field
 *  int           nSelRecs    - number of records to select
 *  int           nSelFlds    - number of fields to select
 *  LPRECORDCORE  lpRecLast   - pointer to final record that was selected
 *  LPFIELDINFO   lpFldLast   - pointer to final field that was selected
 *                              
 * Return Value:
 *  BOOL          TRUE  if selection was set with no errors
 *                FALSE if selection could not be set
 ****************************************************************************/

BOOL NEAR BlkSelectFld( HWND hWnd, LPCONTAINER lpCnt, LPRECORDCORE lpRec,
                        LPFIELDINFO lpFld, int nSelRecs, int nSelFlds,
                        LPRECORDCORE FAR *lpRecLast, LPFIELDINFO FAR *lpFldLast )
    {
    LPRECORDCORE lpRec2;
    LPFIELDINFO  lpFld2;
    int  i, j;
    int  nSelectedRecs=nSelRecs;
    int  nSelectedFlds=nSelFlds;

    if( !lpRec || !lpFld )
      return FALSE;

    if( nSelRecs < 0 )
      nSelectedRecs = -nSelectedRecs;
    if( nSelFlds < 0 )
      nSelectedFlds = -nSelectedFlds;

    lpRec2 = *lpRecLast = lpRec;
    for( i=0; lpRec2 && i<nSelectedRecs; i++ )
      {
      lpFld2 = *lpFldLast = lpFld;
      for( j=0; lpFld2 && j<nSelectedFlds; j++ )
        {
        // Toggle the selection state of this field.
        if( CntIsFldSelected( hWnd, lpRec2, lpFld2 ) )
          {
          CntUnSelectFld( hWnd, lpRec2, lpFld2 );
          }
        else
          {
          CntSelectFld( hWnd, lpRec2, lpFld2 );
          }

        // Update the return field pointer.
        *lpFldLast = lpFld2;

        // Get the next or previous field.
        if( nSelFlds > 0 )
          lpFld2 = CntNextFld( lpFld2 );
        else
          lpFld2 = CntPrevFld( lpFld2 );
        }

      // Update the return record pointer.
      *lpRecLast = lpRec2;

      // Get the next or previous record.
      if( nSelRecs > 0 )
        lpRec2 = CntNextRec( lpRec2 );
      else
        lpRec2 = CntPrevRec( lpRec2 );
      }

    return TRUE;
    }

/****************************************************************************
 * ExtSelectRec
 *
 * Purpose:
 *  Selects the specified record when extended selection is active.
 *  This is a slightly optimized version of CntSelectRec.
 *
 * Parameters:
 *  HWND          hWnd        - Container child window handle 
 *  LPCONTAINER   lpCnt       - pointer to window's CONTAINER structure.
 *  LPRECORDCORE  lpRec       - pointer to starting record to be selected
 *  int           nSelRecs    - number of records to select
 *  LPRECORDCORE  lpRecLast   - pointer to final record that was selected
 *  BOOL          bShiftKey   - state of the shift key - added for Unicenter
 *
 * Return Value:
 *  BOOL          TRUE  if selection was set with no errors
 *                FALSE if selection could not be set
 ****************************************************************************/

BOOL ExtSelectRec( HWND hWnd, LPCONTAINER lpCnt, LPRECORDCORE lpRec, int nSelRecs, 
                   LPRECORDCORE FAR *lpRecLast, BOOL bShiftKey )
{
    LPRECORDCORE lpRec2;
    UINT wEvent;
    int  i;
    int  nSelectedRecs=nSelRecs;

    if( nSelRecs < 0 )
      nSelectedRecs = -nSelectedRecs;
 
    lpRec2 = *lpRecLast = lpRec;
    for( i=0; lpRec2 && i<nSelectedRecs; i++ )
      {
      // Toggle the selection state of this record.
      if( lpRec2->flRecAttr & CRA_SELECTED )
        {
        lpRec2->flRecAttr &= ~CRA_SELECTED;
        wEvent = CN_RECUNSELECTED;
        }
      else
        {
        lpRec2->flRecAttr |= CRA_SELECTED;
        wEvent = CN_RECSELECTED;
        }

      // Repaint the toggled record and notify the parent.
      InvalidateCntRecord( hWnd, lpCnt, lpRec2, NULL, 1 );
      
      if (bShiftKey && i==nSelectedRecs-1)
        NotifyAssocWnd( hWnd, lpCnt, wEvent, 0, lpRec2, NULL, 0, bShiftKey, 0, NULL );
      else
        NotifyAssocWnd( hWnd, lpCnt, wEvent, 0, lpRec2, NULL, 0, 0, 0, NULL );
  
      // Update the return pointer.
      *lpRecLast = lpRec2;

      // Get the next or previous record.
      if( nSelRecs > 0 )
        lpRec2 = CntNextRec( lpRec2 );
      else
        lpRec2 = CntPrevRec( lpRec2 );
      }

    return TRUE;
    }

/****************************************************************************
 * FindFocusRec
 *
 * Purpose:
 *  Determines how far away in the record list the focus record is from a
 *  specified record. The return value sign indicates whether the focus is
 *  forward (positive) or backward (negative) in the list. If the focus
 *  record is the same as the passed in record or is not found in the list
 *  the return value will be 0.
 *
 *  Deciding which direction to look first could probably be optimized
 *  somehow in the future but for now I am not worrying about it.
 *
 * Parameters:
 *  HWND          hWnd        - Container child window handle 
 *  LPCONTAINER   lpCnt       - pointer to window's CONTAINER structure.
 *  LPRECORDCORE  lpRec       - pointer to starting record to be selected
 *
 * Return Value:
 *  int           distance to the focus record (either positive or negative)
 ****************************************************************************/

int FindFocusRec( HWND hWnd, LPCONTAINER lpCnt, LPRECORDCORE lpRec )
    {
    LPRECORDCORE lpRec2=lpRec;
    BOOL         bFound=FALSE;
    int          nInc=0;

    // If no focus record or we are already on it return 0.
    if( !lpCnt->lpCI->lpFocusRec || lpCnt->lpCI->lpFocusRec == lpRec )
      return nInc;

    // First look forward in the list for the focus record.
    while( lpRec2 )
      {
      if( lpRec2 == lpCnt->lpCI->lpFocusRec )
        {
        bFound = TRUE;
        break;
        }
      lpRec2 = lpRec2->lpNext;
      nInc++;
      }

    // If not found forward in the list look backwards.
    if( !bFound )
      {
      lpRec2 = lpRec;
      nInc = 0;
      while( lpRec2 )
        {
        if( lpRec2 == lpCnt->lpCI->lpFocusRec )
          {
          bFound = TRUE;
          break;
          }
        lpRec2 = lpRec2->lpPrev;
        nInc--;
        }
      }

    // If not found it must be currently swapped off the record list.
    if( !bFound )
      nInc = 0;

    return nInc;
    }

/****************************************************************************
 * FindFocusFld
 *
 * Purpose:
 *  Determines how far away in the field list the focus field is from a
 *  specified field. The return value sign indicates whether the focus is
 *  forward (positive) or backward (negative) in the list. If the focus
 *  field is the same as the passed in field or is not found in the list
 *  the return value will be 0.
 *
 * Parameters:
 *  HWND          hWnd        - Container child window handle 
 *  LPCONTAINER   lpCnt       - pointer to window's CONTAINER structure.
 *  LPFIELDINFO   lpFld       - pointer to starting field
 *
 * Return Value:
 *  int           distance to the focus field (either positive or negative)
 ****************************************************************************/

int NEAR FindFocusFld( HWND hWnd, LPCONTAINER lpCnt, LPFIELDINFO lpFld )
    {
    LPFIELDINFO lpFld2=lpFld;
    BOOL        bFound=FALSE;
    int         nInc=0;

    // If no focus field or we are already on it return 0.
    if( !lpCnt->lpCI->lpFocusFld || lpCnt->lpCI->lpFocusFld == lpFld )
      return nInc;

    // First look forward in the list for the focus field.
    while( lpFld2 )
      {
      if( lpFld2 == lpCnt->lpCI->lpFocusFld )
        {
        bFound = TRUE;
        break;
        }
      lpFld2 = lpFld2->lpNext;
      nInc++;
      }

    // If not found forward in the list look backwards.
    if( !bFound )
      {
      lpFld2 = lpFld;
      nInc = 0;
      while( lpFld2 )
        {
        if( lpFld2 == lpCnt->lpCI->lpFocusFld )
          {
          bFound = TRUE;
          break;
          }
        lpFld2 = lpFld2->lpPrev;
        nInc--;
        }
      }

    // It should always be found but just in case...
    if( !bFound )
      nInc = 0;

    return nInc;
    }

/****************************************************************************
 * TrackSelection
 *
 * Purpose:
 *  Tracks record and/or field selection for Extended or Block selection
 *  modes. Does all the necessary mouse tracking and screen inversion.
 *  The record and field return value signs indicate whether selection was
 *  forward (positive) or backward (negative) in the list.
 *
 * Parameters:
 *  HWND          hWnd        - Container child window handle 
 *  LPCONTAINER   lpCnt       - pointer to window's CONTAINER structure.
 *  LPRECORDCORE  lpRec       - pointer to starting record to be selected
 *  LPFIELDINFO   lpFld       - pointer to starting field
 *  LPPOINT       lpPt        - selection starting point
 *  LPINT         lpSelRecs   - returned number of selected records
 *  LPINT         lpSelFlds   - returned number of selected fields
 *  BOOL          bTrackFlds  - flag for tracking fields 
 *
 * Return Value:
 *  BOOL          TRUE  if selection was tracked with no errors
 *                FALSE if selection could not be tracked
 ****************************************************************************/

BOOL NEAR TrackSelection( HWND hWnd, LPCONTAINER lpCnt, LPRECORDCORE lpRec, LPFIELDINFO lpFld, LPPOINT lpPt,
                          LPINT lpSelRecs, LPINT lpSelFlds, BOOL bTrackFlds )
    {
    LPRECORDCORE lpTopRec;
    LPRECORDCORE lpBotRec;
    LPFIELDINFO  lpCurFld=lpFld;
    LPFIELDINFO  lpFld2;
    HDC     hDC;
    MSG     msgModal;
    HCURSOR hOldCursor;
    RECT    rectOrig, rect;
    POINT   pt, ptPrev, ptLast;
    BOOL    bTracking=TRUE;
    BOOL    bDoInvert;
    int     i, nWidth, nHeight, nLastRecBottom;
    int     nSelectedRecs=0;
    int     nSelectedFlds=0;
    int     nHscrollPos, cxScrolledPxls;
#ifdef WIN32
    POINTS  pts;
#endif

    if( !lpRec || !lpFld )
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

    // Remember the starting record and field.
    lpCnt->lp1stSelRec = lpRec;
    lpCnt->lp1stSelFld = lpFld;

    // Calc the where the bottom of the last record will be if
    // it gets scrolled into view. After every scroll we will have
    // to check to see if the last record has been hit. If it has we
    // have to adjust the bottom of the record area.
    nLastRecBottom = lpCnt->lpCI->cyTitleArea + lpCnt->lpCI->cyColTitleArea;
    nLastRecBottom += (lpCnt->lpCI->nRecsDisplayed-1) * lpCnt->lpCI->cyRow;

    // Get a pointer to the current bottom record.
    lpBotRec = lpCnt->lpCI->lpTopRec;
    for( i=0; i < lpCnt->lpCI->nRecsDisplayed-1 && lpBotRec; i++ )
      {
      if( lpBotRec->lpNext )
        lpBotRec = lpBotRec->lpNext;
      else
        break;
      }

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
    if(FALSE /*lpCnt->dwStyle & CTS_DRAGDROP || lpCnt->dwStyle & CTS_DRAGONLY */)
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

          if( PtInRect( &lpCnt->rectRecArea, pt ) )
            {
            // The cursor is in the record area so select
            // or unselect records as cursor passes over them.
            bDoInvert = FALSE;
            if( pt.y < ptPrev.y )   // the cursor is moving upward
              {
              if( bTrackFlds )
                {
                rect.left  = min(rectOrig.left,  lpCnt->rectCursorFld.left);
                rect.right = max(rectOrig.right, lpCnt->rectCursorFld.right);
                }

              if( pt.y < rectOrig.top )
                {
                if( pt.y < lpCnt->rectCursorFld.top )
                  {
                  // If the cursor is moving upward and is above the top
                  // of the original record it means we are selecting.

                  // Clean up any inversion left below the original bottom.
                  // If the cursor is above the original top it means the
                  // bottom of the inversion block should be the bottom of
                  // the original record clicked on. This situation will
                  // occur if the user does a very fast upward stroke with
                  // the mouse. We will get a MOUSEMOVE for a point below the
                  // original bottom and the next MOUSEMOVE point will be well
                  // above the bottom. Limitations of the mouse hardware.
                  if( lpCnt->rectCursorFld.bottom > rectOrig.bottom )
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
              if( bTrackFlds )
                {
                rect.left  = min(rectOrig.left,  lpCnt->rectCursorFld.left);
                rect.right = max(rectOrig.right, lpCnt->rectCursorFld.right);
                }

              if( pt.y > rectOrig.bottom )
                {
                if( pt.y > lpCnt->rectCursorFld.bottom )
                  {
                  // If the cursor is moving downward and is below the bottom
                  // of the original record it means we are selecting.

                  // Clean up any inversion left above the original top.
                  // If the cursor is below the original bottom it means
                  // the top of the inversion block should be the top of
                  // the original record clicked on. This situation will
                  // occur if the user does a very fast downward stroke with
                  // the mouse. We will get a MOUSEMOVE for a point above
                  // the original top and the next MOUSEMOVE point will be
                  // well below the top. Limitations of the mouse hardware.
                  if( lpCnt->rectCursorFld.top < rectOrig.top )
                    {
                    rect.top    = lpCnt->rectCursorFld.top;
                    rect.bottom = rectOrig.top;
                    PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                 rect.bottom - rect.top, DSTINVERT );
                    lpCnt->rectCursorFld.top = rectOrig.top;
                    }

                  // Expand the selected record block downward.
                  rect.top    = lpCnt->rectCursorFld.bottom;
                  while( pt.y > lpCnt->rectCursorFld.bottom )
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
                  // of the original record it means we are unselecting.

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
              if( bTrackFlds )
                {
                rect.left  = min(rectOrig.left,  lpCnt->rectCursorFld.left);
                rect.right = max(rectOrig.right, lpCnt->rectCursorFld.right);
                }

              PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                           rect.bottom - rect.top, DSTINVERT );
              }

            // Track the horizontal movement if tracking fields.
            if( bTrackFlds )
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
                      rect.left  = rectOrig.right;
                      rect.right = lpCnt->rectCursorFld.right;
                      PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                   rect.bottom - rect.top, DSTINVERT );
                      lpCnt->rectCursorFld.right = rectOrig.right;
                      }

                    // Expand the selected field block leftward.
                    rect.right = lpCnt->rectCursorFld.left;
                    while( pt.x < lpCnt->rectCursorFld.left && lpCurFld->lpPrev )
                      {
                      lpCurFld = lpCurFld->lpPrev;
                      lpCnt->rectCursorFld.left -= lpCurFld->cxPxlWidth;
                      }
                    rect.left = max(lpCnt->rectCursorFld.left, lpCnt->rectRecArea.left);
                    bDoInvert = TRUE;
                    }
                  }
                else
                  {
                  if( pt.x < (lpCnt->rectCursorFld.right - (int)lpCurFld->cxPxlWidth) )
                    {
                    // If the cursor is moving leftwards and is to the right
                    // of the original field it means we are unselecting.

                    // Decrease the selected field block leftward.
                    rect.right = lpCnt->rectCursorFld.right;
                    while( pt.x < (lpCnt->rectCursorFld.right - (int)lpCurFld->cxPxlWidth) && lpCurFld->lpPrev )
                      {
                      lpCnt->rectCursorFld.right -= lpCurFld->cxPxlWidth;
                      lpCurFld = lpCurFld->lpPrev;
                      }
                    rect.left = lpCnt->rectCursorFld.right;
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
                    // means the left edge of the inversion block should be
                    // the left edge of the original field clicked on. This
                    // situation will occur if the user does a very fast 
                    // rightward stroke with the mouse. We will get a MOUSEMOVE
                    // for a point to the left of the original field and the
                    // next MOUSEMOVE point will be well to the right of the 
                    // left edge of the original field. This is due to the
                    // limitations of the mouse hardware.
                    if( lpCnt->rectCursorFld.left < rectOrig.left )
                      {
                      rect.left  = lpCnt->rectCursorFld.left;
                      rect.right = rectOrig.left;
                      PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                   rect.bottom - rect.top, DSTINVERT );
                      lpCnt->rectCursorFld.left = rectOrig.left;
                      }

                    // Expand the selected field block rightward.
                    rect.left = lpCnt->rectCursorFld.right;
                    while( pt.x > lpCnt->rectCursorFld.right && lpCurFld->lpNext )
                      {
                      lpCurFld = lpCurFld->lpNext;
                      lpCnt->rectCursorFld.right += lpCurFld->cxPxlWidth;
                      }
                    rect.right = min(lpCnt->rectCursorFld.right, lpCnt->rectRecArea.right);
                    bDoInvert = TRUE;
                    }
                  }
                else
                  {
                  if( pt.x > (lpCnt->rectCursorFld.left + (int)lpCurFld->cxPxlWidth) )
                    {
                    // If the cursor is moving rightward and is to the left
                    // of the original field it means we are unselecting.

                    // Decrease the selected field block rightward.
                    rect.left = lpCnt->rectCursorFld.left;
                    while( pt.x > (lpCnt->rectCursorFld.left + (int)lpCurFld->cxPxlWidth) && lpCurFld->lpNext )
                      {
                      lpCnt->rectCursorFld.left += lpCurFld->cxPxlWidth;
                      lpCurFld = lpCurFld->lpNext;
                      }
                    rect.right = lpCnt->rectCursorFld.left;
                    bDoInvert = TRUE;
                    }
                  }
                }

              // Do the inversion if the cursor moved to a new field.
              if( bDoInvert )
                {
                rect.top    = max(lpCnt->rectRecArea.top ,min(rectOrig.top, lpCnt->rectCursorFld.top));
                rect.bottom = max(rectOrig.bottom, lpCnt->rectCursorFld.bottom);

                PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                             rect.bottom - rect.top, DSTINVERT );
                }
              }
            }
          else
            {
            // Cursor is out of the record area so do an auto-scroll.
            if( pt.y < lpCnt->rectRecArea.top )
              {
              if( bTrackFlds )
                {
                rect.left  = min(rectOrig.left,  lpCnt->rectCursorFld.left);
                rect.right = max(rectOrig.right, lpCnt->rectCursorFld.right);
                }

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
                // the original record clicked on. This situation will
                // occur if the user does a very fast upward stroke with
                // the mouse. We will get a MOUSEMOVE for a point below the
                // original bottom and the next MOUSEMOVE point will be well
                // above the bottom. Limitations of the mouse hardware.
                if( lpCnt->rectCursorFld.bottom > rectOrig.bottom )
                  {
                  rect.top    = rectOrig.bottom;
                  rect.bottom = lpCnt->rectCursorFld.bottom;
                  PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                               rect.bottom - rect.top, DSTINVERT );
                  lpCnt->rectCursorFld.bottom = rectOrig.bottom;
                  }

                // Clean up any non-inverted records between the top of
                // the inverted rect and the record area top. If the cursor
                // is above the record area top it means the top of the
                // inverted rect should be the record area top.
                // This situation could occur if the user does a very fast
                // upward stroke with the mouse. We will get a MOUSEMOVE for
                // a point within the record area and the next MOUSEMOVE point
                // could be above the record area. Mouse hardware limitations.
                if( lpCnt->rectCursorFld.top > lpCnt->rectRecArea.top )
                  {
                  rect.top    = lpCnt->rectRecArea.top;
                  rect.bottom = lpCnt->rectCursorFld.top;
                  PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                               rect.bottom - rect.top, DSTINVERT );
                  lpCnt->rectCursorFld.top = lpCnt->rectRecArea.top;
                  }
                }

              if( lpCnt->dwStyle & CTS_VERTSCROLL )
                {
                // Scroll the record area down 1 record.
                lpTopRec = lpCnt->lpCI->lpTopRec;
                FORWARD_WM_VSCROLL( hWnd, 0, SB_LINEUP, 0, SendMessage );
/*                SendMessage( hWnd, WM_VSCROLL, SB_LINEUP, 0L );*/

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
              if( bTrackFlds )
                {
                rect.left  = min(rectOrig.left,  lpCnt->rectCursorFld.left);
                rect.right = max(rectOrig.right, lpCnt->rectCursorFld.right);
                }

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
                // the original record clicked on. This situation will
                // occur if the user does a very fast downward stroke with
                // the mouse. We will get a MOUSEMOVE for a point above
                // the original top and the next MOUSEMOVE point will be
                // well below the top. Limitations of the mouse hardware.
                if( lpCnt->rectCursorFld.top < rectOrig.top )
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
                if( lpCnt->rectCursorFld.bottom < lpCnt->rectRecArea.bottom )
                  {
                  rect.top    = lpCnt->rectCursorFld.bottom;
                  rect.bottom = lpCnt->rectRecArea.bottom;
                  PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                               rect.bottom - rect.top, DSTINVERT );
                  while( lpCnt->rectCursorFld.bottom < lpCnt->rectRecArea.bottom )
                    lpCnt->rectCursorFld.bottom += lpCnt->lpCI->cyRow;
                  }
                }

              if( lpCnt->dwStyle & CTS_VERTSCROLL )
                {
                // Scroll the record area down 1 record.
                lpTopRec = lpCnt->lpCI->lpTopRec;
                FORWARD_WM_VSCROLL( hWnd, 0, SB_LINEDOWN, 0, SendMessage );
/*                SendMessage( hWnd, WM_VSCROLL, SB_LINEDOWN, 0L );*/

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
            if( bTrackFlds )
              {
              if( pt.x < lpCnt->rectRecArea.left )
                {
                rect.top    = max(lpCnt->rectRecArea.top, min(rectOrig.top, lpCnt->rectCursorFld.top));
                rect.bottom = max(rectOrig.bottom, lpCnt->rectCursorFld.bottom);

                if( rectOrig.right < lpCnt->rectRecArea.left )
                  {
                  // Clean up any non-inverted flds right of the orig field.
                  // If the cursor is at the left of the record area and the
                  // original right is still left the right edge of the 
                  // inversion block should be the left edge of the record
                  // area. In other words there should be no visible 
                  // inverted fields.
                  if( lpCnt->rectCursorFld.right > lpCnt->rectRecArea.left )
                    {
                    rect.left  = lpCnt->rectRecArea.left;
                    rect.right = lpCnt->rectCursorFld.right;
                    PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                 rect.bottom - rect.top, DSTINVERT );

                    while( lpCnt->rectCursorFld.right > lpCnt->rectRecArea.left && lpCurFld->lpPrev )
                      {
                      lpCnt->rectCursorFld.right -= lpCurFld->cxPxlWidth;
                      lpCurFld = lpCurFld->lpPrev;
                      }
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
                    rect.left  = rectOrig.right;
                    rect.right = lpCnt->rectCursorFld.right;
                    PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                 rect.bottom - rect.top, DSTINVERT );
                    lpCnt->rectCursorFld.right = rectOrig.right;
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
                    rect.left  = lpCnt->rectRecArea.left;
                    rect.right = lpCnt->rectCursorFld.left;
                    PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                 rect.bottom - rect.top, DSTINVERT );

                    // Move the cursor fld left until we find the 1st field.
                    while( lpCnt->rectCursorFld.left > lpCnt->rectRecArea.left && lpCurFld->lpPrev )
                      {
                      lpCurFld = lpCurFld->lpPrev;
                      lpCnt->rectCursorFld.left -= lpCurFld->cxPxlWidth;
                      }

                    // Set it here to avoid going negative. This should be 0.
                    lpCnt->rectCursorFld.left = lpCnt->rectRecArea.left;
                    }
                  }

                if( lpCnt->dwStyle & CTS_HORZSCROLL )
                  {
                  // Scroll the record area left by 1 field.
                  nHscrollPos = lpCnt->nHscrollPos;
                  FORWARD_WM_HSCROLL( hWnd, 0, SB_LINEUP, 0, SendMessage );
/*                  SendMessage( hWnd, WM_HSCROLL, SB_LINEUP, 0L );*/

                  // If the horz pos changed it means there were more fields.
                  if( nHscrollPos != lpCnt->nHscrollPos )
                    {
                    // Get how many pixels were actually scrolled.
                    cxScrolledPxls = (nHscrollPos - lpCnt->nHscrollPos) * lpCnt->lpCI->cxChar;

                    // Get the new 1st field after the scroll.
                    lpCurFld = Get1stFldInView( hWnd, lpCnt );

                    // After an leftward scroll the last field can never be
                    // totally exposed so the record area right has to be the
                    // client width of the container.
                    lpCnt->rectRecArea.right = lpCnt->cxClient;

                    // Left edge will always be the record area left after
                    // a leftward scroll.
                    lpCnt->rectCursorFld.left = lpCnt->rectRecArea.left;

                    // Move the original rect right to account for the scroll.
                    rectOrig.left  += cxScrolledPxls;
                    rectOrig.right += cxScrolledPxls;

                    // If the orig right is right of the cursor we are selecting.
                    if( rectOrig.right > lpCnt->rectRecArea.left )
                      {
                      rect.left  = lpCnt->rectRecArea.left;
                      rect.right = lpCnt->rectRecArea.left + cxScrolledPxls;

                      PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                   rect.bottom - rect.top, DSTINVERT );

                      // Move the inverted rect right edge also.
                      lpCnt->rectCursorFld.right = rectOrig.right;
                      }
                    }
                  }
                }
              else if( pt.x > lpCnt->rectRecArea.right )
                {
                rect.top    = max(lpCnt->rectRecArea.top, min(rectOrig.top, lpCnt->rectCursorFld.top));
                rect.bottom = max(rectOrig.bottom, lpCnt->rectCursorFld.bottom);

                if( rectOrig.left > lpCnt->rectRecArea.right )
                  {
                  // Clean up any non-inverted recs left of the the record area 
                  // right edge. If the cursor is at the right edge of the 
                  // record area and the orig field is still to the right then
                  // the left edge of the inversion block should be the right
                  // edge of the record area. In other words there should be
                  // no visible inverted fields.
                  if( lpCnt->rectCursorFld.left < lpCnt->rectRecArea.right )
                    {
                    rect.left  = lpCnt->rectCursorFld.left;
                    rect.right = lpCnt->rectRecArea.right;
                    PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                 rect.bottom - rect.top, DSTINVERT );

                    while( lpCnt->rectCursorFld.left < lpCnt->rectRecArea.right && lpCurFld->lpNext )
                      {
                      lpCurFld = lpCurFld->lpNext;
                      lpCnt->rectCursorFld.left += lpCurFld->cxPxlWidth;
                      }
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
                    rect.left  = lpCnt->rectCursorFld.left;
                    rect.right = rectOrig.left;
                    PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                 rect.bottom - rect.top, DSTINVERT );
                    lpCnt->rectCursorFld.left = rectOrig.left;
                    }

                  // Clean up any non-inverted fields between the right edge of
                  // the inverted rect and the record area right edge. If the cursor
                  // cursor is right of the record area it means the right edge
                  // of the inverted rect should be the record area right edge.
                  // This situation could occur if the user does a very fast
                  // rightward stroke with the mouse. We will get a MOUSEMOVE for
                  // a point within the record area and the next MOUSEMOVE point
                  // could be right of the record area. Mouse hardware limitations.
                  if( lpCnt->rectCursorFld.right < lpCnt->rectRecArea.right )
                    {
                    rect.left  = lpCnt->rectCursorFld.right;
                    rect.right = lpCnt->rectRecArea.right;
                    PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                 rect.bottom - rect.top, DSTINVERT );
                    while( lpCnt->rectCursorFld.right < lpCnt->rectRecArea.right && lpCurFld->lpNext )
                      {
                      lpCurFld = lpCurFld->lpNext;
                      lpCnt->rectCursorFld.right += lpCurFld->cxPxlWidth;
                      }
                    }
                  }

                if( lpCnt->dwStyle & CTS_HORZSCROLL )
                  {
                  // Scroll the record area right 1 field.
                  nHscrollPos = lpCnt->nHscrollPos;
                  FORWARD_WM_HSCROLL( hWnd, 0, SB_LINEDOWN, 0, SendMessage );
/*                  SendMessage( hWnd, WM_HSCROLL, SB_LINEDOWN, 0L );*/

                  // If the field pos changed it means there were more fields.
                  if( nHscrollPos != lpCnt->nHscrollPos )
                    {
                    // Get how many pixels were actually scrolled.
                    cxScrolledPxls = (lpCnt->nHscrollPos - nHscrollPos) * lpCnt->lpCI->cxChar;

                    // Get the new last field after the scroll.
                    lpCurFld = GetLastFldInView( hWnd, lpCnt );

                    lpCnt->rectRecArea.right = min(lpCnt->xLastCol, lpCnt->cxClient);
                    lpCnt->rectCursorFld.right = lpCnt->rectRecArea.right;


                    // Move the original rect left to account for the scroll.
                    rectOrig.left  -= cxScrolledPxls;
                    rectOrig.right -= cxScrolledPxls;

                    // If the orig left is left of the cursor we are selecting.
                    if( rectOrig.left < lpCnt->rectRecArea.right )
                      {
                      rect.left  = max(rectOrig.left, lpCnt->cxClient - cxScrolledPxls);
                      rect.right = lpCnt->rectRecArea.right;

                      PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,
                                   rect.bottom - rect.top, DSTINVERT );

                      // Move the inverted rect left edge also.
                      lpCnt->rectCursorFld.left = rectOrig.left;
                      }
                    }
                  }
                }
              }
            }

          // Save the new cursor position.
          ptPrev.x = pt.x;
          ptPrev.y = pt.y;

          // Don't dispatch this msg since we just processed it.
          continue;
          break;

        case WM_LBUTTONUP:              // End of selection drag          
          // Get the final position of the cursor.
          ptLast.x = LOWORD(msgModal.lParam);
          ptLast.y = HIWORD(msgModal.lParam);

          // If the user released the cursor outside the
          // record area we have to adjust the last point.
          if( !PtInRect( &lpCnt->rectRecArea, ptLast ) )
            {
            if( ptLast.y < lpCnt->rectRecArea.top || ptLast.y > lpCnt->rectRecArea.bottom )
              {
              if( lpCnt->rectCursorFld.left < rectOrig.left )
                ptLast.x = lpCnt->rectCursorFld.left+1;
              else if( lpCnt->rectCursorFld.right > rectOrig.right )
                ptLast.x = lpCnt->rectCursorFld.right-1;
              else
                ptLast.x = lpCnt->rectCursorFld.left+1;
              }
            if( ptLast.x < lpCnt->rectRecArea.left || ptLast.x > lpCnt->rectRecArea.right )
              {
              if( lpCnt->rectCursorFld.top < rectOrig.top )
                ptLast.y = lpCnt->rectCursorFld.top+1;
              else if( lpCnt->rectCursorFld.bottom > rectOrig.bottom )
                ptLast.y = lpCnt->rectCursorFld.bottom-1;
              else
                ptLast.y = lpCnt->rectCursorFld.top+1;
              }
            }

          pt.x = ptLast.x;
          pt.y = ptLast.y;

          // Erase the last inversion.
/*          rect.top    = max(lpCnt->rectCursorFld.top,    lpCnt->rectRecArea.top);*/
/*          rect.bottom = min(lpCnt->rectCursorFld.bottom, lpCnt->rectRecArea.bottom);*/
/*          PatBlt( hDC, rect.left, rect.top, rect.right - rect.left,*/
/*                       rect.bottom - rect.top, DSTINVERT );*/

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
      nSelectedRecs = (pt.y - rectOrig.top) / lpCnt->lpCI->cyRow + 1;
    else
      nSelectedRecs = (pt.y - rectOrig.bottom) / lpCnt->lpCI->cyRow - 1;

    // Calc how many fields the user has selected. A positive value
    // means the user selected forward in the field list while a 
    // negative value means they selected backward.
    if( bTrackFlds )
      {
      lpFld2 = lpFld;
      if( pt.x > rectOrig.left )
        {
        nSelectedFlds = 1;
        while( lpFld2 != lpCurFld && lpFld2->lpNext )
          {
          lpFld2 = lpFld2->lpNext;
          nSelectedFlds++;
          }
        }
      else
        {
        nSelectedFlds = -1;
        while( lpFld2 != lpCurFld && lpFld2->lpPrev )
          {
          lpFld2 = lpFld2->lpPrev;
          nSelectedFlds--;
          }
        }
      }

    // Return the selected count.
    *lpSelRecs = nSelectedRecs;
    *lpSelFlds = nSelectedFlds;
 
    return TRUE;
    }

/****************************************************************************
 * InitVThumbpos
 *
 * Purpose:
 *  For 32 bit scrolling calculations, this function initializes:
 *
 *     -  the number of vertical scroll positions per pixel
 *     -  the starting cursor position
 *     -  the starting vertical scroll position
 *
 *  This function is called on the 1st SB_THUMBTRACK msg after receiving 
 *  a WM_NCLBUTTONDOWN on the vertical scroll bar.
 *  This is necessary for supporting scroll ranges greater than the 16 bit
 *  maximum (32767). Windows only gives you a 16 bit value for real time
 *  scrolling (SB_THUMBPOSITION and SB_THUMBTRACK msgs). This is true even
 *  in Win32. This means that if your scrolling maximum is greater than 32767
 *  you will not get a valid position value in the msg. To support ranges
 *  greater 32767 (32 bit) we have to calculate the position ourselves.
 *
 *  This function will only be called in the Win32 environment.
 *
 * Parameters:
 *  HWND          hWnd        - scrollbar's parent window handle
 *  LPCONTAINER   lpCnt       - pointer to window's CONTAINER structure.
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID InitVThumbpos( HWND hWnd, LPCONTAINER lpCnt )
    {
    RECT   rect;
    POINT  pt;
    int    cyVscrollThumb;
    int    cyVscrollArrows;
    int    cyScrollArea;

    // Get the height of the thumb and arrows on a vertical scrollbar.
    cyVscrollThumb = GetSystemMetrics( SM_CYVTHUMB );
    cyVscrollArrows = GetSystemMetrics( SM_CYVSCROLL ) * 2;

    // Calc the pixel height of the area in which the thumb travels.
    GetClientRect( hWnd, &rect );
    cyScrollArea = rect.bottom - (cyVscrollArrows + cyVscrollThumb);

    // Calc the number of vertical scroll positions per pixel and save it.
    lpCnt->nVPosPerPxl = lpCnt->nVscrollMax / cyScrollArea;

    // Get the starting cursor position and save it.
    GetCursorPos( &pt );
    lpCnt->yCursorStart = pt.y;

    // Save the starting vertical scroll position.
    lpCnt->nVPosStart = GetScrollPos( hWnd, SB_VERT );

    return;
    }

/****************************************************************************
 * CalcVThumbpos
 *
 * Purpose:
 *  Calculates the thumb position of the vertical scrollbar. 
 *  This is necessary for supporting scroll ranges greater than the 16 bit
 *  maximum (32767). Windows only gives you a 16 bit value for real time
 *  scrolling (SB_THUMBPOSITION and SB_THUMBTRACK msgs). This is true even
 *  in Win32. This means that if your scrolling maximum is greater than 32767
 *  you will not get a valid position value in the msg. To support ranges
 *  greater 32767 (32 bit) we have to calculate the position ourselves.
 *
 *  This function will only be called in the Win32 environment.
 *
 * Parameters:
 *  HWND          hWnd        - scrollbar's parent window handle
 *  LPCONTAINER   lpCnt       - pointer to window's CONTAINER structure.
 *
 * Return Value:
 *  int           thumb position
 ****************************************************************************/

int CalcVThumbpos( HWND hWnd, LPCONTAINER lpCnt )
{
    POINT  pt;
    int    nPos, yCursorPos;
    int    nPxlsMoved, nPosMoved;

    // Get the current cursor position and determine how many pixels the 
    // ghost thumb has been moved. The sign will take care of the direction.
    GetCursorPos( &pt );
    yCursorPos = pt.y;
    nPxlsMoved = yCursorPos - lpCnt->yCursorStart;

    // Calc the equivalent positions the ghost thumb has moved.
    nPosMoved = nPxlsMoved * lpCnt->nVPosPerPxl;

    // Set the thumb position.
    nPos = lpCnt->nVPosStart + nPosMoved;

    // Check the position against the scroll range min and max.
    nPos = max(0, nPos);
    nPos = min(lpCnt->nVscrollMax, nPos);

    // Return the new thumb position.
    return nPos;
}

/****************************************************************************
 * ClearExtSelectList
 *
 * Purpose:
 *  Clears all selected records in a container that has extended select
 *  (CTS_EXTENDEDSEL) active.
 *
 * Parameters:
 *  HWND          hWnd        - container child window handle
 *  LPCONTAINER   lpCnt       - pointer to window's CONTAINER structure.
 *
 * Return Value:
 *  int           number of records unselected
 ****************************************************************************/

int WINAPI ClearExtSelectList( HWND hWnd, LPCONTAINER lpCnt )
    {
    LPRECORDCORE lpRec = CntRecHeadGet( hWnd );
    int nRecsUnselected=0;

    // Loop thru the container records and unselect any selected records.
    while( lpRec )
      {
      if( lpRec->flRecAttr & CRA_SELECTED )
        {
        // Unselect the record.
        lpRec->flRecAttr &= ~CRA_SELECTED;

        // Repaint the unselected record and notify the parent.
        InvalidateCntRecord( hWnd, lpCnt, lpRec, NULL, 1 );
        NotifyAssocWnd( hWnd, lpCnt, CN_RECUNSELECTED, 0, lpRec, NULL, 0, 0, 0, NULL );

        nRecsUnselected++;
        }

      lpRec = CntNextRec( lpRec );
      }

    // Send the new-list notification.
    NotifyAssocWnd( hWnd, lpCnt, CN_NEWRECSELECTLIST, 0, NULL, NULL, 0, 0, 0, NULL );

    return nRecsUnselected;
    }

/****************************************************************************
 * ClearBlkSelectList
 *
 * Purpose:
 *  Clears all selected fields in a container that has block select
 *  (CTS_BLOCKSEL) active.
 *
 * Parameters:
 *  HWND          hWnd        - container child window handle
 *  LPCONTAINER   lpCnt       - pointer to window's CONTAINER structure.
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI ClearBlkSelectList( HWND hWnd, LPCONTAINER lpCnt )
    {
    LPRECORDCORE lpRec = CntRecHeadGet( hWnd );

    while( lpRec )
      {
      if( lpRec->flRecAttr & CRA_FLDSELECTED )
        {
        // Unselect all selected fields by killing the selected list.
        CntKillSelFldLst( lpRec );

        // If no more fields are selected clear the flag.
        if( !lpRec->lpSelFldHead )
          lpRec->flRecAttr &= ~CRA_FLDSELECTED;

        // Repaint the unselected field(s) and notify the parent.
        // NULL field ptr indicates all fields were unselected.
        InvalidateCntRecord( hWnd, lpCnt, lpRec, NULL, 1 );
        NotifyAssocWnd( hWnd, lpCnt, CN_FLDUNSELECTED, 0, lpRec, NULL, 0, 0, 0, NULL );
        }
      lpRec = CntNextRec( lpRec );
      }

    return;
    }

/****************************************************************************
 * BatchSelect
 *
 * Purpose:
 *  Selects a batch of records if in extended select or a batch of fields
 *  if in block select.
 *
 * Parameters:
 *  HWND          hWnd        - Container child window handle 
 *  LPCONTAINER   lpCnt       - pointer to window's CONTAINER structure.
 *  LPRECORDCORE  lpRecStr    - pointer to starting record to be selected
 *  LPRECORDCORE  lpRecEnd    - pointer to ending record to be selected
 *  int           nDir        - direction to process: 1=NEXT, -1=PREVIOUS
 *
 * Return Value:
 *  BOOL          TRUE  if selection was set with no errors
 *                FALSE if selection could not be set
 ****************************************************************************/

BOOL BatchSelect( HWND hWnd, LPCONTAINER lpCnt, LPRECORDCORE lpRecStr,
                  LPRECORDCORE lpRecEnd, int nDir )
    {
    LPRECORDCORE lpRec2;

    if( !lpRecStr || !lpRecEnd )
      return FALSE;

    lpRec2 = lpRecStr;
    while( lpRec2 )
      {
      if( lpCnt->dwStyle & CTS_EXTENDEDSEL )
        {
        // Only select unselected records.
        if( !(lpRec2->flRecAttr & CRA_SELECTED) )
          {
          // Repaint the selected record and notify the parent.
          lpRec2->flRecAttr |= CRA_SELECTED;
          InvalidateCntRecord( hWnd, lpCnt, lpRec2, NULL, 1 );
          NotifyAssocWnd( hWnd, lpCnt, CN_RECSELECTED, 0, lpRec2, NULL, 0, 0, 0, NULL );
          }
        else
          {
          // We do this repaint for the sake of the focus rectangle.
          InvalidateCntRecord( hWnd, lpCnt, lpRec2, NULL, 1 );
          }
        }
      else if( lpCnt->dwStyle & CTS_BLOCKSEL )
        {
        // Only select unselected fields.
        if( !CntIsFldSelected( hWnd, lpRec2, lpCnt->lpCI->lpFocusFld ) )
          CntSelectFld( hWnd, lpRec2, lpCnt->lpCI->lpFocusFld );
        else
          InvalidateCntRecord( hWnd, lpCnt, lpRec2, lpCnt->lpCI->lpFocusFld, 1 );
        }
  
      // Break out after we have done the end record.
      if( lpRec2 == lpRecEnd )
        break;

      // Get the next or previous record.
      if( nDir == 1 )
        lpRec2 = CntNextRec( lpRec2 );
      else
        lpRec2 = CntPrevRec( lpRec2 );
      }

    return TRUE;
    }

/****************************************************************************
 * BatchFldSelect
 *
 * Purpose:
 *  Selects a batch of fields on the focus record if in block select.
 *
 * Parameters:
 *  HWND          hWnd        - Container child window handle 
 *  LPCONTAINER   lpCnt       - pointer to window's CONTAINER structure.
 *  LPFIELDINFO   lpFldStr    - pointer to starting field to be selected
 *  LPFIELDINFO   lpFldEnd    - pointer to ending field to be selected
 *  int           nDir        - direction to process: 1=NEXT, -1=PREVIOUS
 *
 * Return Value:
 *  BOOL          TRUE  if selection was set with no errors
 *                FALSE if selection could not be set
 ****************************************************************************/

BOOL NEAR BatchFldSelect( HWND hWnd, LPCONTAINER lpCnt, LPFIELDINFO lpFldStr,
                          LPFIELDINFO lpFldEnd, int nDir )
    {
    LPFIELDINFO lpFld2;

    if( !lpFldStr || !lpFldEnd || !(lpCnt->dwStyle & CTS_BLOCKSEL) )
      return FALSE;

    lpFld2 = lpFldStr;
    while( lpFld2 )
      {
      // Only select unselected fields.
      if( !CntIsFldSelected( hWnd, lpCnt->lpCI->lpFocusRec, lpFld2 ) )
        CntSelectFld( hWnd, lpCnt->lpCI->lpFocusRec, lpFld2 );
      else
        InvalidateCntRecord( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec, lpFld2, 1 );
  
      // Break out after we have done the end field.
      if( lpFld2 == lpFldEnd )
        break;

      // Get the next or previous field.
      if( nDir == 1 )
        lpFld2 = CntNextFld( lpFld2 );
      else
        lpFld2 = CntPrevFld( lpFld2 );
      }

    return TRUE;
    }
