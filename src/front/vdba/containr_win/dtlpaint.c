/****************************************************************************
 * Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * DTLPAINT.C
 *
 * Contains WM_PAINT logic for the DETAIL view format.
 ****************************************************************************/
#include <windows.h>
#include "cntdll.h"

/****************************************************************************
 * PaintDetailView
 *
 * Purpose:
 *  Paints the container in a detail view format.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window.
 *  LPCONTAINER   lpCnt       - pointer to container's CONTAINER structure.
 *  HDC           hDC         - container window DC
 *  LPPAINTSTRUCT lpPs        - pointer to paint struct
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI PaintDetailView( HWND hWnd, LPCONTAINER lpCnt, HDC hDC, LPPAINTSTRUCT lpPs )
    {
    LPFIELDINFO  lpCol;          // pointer to FIELDINFO list
    LPRECORDCORE lpRec;          // pointer to RECORDCORE list
    LPRECT   lpRect;
    LPSTR    lpStr;
    LPSTR    lpData;
    char     szData[MAX_STRING_LEN];
    RECT     CntClientRect;
    RECT     rectFocus, textRect, bkgdRect;
    UINT     uLen;
    UINT     fAlign;
    UINT     fEtoOp;
    BOOL     bCntHasFocus;
    BOOL     bFoundFocFld;
    BOOL     bBtnPressed;
    BOOL     bRecSelected, bFldSelected, bBlkSelect, bRecInUse;
    int      xPos, yPos, yPos2, xOff, yOff;
    int      xOrgBmp, yOrgBmp, xExt, yExt;
    int      xTrnPxl, yTrnPxl;
    int      cxChar, cxChar2, cxChar4;
    int      cyChar, cyChar2;
    int      cyRow,  cyRow2,  cyRow4;
    int      cxCharTtl, cxCharTtl2;
    int      cyCharTtl, cyCharTtl2, cyCharTtl4;
    int      cxCharFldTtl, cyCharFldTtl, cyCharFldTtl2, cyCharFldTtl4;
    int      cyTitleArea=0;
    int      cyColTitleArea=0;
    int      cxCol, xLastCol, cxWidth, cxPxlWidth;
    int      cyLineOff;
    int      nPaintBegRec, nPaintEndRec, i, xStr, xEnd;

    // Only draw the focus rect if the container has focus and the user
    // has the focus rect enabled.
    if( StateTest( lpCnt, CNTSTATE_HASFOCUS ) && lpCnt->lpCI->bDrawFocusRect )
      bCntHasFocus = TRUE;
    else
      bCntHasFocus = FALSE;

   /*-----------------------------------------------------------------------*
    * Set some local character metrics.
    *-----------------------------------------------------------------------*/

    // set whole, half, and quarter char and row dimensions
    cxChar  = lpCnt->lpCI->cxChar;      // width of gen font char  
    cxChar2 = lpCnt->lpCI->cxChar2;     // half width of gen font char
    cxChar4 = lpCnt->lpCI->cxChar4;     // quarter width of gen font char
    cyChar  = lpCnt->lpCI->cyChar;      // height of gen font char
    cyChar2 = lpCnt->lpCI->cyChar2;     // half height of gen font char
    cyRow   = lpCnt->lpCI->cyRow;       // quarter height of record row
    cyRow2  = lpCnt->lpCI->cyRow2;      // half height of record row
    cyRow4  = lpCnt->lpCI->cyRow4;      // quarter height of record row

    // widths and heights of title font char
    cxCharTtl  = lpCnt->lpCI->cxCharTtl;
    cxCharTtl2 = lpCnt->lpCI->cxCharTtl2;
    cyCharTtl  = lpCnt->lpCI->cyCharTtl;
    cyCharTtl2 = lpCnt->lpCI->cyCharTtl2;
    cyCharTtl4 = lpCnt->lpCI->cyCharTtl4;

    // heights of field titles font char
    cxCharFldTtl  = lpCnt->lpCI->cxCharFldTtl;
    cyCharFldTtl  = lpCnt->lpCI->cyCharFldTtl;
    cyCharFldTtl2 = lpCnt->lpCI->cyCharFldTtl2;
    cyCharFldTtl4 = lpCnt->lpCI->cyCharFldTtl4;

    // Space required for the container title and column titles.
    cyTitleArea    = lpCnt->lpCI->cyTitleArea;
    cyColTitleArea = lpCnt->lpCI->cyColTitleArea;

   /*-----------------------------------------------------------------------*
    * Draw the container background.
    *-----------------------------------------------------------------------*/

    // There is nothing to the right of this value.
    xLastCol = min(lpCnt->cxClient, lpCnt->lpCI->nMaxWidth - lpCnt->nHscrollPos * cxChar);

    CntClientRect.left   = 0;
    CntClientRect.top    = 0;
    CntClientRect.right  = lpCnt->cxClient;
    CntClientRect.bottom = lpCnt->cyClient;
    lpRect = &CntClientRect;

    // Repaint the invalidated background.
    // (if we are not simply updating a button)
    if( !EqualRect( &lpPs->rcPaint, &lpCnt->rectPressedBtn ) )
      {
      SetBkColor( hDC, lpCnt->crColor[CNTCOLOR_CNTBKGD] );

      if( lpCnt->lpCI->flCntAttr & CA_OWNERPNTBK && lpCnt->lpfnPaintBk )
        {
        // Call the app to paint the background if owner-paint is enabled.
        (*lpCnt->lpfnPaintBk)( hWnd, NULL, hDC, &lpPs->rcPaint, BK_GENERAL );
        }
      else
        {
        ExtTextOut( hDC, lpPs->rcPaint.left, lpPs->rcPaint.top, ETO_OPAQUE, &lpPs->rcPaint, NULL, 0, NULL );
        }

      // Fill in unused space to the right of the last field if any is exposed.
      if( xLastCol < lpCnt->cxClient && lpPs->rcPaint.right > xLastCol )
        {
        bkgdRect.left   = max(xLastCol, lpPs->rcPaint.left);
        bkgdRect.top    = max(0, lpPs->rcPaint.top);
        bkgdRect.right  = min(lpCnt->cxClient, lpPs->rcPaint.right);
        bkgdRect.bottom = min(lpCnt->cyClient, lpPs->rcPaint.bottom);
        SetBkColor( hDC, lpCnt->crColor[CNTCOLOR_UNUSEDBKGD] );

        if( lpCnt->lpCI->flCntAttr & CA_OWNERPNTUNBK && lpCnt->lpfnPaintBk )
          {
          // Call the app to paint the background if owner-paint is enabled.
          (*lpCnt->lpfnPaintBk)( hWnd, NULL, hDC, &bkgdRect, BK_UNUSED );
          }
        else
          {
          ExtTextOut( hDC, bkgdRect.left, bkgdRect.top, ETO_OPAQUE, &bkgdRect, NULL, 0, NULL );
          }
        }
      }

   /*-----------------------------------------------------------------------*
    * Draw the container title (if it has one).
    *-----------------------------------------------------------------------*/

    // Only redraw the title if it is invalidated.
    if( cyTitleArea && lpPs->rcPaint.top < cyTitleArea )
      {
      uLen = lpCnt->lpCI->lpszTitle ? lstrlen( lpCnt->lpCI->lpszTitle ) : 0;

      // Select the font for the title.
      if( lpCnt->lpCI->hFontTitle )
        SelectObject( hDC, lpCnt->lpCI->hFontTitle );
      else if( lpCnt->lpCI->hFont )
        SelectObject( hDC, lpCnt->lpCI->hFont );
      else
        SelectObject( hDC, GetStockObject( lpCnt->nDefaultFont ) );

      // Adjust the title rectangle if a title button is set.
      if( lpCnt->lpCI->nTtlBtnWidth )
        {
        // Get the title button width in pixels.
        if( lpCnt->lpCI->nTtlBtnWidth > 0 )
          cxPxlWidth = cxCharTtl * lpCnt->lpCI->nTtlBtnWidth;
        else
          cxPxlWidth = -lpCnt->lpCI->nTtlBtnWidth;

        // Make room for the button in the title area.
        if( lpCnt->lpCI->bTtlBtnAlignRt )
          lpRect->right -= cxPxlWidth;
        else
          lpRect->left  += cxPxlWidth;
        }

      // Set the rectangle for the title alignment.
      lpRect->bottom = cyTitleArea;
      CopyRect( &textRect, lpRect );
      textRect.left   += cxCharTtl2;
      textRect.right  -= cxCharTtl2;

      // setup the horz title alignment
      fAlign = DT_NOPREFIX;
      if( lpCnt->lpCI->flTitleAlign & CA_TA_HCENTER )
        fAlign |= DT_CENTER;
      else if( lpCnt->lpCI->flTitleAlign & CA_TA_RIGHT )
        fAlign |= DT_RIGHT;
      else
        fAlign |= DT_LEFT;

      // setup the vert title alignment
      if( lpCnt->lpCI->flTitleAlign & CA_TA_VCENTER )
        {
        fAlign |= DT_VCENTER | DT_SINGLELINE;
        textRect.top -= (lpCnt->lpCI->flCntAttr & CA_TITLE3D) ? 1 : 0;
        }
      else if( lpCnt->lpCI->flTitleAlign & CA_TA_BOTTOM )
        {
        fAlign |= DT_BOTTOM | DT_SINGLELINE;
        textRect.top    -= (lpCnt->lpCI->flCntAttr & CA_TITLE3D) ? 1 : 0;
        textRect.bottom -= cyCharTtl4+1;
        }
      else
        {
        fAlign |= DT_TOP;
        textRect.top    += 1;
        textRect.bottom -= cyCharTtl4+1;
        }

      // Fill the title background.
      SetBkColor( hDC, lpCnt->crColor[CNTCOLOR_TTLBKGD] );
      if( lpCnt->lpCI->flCntAttr & CA_OWNERPNTTTLBK && lpCnt->lpfnPaintBk )
        {
        // Call the app to paint the title background if owner-paint is enabled.
        (*lpCnt->lpfnPaintBk)( hWnd, NULL, hDC, lpRect, BK_TITLE );
        }
      else
        {
        ExtTextOut( hDC, 0, 0, ETO_OPAQUE, lpRect, NULL, 0, (LPINT)NULL );
        }

      // Draw the title.
      if( uLen )
        {
        SetBkMode( hDC, TRANSPARENT );
        SetTextColor( hDC, lpCnt->crColor[CNTCOLOR_TITLE] );
        DrawText( hDC, lpCnt->lpCI->lpszTitle, uLen, &textRect, fAlign );
        }

      // Draw a bitmap with the title if one is defined.
      if( lpCnt->lpCI->hBmpTtl )
        {
        // Get the bitmap extents.
        GetBitmapExt( hDC, lpCnt->lpCI->hBmpTtl, &xExt, &yExt );

        // setup the horizontal alignment
        cxWidth = lpRect->right - lpRect->left + 1;

        if( lpCnt->lpCI->flTtlBmpAlign & CA_TA_HCENTER )
          xOrgBmp = lpRect->left + cxWidth/2 - xExt/2;
        else if( lpCnt->lpCI->flTtlBmpAlign & CA_TA_RIGHT )
          xOrgBmp = lpRect->right - xExt - cxCharTtl2;
        else
          xOrgBmp = lpRect->left + cxCharTtl2;
  
        // setup the vertical alignment
        if( lpCnt->lpCI->flTtlBmpAlign & CA_TA_VCENTER )
          yOrgBmp = cyTitleArea/2 - yExt/2;
        else if( lpCnt->lpCI->flTtlBmpAlign & CA_TA_BOTTOM )
          yOrgBmp = cyTitleArea - yExt - cyCharTtl4;
        else
          yOrgBmp = cyCharTtl4;

        // Apply any offset.
        xOrgBmp += lpCnt->lpCI->xOffTtlBmp;
        yOrgBmp += lpCnt->lpCI->yOffTtlBmp;

        if( lpCnt->lpCI->flCntAttr & CA_TRANTTLBMP )
          {
          xTrnPxl = lpCnt->lpCI->xTPxlTtlBmp;
          yTrnPxl = lpCnt->lpCI->yTPxlTtlBmp;
          DrawTransparentBitmap( hDC, lpCnt->lpCI->hBmpTtl, xOrgBmp, yOrgBmp, 
                                 xTrnPxl, yTrnPxl, lpRect );
          }
        else
          {
          DrawBitmap( hDC, lpCnt->lpCI->hBmpTtl, xOrgBmp, yOrgBmp, lpRect );
          }
        }

      // Add 3d texturing around the title if enabled.
      if( lpCnt->lpCI->flCntAttr & CA_TITLE3D )
        {
        Draw3DFrame( hDC, lpCnt->hPen[CNTCOLOR_3DHIGH], lpCnt->hPen[CNTCOLOR_3DSHADOW], lpCnt->hPen[CNTCOLOR_GRID],
                     lpRect->left-1, lpRect->top-1,
                     lpRect->right, lpRect->bottom-1, FALSE );
        }

     /*-------------------------------------------------------------------*
      * Draw the title button if there is one.
      *-------------------------------------------------------------------*/

      if( lpCnt->lpCI->nTtlBtnWidth )
        {
        // Setup the title button rectangle.
        if( lpCnt->lpCI->bTtlBtnAlignRt )
          {
          lpRect->left  = lpRect->right+1;
          lpRect->right = lpCnt->cxClient;
          }
        else
          {
          lpRect->right = lpRect->left-1;
          lpRect->left  = 0;
          }

        // Set the pressed flag.
        bBtnPressed = lpCnt->lpCI->flCntAttr & CA_TTLBTNPRESSED ? TRUE : FALSE;

        // Fill the title button background.
        SetBkColor( hDC, lpCnt->crColor[CNTCOLOR_TTLBTNBKGD] );
        SetTextColor( hDC, lpCnt->crColor[CNTCOLOR_TTLBTNTXT] );
        ExtTextOut( hDC, 0, 0, ETO_OPAQUE, lpRect, NULL, 0, (LPINT)NULL );

        // Draw the button text if there is any.
        uLen = lpCnt->lpCI->szTtlBtnTxt ? lstrlen( lpCnt->lpCI->szTtlBtnTxt ) : 0;
        if( uLen )
          {
          // Set the rectangle for the text alignment.
          CopyRect( &textRect, lpRect );
          textRect.left   += cxCharTtl2;
          textRect.right  -= cxCharTtl2;

          // setup the horz alignment
          fAlign = DT_NOPREFIX;
          if( lpCnt->lpCI->flTtlBtnTxtAlign & CA_TA_HCENTER )
            fAlign |= DT_CENTER;
          else if( lpCnt->lpCI->flTtlBtnTxtAlign & CA_TA_RIGHT )
            fAlign |= DT_RIGHT;
          else
            fAlign |= DT_LEFT;

          // setup the vert alignment
          if( lpCnt->lpCI->flTtlBtnTxtAlign & CA_TA_VCENTER )
            {
            fAlign |= DT_VCENTER | DT_SINGLELINE;
            textRect.top -= 1;
            }
          else if( lpCnt->lpCI->flTtlBtnTxtAlign & CA_TA_BOTTOM )
            {
            fAlign |= DT_BOTTOM | DT_SINGLELINE;
            textRect.top    -= 1;
            textRect.bottom -= cyCharTtl4+1;
            }
          else
            {
            fAlign |= DT_TOP;
            textRect.top    += 1;
            textRect.bottom -= cyCharTtl4+1;
            }

          // Offset the text to simulate button pressing.
          textRect.top    += bBtnPressed;
          textRect.left   += bBtnPressed;
          textRect.bottom += bBtnPressed;
          textRect.right  += bBtnPressed;

          // Draw the button text.
          DrawText( hDC, lpCnt->lpCI->szTtlBtnTxt, uLen, &textRect, fAlign );
          }

        // Draw a bitmap on the button face if one is defined.
        if( lpCnt->lpCI->hBmpTtlBtn )
          {
          // Get the bitmap extents.
          GetBitmapExt( hDC, lpCnt->lpCI->hBmpTtlBtn, &xExt, &yExt );
      
          // setup the horizontal alignment
          cxWidth = lpRect->right - lpRect->left + 1;
          if( lpCnt->lpCI->flTtlBtnBmpAlign & CA_TA_HCENTER )
            xOrgBmp = lpRect->left + cxWidth/2 - xExt/2;
          else if( lpCnt->lpCI->flTtlBtnBmpAlign & CA_TA_RIGHT )
            xOrgBmp = lpRect->right - xExt - cxCharTtl2;
          else
            xOrgBmp = lpRect->left + cxCharTtl2;
        
          // setup the vertical alignment
          if( lpCnt->lpCI->flTtlBtnBmpAlign & CA_TA_VCENTER )
            yOrgBmp = cyTitleArea/2 - yExt/2;
          else if( lpCnt->lpCI->flTtlBtnBmpAlign & CA_TA_BOTTOM )
            yOrgBmp = cyTitleArea - yExt - cyCharTtl4;
          else
            yOrgBmp = cyCharTtl4;
      
          // Apply any offset.
          xOrgBmp += lpCnt->lpCI->xOffTtlBtnBmp;
          yOrgBmp += lpCnt->lpCI->yOffTtlBtnBmp;
      
          // Offset the bitmap to simulate button pressing.
          xOrgBmp += bBtnPressed;
          yOrgBmp += bBtnPressed;

          if( lpCnt->lpCI->flCntAttr & CA_TRANTTLBTNBMP )
            {
            xTrnPxl = lpCnt->lpCI->xTPxlTtlBtnBmp;
            yTrnPxl = lpCnt->lpCI->yTPxlTtlBtnBmp;
            DrawTransparentBitmap( hDC, lpCnt->lpCI->hBmpTtlBtn, xOrgBmp, yOrgBmp, 
                                   xTrnPxl, yTrnPxl, lpRect );
            }
          else
            {
            DrawBitmap( hDC, lpCnt->lpCI->hBmpTtlBtn, xOrgBmp, yOrgBmp, lpRect );
            }
          }
      
        // Add 3d texturing around the title button.
        Draw3DFrame( hDC, lpCnt->hPen[CNTCOLOR_3DHIGH], lpCnt->hPen[CNTCOLOR_3DSHADOW], lpCnt->hPen[CNTCOLOR_GRID],
                     lpRect->left-1, lpRect->top-1,
                     lpRect->right, lpRect->bottom-1, bBtnPressed );
        }
      }

   /*-----------------------------------------------------------------------*
    * Draw the container column titles (if it has any).
    *-----------------------------------------------------------------------*/

    // Only redraw the column title area if it is invalidated.
    if( cyColTitleArea && lpPs->rcPaint.top < cyTitleArea + cyColTitleArea )
      {
      // Select the font for the field titles.
      if( lpCnt->lpCI->hFontColTitle )
        SelectObject( hDC, lpCnt->lpCI->hFontColTitle );
      else if( lpCnt->lpCI->hFont )
        SelectObject( hDC, lpCnt->lpCI->hFont );
      else
        SelectObject( hDC, GetStockObject( lpCnt->nDefaultFont ) );

      // Clip around the column title area.
      lpRect->top    = cyTitleArea;
      lpRect->bottom = cyTitleArea + cyColTitleArea - 1;

      xOff = -lpCnt->nHscrollPos * cxChar;
      yPos = cyTitleArea;
      lpCol = lpCnt->lpCI->lpFieldHead;  // point at 1st column
      while( lpCol )
        {
        // get the col width in pixels
        cxCol = lpCol->cxWidth * cxChar;

        // if this field is hidden, proceed to the next one
        if( !cxCol )
          {
          lpCol = lpCol->lpNext;
          continue;
          }

        // get this column's title and length (if any)
        lpStr = (LPSTR) lpCol->lpFTitle;
        uLen  = lpStr ? lstrlen( lpStr ) : 0;

        // Set the rectangles for the background and the title alignment.
        lpRect->left  = xOff;
        lpRect->right = xOff + cxCol;

        // Adjust the rectangle if a field title button is set.
        if( lpCol->nFldBtnWidth )
          {
          // Get the field title button width in pixels.
          if( lpCol->nFldBtnWidth > 0 )
            cxPxlWidth = cxCharFldTtl * lpCol->nFldBtnWidth;
          else 
            cxPxlWidth = -lpCol->nFldBtnWidth;

          // Check to make sure the column has not been sized way down.
          cxPxlWidth = min(cxPxlWidth, cxCol);

          // Make room for the button in the title area.
          if( lpCol->bFldBtnAlignRt )
            lpRect->right -= cxPxlWidth;
          else
            lpRect->left  += cxPxlWidth;
          }

        CopyRect( &textRect, lpRect );
        CopyRect( &bkgdRect, lpRect );
        bkgdRect.bottom += 1;            // bump so that all bk is painted
        textRect.left   += cxChar2;
        textRect.right  -= cxChar2;
  
        // setup the horz title alignment
        fAlign = DT_NOPREFIX;
        if( lpCol->flFTitleAlign & CA_TA_HCENTER )
          fAlign |= DT_CENTER;
        else if( lpCol->flFTitleAlign & CA_TA_RIGHT )
          fAlign |= DT_RIGHT;
        else
          fAlign |= DT_LEFT;
  
        // setup the vert title alignment
        if( lpCol->flFTitleAlign & CA_TA_VCENTER )
          {
          fAlign |= DT_VCENTER | DT_SINGLELINE;
          if( lpCnt->lpCI->flCntAttr & CA_FLDTTL3D || lpCol->flColAttr & CFA_FLDTTL3D )
            textRect.top -= 1;
          }
        else if( lpCol->flFTitleAlign & CA_TA_BOTTOM )
          {
          fAlign |= DT_BOTTOM | DT_SINGLELINE;
          if( lpCnt->lpCI->flCntAttr & CA_FLDTTL3D || lpCol->flColAttr & CFA_FLDTTL3D )
            textRect.top -= 1;
          textRect.bottom -= cyCharFldTtl4+1;
          }
        else
          {
          fAlign |= DT_TOP;
          textRect.top    += 2;
/*          textRect.bottom -= cyCharFldTtl4+1;*/
          textRect.bottom -= 2;
          }
  
        // Fill the field title area background.
        SetBkColor( hDC, lpCol->bClrFldTtlBk ? lpCol->crFldTtlBk : lpCnt->crColor[CNTCOLOR_FLDTTLBKGD] );
        if( lpCol->flColAttr & CFA_OWNERPNTFTBK && lpCnt->lpfnPaintBk )
          {
          // Call the app to paint the background if owner-paint is enabled.
          (*lpCnt->lpfnPaintBk)( hWnd, lpCol, hDC, &bkgdRect, BK_FLDTITLE );
          }
        else
          {
          ExtTextOut( hDC, 0, 0, ETO_OPAQUE, &bkgdRect, NULL, 0, (LPINT)NULL );
          }

        // Draw the field title text.
        if( uLen )
          {
          SetBkMode( hDC, TRANSPARENT );
          SetTextColor( hDC, lpCol->bClrFldTtlText ? lpCol->crFldTtlText : lpCnt->crColor[CNTCOLOR_FLDTITLES] );
          DrawText( hDC, lpStr, uLen, &textRect, fAlign );
          }
  
        // Draw a bitmap with the title if one is defined for this column.
        if( lpCol->hBmpFldTtl )
          {
          // Get the bitmap extents.
          GetBitmapExt( hDC, lpCol->hBmpFldTtl, &xExt, &yExt );

          // setup the horizontal alignment
          cxWidth = lpRect->right - lpRect->left + 1;
          if( lpCol->flFTBmpAlign & CA_TA_HCENTER )
            xOrgBmp = lpRect->left + cxWidth/2 - xExt/2;
          else if( lpCol->flFTBmpAlign & CA_TA_RIGHT )
            xOrgBmp = lpRect->right - xExt - cxChar2;
          else
            xOrgBmp = lpRect->left + cxChar2;
  
          // setup the vertical alignment
          if( lpCol->flFTBmpAlign & CA_TA_VCENTER )
            yOrgBmp = yPos + cyColTitleArea/2 - yExt/2;
          else if( lpCol->flFTBmpAlign & CA_TA_BOTTOM )
            yOrgBmp = yPos + cyColTitleArea - yExt - cyCharFldTtl4;
          else
            yOrgBmp = yPos + cyCharFldTtl4;

          // Apply any offset.
          xOrgBmp += lpCol->xOffFldTtlBmp;
          yOrgBmp += lpCol->yOffFldTtlBmp;

          if( lpCol->flColAttr & CFA_TRANFLDTTLBMP )
            {
            xTrnPxl = lpCol->xTPxlFldTtlBmp;
            yTrnPxl = lpCol->yTPxlFldTtlBmp;
            DrawTransparentBitmap( hDC, lpCol->hBmpFldTtl, xOrgBmp, yOrgBmp, 
                                   xTrnPxl, yTrnPxl, lpRect );
            }
          else
            {
            DrawBitmap( hDC, lpCol->hBmpFldTtl, xOrgBmp, yOrgBmp, lpRect );
            }
          }

        // Add 3d texturing around the title if enabled for this column.
        if( lpCnt->lpCI->flCntAttr & CA_FLDTTL3D || lpCol->flColAttr & CFA_FLDTTL3D )
          {
          // Make sure it hasn't been sized way down.
          if( lpRect->right > lpRect->left )
            {
            Draw3DFrame( hDC, lpCnt->hPen[CNTCOLOR_3DHIGH], lpCnt->hPen[CNTCOLOR_3DSHADOW], lpCnt->hPen[CNTCOLOR_GRID],
                         lpRect->left-1,  lpRect->top-1,
                         lpRect->right-1, lpRect->bottom, FALSE );
            }
          }

       /*-----------------------------------------------------------------*
        * Draw the field title button if there is one.
        *-----------------------------------------------------------------*/

        if( lpCol->nFldBtnWidth )
          {
          // Setup the field title button rectangle.
          if( lpCol->bFldBtnAlignRt )
            {
            lpRect->left  = lpRect->right;
            lpRect->right = xOff + cxCol;
            }
          else
            {
            lpRect->right = lpRect->left;
            lpRect->left  = xOff;
            }

          // Set the field title button text and background colors.
          SetTextColor( hDC, lpCol->bClrFldBtnText ? lpCol->crFldBtnText : lpCnt->crColor[CNTCOLOR_FLDBTNTXT] );
          SetBkColor(   hDC, lpCol->bClrFldBtnBk   ? lpCol->crFldBtnBk   : lpCnt->crColor[CNTCOLOR_FLDBTNBKGD] );

          // Fill the field title button background.
          ExtTextOut( hDC, 0, 0, ETO_OPAQUE, lpRect, NULL, 0, (LPINT)NULL );

          // Set the pressed button flag.
          bBtnPressed = lpCol->flColAttr & CFA_FLDBTNPRESSED ? TRUE : FALSE;

          // Draw the button text if there is any.
          uLen = lpCol->szFldBtnTxt ? lstrlen( lpCol->szFldBtnTxt ) : 0;
          if( uLen )
            {
            // Set the rectangle for the text alignment.
            CopyRect( &textRect, lpRect );
            textRect.left   += cxChar2;
            textRect.right  -= cxChar2;

            // setup the horz alignment
            fAlign = DT_NOPREFIX;
            if( lpCol->flFldBtnTxtAlign & CA_TA_HCENTER )
              fAlign |= DT_CENTER;
            else if( lpCol->flFldBtnTxtAlign & CA_TA_RIGHT )
              fAlign |= DT_RIGHT;
            else
              fAlign |= DT_LEFT;

            // setup the vert alignment
            if( lpCol->flFldBtnTxtAlign & CA_TA_VCENTER )
              {
              fAlign |= DT_VCENTER | DT_SINGLELINE;
              textRect.top -= 1;
              }
            else if( lpCol->flFldBtnTxtAlign & CA_TA_BOTTOM )
              {
              fAlign |= DT_BOTTOM | DT_SINGLELINE;
              textRect.top    -= 1;
              textRect.bottom -= cyCharFldTtl4+1;
              }
            else
              {
              fAlign |= DT_TOP;
              textRect.top    += 2;
              textRect.bottom -= cyCharFldTtl4+1;
              }

            // Offset the text to simulate button pressing.
            textRect.top    += bBtnPressed;
            textRect.left   += bBtnPressed;
            textRect.bottom += bBtnPressed;
            textRect.right  += bBtnPressed;

            // Draw the button text.
            DrawText( hDC, lpCol->szFldBtnTxt, uLen, &textRect, fAlign );
            }

          // Draw a bitmap on the button face if one is defined.
          if( lpCol->hBmpFldBtn )
            {
            // Get the bitmap extents.
            GetBitmapExt( hDC, lpCol->hBmpFldBtn, &xExt, &yExt );
        
            // setup the horizontal alignment
            cxWidth = lpRect->right - lpRect->left + 1;
            if( lpCol->flFldBtnBmpAlign & CA_TA_HCENTER )
              xOrgBmp = lpRect->left + cxWidth/2 - xExt/2;
            else if( lpCol->flFldBtnBmpAlign & CA_TA_RIGHT )
              xOrgBmp = lpRect->right - xExt - cxChar2;
            else
              xOrgBmp = lpRect->left + cxChar2;
          
            // setup the vertical alignment
            if( lpCol->flFldBtnBmpAlign & CA_TA_VCENTER )
              yOrgBmp = yPos + cyColTitleArea/2 - yExt/2;
            else if( lpCol->flFldBtnBmpAlign & CA_TA_BOTTOM )
              yOrgBmp = yPos + cyColTitleArea - yExt - cyCharFldTtl4;
            else
              yOrgBmp = yPos + cyCharFldTtl4;
        
            // Apply any offset.
            xOrgBmp += lpCol->xOffFldBtnBmp;
            yOrgBmp += lpCol->yOffFldBtnBmp;
        
            // Offset the bitmap to simulate button pressing.
            xOrgBmp += bBtnPressed;
            yOrgBmp += bBtnPressed;

            // Draw the bitmap on the button face.
            if( lpCol->flColAttr & CFA_TRANFLDBTNBMP )
              {
              xTrnPxl = lpCol->xTPxlFldBtnBmp;
              yTrnPxl = lpCol->yTPxlFldBtnBmp;
              DrawTransparentBitmap( hDC, lpCol->hBmpFldBtn, xOrgBmp, yOrgBmp, 
                                     xTrnPxl, yTrnPxl, lpRect );
              }
            else
              {
              DrawBitmap( hDC, lpCol->hBmpFldBtn, xOrgBmp, yOrgBmp, lpRect );
              }
            }
        
          // Add 3d texturing around the field title button.
          Draw3DFrame( hDC, lpCnt->hPen[CNTCOLOR_3DHIGH], lpCnt->hPen[CNTCOLOR_3DSHADOW], lpCnt->hPen[CNTCOLOR_GRID],
                       lpRect->left-1,  lpRect->top-1,
                       lpRect->right-1, lpRect->bottom, bBtnPressed );
          }

        // advance to next column
        xOff += cxCol;
        lpCol = lpCol->lpNext;
        }
      }

   /*-----------------------------------------------------------------------*
    * Fill the field data area background.
    *-----------------------------------------------------------------------*/

    // Calc the horz and vert offset where records start.
    xOff = -lpCnt->nHscrollPos * cxChar;
    yOff = cyTitleArea + cyColTitleArea;

    // Set clip rect so we don't draw outside the field.
    lpRect->top    = yOff;
    lpRect->bottom = lpCnt->cyClient;

    lpCol = lpCnt->lpCI->lpFieldHead;  // point at 1st column
    while( lpCol )
      {
      // get the col width in pixels
      cxCol = lpCol->cxWidth * cxChar;

      // Do more precise checking on the invalidated rect so that
      // we only repaint fields that are actually invalidated.
      if( xOff >= lpPs->rcPaint.right || xOff+cxCol-1 < lpPs->rcPaint.left )
        {
        // this col is not invalidated so advance to next column
        xOff += cxCol;
        lpCol = lpCol->lpNext;
        continue;
        }

      // Only check fields which are not hidden.
      if( cxCol )
        {
        if( lpCol->flColAttr & CFA_OWNERPNTFLDBK && lpCnt->lpfnPaintBk )
          {
          lpRect->left  = xOff;
          lpRect->right = xOff + cxCol;

          if( lpCol->bClrFldBk )
            SetBkColor( hDC, lpCol->crFldBk );
          else
            SetBkColor( hDC, lpCnt->crColor[CNTCOLOR_CNTBKGD] );

          // Call the app to paint the fld bkgrnd if owner-paint is enabled.
          (*lpCnt->lpfnPaintBk)( hWnd, lpCol, hDC, lpRect, BK_FLD );
          }
        else if( lpCol->bClrFldBk )
          {
          lpRect->left  = xOff;
          lpRect->right = xOff + cxCol;
    
          // Fill the field background.
          SetBkColor( hDC, lpCol->crFldBk );
          ExtTextOut( hDC, 0, 0, ETO_CLIPPED | ETO_OPAQUE, lpRect, NULL, 0, NULL );
          }
        }

      // advance to next column
      xOff += cxCol;
      lpCol = lpCol->lpNext;
      }

   /*-----------------------------------------------------------------------*
    * Draw the container records column by column.
    *-----------------------------------------------------------------------*/

    // Select the font for the data.
    if( lpCnt->lpCI->hFont )
      SelectObject( hDC, lpCnt->lpCI->hFont );
    else
      SelectObject( hDC, GetStockObject( lpCnt->nDefaultFont ) );

    SelectObject( hDC, lpCnt->hPen[CNTCOLOR_GRID] );
    SetBkColor( hDC, lpCnt->crColor[CNTCOLOR_CNTBKGD] );
    SetTextColor( hDC, lpCnt->crColor[CNTCOLOR_TEXT] );
    fEtoOp = ETO_CLIPPED;
    
    bBlkSelect = lpCnt->dwStyle & CTS_BLOCKSEL ? TRUE : FALSE;

    // calc the vertical offset where records start
    yOff = cyTitleArea + cyColTitleArea;

    // Set right/left defaults. It will get overridden when the field
    // which has the focus is processed (if not using wide focus rect).
    if( lpCnt->lpCI->flCntAttr & CA_WIDEFOCUSRECT && 
        lpCnt->lpCI->flCntAttr & CA_HSCROLLBYCHAR )
      {
      rectFocus.left  = -lpCnt->nHscrollPos * cxChar;
      rectFocus.right = lpCnt->lpCI->nMaxWidth - lpCnt->nHscrollPos * cxChar - 1;
      }
    else
      {
      rectFocus.left  = 0;
      rectFocus.right = xLastCol-1;
      }

    // Calc the invalid records which must be repainted and
    // scroll the record list to the 1st invalid record.
    nPaintBegRec = max(0, (lpPs->rcPaint.top-yOff) / cyRow - 1);
    nPaintEndRec = min(lpCnt->lpCI->nRecsDisplayed, (lpPs->rcPaint.bottom-yOff) / cyRow);
    lpRec = CntScrollRecList( hWnd, lpCnt->lpCI->lpTopRec, nPaintBegRec );

    // Loop thru the invalid records which are in the display range.
    for( i = nPaintBegRec; i <= nPaintEndRec && lpRec; i++ )
      {
      // loop thru each field for the record data
      xOff = -lpCnt->nHscrollPos * cxChar;
      yPos = yOff + cyRow * i;

      // Do more precise checking on the invalidated rect so that
      // we only repaint records that are actually invalidated.
/*      if( yPos > lpPs->rcPaint.bottom || yPos+cyRow-1 < lpPs->rcPaint.top )*/
      if( yPos >= lpPs->rcPaint.bottom || yPos+cyRow-1 < lpPs->rcPaint.top )
        {
        // advance to next data record
        lpRec = lpRec->lpNext;
        continue;
        }

      // Set clip rect so we don't draw outside the field.
      lpRect->top    = max(yOff, yPos);
/*      lpRect->bottom = lpRect->top + cyRow - 1;*/
      lpRect->bottom = lpRect->top + cyRow;

      // Setup the focus rect.
      rectFocus.top    = lpRect->top;
/*      rectFocus.bottom = lpRect->bottom;*/
      rectFocus.bottom = lpRect->bottom - 1;

      bRecInUse = bRecSelected = bFldSelected = FALSE;

      // If this record is selected adjust the colors.
      if( lpRec->flRecAttr & CRA_SELECTED )
        {
        bRecSelected = TRUE;
        fEtoOp = ETO_CLIPPED | ETO_OPAQUE;
        SetBkColor( hDC, lpCnt->crColor[CNTCOLOR_HIGHLIGHT] );
        SetTextColor( hDC, lpCnt->crColor[CNTCOLOR_HITEXT] );
        }

      // If this record is in-use adjust the background mode.
      if( lpRec->flRecAttr & CRA_INUSE )
        {
        bRecInUse = TRUE;
        SetBkMode( hDC, OPAQUE );
        fEtoOp &= ~ETO_OPAQUE;
        }

      lpCol = lpCnt->lpCI->lpFieldHead;  // point at 1st column

      // For wide focus rect we always say we found the focus fld.
      if( lpCnt->lpCI->flCntAttr & CA_WIDEFOCUSRECT )
        bFoundFocFld = TRUE;
      else
        bFoundFocFld = FALSE;

      while( lpCol )
        {
        // get the col width in pixels
        cxCol = lpCol->cxWidth * cxChar;

        // Do more precise checking on the invalidated rect so that
        // we only repaint fields that are actually invalidated.
/*        if( xOff > lpPs->rcPaint.right || xOff+cxCol-1 < lpPs->rcPaint.left )*/
        if( xOff >= lpPs->rcPaint.right || xOff+cxCol-1 < lpPs->rcPaint.left )
          {
          // this col is not invalidated so advance to next column
          xOff += cxCol;
          lpCol = lpCol->lpNext;
          continue;
          }

        // Don't do any drawing for hidden fields.
        if( cxCol )
          {
          // If block select is on and this field is selected
          // adjust the text and background colors.
          if( bBlkSelect )
            {
            bFldSelected = FALSE;
            if( CntIsFldSelected( hWnd, lpRec, lpCol ) )
              {
              bFldSelected = TRUE;
              fEtoOp = ETO_CLIPPED | ETO_OPAQUE;
              SetBkColor( hDC, lpCnt->crColor[CNTCOLOR_HIGHLIGHT] );
              SetTextColor( hDC, lpCnt->crColor[CNTCOLOR_HITEXT] );
              }
            }

          // get the record data for this column
          uLen = GetRecordData( hWnd, lpRec, lpCol, MAX_STRING_LEN, (LPSTR)&szData );
  
          // setup the horz data alignment
          fAlign = TA_TOP;
          if( lpCol->flFDataAlign & CA_TA_HCENTER )
            {
            fAlign |= TA_CENTER;
            xPos = xOff + cxCol / 2;
            }
          else if( lpCol->flFDataAlign & CA_TA_RIGHT )
            {
            fAlign |= TA_RIGHT;
            xPos = xOff + cxCol - cxChar2;
            }
          else
            {
            fAlign |= TA_LEFT;
            xPos = xOff + cxChar2;
            }
  
          // setup the vert data alignment
          if( lpCol->flFDataAlign & CA_TA_VCENTER )
            cyLineOff = cyRow2 - cyChar2;
          else if( lpCol->flFDataAlign & CA_TA_BOTTOM )
            cyLineOff = cyRow - cyChar - 1;
          else
            cyLineOff = 0;
  
          // Set clip rect so we don't draw outside the field.
          lpRect->left  = xOff;
          lpRect->right = xOff + cxCol;
  
          // Remember the focus rect if this field has the focus.
          // If the focus rect spans the container we don't adj left/right.
          if( !(lpCnt->lpCI->flCntAttr & CA_WIDEFOCUSRECT) )
            {
            if( lpCol->flColAttr & CFA_CURSORED )
              {
              bFoundFocFld = TRUE;
              rectFocus.left  = lpRect->left;
              rectFocus.right = lpRect->right-1;
              }
            }

          SetTextAlign( hDC, fAlign );   // Set the text alignment.

          // If this field is not selected and not in-use check 
          // for record or field colors.
          if( !bRecSelected && !bFldSelected && !bRecInUse )
            {
            // Set the background attributes by priority.
            if( lpRec->bClrRecBk )
              {
              // Explicit record color is 1st priority.
              fEtoOp |= ETO_OPAQUE;
              SetBkColor( hDC, lpRec->crRecBk );
              }
            else if( lpCol->flColAttr & CFA_OWNERPNTFLDBK && lpCnt->lpfnPaintBk )
              {
              // Owner painting of field background is 2nd priority.
              // Don't alter field background if owner is painting it.
              fEtoOp &= ~ETO_OPAQUE;

              // Set text bkmode to opaque because the user is painting some 
              // kind of pattern in the field background and we want the text
              // to be readable.
              SetBkMode( hDC, OPAQUE );

              // Set the field background color if the user set one.
              if( lpCol->bClrFldBk )
                SetBkColor( hDC, lpCol->crFldBk );
              }
            else if( lpCol->bClrFldBk )
              {
              // Explicit field background color is 3rd priority.
              fEtoOp |= ETO_OPAQUE;
              SetBkColor( hDC, lpCol->crFldBk );
              }
            else if( lpCnt->lpCI->flCntAttr & CA_OWNERPNTBK && lpCnt->lpfnPaintBk )
              {
              // Owner painting of general background is 4th priority.
              // Don't alter field background if owner is painting total bkgrnd.
              fEtoOp &= ~ETO_OPAQUE;

              // Set text bkmode to opaque because the user is painting some 
              // kind of pattern in the general background and we want the
              // text to be readable.
              SetBkMode( hDC, OPAQUE );

              // Set the field background color if the user set one.
              if( lpCol->bClrFldBk )
                SetBkColor( hDC, lpCol->crFldBk );
              }

            if( lpRec->bClrRecText )
              SetTextColor( hDC, lpRec->crRecText );
            else if( lpCol->bClrFldText )
              SetTextColor( hDC, lpCol->crFldText );
            }

          // Paint the in-use emphasis in the field background first.
          if( bRecInUse )
            PaintInUseRect( hWnd, lpCnt, hDC, lpRect );

          // Put out the data.
          if( !(lpCol->flColAttr & CFA_OWNERDRAW) )
            {
            ExtTextOut( hDC, xPos, yPos+cyLineOff, fEtoOp, lpRect, (LPSTR)&szData, uLen, (LPINT)NULL );
            }
          else
            {
            // Call the app to draw the data if owner-draw is enabled.
            lpData = (LPSTR)lpRec->lpRecData + lpCol->wOffStruct;    
            if( lpCol->lpfnDrawData )
              (*lpCol->lpfnDrawData)( hWnd, lpCol, lpRec, lpData, hDC,
                                      xPos, yPos+cyLineOff, fEtoOp, lpRect,
                                      (LPSTR)&szData, uLen );
            }

          // Restore any colors that were changed.
          if( !bRecSelected && !bFldSelected )
            {
            if( lpRec->bClrRecBk || lpCol->bClrFldBk )
              {
              fEtoOp &= ~ETO_OPAQUE;
              SetBkColor( hDC, lpCnt->crColor[CNTCOLOR_CNTBKGD] );
              }
            if( lpRec->bClrRecText || lpCol->bClrFldText )
              SetTextColor( hDC, lpCnt->crColor[CNTCOLOR_TEXT] );
            }

          // If block select is on and this field was selected
          // restore the text and background colors.
          if( bFldSelected )
            {
            fEtoOp = ETO_CLIPPED;
            SetBkColor( hDC, lpCnt->crColor[CNTCOLOR_CNTBKGD] );
            SetTextColor( hDC, lpCnt->crColor[CNTCOLOR_TEXT] );
            }

          }

        // advance to next column
        xOff += cxCol;
        lpCol = lpCol->lpNext;
        }
  
      // Draw the record separator line if turned on.
      if( lpCnt->lpCI->flCntAttr & CA_RECSEPARATOR )
        {
        xStr = 0;
        xEnd = xLastCol;
        MoveToEx( hDC, xStr, yPos+cyRow-1, NULL );
        LineTo( hDC, xEnd, yPos+cyRow-1 );
        }

      // Draw the focus rect if this record has the focus.
      if( bCntHasFocus && bFoundFocFld && (lpRec->flRecAttr & CRA_CURSORED) )
        {
        DrawFocusRect( hDC, &rectFocus );

        // Draw a double width focus rect if desired.
        if( lpCnt->lpCI->bDrawFocusRect > 1 || lpCnt->bSimulateMulSel )
          {
          InflateRect( &rectFocus, -1, -1 );
          DrawFocusRect( hDC, &rectFocus );
          }
        }

      // If this record was selected set the colors back to normal.
      if( bRecSelected )
        {
        fEtoOp = ETO_CLIPPED;
        SetBkColor( hDC, lpCnt->crColor[CNTCOLOR_CNTBKGD] );
        SetTextColor( hDC, lpCnt->crColor[CNTCOLOR_TEXT] );
        }

      // advance to next data record
      lpRec = lpRec->lpNext;
      }

   /*-----------------------------------------------------------------------*
    * Draw the container grid (if enabled).
    *-----------------------------------------------------------------------*/

    SelectObject( hDC, lpCnt->hPen[CNTCOLOR_GRID] );

    // Draw horz title separator line if turned on (and we have a title area).
    if( lpCnt->lpCI->flCntAttr & CA_TTLSEPARATOR && cyTitleArea )
      {
      yPos = cyTitleArea - 1;

      // Only draw the line if its inside the invalidated rect.
      if( yPos >= lpPs->rcPaint.top && yPos <= lpPs->rcPaint.bottom )
        {
        // Only draw invalidated portions of the line.
        xStr = max( 0,              lpPs->rcPaint.left  );
        xEnd = min( lpCnt->cxClient, lpPs->rcPaint.right );
        MoveToEx( hDC, xStr, yPos, NULL );
        LineTo( hDC, xEnd, yPos );
        }
      }

    // Draw the horz column titles separator line if turned on.
    if( lpCnt->lpCI->flCntAttr & CA_FLDSEPARATOR && cyColTitleArea )
      {
      yPos = cyTitleArea + cyColTitleArea - 1;

      // Only draw the line if its inside the invalidated rect.
      if( yPos >= lpPs->rcPaint.top && yPos <= lpPs->rcPaint.bottom )
        {
        // Only draw invalidated portions of the line.
        xStr = max( 0,        lpPs->rcPaint.left  );
        xEnd = min( xLastCol, lpPs->rcPaint.right );
        MoveToEx( hDC, xStr, yPos, NULL );
        LineTo( hDC, xEnd, yPos );
        }
      }

    // Draw the vertical column lines if turned on.
    xPos  = -lpCnt->nHscrollPos * cxChar;
    yPos  = max(cyTitleArea,    lpPs->rcPaint.top    );
    yPos2 = min(lpCnt->cyClient, lpPs->rcPaint.bottom );

    lpCol = lpCnt->lpCI->lpFieldHead;  // point at 1st column
    while( lpCol )
      {
      // column line is drawn after the column
      xPos += lpCol->cxWidth * cxChar;

      if( lpCnt->lpCI->flCntAttr & CA_VERTFLDSEP || lpCol->flColAttr & CFA_VERTSEPARATOR )
        {
        // Only draw the line if its inside the invalidated rect.
        if( xPos-1 >= lpPs->rcPaint.left && xPos-1 <= lpPs->rcPaint.right )
          {
          MoveToEx( hDC, xPos-1, yPos, NULL );
          LineTo( hDC, xPos-1, yPos2 );
          }
        }
      lpCol = lpCol->lpNext;       // advance to next column
      }

    return;
    }

