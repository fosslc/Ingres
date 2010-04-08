/****************************************************************************
 * Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * ICNPAINT.C
 *
 * Contains WM_PAINT logic for the DETAIL view format.
 ****************************************************************************/

#include <windows.h>
#include "cntdll.h"

/****************************************************************************
 * PaintIconView
 *
 * Purpose:
 *  Paints the container in an icon view format.
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

VOID WINAPI PaintIconView( HWND hWnd, LPCONTAINER lpCnt, HDC hDC, LPPAINTSTRUCT lpPs )
{
    LPRECORDCORE lpRec;          // pointer to LPRECORDCORE
    LPZORDERREC  lpOrderRec;     // pointer to Record order list
    LPRECT   lpRect, lpIconRect, lpTextRect;
    RECT     CntClientRect;
    RECT     textRect, rectFocus, IconRect;
    char     szData[MAX_STRING_LEN];
    int      cyChar;            // height of char
    int      cxChar,  cyRow;    // width and height of char
    int      cxChar2, cyRow2;   // half width and height of char
    int      cxChar4, cyRow4;   // quarter width and height of char
    int      cyTitleArea=0;
    int      cxWidth, cxPxlWidth;
    int      cxCharTtl, cxCharTtl2;
    int      cyCharTtl4, cyChar2;
    int      cyLineOff;
    int      xOff, yOff, xPos, yPos;
    UINT     uLen;
    UINT     fAlign;
    int      xOrgBmp, yOrgBmp, xExt, yExt;
    int      xTrnPxl, yTrnPxl;
    BOOL     bBtnPressed, bRet;
    UINT     fEtoOp;            // flags for ExtTextOut style
    BOOL     bRecSelected, bRecInUse;
    BOOL     bUseMemDC;
    HDC      hdcMem;
    HBITMAP  hbmMem, hbmMemOld;
    BOOL     bDrawIcon, bDrawText, bWordWrap=FALSE;

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
    CntClientRect.left   = 0;
    CntClientRect.top    = 0;
    CntClientRect.right  = lpCnt->cxClient;
    CntClientRect.bottom = lpCnt->cyClient;
    lpRect     = &CntClientRect;

    SetBkColor( bUseMemDC ? hdcMem : hDC, lpCnt->crColor[CNTCOLOR_CNTBKGD] );

    // paint the background unless we are simply updating a button
    if( !EqualRect( &lpPs->rcPaint, &lpCnt->rectPressedBtn ) )
    {
      if( lpCnt->lpCI->flCntAttr & CA_OWNERPNTBK && lpCnt->lpfnPaintBk )
      {
        // Call the app to paint the background if owner-paint is enabled.
        (*lpCnt->lpfnPaintBk)( hWnd, NULL, bUseMemDC ? hdcMem : hDC, &lpPs->rcPaint, BK_GENERAL );
      }
      else
      {
        ExtTextOut( bUseMemDC ? hdcMem : hDC, lpPs->rcPaint.left, lpPs->rcPaint.top, ETO_OPAQUE, &lpPs->rcPaint, NULL, 0, NULL );
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
    
    // see if doing word wrapping
    if (lpCnt->lpCI->aiInfo.aiType & CAI_WORDWRAP)
      bWordWrap = TRUE;

    // calc the upper left corner of the workspace
//    xOff = -lpCnt->nHscrollPos;
//    yOff = -lpCnt->nVscrollPos + cyTitleArea;
    xOff = -lpCnt->lpCI->rcViewSpace.left;
    yOff = -lpCnt->lpCI->rcViewSpace.top + cyTitleArea;
    
    lpIconRect = &IconRect; // pointer to icon rectangle
    lpTextRect = &textRect; // pointer to text rectangle

    // start at the tail and work back towards the head (the head should be drawn on top)
    lpOrderRec = lpCnt->lpCI->lpRecOrderTail;

    while (lpOrderRec)
    {
      // get the record
      lpRec = lpOrderRec->lpRec;

      if ( StateTest( lpCnt, CNTSTATE_MOVINGFLD ) && 
           lpOrderRec==lpCnt->lpCI->lpRecOrderHead)
      {
        // if moving icon, do not draw the top level icon - I invalidate it so it will
        // disappear when I move it.
        ; // do nothing
      }
      else
      {
        // define rectangle for this record's icon
        lpIconRect->left   = xOff + lpRec->ptIconOrg.x;
        lpIconRect->top    = yOff + lpRec->ptIconOrg.y;
        lpIconRect->right  = lpIconRect->left + lpCnt->lpCI->ptBmpOrIcon.x;
        lpIconRect->bottom = lpIconRect->top  + lpCnt->lpCI->ptBmpOrIcon.y;

        // if icon rectangle intersects with the painting rectangle, we need to 
        // repaint the icon
        if (WithinRect ( lpIconRect, &lpPs->rcPaint ))
          bDrawIcon = TRUE;
        else
          bDrawIcon = FALSE;

        // define rectangle for this record's text
        if (lpRec->lpszIcon)
        {
          // Create the rectangle that bounds the text 
          CreateIconTextRect( hDC, lpCnt, lpRec->ptIconOrg, lpRec->lpszIcon, lpTextRect );
          OffsetRect( lpTextRect, xOff, yOff );
        
          // if text rectangle intersects with the painting rectangle, we need to 
          // repaint the text
          if (WithinRect ( lpTextRect, &lpPs->rcPaint ))
            bDrawText = TRUE;
          else
            bDrawText = FALSE;
        }
        else
          bDrawText = FALSE;

        if (bDrawIcon) // paint the icon
        {
          if( lpCnt->lpCI->flCntAttr & CA_DRAWBMP )
          {
            // Draw bitmap at specified location
            DrawBitmapXY(lpCnt, lpRec, bUseMemDC ? hdcMem : hDC, lpIconRect->left, lpIconRect->top, 
                         lpIconRect);
          }
          else if( lpCnt->lpCI->flCntAttr & CA_DRAWICON )
          {
            // Draw icon at specified location
            DrawIconXY(lpCnt, lpRec, bUseMemDC ? hdcMem : hDC, lpIconRect->left, lpIconRect->top, 
                       lpCnt->lpCI->ptBmpOrIcon.x, lpCnt->lpCI->ptBmpOrIcon.y);
          }
          else // not specified - first check to see if icon exists and if not chk for bitmap
          {
            // Draw bitmap at specified location
            bRet = DrawIconXY(lpCnt, lpRec, bUseMemDC ? hdcMem : hDC, lpIconRect->left, lpIconRect->top,
                              lpCnt->lpCI->ptBmpOrIcon.x, lpCnt->lpCI->ptBmpOrIcon.y);
            if (bRet==FALSE)
            {
              DrawBitmapXY(lpCnt, lpRec, bUseMemDC ? hdcMem : hDC, lpIconRect->left, lpIconRect->top, 
                           lpIconRect);
            }
          }
        }

        if (bDrawText)
        {
          // lpRect is the rectangle that surrounds the text for drawing selection
          // and color rectangles and the focus rect
          CopyRect( lpRect, lpTextRect );
          InflateRect( lpRect, 1, 1 );

          // Setup the focus rect.
          rectFocus.top    = lpRect->top;
          rectFocus.bottom = lpRect->bottom;

          bRecInUse = bRecSelected = FALSE;

          // the text will be transparent unless otherwise specified below
          SetBkMode( bUseMemDC ? hdcMem : hDC, TRANSPARENT );

          // If this record is selected adjust the colors.
          if( lpRec->flRecAttr & CRA_SELECTED )
          {
            bRecSelected = TRUE;
            fEtoOp = ETO_CLIPPED | ETO_OPAQUE;
            SetBkColor( bUseMemDC ? hdcMem : hDC, lpCnt->crColor[CNTCOLOR_HIGHLIGHT] );
            SetTextColor( bUseMemDC ? hdcMem : hDC, lpCnt->crColor[CNTCOLOR_HITEXT] );
            SetBkMode( bUseMemDC ? hdcMem : hDC, OPAQUE ); // added this DrawText
          }

          // If this record is in-use adjust the background mode.
          if( lpRec->flRecAttr & CRA_INUSE )
          {
            bRecInUse = TRUE;
            SetBkMode( bUseMemDC ? hdcMem : hDC, TRANSPARENT );
            fEtoOp &= ~ETO_OPAQUE;
          }

          // copy the record icon name - if no name, nothing appears
          if (lpRec->lpszIcon)
            lstrcpy(szData, lpRec->lpszIcon);
          else
            lstrcpy(szData," \0");
          uLen = lstrlen(szData);
      
          // setup the alignment - center the text horizontally
          fAlign  = TA_TOP;
          fAlign |= TA_CENTER;
          xPos = lpTextRect->left + (lpTextRect->right - lpTextRect->left) / 2;
          yPos = lpTextRect->top;
          cyLineOff = 0;
  
          // If this record is not selected or in use check for record colors.
          if( !bRecSelected && !bRecInUse )
          {
            // Set the background attributes 
            if( lpRec->bClrRecBk )
            {
              // Explicit record color is 1st priority.
              fEtoOp |= ETO_OPAQUE;
              SetBkMode( bUseMemDC ? hdcMem : hDC, OPAQUE ); // added this DrawText
              SetBkColor( bUseMemDC ? hdcMem : hDC, lpRec->crRecBk );
            }

            if( lpRec->bClrRecText )
              SetTextColor( bUseMemDC ? hdcMem : hDC, lpRec->crRecText );
          }

          // Paint the in-use emphasis in the field background first.
          if( bRecInUse )
            PaintInUseRect( hWnd, lpCnt, bUseMemDC ? hdcMem : hDC, lpRect );

          // draw the background rectangle - note that ExtTextOut only fills in the 
          // background of the text
          if (bRecSelected || bRecInUse || lpRec->bClrRecBk)
            ExtTextOut( bUseMemDC ? hdcMem : hDC, xPos, yPos, fEtoOp, lpRect, "", 0, (LPINT)NULL );

          // Draw the text
          if ( bWordWrap )
            DrawText( bUseMemDC ? hdcMem : hDC, szData, uLen, lpTextRect, 
                      DT_NOPREFIX | DT_TOP | DT_CENTER | DT_WORDBREAK );
          else
            DrawText( bUseMemDC ? hdcMem : hDC, szData, uLen, lpTextRect, 
                      DT_NOPREFIX | DT_TOP | DT_CENTER );

          // Restore any colors that were changed.
          if( lpRec->bClrRecBk )
          {
            fEtoOp &= ~ETO_OPAQUE;
            SetBkMode( bUseMemDC ? hdcMem : hDC, TRANSPARENT ); // added this DrawText
            SetBkColor( bUseMemDC ? hdcMem : hDC, lpCnt->crColor[CNTCOLOR_CNTBKGD] );
          }
          if( lpRec->bClrRecText )
            SetTextColor( bUseMemDC ? hdcMem : hDC, lpCnt->crColor[CNTCOLOR_TEXT] );

          // Draw the focus rect if this record has the focus.
          if( lpRec->flRecAttr & CRA_CURSORED )
          {
            rectFocus.left  = lpRect->left;
            rectFocus.right = lpRect->right;
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
            SetBkMode( bUseMemDC ? hdcMem : hDC, TRANSPARENT ); // added this DrawText
            SetBkColor( bUseMemDC ? hdcMem : hDC, lpCnt->crColor[CNTCOLOR_CNTBKGD] );
            SetTextColor( bUseMemDC ? hdcMem : hDC, lpCnt->crColor[CNTCOLOR_TEXT] );
          }
        } // end if bDrawText
      }

      // move to the previous record
      lpOrderRec = lpOrderRec->lpPrev;
    }
  
   /*-----------------------------------------------------------------------*
    * Draw the container title (if it has one).
    *-----------------------------------------------------------------------*/
    CntClientRect.left   = 0;
    CntClientRect.top    = 0;
    CntClientRect.right  = lpCnt->cxClient;
    CntClientRect.bottom = lpCnt->cyClient;
    lpRect     = &CntClientRect;

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

    if (bUseMemDC)
    {
      // Copy the bitmap to the screen.
      BitBlt( hDC, 0, 0, lpCnt->cxClient, lpCnt->cyClient, hdcMem, 0, 0, SRCCOPY );
      
      DeleteObject( SelectObject( hdcMem, hbmMemOld ) );
      DeleteDC( hdcMem );
    }

    return;
}

/****************************************************************************
 * DrawBitmapXY
 *
 * Purpose:
 *  Paints a bitmap with the specified size at the specified location
 *
 * Parameters:
 *  LPCONTAINER   lpCnt       - pointer to container struct
 *  LPRECORDCORE  lpRec       - pointer to current record
 *  int           nXstart     - bitmap left
 *  int           nYstart     - bitmap top
 *  LPRECT        lpImageRect - bounding rectangle
 *
 * Return Value:
 *  TRUE if bitmap can be drawn, otherwise FALSE
 ****************************************************************************/
BOOL DrawBitmapXY(LPCONTAINER lpCnt, LPRECORDCORE lpRec, HDC hDC, int nXstart, int nYstart, 
                  LPRECT lpImageRect)
{
  int xTrnPxl, yTrnPxl;

  // user specified bitmaps - see if they exist
  if (lpCnt->iView & CV_MINI)
  {
    if (lpRec->hMiniBmp)
    {
      if( lpRec->lpTransp && lpRec->lpTransp->fMiniBmpTransp )
      {
        // using transparent bitmap
        xTrnPxl = lpRec->lpTransp->ptMiniBmp.x;
        yTrnPxl = lpRec->lpTransp->ptMiniBmp.y;
        DrawTransparentBitmap( hDC, lpRec->hMiniBmp, nXstart, nYstart, xTrnPxl, yTrnPxl,
                               lpImageRect );
      }
      else
        DrawBitmap( hDC, lpRec->hMiniBmp, nXstart, nYstart, lpImageRect );
    }
    else 
      return FALSE;
  }
  else
  {
    if (lpRec->hBmp)
    {
      if( lpRec->lpTransp && lpRec->lpTransp->fBmpTransp )
      {
        // using transparent bitmap
        xTrnPxl = lpRec->lpTransp->ptBmp.x;
        yTrnPxl = lpRec->lpTransp->ptBmp.y;
        DrawTransparentBitmap( hDC, lpRec->hBmp, nXstart, nYstart, xTrnPxl, yTrnPxl,
                               lpImageRect );
      }
      else
        DrawBitmap( hDC, lpRec->hBmp, nXstart, nYstart, lpImageRect );
    }
    else 
      return FALSE;
  }

  return TRUE;
}

/****************************************************************************
 * DrawIconXY
 *
 * Purpose:
 *  Paints a icon with the specified size at the specified location
 *
 * Parameters:
 *  LPCONTAINER   lpCnt       - pointer to container struct
 *  LPRECORDCORE  lpRec       - pointer to current record
 *  int           nXstart     - bitmap left
 *  int           nYstart     - bitmap top
 *  int           nXwidth     - bitmap width
 *  int           nYheight    - bitmap height
 *
 * Return Value:
 *  TRUE if bitmap can be drawn, otherwise FALSE
 ****************************************************************************/
BOOL DrawIconXY(LPCONTAINER lpCnt, LPRECORDCORE lpRec, HDC hDC, int nXstart, int nYstart, 
                int nWidth, int nHeight)
{
  // user specified icons - see if they exist
  if (lpCnt->iView & CV_MINI)
  {
    // draw mini icons
    if (lpRec->hMiniIcon)
    {
#ifdef WIN32
      DrawIconEx( hDC, nXstart, nYstart, lpRec->hMiniIcon, nWidth, nHeight, 0, NULL,
                  DI_NORMAL );
#else 
      DrawIcon( hDC, nXstart, nYstart, lpRec->hMiniIcon );
#endif
    }
    else 
      return FALSE;
  }
  else
  {
    // draw regular size icons
    if (lpRec->hIcon)
    {
#ifdef WIN32    
      DrawIconEx( hDC, nXstart, nYstart, lpRec->hIcon, nWidth, nHeight, 0, NULL,
                  DI_NORMAL );
#else 
      DrawIcon( hDC, nXstart, nYstart, lpRec->hIcon );
#endif
    }   
    else 
      return FALSE;
  }

  return TRUE;
}
