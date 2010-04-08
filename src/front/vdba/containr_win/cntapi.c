/****************************************************************************
 * Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * CNTAPI.C
 *
 * Contains the API implementation of the Container control.
 * The advantage of using a function instead of SendMessage is that
 * you get type checking on the parameters and the return value.
 *
 * History:
 *	14-Feb-2000 (somsa01)
 *	    Corrected arguments to LLRemoveTail() and LLRemoveHead().
 ****************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>
#include <malloc.h>
#include <limits.h>
#include <memory.h>
#include <string.h>
#include "cntdll.h"
#include "cntdrag.h"

VOID NEAR UpdateCIptrs( HWND hCntWnd, LPRECORDCORE lpRec );
int  NEAR MeasureFldTitle( HWND hWnd, HDC hDC, LPCONTAINER lpCnt, LPFIELDINFO lpCol );

/****************************************************************************
 * CntGetVersion
 *
 * Purpose:
 *  Gets the version number of the container.
 *
 * Parameters:
 *  none
 *
 * Return Value:
 *  Current version: major release number in the hi-order byte,
 *                   minor release number in the lo-order byte.
 ****************************************************************************/

WORD WINAPI CntGetVersion( void )
    {
    WORD wVersion = CNT_VERSION;

    return wVersion;
    }

/****************************************************************************
 * CntInfoGet
 *
 * Purpose:
 *  Gets a pointer to the container's CNTINFO struct.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *
 * Return Value:
 *  LPCNTINFO     Pointer to the container's CNTINFO struct.
 ****************************************************************************/

LPCNTINFO WINAPI CntInfoGet( HWND hWnd )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);

    return lpCnt->lpCI;
    }

/****************************************************************************
 * CntAssociateSet
 * CntAssociateGet
 *
 * Purpose:
 *  Change or retrieve the associate window of the control.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *  HWND          hWndAssoc   - window handle of new associate
 *
 * Return Value:
 *  HWND          Handle of previous assoc (set) or current assoc (get).
 ****************************************************************************/

HWND WINAPI CntAssociateSet( HWND hWnd, HWND hWndAssoc )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);
    LPCONTAINER lpCntFrame, lpCntLeft;
    HWND       hWndOldAssoc;

    // Save old assoc window handle.
    hWndOldAssoc = lpCnt->hWndAssociate;

    // Let the old assoc know it is no longer the assoc window.
    NotifyAssocWnd( lpCnt->hWnd1stChild, lpCnt, CN_ASSOCIATELOSS, 0, NULL, NULL, 0, 0, 0, NULL );

    // Set the new assoc wnd handle in the struct.
    lpCnt->hWndAssociate = hWndAssoc;

    // If the container is split we must modify the left split window, too.
    if( lpCnt->bIsSplit )
      {
      lpCntFrame = GETCNT(lpCnt->hWndFrame);
      if( lpCntFrame->SplitBar.hWndLeft )
        {
        lpCntLeft = GETCNT(lpCntFrame->SplitBar.hWndLeft);
        lpCntLeft->hWndAssociate = lpCnt->hWndAssociate;
        }
      }

    // Let the new assoc know it is the new assoc window.
    NotifyAssocWnd( lpCnt->hWnd1stChild, lpCnt, CN_ASSOCIATEGAIN, 0, NULL, NULL, 0, 0, 0, NULL );

    // Return the old assoc window handle.
    return hWndOldAssoc;
    }

HWND WINAPI CntAssociateGet( HWND hWnd )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);

    return lpCnt->hWndAssociate;
    }

/****************************************************************************
 * CntAuxWndSet
 * CntAuxWndGet
 *
 * Purpose:
 *  Change or retrieve the auxiliary window of the control.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *  HWND          hAuxWnd     - handle to the auxiliary window
 *
 * Return Value:
 *  HWND          Handle of previous aux wnd (set) or current aux wnd (get)
 ****************************************************************************/

HWND WINAPI CntAuxWndSet( HWND hWnd, HWND hAuxWnd )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);
    HWND hAuxWndOld;

    // Set/Change the auxiliary window of this container.
    if( !IsWindow( hWnd ) )
      return NULL;

    // Save old aux window handle.
    hAuxWndOld = lpCnt->lpCI->hAuxWnd;

    // Set the new aux wnd handle in the struct.
    lpCnt->lpCI->hAuxWnd = hAuxWnd;

    // Return the old aux window handle.
    return hAuxWndOld;
    }

HWND WINAPI CntAuxWndGet( HWND hWnd )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);

    // Return the aux window handle.
    return lpCnt->lpCI->hAuxWnd;
    }

/****************************************************************************
 * CntRangeSet
 *
 * Purpose:
 *  Sets the vertical scrolling range of the container.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *  UINT          iMin        - new minimum of the scrolling range.
 *  UINT          iMax        - new maximum of the scrolling range.
 *
 * Return Value:
 *  DWORD         previous vertical range:  minimum in the lo-order word,
 *                                          maximum in the hi-order word.
 ****************************************************************************/

DWORD WINAPI CntRangeSet( HWND hWnd, UINT iMin, UINT iMax )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);
    int iMinOld, iMaxOld;

    // The minimum is always zero.
    iMin = 0;

    // If the minimum is greater than the max, return error.
    if( iMin > iMax )
      return 0L;

    // Save old values.
    iMinOld = lpCnt->iMin;
    iMaxOld = lpCnt->iMax;

    // Set the range.
    CntRangeExSet( hWnd, iMin, iMax );

    // Return old range.
    return (DWORD) MAKELONG(iMinOld, iMaxOld);
    }

/****************************************************************************
 * CntRangeExSet
 *
 * Purpose:
 *  Sets the vertical scrolling range of the container.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *  LONG          lMin        - new minimum of the scrolling range.
 *  LONG          lMax        - new maximum of the scrolling range.
 *
 * Return Value:
 *  BOOL          0 if successful; non-zero if any error
 ****************************************************************************/

BOOL WINAPI CntRangeExSet( HWND hWnd, LONG lMin, LONG lMax )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);
    LPCONTAINER lpCntFrame, lpCntSplit;
    RECT rect;
    HWND hWndSplit;
    int  iMinOld, iMaxOld;
    int  iMin=(int)lMin;
    int  iMax=(int)lMax;
    BOOL bUpdate=TRUE;

    // Don't update the container if the minimum is set to -1. This is a
    // backdoor method for avoiding a repaint when changing the range.
    // This is typically only a concern for a container which is dynamically
    // updated with more records while the container is up and running.
    // The correct order of events for doing a dynamic update is to change
    // the range first, then add the new records to the scroll bar. Deferred
    // painting should NOT be used while doing this as other areas of the
    // container could become invalidated at the same time from other actions.
    if( iMin == -1 )
      bUpdate = FALSE;

    // The minimum is always zero.
    iMin = 0;

    // If the minimum is greater than the max, return error.
    if( iMin > iMax )
      return 1;

    // Save old values.
    iMinOld = lpCnt->iMin;
    iMaxOld = lpCnt->iMax;

    // Set the new values.
    lpCnt->iMin = iMin;
    lpCnt->iMax = iMax;

    // Set or clear the 32 bit scrolling state if the range
    // maximum is greater than the 16 bit signed maximum.
    // To support this on WIN16 would be a real nightmare so we are
    // not going to do this until someone makes a REALLY good argument
    // for doing it (something like 'Do this or you're fired!').
#ifdef WIN32
    if( lpCnt->iMax > 32767 ) 
      StateSet( lpCnt, CNTSTATE_VSCROLL32 );
    else
      StateClear( lpCnt, CNTSTATE_VSCROLL32 );
#endif

    // Re-initialize the container metrics.
    InitTextMetrics( lpCnt->hWnd1stChild, lpCnt );

    // Calc the rect for the record area.
    rect.left   = 0;
    rect.top    = lpCnt->lpCI->cyTitleArea + lpCnt->lpCI->cyColTitleArea;
    rect.right  = lpCnt->cxClient;
    rect.bottom = lpCnt->cyClient;

    // Force repaint if the user didn't suppress it.
    if( bUpdate )
      UpdateContainer( lpCnt->hWnd1stChild, &rect, TRUE );

    // If the container is split we must modify the range of
    // the other split window, too.
    if( lpCnt->bIsSplit )
      {
      // Get the window handle of the other split child.
      lpCntFrame = GETCNT( lpCnt->hWndFrame );
      if( lpCnt->hWnd1stChild == lpCntFrame->SplitBar.hWndLeft )
        hWndSplit = lpCntFrame->SplitBar.hWndRight;
      else
        hWndSplit = lpCntFrame->SplitBar.hWndLeft;

      if( hWndSplit )
        {
        lpCntSplit = GETCNT( hWndSplit );

        // Set the this split child's values the same as the other.
        lpCntSplit->iMin = lpCnt->iMin;
        lpCntSplit->iMax = lpCnt->iMax;
        lpCntSplit->nVscrollMax = lpCnt->nVscrollMax;
        lpCntSplit->nVscrollPos = lpCnt->nVscrollPos;

        // Re-initialize the container metrics.
        InitTextMetrics( hWndSplit, lpCntSplit );

        // Update the record area.
        rect.right  = lpCntSplit->cxClient;
        rect.bottom = lpCntSplit->cyClient;
        if( bUpdate )
          UpdateContainer( hWndSplit, &rect, TRUE );
        }
      }

    // Notify the assoc of the range change.  
    NotifyAssocWnd( lpCnt->hWnd1stChild, lpCnt, CN_RANGECHANGE, 0, NULL, NULL, 0, 0, 0, NULL );

    return 0;
    }

/****************************************************************************
 * CntRangeMinGet
 * CntRangeMaxGet
 *
 * Purpose:
 *  Gets the min or max of the vertical scrolling range of the container.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *
 * Return Value:
 *  LONG          min or max of the range
 ****************************************************************************/

LONG WINAPI CntRangeMinGet( HWND hWnd )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);

/*    return MAKELONG(lpCnt->iMin, 0);*/
    return (LONG)lpCnt->iMin;
    }

LONG WINAPI CntRangeMaxGet( HWND hWnd )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);

/*    return MAKELONG(lpCnt->iMax, 0);*/
    return (LONG)lpCnt->iMax;
    }

/****************************************************************************
 * CntRangeGet
 *
 * Purpose:
 *  Gets the min and max of the vertical scrolling range of the container.
 *  This function only exists for backward compatibilty. CntRangeMinGet and
 *  CntRangeMaxGet should be used instead.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *
 * Return Value:
 *  DWORD         Current vertical range:  minimum in the lo-order word,
 *                                         maximum in the hi-order word.
 ****************************************************************************/

DWORD WINAPI CntRangeGet( HWND hWnd )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);

    return (DWORD)MAKELONG(lpCnt->iMin, lpCnt->iMax);
    }

/****************************************************************************
 * CntRangeInc
 * CntRangeDec
 *
 * Purpose:
 *  Increment or Decrement the vertical scrolling range maximum.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *  UINT          iMin        - new minimum of the scrolling range.
 *  UINT          iMax        - new maximum of the scrolling range.
 *
 * Return Value:
 *  DWORD         new range maximum after increment or decrement
 ****************************************************************************/

DWORD WINAPI CntRangeInc( HWND hWnd )
    {
    LONG lRangeMin, lRangeMax;

    // Increase the scrolling range by one.
/*    dwRange   = CntRangeGet( hWnd );*/
/*    nRangeMin = LOWORD(dwRange);*/
/*    nRangeMax = HIWORD(dwRange) + 1;*/

    lRangeMin = CntRangeMinGet( hWnd );
    lRangeMax = CntRangeMaxGet( hWnd ) + 1;
    CntRangeExSet( hWnd, lRangeMin, lRangeMax );
    return (DWORD)lRangeMax;
    }

DWORD WINAPI CntRangeDec( HWND hWnd )
    {
    LONG lRangeMin, lRangeMax;

    // Decrease the scrolling range by one.
/*    dwRange   = CntRangeGet( hWnd );*/
/*    nRangeMin = LOWORD(dwRange);*/
/*    nRangeMax = HIWORD(dwRange) - 1;*/

    lRangeMin = CntRangeMinGet( hWnd );
    lRangeMax = CntRangeMaxGet( hWnd ) - 1;
    CntRangeExSet( hWnd, lRangeMin, lRangeMax );
    return (DWORD)lRangeMax;
    }

/****************************************************************************
 * CntCurrentPosExSet
 * CntCurrentPosExGet
 *
 * Purpose:
 *  Change or retrieve the current vertical scrolling position.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *  LONG          lPos        - new position to set.
 *
 * Return Value:
 *  LONG          Previous (set) or current (get) position.
 ****************************************************************************/

LONG WINAPI CntCurrentPosExSet( HWND hWnd, LONG lPos )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);
    int         iOldPos;
    int         iPos=(int)lPos;

    // Save current vertical scroll position.
    iOldPos = lpCnt->nVscrollPos;

    if( (lpCnt->iMin <= iPos) && (lpCnt->iMax >= iPos) )
      {
      // Execute the vertical scroll to the new position.
      // Note that for 32 bit scrolling we are passing the position in the
      // unused hwndCtl parm slot. When the msg is received that value will
      // be used instead of trying to calculate the position.
      if( StateTest( lpCnt, CNTSTATE_VSCROLL32 ) )
        FORWARD_WM_VSCROLL( lpCnt->hWnd1stChild, iPos, SB_THUMBPOSITION, iPos, SendMessage );
      else
        FORWARD_WM_VSCROLL( lpCnt->hWnd1stChild, 0, SB_THUMBPOSITION, iPos, SendMessage );

      //Return old position.
/*      return MAKELONG(iOldPos, 0);*/
      return (LONG)iOldPos;
      }

    //Invalid position.
    return 0L;
    }

LONG WINAPI CntCurrentPosExGet( HWND hWnd )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);

/*    return MAKELONG(lpCnt->nVscrollPos, 0);*/
    return (LONG)lpCnt->nVscrollPos;
    }

/****************************************************************************
 * CntCurrentPosSet
 * CntCurrentPosGet
 *
 * Purpose:
 *  Change or retrieve the current vertical scrolling position.
 *  These functions only exist for backward compatibilty. 
 *  CntCurrentPosExSet and CntCurrentPosExGet should be used instead.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *  int           iPos        - new position to set.
 *
 * Return Value:
 *  int           Previous (set) or current (get) position.
 ****************************************************************************/

int WINAPI CntCurrentPosSet( HWND hWnd, int iPos )
    {
    return (int)CntCurrentPosExSet( hWnd, iPos );
    }

int WINAPI CntCurrentPosGet( HWND hWnd )
    {
    return (int)CntCurrentPosExGet( hWnd );
    }

/****************************************************************************
 * CntColorSet
 * CntColorGet
 * CntColorReset
 *
 * Purpose:
 *  Change, retrieve, or reset a configurable color.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *  UINT          iColor      - index to the control to modify or retrieve.
 *  COLORREF      cr          - new value of the color.
 *                (for Set -1 resets the color to default system color)
 *
 * Return Value:
 *  COLORREF      Previous (set) or current (get) color value.
 ****************************************************************************/

COLORREF WINAPI CntColorSet( HWND hWnd, UINT iColor, COLORREF cr )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);
    LPCONTAINER lpCntFrame, lpCntLeft;
    COLORREF    crOld;
    int         i;
    RECT        rect;

    if( iColor >= CNTCOLORS )
      return 0L;

    // Save the old color.
    crOld = lpCnt->crColor[iColor];

    // Set the new color or reset to the default system color.
    if( cr == -1L )
      lpCnt->crColor[iColor] = GetCntDefColor( iColor );
    else
      lpCnt->crColor[iColor] = cr;

    // Update the container colors.
    CntColorManager( lpCnt->hWnd1stChild, lpCnt, CLR_UPDATE );

    //Force repaint since we changed a state.  Only want to repaint affected areas
    if (iColor==CNTCOLOR_TITLE      || iColor==CNTCOLOR_TTLBKGD || 
        iColor==CNTCOLOR_TTLBTNTXT  || iColor==CNTCOLOR_TTLBTNBKGD)
    {
      // only repaint the title area
      rect.left   = 0;
      rect.right  = lpCnt->cxClient;
      rect.top    = 0;
      rect.bottom = lpCnt->lpCI->cyTitleArea;
      UpdateContainer( lpCnt->hWnd1stChild, &rect, TRUE );
    }
    else if (iColor==CNTCOLOR_FLDTITLES || iColor==CNTCOLOR_FLDTTLBKGD ||
             iColor==CNTCOLOR_FLDBTNTXT || iColor==CNTCOLOR_FLDBTNBKGD)
    {
      // only repaint the field title area
      rect.left   = 0;
      rect.right  = lpCnt->cxClient;
      rect.top    = lpCnt->lpCI->cyTitleArea;
      rect.bottom = rect.top + lpCnt->lpCI->cyColTitleArea;
      UpdateContainer( lpCnt->hWnd1stChild, &rect, TRUE );
    }
    else if (iColor==CNTCOLOR_TEXT   || iColor==CNTCOLOR_HIGHLIGHT ||
             iColor==CNTCOLOR_HITEXT)
    {
      // only repaint the record area
      rect.left   = 0;
      rect.right  = lpCnt->cxClient;
      rect.top    = lpCnt->lpCI->cyTitleArea;
      rect.bottom = lpCnt->cyClient;
      UpdateContainer( lpCnt->hWnd1stChild, &rect, TRUE );
    }
    else
      UpdateContainer( lpCnt->hWnd1stChild, NULL, TRUE );

    // If the container is split we must modify the left split window, too.
    // NOTE: For now the split children share the same pens.
    if( lpCnt->bIsSplit )
      {
      lpCntFrame = GETCNT( lpCnt->hWndFrame );
      if( lpCntFrame->SplitBar.hWndLeft )
        {
        lpCntLeft = GETCNT( lpCntFrame->SplitBar.hWndLeft );
        lpCntLeft->crColor[iColor] = lpCnt->crColor[iColor];

        // Don't want to call this because it deletes the old pens 
/*        CntColorManager( lpCntFrame->SplitBar.hWndLeft, lpCntLeft, CLR_UPDATE );*/

        // Set the new pens in the other split child.
        for( i=0; i < CNTCOLORS; i++ )
          lpCntLeft->hPen[i] = lpCnt->hPen[i];

        UpdateContainer( lpCntFrame->SplitBar.hWndLeft, NULL, TRUE );
        }
      }

    // Return the old color.
    return crOld;
    }

COLORREF WINAPI CntColorGet( HWND hWnd, UINT iColor )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);

    if( iColor >= CNTCOLORS )
      return 0L;

    return lpCnt->crColor[iColor];
    }

BOOL WINAPI CntColorReset( HWND hWnd )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);
    LPCONTAINER lpCntFrame, lpCntLeft;
    int  i;

    if( !lpCnt )
      return 1;

    // Delete all the current pens and create new ones with default colors.
    CntColorManager( lpCnt->hWnd1stChild, lpCnt, CLR_DELETE );
    CntColorManager( lpCnt->hWnd1stChild, lpCnt, CLR_INIT   );

    // Force a repaint to update the container.
    UpdateContainer( lpCnt->hWnd1stChild, NULL, TRUE );

    // If the container is split we must modify the left split window, too.
    // NOTE: For now the split children share the same pens.
    if( lpCnt->bIsSplit )
      {
      lpCntFrame = GETCNT( lpCnt->hWndFrame );
      if( lpCntFrame->SplitBar.hWndLeft )
        {
        lpCntLeft = GETCNT( lpCntFrame->SplitBar.hWndLeft );

        // Update all the colors and pens in the other split child.
        for( i=0; i < CNTCOLORS; i++ )
          {
          lpCntLeft->crColor[i] = lpCnt->crColor[i];
          lpCntLeft->hPen[i]    = lpCnt->hPen[i];
          }

        UpdateContainer( lpCntFrame->SplitBar.hWndLeft, NULL, TRUE );
        }
      }

    return 0;
    }

/****************************************************************************
 * CntFldColorSet
 * CntFldColorGet
 *
 * Purpose:
 *  Change or retrieve a configurable field color.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *  LPFIELDINFO   lpFld       - pointer to field info struct.  
 *  UINT          iColor      - index to the control to modify or retrieve.
 *  COLORREF      cr          - new value of the color.
 *                (for Set -1 resets the field color to default setting)
 *
 * Return Value:
 *  COLORREF      Previous (set) or current (get) color value.
 ****************************************************************************/

COLORREF WINAPI CntFldColorSet( HWND hWnd, LPFIELDINFO lpFld, UINT iColor, COLORREF cr )
    {
    LPCONTAINER  lpCnt = GETCNT(hWnd);
    LPCONTAINER  lpCntFrame;
    COLORREF    crOld;

    switch( iColor )
      {  
      case CNTCOLOR_TEXT:              // field text color
        crOld = lpFld->bClrFldText ? lpFld->crFldText : lpCnt->crColor[iColor];
        if( cr == -1L )
          {
          lpFld->bClrFldText = FALSE;
          }
        else
          {
          lpFld->crFldText = cr;
          lpFld->bClrFldText = TRUE;
          }
        break;
      case CNTCOLOR_FLDBKGD:           // field text background color
        crOld = lpFld->bClrFldBk ? lpFld->crFldBk : lpCnt->crColor[iColor];
        if( cr == -1L )
          {
          lpFld->bClrFldBk = FALSE;
          }
        else
          {
          lpFld->crFldBk = cr;
          lpFld->bClrFldBk = TRUE;
          }
        break;
      case CNTCOLOR_FLDTITLES:         // field title color
        crOld = lpFld->bClrFldTtlText ? lpFld->crFldTtlText : lpCnt->crColor[iColor];
        if( cr == -1L )
          {
          lpFld->bClrFldTtlText = FALSE;
          }
        else
          {
          lpFld->crFldTtlText = cr;
          lpFld->bClrFldTtlText = TRUE;
          }
        break;
      case CNTCOLOR_FLDTTLBKGD:        // field title background color
        crOld = lpFld->bClrFldTtlBk ? lpFld->crFldTtlBk : lpCnt->crColor[iColor];
        if( cr == -1L )
          {
          lpFld->bClrFldTtlBk = FALSE;
          }
        else
          {
          lpFld->crFldTtlBk = cr;
          lpFld->bClrFldTtlBk = TRUE;
          }
        break;
      case CNTCOLOR_FLDBTNTXT:         // field button text color
        crOld = lpFld->bClrFldBtnText ? lpFld->crFldBtnText : lpCnt->crColor[iColor];
        if( cr == -1L )
          {
          lpFld->bClrFldBtnText = FALSE;
          }
        else
          {
          lpFld->crFldBtnText = cr;
          lpFld->bClrFldBtnText = TRUE;
          }
        break;
      case CNTCOLOR_FLDBTNBKGD:        // field button background color
        crOld = lpFld->bClrFldBtnBk ? lpFld->crFldBtnBk : lpCnt->crColor[iColor];
        if( cr == -1L )
          {
          lpFld->bClrFldBtnBk = FALSE;
          }
        else
          {
          lpFld->crFldBtnBk = cr;
          lpFld->bClrFldBtnBk = TRUE;
          }
        break;
      default:
        return (COLORREF)0L;
      }

    //Force repaint since we changed a state.
    UpdateContainer( lpCnt->hWnd1stChild, NULL, TRUE );

    // If the container is split repaint the left split window, too.
    if( lpCnt->bIsSplit )
      {
      lpCntFrame = GETCNT( lpCnt->hWndFrame );
      if( lpCntFrame->SplitBar.hWndLeft )
        UpdateContainer( lpCntFrame->SplitBar.hWndLeft, NULL, TRUE );
      }

    //Return the old color.
    return crOld;
    }

COLORREF WINAPI CntFldColorGet( HWND hWnd, LPFIELDINFO lpFld, UINT iColor )
    {
    LPCONTAINER  lpCnt = GETCNT(hWnd);
    COLORREF    cr=0L;

    switch( iColor )
      {  
      case CNTCOLOR_TEXT:              // field text color
        cr = lpFld->bClrFldText ? lpFld->crFldText : lpCnt->crColor[iColor];
        break;
      case CNTCOLOR_FLDBKGD:           // field text background color
        cr = lpFld->bClrFldBk ? lpFld->crFldBk : lpCnt->crColor[iColor];
        break;
      case CNTCOLOR_FLDTITLES:         // field title color
        cr = lpFld->bClrFldTtlText ? lpFld->crFldTtlText : lpCnt->crColor[iColor];
        break;
      case CNTCOLOR_FLDTTLBKGD:        // field title background color
        cr = lpFld->bClrFldTtlBk ? lpFld->crFldTtlBk : lpCnt->crColor[iColor];
        break;
      case CNTCOLOR_FLDBTNTXT:         // field button text color
        cr = lpFld->bClrFldBtnText ? lpFld->crFldBtnText : lpCnt->crColor[iColor];
        break;
      case CNTCOLOR_FLDBTNBKGD:        // field button background color
        cr = lpFld->bClrFldBtnBk ? lpFld->crFldBtnBk : lpCnt->crColor[iColor];
        break;
      }

    return cr;
    }

/****************************************************************************
 * CntRecColorSet
 * CntRecColorGet
 *
 * Purpose:
 *  Change or retrieve a configurable record color.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *  LPRECORDCORE  lpRec       - pointer to the record struct
 *  UINT          iColor      - index to the control to modify or retrieve.
 *  COLORREF      cr          - new value of the color.
 *                (for Set -1 resets the record color to default setting)
 *
 * Return Value:
 *  COLORREF      Previous (Set) or current (Get) color value.
 ****************************************************************************/

COLORREF WINAPI CntRecColorSet( HWND hWnd, LPRECORDCORE lpRec, UINT iColor, COLORREF cr )
    {
    LPCONTAINER  lpCnt = GETCNT(hWnd);
    COLORREF    crOld;

    switch( iColor )
      {  
      case CNTCOLOR_TEXT:              // record text color
        crOld = lpRec->bClrRecText ? lpRec->crRecText : lpCnt->crColor[iColor];
        if( cr == -1L )
          {
          lpRec->bClrRecText = FALSE;
          }
        else
          {
          lpRec->crRecText = cr;
          lpRec->bClrRecText = TRUE;
          }
        break;
      case CNTCOLOR_CNTBKGD:           // record text background color
        crOld = lpRec->bClrRecBk ? lpRec->crRecBk : lpCnt->crColor[iColor];
        if( cr == -1L )
          {
          lpRec->bClrRecBk = FALSE;
          }
        else
          {
          lpRec->crRecBk = cr;
          lpRec->bClrRecBk = TRUE;
          }
        break;
      default:
        return (COLORREF)0L;
      }

    // Repaint the record (there will be no action if it's not in view).
    InvalidateCntRecord( lpCnt->hWnd1stChild, lpCnt, lpRec, NULL, 2 );

    //Return the old color.
    return crOld;
    }

COLORREF WINAPI CntRecColorGet( HWND hWnd, LPRECORDCORE lpRec, UINT iColor )
    {
    LPCONTAINER  lpCnt = GETCNT(hWnd);
    COLORREF    cr=0L;

    switch( iColor )
      {  
      case CNTCOLOR_TEXT:              // record text color
        cr = lpRec->bClrRecText ? lpRec->crRecText : lpCnt->crColor[iColor];
        break;
      case CNTCOLOR_CNTBKGD:           // record text background color
        cr = lpRec->bClrRecBk ? lpRec->crRecBk : lpCnt->crColor[iColor];
        break;
      }

    return cr;
    }

/****************************************************************************
 * CntTtlSet
 * CntTtlGet
 * CntFldTtlSet
 * CntFldTtlGet
 *
 * Purpose:
 *  Change or retrieve the title or field title.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *  LPSTR         lpszTitle   - of the container title.
 *  LPFIELDINFO   lpFld       - pointer to field info struct.  
 *  LPSTR         lpszColTitle- of the container field title.
 *  UINT          wTitleLen   - length of field title buffer to allocate
 *                (including null terminator)
 *
 * Return Value:
 *  LPSTR         current title or field title
 ****************************************************************************/

BOOL WINAPI CntTtlSet( HWND hWnd, LPSTR lpszTitle )
    {
    LPCONTAINER  lpCnt = GETCNT(hWnd);

    // Check for existing title.
    if ( lpCnt->lpCI->lpszTitle )
/*      GlobalFreePtr( lpCnt->lpCI->lpszTitle );*/
      free( lpCnt->lpCI->lpszTitle );

    // Allocate storage for title.
    lpCnt->lpCI->lpszTitle =
/*      (LPSTR) GlobalAllocPtr( GHND, lstrlen(lpszTitle) + 1 );*/
      (LPSTR) calloc( 1, lstrlen(lpszTitle) + 1 );

    // Set the new title.
    lstrcpy( lpCnt->lpCI->lpszTitle, lpszTitle );

    // Re-initialize the container metrics.
    InitTextMetrics( lpCnt->hWnd1stChild, lpCnt );

    // Force repaint since we changed a state.
    UpdateContainer( lpCnt->hWnd1stChild, NULL, TRUE );

    return 0;
    }

LPSTR WINAPI CntTtlGet( HWND hWnd )
    {
    LPCONTAINER  lpCnt = GETCNT(hWnd);

    return lpCnt->lpCI->lpszTitle;
    }

BOOL WINAPI CntFldTtlSet( HWND hWnd, LPFIELDINFO lpFld, LPSTR lpszColTitle, UINT wTitleLen )
    {
    LPCONTAINER  lpCnt = GETCNT(hWnd);

    if( !lpFld || !lpszColTitle || !wTitleLen )
      return 1;

    // Free old title if there was one.
    if( lpFld->lpFTitle )
      {
/*      GlobalFreePtr(lpFld->lpFTitle);*/
      free(lpFld->lpFTitle);
      lpFld->lpFTitle = NULL;
      }

    // Alloc space for the new title.
/*    lpFld->lpFTitle = (LPVOID) GlobalAllocPtr(GHND, wTitleLen);*/
    lpFld->lpFTitle = (LPVOID) calloc(1, wTitleLen);
    lpFld->wTitleLen = lpFld->lpFTitle ? wTitleLen : 0;

    if( lpFld->lpFTitle )
      {
      _fstrncpy( lpFld->lpFTitle, lpszColTitle, lpFld->wTitleLen );
      ((LPSTR) lpFld->lpFTitle)[lpFld->wTitleLen - 1] = 0;

      // Re-initialize the container metrics and force a repaint.
      InitTextMetrics( lpCnt->hWnd1stChild, lpCnt );
      UpdateContainer( lpCnt->hWnd1stChild, NULL, TRUE );
      return 0;
      }
    else
      return 1;
    }

LPSTR WINAPI CntFldTtlGet( HWND hWnd, LPFIELDINFO lpFld )
    {
    if( lpFld )
      return lpFld->lpFTitle;
    else
      return NULL;
    }

/****************************************************************************
 * CntTtlAlignSet
 * CntFldTtlAlnSet
 * CntFldDataAlnSet
 *
 * Purpose:
 *  Set text alignment for the container title, field title, or field data.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *  DWORD         dwAlign     - text alignment value
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI CntTtlAlignSet( HWND hWnd, DWORD dwAlign )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);

    // Clear any previous horizontal or vertical settings.
    if( (dwAlign & CA_TA_LEFT) || (dwAlign & CA_TA_HCENTER) || (dwAlign & CA_TA_RIGHT) )
      lpCnt->lpCI->flTitleAlign &= ~(CA_TA_LEFT | CA_TA_HCENTER | CA_TA_RIGHT);
    if( (dwAlign & CA_TA_TOP) || (dwAlign & CA_TA_VCENTER) || (dwAlign & CA_TA_BOTTOM) )
      lpCnt->lpCI->flTitleAlign &= ~(CA_TA_TOP | CA_TA_VCENTER | CA_TA_BOTTOM);

    // set the horizontal alignment
    if( dwAlign & CA_TA_HCENTER )
      lpCnt->lpCI->flTitleAlign |= CA_TA_HCENTER;
    else if( dwAlign & CA_TA_RIGHT )
      lpCnt->lpCI->flTitleAlign |= CA_TA_RIGHT;
    else
      lpCnt->lpCI->flTitleAlign |= CA_TA_LEFT;

    // set the vertical alignment
    if( dwAlign & CA_TA_VCENTER )
      lpCnt->lpCI->flTitleAlign |= CA_TA_VCENTER;
    else if( dwAlign & CA_TA_BOTTOM )
      lpCnt->lpCI->flTitleAlign |= CA_TA_BOTTOM;
    else
      lpCnt->lpCI->flTitleAlign |= CA_TA_TOP;

    // Force repaint since we changed a state.
    UpdateContainer( lpCnt->hWnd1stChild, NULL, TRUE );

    return;
    }

VOID WINAPI CntFldTtlAlnSet( HWND hWnd, LPFIELDINFO lpFld, DWORD dwAlign )
    {
    LPCONTAINER  lpCnt = GETCNT(hWnd);

    if( lpFld )
      {
      // Clear any previous horizontal or vertical settings.
      if( (dwAlign & CA_TA_LEFT) || (dwAlign & CA_TA_HCENTER) || (dwAlign & CA_TA_RIGHT) )
        lpFld->flFTitleAlign &= ~(CA_TA_LEFT | CA_TA_HCENTER | CA_TA_RIGHT);
      if( (dwAlign & CA_TA_TOP) || (dwAlign & CA_TA_VCENTER) || (dwAlign & CA_TA_BOTTOM) )
        lpFld->flFTitleAlign &= ~(CA_TA_TOP | CA_TA_VCENTER | CA_TA_BOTTOM);

      // set the horizontal alignment
      if( dwAlign & CA_TA_HCENTER )
        lpFld->flFTitleAlign |= CA_TA_HCENTER;
      else if( dwAlign & CA_TA_RIGHT )
        lpFld->flFTitleAlign |= CA_TA_RIGHT;
      else
        lpFld->flFTitleAlign |= CA_TA_LEFT;

      // set the vertical alignment
      if( dwAlign & CA_TA_VCENTER )
        lpFld->flFTitleAlign |= CA_TA_VCENTER;
      else if( dwAlign & CA_TA_BOTTOM )
        lpFld->flFTitleAlign |= CA_TA_BOTTOM;
      else
        lpFld->flFTitleAlign |= CA_TA_TOP;

      //Force repaint since we changed a state.
      UpdateContainer( lpCnt->hWnd1stChild, NULL, TRUE );
/*      UpdateContainer( hWnd, NULL, TRUE );*/
      }
    return;
    }

VOID WINAPI CntFldDataAlnSet( HWND hWnd, LPFIELDINFO lpFld, DWORD dwAlign )
    {
    LPCONTAINER  lpCnt = GETCNT(hWnd);

    if( lpFld )
      {
      // Clear any previous horizontal or vertical settings.
      if( (dwAlign & CA_TA_LEFT) || (dwAlign & CA_TA_HCENTER) || (dwAlign & CA_TA_RIGHT) )
        lpFld->flFDataAlign &= ~(CA_TA_LEFT | CA_TA_HCENTER | CA_TA_RIGHT);
      if( (dwAlign & CA_TA_TOP) || (dwAlign & CA_TA_VCENTER) || (dwAlign & CA_TA_BOTTOM) )
        lpFld->flFDataAlign &= ~(CA_TA_TOP | CA_TA_VCENTER | CA_TA_BOTTOM);

      // set the horizontal alignment
      if( dwAlign & CA_TA_HCENTER )
        lpFld->flFDataAlign |= CA_TA_HCENTER;
      else if( dwAlign & CA_TA_RIGHT )
        lpFld->flFDataAlign |= CA_TA_RIGHT;
      else
        lpFld->flFDataAlign |= CA_TA_LEFT;

      // set the vertical alignment
      if( dwAlign & CA_TA_VCENTER )
        lpFld->flFDataAlign |= CA_TA_VCENTER;
      else if( dwAlign & CA_TA_BOTTOM )
        lpFld->flFDataAlign |= CA_TA_BOTTOM;
      else
        lpFld->flFDataAlign |= CA_TA_TOP;

      //Force repaint since we changed a state.
      UpdateContainer( lpCnt->hWnd1stChild, NULL, TRUE );
/*      UpdateContainer( hWnd, NULL, TRUE );*/
      }
    return;
    }

/****************************************************************************
 * CntTtlSepSet
 *
 * Purpose:
 *  Draw a separator line under the container title.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI CntTtlSepSet( HWND hWnd )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);

    // turn on the separator line under the container title
    lpCnt->lpCI->flCntAttr |= CA_TTLSEPARATOR;

    //Force repaint since we changed a state.
    UpdateContainer( lpCnt->hWnd1stChild, NULL, TRUE );

    return;
    }

/****************************************************************************
 * CntFldTtlSepSet
 *
 * Purpose:
 *  Draw a separator line under the container field titles.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI CntFldTtlSepSet( HWND hWnd )
    {
    LPCONTAINER  lpCnt = GETCNT(hWnd);

    // turn on the separator line under the field titles
    lpCnt->lpCI->flCntAttr |= CA_FLDSEPARATOR;

    //Force repaint since we changed a state.
    UpdateContainer( lpCnt->hWnd1stChild, NULL, TRUE );

    return;
    }

/****************************************************************************
 * CntFontSet
 * CntFontGet
 *
 * Purpose:
 *  Change or retrieve container fonts.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *  HFONT         hFont       - handle to the new font.
 *  UINT          iFont       - font to set/get
 *
 * Return Value:
 *  HFONT         Current container font.
 ****************************************************************************/

BOOL WINAPI CntFontSet( HWND hWnd, HFONT hFont, UINT iFont )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);

    // set the font into the container info struct
    if( hFont )
      {
      if( iFont == CF_TITLE )            // container title font
        lpCnt->lpCI->hFontTitle = hFont;
      else if( iFont == CF_FLDTITLE )    // field title font
        lpCnt->lpCI->hFontColTitle = hFont;
      else
        lpCnt->lpCI->hFont = hFont;

      // Initialize the character metrics for this font
      InitTextMetrics( lpCnt->hWnd1stChild, lpCnt );

      //Force repaint since if redraw flag is set.
      UpdateContainer( lpCnt->hWnd1stChild, NULL, TRUE );
      }

    return 0;
    }

HFONT WINAPI CntFontGet( HWND hWnd, UINT iFont )
    {
    LPCONTAINER  lpCnt = GETCNT(hWnd);

    if( iFont == CF_TITLE )              // container title font
      return lpCnt->lpCI->hFontTitle;
    else if( iFont == CF_FLDTITLE )      // field title font
      return lpCnt->lpCI->hFontColTitle;
    else if( iFont == CF_GENERAL )       // general font
      return lpCnt->lpCI->hFont;
    else
      return NULL;
    }

/****************************************************************************
 * CntViewSet
 * CntViewGet
 *
 * Purpose:
 *  Change or retrieve the current container view.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *  UINT          iView       - new view to set.
 *
 * Return Value:
 *  UINT          Previous (set) or current (get) view.
 ****************************************************************************/

UINT WINAPI CntViewSet( HWND hWnd, UINT iView )
{
    LPCONTAINER lpCnt = GETCNT(hWnd);
    LPCONTAINER lpCntFrame;
    UINT       iViewOld;

    if( iView == CV_ICON   ||                   // icon view
        iView == CV_NAME   ||                   // name view
        iView == CV_TEXT   ||                   // text view
        iView == CV_DETAIL ||                   // detail view
        iView == (CV_MINI | CV_ICON) ||         // mini icon view
        iView == (CV_MINI | CV_NAME) ||         // mini icon name view
        iView == (CV_FLOW | CV_TEXT) ||         // flowed text view
        iView == (CV_FLOW | CV_NAME) ||         // flowed name view
        iView == (CV_FLOW | CV_NAME | CV_MINI)) // mini icon flowed name view
    {
      // Get the current view
      iViewOld = lpCnt->iView;

      // if currently in detail view and switching to another view - if the window is split, 
      // we must delete the splitbar and save off information about it so we can restore 
      // it when they switch back to detail view.
      if ( iViewOld==CV_DETAIL && iView != iViewOld && lpCnt->bIsSplit )
      {
        lpCntFrame = GETCNT( lpCnt->hWndFrame );
        
        // set flag that window was split before we left, and svae position of split
        lpCntFrame->bSplitSave = TRUE;
        lpCntFrame->xSplitSave = lpCntFrame->SplitBar.xSplitStr; 
        
        // save the scroll position of the two windows
        lpCntFrame->iLeftScrollPos  = (GETCNT( lpCntFrame->SplitBar.hWndLeft ))->nHscrollPos;
        lpCntFrame->iRightScrollPos = (GETCNT( lpCntFrame->SplitBar.hWndRight ))->nHscrollPos;
        
        // delete the split bar
        DeleteSplitBar( lpCntFrame, lpCntFrame->xSplitSave );
      }

      // if we are switching to icon view we must create the record order list (the
      // z-order of the icons).  We only do this if this is the first time we have
      // switched to icon view, or if the list had been killed in another view.  
      // Once this list is created, it is maintained in all views whenever you add or 
      // remove records.
      if (iView & CV_ICON) 
      {
         if ( !lpCnt->lpCI->lpRecOrderHead )
           CreateRecOrderList( hWnd );
      
         // calculate the icon workspace
         CalculateIconWorkspace( hWnd );
         
         // also init size of view rect in case size of window has changed in other views
         lpCnt->lpCI->rcViewSpace.right  = lpCnt->lpCI->rcViewSpace.left + lpCnt->cxClient;
         lpCnt->lpCI->rcViewSpace.bottom = lpCnt->lpCI->rcViewSpace.top  + lpCnt->cyClient - lpCnt->lpCI->cyTitleArea;
      }
      
      // change the view
      lpCnt->lpCI->fViewAttr &= ~iViewOld;
      lpCnt->lpCI->fViewAttr |= iView;
      lpCnt->iView = iView;

      // reinitialize Text Metrics if changed view
      if (iViewOld != iView)
        InitTextMetrics( lpCnt->hWnd1stChild, lpCnt );
      
      // Force repaint since we changed a state.
      UpdateContainer( lpCnt->hWnd1stChild, NULL, TRUE );

      // if switching back to detail view, see if a split bar was saved off in a previous
      // view change, and if so put the split bar back
      if (iView==CV_DETAIL && iView != iViewOld)
      {
        lpCntFrame = GETCNT( lpCnt->hWndFrame );
        if (lpCntFrame->bSplitSave) // detail view was split.
        {
          CreateSplitBar( lpCnt->hWndFrame, lpCntFrame, lpCntFrame->xSplitSave );
          lpCnt->bSplitSave = FALSE;

          // scroll the two windows back to their original positions
          FORWARD_WM_HSCROLL( lpCntFrame->SplitBar.hWndLeft,  0, SB_THUMBPOSITION, lpCntFrame->iLeftScrollPos , SendMessage );
          FORWARD_WM_HSCROLL( lpCntFrame->SplitBar.hWndRight, 0, SB_THUMBPOSITION, lpCntFrame->iRightScrollPos, SendMessage );
        }
      }
      
      // Return old view.
      return iViewOld;
    }

    // Invalid view.
    return 0;
}

UINT WINAPI CntViewGet( HWND hWnd )
    {
    LPCONTAINER  lpCnt = GETCNT(hWnd);

    return lpCnt->iView;
    }

/****************************************************************************
 * CntDeferPaint
 * CntEndDeferPaint
 *
 * Purpose:
 *  Defer update of the container while chgs are made to it or end the 
 *  deferred state which will optionally cause a paint message to be generated.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *  BOOL          bUpdate     - update flag: if non-zero update container
 *                                           if zero don't update container
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI CntDeferPaint( HWND hWnd )
    {
    LPCONTAINER lpCnt = GETCNT( hWnd );
    LPCONTAINER lpCntFrame, lpCntLeft;

    if( !lpCnt )
      return;

    lpCntFrame = GETCNT( lpCnt->hWndFrame );

    StateSet( lpCnt, CNTSTATE_DEFERPAINT );     // set the state
    SetWindowRedraw( lpCnt->hWnd1stChild, 0 );  // clear the child redraw flag

    // If we are split defer the split window's painting, too.
    if( lpCnt->dwStyle & CTS_SPLITBAR && lpCnt->bIsSplit )
      {
      // Get the split window handles from the frame's stuct.
      if( lpCntFrame->SplitBar.hWndLeft )
        {
        lpCntLeft = GETCNT( lpCntFrame->SplitBar.hWndLeft );
        StateSet( lpCntLeft, CNTSTATE_DEFERPAINT );          // set the state
        SetWindowRedraw( lpCntFrame->SplitBar.hWndLeft, 0 ); // clear the redraw flag
        }
      }

    return;
    }

VOID WINAPI CntEndDeferPaint( HWND hWnd, BOOL bUpdate  )
    {
    LPCONTAINER lpCnt = GETCNT( hWnd );
    LPCONTAINER lpCntFrame, lpCntLeft;

    if( !lpCnt )
      return;

    lpCntFrame = GETCNT( lpCnt->hWndFrame );

    StateClear( lpCnt, CNTSTATE_DEFERPAINT );   // clear the state
    SetWindowRedraw( lpCnt->hWnd1stChild, 1 );  // set the child redraw flag

    if( bUpdate )
      UpdateContainer( lpCnt->hWnd1stChild, NULL, TRUE );

    // If we are split end deferring of the split window's painting, too.
    if( lpCnt->dwStyle & CTS_SPLITBAR && lpCnt->bIsSplit )
      {
      // Get the split window handles from the frame's stuct.
      if( lpCntFrame->SplitBar.hWndLeft )
        {
        lpCntLeft = GETCNT( lpCntFrame->SplitBar.hWndLeft );
        StateClear( lpCntLeft, CNTSTATE_DEFERPAINT );            // clear the state
        SetWindowRedraw( lpCntFrame->SplitBar.hWndLeft, 1 ); // set the redraw flag

        if( bUpdate )
          UpdateContainer( lpCntFrame->SplitBar.hWndLeft, NULL, TRUE );
        }
      }

    return;
    }

/****************************************************************************
 * CntDeferPaintEx
 * CntEndDeferPaintEx
 *
 * Purpose:
 *  Defer update of the container while chgs are made to it or end the 
 *  deferred state which will optionally cause a paint message to be generated.
 *  The Ex versions of the defer paint calls use a nested defer count to
 *  allow nested deferrs to take place without causing extra repaints.
 *  CntDeferPaintEx increments the defer count and CntEndDeferPaintEx decrements
 *  the defer count. The container remains in a deferred paint state until
 *  defer count is decreased to zero.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *  BOOL          bUpdate     - update flag: if non-zero update container
 *                                           if zero don't update container
 * Return Value:
 *  int           defer count (after being incremented or decremented)
 ****************************************************************************/

int WINAPI CntDeferPaintEx( HWND hWnd )
    {
    LPCONTAINER lpCnt = GETCNT( hWnd );
    LPCONTAINER lpCntFrame, lpCntLeft;

    // Return -1 if a bad window handle was passed in.
    if( !lpCnt )
      return -1;

    lpCntFrame = GETCNT( lpCnt->hWndFrame );

    StateSet( lpCnt, CNTSTATE_DEFERPAINT );     // set the state
    SetWindowRedraw( lpCnt->hWnd1stChild, 0 );  // clear the child redraw flag

    // If we are split defer the split window's painting, too.
    if( lpCnt->dwStyle & CTS_SPLITBAR && lpCnt->bIsSplit )
      {
      // Get the split window handles from the frame's stuct.
      if( lpCntFrame->SplitBar.hWndLeft )
        {
        lpCntLeft = GETCNT( lpCntFrame->SplitBar.hWndLeft );
        StateSet( lpCntLeft, CNTSTATE_DEFERPAINT );          // set the state
        SetWindowRedraw( lpCntFrame->SplitBar.hWndLeft, 0 ); // clear the redraw flag
        }
      }

    // Bump the nested deferred paint counter.
    lpCntFrame->nNestedDeferPaint++;

    return lpCntFrame->nNestedDeferPaint;
    }

int WINAPI CntEndDeferPaintEx( HWND hWnd, BOOL bUpdate )
    {
    LPCONTAINER lpCnt = GETCNT( hWnd );
    LPCONTAINER lpCntFrame, lpCntLeft;

    if( !lpCnt )
      return -1;

    lpCntFrame = GETCNT( lpCnt->hWndFrame );

    // Pop the nested defer paint counter.
    lpCntFrame->nNestedDeferPaint--;
    lpCntFrame->nNestedDeferPaint = max(0, lpCntFrame->nNestedDeferPaint);

    // If still in a nested deferred paint state, just return.
    if( lpCntFrame->nNestedDeferPaint )
      return lpCntFrame->nNestedDeferPaint;

    StateClear( lpCnt, CNTSTATE_DEFERPAINT );   // clear the state
    SetWindowRedraw( lpCnt->hWnd1stChild, 1 );  // set the child redraw flag

    if( bUpdate )
      UpdateContainer( lpCnt->hWnd1stChild, NULL, TRUE );

    // If we are split end deferring of the split window's painting, too.
    if( lpCnt->dwStyle & CTS_SPLITBAR && lpCnt->bIsSplit )
      {
      // Get the split window handles from the frame's stuct.
      if( lpCntFrame->SplitBar.hWndLeft )
        {
        lpCntLeft = GETCNT( lpCntFrame->SplitBar.hWndLeft );
        StateClear( lpCntLeft, CNTSTATE_DEFERPAINT );            // clear the state
        SetWindowRedraw( lpCntFrame->SplitBar.hWndLeft, 1 ); // set the redraw flag

        if( bUpdate )
          UpdateContainer( lpCntFrame->SplitBar.hWndLeft, NULL, TRUE );
        }
      }

    return lpCntFrame->nNestedDeferPaint;
    }

/****************************************************************************
 * CntFrameWndGet
 *
 * Purpose:
 *  Get the container frame window handle.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *
 * Return Value:
 *  HWND  handle of the frame window which manages the split children
 ****************************************************************************/

HWND WINAPI CntFrameWndGet( HWND hWnd )
    {
    LPCONTAINER lpCnt = GETCNT( hWnd );

    return lpCnt->hWndFrame;
    }

/****************************************************************************
 * CntSpltChildWndGet
 *
 * Purpose:
 *  Gets the window handle of the left or right split child for a container
 *  which is in a split state. If the container is not split the function
 *  will return the handle to the container's permanent 1st child window.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *  UINT          uPane       - flag indicating which child window to return
 *
 * Return Value:
 *  HWND          handle of the left or right split child
 ****************************************************************************/

HWND WINAPI CntSpltChildWndGet( HWND hWnd, UINT uPane )
    {
    LPCONTAINER lpCnt = GETCNT( hWnd );
    LPCONTAINER lpCntFrame;
    HWND hWndChild;

    if( !lpCnt )
      return NULL;
    
    // The 1st child is the right split child when split.
    hWndChild = lpCnt->hWnd1stChild;

    if( uPane == CSB_LEFT && lpCnt->bIsSplit )
      {
      lpCntFrame = GETCNT(lpCnt->hWndFrame);
      hWndChild  = lpCntFrame->SplitBar.hWndLeft;
      }

    return hWndChild;
    }

/****************************************************************************
 * CntSpltBarCreate
 *
 * Purpose:
 *  Creates a split bar or shows the draggable split bar on the container.
 *  For exact placement of a split bar use SB_XCOORD. When using this mode
 *  the coordinate value that is passed in should be one that was returned
 *  from CntCNSplitBarGet after receiving a CN_SPLITBAR_CREATED or
 *  CN_SPLITBAR_MOVED notification.
 *
 *  A split bar can also be created by the user by doing a right button
 *  double click over the split bar (if CA_APPSPLITABLE is not set).
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *  UINT          wMode       - flag indicating the type of action to take
 *  int           xCoord      - X coordinate where split bar should be created
 *                              (only used if wMode = SB_XCOORD)
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI CntSpltBarCreate( HWND hWnd, UINT wMode, int xCoord )
    {
    LPCONTAINER lpCnt = GETCNT( hWnd );
    LPCONTAINER lpCntFrame;
    POINT      pt;
    RECT       rect;
    int        xCreate;

    // Return if split bars not active or we are already split.
    if( !(lpCnt->dwStyle & CTS_SPLITBAR) || lpCnt->bIsSplit || !lpCnt->hWndFrame )
      return;

    lpCntFrame = GETCNT( lpCnt->hWndFrame );

    GetClientRect( lpCnt->hWndFrame, &rect );

    // Account for a vertical scroll bar if it is showing.
    if( lpCnt->dwStyle & CTS_VERTSCROLL && 
        lpCnt->lpCI->dwRecordNbr > (DWORD)lpCnt->lpCI->nRecsDisplayed-1 )
      rect.right -= lpCnt->nMinWndWidth;

    if( wMode == CSB_SHOW )
      {
      // Start split bar drag from the center of the frame window.
      pt.x = rect.right  / 2;
      pt.y = rect.bottom / 2;
      SplitBarManager( lpCnt->hWndFrame, lpCntFrame, SPLITBAR_CREATE, &pt );
      }
    else
      {
      if( wMode == CSB_MIDDLE )       //  create split bar in the middle
        xCreate = rect.right / 2;
      else if( wMode == CSB_RIGHT )   //  create split bar on right side
        xCreate = rect.right - lpCnt->nHalfBarWidth + 1;
      else if( wMode == CSB_XCOORD )  //  create split bar at xCoord
        xCreate = max(lpCnt->nHalfBarWidth, min(xCoord, rect.right-lpCnt->nHalfBarWidth+1));
      else                            //  create split bar on left side
        xCreate = lpCnt->nHalfBarWidth;

      // Create the split bar.
      CreateSplitBar( lpCnt->hWndFrame, lpCntFrame, xCreate );
      }

    return;
    }

/****************************************************************************
 * CntSpltBarDelete
 *
 * Purpose:
 *  Deletes the current container split bar.
 *
 *  The split bar can also be deleted by the user by doing a right button
 *  double click over the split bar (if CA_APPSPLITABLE is not set).
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *  UINT          wMode       - flag indicating the type of action to take
 *  int           xCoord      - X coordinate where split bar should be deleted
 *                              (only used if wMode = SB_XCOORD)
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI CntSpltBarDelete( HWND hWnd, UINT wMode, int xCoord )
    {
    LPCONTAINER lpCnt = GETCNT( hWnd );
    LPCONTAINER lpCntFrame;
    int         xSplit;

    // Return if split bars not active or we are not split.
    if( !(lpCnt->dwStyle & CTS_SPLITBAR) || !lpCnt->bIsSplit || !lpCnt->hWndFrame )
      return;

    // Get the X coordinate of the split bar from the frame struct.
    lpCntFrame = GETCNT( lpCnt->hWndFrame );
    xSplit = lpCntFrame->SplitBar.xSplitStr;

    // Delete the split bar.
    DeleteSplitBar( lpCntFrame, xSplit );

    return;
    }

/****************************************************************************
 * CntAttribSet
 * CntAttribClear
 *
 * Purpose:
 *  Set/Clear general attributes for the specified container.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *  DWORD         dwAttrib    - attribute flag(s) to set or clear
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI CntAttribSet( HWND hWnd, DWORD dwAttrib )
    {
    LPCONTAINER  lpCnt = GETCNT(hWnd);

    // Set the specified attributes.
    lpCnt->lpCI->flCntAttr |= dwAttrib;

    if( lpCnt->lpCI->flCntAttr & CA_WIDEFOCUSRECT )
      lpCnt->bWideFocusRect = TRUE;
    else
      lpCnt->bWideFocusRect = FALSE;

    // No wide focus rectangle if we are in block select.
    if( lpCnt->dwStyle & CTS_BLOCKSEL )
      lpCnt->lpCI->flCntAttr &= ~CA_WIDEFOCUSRECT;

#ifdef WIN32
    // If setting the flag to always show the vertical scroll bar,
    // you have to make it appear first in order for SetScrollInfo to work
    if ( dwAttrib & CA_SHVERTSCROLL )
    {
      SCROLLINFO  ScrInfo;

      if ( !lpCnt->nVscrollMax )
      {
        ScrInfo.fMask  = SIF_RANGE | SIF_DISABLENOSCROLL;
        ScrInfo.nMin   = 0;
        ScrInfo.nMax   = 1;
        ScrInfo.cbSize = sizeof(ScrInfo);
        SetScrollInfo( lpCnt->hWnd1stChild, SB_VERT, &ScrInfo, FALSE );

        ScrInfo.nMax   = 0;
        SetScrollInfo( lpCnt->hWnd1stChild, SB_VERT, &ScrInfo, TRUE );
      }
    }
#endif

    // Force repaint since we changed a state.
    UpdateContainer( lpCnt->hWnd1stChild, NULL, TRUE );

    return;
    }

VOID WINAPI CntAttribClear( HWND hWnd, DWORD dwAttrib )
    {
    LPCONTAINER  lpCnt = GETCNT(hWnd);

    // Clear the specified attributes.
    lpCnt->lpCI->flCntAttr &= (DWORD)~dwAttrib;

    if( lpCnt->lpCI->flCntAttr & CA_WIDEFOCUSRECT )
      lpCnt->bWideFocusRect = TRUE;
    else
      lpCnt->bWideFocusRect = FALSE;

    // Force repaint since we changed a state.
    UpdateContainer( lpCnt->hWnd1stChild, NULL, TRUE );

    return;
    }

/****************************************************************************
 * CntFldAttrSet
 * CntFldAttrGet
 * CntFldAttrClear
 *
 * Purpose:
 *  Set/Gets/Clear attributes for the specified container field.
 *
 * Parameters:
 *  LPFIELDINFO   lpFI        - pointer to the field info struct
 *  DWORD         dwAttrib    - attribute flag(s) to set or clear
 *
 * Return Value:
 *  VOID  - Set/Clear
 *  DWORD - Get
 ****************************************************************************/

VOID WINAPI CntFldAttrSet( LPFIELDINFO lpFI, DWORD dwAttrib )
    {
    // Set the specified attributes.
    lpFI->flColAttr |= dwAttrib;

    return;
    }

VOID WINAPI CntFldAttrClear( LPFIELDINFO lpFI, DWORD dwAttrib )
    {
    // Clear the specified attributes.
    lpFI->flColAttr &= ~dwAttrib;

    return;
    }

DWORD WINAPI CntFldAttrGet( HWND hCntWnd, LPFIELDINFO lpFld )
    {
    return lpFld->flColAttr;
    }

/****************************************************************************
 * CntRecAttrSet
 * CntRecAttrGet
 * CntRecAttrClear
 *
 * Purpose:
 *  Set/Gets/Clear attributes for the specified container record.
 *
 * Parameters:
 *  LPRECORDCORE  lpRec       - pointer to the record struct
 *  DWORD         dwAttrib    - attribute flag(s) to set or clear
 *
 * Return Value:
 *  VOID  - Set/Clear
 *  DWORD - Get
 ****************************************************************************/

VOID WINAPI CntRecAttrSet( LPRECORDCORE lpRec, DWORD dwAttrib )
    {
    lpRec->flRecAttr |= dwAttrib;
    }

VOID WINAPI CntRecAttrClear( LPRECORDCORE lpRec, DWORD dwAttrib )
    {
    lpRec->flRecAttr &= ~dwAttrib;
    }

DWORD WINAPI CntRecAttrGet( LPRECORDCORE lpRec )
    {
    return lpRec->flRecAttr;
    }

/****************************************************************************
 * CntTotalRecsGet
 *
 * Purpose:
 *  Returns the total number of records in the record list.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *
 * Return Value:
 *  DWORD         current record count
 ****************************************************************************/

DWORD WINAPI CntTotalRecsGet( HWND hCntWnd )
    {
    LPCNTINFO lpCI = (GETCNT(hCntWnd))->lpCI;

    return lpCI->dwRecordNbr;
    }

/****************************************************************************
 * CntLineSpaceGet
 *
 * Purpose:
 *  Returns the current container line spacing setting.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *
 * Return Value:
 *  UINT       current line spacing value
 ****************************************************************************/

UINT WINAPI CntLineSpaceGet( HWND hCntWnd )
    {
    LPCNTINFO lpCI = (GETCNT(hCntWnd))->lpCI;

    return  lpCI->wLineSpacing;
    }

/****************************************************************************
 * CntStyleSet
 * CntStyleGet
 * CntStyleClear
 *
 * Purpose:
 *  Set/Get/Clear styles for the specified container.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *  DWORD         dwStyleMod  - style flag(s) to set or clear
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI CntStyleSet( HWND hWnd, DWORD dwStyleMod )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);
    LPCONTAINER lpCntFrame, lpCntLeft;
    RECT       rect;
    POINT      pt;
    DWORD      dwStyle;
    int        cxWidth, cyHeight;

    if( !lpCnt )
      return;

    lpCntFrame = GETCNT( lpCnt->hWndFrame );

    // All WS_* styles (in the hi-word) are applied to the frame.
    dwStyle = (DWORD) GetWindowLong( lpCnt->hWndFrame, GWL_STYLE );

    // Don't ever want these set on any container window.
    dwStyleMod &= ~(WS_VSCROLL | WS_HSCROLL);

    // Clear out any previous selection mode.
    if( (dwStyleMod & CTS_SINGLESEL)   || (dwStyleMod & CTS_MULTIPLESEL) || 
        (dwStyleMod & CTS_EXTENDEDSEL) || (dwStyleMod & CTS_BLOCKSEL) )
      {
      CntStyleClear( hWnd, CTS_SINGLESEL | CTS_MULTIPLESEL | CTS_EXTENDEDSEL | CTS_BLOCKSEL );
      lpCnt->bSimulateMulSel = FALSE;  
      // Clear the wide focus rect if the app didn't have it originally.
      if( !lpCnt->bWideFocusRect )
        lpCnt->lpCI->flCntAttr &= ~CA_WIDEFOCUSRECT;
      }

    dwStyle |= dwStyleMod;

    // No wide focus rectangle if we are in block select.
    if( dwStyle & CTS_BLOCKSEL )
      lpCnt->lpCI->flCntAttr &= ~CA_WIDEFOCUSRECT;

    lpCnt->dwStyle = dwStyle;
    SetWindowLong( lpCnt->hWndFrame, GWL_STYLE, (LONG)dwStyle );

    // No WS_* window styles (HIWORD) are applied to the children.
    dwStyleMod &= 0x0000FFFF;

    if( dwStyleMod )
      {
      dwStyle = (DWORD) GetWindowLong( lpCnt->hWnd1stChild, GWL_STYLE );

      dwStyle |= dwStyleMod;

      lpCnt->dwStyle = dwStyle;

      SetWindowLong( lpCnt->hWnd1stChild, GWL_STYLE, (LONG)lpCnt->dwStyle );

      // Re-initialize the container metrics.
      InitTextMetrics( lpCnt->hWnd1stChild, lpCnt );

      // If the container is split we must modify the styles of
      // the left split window, too.
      if( lpCnt->bIsSplit )
        {
        // Get the left split window handle from the frame's struct.
        if( lpCntFrame->SplitBar.hWndLeft )
          {
          lpCntLeft = GETCNT( lpCntFrame->SplitBar.hWndLeft );

          // The left split never has a vert scrollbar so
          // we will just clear these all the time to be sure.
          lpCntLeft->dwStyle = dwStyle;
          lpCntLeft->dwStyle &= ~CTS_VERTSCROLL; 
          lpCntLeft->dwStyle &= ~WS_VSCROLL;

          if( dwStyleMod & CTS_VERTSCROLL )
            {
            // Adding vert scroll bars so set these values to be 
            // the same as the split child with the vert scroller.
            lpCntLeft->nVscrollMax = lpCnt->nVscrollMax;
            lpCntLeft->nVscrollPos = lpCnt->nVscrollPos;
            }

          SetWindowLong( lpCntFrame->SplitBar.hWndLeft, GWL_STYLE, (LONG)lpCntLeft->dwStyle );

          // Re-initialize the container metrics.
          InitTextMetrics( lpCntFrame->SplitBar.hWndLeft, lpCntLeft );
          }
        }
      }

    // Repaint using MoveWindow because the user may have changed
    // non-client styles (like scrollbars or sizing borders) and
    // in that case MoveWindow guarantees an WM_NCPAINT.
    GetWindowRect( lpCnt->hWndFrame, &rect );
    pt.x = rect.left;
    pt.y = rect.top;
    ScreenToClient( GetParent(lpCnt->hWndFrame), &pt );

    if( ((lpCnt->dwStyle & CTS_INTEGRALWIDTH) || (lpCnt->dwStyle & CTS_INTEGRALHEIGHT)) &&
          (!(lpCnt->iView & CV_ICON)) )
      {
      // Use the original user-set size.
      cxWidth  = lpCntFrame->cxWndOrig;
      cyHeight = lpCntFrame->cyWndOrig;
      }
    else
      {
      // Use the current size.
      cxWidth  = rect.right  - rect.left;
      cyHeight = rect.bottom - rect.top;
      }

    lpCntFrame->bIntegralSizeChg = TRUE;
    InvalidateRect( lpCnt->hWndFrame, NULL, TRUE );
    MoveWindow( lpCnt->hWndFrame, pt.x, pt.y, cxWidth, cyHeight-1, FALSE );
    MoveWindow( lpCnt->hWndFrame, pt.x, pt.y, cxWidth, cyHeight, TRUE );
    lpCntFrame->bIntegralSizeChg = FALSE;

    return;
    }

VOID WINAPI CntStyleClear( HWND hWnd, DWORD dwStyleMod )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);
    LPCONTAINER lpCntFrame, lpCntLeft;
    RECT       rect;
    POINT      pt;
    DWORD      dwStyle;
    int        cxWidth, cyHeight;

    if( !lpCnt )
      return;

    lpCntFrame = GETCNT( lpCnt->hWndFrame );

    // All WS_* styles (in the hi-word) are applied to the frame.
    dwStyle = (DWORD) GetWindowLong( lpCnt->hWndFrame, GWL_STYLE );

    // Don't ever want these set on any container window.
    dwStyleMod &= ~(WS_VSCROLL | WS_HSCROLL);

    dwStyle &= (DWORD)~dwStyleMod;

    lpCnt->dwStyle = dwStyle;
    SetWindowLong( lpCnt->hWndFrame, GWL_STYLE, (LONG)dwStyle );

    // No WS_* window styles (HIWORD) are applied to the children.
    dwStyleMod &= 0x0000FFFF;

    if( dwStyleMod )
      {
      dwStyle = (DWORD) GetWindowLong( lpCnt->hWnd1stChild, GWL_STYLE );

      dwStyle &= (DWORD)~dwStyleMod;

      lpCnt->dwStyle = dwStyle;

      // If they are removing scroll bars reset the range to hide them.
      if( dwStyleMod & CTS_VERTSCROLL )
        {
        lpCnt->nVscrollMax = 0;
        lpCnt->nVscrollPos = 0;
        lpCnt->dwStyle &= ~WS_VSCROLL;
        SetScrollRange( lpCnt->hWnd1stChild, SB_VERT, 0, 0, FALSE );
        }
      if( dwStyleMod & CTS_HORZSCROLL )
        {
        lpCnt->nHscrollMax = 0;
        lpCnt->nHscrollPos = 0;
        lpCnt->dwStyle &= ~WS_HSCROLL;
        SetScrollRange( lpCnt->hWnd1stChild, SB_HORZ, 0, 0, FALSE );
        }

      SetWindowLong( lpCnt->hWnd1stChild, GWL_STYLE, (LONG)lpCnt->dwStyle );

      // Re-initialize the container metrics.
      InitTextMetrics( lpCnt->hWnd1stChild, lpCnt );

      // If the container is split we must modify the styles of
      // the left split window, too.
      if( lpCnt->bIsSplit )
        {
        // Get the left split window handle from the frame's struct.
        if( lpCntFrame->SplitBar.hWndLeft )
          {
          lpCntLeft = GETCNT( lpCntFrame->SplitBar.hWndLeft );

          // The left split never has a vert scrollbar so
          // we will just clear these all the time to be sure.
          lpCntLeft->dwStyle = dwStyle;
          lpCntLeft->dwStyle &= ~CTS_VERTSCROLL; 
          lpCntLeft->dwStyle &= ~WS_VSCROLL;

          if( dwStyleMod & CTS_VERTSCROLL )
            {
            // Removing vert scroll bars so reset these values
            // to prevent any residual scrolling activity.
            lpCntLeft->nVscrollMax = 0;
            lpCntLeft->nVscrollPos = 0;
            }

          // If they are removing the horz scrollbar reset the
          // range to make it go away.
          if( dwStyleMod & CTS_HORZSCROLL )
            {
            lpCntLeft->nHscrollMax = 0;
            lpCntLeft->nHscrollPos = 0;
            lpCntLeft->dwStyle &= ~WS_HSCROLL;
            SetScrollRange( lpCntFrame->SplitBar.hWndLeft, SB_HORZ, 0, 0, FALSE );
            }

          SetWindowLong( lpCntFrame->SplitBar.hWndLeft, GWL_STYLE, (LONG)lpCntLeft->dwStyle );

          // Re-initialize the container metrics.
          InitTextMetrics( lpCntFrame->SplitBar.hWndLeft, lpCntLeft );
          }
        }
      }

    // Repaint using MoveWindow because the user may have changed
    // non-client styles (like scrollbars or sizing borders) and
    // in that case MoveWindow guarantees an WM_NCPAINT.
    GetWindowRect( lpCnt->hWndFrame, &rect );
    pt.x = rect.left;
    pt.y = rect.top;
    ScreenToClient( GetParent(lpCnt->hWndFrame), &pt );

    if( ((lpCnt->dwStyle & CTS_INTEGRALWIDTH) || (lpCnt->dwStyle & CTS_INTEGRALHEIGHT)) &&
          (!(lpCnt->iView & CV_ICON)) )
      {
      // Use the original user-set size.
      cxWidth  = lpCntFrame->cxWndOrig;
      cyHeight = lpCntFrame->cyWndOrig;
      }
    else
      {
      // Use the current size.
      cxWidth  = rect.right  - rect.left;
      cyHeight = rect.bottom - rect.top;
      }

    lpCntFrame->bIntegralSizeChg = TRUE;
    InvalidateRect( lpCnt->hWndFrame, NULL, TRUE );
    MoveWindow( lpCnt->hWndFrame, pt.x, pt.y, cxWidth, cyHeight-1, FALSE );
    MoveWindow( lpCnt->hWndFrame, pt.x, pt.y, cxWidth, cyHeight, TRUE );
    lpCntFrame->bIntegralSizeChg = FALSE;

    return;
    }

DWORD WINAPI CntStyleGet( HWND hWnd )
    {
    LPCONTAINER  lpCnt = GETCNT(hWnd);

    return lpCnt->dwStyle;
    }

/****************************************************************************
 * CntSizeCheck
 *
 * Purpose:
 *  Given a potential width and height for a container, this function 
 *  returns the number of unused pixels to the right of the last field 
 *  for the given width and the number of visible pixels of a partially
 *  displayed bottom row in the record area. To eliminate the extra pixels
 *  after the last field and to have no partial records in view (heightwise)
 *  the application should reduce the width and height of the container
 *  by the number of pixels returned in lpcxUnused and lpcyPartial.
 *
 *  This function is useful for applications that want to have a container
 *  completely fill another window (like an MDI child or a sizeable dialog).
 *  Call this function when receiving a WM_WINDOWPOSCHANGING and shrink the
 *  width by lpcxUnused and shrink the height by lpcyPartial. Providing the
 *  container has styles CTS_INTEGRALWIDTH and CTS_INTEGRALHEIGHT enabled,
 *  the container will fill the outer window with no extra space around it.
 *
 *  NOTE: lpcxUnused will not be valid if this function is called when
 *        the target container is split.
 *
 * Parameters:
 *  HWND          hWndCnt     - handle to the container frame window
 *  int           cxWidth     - future container width  (including non-client area)
 *  int           cxHeight    - future container height (including non-client area)
 *  int           cxMaxWidth  - future max width of all container fields;
 *                              if=0 the current max width will be used;
 *                              (cxMaxWidth = nDisplayWidthOfAllFields * lpCnt->lpCI->cxChar)
 *  LPINT         lpcxUnused  - returned unused bk pixels to right of last field
 *  LPINT         lpcyPartial - returned height in pixels of partial bottom row
 *  LPINT         lpbHscroll  - returned flag: TRUE if there will be a HORZ scrollbar; else FALSE
 *  LPINT         lpbVscroll  - returned flag: TRUE if there will be a VERT scrollbar; else FALSE
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI CntSizeCheck( HWND hWndCnt, int cxWidth, int cyHeight, int cxMaxWidth,
                          LPINT lpcxUnused, LPINT lpcyPartial,
                          LPINT lpbHscroll, LPINT lpbVscroll )
    {
    LPCONTAINER lpCnt = GETCNT(hWndCnt);
    LPFIELDINFO lpFld = CntFldTailGet(hWndCnt);
    RECT rectWnd, rectClient;
    BOOL bHscroll=FALSE;
    BOOL bVscroll=FALSE;
    int  xLastCol, cxUnused, cyPartial;
    int  cyRecordArea, nRecsDisplayed;
    int  nHscrollMax=0;
    int  nVscrollMax=0;
    int  cxClient, cyClient, nExpand;
    int  cxNC, cyNC, nMaxWidth;
    int  cxVscrollBar, cyHscrollBar;

    // Make sure the frame window was passed in.
    if( hWndCnt != lpCnt->hWndFrame )
      lpCnt = GETCNT( lpCnt->hWndFrame );

    // Get the scroll bar metrics. The scroll bars share 1 pixel with the
    // border area of the window. When they appear they shrink the client
    // area of the window by one less than their width or height.
    cxVscrollBar = GetSystemMetrics( SM_CXVSCROLL ) - 1;
    cyHscrollBar = GetSystemMetrics( SM_CYHSCROLL ) - 1;

    // Calc the non-client area.
    GetWindowRect( lpCnt->hWndFrame, &rectWnd );
    GetClientRect( lpCnt->hWndFrame, &rectClient );
    cxNC = (rectWnd.right  - rectWnd.left) - rectClient.right;
    cyNC = (rectWnd.bottom - rectWnd.top)  - rectClient.bottom;

    // Calc what the container client area will be.
    cxClient = cxWidth  - cxNC;
    cyClient = cyHeight - cyNC;

    if ( lpCnt->iView & CV_ICON )
    {
      int nLeftPxls, nRightPxls;
      int nTopPxls, nBottomPxls;

      // don't make sense in Icon View
      cxUnused  = 0;
      cyPartial = 0;

      // see if need a vertical scroll bar
      if (lpCnt->lpCI->bArranged)
        nTopPxls = max(0, lpCnt->lpCI->rcViewSpace.top - 
                         (lpCnt->lpCI->rcWorkSpace.top - lpCnt->lpCI->aiInfo.yIndent));
      else  
        nTopPxls = max(0, lpCnt->lpCI->rcViewSpace.top - lpCnt->lpCI->rcWorkSpace.top);
      nBottomPxls = max(0, lpCnt->lpCI->rcWorkSpace.bottom - lpCnt->lpCI->rcViewSpace.bottom);
      if (nTopPxls + nBottomPxls > 0)
        bVscroll = TRUE;

      // see if need a horizontal scroll bar
      if (lpCnt->lpCI->bArranged)
        nLeftPxls  = max(0, lpCnt->lpCI->rcViewSpace.left  - 
                           (lpCnt->lpCI->rcWorkSpace.left - lpCnt->lpCI->aiInfo.xIndent));
      else  
        nLeftPxls  = max(0, lpCnt->lpCI->rcViewSpace.left  - lpCnt->lpCI->rcWorkSpace.left);
      nRightPxls = max(0, lpCnt->lpCI->rcWorkSpace.right - lpCnt->lpCI->rcViewSpace.right);
      
      if (nLeftPxls + nRightPxls > 0)
        bHscroll = TRUE;
    }
    else
    {
      // Use the current max width unless the user passed one in.
      cxMaxWidth = max(0, cxMaxWidth);
      if( cxMaxWidth )
        nMaxWidth = cxMaxWidth;
      else
        nMaxWidth = lpCnt->lpCI->nMaxWidth;

      // See if this width would require a horizontal scrollbar.
      if( lpCnt->dwStyle & CTS_HORZSCROLL )
      {
        if( nMaxWidth - cxClient > 0 )
          nHscrollMax = max(0, (nMaxWidth - cxClient - 1) / lpCnt->lpCI->cxChar + 1);
        else
          nHscrollMax = 0;

        // If there will be a horz scroll bar, shrink the client area height.
        if( nHscrollMax )
        {
          cyClient -= cyHscrollBar;
          bHscroll = TRUE;
        }
      }

      // Calc the vertical display area for the records.
      cyRecordArea = cyClient - lpCnt->lpCI->cyTitleArea - lpCnt->lpCI->cyColTitleArea;

      // See if this height would require a vertical scrollbar.
      if( lpCnt->dwStyle & CTS_VERTSCROLL )
      {
        nVscrollMax = max(0, lpCnt->iMax - cyRecordArea / lpCnt->lpCI->cyRow);

        // If there will be a vert scroll bar, shrink the client area width.
        if( nVscrollMax || lpCnt->lpCI->flCntAttr & CA_SHVERTSCROLL )
        {
          cxClient -= cxVscrollBar;
          bVscroll = TRUE;
        }
      }

      // If a vertical scroll is necessary, we have to check the width again. 
      // The vertical scroll bar caused the client width to shrink which
      // would cause the nHscrollMax of the horizontal scroll bar to change.
      if( bVscroll )
      {
        if( nMaxWidth - cxClient > 0 )
          nHscrollMax = max(0, (nMaxWidth - cxClient - 1) / lpCnt->lpCI->cxChar + 1);
        else
          nHscrollMax = 0;

        // If there will be a horz scroll bar, shrink the client area height
        // (if not done yet).
        if( nHscrollMax && !bHscroll )
        {
          cyClient -= cyHscrollBar;
          bHscroll = TRUE;
        }
      }

      // Calc the area for the records again (it may have changed).
      cyRecordArea = cyClient - lpCnt->lpCI->cyTitleArea - lpCnt->lpCI->cyColTitleArea;

      // If a horz scroll is necessary, we have to check the height again. 
      // The horz scroll bar caused the client height to shrink which
      // would cause the nVscrollMax of the vertical scroll bar to change.
      if( bHscroll )
      {
        nVscrollMax = max(0, lpCnt->iMax - cyRecordArea / lpCnt->lpCI->cyRow);

        // If there will be a vert scroll bar, shrink the client area width
        // (if not done yet).
        if( (nVscrollMax && !bVscroll) || lpCnt->lpCI->flCntAttr & CA_SHVERTSCROLL )
        {
          cxClient -= cxVscrollBar;
          bVscroll = TRUE;
        }
      }

      // Calc the unused bk pixels to the right of the last field.
      xLastCol = nMaxWidth - nHscrollMax * lpCnt->lpCI->cxChar;

      if( nMaxWidth && xLastCol < cxClient )
      {
        // Allow for last field expansion if the style is enabled.
        if( (lpCnt->dwStyle & CTS_EXPANDLASTFLD) && lpFld )
        {
          // Calc how many char units the field can expand and reset xLastCol.
          nExpand = (cxClient - xLastCol) / lpCnt->lpCI->cxChar;
          xLastCol += nExpand * lpCnt->lpCI->cxChar;
        }

        cxUnused = cxClient - xLastCol;
      }
      else
      {
        cxUnused = 0;
      }
 
      // Get the nbr of records which will be in view.
      nRecsDisplayed = (cyRecordArea-1) / lpCnt->lpCI->cyRow + 1;

      // Calc how many pixels of a partial bottom row will be visible.
      cyPartial = cyRecordArea - ((nRecsDisplayed-1) * lpCnt->lpCI->cyRow);
      if( cyPartial == lpCnt->lpCI->cyRow )
        cyPartial = 0;
    }

    // Return the values.
    *lpcxUnused  = cxUnused;
    *lpcyPartial = cyPartial;
    *lpbHscroll  = bHscroll;
    *lpbVscroll  = bVscroll;
    }

/****************************************************************************
 * CntRetainBaseHt
 *
 * Purpose:
 *  This function allows the application to control whether a container with
 *  CTS_INTEGRALHEIGHT enabled changes its base height when being sized by 
 *  the application. Normally, the container resets the base height each time
 *  it is sized by the application.
 *
 *  This function is useful for applications that want to have a container
 *  completely fill another window (like an MDI child or a sizeable dialog).
 *  Typically, this function would only be called if the application wants
 *  to resize the container width but does not want the current base height 
 *  to change.
 *  
 * Parameters:
 *  HWND          hWndCnt     - handle to the container frame window
 *  BOOL          bRetain     - if TRUE  the base height is NOT changed when sizing the container;
 *                              if FALSE the base height is changed when sizing the container
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI CntRetainBaseHt( HWND hWndCnt, BOOL bRetain )
    {
    LPCONTAINER lpCnt = GETCNT(hWndCnt);

    // Make sure the frame window was passed in.
    if( hWndCnt != lpCnt->hWndFrame )
      lpCnt = GETCNT( lpCnt->hWndFrame );

    lpCnt->bRetainBaseHt = bRetain;
    }

/****************************************************************************
 * CntUserDataSet
 *
 * Purpose:
 *  Copies user data from a buffer to the container.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *  LPVOID        lpUserData  - pointer to user data
 *  UINT          wUserBytes  - number of bytes to copy from lpUserData
 *
 * Return Value:
 *  LPVOID        void pointer to user data stored with container 
 ****************************************************************************/

LPVOID WINAPI CntUserDataSet( HWND hCntWnd, LPVOID lpUserData, UINT wUserBytes )
    {
    LPCNTINFO lpCI = (GETCNT(hCntWnd))->lpCI;

    // Free old user data if there was any.
    if( lpCI->lpUserData )
      {
/*      GlobalFreePtr(lpCI->lpUserData);*/
      free(lpCI->lpUserData);
      lpCI->lpUserData = NULL;
      }

    // Alloc space for the new user data.
/*    lpCI->lpUserData = (LPVOID) GlobalAllocPtr(GHND, wUserBytes);*/
    lpCI->lpUserData = (LPVOID) calloc(1, wUserBytes);
    lpCI->wUserBytes = lpCI->lpUserData ? wUserBytes : 0;

    // Move user data into the container struct.
    if( lpCI->lpUserData )
      _fmemcpy( (LPVOID) lpCI->lpUserData, lpUserData, (size_t) lpCI->wUserBytes);

    return lpCI->lpUserData;
    }

/****************************************************************************
 * CntUserDataGet
 *
 * Purpose:
 *  Get the pointer to a container's user data.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *
 * Return Value:
 *  LPVOID        void pointer to user data stored with container 
 ****************************************************************************/

LPVOID WINAPI CntUserDataGet( HWND hCntWnd )
    {
    LPCNTINFO lpCI = (GETCNT(hCntWnd))->lpCI;

    return lpCI->lpUserData;
    }

/****************************************************************************
 * CntNotifyMsgGet
 *
 * Purpose:
 *  Retrieves the current container notification message value.
 *  This procedure only needs to be used for containers using the
 *  asynchronous notification method (CTS_ASYNCNOTIFY) and should be
 *  called immediately upon receiving a notification message.
 *  
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  LPARAM        lParam      - lparam of a container notification msg
 *                              (ignored if CTS_ASYNCNOTIFY is not enabled)
 *
 * Return Value:
 *  UINT          current notification value
 ****************************************************************************/

UINT WINAPI CntNotifyMsgGet( HWND hCntWnd, LPARAM lParam )
    {
    LPCONTAINER lpCnt = GETCNT(hCntWnd);
#ifdef WIN16
    HGLOBAL     hEvent = (HGLOBAL)HIWORD(lParam);
#else
    HGLOBAL     hEvent = (HGLOBAL)lParam;
#endif
    LPCNEVENT   lpCNEvent;
    UINT        wCurNotify;

    if( (lpCnt->dwStyle & CTS_ASYNCNOTIFY) && hEvent )
      {
      lpCNEvent = (LPCNEVENT) GlobalLock( hEvent );
      if( lpCNEvent )
        {
/*char szDebug[100];*/
/*wsprintf( szDebug, "getting msg = %x\n", hEvent );*/
/*OutputDebugString(szDebug);*/
        wCurNotify = lpCNEvent->wCurNotify;
        GlobalUnlock( hEvent );
        return wCurNotify;
        }
      }
    else
      {
      return lpCnt->lpCI->wCurNotify;
      }
    }

/****************************************************************************
 * CntNotifyMsgDone
 *
 * Purpose:
 *  Release memory for an asynchronous container notification message.
 *  This procedure only needs to be used for containers using the
 *  asynchronous notification method (CTS_ASYNCNOTIFY) and should be
 *  called immediately after processing the notification.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  LPARAM        lParam      - lparam of a container notification msg
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI CntNotifyMsgDone( HWND hCntWnd, LPARAM lParam )
    {
    LPCONTAINER lpCnt = GETCNT(hCntWnd);
#ifdef WIN16
    HGLOBAL     hEvent = (HGLOBAL)HIWORD(lParam);
#else
    HGLOBAL     hEvent = (HGLOBAL)lParam;
#endif
    UINT        wCurNotify;

    if( (lpCnt->dwStyle & CTS_ASYNCNOTIFY) && hEvent )
      {    
/*char szDebug[100];*/
/*wsprintf( szDebug, "freeing msg = %x\n", hEvent );*/
/*OutputDebugString(szDebug);*/

      // 
      wCurNotify = CntNotifyMsgGet( hCntWnd, lParam );
      if( wCurNotify == CN_INSERT || wCurNotify == CN_DELETE )
        CntFocusRecUnlck( hCntWnd );

      GlobalUnlock( hEvent );
      GlobalFree( hEvent );
      }
    return;
    }

/****************************************************************************
 * CntCNChildWndGet
 * CntCNCharGet
 * CntCNIncExGet
 * CntCNIncGet
 * CntCNThumbTrkGet
 * CntCNRecGet
 * CntCNFldGet
 * CntCNShiftKeyGet
 * CntCNCtrlKeyGet
 * CntCNSplitBarGet
 * CntCNUserDataGet
 *
 * Purpose:
 *  Retrieve the handle of the child window sending the current notification.
 *  Also the char value, scrolling increment, record pointer, field pointer
 *  key state, or split bar coordinate associated with the current
 *  notification which is being processed by the parent. The user data is
 *  something passed through CntNotifyAssocEx that only the associate window
 *  knows about. In that case the assoc is throwing and catching without
 *  the container even aware of it.
 *
 *  These values are valid ONLY during the processing of the notification.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  LPARAM        lParam      - lparam of a container notification msg
 *                              (ignored if CTS_ASYNCNOTIFY is not enabled)
 *
 * Return Value:
 *  ansi char value/LPFIELDINFO/LPRECORDCORE
 ****************************************************************************/

HWND WINAPI CntCNChildWndGet( HWND hCntWnd, LPARAM lParam )
    {
    LPCONTAINER lpCnt = GETCNT(hCntWnd);
#ifdef WIN16
    HGLOBAL     hEvent = (HGLOBAL)HIWORD(lParam);
#else
    HGLOBAL     hEvent = (HGLOBAL)lParam;
#endif
    LPCNEVENT   lpCNEvent;
    HWND        hWndSender;
 
    if( (lpCnt->dwStyle & CTS_ASYNCNOTIFY) && hEvent )
      {
      lpCNEvent = (LPCNEVENT) GlobalLock( hEvent );
      if( lpCNEvent )
        {
        hWndSender = lpCNEvent->hWndCurCNSender;
        GlobalUnlock( hEvent );
        return hWndSender;
        }
      }
    else
      {
      return lpCnt->lpCI->hWndCurCNSender;
      }
    }

UINT WINAPI CntCNCharGet( HWND hCntWnd, LPARAM lParam )
    {
    LPCONTAINER lpCnt = GETCNT(hCntWnd);
#ifdef WIN16
    HGLOBAL     hEvent = (HGLOBAL)HIWORD(lParam);
#else
    HGLOBAL     hEvent = (HGLOBAL)lParam;
#endif
    LPCNEVENT   lpCNEvent;
    UINT        wCurAnsiChar;
 
    if( (lpCnt->dwStyle & CTS_ASYNCNOTIFY) && hEvent )
      {
      lpCNEvent = (LPCNEVENT) GlobalLock( hEvent );
      if( lpCNEvent )
        {
        wCurAnsiChar = lpCNEvent->wCurAnsiChar;
        GlobalUnlock( hEvent );
        return wCurAnsiChar;
        }  
      }  
    else
      {
      return lpCnt->lpCI->wCurAnsiChar;
      }
    }

LONG WINAPI CntCNIncExGet( HWND hCntWnd, LPARAM lParam )
    {
    LPCONTAINER lpCnt = GETCNT(hCntWnd);
#ifdef WIN16
    HGLOBAL     hEvent = (HGLOBAL)HIWORD(lParam);
#else
    HGLOBAL     hEvent = (HGLOBAL)lParam;
#endif
    LPCNEVENT   lpCNEvent;
    LONG        lInc;
 
    if( (lpCnt->dwStyle & CTS_ASYNCNOTIFY) && hEvent )
      {
      lpCNEvent = (LPCNEVENT) GlobalLock( hEvent );
      if( lpCNEvent )
        {
        lInc = lpCNEvent->lCurCNInc;
        GlobalUnlock( hEvent );
        return lInc;
        }
      }
    else
      {
      return lpCnt->lCurCNInc;
      }
    }

int WINAPI CntCNIncGet( HWND hCntWnd, LPARAM lParam )
    {
    return (int) CntCNIncExGet( hCntWnd, lParam );
    }

int WINAPI CntCNThumbTrkGet( HWND hCntWnd, LPARAM lParam )
    {
    // The thumb track position uses the increment slot.
    return (int) CntCNIncExGet( hCntWnd, lParam );
    }

LPRECORDCORE WINAPI CntCNRecGet( HWND hCntWnd, LPARAM lParam )
    {
    LPCONTAINER   lpCnt = GETCNT(hCntWnd);
#ifdef WIN16
    HGLOBAL       hEvent = (HGLOBAL)HIWORD(lParam);
#else
    HGLOBAL       hEvent = (HGLOBAL)lParam;
#endif
    LPCNEVENT     lpCNEvent;
    LPRECORDCORE  lpRec;
  
    if( (lpCnt->dwStyle & CTS_ASYNCNOTIFY) && hEvent )
      {
      lpCNEvent = (LPCNEVENT) GlobalLock( hEvent );
      if( lpCNEvent )
        {
        lpRec = lpCNEvent->lpCurCNRec;
        GlobalUnlock( hEvent );
        return lpRec;
        }
      }
    else
      {
      return lpCnt->lpCI->lpCurCNRec;
      }
    }

LPFIELDINFO WINAPI CntCNFldGet( HWND hCntWnd, LPARAM lParam )
    {
    LPCONTAINER  lpCnt = GETCNT(hCntWnd);
#ifdef WIN16
    HGLOBAL      hEvent = (HGLOBAL)HIWORD(lParam);
#else
    HGLOBAL      hEvent = (HGLOBAL)lParam;
#endif
    LPCNEVENT    lpCNEvent;
    LPFIELDINFO  lpFld;
   
    if( (lpCnt->dwStyle & CTS_ASYNCNOTIFY) && hEvent )
      {
      lpCNEvent = (LPCNEVENT) GlobalLock( hEvent );
      if( lpCNEvent )
        {
        lpFld =lpCNEvent->lpCurCNFld;
        GlobalUnlock( hEvent );
        return lpFld;
        }
      }
    else
      {
      return lpCnt->lpCI->lpCurCNFld;
      }
    }

LPVOID WINAPI CntCNUserDataGet( HWND hCntWnd, LPARAM lParam )
    {
    LPCONTAINER  lpCnt = GETCNT(hCntWnd);
#ifdef WIN16
    HGLOBAL      hEvent = (HGLOBAL)HIWORD(lParam);
#else
    HGLOBAL      hEvent = (HGLOBAL)lParam;
#endif
    LPCNEVENT    lpCNEvent;
    LPVOID       lpUser;
   
    if( (lpCnt->dwStyle & CTS_ASYNCNOTIFY) && hEvent )
      {
      lpCNEvent = (LPCNEVENT) GlobalLock( hEvent );
      if( lpCNEvent )
        {
        lpUser = lpCNEvent->lpCurCNUser;
        GlobalUnlock( hEvent );
        return lpUser;
        }
      }
    else
      {
      return lpCnt->lpCI->lpCurCNUser;
      }
    }

BOOL WINAPI CntCNShiftKeyGet( HWND hCntWnd, LPARAM lParam )
    {
    LPCONTAINER lpCnt = GETCNT(hCntWnd);
#ifdef WIN16
    HGLOBAL     hEvent = (HGLOBAL)HIWORD(lParam);
#else
    HGLOBAL     hEvent = (HGLOBAL)lParam;
#endif
    LPCNEVENT   lpCNEvent;
    BOOL        bShiftKey;
 
    if( (lpCnt->dwStyle & CTS_ASYNCNOTIFY) && hEvent )
      {
      lpCNEvent = (LPCNEVENT) GlobalLock( hEvent );
      if( lpCNEvent )
        {
        bShiftKey = lpCNEvent->bCurCNShiftKey;
        GlobalUnlock( hEvent );
        return bShiftKey;
        }
      }
    else
      {
      return lpCnt->lpCI->bCurCNShiftKey;
      }
    }

BOOL WINAPI CntCNCtrlKeyGet( HWND hCntWnd, LPARAM lParam )
    {
    LPCONTAINER lpCnt = GETCNT(hCntWnd);
#ifdef WIN16
    HGLOBAL     hEvent = (HGLOBAL)HIWORD(lParam);
#else
    HGLOBAL     hEvent = (HGLOBAL)lParam;
#endif
    LPCNEVENT   lpCNEvent;
    BOOL        bCtrlKey;
 
    if( (lpCnt->dwStyle & CTS_ASYNCNOTIFY) && hEvent )
      {
      lpCNEvent = (LPCNEVENT) GlobalLock( hEvent );
      if( lpCNEvent )
        {
        bCtrlKey = lpCNEvent->bCurCNCtrlKey;
        GlobalUnlock( hEvent );
        return bCtrlKey;
        }
      }
    else
      {
      return lpCnt->lpCI->bCurCNCtrlKey;
      }
    }

int WINAPI CntCNSplitBarGet( HWND hCntWnd, LPARAM lParam )
    {
    LPCONTAINER lpCnt = GETCNT(hCntWnd);
#ifdef WIN16
    HGLOBAL     hEvent = (HGLOBAL)HIWORD(lParam);
#else
    HGLOBAL     hEvent = (HGLOBAL)lParam;
#endif
    LPCNEVENT   lpCNEvent;
    int         nInc;
 
    if( (lpCnt->dwStyle & CTS_ASYNCNOTIFY) && hEvent )
      {
      lpCNEvent = (LPCNEVENT) GlobalLock( hEvent );
      if( lpCNEvent )
        {
        // Split bar notifications use the inc slot.
        nInc = (int)lpCNEvent->lCurCNInc;
        GlobalUnlock( hEvent );
        return nInc;
        }
      }
    else
      {
      return (int)lpCnt->lCurCNInc;
      }
    }

/****************************************************************************
 * CntFldDefine
 *
 * Purpose:
 *  Defines a field with the specified characteristics.
 *
 * Parameters:
 *  LPFIELDINFO   lpFI          - to the container field
 *  UINT          wFldType      - field data type
 *  DWORD         flColAttr     - field attribute flags
 *  UINT          wFTitleLines  - nbr of lines in field title (if string)
 *  DWORD         flFTitleAlign - field title text alignment flags
 *  DWORD         flFDataAlign  - field data text alignment flags
 *  UINT          xWidth        - width of the field in characters
 *  UINT          xEditWidth    - width in chars to use when editing
 *  UINT          yEditLines    - nbr of lines to use when editing
 *  UINT          wOffStruct    - offset from lpRecData to field data
 *  UINT          wDataBytes    - nbr of bytes used for storing data
 *
 * Return Value:
 *  BOOL          TRUE: success;  FALSE: error
 ****************************************************************************/

BOOL WINAPI CntFldDefine( LPFIELDINFO lpFI, UINT wFldType, DWORD flColAttr,
                          UINT wFTitleLines, DWORD flFTitleAlign,
                          DWORD flFDataAlign, UINT xWidth,
                          UINT xEditWidth, UINT yEditLines,
                          UINT wOffStruct, UINT wDataBytes )
    {
    if( !lpFI )
      return FALSE;

    // load up structure
    lpFI->wColType      = wFldType;
    lpFI->flColAttr     = flColAttr;
    lpFI->wFTitleLines  = wFTitleLines;
    lpFI->flFTitleAlign = flFTitleAlign;
    lpFI->flFDataAlign  = flFDataAlign;
    lpFI->cxWidth       = xWidth;
    lpFI->xEditWidth    = xEditWidth;
    lpFI->yEditLines    = yEditLines;
    lpFI->wOffStruct    = wOffStruct;
    lpFI->wDataBytes    = wDataBytes;

    return TRUE;
    }

/****************************************************************************
 * CntFldUserSet
 * CntFldUserGet
 *
 * Purpose:
 *  Loads/Gets the user data for a field.
 *
 * Parameters:
 *  LFIELDINFO    lpFI        - pointer to the field info struct
 *  LPVOID        lpUserData  - user data
 *  UINT          wUserBytes  - number of bytes to copy from lpUserData
 *
 * Return Value:
 *  BOOL          TRUE: success;  FALSE: error
 ****************************************************************************/

BOOL WINAPI CntFldUserSet( LPFIELDINFO lpFI, LPVOID lpUserData, UINT wUserBytes )
    {
    if( !lpFI || !lpUserData || !wUserBytes )
      return FALSE;

    // Free old user data if there was any.
    if( lpFI->lpUserData )
      {
/*      GlobalFreePtr(lpFI->lpUserData);*/
      free(lpFI->lpUserData);
      lpFI->lpUserData = NULL;
      }

    // Alloc space for the new user data.
/*    lpFI->lpUserData = (LPVOID) GlobalAllocPtr(GHND, wUserBytes);*/
    lpFI->lpUserData = (LPVOID) calloc(1, wUserBytes);
    lpFI->wUserBytes = lpFI->lpUserData ? wUserBytes : 0;

    if (lpFI->lpUserData)
      {
      _fmemcpy( (LPVOID) lpFI->lpUserData, lpUserData, (size_t) lpFI->wUserBytes);
      return TRUE;
      }
    else
      return FALSE;
    }

LPVOID WINAPI CntFldUserGet( LPFIELDINFO lpFld )
    {
    return lpFld->lpUserData;       // user defined field data
    }

/****************************************************************************
 * CntFldIndexSet
 * CntFldIndexGet
 *
 * Purpose:
 *  Sets/Gets field index (arbitrary-- for application reference only)
 *
 * Parameters:
 *  LFIELDINFO    lpFI        - pointer to the field info struct
 *  UINT          wIndex      - index number
 *
 * Return Value:
 *  UINT          Field Index value (get only)
 ****************************************************************************/

VOID WINAPI CntFldIndexSet( LPFIELDINFO lpFI, UINT wIndex )
    {
    if (lpFI)
      lpFI->wIndex = wIndex;
    }

UINT WINAPI CntFldIndexGet( LPFIELDINFO lpFI )
    {
    return (lpFI ? lpFI->wIndex : 0);
    }

/****************************************************************************
 * CntTtlBmpSet
 *
 * Purpose:
 *  Sets a bitmap in the container title area. The bitmap can be aligned
 *  the same as a string. It can also have precision placement by specifying
 *  an X or Y offset (in pixels) which is applied after the alignment.
 *  The bitmap can also be specified as having a transparent background by
 *  turning on the bTransparent flag and specifying the X/Y position of a
 *  pixel in the bitmap that contains the background color.
 *
 * Parameters:
 *  HWND          hCntWnd     - Container window handle 
 *  HBITMAP       hBmpTtl     - bitmap to be drawn in title area
 *  DWORD         flBmpAlign  - title bitmap alignment flags
 *  int           xOffBmp     - horz offset for title bitmap
 *  int           yOffBmp     - vert offset for title bitmap
 *  int           xTransPxl   - xpos of transparent bkgrnd pixel for title bitmap 
 *  int           yTransPxl   - ypos of transparent bkgrnd pixel for title bitmap 
 *  BOOL          bTransparent- if TRUE bmp background is transparent
 *
 * Return Value:
 *  BOOL          TRUE: success;  FALSE: error
 ****************************************************************************/

BOOL WINAPI CntTtlBmpSet( HWND hCntWnd, HBITMAP hBmpTtl,
                              DWORD flBmpAlign, int xOffBmp, int yOffBmp,
                              int xTransPxl, int yTransPxl, BOOL bTransparent )
    {
    LPCONTAINER lpCnt = GETCNT(hCntWnd);
    LPCNTINFO   lpCI  = (GETCNT(hCntWnd))->lpCI;
    RECT        rect;
    HBITMAP     hBmpOld;
    HDC         hDC;
    int         xExt, yExt, cxPxlWidth, cxWidth;
    int         xOrgBmp, yOrgBmp;

    if( !lpCI )
      return FALSE;

    hBmpOld = lpCI->hBmpTtl;
    
    // Set the bitmap info into the field struct.
    lpCI->hBmpTtl       = hBmpTtl;
    lpCI->flTtlBmpAlign = flBmpAlign;
    lpCI->xOffTtlBmp    = xOffBmp;
    lpCI->yOffTtlBmp    = yOffBmp;

    // Set the transparent background info if they want the bkgrnd blended.
    if( bTransparent )
      {
      lpCI->flCntAttr   |= CA_TRANTTLBMP;
      lpCI->xTPxlTtlBmp  = xTransPxl;
      lpCI->yTPxlTtlBmp  = yTransPxl;
      }

    // update the container title where the bitmap lies - have to figure out where the bitmap is
    if ( lpCI->hBmpTtl != hBmpOld )
    {
      // Get the DC
      hDC = GetDC( lpCnt->hWnd1stChild );
      
      // start with the title rect
      rect.left   = 0;
      rect.top    = 0;
      rect.right  = lpCnt->cxClient;
      rect.bottom = lpCI->cyTitleArea;
      
      // if there is a title button, adjust for it
      if( lpCI->nTtlBtnWidth )
      {
        // Get the title button width in pixels.
        if( lpCI->nTtlBtnWidth > 0 )
          cxPxlWidth = lpCI->cxCharTtl * lpCI->nTtlBtnWidth;
        else
          cxPxlWidth = -lpCI->nTtlBtnWidth;

        // Make room for the button in the title area.
        if( lpCI->bTtlBtnAlignRt )
          rect.right -= cxPxlWidth;
        else
          rect.left  += cxPxlWidth;
      }

      // Get the bitmap extents.
      GetBitmapExt( hDC, lpCI->hBmpTtl, &xExt, &yExt );
      ReleaseDC( lpCnt->hWnd1stChild, hDC );

      // setup the horizontal alignment
      cxWidth = rect.right - rect.left + 1;

      if( lpCI->flTtlBmpAlign & CA_TA_HCENTER )
        xOrgBmp = rect.left + cxWidth/2 - xExt/2;
      else if( lpCI->flTtlBmpAlign & CA_TA_RIGHT )
        xOrgBmp = rect.right - xExt - lpCI->cxCharTtl2;
      else
        xOrgBmp = rect.left + lpCI->cxCharTtl2;
  
      // setup the vertical alignment
      if( lpCI->flTtlBmpAlign & CA_TA_VCENTER )
        yOrgBmp = lpCI->cyTitleArea/2 - yExt/2;
      else if( lpCI->flTtlBmpAlign & CA_TA_BOTTOM )
        yOrgBmp = lpCI->cyTitleArea - yExt - lpCI->cyCharTtl4;
      else
        yOrgBmp = lpCI->cyCharTtl4;

      // Apply any offset.
      xOrgBmp += lpCI->xOffTtlBmp;
      yOrgBmp += lpCI->yOffTtlBmp;

      // the final rect
      rect.left   = xOrgBmp;
      rect.top    = yOrgBmp;
      rect.right  = rect.left + xExt;
      rect.bottom = rect.top  + yExt;
    
      UpdateContainer( lpCnt->hWnd1stChild, &rect, TRUE );
    }

    return TRUE;
    }

/****************************************************************************
 * CntFldTtlBmpSet
 *
 * Purpose:
 *  Sets a bitmap in the title area of a field. The bitmap can be aligned
 *  the same as a string. It can also have precision placement by specifying
 *  an X or Y offset (in pixels) which is applied after the alignment.
 *  The bitmap can also be specified as having a transparent background by
 *  turning on the bTransparent flag and specifying the X/Y position of a
 *  pixel in the bitmap that contains the background color.
 *
 * Parameters:
 *  LPFIELDINFO   lpFI        - to the container field
 *  HBITMAP       hBmpFldTtl  - bitmap to be drawn in field title area
 *  DWORD         flFTBmpAlign- field title bitmap alignment flags
 *  int           xOffBmp     - horz offset for field title bitmap
 *  int           yOffBmp     - vert offset for field title bitmap
 *  int           xTransPxl   - xpos of transparent bkgrnd pixel for field title bitmap 
 *  int           yTransPxl   - ypos of transparent bkgrnd pixel for field title bitmap 
 *  BOOL          bTransparent- if TRUE bmp background is transparent
 *
 * Return Value:
 *  BOOL          TRUE: success;  FALSE: error
 ****************************************************************************/

BOOL WINAPI CntFldTtlBmpSet( LPFIELDINFO lpFI, HBITMAP hBmpFldTtl,
                                 DWORD flFTBmpAlign, int xOffBmp, int yOffBmp,
                                 int xTransPxl, int yTransPxl, BOOL bTransparent )
    {
    if( !lpFI )
      return FALSE;

    // Set the bitmap info into the field struct.
    lpFI->hBmpFldTtl    = hBmpFldTtl;
    lpFI->flFTBmpAlign  = flFTBmpAlign;
    lpFI->xOffFldTtlBmp = xOffBmp;
    lpFI->yOffFldTtlBmp = yOffBmp;

    // Set the transparent background info if they want the bkgrnd blended.
    if( bTransparent )
      {
      lpFI->flColAttr     |= CFA_TRANFLDTTLBMP;
      lpFI->xTPxlFldTtlBmp = xTransPxl;
      lpFI->yTPxlFldTtlBmp = yTransPxl;
      }

    return TRUE;
    }

/****************************************************************************
 * CntTtlBtnSet
 *
 * Purpose:
 *  Places a button in the container title area. The button can be aligned
 *  at the right or left of the title area and will be the same height as
 *  the title area. When the user clicks on the button the associate window
 *  will receive a CN_TTLBTNCLK notification. The button can have text or a
 *  bitmap or both drawn on the face. If text is set it will use the same
 *  font as the container title. The bitmap and the text can be aligned
 *  independently. The bitmap can have precision placement by specifying
 *  an X or Y offset (in pixels) which is applied after the alignment.
 *  The bitmap can also be specified as having a transparent background by
 *  turning on the bTransparent flag and specifying the X/Y position of a
 *  pixel in the bitmap that contains the background color.
 *
 * Parameters:
 *  HWND          hCntWnd       - Container window handle 
 *  int           nBtnWidth     - width of the button in characters (if pos) or pixels (if neg)
 *  int           nBtnPlacement - where button will be drawn (on the right or left)
 *  LPSTR         lpszText      - text to be drawn on button face (NULL terminated)
 *  DWORD         flTxtAlign    - text alignment flags
 *  HBITMAP       hBitmap       - bitmap to be drawn on button face
 *  DWORD         flBmpAlign    - bitmap alignment flags
 *  int           xOffBmp       - horz offset for bitmap
 *  int           yOffBmp       - vert offset for bitmap
 *  int           xTransPxl     - xpos of transparent bkgrnd pixel for bitmap 
 *  int           yTransPxl     - ypos of transparent bkgrnd pixel for bitmap 
 *  BOOL          bTransparent  - if TRUE bitmap background is transparent
 *
 * Return Value:
 *  BOOL          TRUE: success;  FALSE: error
 ****************************************************************************/

BOOL WINAPI CntTtlBtnSet( HWND hCntWnd, int nBtnWidth, int nBtnPlacement,
                              LPSTR lpszText, DWORD flTxtAlign, HBITMAP hBitmap,
                              DWORD flBmpAlign, int xOffBmp, int yOffBmp,
                              int xTransPxl, int yTransPxl, BOOL bTransparent )
    {
    LPCNTINFO lpCI = (GETCNT(hCntWnd))->lpCI;
    UINT wLen;

    if( !lpCI )
      return FALSE;

    // Set the title button data into the container info struct.
    lpCI->nTtlBtnWidth     = nBtnWidth;
    lpCI->bTtlBtnAlignRt   = nBtnPlacement;
    lpCI->flTtlBtnTxtAlign = flTxtAlign;
    lpCI->hBmpTtlBtn       = hBitmap;
    lpCI->flTtlBtnBmpAlign = flBmpAlign;
    lpCI->xOffTtlBtnBmp    = xOffBmp;
    lpCI->yOffTtlBtnBmp    = yOffBmp;
    
    if( lpszText )
      {
      wLen = min(lstrlen( lpszText ), MAX_BTNTXT_LEN);
      _fmemcpy(lpCI->szTtlBtnTxt, lpszText, wLen );
      }

    // Set the transparent background info if they want the bkgrnd blended.
    if( bTransparent )
      {
      lpCI->flCntAttr     |= CA_TRANTTLBTNBMP;
      lpCI->xTPxlTtlBtnBmp = xTransPxl;
      lpCI->yTPxlTtlBtnBmp = yTransPxl;
      }

    return TRUE;
    }

/****************************************************************************
 * CntFldTtlBtnSet
 *
 * Purpose:
 *  Places a button in the field title area. The button can be aligned at
 *  the right or left of the field title area and will be the same height as
 *  the field title area. When the user clicks on the button the associate
 *  window will receive a CN_FLDTTLBTNCLK notification. The button can have
 *  text or a bitmap or both drawn on the face. If text is set it will use
 *  the same font as the field title. The bitmap and the text can be aligned
 *  independently. The bitmap can have precision placement by specifying
 *  an X or Y offset (in pixels) which is applied after the alignment.
 *  The bitmap can also be specified as having a transparent background by
 *  turning on the bTransparent flag and specifying the X/Y position of a
 *  pixel in the bitmap that contains the background color.
 *
 * Parameters:
 *  LPFIELDINFO   lpFI          - to the container field
 *  int           nBtnWidth     - width of the button in characters (if pos) or pixels (if neg)
 *  int           nBtnPlacement - where button will be drawn (on the right or left)
 *  LPSTR         lpszText      - text to be drawn on button face (NULL terminated)
 *  DWORD         flTxtAlign    - text alignment flags
 *  HBITMAP       hBitmap       - bitmap to be drawn on button face
 *  DWORD         flBmpAlign    - bitmap alignment flags
 *  int           xOffBmp       - horz offset for the bitmap
 *  int           yOffBmp       - vert offset for the bitmap
 *  int           xTransPxl     - xpos of transparent bkgrnd pixel for the bitmap 
 *  int           yTransPxl     - ypos of transparent bkgrnd pixel for the bitmap 
 *  BOOL          bTransparent  - if TRUE bmp background is transparent
 *
 * Return Value:
 *  BOOL          TRUE: success;  FALSE: error
 ****************************************************************************/

BOOL WINAPI CntFldTtlBtnSet( LPFIELDINFO lpFI, int nBtnWidth, int nBtnPlacement,
                                 LPSTR lpszText, DWORD flTxtAlign, HBITMAP hBitmap,
                                 DWORD flBmpAlign, int xOffBmp, int yOffBmp,
                                 int xTransPxl, int yTransPxl, BOOL bTransparent )
    {
    UINT wLen;

    if( !lpFI )
      return FALSE;

    // Set the field button data into the field info struct.
    lpFI->nFldBtnWidth     = nBtnWidth;
    lpFI->bFldBtnAlignRt   = nBtnPlacement;
    lpFI->flFldBtnTxtAlign = flTxtAlign;
    lpFI->hBmpFldBtn       = hBitmap;
    lpFI->flFldBtnBmpAlign = flBmpAlign;
    lpFI->xOffFldBtnBmp    = xOffBmp;
    lpFI->yOffFldBtnBmp    = yOffBmp;

    if( lpszText )
      {
      wLen = min(lstrlen( lpszText ), MAX_BTNTXT_LEN);
      _fmemcpy(lpFI->szFldBtnTxt, lpszText, wLen );
      }

    // Set the transparent background info if they want the bkgrnd blended.
    if( bTransparent )
      {
      lpFI->flColAttr     |= CFA_TRANFLDBTNBMP;
      lpFI->xTPxlFldBtnBmp = xTransPxl;
      lpFI->yTPxlFldBtnBmp = yTransPxl;
      }

    return TRUE;
    }

/****************************************************************************
 * CntFldCvtProcSet
 *
 * Purpose:
 *  Registers an application defined callback for converting custom data.
 *  This function must be used for fields defined as type CFT_CUSTOM.
 *
 * Parameters:
 *  LPFIELDINFO   lpFI        - pointer to field with type CFT_CUSTOM
 *  CVTPROC       lpfnCvtProc - Specifies the procedure-instance address of
 *                              the application-defined callback function.
 *                              The address MUST be created by the       
 *                              MakeProcInstance function.      
 *
 *  When the container needs to display custom data to the screen in string
 *  format this function is called to convert the data from internal format
 *  to string format. It must be defined in the application as follows:
 * 
 *  int CALLBACK ConvertData( HWND   hCntWnd,   - container wnd handle
 *                            LPVOID lpDesc,    - app-defined descriptor
 *                            LPVOID lpData,    - data in internal format
 *                            UINT   wLenOut,   - len of lpszOut buffer
 *                            LPSTR  lpszOut )  - data in string format
 *
 *  The application defined descriptor which is passed here is set by
 *  calling CntFldDescSet.
 *
 *  Sample usage (includes setting a field descriptor):
 *
 *      CVTPROC lpfnCvtProc;
 *      lpfnCvtProc = (CVTPROC) MakeProcInstance( ConvertData, hInstance );
 *      CntFldCvtProcSet( lpFI, lpfnCvtProc );
 *      CntFldDescSet( lpFI, lpDesc, xxx );
 *
 *  The ConvertData name is a placeholder for the application-defined
 *  function name. The actual name must be exported by including it in an
 *  EXPORTS statement in the application's module-definition (.DEF) file.
 *
 * Return Value:
 *  BOOL          TRUE: success;  FALSE: error
 ****************************************************************************/

BOOL WINAPI CntFldCvtProcSet( LPFIELDINFO lpFI, CVTPROC lpfnCvtProc )
    {
    if( !lpFI )
      return FALSE;

    // Set the function address into the field struct.
    lpFI->lpfnCvtToStr = lpfnCvtProc;
    return TRUE;
    }

/****************************************************************************
 * CntFldDescSet
 *
 * Purpose:
 *  Sets a descriptor which is used in converting field data from internal
 *  format to external format. For most field types this will be a null 
 *  terminated character string containing formatting specifications similar
 *  to those used in printf() (such as "%3d"). If a descriptor is not set
 *  the data is displayed in a default format appropriate for its type.
 *
 * Parameters:
 *  LPFIELDINFO   lpFI        - pointer to field
 *  LPVOID        lpDesc      - descriptor to be used in converting field data
 *  UINT          wDescBytes  - number of bytes to copy from lpDesc
 *                              (including null terminator if applicable)
 *
 *  The following conversion types are supported for field data:
 *
 *        %s        string
 *        %d %ld    decimal, long decimal
 *        %u %lu    unsigned int, long unsigned int
 *        %x %lx    hex, long hex
 *        %o %lo    octal, long octal
 *        %b %lb    binary, long binary
 *        %f %lf    float, double
 *        %e %le    float, double
 *        %E %lE    float, double
 *        %p        far pointer (in hex xxxx:xxxx)
 *
 *  The following modifiers are supported for all conversions:
 *
 *        %0x       zero left fill
 *        %-x       left justify in field (unnecessary when using container justification)
 *        %|x       center in field       (unnecessary when using container justification)
 *
 *  Precision is supported for strings and floating point:
 *
 *        %X.Y      strings: X-character wide field, print at most Y
 *                           characters (even if the string is longer).
 *
 *                  floats:  X-character wide field, print Y digits after
 *                           the decimal point. Default precision=6.
 *
 *  For field type CFT_CUSTOM use of a descriptor and this function is optional.
 *  For field types CFT_CANUMBER, CFT_CANUMBERUNSGN, CFT_CAMASK, CFT_CADATE,
 *  and CFT_CATIME this function MUST be used to set descriptors appropriate 
 *  to those types. See documentation on those custom controls for information
 *  concerning specifying their descriptors.
 *
 * Return Value:
 *  BOOL          TRUE: success;  FALSE: error
 ****************************************************************************/

BOOL WINAPI CntFldDescSet( LPFIELDINFO lpFI, LPVOID lpDesc, UINT wDescBytes )
    {
    if( !lpFI || !lpDesc || !wDescBytes )
      return FALSE;

    // Free old descriptor if there was one.
    if( lpFI->lpDescriptor )
      {
/*      GlobalFreePtr(lpFI->lpDescriptor);*/
      free(lpFI->lpDescriptor);
      lpFI->lpDescriptor = NULL;
      }

    // Alloc space for the new descriptor.
/*    lpFI->lpDescriptor = (LPVOID) GlobalAllocPtr(GHND, wDescBytes);*/
    lpFI->lpDescriptor = (LPVOID) calloc(1, wDescBytes);
    lpFI->wDescBytes = lpFI->lpDescriptor ? wDescBytes : 0;

    if( lpFI->lpDescriptor )
      {
      _fmemcpy( lpFI->lpDescriptor, lpDesc, (size_t) lpFI->wDescBytes );
      lpFI->bDescEnabled = TRUE;
      }

    return TRUE;
    }

/****************************************************************************
 * CntFldDescEnable
 *
 * Purpose:
 *  Enables or disables a field descriptor. A descriptor is automatically
 *  enabled when it is initially set with a call to CntFldDescSet. If a
 *  descriptor is disabled field data is displayed in a default manner
 *  appropriate for its type.
 *
 *  This function does not affect field types CFT_CUSTOM, CFT_CANUMBER, 
 *  CFT_CANUMBERUNSGN, CFT_CAMASK, CFT_CADATE, or CFT_CATIME. 
 *
 * Parameters:
 *  LPFIELDINFO   lpFI        - pointer to field with descriptor
 *  BOOL          bEnable     - flag indicating whether to enable descriptor:
 *                              if TRUE  descriptor will be used;
 *                              if FALSE descriptor will NOT be used
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI CntFldDescEnable( LPFIELDINFO lpFI, BOOL bEnable )
    {
    if( !lpFI )
      return;

    lpFI->bDescEnabled = bEnable;

    return;
    }

/****************************************************************************
 * CntFldDrwProcSet
 *
 * Purpose:
 *  Registers an application defined callback for drawing field data.
 *  This function must be used for fields with CFA_OWNERDRAW enabled.
 *
 * Parameters:
 *  LPFIELDINFO   lpFI         - pointer to CFA_OWNERDRAW field
 *  DRAWPROC      lpfnDrawProc - Specifies the procedure-instance address of
 *                               the application-defined callback function.
 *                               The address MUST be created by the       
 *                               MakeProcInstance function.      
 *
 *  When the data for a field has been converted to string format and is
 *  ready to be displayed on the screen this function is called to execute
 *  the draw. ExtTextOut should be used. The data in its internal format is
 *  also passed in to allow the application to check its value and possibly 
 *  change the text or background color accordingly. The application must
 *  take care to restore the original text color or background color if it
 *  does this. It must also be sensitive to the selection status of the record
 *  being drawn as selected records have a different text and background color.
 *  The drawing callback must be defined in the application as follows
 *  (note that the last 7 args are prepared for ExtTextOut):
 * 
 *  int CALLBACK DrawFldData( HWND   hCntWnd,     - container window handle
 *                            LPFIELDINFO  lpFld, - field being drawn
 *                            LPRECORDCORE lpRec, - record being drawn
 *                            LPVOID lpData,      - data in internal format
 *                            HDC    hDC,         - container window DC
 *                            int    nXstart,     - Xcoord of starting position
 *                            int    nYstart,     - Ycoord of starting position
 *                            UINT   fuOptions,   - rectangle type
 *                            LPRECT lpRect,      - rect of data cell
 *                            LPSTR  lpszOut,     - data in string format
 *                            UINT   uLenOut )    - len of lpszOut buffer
 *
 *  Sample usage:   
 *
 *      DRAWPROC lpfnDrawProc;
 *      lpfnDrawProc = (DRAWPROC) MakeProcInstance( DrawFldData, hInstance );
 *      CntFldDrwProcSet( lpFI, lpfnDrawProc );
 *
 *  The DrawFldData name is a placeholder for the application-defined
 *  function name. The actual name must be exported by including it in an
 *  EXPORTS statement in the application's module-definition (.DEF) file.
 *
 * Return Value:
 *  BOOL          TRUE: success;  FALSE: error
 ****************************************************************************/

BOOL WINAPI CntFldDrwProcSet( LPFIELDINFO lpFI, DRAWPROC lpfnDrawProc )
    {
    if( !lpFI )
      return FALSE;

    // Set the function address into the field struct.
    lpFI->lpfnDrawData = lpfnDrawProc;
    return TRUE;
    }

/****************************************************************************
 * CntPaintProcSet
 *
 *  Registers an application defined callback for painting various sections
 *  of the container background. This function must be used for containers
 *  with the CA_OWNERPNTBK, CA_OWNERPNTTTLBK, CA_OWNERPNTFTBK, or
 *  CFA_OWNERPNTFTBK attributes enabled.
 *
 *  Params              Definition
 *  -----------------   --------------------------------------------
 *  hCntWnd             Container window handle 
 *  lpfnPaintProc       Specifies the procedure-instance address of
 *                      the application-defined callback function.
 *                      The address MUST be created by the       
 *                      MakeProcInstance function.      
 *
 *  When the container needs to paint the general background, unused
 *  background, title background, or field title background the callback
 *  function registered here is called to paint the rectangle for that
 *  particular area. The unused background is any exposed area to the right
 *  of the last field. Considering that the application already has color
 *  control over all these areas, owner painting of the background should
 *  be necessary only when it is desirous to use a special brush or paint
 *  a bitmap in the particular background area. If you set a color for the
 *  background area it will be the current background color when the callback 
 *  function is called. You don't need to set it again in that function.
 *
 *  The painting callback must be defined in the application as follows:
 *  
 *  int CALLBACK PaintBkgd( HWND   hCntWnd,    // container window handle
 *                          LPFIELDINFO lpFld, // field to paint in
 *                          HDC    hDC,        // container window DC
 *                          LPRECT lpRect,     // background rectangle
 *                          UINT   uBkArea )   // background area to paint
 *
 *   uBkArea will be one of the following:
 *
 *        BK_GENERAL   - the call is for painting the general background
 *        BK_UNUSED    - the call is for painting the unused background
 *        BK_TITLE     - the call is for painting the title background
 *        BK_FLDTITLE  - the call is for painting a field title background
 *
 *  Note that lpFld is only valid when uBkArea == BK_FLDTITLE.
 *
 *  Sample usage:   
 *
 *      PAINTPROC lpfnPaintProc;
 *      lpfnPaintProc = (PAINTPROC) MakeProcInstance( PaintBkgd, hInstance );
 *      CntPaintProcSet( lpFI, lpfnPaintProc );
 *
 *  The PaintBkgd name is a placeholder for the application-defined function
 *  name. The actual name *MUST* be exported by including it in an EXPORTS
 *  statement in the application's module-definition (.DEF) file.
 *
 * Return Value:
 *  BOOL          TRUE: success;  FALSE: error
 ****************************************************************************/

BOOL WINAPI CntPaintProcSet( HWND hCntWnd, PAINTPROC lpfnPaintProc )
    {
    LPCONTAINER lpCnt = GETCNT(hCntWnd);

    // Set the function address into the struct.
    lpCnt->lpfnPaintBk = lpfnPaintProc;
    return TRUE;
    }

/****************************************************************************
 * CntRecDataSet
 *
 * Purpose:
 *  Copies record data from a buffer to a record. The number of bytes
 *  copied to lpRecord is equal to the number of bytes allocated for the
 *  record data (1st argument to CntNewRecCore).
 *
 * Parameters:
 *  LPRECORDCORE  lpRec       - pointer to the record core struct
 *  LPVOID        lpData      - record data
 *
 * Return Value:
 *  BOOL          TRUE: success;  FALSE: error
 ****************************************************************************/

BOOL WINAPI CntRecDataSet( LPRECORDCORE lpRec, LPVOID lpData )
   {
   if( !lpRec || !lpData )
      return FALSE;

   if( lpRec->lpRecData )
      _fmemcpy( (LPVOID) lpRec->lpRecData, lpData, (size_t) lpRec->dwRecSize );

   return TRUE;
   }

/****************************************************************************
 * CntRecDataGet
 *
 * Purpose:
 *  Copies record data from a record to a buffer. The number of bytes
 *  copied to lpBuf is equal to the number of bytes allocated for the
 *  record data (1st argument to CntNewRecCore).
 *
 * Parameters:
 *  LPRECORDCORE  lpRec       - pointer to the record core struct
 *  LPVOID        lpBuf       - buffer for record data
 *
 * Return Value:
 *  LPVOID        pointer to record data buffer.
 ****************************************************************************/

LPVOID WINAPI CntRecDataGet( HWND hCntWnd, LPRECORDCORE lpRec, LPVOID lpBuf )
    {
    if( !lpRec || !lpBuf )
      return NULL;

    if( lpRec->lpRecData )
      _fmemcpy( lpBuf, lpRec->lpRecData, (size_t) lpRec->dwRecSize );

    return lpRec->lpRecData;
    }

/****************************************************************************
 * CntRecUserSet
 * CntRecUserGet
 *
 * Purpose:
 *  Loads/Gets the user data for a record.
 *
 * Parameters:
 *  LPRECORDCORE  lpRec       - pointer to the record core struct
 *  LPVOID        lpUserData  - user data
 *  UINT          wUserBytes  - number of bytes to copy from lpUserData
 *
 * Return Value:
 *  BOOL          TRUE: success;  FALSE: error
 ****************************************************************************/

BOOL WINAPI CntRecUserSet( LPRECORDCORE lpRec, LPVOID lpUserData, UINT wUserBytes )
    {
    if( !lpRec || !lpUserData || !wUserBytes )
      return FALSE;

    // Free old user data if there was any.
    if( lpRec->lpUserData )
      {
/*      GlobalFreePtr(lpRec->lpUserData);*/
      free(lpRec->lpUserData);
      lpRec->lpUserData = NULL;
      }

    // Alloc space for the new user data.
/*    lpRec->lpUserData = (LPVOID) GlobalAllocPtr(GHND, wUserBytes);*/
    lpRec->lpUserData = (LPVOID) calloc(1, wUserBytes);
    lpRec->wUserBytes = lpRec->lpUserData ? wUserBytes : 0;

    if( lpRec->lpUserData )
      {
      _fmemcpy( (LPVOID) lpRec->lpUserData, lpUserData, (size_t) lpRec->wUserBytes);
      return TRUE;
      }
    else
      return FALSE;
    }

LPVOID WINAPI CntRecUserGet( LPRECORDCORE lpRec )
    {
    return lpRec->lpUserData;       // user defined record data
    }

/****************************************************************************
 * CntFldDataSet
 * CntFldDataGet
 *
 * Purpose:
 *  Sets/Gets the data in/from a field.
 *
 * Parameters:
 *  LPRECORDCORE  lpRec       - pointer to the record
 *  LPFIELDINFO   lpFld       - pointer to field
 *  UINT          wBufSize    - number of bytes to set/get in/from lpBuf 
 *                              (if wBufSize <= 0 wDataBytes is used)
 *  LPVOID        lpBuf       - field data
 *
 * Return Value:
 *  BOOL          TRUE: success;  FALSE: error
 ****************************************************************************/

BOOL WINAPI CntFldDataSet( LPRECORDCORE lpRec, LPFIELDINFO lpFld, UINT wBufSize, LPVOID lpBuf )
    {
    LPSTR lpC;

    if( !lpRec || !lpFld || !lpBuf )
      return FALSE;

    lpC = (LPSTR) lpRec->lpRecData;
    lpC += lpFld->wOffStruct;

    if( wBufSize <= 0 || wBufSize > lpFld->wDataBytes )
      wBufSize = lpFld->wDataBytes;

    _fmemcpy( (LPVOID) lpC, lpBuf, (size_t) wBufSize );

    return TRUE;
    }

BOOL WINAPI CntFldDataGet( LPRECORDCORE lpRec, LPFIELDINFO lpFld, UINT wBufSize, LPVOID lpBuf )
    {
    LPSTR lpC;

    if( !lpRec || !lpFld || !lpBuf )
      return FALSE;

    lpC = (LPSTR) lpRec->lpRecData;
    lpC += lpFld->wOffStruct;

    if( wBufSize <= 0 || wBufSize > lpFld->wDataBytes )
      wBufSize = lpFld->wDataBytes;

    _fmemcpy( lpBuf, (LPVOID) lpC, (size_t) wBufSize );

    return TRUE;
    }


/****************************************************************************
 * linked list stuff
 ****************************************************************************/

/****************************************************************************
 * CntNewFldInfo
 *
 * Purpose:
 *  Allocates storage for the field node structure type.
 *
 * Parameters:
 *  none
 *
 * Return Value:
 *  LPFIELDINFO   pointer to allocated storage
 ****************************************************************************/

LPFIELDINFO WINAPI CntNewFldInfo( void )
   {
/*   LPFIELDINFO lpFI = (LPFIELDINFO) GlobalAllocPtr(GHND, LEN_FIELDINFO);*/
   LPFIELDINFO lpFI = (LPFIELDINFO) calloc(1, LEN_FIELDINFO);
   if( lpFI )
     {
     lpFI->dwFieldInfoLen = LEN_FIELDINFO;

     // Set the default field width.
     lpFI->cxWidth = 16;
     }
   return lpFI;
   }

/****************************************************************************
 * CntNewRecCore
 *
 * Purpose:
 *  Allocates storage for record node structure type and record data.
 *
 * Parameters:
 *  DWORD         dwRecDataSize - length of buffer to allocate for record data
 *
 * Return Value:
 *  LPRECORDCORE  pointer to allocated storage
 ****************************************************************************/

LPRECORDCORE WINAPI CntNewRecCore( DWORD dwRecDataSize )
   {
/*   LPRECORDCORE lpRC = (LPRECORDCORE) GlobalAllocPtr(GHND, LEN_RECORDCORE);*/
   LPRECORDCORE lpRC = (LPRECORDCORE) calloc(1, LEN_RECORDCORE);

   // Set the limit to the max UINT because we are using calloc and _fmemcpy.
   dwRecDataSize = min(dwRecDataSize, UINT_MAX);
     
   if( lpRC )
      {
/*      lpRC->lpRecData  = (LPVOID) GlobalAllocPtr(GHND, dwRecDataSize);*/
      lpRC->lpRecData  = (LPVOID) calloc(1, (size_t)dwRecDataSize);
      lpRC->dwRecSize  = lpRC->lpRecData ? dwRecDataSize : 0L;
      }
   return lpRC;
   }

/****************************************************************************
 * CntFreeFldInfo
 * CntFreeRecCore
 *
 * Purpose:
 *  Frees the storage for the appropriate node structure type.
 *
 * Parameters:
 *  LPFIELDINFO/LPRECORDCORE   pointer to allocated storage
 *
 * Return Value:
 *  BOOL          TRUE if storage was released
 ****************************************************************************/

BOOL WINAPI CntFreeFldInfo(LPFIELDINFO lpField)
   {
   if( lpField )
     {
     if( lpField->lpFTitle )
/*       GlobalFreePtr(lpField->lpFTitle);*/
       free(lpField->lpFTitle);
     if( lpField->lpUserData )
/*       GlobalFreePtr(lpField->lpUserData);*/
       free(lpField->lpUserData);
     if( lpField->lpDescriptor )
/*       GlobalFreePtr(lpField->lpDescriptor);*/
       free(lpField->lpDescriptor);

/*     return GlobalFreePtr(lpField);*/
     free(lpField);
     return TRUE;
     }
   else
     {
     return FALSE; 
     }
   }

BOOL WINAPI CntFreeRecCore(LPRECORDCORE lpRecord)
   {
   if( lpRecord )
     {
     if( lpRecord->lpRecData )
/*       GlobalFreePtr(lpRecord->lpRecData);*/
       free(lpRecord->lpRecData);
     if( lpRecord->lpUserData )
/*       GlobalFreePtr(lpRecord->lpUserData);*/
       free(lpRecord->lpUserData);

     // free text strings
     if( lpRecord->lpszText )
       free(lpRecord->lpszText);
     if( lpRecord->lpszName )
       free(lpRecord->lpszName);
     if( lpRecord->lpszIcon )
       free(lpRecord->lpszIcon);

     // free icons and bitmaps 
//     if (lpRecord->hIcon)
//       DestroyIcon(lpRecord->hIcon);
//     if (lpRecord->hMiniIcon)
//       DestroyIcon(lpRecord->hMiniIcon);
//     if (lpRecord->hBmp)
//       DeleteObject(lpRecord->hBmp);
//     if (lpRecord->hMiniBmp)
//       DeleteObject(lpRecord->hMiniBmp);
     if (lpRecord->lpTransp)
       free(lpRecord->lpTransp);

     if( lpRecord->lpSelFldHead )
       CntKillSelFldLst( lpRecord );

/*     return GlobalFreePtr(lpRecord);*/
     free(lpRecord);
     return TRUE;
     }
   else
     {
     return FALSE; 
     }
   }

/****************************************************************************
 * CntFldHeadGet
 * CntRecHeadGet
 *
 * Purpose:
 *  Retrieves the corresponding head pointer associated with the container.
 *
 * Parameters:
 *  HWND          hCntWnd     - container window handle 
 *
 * Return Value:
 *  LPFIELDINFO/LPRECORDCORE  pointer to head node of the list
 ****************************************************************************/

LPFIELDINFO WINAPI CntFldHeadGet(HWND hCntWnd)
   {
   LPCNTINFO lpCI = (GETCNT(hCntWnd))->lpCI;
   return lpCI->lpFieldHead;
   }

LPRECORDCORE WINAPI CntRecHeadGet(HWND hCntWnd)
   {
   LPCNTINFO lpCI = (GETCNT(hCntWnd))->lpCI;
   return lpCI->lpRecHead;
   }

/****************************************************************************
 * CntFldTailGet
 * CntRecTailGet
 *
 * Purpose:
 *  Retrieves the corresponding tail pointer associated with the container.
 *
 * Parameters:
 *  HWND          hCntWnd     - container window handle 
 *
 * Return Value:
 *  LPFIELDINFO/LPRECORDCORE  pointer to tail node of the list
 ****************************************************************************/

LPFIELDINFO WINAPI CntFldTailGet(HWND hCntWnd)
   {
   LPCNTINFO lpCI = (GETCNT(hCntWnd))->lpCI;
   return lpCI->lpFieldTail;
   }

LPRECORDCORE WINAPI CntRecTailGet(HWND hCntWnd)
   {
   LPCNTINFO lpCI = (GETCNT(hCntWnd))->lpCI;
   return lpCI->lpRecTail;
   }

/****************************************************************************
 * CntAddFldTail
 * CntAddRecTail
 *
 * Purpose:
 *  Adds a new record to the end of the current linked list.
 *
 * Parameters:
 *  HWND          hCntWnd     - container window handle 
 *  LPFIELDINFO   lpNew       - pointer to field node
 *  LPRECORDCORE  lpNew       - pointer to record node
 *
 * Return Value:
 *  BOOL          TRUE if tail successfully added
 ****************************************************************************/

BOOL WINAPI CntAddFldTail(HWND hCntWnd, LPFIELDINFO lpNew)
   {
   LPCONTAINER lpCnt = GETCNT(hCntWnd);
   LPCNTINFO  lpCI = (GETCNT(hCntWnd))->lpCI;

   if (!lpCI)
      return FALSE;

   lpCI->lpFieldTail = (LPFIELDINFO) LLAddTail( (LPLINKNODE) lpCI->lpFieldTail, 
                                                           (LPLINKNODE) lpNew);
   // Update head pointer, if necessary
   if (!lpCI->lpFieldHead)
      lpCI->lpFieldHead = lpCI->lpFieldTail;

   // Recalc the max horz scrolling width.
   InitTextMetrics( lpCnt->hWnd1stChild, lpCnt );
/*   InitTextMetrics( hCntWnd, GETCNT(hCntWnd) );*/

   return lpCI->lpFieldTail ? TRUE : FALSE;
   }

BOOL WINAPI CntAddRecTail(HWND hCntWnd, LPRECORDCORE lpNew)
   {
   LPCONTAINER lpCnt = GETCNT(hCntWnd);
   LPCNTINFO   lpCI = (GETCNT(hCntWnd))->lpCI;
   LPZORDERREC lpOrderRec;

   if (!lpCI || !lpNew)
      return FALSE;

   lpCI->lpRecTail = (LPRECORDCORE) LLAddTail( (LPLINKNODE) lpCI->lpRecTail, 
                                               (LPLINKNODE) lpNew);
   // Update head pointer, if necessary
   if (!lpCI->lpRecHead)
      lpCI->lpRecHead = lpCI->lpRecTail;

   // add record to record order list (used in icon view to determine the z-order 
   // of the icons).  Only add record when in Icon view or if the list had been
   // previously created by user being in icon view and then switching to
   // another view.
   if ((lpCnt->iView & CV_ICON) || lpCI->lpRecOrderHead)
   {
     BOOL bChanged;

     lpOrderRec = CntNewOrderRec();
     if (lpOrderRec)
     { 
       lpOrderRec->lpRec = lpNew;
       CntAddRecOrderTail( hCntWnd, lpOrderRec );
     }

     // Adjust the icon workspace (if necessary)
     bChanged = AdjustIconWorkspace( hCntWnd, lpCnt, lpNew );

     // adjust the horizontal scroll bar if workspace changed
     if ( lpCnt->dwStyle & CTS_HORZSCROLL && bChanged )
       IcnAdjustHorzScroll( lpCnt->hWnd1stChild, lpCnt );
   }
   
   lpCI->dwRecordNbr++;

   // if text or name view, update the FLOWCOLINFO structure
   if ((lpCnt->iView & CV_TEXT) || (lpCnt->iView & CV_NAME))
     CreateFlowColStruct(hCntWnd, lpCnt);
   
   // Adjust the vertical scrollbar.
   AdjustVertScroll( lpCnt->hWnd1stChild, lpCnt, FALSE );

   // if flowed, adjust the horizontal scroll bar
   if ((lpCnt->iView & CV_FLOW) && (lpCnt->dwStyle & CTS_HORZSCROLL) )
   {
      if( lpCI->nMaxWidth - lpCnt->cxClient > 0 )
        lpCnt->nHscrollMax = max(0, (lpCI->nMaxWidth - lpCnt->cxClient - 1) / lpCI->cxChar + 1);
      else
        lpCnt->nHscrollMax = 0;
      lpCnt->nHscrollPos = min(lpCnt->nHscrollPos, lpCnt->nHscrollMax);
  
      SetScrollPos  ( lpCnt->hWnd1stChild, SB_HORZ,    lpCnt->nHscrollPos, FALSE );
      SetScrollRange( lpCnt->hWnd1stChild, SB_HORZ, 0, lpCnt->nHscrollMax, TRUE  );

      // There is nothing to the right of this value.
      lpCnt->xLastCol = lpCI->nMaxWidth - lpCnt->nHscrollPos * lpCI->cxChar;
   }

   // Repaint the new tail record.
   if( lpCI->lpRecTail && !lpCnt->bDoingQueryDelta )
     InvalidateCntRecord( lpCnt->hWnd1stChild, lpCnt, lpCI->lpRecTail, NULL, 2 );

   return lpCI->lpRecTail ? TRUE : FALSE;
   }

/****************************************************************************
 * CntAddFldHead
 * CntAddRecHead
 *
 * Purpose:
 *  Adds a new record to the beginning of the current linked list.
 *
 * Parameters:
 *  HWND          hCntWnd     - container window handle 
 *  LPFIELDINFO   lpNew       - pointer to field node
 *  LPRECORDCORE  lpNew       - pointer to record node
 *
 * Return Value:
 *  BOOL          TRUE if head successfully added
 ****************************************************************************/

BOOL WINAPI CntAddFldHead(HWND hCntWnd, LPFIELDINFO lpNew)
   {
   LPCONTAINER lpCnt = GETCNT(hCntWnd);
   LPCNTINFO  lpCI = (GETCNT(hCntWnd))->lpCI;

   if (!lpCI)
      return FALSE;

   lpCI->lpFieldHead = (LPFIELDINFO) LLAddHead( (LPLINKNODE) lpCI->lpFieldHead, 
                                                           (LPLINKNODE) lpNew);
   // Update tail pointer, if necessary
   if (!lpCI->lpFieldTail)
      lpCI->lpFieldTail = lpCI->lpFieldHead;

   // Recalc the max horz scrolling width.
   InitTextMetrics( lpCnt->hWnd1stChild, lpCnt );
/*   InitTextMetrics( hCntWnd, GETCNT(hCntWnd) );*/

   return lpCI->lpFieldHead ? TRUE : FALSE;
   }

BOOL WINAPI CntAddRecHead(HWND hCntWnd, LPRECORDCORE lpNew)
   {
   LPCONTAINER lpCnt = GETCNT(hCntWnd);
   LPCNTINFO   lpCI = (GETCNT(hCntWnd))->lpCI;
   LPZORDERREC lpOrderRec;

   if (!lpCI || !lpNew)
      return FALSE;

   lpCI->lpRecHead = (LPRECORDCORE) LLAddHead( (LPLINKNODE) lpCI->lpRecHead, 
                                                           (LPLINKNODE) lpNew);
   // Update tail pointer, if necessary
   if (!lpCI->lpRecTail)
      lpCI->lpRecTail = lpCI->lpRecHead;

   // add record to record order list (used in icon view to determine the z-order 
   // of the icons).  Only add record when in Icon view or if the list had been
   // previously created by user being in icon view and then switching to
   // another view.
   if ((lpCnt->iView & CV_ICON) || lpCI->lpRecOrderHead)
   {
     BOOL bChanged;

     lpOrderRec = CntNewOrderRec();
     if (lpOrderRec)
     { 
       lpOrderRec->lpRec = lpNew;
       CntAddRecOrderHead( hCntWnd, lpOrderRec );
     }

     // Adjust the icon workspace (if necessary)
     bChanged = AdjustIconWorkspace( hCntWnd, lpCnt, lpNew );

     // adjust the horizontal scroll bar if workspace changed
     if ( lpCnt->dwStyle & CTS_HORZSCROLL && bChanged )
       IcnAdjustHorzScroll( lpCnt->hWnd1stChild, lpCnt );
   }
   
   lpCI->dwRecordNbr++;   // bump total record count

   // if text or name view, update the FLOWCOLINFO structure
   if ((lpCnt->iView & CV_TEXT) || (lpCnt->iView & CV_NAME))
     CreateFlowColStruct(hCntWnd, lpCnt);
   
   // Adjust the vertical scrollbar.
   AdjustVertScroll( lpCnt->hWnd1stChild, lpCnt, FALSE );
/*   AdjustVertScroll( hCntWnd, GETCNT(hCntWnd) );*/

   // Repaint if it is the new tail record and we are not
   // in the middle of a query delta.
   if( lpCI->lpRecHead == lpCI->lpRecTail && !lpCnt->bDoingQueryDelta )
     InvalidateCntRecord( lpCnt->hWnd1stChild, lpCnt, lpCI->lpRecTail, NULL, 2 );

   return lpCI->lpRecHead ? TRUE : FALSE;
   }

/****************************************************************************
 * CntRemoveFldHead
 * CntRemoveRecHead
 *
 * Purpose:
 *  Removes the item at the head of the current linked list.
 *  Does not free any storage.
 *
 * Parameters:
 *  HWND          hCntWnd     - container window handle 
 *
 * Return Value:
 *  LPFIELDINFO/LPRECORDCORE  pointer to node that was removed
 ****************************************************************************/

LPFIELDINFO WINAPI CntRemoveFldHead(HWND hCntWnd)
   {
   LPCONTAINER lpCnt = GETCNT(hCntWnd);
   LPCNTINFO  lpCI = (GETCNT(hCntWnd))->lpCI;
   LPFIELDINFO lpTemp;

   if (!lpCI)
      return (LPFIELDINFO) NULL;

   lpCI->lpFieldHead = (LPFIELDINFO) LLRemoveHead((LPLINKNODE) lpCI->lpFieldHead, 
                                                      ((LPLINKNODE FAR *)&lpTemp));
   // Update tail pointer, if necessary
   if (!lpCI->lpFieldHead)
      lpCI->lpFieldTail = lpCI->lpFieldHead;

   // Recalc the max horz scrolling width.
   InitTextMetrics( lpCnt->hWnd1stChild, lpCnt );
/*   InitTextMetrics( hCntWnd, GETCNT(hCntWnd) );*/

   // Move the focus if we are removing the focus field.
   if( lpTemp == lpCI->lpFocusFld )
     {
     // Find a field to move the focus to.
     if( lpTemp->lpNext == NULL )
       lpCI->lpFocusFld = lpTemp->lpPrev;
     else
       lpCI->lpFocusFld = lpTemp->lpNext;

     if( lpCI->lpFocusFld )
       lpCI->lpFocusFld->flColAttr |= CFA_CURSORED;

     // Clear the focus attribute in case this field is reused.
     lpTemp->flColAttr &= ~CFA_CURSORED;
     }

   return lpTemp;
   }

LPRECORDCORE WINAPI CntRemoveRecHead(HWND hCntWnd)
   {
   LPCONTAINER lpCnt = GETCNT(hCntWnd);
   LPCNTINFO  lpCI = (GETCNT(hCntWnd))->lpCI;
   LPRECORDCORE lpTemp;
   LPZORDERREC  lpOrderRec;

   if (!lpCI)
      return (LPRECORDCORE) NULL;

   lpCI->lpRecHead = (LPRECORDCORE) LLRemoveHead((LPLINKNODE) lpCI->lpRecHead, 
                                                      ((LPLINKNODE FAR *)&lpTemp));
   // Update tail pointer, if necessary
   if (!lpCI->lpRecHead)
      lpCI->lpRecTail = lpCI->lpRecHead;

   UpdateCIptrs( lpCnt->hWnd1stChild, lpTemp );
/*   UpdateCIptrs( hCntWnd, lpTemp );*/

   // remove node from the record order list
   if ((lpCnt->iView & CV_ICON) || lpCI->lpRecOrderHead)
   {
     BOOL bChanged;

     lpOrderRec = CntFindOrderRec( hCntWnd, lpCI->lpRecHead );
     if (lpOrderRec)
     { 
       lpOrderRec = CntRemoveOrderRec( hCntWnd, lpOrderRec );
       CntFreeOrderRec( lpOrderRec );
     }

     // recalculate the icon workspace (if necessary)
     bChanged = CalculateIconWorkspace( hCntWnd );

     // adjust the horizontal scroll bar if workspace changed
     if ( lpCnt->dwStyle & CTS_HORZSCROLL && bChanged )
       IcnAdjustHorzScroll( lpCnt->hWnd1stChild, lpCnt );
   }
   
   if (lpTemp)
     {  
     lpCI->dwRecordNbr--;   // decrement total record count

     // Adjust the vertical scrollbar.
     AdjustVertScroll( lpCnt->hWnd1stChild, lpCnt, FALSE );
/*     AdjustVertScroll( hCntWnd, GETCNT(hCntWnd) );*/
     }

   return lpTemp;
   }

/****************************************************************************
 * CntRemoveFldTail
 * CntRemoveRecTail
 *
 * Purpose:
 *  Removes the item at the tail of the current linked list.
 *  Does not free any storage.
 *
 * Parameters:
 *  HWND          hCntWnd     - container window handle 
 *
 * Return Value:
 *  LPFIELDINFO/LPRECORDCORE  pointer to node that was removed
 ****************************************************************************/

LPFIELDINFO WINAPI CntRemoveFldTail(HWND hCntWnd)
   {
   LPCONTAINER lpCnt = GETCNT(hCntWnd);
   LPCNTINFO  lpCI = (GETCNT(hCntWnd))->lpCI;
   LPFIELDINFO lpTemp;

   if (!lpCI)
      return (LPFIELDINFO) NULL;

   lpCI->lpFieldTail = (LPFIELDINFO) LLRemoveTail((LPLINKNODE) lpCI->lpFieldTail, 
                                                      ((LPLINKNODE FAR *)&lpTemp));
   // Update head pointer, if necessary
   if (!lpCI->lpFieldTail)
      lpCI->lpFieldHead = lpCI->lpFieldTail;

   // Recalc the max horz scrolling width.
   InitTextMetrics( lpCnt->hWnd1stChild, lpCnt );
/*   InitTextMetrics( hCntWnd, GETCNT(hCntWnd) );*/

   // Move the focus if we are removing the focus field.
   if( lpTemp == lpCI->lpFocusFld )
     {
     // Find a field to move the focus to.
     if( lpTemp->lpNext == NULL )
       lpCI->lpFocusFld = lpTemp->lpPrev;
     else
       lpCI->lpFocusFld = lpTemp->lpNext;

     if( lpCI->lpFocusFld )
       lpCI->lpFocusFld->flColAttr |= CFA_CURSORED;

     // Clear the focus attribute in case this field is reused.
     lpTemp->flColAttr &= ~CFA_CURSORED;
     }

   return lpTemp;
   }

LPRECORDCORE WINAPI CntRemoveRecTail(HWND hCntWnd)
   {
   LPCONTAINER  lpCnt = GETCNT(hCntWnd);
   LPCNTINFO    lpCI = (GETCNT(hCntWnd))->lpCI;
   LPRECORDCORE lpTemp;
   LPZORDERREC  lpOrderRec;

   if (!lpCI)
      return (LPRECORDCORE) NULL;

   lpCI->lpRecTail = (LPRECORDCORE) LLRemoveTail((LPLINKNODE) lpCI->lpRecTail, 
                                                      ((LPLINKNODE FAR *)&lpTemp));

   // Update head pointer, if necessary
   if (!lpCI->lpRecTail)
      lpCI->lpRecHead = lpCI->lpRecTail;

   UpdateCIptrs( lpCnt->hWnd1stChild, lpTemp );
/*   UpdateCIptrs(hCntWnd, lpTemp);*/

   // remove node from the record order list
   if ((lpCnt->iView & CV_ICON) || lpCI->lpRecOrderHead)
   {
     BOOL bChanged;

     lpOrderRec = CntFindOrderRec( hCntWnd, lpCI->lpRecTail );
     if (lpOrderRec)
     { 
       lpOrderRec = CntRemoveOrderRec( hCntWnd, lpOrderRec );
       CntFreeOrderRec( lpOrderRec );
     }

     // recalculate the icon workspace (if necessary)
     bChanged = CalculateIconWorkspace( hCntWnd );

     // adjust the horizontal scroll bar if workspace changed
     if ( lpCnt->dwStyle & CTS_HORZSCROLL && bChanged )
       IcnAdjustHorzScroll( lpCnt->hWnd1stChild, lpCnt );
   }
   
   if (lpTemp)
     {
     lpCI->dwRecordNbr--;   // decrement total record count

     // Adjust the vertical scrollbar.
     AdjustVertScroll( lpCnt->hWnd1stChild, lpCnt, FALSE );
/*     AdjustVertScroll( hCntWnd, GETCNT(hCntWnd) );*/
     }

   return lpTemp;
   }

/****************************************************************************
 * CntRemoveFld
 * CntRemoveRec
 *
 * Purpose:
 *  Removes the given node from the current linked list.
 *  Does not free any storage.
 *
 * Parameters:
 *  HWND          hCntWnd     - container window handle 
 *  LPFIELDINFO   lpNew       - pointer to field node
 *  LPRECORDCORE  lpNew       - pointer to record node
 *
 * Return Value:
 *  LPFIELDINFO/LPRECORDCORE  pointer to node that was removed
 ****************************************************************************/

LPFIELDINFO WINAPI CntRemoveFld(HWND hCntWnd, LPFIELDINFO lpField)
   {
   LPCONTAINER lpCnt = GETCNT(hCntWnd);
   LPCNTINFO  lpCI = (GETCNT(hCntWnd))->lpCI;
   LPFIELDINFO lpTemp;

   if (!lpCI)
      return (LPFIELDINFO) NULL;

   // Need to check for three cases here:
   //      Removing a head node- call LLRemoveHead
   //      Removing a tail node- call LLRemoveTail
   //      Removing an internal node - call LLRemoveNode

   if (lpField == lpCI->lpFieldHead)   // Current node is head
      {
      lpCI->lpFieldHead = (LPFIELDINFO) 
         LLRemoveHead( (LPLINKNODE) lpCI->lpFieldHead, ((LPLINKNODE FAR *)&lpTemp));
      if (!lpCI->lpFieldHead)
         lpCI->lpFieldTail = lpCI->lpFieldHead;
      }
   else if (lpField == lpCI->lpFieldTail) // Current node is tail
      {
      lpCI->lpFieldTail = (LPFIELDINFO) 
         LLRemoveTail( (LPLINKNODE) lpCI->lpFieldTail, ((LPLINKNODE FAR *)&lpTemp));
      if (!lpCI->lpFieldTail)
         lpCI->lpFieldHead = lpCI->lpFieldTail;
      }
   else  // current node is internal
      {
      lpTemp = (LPFIELDINFO) LLRemoveNode( (LPLINKNODE) lpField);
      }

   // Recalc the max horz scrolling width.
   InitTextMetrics( lpCnt->hWnd1stChild, lpCnt );
/*   InitTextMetrics( hCntWnd, GETCNT(hCntWnd) );*/

    // Move the focus if we are removing the focus field.
    if( lpTemp == lpCI->lpFocusFld )
      {
      // Find a field to move the focus to.
      if( lpTemp->lpNext == NULL )
        lpCI->lpFocusFld = lpTemp->lpPrev;
      else
        lpCI->lpFocusFld = lpTemp->lpNext;

      if( lpCI->lpFocusFld )
        lpCI->lpFocusFld->flColAttr |= CFA_CURSORED;

      // Clear the focus attribute in case this field is reused.
      lpTemp->flColAttr &= ~CFA_CURSORED;
      }

   return lpTemp;
   }

LPRECORDCORE WINAPI CntRemoveRec(HWND hCntWnd, LPRECORDCORE lpRecord)
   {
   LPCONTAINER  lpCnt = GETCNT(hCntWnd);
   LPCNTINFO    lpCI = (GETCNT(hCntWnd))->lpCI;
   LPRECORDCORE lpTemp;
   LPZORDERREC  lpOrderRec;

   if (!lpCI || !lpRecord)
      return (LPRECORDCORE) NULL;

   // Need to check for three cases here:
   //      Removing a head node- call LLRemoveHead
   //      Removing a tail node- call LLRemoveTail
   //      Removing an internal node - call LLRemoveNode

   if (lpRecord == lpCI->lpRecHead) // Current node is head
      {
      lpCI->lpRecHead = (LPRECORDCORE) 
         LLRemoveHead( (LPLINKNODE) lpCI->lpRecHead, ((LPLINKNODE FAR *)&lpTemp));
      if (!lpCI->lpRecHead)
         lpCI->lpRecTail = lpCI->lpRecHead;
      }
   else if (lpRecord == lpCI->lpRecTail)  // Current node is tail
      {
      lpCI->lpRecTail = (LPRECORDCORE) 
         LLRemoveTail( (LPLINKNODE) lpCI->lpRecTail, ((LPLINKNODE FAR *)&lpTemp));
      if (!lpCI->lpRecTail)
         lpCI->lpRecHead = lpCI->lpRecTail;
      }
   else  // Current node is internal
      {
      lpTemp = (LPRECORDCORE) LLRemoveNode( (LPLINKNODE) lpRecord);
      }

   UpdateCIptrs( lpCnt->hWnd1stChild, lpTemp );
/*   UpdateCIptrs( hCntWnd, lpTemp );*/

   // remove node from the record order list
   if ((lpCnt->iView & CV_ICON) || lpCI->lpRecOrderHead)
   {
     BOOL bChanged;

     lpOrderRec = CntFindOrderRec( hCntWnd, lpRecord );
     if (lpOrderRec)
     { 
       lpOrderRec = CntRemoveOrderRec( hCntWnd, lpOrderRec );
       CntFreeOrderRec( lpOrderRec );
     }

     // recalculate the icon workspace (if necessary)
     bChanged = CalculateIconWorkspace( hCntWnd );

     // adjust the horizontal scroll bar if workspace changed
     if ( lpCnt->dwStyle & CTS_HORZSCROLL && bChanged )
       IcnAdjustHorzScroll( lpCnt->hWnd1stChild, lpCnt );
   }
   
   if (lpTemp)
     {
     lpCI->dwRecordNbr--;   // decrement total record count

     // Adjust the vertical scrollbar.
     AdjustVertScroll( lpCnt->hWnd1stChild, lpCnt, FALSE );
/*     AdjustVertScroll( hCntWnd, GETCNT(hCntWnd) );*/
     }

   return lpTemp;
   }

/****************************************************************************
 * CntMoveRecAfter
 *
 * Purpose:
 *  Moves an existing record to the next position after the given record.
 *
 * Parameters:
 *  HWND          hCntWnd     - container window handle 
 *  LPRECORDCORE  lpRecord    - pointer to record to move after.
 *  LPRECORDCORE  lpRecMov    - pointer to record to move.
 *
 * Return Value:
 *  BOOL    TRUE if record was moved successfully; else FALSE
 ****************************************************************************/

BOOL WINAPI CntMoveRecAfter( HWND hCntWnd, LPRECORDCORE lpRecord, LPRECORDCORE lpRecMov )
   {
   LPCONTAINER lpCnt = GETCNT(hCntWnd);
   LPCNTINFO  lpCI = (GETCNT(hCntWnd))->lpCI;
   LPRECORDCORE lpTempMov;

   if( !lpCI || !lpRecord || !lpRecMov )
     return FALSE;

   // The move will be done by first removing lpRecMov from
   // the list and then inserting it after lpRecord.

   if( lpRecMov == lpCI->lpRecHead ) 
     {
     // target node is the head
     lpCI->lpRecHead = (LPRECORDCORE) LLRemoveHead( (LPLINKNODE) lpCI->lpRecHead, ((LPLINKNODE FAR *)&lpTempMov) );
     if( !lpCI->lpRecHead )
       lpCI->lpRecTail = lpCI->lpRecHead;
     }
   else if( lpRecMov == lpCI->lpRecTail )
     {
     // target node is the tail
     lpCI->lpRecTail = (LPRECORDCORE) LLRemoveTail( (LPLINKNODE) lpCI->lpRecTail, ((LPLINKNODE FAR *)&lpTempMov) );
     if( !lpCI->lpRecTail )
       lpCI->lpRecHead = lpCI->lpRecTail;
     }
   else  
     {
     // target node is internal
     lpTempMov = (LPRECORDCORE) LLRemoveNode( (LPLINKNODE) lpRecMov);
     }

   // Swap the top record ptr if either record is the current top record.
   if( lpTempMov == lpCI->lpTopRec )
     lpCI->lpTopRec = lpRecord;
   else if( lpRecord == lpCI->lpTopRec )
     lpCI->lpTopRec = lpTempMov;

   // Now insert the removed record after lpRecord in the list.
   lpCI->lpRecHead = (LPRECORDCORE) LLInsertAfter( (LPLINKNODE) lpRecord, (LPLINKNODE) lpTempMov );

   // Update tail pointer, if necessary
   if( !lpCI->lpRecTail )
     lpCI->lpRecTail = lpCI->lpRecHead;

   // Check for hosed tail
   if( lpCI->lpRecTail->lpNext )
     lpCI->lpRecTail = lpCI->lpRecTail->lpNext;

   return lpCI->lpRecHead ? TRUE : FALSE;
   }

/****************************************************************************
 * CntInsFldAfter
 * CntInsRecAfter
 *
 * Purpose:
 *  Adds a new item to the list, after the given node.
 *
 * Parameters:
 *  hCntWnd    HWND Container window handle 
 *  LPFIELDINFO/LPRECORDCORE  pointer to existing node to insert after.
 *  LPFIELDINFO/LPRECORDCORE  pointer to new node to insert.
 *
 * Return Value:
 *  BOOL    TRUE if node added to list.
 ****************************************************************************/

BOOL WINAPI CntInsFldAfter(HWND hCntWnd, LPFIELDINFO lpField, LPFIELDINFO lpNew)
   {
   LPCONTAINER lpCnt = GETCNT(hCntWnd);
   LPCNTINFO  lpCI = (GETCNT(hCntWnd))->lpCI;

   if (!lpCI || !lpField || !lpNew)
      return FALSE;

   lpCI->lpFieldHead = (LPFIELDINFO) LLInsertAfter( (LPLINKNODE) lpField, 
                                                         (LPLINKNODE) lpNew);
   // Update tail pointer, if necessary
   if (!lpCI->lpFieldTail)
      lpCI->lpFieldTail = lpCI->lpFieldHead;

   // Check for hosed tail
   if (lpCI->lpFieldTail->lpNext)
      lpCI->lpFieldTail = lpCI->lpFieldTail->lpNext;
  
   // Recalc the max horz scrolling width.
   InitTextMetrics( lpCnt->hWnd1stChild, lpCnt );
/*   InitTextMetrics( hCntWnd, GETCNT(hCntWnd) );*/

   return lpCI->lpFieldHead ? TRUE : FALSE;
   }

BOOL WINAPI CntInsRecAfter(HWND hCntWnd, LPRECORDCORE lpRecord, LPRECORDCORE lpNew)
   {
   LPCONTAINER lpCnt = GETCNT(hCntWnd);
   LPCNTINFO   lpCI = (GETCNT(hCntWnd))->lpCI;
   LPZORDERREC lpOrderRec;

   if (!lpCI || !lpRecord || !lpNew)
      return FALSE;

   lpCI->lpRecHead = (LPRECORDCORE) LLInsertAfter( (LPLINKNODE) lpRecord, 
                                                         (LPLINKNODE) lpNew);
   // Update tail pointer, if necessary
   if (!lpCI->lpRecTail)
      lpCI->lpRecTail = lpCI->lpRecHead;

   // Check for hosed tail
   if (lpCI->lpRecTail->lpNext)
      lpCI->lpRecTail = lpCI->lpRecTail->lpNext;

   // add record to record order list (used in icon view to determine the z-order 
   // of the icons).  Only add record when in Icon view or if the list had been
   // previously created by user being in icon view and then switching to
   // another view.
   if ((lpCnt->iView & CV_ICON) || lpCI->lpRecOrderHead)
   {
     BOOL bChanged;

     lpOrderRec = CntNewOrderRec();
     if (lpOrderRec)
     { 
       lpOrderRec->lpRec = lpNew;
       CntAddRecOrderHead( hCntWnd, lpOrderRec );
     }

     // adjust the icon workspace (if necessary)
     bChanged = AdjustIconWorkspace( hCntWnd, lpCnt, lpNew );

     // adjust the horizontal scroll bar if workspace changed
     if ( lpCnt->dwStyle & CTS_HORZSCROLL && bChanged )
       IcnAdjustHorzScroll( lpCnt->hWnd1stChild, lpCnt );
   }
   
   lpCI->dwRecordNbr++;   // bump total record count

   // Adjust the vertical scrollbar.
   AdjustVertScroll( lpCnt->hWnd1stChild, lpCnt, FALSE );
/*   AdjustVertScroll( hCntWnd, GETCNT(hCntWnd) );*/

   // Repaint if it is the new tail record.
   if( lpNew == lpCI->lpRecTail && !lpCnt->bDoingQueryDelta )
     InvalidateCntRecord( lpCnt->hWnd1stChild, lpCnt, lpCI->lpRecTail, NULL, 2 );

   return lpCI->lpRecHead ? TRUE : FALSE;
   }

/****************************************************************************
 * CntInsFldBefore
 * CntInsRecBefore
 *
 * Purpose:
 *  Adds a new item to the list, before the given node.
 *
 * Parameters:
 *  hCntWnd    HWND Container window handle 
 *  LPFIELDINFO/LPRECORDCORE  pointer to existing node to insert before.
 *  LPFIELDINFO/LPRECORDCORE  pointer to new node to insert.
 *
 * Return Value:
 *  BOOL    TRUE if node added to list.
 ****************************************************************************/

BOOL WINAPI CntInsFldBefore(HWND hCntWnd, LPFIELDINFO lpField, LPFIELDINFO lpNew)
   {
   LPCONTAINER lpCnt = GETCNT(hCntWnd);
   LPCNTINFO  lpCI = (GETCNT(hCntWnd))->lpCI;

   if (!lpCI || !lpField || !lpNew)
      return FALSE;

   lpCI->lpFieldHead = (LPFIELDINFO) LLInsertBefore( (LPLINKNODE) lpField, 
                                                         (LPLINKNODE) lpNew);
   // Update tail pointer, if necessary
   if (!lpCI->lpFieldTail)
      lpCI->lpFieldTail = lpCI->lpFieldHead;

   // Recalc the max horz scrolling width.
   InitTextMetrics( lpCnt->hWnd1stChild, lpCnt );
/*   InitTextMetrics( hCntWnd, GETCNT(hCntWnd) );*/

   return lpCI->lpFieldHead ? TRUE : FALSE;
   }

BOOL WINAPI CntInsRecBefore(HWND hCntWnd, LPRECORDCORE lpRecord, LPRECORDCORE lpNew)
   {
   LPCONTAINER lpCnt = GETCNT(hCntWnd);
   LPCNTINFO   lpCI = (GETCNT(hCntWnd))->lpCI;
   LPZORDERREC lpOrderRec;

   if (!lpCI || !lpRecord || !lpNew)
      return FALSE;

   lpCI->lpRecHead = (LPRECORDCORE) LLInsertBefore( (LPLINKNODE) lpRecord, 
                                                         (LPLINKNODE) lpNew);
   // Update tail pointer, if necessary
   if (!lpCI->lpRecTail)
      lpCI->lpRecTail = lpCI->lpRecHead;

   // add record to record order list (used in icon view to determine the z-order 
   // of the icons).  Only add record when in Icon view or if the list had been
   // previously created by user being in icon view and then switching to
   // another view.
   if ((lpCnt->iView & CV_ICON) || lpCI->lpRecOrderHead)
   {
     BOOL bChanged;

     lpOrderRec = CntNewOrderRec();
     if (lpOrderRec)
     { 
       lpOrderRec->lpRec = lpNew;
       CntAddRecOrderHead( hCntWnd, lpOrderRec );
     }

     // adjust the icon workspace (if necessary)
     bChanged = AdjustIconWorkspace( hCntWnd, lpCnt, lpNew );

     // adjust the horizontal scroll bar if workspace changed
     if ( lpCnt->dwStyle & CTS_HORZSCROLL && bChanged )
       IcnAdjustHorzScroll( lpCnt->hWnd1stChild, lpCnt );
   }

   lpCI->dwRecordNbr++;   // bump total record count

   // Adjust the vertical scrollbar.
   AdjustVertScroll( lpCnt->hWnd1stChild, lpCnt, FALSE );
/*   AdjustVertScroll( hCntWnd, GETCNT(hCntWnd) );*/

   return lpCI->lpRecHead ? TRUE : FALSE;
   }

/****************************************************************************
 * CntKillFldList
 * CntKillRecList
 *
 * Purpose:
 *  Destroys a list, freeing its memory.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *
 * Return Value:
 *  BOOL          TRUE if list was destroyed.
 ****************************************************************************/

BOOL WINAPI CntKillFldList( HWND hCntWnd )
   {
   LPCNTINFO lpCI = (GETCNT(hCntWnd))->lpCI;
   LPFIELDINFO lpTemp;

   if (!lpCI)
      return FALSE;
   
   while (lpCI->lpFieldHead)
      {
      lpCI->lpFieldHead = (LPFIELDINFO) 
         LLRemoveHead( (LPLINKNODE) lpCI->lpFieldHead, ((LPLINKNODE FAR *)&lpTemp));
      if (lpTemp)
         CntFreeFldInfo(lpTemp);
      }

   lpCI->lpFieldTail = NULL;
   lpCI->lpFocusFld = NULL;
   lpCI->lpCurCNFld = NULL;   

   lpCI->bFocusFldLocked = 0;  // zero out the focus field lock count

   lpCI->nFieldNbr = 0;

   return TRUE;
   }

BOOL WINAPI CntKillRecList(HWND hCntWnd)
   {
   LPCONTAINER lpCnt = GETCNT(hCntWnd);
   LPCNTINFO  lpCI = (GETCNT(hCntWnd))->lpCI;
   LPRECORDCORE lpTemp;

   if (!lpCI)
      return FALSE;
   
   while (lpCI->lpRecHead)
      {
      lpCI->lpRecHead = (LPRECORDCORE) 
         LLRemoveHead( (LPLINKNODE) lpCI->lpRecHead, ((LPLINKNODE FAR *)&lpTemp));
      if (lpTemp)
         CntFreeRecCore(lpTemp);
      }

   lpCI->lpRecTail = NULL;
   lpCI->lpFocusRec = NULL;
   lpCI->lpSelRec = NULL;
   lpCI->lpTopRec = NULL;
   lpCI->lpCurCNRec = NULL;   

   lpCI->bFocusRecLocked = 0;  // zero out the focus record lock count

   // Kill the record order list
   if (lpCI->lpRecOrderHead)
   {
     CntKillRecOrderLst( hCntWnd );
   }

   // set workspace to 0 in icon view
   if (lpCnt->iView & CV_ICON)
     SetRectEmpty( &lpCI->rcWorkSpace );

//   lpCI->nRecsDisplayed = 0;
   lpCI->dwRecordNbr = 0;   // zero out record count

   // Adjust the vertical scrollbar.
   AdjustVertScroll( lpCnt->hWnd1stChild, lpCnt, FALSE );
/*   AdjustVertScroll( hCntWnd, GETCNT(hCntWnd) );*/

   return TRUE;
   }

/****************************************************************************
 * CntNextFld
 * CntNextRec
 * CntPrevFld
 * CntPrevRec
 *
 * Purpose:
 *  Gets the next/prev item in the appropriate list.
 *
 * Parameters:
 *  LPFIELDINFO/LPRECORDCORE  pointer to node to start from.
 *
 * Return Value:
 *  LPFIELDINFO/LPRECORDCORE  pointer to next/prev node.
 ****************************************************************************/

LPFIELDINFO WINAPI CntNextFld( LPFIELDINFO lpField )
   {
   if (lpField)
      return (LPFIELDINFO) NEXT(lpField);
   else
      return NULL;
   }

LPRECORDCORE WINAPI CntNextRec( LPRECORDCORE lpRecord )
   {
   if (lpRecord)
      return (LPRECORDCORE) NEXT(lpRecord);
   else
      return NULL;
   }

LPFIELDINFO WINAPI CntPrevFld( LPFIELDINFO lpField )
   {
   if (lpField)
      return (LPFIELDINFO) PREV(lpField);
   else
      return NULL;
   }

LPRECORDCORE WINAPI CntPrevRec( LPRECORDCORE lpRecord )
   {
   if (lpRecord)
      return (LPRECORDCORE) PREV(lpRecord);
   else
      return NULL;
   }

/****************************************************************************
 * CntFldByIndexGet
 *
 * Purpose:
 *  Gets a field node based on the given index number.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  UINT          wIndex      - Index value
 *
 * Return Value:
 *  LPFIELDINFO    pointer to field node.
 ****************************************************************************/

LPFIELDINFO WINAPI CntFldByIndexGet( HWND hCntWnd, UINT wIndex )
    {
    LPCNTINFO lpCI = (GETCNT(hCntWnd))->lpCI;
    LPFIELDINFO lpFld = NULL;

    if (lpCI)
      {
      lpFld = lpCI->lpFieldHead;
      while (lpFld)
        {
        if (lpFld->wIndex == wIndex)
          break;
        else
          lpFld = (LPFIELDINFO) NEXT(lpFld);
        }
      }

    return lpFld;
    }

/****************************************************************************
 * CntScrollFldList
 * CntScrollRecList
 *
 * Purpose:
 *  Scrolls a list in either direction by the number of nodes requested.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  LPFIELDINFO   lpField     - pointer to field node to start scroll from
 *  LPRECORDCORE  lpRecord    - pointer to record node to start scroll from
 *
 * Return Value:
 *  LPFIELDINFO/LPRECORDCORE  pointer to node to scrolled to.
 ****************************************************************************/

LPFIELDINFO WINAPI CntScrollFldList(HWND hCntWnd, LPFIELDINFO lpField, LONG lDelta)
   {
   return (LPFIELDINFO) LLScrollList( (LPLINKNODE) lpField, lDelta);
   }

LPRECORDCORE WINAPI CntScrollRecList(HWND hCntWnd, LPRECORDCORE lpRecord, LONG lDelta)
   {
   return (LPRECORDCORE) LLScrollList( (LPLINKNODE) lpRecord, lDelta);
   }


/****************************************************************************
 * Record and field manipulation procs
 ****************************************************************************/


/****************************************************************************
 * CntTopRecGet
 * CntTopRecSet
 *
 * Purpose:
 *  Gets/Sets the record which is currently the top record visible on the
 *  display. This is NOT the top of the record list.
 *  It can return NULL if called before the 1st record has been loaded.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *
 * Get Return Value:
 *  LPRECORDCORE  record which is currently at top of display
 ****************************************************************************/

LPRECORDCORE WINAPI CntTopRecGet( HWND hCntWnd )
    {
    LPCNTINFO lpCI = (GETCNT(hCntWnd))->lpCI;
    return lpCI->lpTopRec;
    }

BOOL WINAPI CntTopRecSet( HWND hCntWnd, LPRECORDCORE lpRec )
    {
    LPCNTINFO lpCI = (GETCNT(hCntWnd))->lpCI;

    if( !lpRec )
      return FALSE;

    lpCI->lpTopRec = lpRec;

    return TRUE;
    }

/****************************************************************************
 * CntBotRecGet
 *
 * Purpose:
 *  Gets the record which is currently the bottom record visible on the
 *  display. This is NOT the bottom of the record list.
 *  It can return NULL if called before the 1st record has been loaded.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *
 * Get Return Value:
 *  LPRECORDCORE  record which is currently at bottom of display
 ****************************************************************************/

LPRECORDCORE WINAPI CntBotRecGet( HWND hCntWnd )
{
    LPCONTAINER  lpCnt = GETCNT(hCntWnd);
    LPCNTINFO    lpCI  = (GETCNT(hCntWnd))->lpCI;
    LPRECORDCORE lpRec;

    if (lpCnt->iView & CV_ICON)
    {
      // makes no sense in Icon View
      lpRec = NULL;
    }
    else if ((lpCnt->iView & CV_TEXT || lpCnt->iView & CV_NAME) && lpCnt->iView & CV_FLOW)
    {
      // Find the last column and go to the last record
      LPFLOWCOLINFO lpCol;
      int           xStrCol, xEndCol;

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

      // Get a pointer to the last visible record in the display area.
      lpRec = CntScrollRecList( hCntWnd, lpCol->lpTopRec, lpCol->nRecs-1 );
    }
    else
    {
      // Get a pointer to the last visible record in the display area.
      lpRec = CntScrollRecList( hCntWnd, lpCI->lpTopRec, lpCI->nRecsDisplayed-1 );
    }

    return lpRec;
}

/****************************************************************************
 * CntRecsDispGet
 *
 * Purpose:
 *  Gets the number of records able to be displayed in the container record
 *  area at one time.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *
 * Return Value:
 *  int           nbr of records displayable
 ****************************************************************************/

int WINAPI CntRecsDispGet( HWND hCntWnd )
{
    LPCONTAINER lpCnt = GETCNT(hCntWnd);

    if (!(lpCnt->iView & CV_ICON) && !(lpCnt->iView & CV_FLOW))
      return lpCnt->lpCI->nRecsDisplayed;
    else
      return -1;
}

/****************************************************************************
 * CntScrollRecAreaEx
 *
 * Purpose:
 *  Vertically scrolls the record area of the container the number of rows
 *  specified. This function is used by applications which have turned on the
 *  CA_OWNERVSCROLL attribute or are using record locking.
 *
 *  For CA_OWNERVSCROLL containers:
 *    The container will send the associate window the vertical scrolling 
 *    notifications (CN_VSCROLL_*) but will not actually do any scrolling.
 *    The associate will execute the vertical scrolling by calling this
 *    function in response to the CN_VSCROLL_* notifications.
 *    The scroll thumb will NOT be adjusted for CA_OWNERVSCROLL containers.
 *
 *    NOTE: The application must ensure that the record list can accommodate
 *          the scroll before calling this function. In other words, if you
 *          want to increment 10 records make sure there are 10 records left
 *          in the list. If there are not, add to the list (or whatever is
 *          appropriate) before calling CntScrollRecArea.
 *
 *  For record locking containers:
 *    If the associate has locked the focus record and the user trys to do
 *    a vertical scroll of some sort the container will send the associate
 *    window a 'locked' vertical scrolling notification (CN_LK_VS_*) but will
 *    not actually do any scrolling. The associate is now alerted that the
 *    user is trying to move off the locked record and can take appropriate
 *    action. When the data is validated (or whatever) the associate should
 *    unlock the focus record by calling CntFocusRecUnlck and then call this
 *    function to execute the vertical scroll the user desired.
 *    The scroll thumb will also be adjusted.
 *
 *  The associate should call CntCNIncGet upon receiving a CN_VSCROLL_* or
 *  CN_LK_VS_* notification to get the increment value to pass to this
 *  function. If nIncrement is positive the net effect will be that of
 *  scrolling downward in the list. If nIncrement is negative the effect
 *  will be that scrolling upward (toward the beginning) in the list.
 *  It will return a pointer to the record which is visually at the top of
 *  the display AFTER the scrolling is done. This will be the new TOP record.
 *                   
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  LONG          lIncrement  - number of records to scroll (either forward or back)
 *
 * Return Value:
 *  LPRECORDCORE  record which is at top of display after scrolling is done
 ****************************************************************************/

LPRECORDCORE WINAPI CntScrollRecAreaEx( HWND hCntWnd, LONG lIncrement )
    {
    LPCNTINFO   lpCI = (GETCNT(hCntWnd))->lpCI;
    LPCONTAINER lpCnt = GETCNT(hCntWnd);
    LPCONTAINER lpCntFrame;
    LPCONTAINER lpCntSplit;
    HWND        hWndSplit=NULL;
    RECT        rect;
    BOOL        bSentQueryDelta=FALSE;
    LONG        lNewPos;
    int         nVscrollInc;
    int         nIncrement=(int)lIncrement;

    // Adjust the increment for the thumbpos if not owner vscroll.
    if( !(lpCnt->lpCI->flCntAttr & CA_OWNERVSCROLL) )
      nVscrollInc = max(-lpCnt->nVscrollPos, min(nIncrement, lpCnt->nVscrollMax - lpCnt->nVscrollPos));
    else
      nVscrollInc = nIncrement;

    // Scroll the record area of the container.
    if( nVscrollInc )
      {
      // Set the new vertical position if not owner vscroll.
      if( !(lpCnt->lpCI->flCntAttr & CA_OWNERVSCROLL) )
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
            lpCntFrame = GETCNT(lpCnt->hWndFrame);
            lpCntFrame->bDoingQueryDelta = TRUE;
            NotifyAssocWnd( lpCnt->hWnd1stChild, lpCnt, CN_QUERYDELTA, 0, NULL, NULL, nVscrollInc, 0, 0, NULL );
            lpCntFrame->bDoingQueryDelta = FALSE;
            }
          }

        // Set the new vertical position.
        lpCnt->nVscrollPos += nVscrollInc;
        }

      // Set the scrolling rect and execute the scroll.
      rect.left   = 0;
      rect.top    = lpCI->cyTitleArea + lpCI->cyColTitleArea;
      rect.right  = min( lpCnt->cxClient, lpCnt->xLastCol);
      rect.bottom = lpCnt->cyClient;
      ScrollWindow( lpCnt->hWnd1stChild, 0, -lpCI->cyRow * nVscrollInc, NULL, &rect );

      // Adjust the scroll thumb if not owner vscroll.
      if( !(lpCnt->lpCI->flCntAttr & CA_OWNERVSCROLL) )
        {
        SetScrollPos( lpCnt->hWnd1stChild, SB_VERT, lpCnt->nVscrollPos, TRUE );
        }

      // If container is split scroll the other split child also.
      if( lpCnt->dwStyle & CTS_SPLITBAR && lpCnt->bIsSplit )
        {
        // Get the window handle of the other split child.
        lpCntFrame = GETCNT( lpCnt->hWndFrame );
        if( hCntWnd == lpCntFrame->SplitBar.hWndLeft )
          hWndSplit = lpCntFrame->SplitBar.hWndRight;
        else
          hWndSplit = lpCntFrame->SplitBar.hWndLeft;

        // Get the container struct of the other split child.
        lpCntSplit = GETCNT( hWndSplit );

        // calc the scrolling rect
        rect.right  = min( lpCntSplit->cxClient, lpCntSplit->xLastCol);
        rect.bottom = lpCntSplit->cyClient;
    
        ScrollWindow( hWndSplit, 0, -lpCntSplit->lpCI->cyRow * nVscrollInc, NULL, &rect );

        // Adjust the thumbpos if not owner vscroll.
        if( !(lpCnt->lpCI->flCntAttr & CA_OWNERVSCROLL) )
          {
          lpCntSplit->nVscrollPos += nVscrollInc;
          SetScrollPos( hWndSplit, SB_VERT, lpCntSplit->nVscrollPos, TRUE );
          }
        }

      // Scroll the record list also so we always have a pointer to the 
      // record currently displayed at the top of the container wnd. 
      // If a CN_QUERYDELTA was sent we want to scroll the records from
      // the head taking into account the delta position.
      if( !bSentQueryDelta )
        lpCI->lpTopRec = CntScrollRecList( lpCnt->hWnd1stChild, lpCI->lpTopRec, nVscrollInc );
      else
        lpCI->lpTopRec = CntScrollRecList( lpCnt->hWnd1stChild, lpCnt->lpCI->lpRecHead, lpCnt->nVscrollPos-lpCnt->lDeltaPos );

      // Repaint the container.
      if( hWndSplit )
        UpdateWindow( hWndSplit );
      UpdateWindow( lpCnt->hWnd1stChild );
      }

    // Return the new top record.
    return lpCI->lpTopRec;
    }

/****************************************************************************
 * CntScrollRecArea
 *
 * Purpose:
 *  Vertically scrolls the record area of the container the number of rows
 *  specified. This function is used by applications which have turned on the
 *  CA_OWNERVSCROLL attribute or are using record locking.
 *
 *  This function only exists for backward compatibilty. 
 *  CntScrollRecAreaEx should be used instead.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  int           nIncrement  - number of records to scroll (either forward or back)
 *
 * Return Value:
 *  LPRECORDCORE  record which is at top of display after scrolling is done
 ****************************************************************************/

LPRECORDCORE WINAPI CntScrollRecArea( HWND hCntWnd, int nIncrement )
    {
    return CntScrollRecAreaEx( hCntWnd, nIncrement );
    }

/****************************************************************************
 * CntVScrollPosExSet
 *
 * Purpose:
 *  Sets the scrolling thumb for the vertical scroll bar in the container.
 *  This function is only used by applications which have turned on the
 *  CA_OWNERVSCROLL attribute for a container. If CA_OWNERVSCROLL is set
 *  the container will not do any positioning of the scrolling thumb.
 *  This function is also used by applications using the Delta model for
 *  vertical scrolling. When responding to a CN_QUERYFOCUS in the Delta
 *  model it is necessary to reset the thumb position when restoring the
 *  focus record.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  LONG          lPosition   - position of the scrolling thumb
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI CntVScrollPosExSet( HWND hCntWnd, LONG lPosition )
    {
    LPCONTAINER lpCnt = GETCNT(hCntWnd);
    LPCONTAINER lpCntFrame;
    LPCONTAINER lpCntSplit;
    int         nPosition=(int)lPosition;

    lpCnt->nVscrollPos = nPosition;
    SetScrollPos( lpCnt->hWnd1stChild, SB_VERT, lpCnt->nVscrollPos, TRUE );

    // If container is split update the other split child also.
    if( lpCnt->dwStyle & CTS_SPLITBAR && lpCnt->bIsSplit )
      {
      // Get the container struct of the left split child.
      lpCntFrame = GETCNT( lpCnt->hWndFrame );
      lpCntSplit = GETCNT( lpCntFrame->SplitBar.hWndLeft );

      lpCntSplit->nVscrollPos = nPosition;
      SetScrollPos( lpCntFrame->SplitBar.hWndLeft, SB_VERT, lpCntSplit->nVscrollPos, TRUE );
      }

    return;
    }

/****************************************************************************
 * CntVScrollPosSet
 *
 * Purpose:
 *  Sets the scrolling thumb for the vertical scroll bar in the container.
 *  This function is only used by applications which have turned on the
 *  CA_OWNERVSCROLL attribute for a container. If CA_OWNERVSCROLL is set
 *  the container will not do any positioning of the scrolling thumb.
 *
 *  This function only exists for backward compatibilty. 
 *  CntVScrollPosExSet should be used instead.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  int           nPosition   - position of the scrolling thumb
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI CntVScrollPosSet( HWND hCntWnd, int nPosition )
    {
    CntVScrollPosExSet( hCntWnd, nPosition );
    }

/****************************************************************************
 * CntScrollFldArea
 *
 * Purpose:
 *  Horizontally scrolls the field area of the container the number of 
 *  character widths specified. This function is used by applications which
 *  are using field locking. If the associate has locked the focus field and
 *  the user trys to do a horizontal scroll of some sort the container will
 *  send the associate window a 'locked' horizontal scrolling notification
 *  (CN_LK_HS_*) but will not actually do any scrolling. The associate is now
 *  alerted that the user is trying to move off the locked field and can take
 *  appropriate action. When the data is validated (or whatever) the associate
 *  should unlock the focus field by calling CntFocusFldUnlck and then call 
 *  this function to execute the horizontal scroll the user desired.
 *
 *  The associate should call CntCNIncGet upon receiving a CN_LK_HS_* 
 *  notification to get the increment value to pass to this function. If the
 *  increment is positive the net effect will be that of scrolling the fields
 *  to the right. If the increment is negative the effect will be that of
 *  scrolling the fields left (toward the 1st field).
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  int           nIncrement  - number of character widths to scroll (either left or right)
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI CntScrollFldArea( HWND hCntWnd, int nIncrement )
    {
    LPCONTAINER lpCnt = GETCNT(hCntWnd);
    LPCONTAINER lpCntFrame;
    RECT  rect;
/*    HWND  hWnd=GetFocus();    // scroll the child that has the focus*/
    HWND  hWnd;
    int   nHscrollInc=nIncrement;

    if( !lpCnt )
      return;

    // Figure out which container child to scroll and get its CONTAINER struct.
    // If the app passes in one of the child windows we will scroll that one.
    // If the app passes in the container handle we will scroll the permanent
    // 1st child (which is the right child when split). Remember, the frame
    // and the 1st child share the same CONTAINER struct. The left child
    // (which is created only when the container is split) has a different 
    // CONTAINER struct.

    hWnd = lpCnt->hWnd1stChild;
    if( hCntWnd != lpCnt->hWndFrame && hCntWnd != lpCnt->hWnd1stChild )
      {
      // If its not the frame or 1st child it must be the left split child.
      if( lpCnt->bIsSplit )
        {
        lpCntFrame = GETCNT(lpCnt->hWndFrame);
        if( hCntWnd == lpCntFrame->SplitBar.hWndLeft )
          {
          // We will be scrolling the left split child so get its CONTAINER struct.
          hWnd = lpCntFrame->SplitBar.hWndLeft;
          lpCnt = GETCNT(hWnd);

          if( !lpCnt )  // This shouldn't ever happen, but...
            return;
          }
        }
      }

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
      UpdateWindow( hWnd );
      }

    return;      
    }

/****************************************************************************
 * CntSelRecGet
 *
 * Purpose:
 *  Gets the record which is currently selected. 
 *  This is valid for containers with single select ONLY.
 *  It can return NULL if called before the 1st record has been selected.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *
 * Return Value:
 *  LPRECORDCORE  pointer to the record which is currently selected (if any)
 ****************************************************************************/

LPRECORDCORE WINAPI CntSelRecGet( HWND hCntWnd )
    {
    LPCNTINFO lpCI = (GETCNT(hCntWnd))->lpCI;
    return lpCI->lpSelRec;
    }

/****************************************************************************
 * CntSelectRec
 *
 * Purpose:
 *  Selects the specified record.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  LPRECORDCORE  lpRec       - pointer to record to be selected
 *
 * Return Value:
 *  BOOL          TRUE  if selection was set with no errors
 *                FALSE if selection could not be set
 ****************************************************************************/

BOOL WINAPI CntSelectRec( HWND hCntWnd, LPRECORDCORE lpRec )
    {
    LPCONTAINER lpCnt = GETCNT(hCntWnd);
    BOOL bSuccess=FALSE;

    if( !lpRec )
      return bSuccess;

    if( lpCnt->dwStyle & CTS_SINGLESEL || lpCnt->dwStyle & CTS_MULTIPLESEL || lpCnt->dwStyle & CTS_EXTENDEDSEL )
      {
      // If single select is active we will unselect the old selected record.
      if( lpCnt->dwStyle & CTS_SINGLESEL && lpCnt->lpCI->lpSelRec )
        {
        lpCnt->lpCI->lpSelRec->flRecAttr &= ~CRA_SELECTED;
/*        InvalidateCntRecord( hCntWnd, lpCnt, lpCnt->lpCI->lpSelRec, NULL, 1 );*/
        InvalidateCntRecord( lpCnt->hWnd1stChild, lpCnt, lpCnt->lpCI->lpSelRec, NULL, 1 );
        }
  
      // Mark the record as selected.
      lpRec->flRecAttr |= CRA_SELECTED;
      lpCnt->lpCI->lpSelRec = lpRec;
  
      // Unfocus the previously focused record and repaint it.
      // Leave the focus field unchanged.
      if( lpCnt->lpCI->lpFocusRec && lpCnt->lpCI->lpFocusFld )
        {
        // Clear out the old focus record but not the field.
        lpCnt->lpCI->lpFocusRec->flRecAttr &= ~CRA_CURSORED;
/*        InvalidateCntRecord( hCntWnd, lpCnt, lpCnt->lpCI->lpFocusRec, lpCnt->lpCI->lpFocusFld, 1 );*/
        if( lpCnt->lpCI->flCntAttr & CA_WIDEFOCUSRECT )
          InvalidateCntRecord( lpCnt->hWnd1stChild, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 1 );
        else
          InvalidateCntRecord( lpCnt->hWnd1stChild, lpCnt, lpCnt->lpCI->lpFocusRec, lpCnt->lpCI->lpFocusFld, 1 );
        }
  
      // This is the new focus record (focus field remains the same).
      lpRec->flRecAttr |= CRA_CURSORED;
      lpCnt->lpCI->lpFocusRec = lpRec;
  
      // Repaint the new selected record and notify the parent.
/*      InvalidateCntRecord( hCntWnd, lpCnt, lpRec, NULL, 2 );*/
      if( lpCnt->lpCI->flCntAttr & CA_WIDEFOCUSRECT )
        InvalidateCntRecord( lpCnt->hWnd1stChild, lpCnt, lpRec, NULL, 2 );
      else
        InvalidateCntRecord( lpCnt->hWnd1stChild, lpCnt, lpRec, NULL, 2 );

      NotifyAssocWnd( lpCnt->hWnd1stChild, lpCnt, CN_RECSELECTED, 0, lpRec, NULL, 0, 0, 0, NULL );
      NotifyAssocWnd( lpCnt->hWnd1stChild, lpCnt, CN_NEWFOCUSREC, 0, lpRec, NULL, 0, 0, 0, NULL );
  
      bSuccess = TRUE;
      }
    else if( lpCnt->dwStyle & CTS_BLOCKSEL )
      {
      // Select the whole record if in block select mode.
      bSuccess = CntSelectFld( hCntWnd, lpRec, NULL );
      }

    return bSuccess;
    }

/****************************************************************************
 * CntUnSelectRec
 *
 * Purpose:
 *  Unselects the specified record.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  LPRECORDCORE  lpRec       - pointer to record to be unselected
 *
 * Return Value:
 *  BOOL          TRUE  if selection was cleared with no errors
 *                FALSE if selection was not cleared
 ****************************************************************************/

BOOL WINAPI CntUnSelectRec( HWND hCntWnd, LPRECORDCORE lpRec )
    {
    LPCONTAINER lpCnt = GETCNT(hCntWnd);
    BOOL bSuccess=FALSE;

    if( !lpRec )
      return bSuccess;

    // Don't do anything if the record is not selected.
    if( lpRec->flRecAttr & CRA_SELECTED )
      {
      // NULL out the single select record if its the same.
      if( lpCnt->lpCI->lpSelRec == lpRec )
        lpCnt->lpCI->lpSelRec = NULL;

      // Unselect the record.
      lpRec->flRecAttr &= ~CRA_SELECTED;

      // Repaint the unselected record and notify the parent.
/*      InvalidateCntRecord( hCntWnd, lpCnt, lpRec, NULL, 2 );*/
      InvalidateCntRecord( lpCnt->hWnd1stChild, lpCnt, lpRec, NULL, 2 );
      NotifyAssocWnd( lpCnt->hWnd1stChild, lpCnt, CN_RECUNSELECTED, 0, lpRec, NULL, 0, 0, 0, NULL );

      bSuccess = TRUE;
      }
    else if( lpRec->flRecAttr & CRA_FLDSELECTED )
      {
      // Unselect the whole record if in block select mode.
      bSuccess = CntUnSelectFld( hCntWnd, lpRec, NULL );
      }

    return bSuccess;
    }

/****************************************************************************
 * CntIsRecSelected
 *
 * Purpose:
 *  Determines whether the specified record is currently selected.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  LPRECORDCORE  lpRec       - pointer to record of interest
 *
 * Return Value:
 *  BOOL          TRUE  if record is currently selected
 *                FALSE if record is not currently selected
 ****************************************************************************/

BOOL WINAPI CntIsRecSelected( HWND hCntWnd, LPRECORDCORE lpRec )
    {
    if( !lpRec )
      return FALSE;

    if( lpRec->flRecAttr & CRA_SELECTED || lpRec->flRecAttr & CRA_FLDSELECTED )
      return TRUE;
    else
      return FALSE;
    }

/****************************************************************************
 * CntSelectFld
 *
 * Purpose:
 *  Selects the specified record/field cell. If the field pointer is NULL
 *  then all fields for this record will be selected. This function is for
 *  containers using block field selection (CTS_BLOCKSEL).
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  LPRECORDCORE  lpRec       - pointer to record
 *  LPFIELDINFO   lpFld       - pointer to field to be selected
 *                              (if NULL then all fields will be selected)
 * Return Value:
 *  BOOL          TRUE  if selection was set with no errors
 *                FALSE if selection could not be set
 ****************************************************************************/

BOOL WINAPI CntSelectFld( HWND hCntWnd, LPRECORDCORE lpRec, LPFIELDINFO lpFld )
    {
    LPCONTAINER    lpCnt = GETCNT(hCntWnd);
    LPSELECTEDFLD lpSelFld;

    if( !(lpCnt->dwStyle & CTS_BLOCKSEL) || !lpRec )
      return FALSE;
 
    // If the frame window was passed in make it the 1st child.
    if( hCntWnd == lpCnt->hWndFrame )
      hCntWnd = lpCnt->hWnd1stChild;

    // Add this field to the record's selected field list.    
    lpSelFld = CntNewSelFld();
    if( lpSelFld )
      {
      lpSelFld->lpFld = lpFld;
      CntAddSelFldTail( lpRec, lpSelFld );
 
      // Mark the record as having at least one field selected.
      lpRec->flRecAttr |= CRA_FLDSELECTED;

      // Repaint the newly selected record cell and notify the parent.
/*      InvalidateCntRecord( hCntWnd, lpCnt, lpRec, lpFld, 2 );*/
      InvalidateCntRecord( hCntWnd, lpCnt, lpRec, lpFld, 1 );
      NotifyAssocWnd( hCntWnd, lpCnt, CN_FLDSELECTED, 0, lpRec, lpFld, 0, 0, 0, NULL );
      }

    return TRUE;
    }

/****************************************************************************
 * CntUnSelectFld
 *
 * Purpose:
 *  Unselects the specified record/field cell. If the field pointer is NULL
 *  then all fields for this record will be unselected. This function is for
 *  containers using block field selection (CTS_BLOCKSEL).
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  LPRECORDCORE  lpRec       - pointer to record
 *  LPFIELDINFO   lpFld       - pointer to field to be unselected
 *                              (if NULL then all fields will be unselected)
 * Return Value:
 *  BOOL          TRUE  if selection was cleared with no errors
 *                FALSE if selection was not cleared
 ****************************************************************************/

BOOL WINAPI CntUnSelectFld( HWND hCntWnd, LPRECORDCORE lpRec, LPFIELDINFO lpFld )
    {
    LPCONTAINER    lpCnt = GETCNT(hCntWnd);
    LPSELECTEDFLD lpSelFld, lpTemp;

    if( !(lpCnt->dwStyle & CTS_BLOCKSEL) || !lpRec )
      return FALSE;

    // Don't do anything if the record is not selected.
    if( !(lpRec->flRecAttr & CRA_FLDSELECTED) )
      return FALSE;

    // If the frame window was passed in make it the 1st child.
    if( hCntWnd == lpCnt->hWndFrame )
      hCntWnd = lpCnt->hWnd1stChild;

    if( lpFld )
      {
      // Remove the specified field from the selected field list.
      if( lpSelFld = CntFindSelFld( lpRec, lpFld ) )
        {
        if( lpTemp = CntRemSelFld( lpRec, lpSelFld ) )
          CntFreeSelFld( lpTemp );
        }
      }
    else
      {
      // Unselect all selected fields by killing the selected list.
      CntKillSelFldLst( lpRec );
      }

    // If no more fields are selected clear the flag.
    if( !lpRec->lpSelFldHead )
      lpRec->flRecAttr &= ~CRA_FLDSELECTED;

    // Repaint the unselected field(s) and notify the parent.
/*    InvalidateCntRecord( hCntWnd, lpCnt, lpRec, lpFld, 2 );*/
    InvalidateCntRecord( hCntWnd, lpCnt, lpRec, lpFld, 1 );
    NotifyAssocWnd( hCntWnd, lpCnt, CN_FLDUNSELECTED, 0, lpRec, lpFld, 0, 0, 0, NULL );

    return TRUE;
    }

/****************************************************************************
 * CntIsFldSelected
 *
 * Purpose:
 *  Determines whether the specified record/field cell is currently selected.
 *  If the field pointer is NULL the function will return TRUE if ANY field
 *  in this record is currently selected. This function is for containers
 *  using block field selection (CTS_BLOCKSEL).
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  LPRECORDCORE  lpRec       - pointer to record
 *  LPFIELDINFO   lpFld       - pointer to field
 *                              (if lpFld=NULL the function will return
 *                               TRUE if any field is selected)
 * Return Value:
 *  BOOL          TRUE  if record/field cell is currently selected
 *                FALSE if record/field cell is not currently selected
 ****************************************************************************/

BOOL WINAPI CntIsFldSelected( HWND hCntWnd, LPRECORDCORE lpRec, LPFIELDINFO lpFld )
    {
    LPCONTAINER    lpCnt = GETCNT(hCntWnd);
    LPSELECTEDFLD lpSelFld;
    BOOL          bFound=FALSE;

    if( !(lpCnt->dwStyle & CTS_BLOCKSEL) || !lpRec )
      return FALSE;

    // If no field pointer let em know if any field is selected.
    if( !lpFld )
      {
      // If set it means at least one field is selected.
      if( lpRec->flRecAttr & CRA_FLDSELECTED )
        bFound = TRUE;
      }
    else
      {
      // Check out the selected field list for this record.
      lpSelFld = lpRec->lpSelFldHead;
      while( lpSelFld )
        {
        // break out if we find the field
        if( lpSelFld->lpFld == lpFld )
          {
          bFound = TRUE;
          break;  
          }
        lpSelFld = lpSelFld->lpNext;
        }
      }

    return bFound;
    }

/****************************************************************************
 * CntFldAtPtGet
 * CntRecAtPtGet
 *
 * Purpose:
 *  Gets the field or record located at the specified point.
 *  Returns a NULL pointer if no field or record was found at the point.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  LPPOINT       lpPt        - x/y point in container client coordinates
 *
 * Return Value:
 *  LPFIELDINFO   pointer to selected field node
 *  LPRECORDCORE  pointer to selected record node
 ****************************************************************************/

LPFIELDINFO WINAPI CntFldAtPtGet( HWND hCntWnd, LPPOINT lpPt )
    {
    LPCONTAINER   lpCnt = GETCNT(hCntWnd);
    LPRECORDCORE lpRec=NULL;
    LPFIELDINFO  lpCol=NULL;
    RECT         rect;
    int          xLastCol, yLastRec, i;
    int          xStrCol, xEndCol;

    // There is nothing to the right of this value.
    xLastCol = lpCnt->lpCI->nMaxWidth - lpCnt->nHscrollPos * lpCnt->lpCI->cxChar;

    // Get the bottom of the last visible record.
    yLastRec = lpCnt->lpCI->cyTitleArea + lpCnt->lpCI->cyColTitleArea;
    if( lpCnt->lpCI->lpTopRec )
      {
      lpRec = lpCnt->lpCI->lpTopRec;
      for( i=0; i < lpCnt->lpCI->nRecsDisplayed; i++ )
        {
        yLastRec += lpCnt->lpCI->cyRow;   // add a row height
        if( lpRec->lpNext )              // break if we hit the last record
          lpRec = lpRec->lpNext;
        else
          break;
        }
      }

    // Set up the rect for the field area (including field titles).
    rect.left   = 0;
    rect.right  = min(lpCnt->cxClient, xLastCol);
    rect.top    = lpCnt->lpCI->cyTitleArea;
    rect.bottom = min(lpCnt->cyClient, yLastRec);

    // If the point is within the field area get the field.
    if( PtInRect( &rect, *lpPt) )
      lpCol = GetFieldAtPt( lpCnt, lpPt, &xStrCol, &xEndCol );

    return lpCol;
    }

LPRECORDCORE WINAPI CntRecAtPtGet( HWND hCntWnd, LPPOINT lpPt )
    {
    LPCONTAINER  lpCnt = GETCNT(hCntWnd);
    LPRECORDCORE lpRec=NULL;
    RECT         rect;
    int          xLastCol, yLastRec, nRec, i;
    LPFLOWCOLINFO lpCol;
    BOOL         bFound=TRUE;

    if (lpCnt->iView & CV_ICON)
    {
      LPZORDERREC lpOrderRec;

      // find record at the point - this function is in CVICON.C - returns NULL if no
      // record at the point
      lpOrderRec = FindClickedRecordIcn( hCntWnd, lpCnt, *lpPt );

      if (lpOrderRec)
        lpRec = lpOrderRec->lpRec;
    }
    else 
    {
      // There is nothing to the right of this value.
      xLastCol = lpCnt->lpCI->nMaxWidth - lpCnt->nHscrollPos * lpCnt->lpCI->cxChar;

      // Get the bottom of the last visible record.
      yLastRec = lpCnt->lpCI->cyTitleArea + lpCnt->lpCI->cyColTitleArea;
      if( lpCnt->lpCI->lpTopRec )
      {
        lpRec = lpCnt->lpCI->lpTopRec;
        for( i=0; i < lpCnt->lpCI->nRecsDisplayed; i++ )
        {
          yLastRec += lpCnt->lpCI->cyRow;   // add a row height
          if( lpRec->lpNext )              // break if we hit the last record
            lpRec = lpRec->lpNext;
          else
            break;
        }
        lpRec = NULL;                      // reset to NULL
      }

      // Set up the rect for the record area.         
      rect.left   = 0;
      rect.right  = min(lpCnt->cxClient, xLastCol);
      rect.top    = lpCnt->lpCI->cyTitleArea + lpCnt->lpCI->cyColTitleArea;
      rect.bottom = min(lpCnt->cyClient, yLastRec);

      // If the point is within the record area get the record.
      if( PtInRect( &rect, *lpPt) )
      {
        if (lpCnt->iView & CV_FLOW)
        {
          int xStrCol, xEndCol;

          // get the column at this point
          lpCol = TxtGetColAtPt( lpCnt, lpPt, &xStrCol, &xEndCol );
          if( lpCol )
          {
            // If in flowed view and on the last column
            // but below the last displayed record, then no record
            if (!lpCol->lpNext)
            {
              nRec = (lpPt->y - rect.top) / lpCnt->lpCI->cyRow + 1;
              if (nRec > lpCol->nRecs)
                bFound = FALSE;
            }
          }
          else 
            bFound = FALSE;
        }
        
        if (bFound)
        {
          // Calc which record the point is in.
          nRec = (lpPt->y - rect.top) / lpCnt->lpCI->cyRow;
      
          // Scroll the record list to the record of interest.
          if (lpCnt->iView & CV_FLOW)
          {
            if (lpCol->lpTopRec)
              lpRec = CntScrollRecList( lpCnt->hWnd1stChild, lpCol->lpTopRec, nRec );
          }
          else
          {
            if( lpCnt->lpCI->lpTopRec )
              lpRec = CntScrollRecList( lpCnt->hWnd1stChild, lpCnt->lpCI->lpTopRec, nRec );
          }
        }
      }
    }

    return lpRec;
    }

/****************************************************************************
 * CntKeyBdEnable
 *
 * Purpose:
 *  Enables or disables keyboard input to the container.
 *  Under certain conditions the associate may not want the container to 
 *  send keystroke events. If the keyboard is disabled the container will
 *  beep when it receives a keyboard event but will NOT send the associate
 *  a notification about it.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  BOOL          bEnable     - flag indicating whether to enable keyboard:
 *                                if TRUE  keyboard event notifications will be sent;
 *                                if FALSE keyboard event notifications will NOT be sent
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI CntKeyBdEnable( HWND hCntWnd, BOOL bEnable )
    {
    LPCONTAINER lpCnt = GETCNT(hCntWnd);

    lpCnt->lpCI->bSendKBEvents = bEnable;
    }

/****************************************************************************
 * CntMouseEnable
 *
 * Purpose:
 *  Enables or disables mouse input to the container.
 *  Under certain conditions the associate may not want the container to 
 *  respond to mouse events. If the mouse is disabled the container will
 *  beep when it receives a mouse event but will NOT send the associate
 *  a notification about it.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  BOOL          bEnable     - flag indicating whether to enable mouse events:
 *                                if TRUE  mouse event notifications will be sent;
 *                                if FALSE mouse event notifications will NOT be sent
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI CntMouseEnable( HWND hCntWnd, BOOL bEnable )
    {
    LPCONTAINER lpCnt = GETCNT(hCntWnd);

    lpCnt->lpCI->bSendMouseEvents = bEnable;
    }

/****************************************************************************
 * CntFocusMove
 *
 * Purpose:
 *  Moves the focus cell in the specified direction. If moving left or
 *  right it will move to the next non-hidden field. It will return FALSE
 *  if at the first or last non-hidden field for horizontal moves or at
 *  the first or last record for vertical moves.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *  UINT          wDir        - direction to move
 *
 * Return Value:
 *  BOOL          TRUE  if focus was moved
 *                FALSE if focus could not be moved
 ****************************************************************************/

BOOL WINAPI CntFocusMove( HWND hWnd, UINT wDir )
    {
    HWND         hCntWnd;
    LPCONTAINER  lpCnt = GETCNT(hWnd);
    LPCONTAINER  lpCntFrame;
    LPFIELDINFO  lpCol;
    BOOL         bMoved=FALSE;
    LPRECORDCORE lpRec;
    int          i;
    
    // Make sure the frame window was passed in.
    if( hWnd != lpCnt->hWndFrame )
      lpCnt = GETCNT( lpCnt->hWndFrame );

    // Get the frame window CONTAINER struct.
    // Note: The frame and the right child have the same CONTAINER struct.
    lpCntFrame = GETCNT( lpCnt->hWndFrame );

    // Get the container child which currently has the focus.
    hCntWnd = GetFocus();

    // If the left split child has the focus use its CONTAINER struct.
    if( (lpCntFrame->dwStyle & CTS_SPLITBAR) && lpCntFrame->bIsSplit &&
         hCntWnd == lpCntFrame->SplitBar.hWndLeft )
      lpCnt = GETCNT( lpCntFrame->SplitBar.hWndLeft );

    // Fail if no focus record (means there are no records).
    if( !lpCnt->lpCI->lpFocusRec )
      return bMoved;

    // If moving horizontally fail if no focus field.
    if( wDir == CFM_LEFT     || wDir == CFM_RIGHT   ||
        wDir == CFM_FIRSTFLD || wDir == CFM_LASTFLD )
    {
      if (lpCnt->iView & CV_DETAIL)
        if( !lpCnt->lpCI->lpFocusFld )
          return bMoved;
    }

    // Don't change windows if the focus record or field is locked.
    if( wDir == CFM_NEXTSPLITWND )
    {
      if( lpCnt->lpCI->bFocusRecLocked || lpCnt->lpCI->bFocusFldLocked )
        return bMoved;
    }

    // Simulate a pressed control key for these cases.
    if( wDir == CFM_FIRSTFLD || wDir == CFM_LASTFLD )
    {
      lpCnt->bSimulateCtrlKey  = TRUE;
      lpCnt->lpCI->bSimCtrlKey = TRUE;
    }

    switch( wDir )
    {
      case CFM_LEFT: // move focus left
        if ( lpCnt->iView & CV_DETAIL )
        {
          // Detail view - move left one field
          if( lpCol = CntPrevFld( lpCnt->lpCI->lpFocusFld ) )
          {
            // If field is hidden find the 1st non-hidden before it.
            if( !lpCol->cxWidth )
            {
              // Break out when we find a col with some width.
              while( lpCol->lpPrev )
              {
                lpCol = lpCol->lpPrev;
                if( lpCol->cxWidth )
                  break;  
              }
            }

            // Move the focus if there was a non-hidden field to the left.
            if( lpCol->cxWidth )
            {                 
              FORWARD_WM_KEYDOWN( hCntWnd, VK_LEFT, 0, 0, SendMessage );
              bMoved = TRUE;
            }
          }
        }
        else if ((lpCnt->iView & CV_TEXT) || (lpCnt->iView & CV_NAME))
        {
          // Text and Name views - move left one column if in flowed view
          if (lpCnt->iView & CV_FLOW)
          {
            BOOL bFound=FALSE;

            // if focus rec not in first column, we are able to move left
            lpRec = lpCnt->lpColHead->lpTopRec;
            for (i=0; i<lpCnt->lpColHead->nRecs && lpRec; i++)
            {
              if (lpRec==lpCnt->lpCI->lpFocusRec)
                bFound = TRUE;
              lpRec = lpRec->lpNext;
            }
            if (!bFound)  
            {                 
              FORWARD_WM_KEYDOWN( hCntWnd, VK_LEFT, 0, 0, SendMessage );
              bMoved = TRUE;
            }
          }
        }
        else if (lpCnt->iView & CV_ICON)
        {
          // find the next record to the left - if a record is returned we are able to move
          lpRec = FindNextRecIcn( hCntWnd, lpCnt, lpCnt->lpCI->lpFocusRec, VK_LEFT );

          if (lpRec)
          {                 
            FORWARD_WM_KEYDOWN( hCntWnd, VK_LEFT, 0, 0, SendMessage );
            bMoved = TRUE;
          }
        }
        break;

      case CFM_RIGHT: // move focus right
        if ( lpCnt->iView & CV_DETAIL )
        {
          // detail view - move right one field
          if( lpCol = CntNextFld( lpCnt->lpCI->lpFocusFld ) )
          {
            // If field is hidden find the 1st non-hidden after it.
            if( !lpCol->cxWidth )
            {
              // Break out when we find a col with some width.
              while( lpCol->lpNext )
              {
                lpCol = lpCol->lpNext;
                if( lpCol->cxWidth )
                  break;  
              }
            }

            // Move the focus if there was a non-hidden field to the right.
            if( lpCol->cxWidth )
            {                 
              FORWARD_WM_KEYDOWN( hCntWnd, VK_RIGHT, 0, 0, SendMessage );
              bMoved = TRUE;
            }
          }
        }
        else if ((lpCnt->iView & CV_TEXT) || (lpCnt->iView & CV_NAME))
        {
          // Text and Name views - move right one column if in flowed view
          if (lpCnt->iView & CV_FLOW)
          {
            BOOL bFound=FALSE;

            // if focus rec not in last column, we are able to move right
            lpRec = lpCnt->lpColTail->lpTopRec;
            for (i=0; i<lpCnt->lpColTail->nRecs && lpRec; i++)
            {
              if (lpRec==lpCnt->lpCI->lpFocusRec)
                bFound = TRUE;
              lpRec = lpRec->lpNext;
            }
            if (!bFound)  
            {                 
              FORWARD_WM_KEYDOWN( hCntWnd, VK_RIGHT, 0, 0, SendMessage );
              bMoved = TRUE;
            }
          }
        }
        else if (lpCnt->iView & CV_ICON)
        {
          // find the next record to the right - if a record is returned we are able to move
          lpRec = FindNextRecIcn( hCntWnd, lpCnt, lpCnt->lpCI->lpFocusRec, VK_RIGHT );

          if (lpRec)
          {                 
            FORWARD_WM_KEYDOWN( hCntWnd, VK_RIGHT, 0, 0, SendMessage );
            bMoved = TRUE;
          }
        }
        break;

      case CFM_UP: // move focus up
        if (lpCnt->iView & CV_DETAIL)
        {
          // Detail view - move up one record
          if( CntPrevRec( lpCnt->lpCI->lpFocusRec ) )
          {
            FORWARD_WM_KEYDOWN( hCntWnd, VK_UP, 0, 0, SendMessage );
            bMoved = TRUE;
          }
        }
        else if ((lpCnt->iView & CV_TEXT) || (lpCnt->iView & CV_NAME))
        {
          if ( lpCnt->iView & CV_FLOW )
          {
            int nRecPos;

            // flowed - Get the position of the focus in the display area - if it is zero
            // we cannot move up

            // Remember that with mode=0 nothing happens in this call.
            nRecPos = InvalidateCntRecord( hCntWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 0 );
            if( nRecPos )
            {
              FORWARD_WM_KEYDOWN( hCntWnd, VK_UP, 0, 0, SendMessage );
              bMoved = TRUE;
            }
          }
          else
          {
            // non-flowed - see if there is a record above
            if( CntPrevRec( lpCnt->lpCI->lpFocusRec ) )
            {
              FORWARD_WM_KEYDOWN( hCntWnd, VK_UP, 0, 0, SendMessage );
              bMoved = TRUE;
            }
          }
        }
        else if (lpCnt->iView & CV_ICON)
        {
          // find the next record up - if a record is returned we are able to move
          lpRec = FindNextRecIcn( hCntWnd, lpCnt, lpCnt->lpCI->lpFocusRec, VK_UP );

          if (lpRec)
          {                 
            FORWARD_WM_KEYDOWN( hCntWnd, VK_UP, 0, 0, SendMessage );
            bMoved = TRUE;
          }
        }
        break;
      case CFM_DOWN: // move focus down
        if (lpCnt->iView & CV_DETAIL)
        {
          // Detail view - move down one record
          if( CntNextRec( lpCnt->lpCI->lpFocusRec ) )
          {
            FORWARD_WM_KEYDOWN( hCntWnd, VK_DOWN, 0, 0, SendMessage );
            bMoved = TRUE;
          }
        }
        else if ((lpCnt->iView & CV_TEXT) || (lpCnt->iView & CV_NAME))
        {
          if ( lpCnt->iView & CV_FLOW )
          {
            int nRecPos;

            // flowed - Get the position of the focus in the display area - if it is at the
            // bottom, we cannot move down (or if it is the last record)

            // Remember that with mode=0 nothing happens in this call.
            nRecPos = InvalidateCntRecord( hCntWnd, lpCnt, lpCnt->lpCI->lpFocusRec, NULL, 0 );
            if( nRecPos != lpCnt->lpCI->nRecsDisplayed-1 &&
                CntNextRec( lpCnt->lpCI->lpFocusRec ) )
            {
              FORWARD_WM_KEYDOWN( hCntWnd, VK_DOWN, 0, 0, SendMessage );
              bMoved = TRUE;
            }
          }
          else
          {
            // non-flowed - see if there is a record below
            if( CntNextRec( lpCnt->lpCI->lpFocusRec ) )
            {
              FORWARD_WM_KEYDOWN( hCntWnd, VK_DOWN, 0, 0, SendMessage );
              bMoved = TRUE;
            }
          }
        }
        else if (lpCnt->iView & CV_ICON)
        {
          // find the next record down - if a record is returned we are able to move
          lpRec = FindNextRecIcn( hCntWnd, lpCnt, lpCnt->lpCI->lpFocusRec, VK_DOWN );

          if (lpRec)
          {                 
            FORWARD_WM_KEYDOWN( hCntWnd, VK_DOWN, 0, 0, SendMessage );
            bMoved = TRUE;
          }
        }
        break;
      case CFM_PAGEUP:                             // move focus up 1 page
        if (lpCnt->iView & CV_ICON)
        {
          // icon view - we will move focus if there is a record above or
          // if we can still scroll
          lpRec = FindNextRecIcn( hCntWnd, lpCnt, lpCnt->lpCI->lpFocusRec, VK_PRIOR );

          if( (lpRec != lpCnt->lpCI->lpFocusRec && lpRec) ||
               lpCnt->nVscrollPos )
          {
            FORWARD_WM_KEYDOWN( hCntWnd, VK_PRIOR, 0, 0, SendMessage );
            bMoved = TRUE;
          }
        }
        else
        {
          if( CntPrevRec( lpCnt->lpCI->lpFocusRec ) )
          {
            FORWARD_WM_KEYDOWN( hCntWnd, VK_PRIOR, 0, 0, SendMessage );
            bMoved = TRUE;
          }
        }
        break;
      case CFM_PAGEDOWN:                           // move focus down 1 page
        if (lpCnt->iView & CV_ICON)
        {
          // icon view - we will move focus if there is a record below or
          // if we can still scroll
          lpRec = FindNextRecIcn( hCntWnd, lpCnt, lpCnt->lpCI->lpFocusRec, VK_NEXT );

          if( (lpRec != lpCnt->lpCI->lpFocusRec && lpRec) ||
               lpCnt->nVscrollPos != lpCnt->nVscrollMax)
          {
            FORWARD_WM_KEYDOWN( hCntWnd, VK_NEXT, 0, 0, SendMessage );
            bMoved = TRUE;
          }
        }
        else
        {
          if( CntNextRec( lpCnt->lpCI->lpFocusRec ) )
          {
            FORWARD_WM_KEYDOWN( hCntWnd, VK_NEXT, 0, 0, SendMessage );
            bMoved = TRUE;
          }
        }
        break;
      case CFM_FIRSTFLD:                           // move focus to 1st fld
        if (lpCnt->iView & CV_DETAIL)
        {
          FORWARD_WM_KEYDOWN( hCntWnd, VK_LEFT, 0, 0, SendMessage );
          bMoved = TRUE;
        }
        break;
      case CFM_LASTFLD:                            // move focus to last fld
        if (lpCnt->iView & CV_DETAIL)
        {
          FORWARD_WM_KEYDOWN( hCntWnd, VK_RIGHT, 0, 0, SendMessage );
          bMoved = TRUE;
        }
        break;
      case CFM_HOME:                               // move focus to 1st rec
        FORWARD_WM_KEYDOWN( hCntWnd, VK_HOME, 0, 0, SendMessage );
        bMoved = TRUE;
        break;
      case CFM_END:                                // move focus to last rec
        FORWARD_WM_KEYDOWN( hCntWnd, VK_END, 0, 0, SendMessage );
        bMoved = TRUE;
        break;
      case CFM_NEXTSPLITWND:                       // move focus to next split wnd
        if( (lpCntFrame->dwStyle & CTS_SPLITBAR) && lpCntFrame->bIsSplit )
          {
          // Set the focus to the split child who currently does not have it.
          if( hCntWnd == lpCntFrame->SplitBar.hWndLeft )
            SetFocus( lpCntFrame->SplitBar.hWndRight );
          else
            SetFocus( lpCntFrame->SplitBar.hWndLeft );
          bMoved = TRUE;
          }
        break;
      }

    // If trying to move horizontally return FALSE if focus field is locked.
    // (the assoc will already have gotten a CN_LK_NEWFOCUSFLD msg)
    if (lpCnt->iView & CV_DETAIL)
    {
      if( wDir == CFM_LEFT     || wDir == CFM_RIGHT   ||
          wDir == CFM_FIRSTFLD || wDir == CFM_LASTFLD )
      {
        if( lpCnt->lpCI->bFocusFldLocked )
          bMoved = FALSE;
      }
    }

    // If trying to move vertically return FALSE if focus record is locked.
    // (the assoc will already have gotten a CN_LK_NEWFOCUSREC msg)
    if( wDir == CFM_UP       || wDir == CFM_DOWN     ||
        wDir == CFM_PAGEUP   || wDir == CFM_PAGEDOWN ||
        wDir == CFM_HOME     || wDir == CFM_END )
    {
      if( lpCnt->lpCI->bFocusRecLocked )
        bMoved = FALSE;
    }

    lpCnt->bSimulateCtrlKey  = FALSE;
    lpCnt->lpCI->bSimCtrlKey = FALSE;
    return bMoved;
    }

/****************************************************************************
 * CntIsFocusCellRO
 *
 * Purpose:
 *  Queries whether the focus cell is positioned on a read-only cell.
 *  It will return TRUE if any of the following are true:
 *
 *    - there is no focus cell (can only happen if there are no records)
 *    - the container    is read-only (CTS_READONLY)
 *    - the focus field  is read-only (CFA_FLDREADONLY)
 *    - the focus record is read-only (CRA_RECREADONLY)
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *
 * Return Value:
 *  BOOL          TRUE  if focus cell is read-only
 *                FALSE if focus cell is editable
 ****************************************************************************/

BOOL WINAPI CntIsFocusCellRO( HWND hCntWnd )
    {
    LPCONTAINER  lpCnt = GETCNT(hCntWnd);
    BOOL        bCellIsRO=TRUE;

    if( lpCnt->lpCI->lpFocusRec && lpCnt->lpCI->lpFocusFld )
      {
      if( !(lpCnt->dwStyle & CTS_READONLY) &&
          !(lpCnt->lpCI->lpFocusFld->flColAttr & CFA_FLDREADONLY) &&
          !(lpCnt->lpCI->lpFocusRec->flRecAttr & CRA_RECREADONLY) )
        {
        // The focus cell is editable.
        bCellIsRO = FALSE;
        }
      }

    return bCellIsRO;
    }

/****************************************************************************
 * CntFocusSet
 *
 * Purpose:
 *  Sets the focus on the specified cell (record/field).
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  LPRECORDCORE  lpRec       - pointer to new focus record
 *  LPFIELDINFO   lpFld       - pointer to new focus field
 *                              (if NULL the 1st field will get the focus, or if not in
 *                               detail view, it is ignored)
 * Return Value:
 *  BOOL          TRUE  if focus was set with no errors
 *                FALSE if focus could not be set to the new cell
 ****************************************************************************/

BOOL WINAPI CntFocusSet( HWND hCntWnd, LPRECORDCORE lpRec, LPFIELDINFO lpFld )
    {
    LPCONTAINER   lpCnt = GETCNT(hCntWnd);
    LPCONTAINER   lpCntFrame, lpCntFocus;
    HWND         hWndFocus;
    LPFIELDINFO  lpCol;
    BOOL         bSSel=FALSE;
    BOOL         bXSel=FALSE;
    BOOL         bBlkSel=FALSE;
    BOOL         bDtlView=FALSE;

    if (lpCnt->iView & CV_DETAIL)
      bDtlView = TRUE;

    if( !lpFld && bDtlView )
      lpCol = CntFldHeadGet( hCntWnd );
    else
      lpCol = lpFld;

    if( !lpRec )
      return FALSE;
    else if ( !lpCol && bDtlView )
      return FALSE;

    // Set select type flags if active.
    if( lpCnt->dwStyle & CTS_SINGLESEL )
      bSSel = TRUE;
    if( lpCnt->dwStyle & CTS_EXTENDEDSEL )
      bXSel = TRUE;
    if( lpCnt->dwStyle & CTS_BLOCKSEL )
      bBlkSel = TRUE;

    // Get the focus wnd and container struct.
    if( lpCnt->dwStyle & CTS_SPLITBAR && lpCnt->bIsSplit )
      {
      // Get the split window handles from the frame's stuct.
      lpCntFrame = GETCNT( lpCnt->hWndFrame );

      // See which split window has the focus.
      if( GetFocus() == lpCntFrame->SplitBar.hWndLeft )
        hWndFocus = lpCntFrame->SplitBar.hWndLeft;
      else
        hWndFocus = lpCntFrame->SplitBar.hWndRight;

      lpCntFocus = GETCNT( hWndFocus );
      }
    else
      {
      hWndFocus = lpCnt->hWnd1stChild;
      lpCntFocus = lpCnt;
      }

    if( bSSel && lpCnt->lpCI->lpSelRec )
      {
      // If single select is active we will chg the selected record also.
      lpCnt->lpCI->lpSelRec->flRecAttr &= ~CRA_SELECTED;
      InvalidateCntRecord( hWndFocus, lpCntFocus, lpCntFocus->lpCI->lpSelRec, NULL, 1 );
      }
    else if( bXSel )
      {
      // If extended select is active we will clear the old selection list.
      ClearExtSelectList( hWndFocus, lpCntFocus );
      }
    else if( bBlkSel )
      {
      // If block select is active we will clear the old selection list.
      ClearBlkSelectList( hWndFocus, lpCntFocus );
      }

    // Unfocus the previously focused record/field and repaint it.
    if( lpCnt->lpCI->lpFocusRec )
      lpCnt->lpCI->lpFocusRec->flRecAttr &= ~CRA_CURSORED;
    if( lpCnt->lpCI->lpFocusFld && bDtlView )
      lpCnt->lpCI->lpFocusFld->flColAttr &= ~CFA_CURSORED;
    if ( bDtlView )
    {
      if( lpCnt->lpCI->lpFocusRec && lpCnt->lpCI->lpFocusFld )
      {
        if( lpCntFocus->lpCI->flCntAttr & CA_WIDEFOCUSRECT )
          InvalidateCntRecord( hWndFocus, lpCntFocus, lpCnt->lpCI->lpFocusRec, NULL, 2 );
        else
          InvalidateCntRecord( hWndFocus, lpCntFocus, lpCnt->lpCI->lpFocusRec,
                                                      lpCnt->lpCI->lpFocusFld, 2 );
      }
    }
    else
    {
      if( lpCnt->lpCI->lpFocusRec )
        InvalidateCntRecord( hWndFocus, lpCntFocus, lpCnt->lpCI->lpFocusRec, NULL, 2 );
    }

    // Set the new focus record and field.
    lpCnt->lpCI->lpFocusRec = lpRec;
    lpRec->flRecAttr |= CRA_CURSORED;
    if ( bDtlView )
    {
      lpCnt->lpCI->lpFocusFld = lpCol;
      lpCol->flColAttr |= CFA_CURSORED;
    }

    // If selection is active select the new focus record also.
    if( bSSel )
      {
      lpCnt->lpCI->lpSelRec = lpRec;
      lpRec->flRecAttr |= CRA_SELECTED;
      }
    else if( bXSel )
      {
      lpRec->flRecAttr |= CRA_SELECTED;
      }
    else if( bBlkSel )
      {
      CntSelectFld( hWndFocus, lpRec, lpCol );
      }

    // Repaint the new focus cell (or entire record if single select).
    if( lpCntFocus->lpCI->flCntAttr & CA_WIDEFOCUSRECT || !bDtlView )
      InvalidateCntRecord( hWndFocus, lpCntFocus, lpRec, NULL, 2 );
    else
      InvalidateCntRecord( hWndFocus, lpCntFocus, lpRec, (bSSel || bXSel) ? NULL : lpCol, 2 );

    // If new focus cell is not in view, scroll to it (if not OwnerVscroll or Delta).
/*    if( !(lpCnt->lpCI->flCntAttr & CA_OWNERVSCROLL ) )*/
    if( !(lpCnt->lpCI->flCntAttr & CA_OWNERVSCROLL) && !lpCnt->lDelta )
    {
      if (lpCnt->iView & CV_DETAIL)
      {
        if( !IsFocusInView( hWndFocus, lpCntFocus ) )
          ScrollToFocus( hWndFocus, lpCntFocus, 0 );
      }
      else if (lpCnt->iView & CV_TEXT && lpCnt->iView & CV_NAME)
      {
        if( !TxtIsFocusInView( hWndFocus, lpCntFocus ) )
          TxtScrollToFocus( hWndFocus, lpCntFocus, 0 );
      }
      else if ( lpCnt->iView & CV_ICON )
      {
        if( !IcnIsFocusInView( hWndFocus, lpCntFocus ) )
          IcnScrollToFocus( hWndFocus, lpCntFocus );
      }
    }

    // Notify parent of a new selected record if single or extended select is active.
    if( bSSel || bXSel )
      NotifyAssocWnd( hWndFocus, lpCntFocus, CN_RECSELECTED, 0, lpRec, NULL, 0, 0, 0, NULL );

    return TRUE;
    }

/****************************************************************************
 * CntTopFocusSet
 * CntBotFocusSet
 *
 * Purpose:
 *  Sets the focus on the top or bottom record in the display area.
 *  The focus field is not changed.
 *
 *  NOTE: this is NOT the top or bottom record in the record list!
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *
 * Return Value:
 *  BOOL          TRUE  if focus was set with no errors
 *                FALSE if focus could not be set to the top or bottom record
 ****************************************************************************/

BOOL WINAPI CntTopFocusSet( HWND hCntWnd )
    {
    LPCONTAINER lpCnt = GETCNT(hCntWnd);
    BOOL bRet = FALSE;

    if (lpCnt->iView & CV_ICON)
    {
      if ( lpCnt->lpCI->lpRecOrderHead )
        bRet = CntFocusSet( hCntWnd, lpCnt->lpCI->lpRecOrderHead->lpRec, NULL );
    }
    else if (lpCnt->iView & CV_DETAIL)
      bRet = CntFocusSet( hCntWnd, lpCnt->lpCI->lpTopRec, lpCnt->lpCI->lpFocusFld );
    else  
      bRet = CntFocusSet( hCntWnd, lpCnt->lpCI->lpTopRec, NULL );

    if( bRet )
      NotifyAssocWnd( hCntWnd, lpCnt, CN_NEWFOCUSREC, 0, lpCnt->lpCI->lpFocusRec, NULL, 0, 0, 0, NULL );

    return bRet;
    }

BOOL WINAPI CntBotFocusSet( HWND hCntWnd )
{
    LPCONTAINER  lpCnt = GETCNT(hCntWnd);
    LPRECORDCORE lpRec;
    BOOL         bRet=FALSE;

    if ( lpCnt->iView & CV_ICON )
    {
      // Get a pointer to the last record in the z-order
      if (lpCnt->lpCI->lpRecOrderTail)
      {
        bRet = CntFocusSet( hCntWnd, lpCnt->lpCI->lpRecOrderTail->lpRec, NULL );
      }
    }
    else if ( lpCnt->iView & CV_DETAIL )
    {
      // Get a pointer to the last visible record in the display area.
      lpRec = CntScrollRecList( hCntWnd, lpCnt->lpCI->lpTopRec, lpCnt->lpCI->nRecsDisplayed-1 );
      
      bRet = CntFocusSet( hCntWnd, lpRec, lpCnt->lpCI->lpFocusFld );
    }
    else if ( lpCnt->iView & CV_NAME || lpCnt->iView & CV_TEXT )
    {
      // Get a pointer to the last visible record in the display area. - for flowed, it will
      // set it to the bottom of the first column for now.
      lpRec = CntScrollRecList( hCntWnd, lpCnt->lpCI->lpTopRec, lpCnt->lpCI->nRecsDisplayed-1 );
      
      bRet = CntFocusSet( hCntWnd, lpRec, NULL );
    }

    if( bRet )
      NotifyAssocWnd( hCntWnd, lpCnt, CN_NEWFOCUSREC, 0, lpCnt->lpCI->lpFocusRec, NULL, 0, 0, 0, NULL );

    return bRet;
}

/****************************************************************************
 * CntFocRectEnable
 *
 * Purpose:
 *  Enables or disables the focus rectangle in the container.
 *  Under certain conditions the associate may not want the focus rectangle
 *  to appear in the container cells. Calling this function allows the
 *  associate control over whether the container draws it or not.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  BOOL          bEnable     - flag indicating whether to draw focus rectangle
 *                                if TRUE  focus rectangle will be drawn;
 *                                if FALSE focus rectangle will NOT be drawn
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI CntFocRectEnable( HWND hCntWnd, BOOL bEnable )
    {
    LPCONTAINER lpCnt = GETCNT(hCntWnd);

    lpCnt->lpCI->bDrawFocusRect = bEnable;
    }

/****************************************************************************
 * CntFocMsgEnable
 *
 * Purpose:
 *  Enables or disables focus changing notifications from the container.
 *  Under certain conditions the associate may not want to receive the 
 *  CN_SETFOCUS or CN_KILLFOCUS notifications. Calling this function allows
 *  the associate control over whether the container sends them or not.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  BOOL          bEnable     - flag indicating whether to send focus notifications
 *                                if TRUE  focus notifications will be sent;
 *                                if FALSE focus notifications will NOT be sent
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI CntFocMsgEnable( HWND hCntWnd, BOOL bEnable )
    {
    LPCONTAINER lpCnt = GETCNT(hCntWnd);

    lpCnt->lpCI->bSendFocusMsg = bEnable;
    }

/****************************************************************************
 * CntFocusScrollEnable
 *
 * Purpose:
 *  Enables or disables the scrolling action associated with the focus 
 *  rectangle in the container. This function would typically be used when
 *  the focus rectangle is disabled in the container. Normally, any keyboard
 *  action will cause the container to scroll the focus record/field into
 *  view. This 'snap-back' behavior makes sense when you have a focus rectangle
 *  but would be very confusing for the user if you weren't even displaying
 *  a focus rectangle. Calling this function with bEnable=FALSE disables
 *  this 'snap-back' behavior. It also maps the keyboard arrow keys to act
 *  like vertical and horizontal scroll commands. For example, pressing the
 *  keyboard down arrow will be the same as pressing the down arrow on the
 *  vertical scroll bar.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  BOOL          bEnable     - flag indicating whether to scroll to the
 *                              focus cell upon receiving a keyboard action:
 *                                if = TRUE  the focus cell will be scrolled into view
 *                                if = FALSE the focus cell will NOT be scrolled into view
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI CntFocusScrollEnable( HWND hCntWnd, BOOL bEnable )
    {
    LPCONTAINER lpCnt = GETCNT(hCntWnd);

    lpCnt->bScrollFocusRect = bEnable;
    }

/****************************************************************************
 * CntFocusFldGet
 * CntFocusRecGet
 *
 * Purpose:
 *  Gets the field/record which currently has the focus rect.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *
 * Return Value:
 *  LPFIELDINFO   pointer to selected field node
 *  LPRECORDCORE  pointer to selected record node
 ****************************************************************************/

LPFIELDINFO WINAPI CntFocusFldGet( HWND hCntWnd )
{
    LPCNTINFO lpCI = (GETCNT(hCntWnd))->lpCI;
    return lpCI->lpFocusFld;
}

LPRECORDCORE WINAPI CntFocusRecGet( HWND hCntWnd )
{
    LPCNTINFO lpCI = (GETCNT(hCntWnd))->lpCI;
    return lpCI->lpFocusRec;
}

/****************************************************************************
 * CntFocusFldLock
 * CntFocusRecLock
 * CntFocusFldUnlck
 * CntFocusRecUnlck
 *
 * Purpose:
 *  Locks/unlocks the field/record which currently has the focus.
 *  If the focus cell is locked the container sends keystroke notifications
 *  but does not actually move the focus. This gives the associate the
 *  ability to check out user editing info. If everything is OK then the
 *  associate moves the focus to the new position.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *
 * Return Value:
 *  int           current lock count
 ****************************************************************************/

int WINAPI CntFocusFldLock( HWND hCntWnd )
    {
    LPCNTINFO lpCI = (GETCNT(hCntWnd))->lpCI;

/*    lpCI->bFocusFldLocked = TRUE;*/
    // Increment the lock count.
    lpCI->bFocusFldLocked += 1;
    return lpCI->bFocusFldLocked;
    }

int WINAPI CntFocusRecLock( HWND hCntWnd )
    {
    LPCNTINFO lpCI = (GETCNT(hCntWnd))->lpCI;

/*    lpCI->bFocusRecLocked = TRUE;*/
    // Increment the lock count.
    lpCI->bFocusRecLocked += 1;

    return lpCI->bFocusRecLocked;
    }

int WINAPI CntFocusFldUnlck( HWND hCntWnd )
    {
    LPCNTINFO lpCI = (GETCNT(hCntWnd))->lpCI;

/*    lpCI->bFocusFldLocked = FALSE;*/

    // Decrement the lock count.
    if( lpCI->bFocusFldLocked )
      lpCI->bFocusFldLocked -= 1;

    return lpCI->bFocusFldLocked;
    }

int WINAPI CntFocusRecUnlck( HWND hCntWnd )
    {
    LPCNTINFO lpCI = (GETCNT(hCntWnd))->lpCI;

/*    lpCI->bFocusRecLocked = FALSE;*/

    // Decrement the lock count.
    if( lpCI->bFocusRecLocked )
      lpCI->bFocusRecLocked -= 1;

    return lpCI->bFocusRecLocked;
    }

/****************************************************************************
 * CntFocusOrgExGet
 *
 * Purpose:
 *  Gets the x/y origin (left/top) of the field/record cell which currently
 *  has the focus rectangle.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  LPINT         lpnXorg     - returned X coord of the cell origin
 *  LPINT         lpnYorg     - returned Y coord of the cell origin
 *  BOOL          bScreen     - flag to indicate if caller wants screen coordinates:
 *                              if TRUE  return values will be in SCREEN coordinates
 *                              if FALSE return values will be in CLIENT coordinates
 *
 * Return Value:
 *  BOOL          TRUE if function is successful; else FALSE
 ****************************************************************************/

BOOL WINAPI CntFocusOrgExGet( HWND hCntWnd, LPINT lpnXorg, LPINT lpnYorg, BOOL bScreen )
{
    LPCONTAINER  lpCnt = GETCNT(hCntWnd);
    LPCONTAINER  lpCntFrame, lpCntFocus;
    HWND         hWndFocus;
    LPFIELDINFO  lpCol;
    LPRECORDCORE lpRec;
    BOOL         bFound=FALSE;
    int          xStrCol, i;
    int          yRec=0;
    int          yTopRecArea;
    POINT        pt;

    if( !lpnXorg || !lpnYorg )
      return FALSE;

   /*-----------------------------------------------------------------------*
    *  Get the focus wnd and container struct.
    *-----------------------------------------------------------------------*/

    if( lpCnt->dwStyle & CTS_SPLITBAR && lpCnt->bIsSplit )
    {
      // Get the split window handles from the frame's stuct.
      lpCntFrame = GETCNT( lpCnt->hWndFrame );

      // See which split window has the focus.
      if( GetFocus() == lpCntFrame->SplitBar.hWndLeft )
        hWndFocus = lpCntFrame->SplitBar.hWndLeft;
      else
        hWndFocus = lpCntFrame->SplitBar.hWndRight;

      lpCntFocus = GETCNT( hWndFocus );
    }
    else
    {
      hWndFocus = lpCnt->hWnd1stChild;
      lpCntFocus = lpCnt;
    }

    if (lpCnt->iView & CV_DETAIL)
    {
      /*-----------------------------------------------------------------------*
       *  Find the beginning of the field which has the focus.
       *-----------------------------------------------------------------------*/
      // break out of the loop when we find the field which has focus
      xStrCol = -lpCntFocus->nHscrollPos * lpCnt->lpCI->cxChar;
      lpCol = lpCnt->lpCI->lpFieldHead;  // point at 1st field
      while( lpCol )
      {
        if( lpCol == lpCnt->lpCI->lpFocusFld )
          break;               
        xStrCol += lpCol->cxWidth * lpCnt->lpCI->cxChar;
        lpCol = lpCol->lpNext;
      }

      if( !lpCol )   
        xStrCol = 0;       // this shouldn't ever happen, but...

      /*-----------------------------------------------------------------------*
       *  Find the record which has the focus (must be in display area).
       *-----------------------------------------------------------------------*/

      lpRec = lpCnt->lpCI->lpTopRec;
      for( i=0; i < lpCnt->lpCI->nRecsDisplayed && lpRec; i++ )
      {
        if( lpRec == lpCnt->lpCI->lpFocusRec )
        {
          bFound = TRUE;
          break;
        }
        lpRec = lpRec->lpNext;
      }

      if( bFound )
      {
        yTopRecArea = lpCnt->lpCI->cyTitleArea + lpCnt->lpCI->cyColTitleArea;
        yRec = yTopRecArea + i * lpCnt->lpCI->cyRow;
      }
    }
    else if (lpCnt->iView & CV_ICON)
    {
      HDC   hDC;
      HFONT hFontOld;
      RECT  rcText;

      if (lpCnt->lpCI->lpFocusRec)
      {
        // get the DC
        hDC = GetDC( hCntWnd );

        // select the font into the DC so metrics are calculated accurately
        if( lpCnt->lpCI->hFont )
          hFontOld = SelectObject( hDC, lpCnt->lpCI->hFont );
        else
          hFontOld = SelectObject( hDC, GetStockObject( lpCnt->nDefaultFont ) );

        // create the text rectangle for the focus record
        CreateIconTextRect( hDC, lpCnt, lpCnt->lpCI->lpFocusRec->ptIconOrg, 
                            lpCnt->lpCI->lpFocusRec->lpszIcon, &rcText );

        // inflate the rectangle by 1 to encompass focus rect
        InflateRect( &rcText, 1, 1 );

        xStrCol = rcText.left - lpCnt->lpCI->rcViewSpace.left;
        yRec    = rcText.top  - lpCnt->lpCI->rcViewSpace.top;
      }
    }
    else if (lpCnt->iView & CV_TEXT || lpCnt->iView & CV_NAME)
    {
      LPFLOWCOLINFO lpFlowCol;
      
      if (lpCnt->iView & CV_FLOW)
      {
        xStrCol = -lpCntFocus->nHscrollPos * lpCnt->lpCI->cxChar;

        // find the column the focus record is in
        lpFlowCol = lpCnt->lpColHead;
        while (lpFlowCol)
        {
          // loop thru the records in this column looking for the focus rec
          lpRec = lpFlowCol->lpTopRec;
          for (i=0 ; i<lpFlowCol->nRecs && lpRec ; i++)
          {
            if (lpRec == lpCnt->lpCI->lpFocusRec)
            {
              bFound = TRUE;
              break;
            }
          }

          if (bFound)
            break;

          // advance to the next column
          xStrCol  += lpFlowCol->cxPxlWidth;
          lpFlowCol = lpFlowCol->lpNext;
        }

        if( bFound )
        {
          yTopRecArea = lpCnt->lpCI->cyTitleArea;
          yRec = yTopRecArea + i * lpCnt->lpCI->cyRow;
        }
      }
      else
      {
        xStrCol = -lpCntFocus->nHscrollPos * lpCnt->lpCI->cxChar;

        /*-----------------------------------------------------------------------*
         *  Find the record which has the focus (must be in display area).
         *-----------------------------------------------------------------------*/
        lpRec = lpCnt->lpCI->lpTopRec;
        for( i=0; i < lpCnt->lpCI->nRecsDisplayed && lpRec; i++ )
        {
          if( lpRec == lpCnt->lpCI->lpFocusRec )
          {
            bFound = TRUE;
            break;
          }
          lpRec = lpRec->lpNext;
        }

        if( bFound )
        {
          yTopRecArea = lpCnt->lpCI->cyTitleArea + lpCnt->lpCI->cyColTitleArea;
          yRec = yTopRecArea + i * lpCnt->lpCI->cyRow;
        }
      }
    }

    /*-----------------------------------------------------------------------*
     *  Return the top left coords of the cell which has the focus.
     *-----------------------------------------------------------------------*/
    pt.x = xStrCol;
    pt.y = yRec;

    if( bScreen )
      ClientToScreen( hWndFocus, &pt );

/*    dwPts = MAKELONG(pt.x, pt.y);*/
/*    return dwPts;*/

    *lpnXorg = pt.x;
    *lpnYorg = pt.y;
    return TRUE;
}

/****************************************************************************
 * CntFocusOrgGet
 *
 * Purpose:
 *  Gets the x/y origin (left/top) of the field/record cell which currently
 *  has the focus rectangle.
 *
 *  This function only exists for backward compatibilty.
 *  CntFocusOrgExGet should be used instead.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  BOOL          bScreen     - flag to indicate if caller wants screen coordinates:
 *                              if TRUE  return values will be in SCREEN coordinates
 *                              if FALSE return values will be in CLIENT coordinates
 *
 * Return Value:
 *  DWORD   lo-order word contains the x coord of the cell left;
 *          hi-order word contains the y coord of the cell top
 ****************************************************************************/

DWORD WINAPI CntFocusOrgGet( HWND hCntWnd, BOOL bScreen )
    {
    int xStrCol=0;
    int yRec=0;

    CntFocusOrgExGet( hCntWnd, &xStrCol, &yRec, bScreen );

    return (DWORD) MAKELONG(xStrCol, yRec);
    }

/****************************************************************************
 * CntFocusExtExGet
 *
 * Purpose:
 *  Gets the x- and y-extents of the field/record cell which currently
 *  has the focus rectangle.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  LPINT         lpnXext     - returned X extent of the focus cell
 *  LPINT         lpnYext     - returned Y extent of the focus cell
 *
 * Return Value:
 *  BOOL          TRUE if function is successful; else FALSE
 ****************************************************************************/

BOOL WINAPI CntFocusExtExGet( HWND hCntWnd, LPINT lpnXext, LPINT lpnYext )
    {
    LPCONTAINER   lpCnt = GETCNT(hCntWnd);
    LPFIELDINFO   lpCol;
    LPFLOWCOLINFO lpFlowCol;
    BOOL          bFound=FALSE;
    int           cxCol=0;
    int           cyCol=0;

    if( !lpnXext || !lpnYext )
      return FALSE;

    if (lpCnt->iView & CV_DETAIL)
    {
      // Find the field which has the focus.
      lpCol = lpCnt->lpCI->lpFieldHead;
      while( lpCol )
      {
        if( lpCol == lpCnt->lpCI->lpFocusFld )
        {
          bFound = TRUE;
          break;
        }
        lpCol = lpCol->lpNext;
      }

      if( bFound )
      {
        // Calc the length of the field which has the focus.
        cxCol = lpCol->cxWidth * lpCnt->lpCI->cxChar;
        cyCol = lpCnt->lpCI->cyRow;
      }
    }
    else if (lpCnt->iView & CV_ICON)
    {
      HDC   hDC;
      HFONT hFontOld;
      RECT  rcText;

      if (lpCnt->lpCI->lpFocusRec)
      {
        // get the DC
        hDC = GetDC( hCntWnd );

        // select the font into the DC so metrics are calculated accurately
        if( lpCnt->lpCI->hFont )
          hFontOld = SelectObject( hDC, lpCnt->lpCI->hFont );
        else
          hFontOld = SelectObject( hDC, GetStockObject( lpCnt->nDefaultFont ) );

        // create the text rectangle for the record
        CreateIconTextRect( hDC, lpCnt, lpCnt->lpCI->lpFocusRec->ptIconOrg, 
                            lpCnt->lpCI->lpFocusRec->lpszIcon, &rcText );

        // inflate the rectangle by 1 to encompass focus rect
        InflateRect( &rcText, 1, 1 );

        // calc the width and height of focus rect
        cxCol = rcText.right  - rcText.left;
        cyCol = rcText.bottom - rcText.top;

        // release the DC
        SelectObject ( hDC, hFontOld );
        ReleaseDC( hCntWnd, hDC );
      }
    }
    else if (lpCnt->iView & CV_TEXT || lpCnt->iView & CV_ICON )
    {
      int i;
      LPRECORDCORE lpRec;

      lpFlowCol = lpCnt->lpColHead;

      if ( lpFlowCol )
      {
        if ( lpCnt->iView & CV_FLOW )
        {
          // find which column the focus rect is in
          while (lpFlowCol)
          {
            // loop thru the records in the column and see if the focus rect is there
            lpRec = lpFlowCol->lpTopRec;
            for (i=0 ; i<lpFlowCol->nRecs && lpRec ; i++)
            {
              if (lpRec == lpCnt->lpCI->lpFocusRec)
              {
                bFound = TRUE;
                break;
              }
            }

            if (bFound)
              break;
            
            // advance to the next column
            lpFlowCol = lpFlowCol->lpNext;
          }

          // set the extents if focus record found
          if (lpFlowCol)
          {
            cxCol = lpFlowCol->cxPxlWidth;
            cyCol = lpCnt->lpCI->cyRow;
          }
        }
        else
        {
          // only one column in non-flowed
          cxCol = lpFlowCol->cxPxlWidth;
          cyCol = lpCnt->lpCI->cyRow;
        }
      }
    }

/*    return MAKELONG(cxCol, cyCol);*/
    *lpnXext = cxCol;
    *lpnYext = cyCol;
    return TRUE;
    }

/****************************************************************************
 * CntFocusExtGet
 *
 * Purpose:
 *  Gets the extent of the field/record cell which currently
 *  has the focus rectangle.
 *
 *  This function only exists for backward compatibilty.
 *  CntFocusExtExGet should be used instead.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *
 * Return Value:
 *  DWORD         lo-order word contains the cell width;
 *                hi-order word contains the cell height
 ****************************************************************************/

DWORD WINAPI CntFocusExtGet( HWND hCntWnd )
    {
    int cxCol=0;
    int cyCol=0;

    CntFocusExtExGet( hCntWnd, &cxCol, &cyCol );

    return (DWORD) MAKELONG(cxCol, cyCol);
    }

/****************************************************************************
 * CntFldTtlOrgExGet
 *
 * Purpose:
 *  Gets the x/y origin (left/top) of the field title area.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  LPFIELDINFO   lpFld       - pointer to the field node
 *  LPINT         lpnXorg     - returned X coord of the field title origin
 *  LPINT         lpnYorg     - returned Y coord of the field title origin
 *  BOOL          bScreen     - flag to indicate if caller wants screen coordinates:
 *                              if TRUE  return values will be in SCREEN coordinates
 *                              if FALSE return values will be in CLIENT coordinates
 *
 * Return Value:
 *  BOOL          TRUE if function is successful; else FALSE
 ****************************************************************************/

BOOL WINAPI CntFldTtlOrgExGet( HWND hCntWnd, LPFIELDINFO lpFld, 
                               LPINT lpnXorg, LPINT lpnYorg, BOOL bScreen )
    {
    LPCONTAINER lpCnt = GETCNT(hCntWnd);
    LPCONTAINER lpCntFrame, lpCntFocus;
    HWND        hWndFocus;
    LPFIELDINFO lpCol;
    int         xStrCol;
    int         yTopTtlArea;
    POINT       pt;

    if( !lpnXorg || !lpnYorg )
      return FALSE;

    // Get the focus wnd and container struct.
    if( lpCnt->dwStyle & CTS_SPLITBAR && lpCnt->bIsSplit )
      {
      // Get the split window handles from the frame's stuct.
      lpCntFrame = GETCNT( lpCnt->hWndFrame );

      // See which split window has the focus.
      if( GetFocus() == lpCntFrame->SplitBar.hWndLeft )
        hWndFocus = lpCntFrame->SplitBar.hWndLeft;
      else
        hWndFocus = lpCntFrame->SplitBar.hWndRight;

      lpCntFocus = GETCNT( hWndFocus );
      }
    else
      {
      hWndFocus = lpCnt->hWnd1stChild;
      lpCntFocus = lpCnt;
      }

    // Break out of the loop when we find the field of interest.
/*    xStrCol = -lpCnt->nHscrollPos * lpCnt->lpCI->cxChar;*/
    xStrCol = -lpCntFocus->nHscrollPos * lpCnt->lpCI->cxChar;
    lpCol = lpCnt->lpCI->lpFieldHead;  // point at 1st field
    while( lpCol )
      {
      if( lpCol == lpFld )
        break;               
      xStrCol += lpCol->cxWidth * lpCnt->lpCI->cxChar;
      lpCol = lpCol->lpNext;
      }

    if( !lpCol )   
      xStrCol = 0;       // this shouldn't ever happen, but...

    // Field title area starts right below the container title area.
    yTopTtlArea = lpCnt->lpCI->cyTitleArea;

    // Return the top left coords of this field's title area.
    pt.x = xStrCol;
    pt.y = yTopTtlArea;

    // Convert to screen coords if desired.
    if( bScreen )
      ClientToScreen(hWndFocus, &pt);

/*    dwPts = MAKELONG(pt.x, pt.y);*/
/*    return dwPts;*/

    *lpnXorg = pt.x;
    *lpnYorg = pt.y;
    return TRUE;
    }

/****************************************************************************
 * CntFldTtlOrgGet
 *
 * Purpose:
 *  Gets the x/y origin (left/top) of the field title area.
 *
 *  This function only exists for backward compatibilty.
 *  CntFldTtlOrgExGet should be used instead.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  LPFIELDINFO   lpFld       - pointer to the field node
 *  BOOL          bScreen     - flag to indicate if caller wants screen coordinates:
 *                              if TRUE  return values will be in SCREEN coordinates
 *                              if FALSE return values will be in CLIENT coordinates
 *
 * Return Value:
 *  DWORD   lo-order word contains the x coord of the field title area;
 *          hi-order word contains the y coord of the field title area
 ****************************************************************************/

DWORD WINAPI CntFldTtlOrgGet( HWND hCntWnd, LPFIELDINFO lpFld, BOOL bScreen )
    {
    int xStrCol=0;
    int yTopTtlArea=0;

    // Return the top left coords of this field's title area.
    CntFldTtlOrgExGet( hCntWnd, lpFld, &xStrCol, &yTopTtlArea, bScreen );

    return (DWORD) MAKELONG(xStrCol, yTopTtlArea);
    }

/****************************************************************************
 * CntFldEditExtExGet
 *
 * Purpose:
 *  Gets the x- and y-extents of a cell in a field. The extents are 
 *  calculated using the cxEditWidth and cyEditLines values which were 
 *  optionally specified through the CntFldDefine function.
 *  If cxEditWidth was specified it will be the returned X extent. If
 *  cxEditWidth was not specified the X extent will be the field width.
 *  If cyEditLines was specified the returned Y extent will be the row
 *  height multiplied by cyEditLines. If cyEditLines was not specified
 *  the Y extent will be equal to the row height.
 *
 *  Normally this function would be used to determine the size of an
 *  edit control to use for editing a data cell.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  LPFIELDINFO   lpFld       - pointer to the field node
 *  LPINT         lpnXext     - returned X extent of a field cell
 *  LPINT         lpnYext     - returned Y extent of a field cell
 *
 * Return Value:
 *  BOOL          TRUE if function is successful; else FALSE
 ****************************************************************************/

BOOL WINAPI CntFldEditExtExGet( HWND hCntWnd, LPFIELDINFO lpFld, LPINT lpnXext, LPINT lpnYext )
    {
    LPCONTAINER lpCnt = GETCNT(hCntWnd);
    int xWidth;
    int yLines;
    int cxEditCntl;
    int cyEditCntl;

    if( !lpnXext || !lpnYext )
      return FALSE;

    // Use the edit width if one was entered, else use field width.
    if( lpFld->xEditWidth )
      xWidth = lpFld->xEditWidth;
    else
      xWidth = lpFld->cxWidth;

    // Use the nbr of edit lines if entered, else use 1.
    if( lpFld->yEditLines )
      yLines = lpFld->yEditLines;
    else
      yLines = 1;

    // Calc the length of an edit control for this field.
/*    cxEditCntl = xWidth * lpCnt->lpCI->cxChar + lpCnt->lpCI->cxChar;*/
    cxEditCntl = xWidth * lpCnt->lpCI->cxChar;
    cyEditCntl = yLines * lpCnt->lpCI->cyRow;

/*    return MAKELONG(cxEditCntl, cyEditCntl);*/
    *lpnXext = cxEditCntl;
    *lpnYext = cyEditCntl;
    return TRUE;
    }

/****************************************************************************
 * CntFldEditExtGet
 *
 * Purpose:
 *  Gets the extent of the field/record cell for to use for editing.
 *  Normally would be used to find the size of an edit control to use
 *  for editing a data cell.
 *
 *  This function only exists for backward compatibilty.
 *  CntFldEditExtExGet should be used instead.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  LPFIELDINFO   lpFld       - pointer to the field node
 *
 * Return Value:
 *  DWORD         lo-order word contains the edit control width;
 *                hi-order word contains the edit control height
 ****************************************************************************/

DWORD WINAPI CntFldEditExtGet( HWND hCntWnd, LPFIELDINFO lpFld )
    {
    int cxEditCntl=0;
    int cyEditCntl=0;

    CntFldEditExtExGet( hCntWnd, lpFld, &cxEditCntl, &cyEditCntl );

    return (DWORD) MAKELONG(cxEditCntl, cyEditCntl);
    }

/****************************************************************************
 * CntFldWidthGet
 *
 * Purpose:
 *  Gets the width of a field in pixels or character widths.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  LPFIELDINFO   lpFld       - pointer to the field node
 *  BOOL          bPixels     - if TRUE  function returns width in pixels
 *                              if FALSE function returns width in character widths
 *
 * Return Value:
 *  int           width of field in pixels or characters
 ****************************************************************************/

int WINAPI CntFldWidthGet( HWND hCntWnd, LPFIELDINFO lpFld, BOOL bPixels )
    {
    LPCONTAINER  lpCnt = GETCNT(hCntWnd);
    int         cxField;

    // return the width of this field in pixels or characters
    if( bPixels )
      cxField = lpFld->cxWidth * lpCnt->lpCI->cxChar;
    else
      cxField = lpFld->cxWidth;

    return cxField;
    }

/****************************************************************************
 * CntFldWidthSet
 *
 * Purpose:
 *  Sets the width of a field in characters.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  LPFIELDINFO   lpFld       - pointer to the field node
 *  UINT          nWidth      - width of the field in characters 
 *
 * Return Value:
 *  int           new width of the field in pixels
 ****************************************************************************/

int WINAPI CntFldWidthSet( HWND hCntWnd, LPFIELDINFO lpFld, UINT nWidth )
{
    LPCONTAINER lpCnt = GETCNT(hCntWnd);

    if( !lpFld )
      return 0;

    // Set the new field width in characters 
    lpFld->cxWidth = nWidth;

    // Re-initialize the container metrics.
    InitTextMetrics( lpCnt->hWnd1stChild, lpCnt );

    // Notify the user of the change in field size.
    NotifyAssocWnd( lpCnt->hWnd1stChild, lpCnt, CN_FLDSIZECHANGED, 0, NULL, lpFld, 0, 0, 0, NULL );

    // Force repaint since we changed a state.
    UpdateContainer( lpCnt->hWnd1stChild, NULL, TRUE );

    // Return the new width of this field in pixels.
    return( lpFld->cxWidth * lpCnt->lpCI->cxChar );
}

/****************************************************************************
 * CntFldPxlWidthSet
 *
 * Purpose:
 *  Sets the width of a field in pixels.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  LPFIELDINFO   lpFld       - pointer to the field node
 *  UINT          nWidth      - width of the field in pixels
 *
 * Return Value:
 *  int           new width of the field in characters
 ****************************************************************************/
int WINAPI CntFldPxlWidthSet( HWND hCntWnd, LPFIELDINFO lpFld, UINT nWidth )
{
    LPCONTAINER lpCnt = GETCNT(hCntWnd);

    if( !lpFld )
      return 0;

    // width is in pixels - note that this has to be divisible by
    // the pixel width of a character.  If the width of a character
    // has not been determined yet, this will set width to zero. (don't 
    // think this will ever happen, but...)
    if (lpCnt->lpCI->cxChar)
    {
      // if not divisible by character width, round up.
      if (nWidth % lpCnt->lpCI->cxChar)
        nWidth += lpCnt->lpCI->cxChar - (nWidth % lpCnt->lpCI->cxChar);
        
      // Set the new field width in characters - the pixel width will be
      // calculated in InitTextMetrics (this will be equal to nWidth)
      lpFld->cxWidth = nWidth / lpCnt->lpCI->cxChar;
    }
    else
      lpFld->cxWidth = 0;

    // Re-initialize the container metrics.
    InitTextMetrics( lpCnt->hWnd1stChild, lpCnt );

    // Notify the user of the change in field size.
    NotifyAssocWnd( lpCnt->hWnd1stChild, lpCnt, CN_FLDSIZECHANGED, 0, NULL, lpFld, 0, 0, 0, NULL );

    // Force repaint since we changed a state.
    UpdateContainer( lpCnt->hWnd1stChild, NULL, TRUE );

    // Return the new width of this field in characters
    return lpFld->cxWidth;
}

/****************************************************************************
 * CntLastFldExpand
 *
 * Purpose:
 *  Automatically expands the width of the last container field to use up
 *  any unused background area. If there is no unused background area the
 *  width of the last field is unchanged.
 *
 * Parameters:
 *  HWND          hCntWnd      - handle to the container window
 *  int           nExtra       - extra character widths added to the field
 *                               after expansion
 * Return Value:
 *  UINT          new width of the last field in character units
 ****************************************************************************/

UINT WINAPI CntLastFldExpand( HWND hCntWnd, int nExtra )
    {
    LPCONTAINER lpCnt = GETCNT(hCntWnd);
    LPFIELDINFO lpFld = CntFldTailGet(hCntWnd);
    int nExpand=0;

    // Make sure we have some fields.
    if( !lpFld )
      return 0;

    // Dont resize if field is non-sizeable
    if ( lpFld->flColAttr & CFA_NONSIZEABLEFLD ) 
      return 0;

    // Don't do anything if there is no unused background area.
    if( lpCnt->xLastCol < lpCnt->cxClient )
      {
      // Calc how much the field needs to expand.
      nExpand = (lpCnt->cxClient - lpCnt->xLastCol) / lpCnt->lpCI->cxChar;

      // Expand the field width.
      lpFld->cxWidth += nExpand;

      // Re-initialize the container metrics.
      InitTextMetrics( lpCnt->hWnd1stChild, lpCnt );

      // Notify the user of the change in field size.
      NotifyAssocWnd( lpCnt->hWnd1stChild, lpCnt, CN_FLDSIZECHANGED, 0, NULL, lpFld, 0, 0, 0, NULL );

      // Update the container.
      UpdateContainer( lpCnt->hWnd1stChild, NULL, TRUE );
      }

    // Return the new width of the last field.
    return lpFld->cxWidth;
    }

/****************************************************************************
 * CntFldAutoSize
 *
 * Purpose:
 *  Automatically sizes the width of a field based on the widest cell
 *  in the field in all records. This function is only valid for fields
 *  of type CFT_STRING and CFT_LPSTRING. Also, this function is only valid
 *  when called after records are loaded in the container. 
 *
 * Parameters:
 *  HWND          hCntWnd      - handle to the container window
 *  LPFIELDINFO   lpFld        - pointer to the field node
 *  UINT          uMethod      - method of sizing to use on field cells
 *  int           nExtra       - extra character widths added to the field
 *  int           nMinFldWidth - minimum field width in character units 
 *                               (if -1 the field title width is used as minimum)
 *  int           nMaxFldWidth - maximum field width in character units (up to 256)
 *
 *  uMethod can be one of the following values:
 *
 *       AS_AVGCHAR  -  cell size = string characters x avg char width
 *       AS_MAXCHAR  -  cell size = string characters x max char width
 *       AS_PIXELS   -  cell size = exact string length in pixels
 *                      (not recommended for large number of records)
 *
 * Return Value:
 *  int           new width of the field in pixels
 ****************************************************************************/

int WINAPI CntFldAutoSize( HWND hCntWnd, LPFIELDINFO lpFld, UINT uMethod,
                           int nExtra, int nMinFldWidth, int nMaxFldWidth )
    {
    TEXTMETRIC tm;
    SIZE Size;
    HDC hDC;
    HFONT hFontOld;
    LPCONTAINER  lpCnt = GETCNT(hCntWnd);
    LPRECORDCORE lpRec = CntRecHeadGet( hCntWnd );
    LPSTR lpData, lpStr, lpStrCell;
    LPSTR lpDescData=NULL;
    BOOL  bDescriptor=FALSE;
    int   nMaxWidth=0;
    int   cxMaxChar;
    int   nStrLen;
    int   nFldTtlWidth;

    // Only do auto-sizing on string-type fields.
/*    if( lpFld->wColType != CFT_STRING && lpFld->wColType != CFT_LPSTRING )*/
/*      return 0;*/

    // Make sure we have some records.
/*    if( !lpRec )*/
/*      return 0;*/

    // Make sure we have a valid method.
    if( uMethod < AS_AVGCHAR || uMethod > AS_PIXELS )
      return 0;

    // Make sure the min, max, and extra parms make sense.
    if( nMinFldWidth > 255 )
      nMinFldWidth = 0;

    if( nMaxFldWidth < 1 || nMaxFldWidth <= nMinFldWidth || nMaxFldWidth > 256 )
      nMaxFldWidth = 256;

    if( nExtra < 0 )
      nExtra = 0;

    // If a descriptor is active for this field get some space for formatting.
    if( lpFld->lpDescriptor && lpFld->bDescEnabled && lstrlen(lpFld->lpDescriptor) )
      {
      bDescriptor = TRUE;
      lpDescData = (LPSTR) malloc( 1030 );
      if( !lpDescData )
        return 0;
      }

    // Select the font for the data and get the max char width.
    hDC = GetDC( hCntWnd );
    if( lpCnt->lpCI->hFont )
      hFontOld = SelectObject( hDC, lpCnt->lpCI->hFont );
    else
      hFontOld = SelectObject( hDC, GetStockObject( lpCnt->nDefaultFont ) );

    GetTextMetrics( hDC, &tm );
    cxMaxChar = tm.tmMaxCharWidth;

    // Only do auto-sizing on string-type fields.
    if( lpFld->wColType == CFT_STRING || lpFld->wColType == CFT_LPSTRING )
      {
      // Loop thru the record list measuring each cell for the specified field.
      while( lpRec )
        {
        lpData = (LPSTR)lpRec->lpRecData + lpFld->wOffStruct;    

        if( lpFld->wColType == CFT_STRING )
          {
          // We have to format the string if there is a descriptor active.
          if( bDescriptor )
            {
            wsprintf( lpDescData, lpFld->lpDescriptor, lpData ); 
            lpStrCell = lpDescData;
            }
          else    
            lpStrCell = lpData;
          }  
        else
          {  
          lpStr = *(LPSTR FAR *)lpData;  // NOTE: this could be NULL!

          // We have to format the string if there is a descriptor active.
          if( lpStr && bDescriptor )
            {
            wsprintf( lpDescData, lpFld->lpDescriptor, lpStr ); 
            lpStrCell = lpDescData;
            }
          else    
            lpStrCell = lpStr;
          }

        nStrLen  = lpStrCell ? lstrlen(lpStrCell) : 0;

        // Bump the max width if this record's string is the widest so far.
        if( uMethod == AS_AVGCHAR )
          {
          if( nStrLen > nMaxWidth )
            nMaxWidth = nStrLen;

          // Break out if we hit the maximum width.
          if( nMaxWidth+nExtra >= nMaxFldWidth )
            break;
          }
        else if( uMethod == AS_MAXCHAR )
          {
          if( nStrLen > nMaxWidth )
            nMaxWidth = nStrLen;
          }
        else
          {
          // Measure the pixel width of the string.
          if( nStrLen )
            {
            GetTextExtentPoint( hDC, lpStrCell, nStrLen, &Size );
            if( Size.cx > nMaxWidth )
              nMaxWidth = Size.cx;
            }
          }

        // Advance to the next record.
        lpRec = lpRec->lpNext;
        }
      }

    if( uMethod == AS_AVGCHAR )
      {
      // Add on the extra chars.
      nMaxWidth += nExtra;
      }
    else if( uMethod == AS_MAXCHAR )
      {
      // Add on the extra chars before converting the 
      // max char widths into avg char widths.
      nMaxWidth += nExtra;
      nMaxWidth = (nMaxWidth * cxMaxChar)/lpCnt->lpCI->cxChar + 1;
      }
    else
      {
      // Convert the longest pixel width into avg char widths
      // and then add on the extra chars.
      nMaxWidth = nMaxWidth/lpCnt->lpCI->cxChar + 1;
      nMaxWidth += nExtra;
      }

    if( nMinFldWidth >= 0 )
      {
      // Chk the widest cell width against the user defined minimum.
      nMaxWidth = max( nMinFldWidth, nMaxWidth );
      }
    else
      {
      // Chk the widest cell width against the field title width.
      nFldTtlWidth = MeasureFldTitle( hCntWnd, hDC, lpCnt, lpFld ) + nExtra + 1;
      nMaxWidth = max( nFldTtlWidth, nMaxWidth );
      }

    // Set the field width to the widest cell width (or maximum).
    lpFld->cxWidth = min( nMaxWidth, nMaxFldWidth );

    if( lpDescData )
      free( lpDescData );

    // Reset the font and release the DC.
    SelectObject( hDC, hFontOld );
    ReleaseDC( hCntWnd, hDC );

    // Re-initialize the container metrics.
    InitTextMetrics( lpCnt->hWnd1stChild, lpCnt );

    // Notify the user of the change in field size.
    NotifyAssocWnd( lpCnt->hWnd1stChild, lpCnt, CN_FLDSIZECHANGED, 0, NULL, lpFld, 0, 0, 0, NULL );

    // Update the container.
    UpdateContainer( lpCnt->hWnd1stChild, NULL, TRUE );

    // Return the new width of this field in pixels.
    return( lpFld->cxWidth * lpCnt->lpCI->cxChar );
    }

int NEAR MeasureFldTitle( HWND hWnd, HDC hDC, LPCONTAINER lpCnt, LPFIELDINFO lpCol )
    {
    HFONT hFontOld;
    RECT  rect;
    LPSTR lpStr;
    int nWidth=0;
    int uLen;
    int cxPxlBtn=0;
    int cxPxlText=0;

    // Get this field's title and length (if any).
    lpStr = (LPSTR) lpCol->lpFTitle;
    uLen  = lpStr ? lstrlen( lpStr ) : 0;

    // Measure the field title text if there is any.
    if( uLen )
      {
      // Select the font for the field titles.
      if( lpCnt->lpCI->hFontColTitle )
        hFontOld = SelectObject( hDC, lpCnt->lpCI->hFontColTitle );
      else if( lpCnt->lpCI->hFont )
        hFontOld = SelectObject( hDC, lpCnt->lpCI->hFont );
      else
        hFontOld = SelectObject( hDC, GetStockObject( lpCnt->nDefaultFont ) );

      // Calc the width of the field title text.
      SetRectEmpty( &rect );
      DrawText( hDC, lpStr, uLen, &rect, DT_CENTER | DT_TOP | DT_CALCRECT );
      cxPxlText = rect.right;  

      // Reset the font.
      SelectObject( hDC, hFontOld );
      }

    // Account for a field title button if set.
    if( lpCol->nFldBtnWidth )
      {
      // Get the field title button width in pixels.
      if( lpCol->nFldBtnWidth > 0 )
        cxPxlBtn = lpCnt->lpCI->cxCharFldTtl * lpCol->nFldBtnWidth;
      else 
        cxPxlBtn = -lpCol->nFldBtnWidth;
      }

    // Convert the field title pixel width into avg char widths.
    nWidth = (cxPxlText + cxPxlBtn) / lpCnt->lpCI->cxChar + 1;

    return nWidth;
    }

/****************************************************************************
 * CntRowHtSet
 *
 * Purpose:
 *  Sets the height of a container data row.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  int           nHeight     - row height in characters (if pos) or pixels (if neg)
 *  UINT          wLineSpace  - line space value (ignored if entering height in pixels)
 *
 * Return Value:
 *  int           row height in pixels
 ****************************************************************************/

int WINAPI CntRowHtSet( HWND hCntWnd, int nHeight, UINT wLineSpace )
    {
    LPCONTAINER  lpCnt = GETCNT(hCntWnd);

    if( !nHeight )
      nHeight = 1;

    if( nHeight < 0 )
      {
      // Set the row height in pixels.
      lpCnt->lpCI->cyRow = -nHeight;
      lpCnt->lpCI->wRowLines = 0;
      }
    else  
      {
      // Set the row height in char height of the current font.
      lpCnt->lpCI->wRowLines = nHeight;

      // Set the line spacing if they passed in a valid value.
      if( wLineSpace == CA_LS_NARROW )
        lpCnt->lpCI->wLineSpacing = CA_LS_NARROW;
      else if( wLineSpace == CA_LS_MEDIUM )
        lpCnt->lpCI->wLineSpacing = CA_LS_MEDIUM;
      else if( wLineSpace == CA_LS_WIDE )
        lpCnt->lpCI->wLineSpacing = CA_LS_WIDE;
      else if( wLineSpace == CA_LS_DOUBLE )
        lpCnt->lpCI->wLineSpacing = CA_LS_DOUBLE;
      else
        lpCnt->lpCI->wLineSpacing = CA_LS_NONE;
      }

    // Re-initialize the container metrics.
    InitTextMetrics( lpCnt->hWnd1stChild, lpCnt );
/*    InitTextMetrics( hCntWnd, lpCnt );*/

    // Force repaint since we changed a state.
    UpdateContainer( lpCnt->hWnd1stChild, NULL, TRUE );
/*    UpdateContainer( hCntWnd, NULL, TRUE );*/

    return lpCnt->lpCI->cyRow;
    }

/****************************************************************************
 * CntRowHtGet
 *
 * Purpose:
 *  Gets the height of a container row in pixels.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *
 * Return Value:
 *  int           row height in pixels
 ****************************************************************************/

int WINAPI CntRowHtGet( HWND hCntWnd )
    {
    LPCONTAINER  lpCnt = GETCNT(hCntWnd);

    return lpCnt->lpCI->cyRow;
    }

/****************************************************************************
 * CntTtlHtSet
 *
 * Purpose:
 *  Sets the height of the AREA used for the container title.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  int           nHeight     - title height in characters (if pos) or pixels (if neg)
 *
 * Return Value:
 *  int           title area height in pixels
 ****************************************************************************/

int WINAPI CntTtlHtSet( HWND hCntWnd, int nHeight )
    {
    LPCONTAINER  lpCnt = GETCNT(hCntWnd);

    if( nHeight < 0 )
      {
      lpCnt->lpCI->cyTitleArea = -nHeight;
      lpCnt->lpCI->wTitleLines = 0;
      }
    else  
      {
      lpCnt->lpCI->wTitleLines = nHeight;
      lpCnt->lpCI->cyTitleArea = 0;
      }

    // Re-initialize the container metrics.
    InitTextMetrics( lpCnt->hWnd1stChild, lpCnt );
/*    InitTextMetrics( hCntWnd, lpCnt );*/

    // Force repaint since we changed a state.
    UpdateContainer( lpCnt->hWnd1stChild, NULL, TRUE );
/*    UpdateContainer( hCntWnd, NULL, TRUE );*/

    return lpCnt->lpCI->cyTitleArea;
    }

/****************************************************************************
 * CntTtlHtGet
 *
 * Purpose:
 *  Gets the height of the AREA used for the container title.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *
 * Return Value:
 *  int           title area height in pixels
 ****************************************************************************/

int WINAPI CntTtlHtGet( HWND hCntWnd )
    {
    LPCONTAINER  lpCnt = GETCNT(hCntWnd);

    return lpCnt->lpCI->cyTitleArea;
    }

/****************************************************************************
 * CntFldTtlHtSet
 *
 * Purpose:
 *  Sets the height of the AREA used for the container field titles.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  int           nHeight     - title height in characters (if pos) or pixels (if neg)
 *
 * Return Value:
 *  int           field title area height in pixels
 ****************************************************************************/

int WINAPI CntFldTtlHtSet( HWND hCntWnd, int nHeight )
    {
    LPCONTAINER  lpCnt = GETCNT(hCntWnd);

    if( nHeight < 0 )
      {
      lpCnt->lpCI->cyColTitleArea = -nHeight;
      lpCnt->lpCI->wColTitleLines = 0;
      }
    else  
      {
      lpCnt->lpCI->wColTitleLines = nHeight;
      lpCnt->lpCI->cyColTitleArea = 0;
      }

    // Re-initialize the container metrics.
    InitTextMetrics( lpCnt->hWnd1stChild, lpCnt );
/*    InitTextMetrics( hCntWnd, lpCnt );*/

    // Force repaint since we changed a state.
    UpdateContainer( lpCnt->hWnd1stChild, NULL, TRUE );
/*    UpdateContainer( hCntWnd, NULL, TRUE );*/

    return lpCnt->lpCI->cyColTitleArea;
    }

/****************************************************************************
 * CntFldTtlHtGet
 *
 * Purpose:
 *  Gets the height of the AREA used for the container field titles.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *
 * Return Value:
 *  int           field title area height in pixels
 ****************************************************************************/

int WINAPI CntFldTtlHtGet( HWND hCntWnd )
    {
    LPCONTAINER  lpCnt = GETCNT(hCntWnd);

    return lpCnt->lpCI->cyColTitleArea;
    }

/****************************************************************************
 * CntFldBytesExGet
 *
 * Purpose:
 *  Gets the byte length used for field data and the record offset to
 *  data for this field. These are the values wOffStruct and wDataBytes
 *  entered throught the CntFldDefine function.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  LPFIELDINFO   lpFld       - pointer to the field node
 *  UINT FAR *    lpuBytes    - returned nbr of bytes used for field data
 *  UINT FAR *    lpuOffset   - returned record offset to data for this field
 *
 * Return Value:
 *  BOOL          TRUE if function is successful; else FALSE
 ****************************************************************************/

BOOL WINAPI CntFldBytesExGet( HWND hCntWnd, LPFIELDINFO lpFld, UINT FAR *lpuBytes, UINT FAR *lpuOffset )
    {
    if( !lpuBytes || !lpuOffset )
      return FALSE;

    *lpuBytes  = lpFld->wDataBytes;
    *lpuOffset = lpFld->wOffStruct;
    return TRUE;
    }

/****************************************************************************
 * CntFldBytesGet
 *
 * Purpose:
 *  Gets the byte length (used for data) of the field.
 *
 *  This function only exists for backward compatibilty. 
 *  CntFldBytesExGet should be used instead.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  LPFIELDINFO   lpFld       - pointer to the field node
 *
 * Return Value:
 *  DWORD         lo-order word= nbr of bytes used for data in this field
 *                hi-order word= offset to data for this field
 ****************************************************************************/

DWORD WINAPI CntFldBytesGet( HWND hCntWnd, LPFIELDINFO lpFld )
    {
    return (DWORD) MAKELONG(lpFld->wDataBytes, lpFld->wOffStruct);
    }

/****************************************************************************
 * CntEndRecEdit
 *
 * Purpose:
 *  A record field has been edited and now needs to be repainted.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  LPRECORDCORE  lpRecord    - pointer to Record Core node
 *  LPFIELDINFO   lpFld       - pointer to the field that was edited
 *                              (if NULL the entire record will be updated)
 *
 * Return Value:
 *  BOOL          TRUE if record was found in the display area
 *                FALSE if record was NOT found in the display area
 ****************************************************************************/

BOOL WINAPI CntEndRecEdit( HWND hCntWnd, LPRECORDCORE lpRecord, LPFIELDINFO lpFld  )
    {
    LPCONTAINER  lpCnt = GETCNT(hCntWnd);
    LPCONTAINER  lpCntFrame, lpCntFocus;
    HWND        hWndFocus;
    int         nRet;

    // Get the focus wnd and container struct.
    if( lpCnt->dwStyle & CTS_SPLITBAR && lpCnt->bIsSplit )
      {
      // Get the split window handles from the frame's stuct.
      lpCntFrame = GETCNT( lpCnt->hWndFrame );

      // See which split window has the focus.
      if( GetFocus() == lpCntFrame->SplitBar.hWndLeft )
        hWndFocus = lpCntFrame->SplitBar.hWndLeft;
      else
        hWndFocus = lpCntFrame->SplitBar.hWndRight;

      lpCntFocus = GETCNT( hWndFocus );
      }
    else
      {
      hWndFocus = lpCnt->hWnd1stChild;
      lpCntFocus = lpCnt;
      }

    nRet = InvalidateCntRecord( hWndFocus, lpCntFocus, lpRecord, lpFld, 2 );
/*    nRet = InvalidateCntRecord( hCntWnd, lpCnt, lpRecord, lpFld, 2 );*/

    return (nRet < 0) ? FALSE : TRUE;
    }

/****************************************************************************
 * CntUpdateRecArea
 *
 * Purpose:
 *  Repaints a container cell, record, field, or entire record area based
 *  on whether the record and field pointer passed in are NULL or valid.
 *
 *       lpRecord      lpFld      Action
 *      ----------   ---------    ------------------------------------
 *        valid        valid      repaints a container cell
 *        valid        NULL       repaints a container record
 *        NULL         valid      repaints a container field
 *        NULL         NULL       repaints entire container record area
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  LPRECORDCORE  lpRecord    - pointer to a record (can be NULL)
 *  LPFIELDINFO   lpFld       - pointer to a field  (can be NULL)
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI CntUpdateRecArea( HWND hCntWnd, LPRECORDCORE lpRecord, LPFIELDINFO lpFld  )
    {
    LPCONTAINER lpCnt = GETCNT(hCntWnd);
    LPCONTAINER lpCntFrame, lpCntFocus;
    HWND        hWndFocus;

    // Get the focus wnd and container struct.
    if( lpCnt->dwStyle & CTS_SPLITBAR && lpCnt->bIsSplit )
      {
      // Get the split window handles from the frame's stuct.
      lpCntFrame = GETCNT( lpCnt->hWndFrame );

      // See which split window has the focus.
      if( GetFocus() == lpCntFrame->SplitBar.hWndLeft )
        hWndFocus = lpCntFrame->SplitBar.hWndLeft;
      else
        hWndFocus = lpCntFrame->SplitBar.hWndRight;

      lpCntFocus = GETCNT( hWndFocus );
      }
    else
      {
      hWndFocus = lpCnt->hWnd1stChild;
      lpCntFocus = lpCnt;
      }

    InvalidateCntRecord( hWndFocus, lpCntFocus, lpRecord, lpFld, 2 );

    return;
    }

/****************************************************************************
 * CntDeltaExSet
 * CntDeltaExGet
 *
 * Purpose:
 *  Sets/Gets the delta value for the container. The delta value represents
 *  the maximum size of the record list in memory. This should be used
 *  when the entire record list cannot reasonably be stored in memory.
 *  If the user scrolls beyond the delta value the container will send the
 *  associate window a CN_QUERYDELTA notification to indicate that the record
 *  list needs to be loaded with records starting at the new position.
 *  The new delta position is returned from CntDeltaPosGet.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  LONG          lDelta      - delta value for max records to hold in memory
 *
 * Return Value:
 *  VOID          for CntDeltaExSet
 *  LONG          current delta value for CntDeltaExGet
 ****************************************************************************/

VOID WINAPI CntDeltaExSet( HWND hCntWnd, LONG lDelta )
    {
    LPCONTAINER lpCnt = GETCNT(hCntWnd);

    lpCnt->lDelta = lDelta;
    }

LONG WINAPI CntDeltaExGet( HWND hCntWnd )
    {
    LPCONTAINER lpCnt = GETCNT(hCntWnd);
    return lpCnt->lDelta;
    }

/****************************************************************************
 * CntDeltaPosExSet
 * CntDeltaPosExGet
 *
 * Purpose:
 *  Sets/Gets the virtual position in the entire record list of the current
 *  head record held in memory. Called by associates using the delta model
 *  to manage only part of a record list in memory at any given time.
 *
 *  The virtual delta position is where the delta calculation is tracked.
 *  For example, if you only wanted to hold 100 members of a 10,000 member
 *  record list in memory the following must be done by the associate
 *  window when initializing the container at startup:
 *
 *      1) Call CntDeltaExSet with a delta value of 100.
 *      2) Call CntDeltaPosExSet with the delta position of 0 which
 *         specifies that the head of the current record list is virtual
 *         position 0 in the entire 10,000 member record list.
 *      3) Build a record list consisting of the 1st 100 records.
 *      4) Call CntFocusSet setting the focus on the head record in the list.
 *      5) Call CntRangeExSet with 10,000 as the maximum.
 *
 *  If the user grabs the scroll thumb and scrolls down to position
 *  455 the container will send a CN_QUERYDELTA to the associate window
 *  indicating that the user scrolled beyond the current delta position.
 *  Upon receiving the CN_QUERYDELTA the associate should execute the 
 *  following actions in this order:
 *
 *      1) Call CntCurrentPosExGet to get the current position.
 *         Note that at this point the container position has not changed.
 *         The CN_QUERYDELTA tells you it is ABOUT to change.
 *      2) Call CntCNIncExGet to get the scrolling increment.
 *         This tells you how far the user has moved the thumb from its
 *         previous position and in what direction. A negative increment 
 *         indicates upward movement of the thumb. If the user had pressed
 *         the scrollbar up arrow the increment would be -1. If the down
 *         arrow was pressed it would be 1. In this example the increment
 *         would be 455 since the original position was 0.
 *      3) Add the current position and increment value to get the target
 *         position. This is the position at which to start building the 
 *         new record list (in this case it would be 455).
 *      4) Call CntKillRecList to delete the current 100 records in the list.
 *      5) Call CntDeltaPosExSet with the virtual position of the head of
 *         the new record list (in this case it would be 455).
 *
 *         IMPORTANT: The new delta position MUST be set BEFORE building
 *                    the new record list!
 *
 *      6) Build a new record list of 100 records starting with record 455.
 *
 *  The container will now track the delta value (100 records) from the new
 *  delta position (which is now 455) and will send a CN_QUERYDELTA if the
 *  user scrolls to a position before 455 or 100 records after 455.
 *  There are several variations on this scenario. For example, instead of
 *  killing the record list and changing the delta position you could simply
 *  add records to the end of the list and increase the delta value. 
 *  Another variation would be to load records ahead of the new current
 *  position so as to allow backward scrolling without immediately causing
 *  a CN_QUERYDELTA to be sent. For instance, in the above example instead
 *  of setting the delta position to 455 and loading records 455 thru 554,
 *  you could set the delta position to 405 and load records 405 thru 504,
 *  thereby creating room to scroll backward without causing a CN_QUERYDELTA.
 *
 *  It should be noted here that similar actions must also be executed
 *  upon receiving a CN_QUERYFOCUS notification. This message is only sent
 *  to applications using Owner Vscroll or Delta scrolling models because it
 *  is possible for the focus record to get 'paged' out of the record list.
 *  This message will be sent whenever the container needs to move the focus
 *  record back into view because of some keyboard action and the focus 
 *  record cannot be found among the records currently stored in memory. 
 *
 *  For example, the user sets the focus to record 238 by clicking on it.
 *  Next the user grabs the scroll thumb and scrolls down to position
 *  873. The container would have sent a CN_QUERYDELTA and a new batch of
 *  records would now be in memory (based on a delta value of 100). The
 *  focus record is now paged out of memory. Now the user presses a keyboard
 *  character or arrow key which causes the container to scroll the focus
 *  record back into view (all keyboard events affect the focus record).
 *  When the container cannot find the focus record in the record list, it
 *  will send a CN_QUERYFOCUS to the application to restore the focus record.
 *  The container expects the application to build a new record list with
 *  the restored focus record as the head. The application must also
 *  explicitly set the head of the new list as the current position and the
 *  delta position. Notice that this requires the application to have some 
 *  kind of method for keeping track of the virtual position of the focus 
 *  record so that the current position can be reset to it when responding 
 *  to a CN_QUERYFOCUS message. Upon receiving the CN_QUERYFOCUS the 
 *  associate should execute the following actions in this order:
 *
 *      1) Call CntKillRecList to delete the current 100 records in the list.
 *      2) Call CntVscrollPosExSet with the virtual position of the focus
 *         record (in this case it would be 238).
 *      3) Call CntDeltaPosExSet with the virtual position of the head of
 *         the new record list (in this case it would be 238).
 *
 *         IMPORTANT: The new delta position MUST be set BEFORE building
 *                    the new record list!
 *
 *      4) Build a new record list of 100 records starting with record 238.
 *      5) Call CntFocusSet setting the focus on the head record in the list.
 *
 *  In some cases it may not be possible or desirable to keep track of the
 *  position of the focus record. If the application does not care to track 
 *  and retreive the focus record, upon receiving a CN_QUERYFOCUS it can 
 *  simply execute the following actions:
 *
 *      1) Call CntTopRecGet to get the top record displayed in the container.
 *      2) Call CntFocusSet setting the focus to the current top record.
 *
 *  This will disable the 'snap back' behavior of the focus record and simply
 *  put the focus on the top record in view.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  LONG          lDeltaPos   - virtual starting position for delta calculation
 *
 * Return Value:
 *  VOID          for CntDeltaPosExSet;
 *  LONG          current delta position value for CntDeltaPosExGet
 ****************************************************************************/

VOID WINAPI CntDeltaPosExSet( HWND hCntWnd, LONG lDeltaPos )
    {
    LPCONTAINER lpCnt = GETCNT(hCntWnd);

    lpCnt->lDeltaPos = lDeltaPos;
    }

LONG WINAPI CntDeltaPosExGet( HWND hCntWnd )
    {
    LPCONTAINER lpCnt = GETCNT(hCntWnd);

/*    return MAKELONG(lpCnt->nVscrollPos, 0);*/
    return (LONG)lpCnt->nVscrollPos;
    }

/****************************************************************************
 * CntDeltaSet
 * CntDeltaGet
 *
 * Purpose:
 *  Sets/Gets the delta value for the container.
 *
 *  These functions only exist for backward compatibilty. 
 *  CntDeltaExSet and CntDeltaExGet should be used instead.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  int           nDelta      - delta value for max records to hold in memory
 *
 * Return Value:
 *  VOID          for CntDeltaSet
 *  int           current delta value for CntDeltaGet
 ****************************************************************************/

VOID WINAPI CntDeltaSet( HWND hCntWnd, int nDelta )
    {
    CntDeltaExSet( hCntWnd, nDelta );
    }

int WINAPI CntDeltaGet( HWND hCntWnd )
    {
    return (int)CntDeltaExGet( hCntWnd );
    }

/****************************************************************************
 * CntDeltaPosSet
 * CntDeltaPosGet
 *
 * Purpose:
 *  Sets/Gets the virtual position in the entire record list of the
 *  current head record held in memory.
 *
 *  These functions only exist for backward compatibilty. 
 *  CntDeltaPosExSet and CntDeltaPosExGet should be used instead.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  int           nDeltaPos   - delta starting position for delta calculation
 *
 * Return Value:
 *  VOID          for CntDeltaPosSet;
 *  int           current delta position value for CntDeltaPosGet
 ****************************************************************************/

VOID WINAPI CntDeltaPosSet( HWND hCntWnd, int nDeltaPos )
    {
    CntDeltaPosExSet( hCntWnd, nDeltaPos );
    }

int WINAPI CntDeltaPosGet( HWND hCntWnd )
    {
    return (int) CntDeltaPosExGet( hCntWnd );
    }

/****************************************************************************
 * CntCursorSet
 *
 * Purpose:
 *  Sets a special cursor for the container title area, field title area,
 *  or the container in general.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  HCURSOR       hCursor     - handle to the new cursor.
 *  UINT          iArea       - flag indicating which area the cursor is for
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

BOOL WINAPI CntCursorSet( HWND hCntWnd, HCURSOR hCursor, UINT iArea )
    {
    LPCNTINFO lpCI = (GETCNT(hCntWnd))->lpCI;

    if( iArea == CC_TITLE )
      {
      lpCI->hCurTtl = hCursor;       // title area cursor 
      }
    else if( iArea == CC_FLDTITLE )
      {
      lpCI->hCurFldTtl = hCursor;    // field title area cursor 
      }
    else
      {
      lpCI->hCursor = hCursor;       // general container cursor

      if( !lpCI->hCurTtl )
        lpCI->hCurTtl = hCursor; 

      if( !lpCI->hCurFldTtl )
        lpCI->hCurFldTtl = hCursor; 
      }
    return TRUE;
    }


/****************************************************************************
 * CntFldTypeGet
 *
 * Purpose:
 *  Gets the field data type MINUS all extraneous attributes so
 *  app can avoid the IF THEN ELSE stuff and use CASE statements.
 *
 * Parameters:
 *  HWND          hCntWnd     - handle to the container window
 *  LPFIELDINFO   lpFld       - pointer to the field node
 *
 * Return Value:
 *  UINT          Field type 
 ****************************************************************************/

UINT WINAPI CntFldTypeGet( HWND hCntWnd, LPFIELDINFO lpFld )
   {
   return lpFld->wColType;
   }

/****************************************************************************
 * CntNotifyAssocEx
 *
 * Purpose:
 *  Allows an application to pipe a notification message thru the container
 *  to its associate window. In most cases this would be the parent window.
 *  If there is a char, record, or field associated with the current
 *  notification it is passed in and temporarily held until the notification
 *  is processed.
 *
 * Parameter:
 *  HWND          hCntWnd     - handle to the container window
 *  UINT          wEvent      - identifier of the event.
 *  UINT          wOemCharVal - OEM value of char pressed (only for CN_CHARHIT)
 *  LPRECORDCORE  lpRec       - pointer to record of interest 
 *  LPFIELDINFO   lpFld       - pointer to field of interest 
 *  LONG          lInc        - increment for a scrolling notification
 *  BOOL          bShiftKey   - state of the shift key   (TRUE means pressed)
 *  BOOL          bCtrlKey    - state of the control key (TRUE means pressed)
 *  LPVOID        lpUserData  - pointer to user data
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI CntNotifyAssocEx( HWND hCntWnd, UINT wEvent, UINT wOemCharVal,
                              LPRECORDCORE lpRec, LPFIELDINFO lpFld, LONG lInc, 
                              BOOL bShiftKey, BOOL bCtrlKey, LPVOID lpUserData )
    {
    LPCONTAINER lpCnt=GETCNT(hCntWnd);
    LPCONTAINER lpCntFrame;
    HWND hWnd;

    // Make sure they passed in a valid container window.
    if( lpCnt )
      lpCntFrame = GETCNT( lpCnt->hWndFrame );
    else 
      return;

    if( hCntWnd == lpCnt->hWndFrame )
      {
      // If the frame was passed in change it to the child which has focus.
      if( GetFocus() == lpCntFrame->SplitBar.hWndLeft )
        hWnd = lpCntFrame->SplitBar.hWndLeft;
      else
        hWnd = lpCntFrame->hWnd1stChild;
      }
    else
      {
      // If the frame wasn't passed in make sure it is a child.
      if( hCntWnd == lpCntFrame->SplitBar.hWndLeft )
        hWnd = lpCntFrame->SplitBar.hWndLeft;
      else
        hWnd = lpCntFrame->hWnd1stChild;
      }

    // Pipe the message to the associate window. If we are async and they
    // are faking an insert or delete send it thru as a keystroke to handle
    // the scroll-to-focus and auto-record lock.

    if( (lpCnt->dwStyle & CTS_ASYNCNOTIFY) &&
        (wEvent == CN_INSERT || wEvent == CN_DELETE) )
      {    
      lpCnt->bSimulateShftKey   = TRUE;
      lpCnt->bSimulateCtrlKey   = TRUE;
      lpCnt->lpCI->bSimShiftKey = bShiftKey;
      lpCnt->lpCI->bSimCtrlKey  = bCtrlKey;

      if( wEvent == CN_INSERT )
        FORWARD_WM_KEYDOWN( hWnd, VK_INSERT, 0, 0, SendMessage );
      else
        FORWARD_WM_KEYDOWN( hWnd, VK_DELETE, 0, 0, SendMessage );

      lpCnt->bSimulateShftKey   = FALSE;
      lpCnt->bSimulateCtrlKey   = FALSE;
      lpCnt->lpCI->bSimShiftKey = FALSE;
      lpCnt->lpCI->bSimCtrlKey  = FALSE;
      }
    else  
      {
      NotifyAssocWnd( hWnd, lpCnt, wEvent, wOemCharVal, lpRec, lpFld, 
                      lInc, bShiftKey, bCtrlKey, lpUserData );
      }
    }

/****************************************************************************
 * CntNotifyAssoc
 *
 * Purpose:
 *  Allows an application to pipe a notification message thru the container
 *  to its associate window. In most cases this would be the parent window.
 *  If there is a char, record, or field associated with the current
 *  notification it is passed in and temporarily held until the notification
 *  is processed.
 *
 * Parameter:
 *  HWND          hCntWnd     - handle to the container window
 *  UINT          wEvent      - identifier of the event.
 *  UINT          wOemCharVal - OEM value of char pressed (only for CN_CHARHIT)
 *  LPRECORDCORE  lpRec       - pointer to record of interest 
 *  LPFIELDINFO   lpFld       - pointer to field of interest 
 *  int           nInc        - increment for a scrolling notification
 *  BOOL          bShiftKey   - state of the shift key   (TRUE means pressed)
 *  BOOL          bCtrlKey    - state of the control key (TRUE means pressed)
 *  LPVOID        lpUserData  - pointer to user data
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI CntNotifyAssoc( HWND hCntWnd, UINT wEvent, UINT wOemCharVal,
                            LPRECORDCORE lpRec, LPFIELDINFO lpFld, int nInc, 
                            BOOL bShiftKey, BOOL bCtrlKey, LPVOID lpUserData )
    {
    CntNotifyAssocEx( hCntWnd, wEvent, wOemCharVal, lpRec, lpFld,
                      nInc, bShiftKey, bCtrlKey, lpUserData );
    }

/****************************************************************************
 * UpdateCIptrs 
 *
 * Purpose:
 *  Internal helper function for list management procs.  Manages the data
 *  pointers held in the CI structure when records are moved.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *  LPRECORDCORE  lpRec       - pointer to Record Core node
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID NEAR UpdateCIptrs( HWND hCntWnd, LPRECORDCORE lpRec )
    {
    LPCONTAINER  lpCnt=GETCNT(hCntWnd);
    LPCNTINFO    lpCI=(GETCNT(hCntWnd))->lpCI;
    LPRECORDCORE lpMove;
    
    if( !lpRec || !lpCI )
      return;

    // First find a record to move attributes to.
    if( lpRec->lpNext == NULL )
      lpMove = lpRec->lpPrev;
    else
      lpMove = lpRec->lpNext;

    // Move the top record.
    if( lpRec == lpCI->lpTopRec )
      lpCI->lpTopRec = lpMove;

    if( lpRec == lpCI->lpFocusRec )
      {
      // If app is handling the vertical scroll, NULL out the focus.
      if( lpCI->flCntAttr & CA_OWNERVSCROLL || lpCnt->lDelta )
        {
        lpCI->lpFocusRec = NULL;
        }
      else
        {
        lpCI->lpFocusRec = lpMove;
        if( lpMove )
          lpMove->flRecAttr |= CRA_CURSORED;
        }

      // Clear the focus attribute in case they reuse this record.
      lpRec->flRecAttr &= ~CRA_CURSORED;
      }

    if( lpRec == lpCI->lpSelRec )
      {
      // If single select NULL out the selected record ptr.
      lpCI->lpSelRec = NULL;

      // Clear the selected attribute in case they reuse this record.
      lpRec->flRecAttr &= ~CRA_SELECTED;

#ifdef ZOT
      // If app is handling the vertical scroll, NULL out the selected rec.
      if( lpCI->flCntAttr & CA_OWNERVSCROLL )
        {
        lpCI->lpSelRec = NULL;
        }
      else
        {
        lpCI->lpSelRec = lpMove;
        if( lpMove )
          lpMove->flRecAttr |= CRA_SELECTED;
        }
#endif
      }
    }


/****************************************************************************
 * CntNewSelFld
 *
 * Purpose:
 *  Allocates storage for the selected field node structure type.
 *
 * Parameters:
 *  none
 *
 * Return Value:
 *  LPSELECTEDFLD pointer to allocated selected field node
 ****************************************************************************/

LPSELECTEDFLD WINAPI CntNewSelFld( void )
    {
/*    LPSELECTEDFLD lpSF = (LPSELECTEDFLD) GlobalAllocPtr(GHND, LEN_SELECTEDFLD);*/
    LPSELECTEDFLD lpSF = (LPSELECTEDFLD) calloc(1, LEN_SELECTEDFLD);

    return lpSF;
    }

/****************************************************************************
 * CntFreeSelFld
 *
 * Purpose:
 *  Frees storage for the selected field node structure type.
 *
 * Parameters:
 *  LPSELECTEDFLD lpSelField  - pointer to selected field node
 *
 * Return Value:
 *  BOOL          TRUE if storage was released; FALSE if error
 ****************************************************************************/

BOOL WINAPI CntFreeSelFld( LPSELECTEDFLD lpSelField )
    {
    if( lpSelField )
      {
      if( lpSelField->lpUserData )
/*        GlobalFreePtr(lpSelField->lpUserData);*/
        free(lpSelField->lpUserData);

/*      return GlobalFreePtr( lpSelField );*/
      free( lpSelField );
      return TRUE;
      }
    else
      {
      return FALSE; 
      }
    }

/****************************************************************************
 * CntSelFldHeadGet
 * CntSelFldTailGet
 *
 * Purpose:
 *  Retrieves the head or tail of the record's selected field list.
 *
 * Parameters:
 *  LPRECORDCORE  lpRecord    - pointer to record
 *
 * Return Value:
 *  LPSELECTEDFLD pointer to head or tail node of the list
 ****************************************************************************/

LPSELECTEDFLD WINAPI CntSelFldHeadGet( LPRECORDCORE lpRecord )
    {
    return lpRecord->lpSelFldHead;
    }

LPSELECTEDFLD WINAPI CntSelFldTailGet( LPRECORDCORE lpRecord )
    {
    return lpRecord->lpSelFldTail;
    }

/****************************************************************************
 * CntAddSelFldHead
 * CntAddSelFldTail
 *
 * Purpose:
 *  Adds a selected field node to the beginning or end of the selected 
 *  field list of the specified record.
 *
 * Parameters:
 *  LPRECORDCORE  lpRecord    - pointer to record
 *  LPSELECTEDFLD lpNew       - pointer to selected field node to add
 *
 * Return Value:
 *  BOOL          TRUE if head or tail successfully added
 ****************************************************************************/

BOOL WINAPI CntAddSelFldHead( LPRECORDCORE lpRecord, LPSELECTEDFLD lpNew )
    {
    if( !lpRecord )
      return FALSE;
 
    lpRecord->lpSelFldHead = (LPSELECTEDFLD) LLAddHead( (LPLINKNODE) lpRecord->lpSelFldHead, 
                                                   (LPLINKNODE) lpNew );
    // Update tail pointer, if necessary
    if( !lpRecord->lpSelFldTail )
      lpRecord->lpSelFldTail = lpRecord->lpSelFldHead;
 
    return lpRecord->lpSelFldHead ? TRUE : FALSE;
    }
 
BOOL WINAPI CntAddSelFldTail( LPRECORDCORE lpRecord, LPSELECTEDFLD lpNew )
    {
    if( !lpRecord )
      return FALSE;
 
    lpRecord->lpSelFldTail = (LPSELECTEDFLD) LLAddTail( (LPLINKNODE) lpRecord->lpSelFldTail, 
                                                   (LPLINKNODE) lpNew );
    // Update head pointer, if necessary
    if( !lpRecord->lpSelFldHead )
      lpRecord->lpSelFldHead = lpRecord->lpSelFldTail;
 
    return lpRecord->lpSelFldTail ? TRUE : FALSE;
    }
 
/****************************************************************************
 * CntRemSelFldHead
 * CntRemSelFldTail
 *
 * Purpose:
 *  Removes a selected field node from the beginning or end of the selected 
 *  field list of the specified record. Does not free any storage.
 *
 * Parameters:
 *  LPRECORDCORE  lpRecord    - pointer to record
 *
 * Return Value:
 *  LPSELECTEDFLD pointer to node that was removed
 ****************************************************************************/

LPSELECTEDFLD WINAPI CntRemSelFldHead( LPRECORDCORE lpRecord )
    {
    LPSELECTEDFLD lpTemp;
 
    if( !lpRecord )
      return (LPSELECTEDFLD) NULL;

    lpRecord->lpSelFldHead = (LPSELECTEDFLD) LLRemoveHead((LPLINKNODE) lpRecord->lpSelFldHead, 
                                                    ((LPLINKNODE FAR *)&lpTemp) );
    // Update tail pointer, if necessary
    if( !lpRecord->lpSelFldHead )
      lpRecord->lpSelFldTail = lpRecord->lpSelFldHead;
 
    return lpTemp;
    }

LPSELECTEDFLD WINAPI CntRemSelFldTail( LPRECORDCORE lpRecord )
    {
    LPSELECTEDFLD lpTemp;
 
    if( !lpRecord )
      return (LPSELECTEDFLD) NULL;

    lpRecord->lpSelFldTail = (LPSELECTEDFLD) LLRemoveTail((LPLINKNODE) lpRecord->lpSelFldTail, 
                                                    ((LPLINKNODE FAR *)&lpTemp) );
    // Update head pointer, if necessary
    if( !lpRecord->lpSelFldTail )
      lpRecord->lpSelFldHead = lpRecord->lpSelFldTail;
 
    return lpTemp;
    }

/****************************************************************************
 * CntRemSelFld
 *
 * Purpose:
 *  Removes a selected field node from the selected field list of the
 *  specified record. Does not free any storage.
 *
 * Parameters:
 *  LPRECORDCORE  lpRecord    - pointer to record
 *  LPSELECTEDFLD lpSelField  - pointer to selected field node to remove
 *
 * Return Value:
 *  LPSELECTEDFLD pointer to node that was removed
 ****************************************************************************/

LPSELECTEDFLD WINAPI CntRemSelFld( LPRECORDCORE lpRecord, LPSELECTEDFLD lpSelField )
    {
    LPSELECTEDFLD lpTemp;
 
    if( !lpRecord )
      return (LPSELECTEDFLD) NULL;

    // Need to check for three cases here:
    //      Removing a head node- call LLRemoveHead
    //      Removing a tail node- call LLRemoveTail
    //      Removing an internal node - call LLRemoveNode
 
    if( lpSelField == lpRecord->lpSelFldHead )   // Current node is head
      {
      lpRecord->lpSelFldHead = (LPSELECTEDFLD) 
         LLRemoveHead( (LPLINKNODE) lpRecord->lpSelFldHead, ((LPLINKNODE FAR *)&lpTemp) );
      if( !lpRecord->lpSelFldHead )
        lpRecord->lpSelFldTail = lpRecord->lpSelFldHead;
      }
    else if( lpSelField == lpRecord->lpSelFldTail ) // Current node is tail
      {
      lpRecord->lpSelFldTail = (LPSELECTEDFLD) 
         LLRemoveTail( (LPLINKNODE) lpRecord->lpSelFldTail, ((LPLINKNODE FAR *)&lpTemp) );
      if( !lpRecord->lpSelFldTail )
        lpRecord->lpSelFldHead = lpRecord->lpSelFldTail;
      }
    else  // current node is internal
      {
      lpTemp = (LPSELECTEDFLD) LLRemoveNode( (LPLINKNODE) lpSelField );
      }

    return lpTemp;
    }

/****************************************************************************
 * CntInsSelFldBfor
 * CntInsSelFldAftr
 *
 * Purpose:
 *  Adds a selected field node to the selected field list of the specified
 *  record, before or after the given node.
 *
 * Parameters:
 *  LPRECORDCORE  lpRecord    - pointer to record
 *  LPSELECTEDFLD lpSelField  - pointer to existing node to insert before or after
 *  LPSELECTEDFLD lpNew       - pointer to new node to insert.
 *
 * Return Value:
 *  BOOL          TRUE if node was successfully added to list
 ****************************************************************************/

BOOL WINAPI CntInsSelFldBfor( LPRECORDCORE lpRecord, LPSELECTEDFLD lpSelField, LPSELECTEDFLD lpNew )
    {
    if( !lpRecord || !lpSelField || !lpNew )
      return FALSE;
 
    lpRecord->lpSelFldHead = (LPSELECTEDFLD) LLInsertBefore( (LPLINKNODE) lpSelField, 
                                                         (LPLINKNODE) lpNew );
    // Update tail pointer, if necessary
    if( !lpRecord->lpSelFldTail )
      lpRecord->lpSelFldTail = lpRecord->lpSelFldHead;
 
    return lpRecord->lpSelFldHead ? TRUE : FALSE;
    }

BOOL WINAPI CntInsSelFldAftr( LPRECORDCORE lpRecord, LPSELECTEDFLD lpSelField, LPSELECTEDFLD lpNew )
    {
    if( !lpRecord || !lpSelField || !lpNew )
      return FALSE;
 
    lpRecord->lpSelFldHead = (LPSELECTEDFLD) LLInsertAfter( (LPLINKNODE) lpSelField, 
                                                        (LPLINKNODE) lpNew );
    // Update tail pointer, if necessary
    if( !lpRecord->lpSelFldTail )
      lpRecord->lpSelFldTail = lpRecord->lpSelFldHead;
 
    // Check for hosed tail
    if( lpRecord->lpSelFldTail->lpNext )
      lpRecord->lpSelFldTail = lpRecord->lpSelFldTail->lpNext;
   
    return lpRecord->lpSelFldHead ? TRUE : FALSE;
    }

/****************************************************************************
 * CntKillSelFldLst
 *
 * Purpose:
 *  Destroys a record's selected field list.
 *  All memory for the list is also freed.
 *
 * Parameters:
 *  LPRECORDCORE  lpRecord    - pointer to record
 *
 * Return Value:
 *  BOOL          TRUE if list was successfully destroyed
 ****************************************************************************/

BOOL WINAPI CntKillSelFldLst( LPRECORDCORE lpRecord )
    {
    LPSELECTEDFLD lpTemp;

    if( !lpRecord )
      return FALSE;
   
    while( lpRecord->lpSelFldHead )
      {
      lpRecord->lpSelFldHead = (LPSELECTEDFLD) 
         LLRemoveHead( (LPLINKNODE) lpRecord->lpSelFldHead, ((LPLINKNODE FAR *)&lpTemp) );
      if( lpTemp )
        CntFreeSelFld( lpTemp );
      }

    lpRecord->lpSelFldTail = NULL;

    return TRUE;
    }

/****************************************************************************
 * CntNextSelFld
 * CntPrevSelFld
 *
 * Purpose:
 *  Gets the next or previous selected field node in the list.
 *
 * Parameters:
 *  LPSELECTEDFLD lpSelField  - pointer to node to start from
 *
 * Return Value:
 *  LPSELECTEDFLD pointer to next or previous node
 ****************************************************************************/

LPSELECTEDFLD WINAPI CntNextSelFld( LPSELECTEDFLD lpSelField )
    {
    if( lpSelField )
      return (LPSELECTEDFLD) NEXT(lpSelField);
    else
      return NULL;
    }

LPSELECTEDFLD WINAPI CntPrevSelFld( LPSELECTEDFLD lpSelField )
    {
    if( lpSelField )
      return (LPSELECTEDFLD) PREV(lpSelField);
    else
      return NULL;
    }

/****************************************************************************
 * CntFindSelFld
 *
 * Purpose:
 *  Looks for a container field in a record's selected field list.
 *
 * Parameters:
 *  LPRECORDCORE  lpRecord    - pointer to record
 *  LPSELECTEDFLD lpField     - pointer to field
 *
 * Return Value:
 *  LPSELECTEDFLD pointer to selected field node if found else NULL
 ****************************************************************************/

LPSELECTEDFLD WINAPI CntFindSelFld( LPRECORDCORE lpRecord, LPFIELDINFO lpField )
    {
    LPSELECTEDFLD lpSelFldFound=NULL;
    LPSELECTEDFLD lpSelFld;
 
    // Check out the selected field list for this record.
    lpSelFld = lpRecord->lpSelFldHead;
    while( lpSelFld )
      {
      // break out if we find the field
      if( lpSelFld->lpFld == lpField )
        {
        lpSelFldFound = lpSelFld;
        break;  
        }
      lpSelFld = lpSelFld->lpNext;
      }

    return lpSelFldFound;
    }

/****************************************************************************
 * CntSelFldUserSet
 * CntSelFldUserGet
 *
 * Purpose:
 *  Loads/Gets the user data for a selected field.
 *
 * Parameters:
 *  LPSELECTEDFLD lpSF        - pointer to selected field
 *  LPVOID        lpUserData  - user data
 *  UINT          wUserBytes  - number of bytes to copy from lpUserData
 *
 * Return Value:
 *  BOOL          TRUE: success;  FALSE: error
 ****************************************************************************/

BOOL WINAPI CntSelFldUserSet( LPSELECTEDFLD lpSF, LPVOID lpUserData, UINT wUserBytes )
    {
    if( !lpSF || !lpUserData || !wUserBytes )
      return FALSE;

    // Free old user data if there was any.
    if( lpSF->lpUserData )
      {
/*      GlobalFreePtr(lpSF->lpUserData);*/
      free(lpSF->lpUserData);
      lpSF->lpUserData = NULL;
      }

    // Alloc space for the new user data.
/*    lpSF->lpUserData = (LPVOID) GlobalAllocPtr(GHND, wUserBytes);*/
    lpSF->lpUserData = (LPVOID) calloc(1, wUserBytes);
    lpSF->wUserBytes = lpSF->lpUserData ? wUserBytes : 0;

    if( lpSF->lpUserData )
      {
      _fmemcpy( (LPVOID) lpSF->lpUserData, lpUserData, (size_t) lpSF->wUserBytes);
      return TRUE;
      }
    else
      return FALSE;
    }

LPVOID WINAPI CntSelFldUserGet( LPSELECTEDFLD lpSelFld )
    {
    return lpSelFld->lpUserData;       // user defined field data
    }

/****************************************************************************
 * CntNameSetEx
 *
 * Purpose:
 *  Set the names of a record for Text view, Name View, and Icon View.  Send 
 *  in NULL for a parameter you do not wish to set.
 *
 * Parameters:
 *  LPRECORDCORE  lpRC     - RECORDCORE Structure
 *  LPSTR         lpszText - Text View name of the record
 *  LPSTR         lpszName - Name View name of the record
 *  LPSTR         lpszIcon - Icon View name of the record
 *
 ****************************************************************************/
BOOL WINAPI CntNameSetEx( LPRECORDCORE lpRC, LPSTR lpszText, LPSTR lpszName, LPSTR lpszIcon )
{
   if (lpRC)
   {
     // set the text for Text View
     if (lpszText)
     {
       // Check for existing text view name.
       if ( lpRC->lpszText )
         free( lpRC->lpszText );

       // Allocate storage and set the name
       lpRC->lpszText = (LPSTR) calloc( 1, lstrlen(lpszText) + 1 );
       lstrcpy( lpRC->lpszText, lpszText );
     }

     // set the text for Name View
     if (lpszName)
     {
       // Check for existing name view name.
       if ( lpRC->lpszName )
         free( lpRC->lpszName );

       // Allocate storage and set the name
       lpRC->lpszName = (LPSTR) calloc( 1, lstrlen(lpszName) + 1 );
       lstrcpy( lpRC->lpszName, lpszName );
     }

     // set the text for Icon View
     if (lpszIcon)
     {
       // Check for existing icon view name.
       if ( lpRC->lpszIcon )
         free( lpRC->lpszIcon );

       // Allocate storage and set the name
       lpRC->lpszIcon = (LPSTR) calloc( 1, lstrlen(lpszIcon) + 1 );
       lstrcpy( lpRC->lpszIcon, lpszIcon );
     }

     return TRUE;
   }

   return FALSE;
}

/****************************************************************************
 * CntRecIconSetEx
 *
 * Purpose:
 *  Set the icons or bitmaps of a record for Name View and Icon View.  
 *  If the user wishes the image to be transparent (i.e. one of the 
 *  colors in the bitmap represents a transparent color), set the x and
 *  y value of the pixel in the bitmap that represents the transparent color.
 *  This is only applicable to bitmaps since icons can be drawn with a
 *  built in transparency color.
 *
 * Parameters:
 *  LPRECORDCORE  lpRC        - RECORDCORE Structure
 *  HANDLE        hImage      - handle to icon or bitmap.
 *  int           fImageType  - Type of image (icon or bitmap, mini or normal)
 *  BOOL          fUseTransp  - flag whether using transparency or not.
 *  int           xTrans      - x-value of the pixel that represents the transparency color.
 *  int           yTrans      - y-value of the pixel that represents the transparency color.
 *
 ****************************************************************************/
BOOL WINAPI CntRecIconSetEx(LPRECORDCORE lpRC, HANDLE hImage, int fImageType, BOOL fUseTransp, 
                            int xTrans, int yTrans)
{
   if (lpRC && hImage)
   {
     // allocate structure if using transparency
     if (!lpRC->lpTransp  && fUseTransp)
       lpRC->lpTransp = (LPRECTRANSPINFO) calloc(1, LEN_RECTRANSPINFO);

     // determine the type of image and set it
     switch (fImageType)
     {
       case CNTIMAGE_ICON:
         // set the Normal sized icon
//         if (lpRC->hIcon)
//           DestroyIcon(lpRC->hIcon);
     
         // set the handle
         lpRC->hIcon = (HICON)hImage;
     
         // set transparency flag
         if (fUseTransp)
         {
           lpRC->lpTransp->fIconTransp = TRUE;
           lpRC->lpTransp->ptIcon.x = xTrans;
           lpRC->lpTransp->ptIcon.y = yTrans;
         }
         else if (lpRC->lpTransp)
           lpRC->lpTransp->fIconTransp = FALSE;
         break;

       case CNTIMAGE_MINIICON:
         // set the mini sized icon
//         if (lpRC->hMiniIcon)
//           DestroyIcon(lpRC->hMiniIcon);
     
         // set the handle
         lpRC->hMiniIcon = (HICON)hImage;
     
         // set transparency flag
         if (fUseTransp)
         {
           lpRC->lpTransp->fMiniIconTransp = TRUE;
           lpRC->lpTransp->ptMiniIcon.x = xTrans;
           lpRC->lpTransp->ptMiniIcon.y = yTrans;
         }
         else if (lpRC->lpTransp)
           lpRC->lpTransp->fMiniIconTransp = FALSE;
         break;

       case CNTIMAGE_BMP:
         // set the Normal sized Bitmap
//         if (lpRC->hBmp)
//           DeleteObject(lpRC->hBmp);
     
         // set the handle
         lpRC->hBmp = (HBITMAP)hImage;
     
         // set transparency flag
         if (fUseTransp)
         {
           lpRC->lpTransp->fBmpTransp = TRUE;
           lpRC->lpTransp->ptBmp.x = xTrans;
           lpRC->lpTransp->ptBmp.y = yTrans;
         }
         else if (lpRC->lpTransp)
           lpRC->lpTransp->fBmpTransp = FALSE;
         break;

       case CNTIMAGE_MINIBMP:   
         // set the Mini sized Bitmap
//         if (lpRC->hMiniBmp)
//           DeleteObject(lpRC->hMiniBmp);
     
         // set the handle
         lpRC->hMiniBmp = (HBITMAP)hImage;

         // set transparency flag
         if (fUseTransp)
         {
           lpRC->lpTransp->fMiniBmpTransp = TRUE;
           lpRC->lpTransp->ptMiniBmp.x = xTrans;
           lpRC->lpTransp->ptMiniBmp.y = yTrans;
         }
         else if (lpRC->lpTransp)
           lpRC->lpTransp->fMiniBmpTransp = FALSE;
         break;
     }
     
     return TRUE;
   }

   return FALSE;
}

/****************************************************************************
 * CntSetFlowedHorSpacing
 *
 * Purpose:
 *  Used when the container view is set to CV_FLOW.  Sets a width
 *  used to space the columns apart in flowed view.  If nWidth is negative
 *  it denotes pixels; if it is positive, it denotes characters.  This width 
 *  will be added to the width of each column (the width of the widest item 
 *  in the column).
 *
 * Parameters:
 *  HWND  hCnt   - handle to the container
 *  int   nWidth - width 
 *
 ****************************************************************************/
BOOL WINAPI CntSetFlowedHorSpacing( HWND hCntWnd, int nWidth )
{
    LPCNTINFO lpCI = (GETCNT(hCntWnd))->lpCI;

    lpCI->cxFlowSpacing = nWidth;
    return TRUE;
}

/****************************************************************************
 * CntIconArrangeAttrSet
 * CntIconArrangeAttrGet
 *
 * Purpose:
 *  Sets or gets the attributes for arranging icons in icon view.
 *
 * Parameters:
 *  HWND hCnt       - handle to the container
 *  int  aiType     - method of arranging icons - the possible ways follow:
 *    
 *    CAI_FIXED      - Icons are arranged with a fixed width and height specified
 *                     by the nHeight and nWidth parameters.  Note that the text is
 *                     not constrained to these dimensions - this is for merely
 *                     arranging the icons.  Therefore, if the width or height is
 *                     too small, overlap will occur.
 *    CAI_FIXEDWIDTH - Icons are arranged with a fixed width - height is determined
 *                     by the string with the most lines of text
 *    CAI_LARGEST    - width and height of the icon space are determined by the largest
 *                     icon string in the record list
 *    CAI_WORDWRAP   - can be combined with CAI_FIXED or CAI_FIXEDWIDTH - causes
 *                     automatic word wrapping to keep the text constrained to the width.
 *
 *  int  nWidth      - width of icon space in pixels - applicable in FIXED and FIXEDWIDTH
 *  int  nHeight     - height of icon space in pixels - applicable in FIXED
 *  int  nHorSpace   - horizontal spacing in pixels between records when arranged
 *  int  nVerSpace   - vertical spacing in pixels between records when arranged
 *  int  xIndent     - space indented in pixels from left of window when arranged
 *  int  yIndent     - space indented in pixels from left of window when arranged
 *
 ****************************************************************************/
BOOL WINAPI CntIconArrangeAttrSet( HWND hCntWnd, int aiType, int nWidth, int nHeight,
                                   int nHorSpace, int nVerSpace, int xIndent, int yIndent )
{
    LPCNTINFO lpCI = (GETCNT(hCntWnd))->lpCI;

    // set the parameters
    lpCI->aiInfo.aiType      = aiType;
    lpCI->aiInfo.nHeight     = nHeight;
    lpCI->aiInfo.nWidth      = nWidth;
    lpCI->aiInfo.nHorSpacing = nHorSpace;
    lpCI->aiInfo.nVerSpacing = nVerSpace;
    lpCI->aiInfo.xIndent     = xIndent;
    lpCI->aiInfo.yIndent     = yIndent;

    // cannot have CAI_LARGEST and CAI_WORDWRAP combined
    if ((lpCI->aiInfo.aiType & CAI_LARGEST) && (lpCI->aiInfo.aiType & CAI_WORDWRAP))
      lpCI->aiInfo.aiType &= ~CAI_WORDWRAP;

    return TRUE;
}

BOOL WINAPI CntIconArrangeAttrGet( HWND hCntWnd, int *aiType, int *nWidth, int *nHeight,
                                   int *nHorSpace, int *nVerSpace, int *xIndent, int *yIndent )
{
    LPCNTINFO lpCI = (GETCNT(hCntWnd))->lpCI;

    // get the parameters
    if (aiType)
      *aiType    = lpCI->aiInfo.aiType;

    // get the width
    if (aiType && nWidth)
    {
      if ((*aiType & CAI_FIXED) || (*aiType & CAI_FIXEDWIDTH))
        *nWidth    = lpCI->aiInfo.nWidth;
      else
        *nWidth = 0;
    }

    // get the height
    if (aiType && nHeight)
    {
      if (*aiType & CAI_FIXED)
        *nHeight   = lpCI->aiInfo.nHeight;
      else
        *nHeight = 0;
    }

    // get the spacing and indents
    if (aiType)
    {
      if (*aiType == 0)
      {
        if (nHorSpace)
          *nHorSpace = 0;
        if (nVerSpace)
          *nVerSpace = 0;
        if (xIndent)
          *xIndent   = 0;
        if (yIndent)
          *yIndent   = 0;
      }
      else
      {
        if (nHorSpace)
          *nHorSpace = lpCI->aiInfo.nHorSpacing;
        if (nVerSpace)
          *nVerSpace = lpCI->aiInfo.nVerSpacing;
        if (xIndent)
          *xIndent   = lpCI->aiInfo.xIndent;
        if (yIndent)
          *yIndent   = lpCI->aiInfo.yIndent;
      }
    }

    return TRUE;
}

/****************************************************************************
 * CntArrangeIcons
 *
 * Purpose:
 *  Arranges the records for icon view - if in icon view, repaints the container
 *
 * Parameters:
 *  HWND hCnt       - handle to the container
 *
 ****************************************************************************/
BOOL WINAPI CntArrangeIcons( HWND hCntWnd )
{
    LPCONTAINER  lpCnt = GETCNT(hCntWnd);
    LPCNTINFO    lpCI = (GETCNT(hCntWnd))->lpCI;
    int          nWidth=0, nHeight=0, nOffset;
    int          iRow=0, iCol=0;
    LPRECORDCORE lpRec;
    RECT         rcCalc;
    HDC          hDC;
    HFONT        hFontOld;
    int          xIndent, yIndent;

    // get the DC of the container
    hDC = GetDC( hCntWnd );

    // Select font into the DC
    if( lpCnt->lpCI->hFont )
      hFontOld = SelectObject( hDC, lpCI->hFont );
    else
      hFontOld = SelectObject( hDC, GetStockObject( lpCnt->nDefaultFont ) );

    // figure out the width and the height of the icon space
    if ( (lpCI->aiInfo.aiType==0)            ||
         (lpCI->aiInfo.aiType & CAI_LARGEST) ||  
         (lpCI->aiInfo.aiType & CAI_FIXEDWIDTH) )
    {
      // if no arrange method has been specified, default is CAI_LARGEST
      // figure out the widest string and the highest string in the list (only highest
      // if CAI_FIXEDWIDTH)
      SetRectEmpty( &rcCalc );
      lpRec = lpCI->lpRecHead;
      while (lpRec)
      {
        if ( lpRec->lpszIcon )
        {
          // calc the width and height of the icon text
          if (lpCnt->lpCI->aiInfo.aiType & CAI_WORDWRAP) // automatic word wrapping
            DrawText( hDC, lpRec->lpszIcon, lstrlen(lpRec->lpszIcon), &rcCalc, 
                      DT_NOPREFIX | DT_TOP | DT_CENTER | DT_WORDBREAK | DT_CALCRECT );
          else
            DrawText( hDC, lpRec->lpszIcon, lstrlen(lpRec->lpszIcon), &rcCalc, 
                      DT_NOPREFIX | DT_TOP | DT_CENTER | DT_CALCRECT );
  
          // see if the width is the greatest so far
          if (rcCalc.right > nWidth && !(lpCI->aiInfo.aiType & CAI_FIXEDWIDTH))
            nWidth = rcCalc.right;

          // see if the height is the greatest so far
          if (rcCalc.bottom > nHeight)
            nHeight = rcCalc.bottom;
        }

        // advance to the next record
        lpRec = lpRec->lpNext;
      }

      // set the width for fixed width or, if CAI_LARGEST, make width at least the width 
      // of the icon
      if (lpCI->aiInfo.aiType & CAI_FIXEDWIDTH)
        nWidth = lpCI->aiInfo.nWidth;
      else 
      {
        if (nWidth < lpCI->ptBmpOrIcon.x)
          nWidth = lpCI->ptBmpOrIcon.x;
      }

      // add the height of the icon to the height
      nHeight += lpCI->ptBmpOrIcon.y;
    }
    else if (lpCI->aiInfo.aiType & CAI_FIXED)
    {
      // width and height are fixed
      nWidth  = lpCI->aiInfo.nWidth;
      nHeight = lpCI->aiInfo.nHeight;
    }
    else 
    {
      SelectObject ( hDC, hFontOld );
      ReleaseDC( hCntWnd, hDC );
      return FALSE;
    }

    // Release the DC.
    SelectObject ( hDC, hFontOld );
    ReleaseDC( hCntWnd, hDC );

    // set the indents from the upper left corner of the workspace
    xIndent = lpCI->aiInfo.xIndent;
    yIndent = lpCI->aiInfo.yIndent;
    
    // set the offset - the point of origin of the record is the upper left of the icon
    // so we need to offset it from the upper left of the icon space
    nOffset = (nWidth - lpCI->ptBmpOrIcon.x) / 2;
    
    // set the points of origin for all the records in the list.  The point of origin
    // is defined as the upper left point of the icon.  Note that as of now, the
    // records are arranged in the order of the record linked list
    lpRec = lpCI->lpRecHead;
    while (lpRec)
    {
      // set the point of origin for the record
      lpRec->ptIconOrg.x = xIndent + iCol + nOffset;
      lpRec->ptIconOrg.y = yIndent + iRow;

      // go right and check to see if the next record will be off the right side 
      // of the container
      iCol += (nWidth + lpCI->aiInfo.nHorSpacing);
      if (iCol + nWidth >= lpCnt->cxClient)
      {
        // set the right of the icon workspace
        if (iRow==0)
          lpCI->rcWorkSpace.right = xIndent + iCol - lpCI->aiInfo.nHorSpacing;

        // go to the next row
        iCol = 0;
        iRow += (nHeight + lpCI->aiInfo.nVerSpacing);
      }

      // advance to the next record
      lpRec = lpRec->lpNext;
    }
    
    // set the icon workspace
    lpCI->rcWorkSpace.top    = xIndent;
    lpCI->rcWorkSpace.left   = yIndent;
    if (iCol==0)
      lpCI->rcWorkSpace.bottom = yIndent + iRow - lpCI->aiInfo.nVerSpacing;
    else
      lpCI->rcWorkSpace.bottom = yIndent + iRow + nHeight;
    
    // reset the viewspace
    lpCI->rcViewSpace.top    = 0;
    lpCI->rcViewSpace.bottom = lpCnt->cyClient - lpCnt->lpCI->cyTitleArea;
    lpCI->rcViewSpace.left   = 0;
    lpCI->rcViewSpace.right  = lpCnt->cxClient;

    // save off the parameters so we can scroll by nice even increments
    lpCI->bArranged = TRUE;
    lpCI->aiInfo.nWidth  = nWidth;
    lpCI->aiInfo.nHeight = nHeight;
    
    // repaint the container if in icon view
    if ( lpCnt->iView & CV_ICON)
    {
      if ( lpCnt->dwStyle & CTS_VERTSCROLL )
        AdjustVertScroll( lpCnt->hWnd1stChild, lpCnt, TRUE );

      if ( lpCnt->dwStyle & CTS_HORZSCROLL )
        IcnAdjustHorzScroll( lpCnt->hWnd1stChild, lpCnt );
      
      InvalidateRect( hCntWnd, NULL, TRUE );
      UpdateContainer( hCntWnd, NULL, TRUE );
    }
    
    return TRUE;
}

/****************************************************************************
 * CntDragInitInfoGet
 *
 * Purpose:
 *  Get the cntdraginitinfo structure
 *
 * Parameters:
 *  HWND          hCntWnd  - handle to container
 *
 * Return Value:
 *   pointer to the cntdraginit structure.  (Can be NULL)
 ****************************************************************************/
LPCNTDRAGINIT WINAPI CntDragInitInfoGet( HWND hCntWnd)
{
  LPDRGINPROG lpDrgTemp = NULL;
  LPCNTDRAGINIT lpDragInit = NULL;
  LPCONTAINER lpCnt=GETCNT(hCntWnd);

  // use current index to get the correct draginprog struct
  if(lpCnt->lpDrgInProg != NULL)  
    {
      lpDrgTemp = FindDrgInfoByIndex(hCntWnd,lpCnt->nDrgop);
      if(lpDrgTemp != NULL)
        lpDragInit = lpDrgTemp->lpDragInit;
    }

 return(lpDragInit);
}
/****************************************************************************
 * CntDragIndexGet
 *
 * Purpose:
 *  Returns the index for the drag info structure that was used for a drop
 *  notification.  
 *
 *  Applications make this call during the CN_DROPCOMPLETE processing to
 *  get the index to the drag info structure that was used during this 
 *  drag and drop information.
 *  This index is then used to retrieve the drag info structure that was
 *  set with the CntDragInfoSet call.  The drag info structure should then
 *  be freed.
 *
 * Parameters:
 *  HWND          hCntWnd  - handle to container
 *
 * Return Value:
 *   index of the drag info structure.
 ****************************************************************************/
int WINAPI CntDragIndexGet(HWND hCntWnd)
{
  
  LPCONTAINER lpCnt = GETCNT(hCntWnd);

  return(lpCnt->nDropComplete);
}

/****************************************************************************
 * CntDragIndexSet
 *
 * Purpose:
 *   This function is used to set the drag info structure into the correct
 *   drag inprogress structure.
 *
 *  Applications make this call during CN_DRAGINIT processing to
 *  set the drag info structure that is to be used during the drag and drop
 *  operation.
 *
 * Parameters:
 *  HWND          hCntWnd    - handle to container
 *  LPCDRAGINFO   lpDragInfo - pointer to drag info structure
 *
 * Return Value:
 *   index of the drag info structure.
 ****************************************************************************/
int WINAPI CntDragInfoSet(HWND hCntWnd, LPCDRAGINFO lpDragInfo)
{
  int nIndex = -1;
  LPDRGINPROG lpDrgTemp = NULL;
  LPCONTAINER lpCnt=GETCNT(hCntWnd);
 
  // use current index to get the correct draginprog struct
  lpDrgTemp = FindDrgInfoByIndex(hCntWnd,lpCnt->nDrgop);
  if(lpDrgTemp != NULL)
    {
      lpDrgTemp->lpDragInfo = lpDragInfo;
      nIndex = lpDrgTemp->nIndex;
    }

return(nIndex);
}
/****************************************************************************
 * CntDragIndexGet
 *
 * Purpose:
 *   This function is used to get the drag info structure 
 *
 *  Applications make this call during CN_DROPCOMPLETE after the CntDragIndexGet
 *  call.  This will return the pointer to the drag info structure that was set
 *  with the call CntDragInfoSet.
 *
 * Parameters:
 *  HWND          hCntWnd    - handle to container
 *  LPCDRAGINFO   lpDragInfo - pointer to drag info structure
 *
 * Return Value:
 *   Pointer to draginfo structure or NULL
 ****************************************************************************/

LPCDRAGINFO WINAPI CntDragInfoGet(HWND hCntWnd,int nIndex)
{
  LPCDRAGINFO lpcDragTemp = NULL;
  LPDRGINPROG lpDrgTemp = NULL;
  LPCONTAINER lpCnt=GETCNT(hCntWnd);
    
  if(nIndex > -1)
    lpDrgTemp = FindDrgInfoByIndex(hCntWnd,nIndex);

  if(lpDrgTemp != NULL)
    lpcDragTemp = lpDrgTemp->lpDragInfo;    

  return(lpcDragTemp);
}


/****************************************************************************
 * CntDragTargetInfoGet
 *
 * Purpose:
 *   This function is used to get the drag info structure during a CN_DRAGOVER
 *   or CN_DROP notification.
 *
 *  Applications make this call during CN_DRAGOVER or CN_DROP notifications to
 *  retrieve the drag info structure that was received during a WM_CNTDRAGOVER,
 *  or WM_CNTDROP messages.
 *
 * Parameters:
 *  HWND          hCntWnd    - handle to container
 *
 * Return Value:
 *   Pointer to draginfo structure or NULL
 ****************************************************************************/
LPCDRAGINFO WINAPI CntDragTargetInfoGet(HWND hCntWnd)
{
  LPCONTAINER lpCnt=GETCNT(hCntWnd);

  return(lpCnt->lpTargetInfo);
}

/****************************************************************************
 * CntDragTransferInfoGet
 *
 * Purpose:
 *   This function is used to get the drag info structure during a CN_RENDERDATA
 *   notification.
 *
 *  Applications make this call during CN_RENDERDATA notifications to
 *  retrieve the dragtransferinfo structure that was received during a WM_CNTRENDERDATA,
 *  message.
 *
 * Parameters:
 *  HWND          hCntWnd    - handle to container
 *
 * Return Value:
 *   Pointer to dragtransferinfo structure or NULL
 ****************************************************************************/
LPCNTDRAGTRANSFER WINAPI CntDragTransferInfoGet(HWND hCntWnd)
{
  LPCONTAINER lpCnt=GETCNT(hCntWnd);

  return(lpCnt->lpDragTransfer);
}

/****************************************************************************
 * CntRenderCompleteInfoGet
 *
 * Purpose:
 *   This function is used to get the dragtransfer info structure during a 
 *   CN_RENDERDATACOMPLETE notification.
 *
 *  Applications make this call during CN_RENDERDATACOMPLETE notifications to
 *  retrieve the dragtransferinfo structure that was received during a 
 *  WM_CNTRENDERDATACOMPLETEmessage.
 *
 * Parameters:
 *  HWND          hCntWnd    - handle to container
 *
 * Return Value:
 *   Pointer to dragtransferinfo structure or NULL
 ****************************************************************************/
LPCNTDRAGTRANSFER WINAPI CntRenderCompleteInfoGet(HWND hCntWnd)
{
  LPCONTAINER lpCnt=GETCNT(hCntWnd);

  return(lpCnt->lpRenderComplete);

}
/****************************************************************************
 * CntDragOverRecGet
 *
 * Purpose:
 *   This function is used to get the lprecordcore where the dragover occurred 
 *   during the WM_CNTDRAGOVER message notification.
 *
 *  Applications make this call during CN_DRAGOVER notification to get the
 *  lprecordcore where the dragover occurred.
 *
 * Parameters:
 *  HWND          hCntWnd    - handle to container
 *
 * Return Value:
 *   Pointer to lprecordcore or NULL
 ****************************************************************************/
LPRECORDCORE WINAPI CntDragOverRecGet(HWND hCntWnd)
{
  LPCONTAINER lpCnt=GETCNT(hCntWnd);

  return(lpCnt->lpDragOverRec);
}
/****************************************************************************
 * CntDropOverRecGet
 *
 * Purpose:
 *   This function is used to get the lprecordcore where the drop occurred 
 *   during the WM_CNTDROP message notification.
 *
 *  Applications make this call during CN_DROP notification to get the
 *  lprecordcore where the drop occurred.
 *
 * Parameters:
 *  HWND          hCntWnd    - handle to container
 *
 * Return Value:
 *   Pointer to lprecordcore or NULL
 ****************************************************************************/
LPRECORDCORE WINAPI CntDropOverRecGet(HWND hCntWnd)
{
  LPCONTAINER lpCnt=GETCNT(hCntWnd);

  return(lpCnt->lpDropOverRec);
}

/****************************************************************************
 * CntSetDragCursors
 *
 * Purpose:
 *  Set cursors to be used for drag and drop operation
 *
 * Parameters:
 *  HWND          hCntWnd         - handle to container
 *  HCURSOR       hDropNotAllowed - Cursor to be displayed over window were a drop
 *                                  cannot be performed.
 *  HCURSOR       hSingleItem     - Cursor to be displayed when a single item is being
 *                                  dragged.
 *  HCURSOR       hMultiItem      - Cursor to be displayed when multiple item are
 *                                  being dragged.
 *
 * Return Value:  None
 *                
 ****************************************************************************/
VOID WINAPI CntSetDragCursors(HWND hCntWnd, HCURSOR hDropNotAllowed, HCURSOR hCopySingle, 
                              HCURSOR hCopyMulti, HCURSOR hMoveSingle, HCURSOR hMoveMulti)
{
  LPCONTAINER lpCnt=GETCNT(hCntWnd);

  if(hDropNotAllowed)
    lpCnt->lpCI->hDropNotAllow = hDropNotAllowed;

  if(hCopySingle)
    lpCnt->lpCI->hDropCopySingle = hCopySingle;

  if(hCopyMulti)
    lpCnt->lpCI->hDropCopyMulti = hCopyMulti;

  if(hMoveSingle)
    lpCnt->lpCI->hDropMoveSingle = hMoveSingle;

  if(hMoveMulti)
    lpCnt->lpCI->hDropMoveMulti = hMoveMulti;

}


/****************************************************************************
 * CntSelRecCountGet
 *
 * Purpose:
 *   This function is used to get the number of records that are marked selected
 *
 *  Applications make this to retrieve the number of records that are marked
 *  selected.
 
 *
 * Parameters:
 *  HWND          hCntWnd    - handle to container
 *
 * Return Value:
 *   number of selected records or 0.
 ****************************************************************************/
DWORD WINAPI CntSelRecCountGet(HWND hCntWnd)
{
  DWORD dwCount = 0;
  LPRECORDCORE lpRec;
  LPCONTAINER lpCnt=GETCNT(hCntWnd);

  lpRec = CntRecHeadGet( hCntWnd );        
  if(lpRec != NULL)
    {          
      while(lpRec != NULL)  
        {
          if( lpRec->flRecAttr & CRA_SELECTED || lpRec->flRecAttr & CRA_FLDSELECTED )
            dwCount++;  
     
      
          lpRec = lpRec->lpNext;  
        }
    }

return(dwCount);


}
/****************************************************************************
 * CntAllocDragInfo
 *
 * Purpose:
 *   This function is used to Allocate a block of storage that is big
 *   enough to hold 1 draginfo structure and the specified number of
 *   DRAGITEM structures
 *
 *  Applications make this to allocate memory at the start of a drag and drop
 *  operation.
 *
 * Parameters:
 *  HWND          hCntWnd    - handle to container
 *  DWORD         dwItems    - number of items being dragged
 *
 * Return Value:
 *   LPVOID - pointer to starting address of memory or NULL
 ****************************************************************************/
LPVOID WINAPI CntAllocDragInfo(HWND hCntWnd, DWORD dwItems )
{
  DWORD dwDataSize;
  LPCONTAINER lpCnt=GETCNT(hCntWnd);

  dwDataSize = LEN_DRAGINFO + (dwItems * LEN_DRAGITEM);
  return( calloc(1,(size_t) dwDataSize) );

}

/****************************************************************************
 * CntDragItemGet
 *
 * Purpose:
 *   This function is used to fetch the specified drag item structure
 *
 *  Applications make this to get a pointer to the desirec drag item
 *
 * Parameters:
 *  HWND          hCntWnd    - handle to container
 *  LPDRAGINFO    lpDragInfo - pointer to the draginfo structure
 *  DWORD         dwItems    - number of items being dragged
 *
 * Return Value:
 *   LPDRAGITEM - pointer to a dragitem structure or NULL
 ****************************************************************************/
LPDRAGITEM WINAPI CntDragItemGet(HWND hCntWnd,LPCDRAGINFO lpDragInfo, DWORD dwItem)
{
  DWORD dwCnt = 0;
  LPDRAGITEM lpDragItem = NULL;
  LPCONTAINER lpCnt=GETCNT(hCntWnd);


  if(lpDragInfo != NULL)
    {
      // get pointer to first drag item struct
      lpDragItem = (LPDRAGITEM) ((char FAR *)lpDragInfo + LEN_DRAGINFO);
  
      if(dwItem > 0)
        {
          while(dwCnt != dwItem)
            {
              lpDragItem++;
              dwCnt++;
            }
        }
    }

  return( lpDragItem );

}

/****************************************************************************
 * CntSetMsgValue
 *
 * Purpose:
 *  This function allows the user to return a value to the container after 
 *  processing a notification message from the container.  This message is
 *  only valid if the app was notified via NotifyAssocWndEx().  Currently these
 *  messages include Drag and Drop msgs and CN_DONTSETFOCUSREC.
 *
 * Parameters:
 *  HWND          hCntWnd - handle to container
 *  LRESULT       lVal    - Value the user is setting
 *
 * Return Value:
 *   None.
 ****************************************************************************/
VOID WINAPI CntSetMsgValue( HWND hCntWnd, LRESULT lVal )
{
    LPCONTAINER lpCnt = GETCNT(hCntWnd);

    lpCnt->lReturnMsgVal = lVal;
}

/****************************************************************************
 * CntScrollWithNoBarHorzEx
 *
 * Purpose:
 *   This function allows the user to scroll the container horizontally
 *   with no existing scroll bar.  This is an undocumented API which is used
 *   by Unicenter Console app.  This could hose the container in a few 
 *   different ways.  I have not investigated this as of yet but I assume that
 *   setting an HscrollMax with no scroll bar could possibly cause funky
 *   behavior (for example, arrow keys will scroll window when reach the end -
 *   this is not bad but it is different than how the container performs 
 *   without a scrollbar)
 *
 * Parameters:
 *  HWND          hCntWnd    - handle to container
 *  int           nScrollPos - new ScrollPosition
 *
 * Return Value:
 *   TRUE if the window can be scrolled by the increment, otherwise FALSE.
 ****************************************************************************/
BOOL WINAPI CntScrollWithNoBarHorzEx( HWND hCntWnd, int nScrollPos )
{
    LPCNTINFO   lpCI = (GETCNT(hCntWnd))->lpCI;
    LPCONTAINER lpCnt = GETCNT(hCntWnd);
    RECT        rect;
    int         nHscrollInc;

    // Calculate the horizontal scrolling max
    if( lpCnt->lpCI->nMaxWidth - lpCnt->cxClient > 0 )
      lpCnt->nHscrollMax = max(0, (lpCnt->lpCI->nMaxWidth - lpCnt->cxClient - 1) / lpCnt->lpCI->cxChar + 1);
    else
      lpCnt->nHscrollMax = 0;
    lpCnt->nHscrollPos = min(lpCnt->nHscrollPos, lpCnt->nHscrollMax);
    
    // Adjust the increment 
    nHscrollInc = nScrollPos - lpCnt->nHscrollPos;
    nHscrollInc = max(-lpCnt->nHscrollPos, min(nHscrollInc, lpCnt->nHscrollMax - lpCnt->nHscrollPos));

    // Scroll the record area of the container.
    if( nHscrollInc )
    {
      // Set the new horizontal position if not owner vscroll.
      lpCnt->nHscrollPos += nHscrollInc;

      // Set the scrolling rect and execute the scroll.
      rect.left   = 0;
      rect.top    = lpCI->cyTitleArea;
//      rect.right  = min( lpCnt->cxClient, lpCnt->xLastCol);
      rect.right  = lpCnt->cxClient;
      rect.bottom = lpCnt->cyClient;
      ScrollWindow( lpCnt->hWnd1stChild, -lpCnt->lpCI->cxChar * nHscrollInc, 0, NULL, &rect );

      UpdateWindow( lpCnt->hWnd1stChild );
    }
    else
      return FALSE;

    return TRUE;
}

/****************************************************************************
 * CntCNInputIDGet
 *
 * Purpose:
 *  This function is used by the application to query in which msg certain
 *  notifications were sent.  As of now, this is only valid if the notification
 *  was sent within the WM_LBUTTONDOWN msg or the WM_KEYDOWN msg.  This allows
 *  the application to tell if a RECSELECTED msg was generated by a mouse
 *  click or a keystroke, for example.
 *
 * Parameters:
 *  HWND          hCntWnd    - handle to container
 *
 * Return Value:
 *   the message ID
 ****************************************************************************/
int WINAPI CntCNInputIDGet( HWND hCntWnd )
{
    LPCONTAINER lpCnt = GETCNT(hCntWnd);

    return lpCnt->iInputID;
}

/****************************************************************************
 * CntGetRecordData
 *
 * Purpose:
 *  Extracts and formats record data for a specified field.  Returns data in string
 *  format in lpStrData.
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

UINT WINAPI CntGetRecordData( HWND hWndCnt, LPRECORDCORE lpRec, LPFIELDINFO lpField, 
                              UINT wMaxLen, LPSTR lpStrData )
{
    UINT iLen;
    
    iLen = GetRecordData( hWndCnt, lpRec, lpField, wMaxLen, lpStrData );
    
    return iLen;
}
