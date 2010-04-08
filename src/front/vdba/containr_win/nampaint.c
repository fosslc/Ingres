/****************************************************************************
 * Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * NAMPAINT.C
 *
 * Contains WM_PAINT logic for the NAME view format.
 ****************************************************************************/

#include <windows.h>
#include "cntdll.h"

/****************************************************************************
 * PaintNameView
 *
 * Purpose:
 *  Paints the container in a name view format.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window.
 *  LPCONTAINER   lpCnt       - pointer to container's CONTAINER structure.
 *  DC            hDC         - container window DC
 *  LPPAINTSTRUCT lpPs        - pointer to paint struct
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI PaintNameView( HWND hWnd, LPCONTAINER lpCnt, HDC hDC, LPPAINTSTRUCT lpPs )
{
    LPRECORDCORE   lpRec;          // pointer to RECORDCORE list
    LPFLOWCOLINFO  lpCol;          // pointer to FLOWCOLINFO list
    LPRECT   lpRect, lpImageRect;
    RECT     CntClientRect;
    RECT     textRect, rectFocus;
    char     szData[MAX_STRING_LEN];
    int      cyChar;            // height of char
    int      cxChar,  cyRow;    // width and height of char
    int      cxChar2, cyRow2;   // half width and height of char
    int      cxChar4, cyRow4;   // quarter width and height of char
    int      cyTitleArea=0;
    int      cxCol, cxWidth, cxPxlWidth, xLastCol;
    int      cxCharTtl, cxCharTtl2;
    int      cyCharTtl4, cyChar2;
    int      cyLineOff, cyIcon;
    int      xPos, yPos, xOff, yOff;
    UINT     uLen;
    UINT     fAlign;
    int      xOrgBmp, yOrgBmp, xExt, yExt;
    int      xTrnPxl, yTrnPxl;
    BOOL     bBtnPressed, bRet;
    UINT     fEtoOp;            // flags for ExtTextOut style
    int      nPaintBegRec, nPaintEndRec, i, nEndRec;
    BOOL     bRecSelected, bRecInUse;
    BOOL     bUseMemDC;
    HDC      hdcMem;
    HBITMAP  hbmMem, hbmMemOld;

   /*-----------------------------------------------------------------------*
    * Select the container font and set some character metrics.
    *-----------------------------------------------------------------------*/

    if( lpCnt->lpCI->hFont )
      SelectObject( hDC, lpCnt->lpCI->hFont );
    else
      SelectObject( hDC, GetStockObject( lpCnt->nDefaultFont ) );

    // set whole, half, and quarter char dimensions
    cxChar  = lpCnt->lpCI->cxChar;
    cyChar  = lpCnt->lpCI->cyChar;
    cyChar2 = lpCnt->lpCI->cyChar2;     // half height of gen font char
    cyRow   = lpCnt->lpCI->cyRow;
    cxChar2 = lpCnt->lpCI->cxChar2;
    cyRow2  = lpCnt->lpCI->cyRow2;
    cxChar4 = lpCnt->lpCI->cxChar4;
    cyRow4  = lpCnt->lpCI->cyRow4;

    // widths and heights of title font char
    cxCharTtl  = lpCnt->lpCI->cxCharTtl;
    cxCharTtl2 = lpCnt->lpCI->cxCharTtl2;
    cyCharTtl4 = lpCnt->lpCI->cyCharTtl4;

    // Space required for the container title and column titles.
    cyTitleArea    = lpCnt->lpCI->cyTitleArea;

    // if redrawing the entire container, draw it to memory, than bitblt it to the screen
    // this causes a visual pause in the painting.  we decided this was better than 
    // watching the records paint one by one.  If you disagree, comment out this code
    // and set bUseMemDC = FALSE.
    if (lpPs->rcPaint.left == 0 && lpPs->rcPaint.top == 0 &&
        lpPs->rcPaint.right == lpCnt->cxClient && lpPs->rcPaint.bottom == lpCnt->cyClient)
    {
      bUseMemDC = TRUE;
      hdcMem    = CreateCompatibleDC( hDC );
      hbmMem    = CreateCompatibleBitmap( hDC, lpCnt->cxClient, lpCnt->cyClient);
      hbmMemOld = SelectObject( hdcMem, hbmMem );
    }
    else 
      bUseMemDC = FALSE;
    
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

    SetBkColor( bUseMemDC ? hdcMem : hDC, lpCnt->crColor[CNTCOLOR_CNTBKGD] );

    if( lpCnt->lpCI->flCntAttr & CA_OWNERPNTBK && lpCnt->lpfnPaintBk )
    {
      // Call the app to paint the background if owner-paint is enabled.
      (*lpCnt->lpfnPaintBk)( hWnd, NULL, bUseMemDC ? hdcMem : hDC, &lpPs->rcPaint, BK_GENERAL );
    }
    else
    {
      ExtTextOut( bUseMemDC ? hdcMem : hDC, lpPs->rcPaint.left, lpPs->rcPaint.top, ETO_OPAQUE, &lpPs->rcPaint, NULL, 0, NULL );
    }
//    ExtTextOut( hDC, lpRect->left, lpRect->top, ETO_OPAQUE, lpRect, NULL, 0, NULL);

   /*-----------------------------------------------------------------------*
    * Draw the container title (if it has one).
    *-----------------------------------------------------------------------*/

    // Only redraw the title if it is invalidated.
    if( cyTitleArea && lpPs->rcPaint.top < cyTitleArea )
    {
      uLen = lpCnt->lpCI->lpszTitle ? lstrlen( lpCnt->lpCI->lpszTitle ) : 0;

      // Select the font for the title.
      if( lpCnt->lpCI->hFontTitle )
        SelectObject( bUseMemDC ? hdcMem : hDC, lpCnt->lpCI->hFontTitle );
      else if( lpCnt->lpCI->hFont )
        SelectObject( bUseMemDC ? hdcMem : hDC, lpCnt->lpCI->hFont );
      else
        SelectObject( bUseMemDC ? hdcMem : hDC, GetStockObject( lpCnt->nDefaultFont ) );

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
      SetBkColor( bUseMemDC ? hdcMem : hDC, lpCnt->crColor[CNTCOLOR_TTLBKGD] );
      if( lpCnt->lpCI->flCntAttr & CA_OWNERPNTTTLBK && lpCnt->lpfnPaintBk )
      {
        // Call the app to paint the title background if owner-paint is enabled.
        (*lpCnt->lpfnPaintBk)( hWnd, NULL, bUseMemDC ? hdcMem : hDC, lpRect, BK_TITLE );
      }
      else
      {
        ExtTextOut( bUseMemDC ? hdcMem : hDC, 0, 0, ETO_OPAQUE, lpRect, NULL, 0, (LPINT)NULL );
      }

      // Draw the title.
      if( uLen )
      {
        SetBkMode( bUseMemDC ? hdcMem : hDC, TRANSPARENT );
        SetTextColor( bUseMemDC ? hdcMem : hDC, lpCnt->crColor[CNTCOLOR_TITLE] );
        DrawText( bUseMemDC ? hdcMem : hDC, lpCnt->lpCI->lpszTitle, uLen, &textRect, fAlign );
      }

      // Draw a bitmap with the title if one is defined.
      if( lpCnt->lpCI->hBmpTtl )
      {
        // Get the bitmap extents.
        GetBitmapExt( bUseMemDC ? hdcMem : hDC, lpCnt->lpCI->hBmpTtl, &xExt, &yExt );

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
          DrawTransparentBitmap( bUseMemDC ? hdcMem : hDC, lpCnt->lpCI->hBmpTtl, xOrgBmp, yOrgBmp, 
                                 xTrnPxl, yTrnPxl, lpRect );
        }
        else
        {
          DrawBitmap( bUseMemDC ? hdcMem : hDC, lpCnt->lpCI->hBmpTtl, xOrgBmp, yOrgBmp, lpRect );
        }
      }

      // Add 3d texturing around the title if enabled.
      if( lpCnt->lpCI->flCntAttr & CA_TITLE3D )
      {
        Draw3DFrame( bUseMemDC ? hdcMem : hDC, lpCnt->hPen[CNTCOLOR_3DHIGH], lpCnt->hPen[CNTCOLOR_3DSHADOW], lpCnt->hPen[CNTCOLOR_GRID],
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
        SetBkColor( bUseMemDC ? hdcMem : hDC, lpCnt->crColor[CNTCOLOR_TTLBTNBKGD] );
        SetTextColor( bUseMemDC ? hdcMem : hDC, lpCnt->crColor[CNTCOLOR_TTLBTNTXT] );
        ExtTextOut( bUseMemDC ? hdcMem : hDC, 0, 0, ETO_OPAQUE, lpRect, NULL, 0, (LPINT)NULL );

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
          DrawText( bUseMemDC ? hdcMem : hDC, lpCnt->lpCI->szTtlBtnTxt, uLen, &textRect, fAlign );
        }

        // Draw a bitmap on the button face if one is defined.
        if( lpCnt->lpCI->hBmpTtlBtn )
        {
          // Get the bitmap extents.
          GetBitmapExt( bUseMemDC ? hdcMem : hDC, lpCnt->lpCI->hBmpTtlBtn, &xExt, &yExt );
      
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
            DrawTransparentBitmap( bUseMemDC ? hdcMem : hDC, lpCnt->lpCI->hBmpTtlBtn, xOrgBmp, yOrgBmp, 
                                   xTrnPxl, yTrnPxl, lpRect );
          }
          else
          {
            DrawBitmap( bUseMemDC ? hdcMem : hDC, lpCnt->lpCI->hBmpTtlBtn, xOrgBmp, yOrgBmp, lpRect );
          }
        }
      
        // Add 3d texturing around the title button.
        Draw3DFrame( bUseMemDC ? hdcMem : hDC, lpCnt->hPen[CNTCOLOR_3DHIGH], lpCnt->hPen[CNTCOLOR_3DSHADOW], lpCnt->hPen[CNTCOLOR_GRID],
                     lpRect->left-1, lpRect->top-1,
                     lpRect->right, lpRect->bottom-1, bBtnPressed );
      }
    }

   /*-----------------------------------------------------------------------*
    * Draw the container items.
    *-----------------------------------------------------------------------*/

    // Select the font for the data.
    if( lpCnt->lpCI->hFont )
      SelectObject( bUseMemDC ? hdcMem : hDC, lpCnt->lpCI->hFont );
    else
      SelectObject( bUseMemDC ? hdcMem : hDC, GetStockObject( lpCnt->nDefaultFont ) );

    SetBkColor( bUseMemDC ? hdcMem : hDC, lpCnt->crColor[CNTCOLOR_CNTBKGD] );
    SetTextColor( bUseMemDC ? hdcMem : hDC, lpCnt->crColor[CNTCOLOR_TEXT] );
    fEtoOp = ETO_CLIPPED;
    
    // calc the vertical offset where records start
    yOff = cyTitleArea;

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
    nPaintBegRec = max(0, (lpPs->rcPaint.top-yOff) / cyRow - 1);
    nPaintEndRec = min(lpCnt->lpCI->nRecsDisplayed, (lpPs->rcPaint.bottom-yOff) / cyRow);

    xOff  = -lpCnt->nHscrollPos * cxChar;
    lpCol =  lpCnt->lpColHead;

    while (lpCol)
    {
      // get the col width in pixels
      cxCol = lpCol->cxPxlWidth;

      // Do more precise checking on the invalidated rect so that
      // we only repaint fields that are actually invalidated.
      if( xOff >= lpPs->rcPaint.right || xOff+cxCol-1 < lpPs->rcPaint.left )
      {
        // this col is not invalidated so advance to next column
        xOff += cxCol;
        lpCol = lpCol->lpNext;
        continue;
      }

      // Don't do any drawing if no text strings.
      if( cxCol > lpCnt->lpCI->ptBmpOrIcon.x )
      {
        // scroll the record list to the 1st invalid record.
        if (lpCnt->iView & CV_FLOW) 
          lpRec = CntScrollRecList( hWnd, lpCol->lpTopRec, nPaintBegRec );
        else
          lpRec = CntScrollRecList( hWnd, lpCnt->lpCI->lpTopRec, nPaintBegRec );

        // Loop thru the invalid records which are in the display range.
        nEndRec = min(nPaintEndRec, lpCol->nRecs-1); // if incomplete column (last one) use # recs
        for( i = nPaintBegRec; i <= nEndRec && lpRec; i++ )
        {
          // loop thru each field for the record data
          yPos = yOff + cyRow * i;

          // Do more precise checking on the invalidated rect so that
          // we only repaint records that are actually invalidated.
          if( yPos >= lpPs->rcPaint.bottom || yPos+cyRow-1 < lpPs->rcPaint.top )
          {
            // advance to next data record
            lpRec = lpRec->lpNext;
            continue;
          }

          // Set clip rect so we don't draw outside the field.
          lpRect->top    = max(yOff, yPos);
          lpRect->bottom = lpRect->top + cyRow;

          // Setup the focus rect.
          rectFocus.top    = lpRect->top;
          rectFocus.bottom = lpRect->bottom - 1;

          bRecInUse = bRecSelected = FALSE;

          // If this record is selected adjust the colors.
          if( lpRec->flRecAttr & CRA_SELECTED )
          {
            bRecSelected = TRUE;
            fEtoOp = ETO_CLIPPED | ETO_OPAQUE;
            SetBkColor( bUseMemDC ? hdcMem : hDC, lpCnt->crColor[CNTCOLOR_HIGHLIGHT] );
            SetTextColor( bUseMemDC ? hdcMem : hDC, lpCnt->crColor[CNTCOLOR_HITEXT] );
          }

          // If this record is in-use adjust the background mode.
          if( lpRec->flRecAttr & CRA_INUSE )
          {
            bRecInUse = TRUE;
            SetBkMode( bUseMemDC ? hdcMem : hDC, OPAQUE );
            fEtoOp &= ~ETO_OPAQUE;
          }

          // copy the record name - if no name, nothing appears
          if (lpRec->lpszName)
            lstrcpy(szData, lpRec->lpszName);
          else
            lstrcpy(szData," \0");
          uLen = lstrlen(szData);
      
          // setup the horz data alignment
          fAlign = TA_TOP;
//          if( lpCol->flFDataAlign & CA_TA_HCENTER )
//          {
//            fAlign |= TA_CENTER;
//            xPos = xOff + cxCol / 2;
//          }
//          else if( lpCol->flFDataAlign & CA_TA_RIGHT )
//          {
//            fAlign |= TA_RIGHT;
//            xPos = xOff + cxCol - cxChar2;
//          }
//          else
//          {
          fAlign |= TA_LEFT;
          xPos = xOff + cxChar2 + lpCnt->lpCI->ptBmpOrIcon.x + cxChar2;
//          }
  
          cyIcon = cyRow2 - (lpCnt->lpCI->ptBmpOrIcon.y / 2);

          // setup the vert data alignment
//          if( lpCol->flFDataAlign & CA_TA_VCENTER )
          cyLineOff = cyRow2 - cyChar2;
//          else if( lpCol->flFDataAlign & CA_TA_BOTTOM )
//            cyLineOff = cyRow - cyChar - 1;
//          else
//            cyLineOff = 0;
  
          // Set clip rect so we don't draw outside the field.
          lpRect->left  = xOff;
          lpRect->right = xOff + cxCol;
  
          SetTextAlign( bUseMemDC ? hdcMem : hDC, fAlign );   // Set the text alignment.

          // If this record is not selected check for record colors.
          if( !bRecSelected && !bRecInUse )
          {
            // Set the background attributes 
            if( lpRec->bClrRecBk )
            {
              // Explicit record color is 1st priority.
              fEtoOp |= ETO_OPAQUE;
              SetBkColor( bUseMemDC ? hdcMem : hDC, lpRec->crRecBk );
            }

            if( lpRec->bClrRecText )
              SetTextColor( bUseMemDC ? hdcMem : hDC, lpRec->crRecText );
          }

          // Paint the in-use emphasis in the field background first.
          if( bRecInUse )
            PaintInUseRect( hWnd, lpCnt, bUseMemDC ? hdcMem : hDC, lpRect );

          // Put out the text data.
          ExtTextOut( bUseMemDC ? hdcMem : hDC, xPos, yPos+cyLineOff, fEtoOp, lpRect, (LPSTR)&szData, uLen, (LPINT)NULL );

          // draw the icon (or bitmap) - if no attribute specified, default is icon
          lpImageRect = &textRect;
          lpImageRect->left   = xOff + cxChar2;
          lpImageRect->right  = lpImageRect->left + lpCnt->lpCI->ptBmpOrIcon.x;
          lpImageRect->top    = yPos + cyIcon;
          lpImageRect->bottom = lpImageRect->top + lpCnt->lpCI->ptBmpOrIcon.y;
          if( lpCnt->lpCI->flCntAttr & CA_DRAWBMP )
          {
            // Draw bitmap at specified location
            DrawBitmapXY(lpCnt, lpRec, bUseMemDC ? hdcMem : hDC, lpImageRect->left, lpImageRect->top, 
                         lpImageRect);
          }
          else if( lpCnt->lpCI->flCntAttr & CA_DRAWICON )
          {
            // Draw icon at specified location
            DrawIconXY(lpCnt, lpRec, bUseMemDC ? hdcMem : hDC, xOff + cxChar2, yPos + cyIcon, 
                       lpCnt->lpCI->ptBmpOrIcon.x, lpCnt->lpCI->ptBmpOrIcon.y);
          }
          else // not specified - first check to see if icon exists and if not chk for bitmap
          {
            // Draw bitmap at specified location
            bRet = DrawIconXY(lpCnt, lpRec, bUseMemDC ? hdcMem : hDC, xOff + cxChar2, yPos + cyIcon, 
                              lpCnt->lpCI->ptBmpOrIcon.x, lpCnt->lpCI->ptBmpOrIcon.y);
            if (bRet==FALSE)
            {
              DrawBitmapXY(lpCnt, lpRec, bUseMemDC ? hdcMem : hDC, lpImageRect->left, lpImageRect->top, 
                           lpImageRect);
            }
          }

        // Restore any colors that were changed.
//        if( !bRecSelected && !bFldSelected )
//        {
          if( lpRec->bClrRecBk )
          {
            fEtoOp &= ~ETO_OPAQUE;
            SetBkColor( bUseMemDC ? hdcMem : hDC, lpCnt->crColor[CNTCOLOR_CNTBKGD] );
          }
          if( lpRec->bClrRecText )
            SetTextColor( bUseMemDC ? hdcMem : hDC, lpCnt->crColor[CNTCOLOR_TEXT] );
//        }

          // Draw the record separator line if turned on.
          if( lpCnt->lpCI->flCntAttr & CA_RECSEPARATOR )
          {
            MoveToEx( bUseMemDC ? hdcMem : hDC, lpRect->left, yPos+cyRow-1, NULL );
            LineTo( bUseMemDC ? hdcMem : hDC, lpRect->right, yPos+cyRow-1 );
          }

          // Draw the focus rect if this record has the focus.
          if( lpRec->flRecAttr & CRA_CURSORED )
          {
            rectFocus.left  = lpRect->left;
            rectFocus.right = lpRect->right-1;
            DrawFocusRect( bUseMemDC ? hdcMem : hDC, &rectFocus );

            // Draw a double width focus rect if desired.
            if( lpCnt->lpCI->bDrawFocusRect > 1 || lpCnt->bSimulateMulSel )
            {
              InflateRect( &rectFocus, -1, -1 );
              DrawFocusRect( bUseMemDC ? hdcMem : hDC, &rectFocus );
            }
          }

          // If this record was selected set the colors back to normal.
          if( bRecSelected )
          {
            fEtoOp = ETO_CLIPPED;
            SetBkColor( bUseMemDC ? hdcMem : hDC, lpCnt->crColor[CNTCOLOR_CNTBKGD] );
            SetTextColor( bUseMemDC ? hdcMem : hDC, lpCnt->crColor[CNTCOLOR_TEXT] );
          }

          // advance to next data record
          lpRec = lpRec->lpNext;
        } // end for loop
      } // end if (cxCol)

      // advance to next column
      xOff += cxCol;
      lpCol = lpCol->lpNext;
    }

    if (bUseMemDC)
    {
      // Copy the bitmap to the screen.
      BitBlt( hDC, 0, 0, lpCnt->cxClient, lpCnt->cyClient, hdcMem, 0, 0, SRCCOPY );
      
      DeleteObject( SelectObject( hdcMem, hbmMemOld ) );
      DeleteDC( hdcMem );
    }

    return;
}

