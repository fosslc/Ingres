/****************************************************************************
 * Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * SPINPNT.C
 *
 * Contains WM_PAINT logic for the Spin control.
 ****************************************************************************/

#include <windows.h>
#include "spindll.h"

// Array of default colors, matching the order of SPC_* color indices.
int crDefColor[SPINCOLORS]={ COLOR_BTNFACE,       // button face color
                             COLOR_BTNTEXT,       // direction indicator
                             COLOR_WINDOWFRAME,   // border around the button halves
                             COLOR_BTNHIGHLIGHT,  // highlight color used for 3D look
                             COLOR_BTNSHADOW      // shadow color used for 3D look
                            };

VOID NEAR Draw3DButtonRect( HDC hDC, HPEN hPenHigh, HPEN hPenShadow,
                            int x1, int y1, int x2, int y2, BOOL bClicked );
VOID NEAR DrawBitmap( HDC hdc, HBITMAP hBitmap, int xOrg, int yOrg, LPRECT lpRect );
VOID NEAR DrawTransparentBitmap(HDC hdc, HBITMAP hBitmap, int xOrg, int yOrg,
                                int xTransPxl, int yTransPxl, LPRECT lpRect );
VOID NEAR DrawDisabledBitmap( HDC hDC, HBITMAP hBitmap, int xOrg, int yOrg,
                              COLORREF crFace, COLORREF crHigh, COLORREF crShadow, LPRECT lpRect );
VOID NEAR GetBitmapExt( HDC hdc, HBITMAP hBitmap, LPINT xExt, LPINT yExt );

#define RGB_WHITE   RGB(255,255,255)
#define RGB_BLACK   RGB(0,0,0)
#define PSDPxax     0x00B8074A

/****************************************************************************
 * PaintSpinButton
 *
 * Purpose:
 *  Handles all WM_PAINT messages for the control and paints the control
 *  for the current state, whether it be clicked or disabled.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the spin window
 *  LPSPIN        lpSpin      - pointer to spin struct
 *  HDC           hDC         - device context for the spin window
 *  LPPAINTSTRUCT lpPs        - paint struct for the spin
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI PaintSpinButton( HWND hWnd, LPSPIN lpSpin, HDC hDC, LPPAINTSTRUCT lpPs )
    {
    HBRUSH hBrushArrow;
    HBRUSH hBrushFace;
    LPRECT lpRect;
    RECT   rect, rectInc, rectDec;
    POINT  ptArrow1[3];
    POINT  ptArrow2[3];
    int    xAdd1=0, yAdd1=0;
    int    xAdd2=0, yAdd2=0;
    int    xPos, yPos;
    int    xExt, yExt;
    int    xOrgBmp, yOrgBmp;
    int    cx,  cy;    // Whole dimensions
    int    cx2, cy2;   // Half dimensions
    int    cx4, cy4;   // Quarter dimensions


    lpRect = &rect;

    GetClientRect( hWnd, lpRect );

    hBrushFace  = CreateSolidBrush( lpSpin->crSpin[SPC_FACE] );
    hBrushArrow = CreateSolidBrush( lpSpin->crSpin[SPC_ARROW] );

    // These values are extremely cheap to calculate for the amount
    // we are going to use them.
    cx = lpRect->right  - lpRect->left;
    cy = lpRect->bottom - lpRect->top;
    cx2=cx  >> 1;
    cy2=cy  >> 1;
    cx4=cx2 >> 1;
    cy4=cy2 >> 1;

    // If one half is depressed, set the x/yAdd varaibles that we use
    // to shift the small arrow image down and right.
    if( !StateTest(lpSpin, SPSTATE_MOUSEOUT) )
      {
      if(StateTest(lpSpin, SPSTATE_UPCLICK | SPSTATE_LEFTCLICK) )
        {
        xAdd1 = 1;
        yAdd1 = 1;
        }

      if( StateTest(lpSpin, SPSTATE_DOWNCLICK | SPSTATE_RIGHTCLICK) )
        {
        xAdd2 = 1;
        yAdd2 = 1;
        }
      }

    // Draw the face color and the outer frame
    SelectObject( hDC, hBrushFace );
    SelectObject( hDC, lpSpin->hPen[SPC_FRAME] );
    Rectangle( hDC, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom );

    // Draw the arrows depending on the orientation.
    if( SPS_VERTICAL & lpSpin->dwStyle )
      {
      // Draw the horizontal center line.
      MoveToEx( hDC, 0,  cy2, NULL );
      LineTo( hDC, cx, cy2 );

     /*---------------------------------------------------------------------*
      * We do one of three modifications for drawing the borders:
      *  1) Both halves un-clicked.
      *  2) Top clicked,   bottom unclicked.
      *  3) Top unclicked, bottom clicked.
      *
      * Case 1 is xAdd1==xAdd2==0
      * Case 2 is xAdd1==1, xAdd2=0
      * Case 3 is xAdd1==0, xAdd2==1
      *---------------------------------------------------------------------*/

      // Draw top and bottom buttons borders.
      Draw3DButtonRect( hDC, lpSpin->hPen[SPC_HIGHLIGHT], lpSpin->hPen[SPC_SHADOW],
                        0,  0,  cx-1, cy2,  xAdd1 );

      Draw3DButtonRect( hDC, lpSpin->hPen[SPC_HIGHLIGHT], lpSpin->hPen[SPC_SHADOW],
                        0, cy2, cx-1, cy-1, xAdd2 );

      // Draw a bitmap on the decrement button if one is defined.
      if( lpSpin->hBmpDec )
        {
        CopyRect( &rectDec, lpRect );
        rectDec.bottom = cy2;

        xPos = rectDec.left;
        yPos = rectDec.top;

        // Get the bitmap extents.
        GetBitmapExt( hDC, lpSpin->hBmpDec, &xExt, &yExt );

        // Center the bitmap horizontally and vertically.
        xOrgBmp = xAdd1 + xPos + cx2   - xExt/2;
        yOrgBmp = yAdd1 + yPos + cy2/2 - yExt/2;

        // Apply any offset.
        xOrgBmp += lpSpin->xOffBmpDec;
        yOrgBmp += lpSpin->yOffBmpDec;

        if( StateTest(lpSpin, SPSTATE_UPGRAYED) )
          {
          DrawDisabledBitmap( hDC, lpSpin->hBmpDec, xOrgBmp, yOrgBmp,
                              lpSpin->crSpin[SPC_FACE], lpSpin->crSpin[SPC_HIGHLIGHT],
                              lpSpin->crSpin[SPC_SHADOW], &rectDec );
          }
        else
          {
          if( lpSpin->bDecBmpTrans )
            DrawTransparentBitmap( hDC, lpSpin->hBmpDec, xOrgBmp, yOrgBmp, 
                                   lpSpin->xTransPxlDec, lpSpin->yTransPxlDec, &rectDec );
          else
            DrawBitmap( hDC, lpSpin->hBmpDec, xOrgBmp, yOrgBmp, &rectDec );
          }
        }
      else
        {
        if( !(lpSpin->flSpinAttr & SA_NOTOPARROW) )
          {
          // Select default line color.
          SelectObject( hDC, lpSpin->hPen[SPC_ARROW] );

          // Draw the arrows depending on the enable state.
          xPos = cx2-1;
          yPos = cy4;
          if( StateTest(lpSpin, SPSTATE_UPGRAYED) )
            {
            // Draw arrow color lines in the upper left of the
            // top arrow and on the top of the bottom arrow.
            // Pen was already selected as a default.
            MoveToEx( hDC, xPos,   yPos-2, NULL );      // Top arrow
            LineTo(   hDC, xPos-3, yPos+1 );
      
            // Draw highlight color lines in the bottom of the
            // top arrow and on the lower right of the bottom arrow.
            SelectObject( hDC, lpSpin->hPen[SPC_HIGHLIGHT] );
            MoveToEx( hDC, xPos-3, yPos+1, NULL );      // Top arrow
            LineTo(   hDC, xPos+3, yPos+1 );
            }
          else
            {
            // Draw the top arrow polygon.
            ptArrow1[0].x = xAdd1 + xPos;
            ptArrow1[0].y = yAdd1 + yPos-2;
            ptArrow1[1].x = xAdd1 + xPos-3;
            ptArrow1[1].y = yAdd1 + yPos+1;
            ptArrow1[2].x = xAdd1 + xPos+3;
            ptArrow1[2].y = yAdd1 + yPos+1;
            SelectObject( hDC, hBrushArrow );
            Polygon( hDC, (LPPOINT)ptArrow1, 3 );
            }
          }
        }

      // Draw a bitmap on the increment button if one is defined.
      if( lpSpin->hBmpInc )
        {
        CopyRect( &rectInc, lpRect );
        rectInc.top = cy2;
        rectInc.bottom -= yAdd2;

        xPos = rectInc.left;
        yPos = rectInc.top;

        // Get the bitmap extents.
        GetBitmapExt( hDC, lpSpin->hBmpInc, &xExt, &yExt );

        // Center the bitmap horizontally and vertically.
        xOrgBmp = xAdd2 + xPos + cx2   - xExt/2;
        yOrgBmp = yAdd2 + yPos + cy2/2 - yExt/2;

        // Apply any offset.
        xOrgBmp += lpSpin->xOffBmpInc;
        yOrgBmp += lpSpin->yOffBmpInc;

        if( StateTest(lpSpin, SPSTATE_DOWNGRAYED) )
          {
          DrawDisabledBitmap( hDC, lpSpin->hBmpInc, xOrgBmp, yOrgBmp,
                              lpSpin->crSpin[SPC_FACE], lpSpin->crSpin[SPC_HIGHLIGHT],
                              lpSpin->crSpin[SPC_SHADOW], &rectInc );
          }
        else
          {
          if( lpSpin->bIncBmpTrans )
            DrawTransparentBitmap( hDC, lpSpin->hBmpInc, xOrgBmp, yOrgBmp, 
                                   lpSpin->xTransPxlInc, lpSpin->yTransPxlInc, &rectInc );
          else
            DrawBitmap( hDC, lpSpin->hBmpInc, xOrgBmp, yOrgBmp, &rectInc );
          }
        }
      else
        {
        if( !(lpSpin->flSpinAttr & SA_NOBOTTOMARROW) )
          {
          // Select default line color.
          SelectObject( hDC, lpSpin->hPen[SPC_ARROW] );

          // Draw the arrows depending on the enable state.
          xPos = cx2-1;
          yPos = cy2+cy4;
          if( StateTest(lpSpin, SPSTATE_DOWNGRAYED) )
            {
            // Draw arrow color lines in the upper left of the
            // top arrow and on the top of the bottom arrow.
            // Pen was already selected as a default.
            MoveToEx( hDC, xPos-3, yPos-2, NULL );  // Bottom arrow
            LineTo(   hDC, xPos+3, yPos-2 );
      
            // Draw highlight color lines in the bottom of the
            // top arrow and on the lower right of the bottom arrow.
            SelectObject( hDC, lpSpin->hPen[SPC_HIGHLIGHT] );
            MoveToEx( hDC, xPos+3, yPos-2, NULL );  // Bottom arrow
            LineTo(   hDC, xPos,   yPos+1 );
            SetPixel( hDC, xPos,   yPos+1, lpSpin->crSpin[SPC_HIGHLIGHT] );
            }
          else
            {
            // Draw the bottom arrow polygon.
            ptArrow2[0].x = xAdd2 + xPos;
            ptArrow2[0].y = yAdd2 + yPos+1;
            ptArrow2[1].x = xAdd2 + xPos-3;
            ptArrow2[1].y = yAdd2 + yPos-2;
            ptArrow2[2].x = xAdd2 + xPos+3;
            ptArrow2[2].y = yAdd2 + yPos-2;
            SelectObject( hDC, hBrushArrow );
            Polygon( hDC, (LPPOINT)ptArrow2, 3 );
            }
          }
        }
      }
    else
      {
      // Draw the vertical center line, assume the frame color is selected.
      MoveToEx( hDC, cx2, 0, NULL );
      LineTo(   hDC, cx2, cy );

     /*---------------------------------------------------------------------*
      * We do one of three modifications for drawing the borders:
      *  1) Both halves un-clicked.
      *  2) Left clicked,   right unclicked.
      *  3) Left unclicked, right clicked.
      *
      * Case 1 is xAdd1==xAdd2==0
      * Case 2 is xAdd1==1, xAdd2=0
      * Case 3 is xAdd1==0, xAdd2==1
      *---------------------------------------------------------------------*/

      // Draw left and right buttons borders.
      Draw3DButtonRect( hDC, lpSpin->hPen[SPC_HIGHLIGHT], lpSpin->hPen[SPC_SHADOW],
                        0, 0, cx2, cy-1, xAdd1);

      Draw3DButtonRect( hDC, lpSpin->hPen[SPC_HIGHLIGHT], lpSpin->hPen[SPC_SHADOW],
                        cx2, 0, cx-1, cy-1, xAdd2);

      // Draw a bitmap on the decrement button if one is defined.
      if( lpSpin->hBmpDec )
        {
        CopyRect( &rectDec, lpRect );
        rectDec.right = cx2;

        xPos = rectDec.left;
        yPos = rectDec.top;

        // Get the bitmap extents.
        GetBitmapExt( hDC, lpSpin->hBmpDec, &xExt, &yExt );

        // Center the bitmap horizontally and vertically.
        xOrgBmp = xAdd1 + xPos + cx2/2 - xExt/2;
        yOrgBmp = yAdd1 + yPos + cy2   - yExt/2;

        // Apply any offset.
        xOrgBmp += lpSpin->xOffBmpDec;
        yOrgBmp += lpSpin->yOffBmpDec;

        if( StateTest(lpSpin, SPSTATE_LEFTGRAYED) )
          {
          DrawDisabledBitmap( hDC, lpSpin->hBmpDec, xOrgBmp, yOrgBmp,
                              lpSpin->crSpin[SPC_FACE], lpSpin->crSpin[SPC_HIGHLIGHT],
                              lpSpin->crSpin[SPC_SHADOW], &rectDec );
          }
        else
          {
          if( lpSpin->bDecBmpTrans )
            DrawTransparentBitmap( hDC, lpSpin->hBmpDec, xOrgBmp, yOrgBmp, 
                                   lpSpin->xTransPxlDec, lpSpin->yTransPxlDec, &rectDec );
          else
            DrawBitmap( hDC, lpSpin->hBmpDec, xOrgBmp, yOrgBmp, &rectDec );
          }
        }
      else
        {
        if( !(lpSpin->flSpinAttr & SA_NOLEFTARROW) )
          {
          // Select default line color.
          SelectObject( hDC, lpSpin->hPen[SPC_ARROW] );

          // Draw the arrows depending on the enable state.
          xPos = cx4;
          yPos = cy2-1;
          if( StateTest(lpSpin, SPSTATE_LEFTGRAYED) )
            {
            // Draw arrow color lines in the upper left of the
            // left arrow and on the left of the right arrow.
            // Pen was already selected as a default.
            MoveToEx( hDC, xPos-2, yPos, NULL );        // Left arrow
            LineTo(   hDC, xPos+1, yPos-3 );
      
            // Draw highlight color lines in the bottom of the
            // top arrow and on the ;pwer right of the bottom arrow.
            SelectObject( hDC, lpSpin->hPen[SPC_HIGHLIGHT] );
            MoveToEx( hDC, xPos+1, yPos-3, NULL );
            LineTo(   hDC, xPos+1, yPos+3 );
            }
          else
            {
            // Draw the left arrow polygon.
            ptArrow1[0].x = xAdd1 + xPos-2;
            ptArrow1[0].y = yAdd1 + yPos;
            ptArrow1[1].x = xAdd1 + xPos+1;
            ptArrow1[1].y = yAdd1 + yPos+3;
            ptArrow1[2].x = xAdd1 + xPos+1;
            ptArrow1[2].y = yAdd1 + yPos-3;
            SelectObject( hDC, hBrushArrow );
            Polygon( hDC, (LPPOINT)ptArrow1, 3 );
            }
          }
        }

      // Draw a bitmap on the increment button if one is defined.
      if( lpSpin->hBmpInc )
        {
        CopyRect( &rectInc, lpRect );
        rectInc.left = cx2;
        rectInc.right -= xAdd2;

        xPos = rectInc.left;
        yPos = rectInc.top;

        // Get the bitmap extents.
        GetBitmapExt( hDC, lpSpin->hBmpInc, &xExt, &yExt );

        // Center the bitmap horizontally and vertically.
        xOrgBmp = xAdd2 + xPos + cx2/2 - xExt/2;
        yOrgBmp = yAdd2 + yPos + cy2   - yExt/2;

        // Apply any offset.
        xOrgBmp += lpSpin->xOffBmpInc;
        yOrgBmp += lpSpin->yOffBmpInc;

        if( StateTest(lpSpin, SPSTATE_RIGHTGRAYED) )
          {
          DrawDisabledBitmap( hDC, lpSpin->hBmpInc, xOrgBmp, yOrgBmp,
                              lpSpin->crSpin[SPC_FACE], lpSpin->crSpin[SPC_HIGHLIGHT],
                              lpSpin->crSpin[SPC_SHADOW], &rectInc );
          }
        else
          {
          if( lpSpin->bIncBmpTrans )
            DrawTransparentBitmap( hDC, lpSpin->hBmpInc, xOrgBmp, yOrgBmp, 
                                   lpSpin->xTransPxlInc, lpSpin->yTransPxlInc, &rectInc );
          else
            DrawBitmap( hDC, lpSpin->hBmpInc, xOrgBmp, yOrgBmp, &rectInc );
          }
        }
      else
        {
        if( !(lpSpin->flSpinAttr & SA_NORIGHTARROW) )
          {
          // Select default line color.
          SelectObject( hDC, lpSpin->hPen[SPC_ARROW] );

          // Draw the arrows depending on the enable state.
          xPos = cx2+cx4;
          yPos = cy2-1;
          if( StateTest(lpSpin, SPSTATE_RIGHTGRAYED) )
            {
            // Draw arrow color lines in the upper left of the
            // left arrow and on the left of the right arrow.
            // Pen was already selected as a default.
            MoveToEx( hDC, xPos-2, yPos-3, NULL );      // Right arrow
            LineTo(   hDC, xPos-2, yPos+3 );
      
            // Draw highlight color lines in the bottom of the
            // top arrow and on the ;pwer right of the bottom arrow.
            SelectObject( hDC, lpSpin->hPen[SPC_HIGHLIGHT] );
            MoveToEx( hDC, xPos+1, yPos, NULL );
            LineTo(   hDC, xPos-2, yPos+3 );
            }
          else
            {
            // Draw the right arrow polygon.
            ptArrow2[0].x = xAdd2 + xPos+1;
            ptArrow2[0].y = yAdd2 + yPos;
            ptArrow2[1].x = xAdd2 + xPos-2;
            ptArrow2[1].y = yAdd2 + yPos+3;
            ptArrow2[2].x = xAdd2 + xPos-2;
            ptArrow2[2].y = yAdd2 + yPos-3;
            SelectObject( hDC, hBrushArrow );
            Polygon( hDC, (LPPOINT)ptArrow2, 3 );
            }
          }
        }
      }

    // Clean up
    SelectObject( hDC, GetStockObject( BLACK_BRUSH ) );
    DeleteObject( hBrushFace );
    DeleteObject( hBrushArrow );

    return;
    }

/****************************************************************************
 * Draw3DButtonRect
 *
 * Purpose:
 *  Draws the 3D button look within a given rectangle.  This rectangle
 *  is assumed to be bounded by a one pixel black border, so everything
 *  is bumped in by one.
 *
 * Parameters:
 *  HDC           hDC         - DC to draw to
 *  HPEN          hPenHigh    - highlight color pen
 *  HPEN          hPenShadow  - shadow color pen
 *  int           x1          - Upper left corner x
 *  int           y1          - Upper left corner y
 *  int           x2          - Lower right corner x
 *  int           y2          - Lower right corner y
 *  bClicked    BOOL specifies if the button is down or not (TRUE==DOWN)
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID NEAR Draw3DButtonRect( HDC hDC, HPEN hPenHigh, HPEN hPenShadow,
                            int x1, int y1, int x2, int y2, BOOL bClicked )
    {
    HPEN hPenOrg;

    // Shrink the rectangle to account for borders.
    x1 += 1;
    x2 -= 1;
    y1 += 1;
    y2 -= 1;

    hPenOrg = SelectObject( hDC, hPenShadow );

    if( bClicked )
      {
      // Shadow on left and top edge when clicked.
      MoveToEx( hDC, x1, y2, NULL );
      LineTo( hDC, x1, y1 );
      LineTo( hDC, x2+1, y1 );
      }
    else
      {
      // Lowest shadow line.
      MoveToEx( hDC, x1, y2, NULL );
      LineTo( hDC, x2, y2   );
      LineTo( hDC, x2, y1-1 );

      // Upper shadow line.
      MoveToEx( hDC, x1+1, y2-1, NULL );
      LineTo( hDC, x2-1, y2-1 );
      LineTo( hDC, x2-1, y1   );

      SelectObject( hDC, hPenHigh );

      // Upper highlight line.
      MoveToEx( hDC, x1, y2-1, NULL );
      LineTo( hDC, x1, y1   );
      LineTo( hDC, x2, y1   );
      }

    SelectObject( hDC, hPenOrg );
    return;
    }

/****************************************************************************
 * DrawBitmap
 *
 * Purpose:
 *  Draws a bitmap starting at the passed in x/y origin.
 *
 * Parameters:
 *  HDC           hDC         - dc to draw to
 *  HBITMAP       hBitmap     - handle to bitmap
 *  int           xOrg        - x coord to upper left corner of destination.
 *  int           yOrg        - y coord to upper left corner of destination.
 *  LPRECT        lpRect      - bounding rectangle
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID NEAR DrawBitmap( HDC hdc, HBITMAP hBitmap, int xOrg, int yOrg, LPRECT lpRect )
    {
    BITMAP  bm;
    HDC     hdcMem;
    POINT   ptSize;    
    int     xExt, yExt;

    // Get the bitmap into a memory DC.
    hdcMem = CreateCompatibleDC( hdc );
    SelectObject( hdcMem, hBitmap );
    SetMapMode( hdcMem, GetMapMode( hdc ) );

    // Get the extents of the bitmap.
    GetObject( hBitmap, sizeof(BITMAP), (LPSTR) &bm );
    ptSize.x = bm.bmWidth;
    ptSize.y = bm.bmHeight;
    DPtoLP( hdc, &ptSize, 1 );

    // Check the extents against the clipping rect and adjust if necessary.
    xExt = ptSize.x;
    yExt = ptSize.y;

    if( lpRect )
      {
      if( xOrg >= lpRect->right )
        xOrg = lpRect->right - xExt;
      if( xOrg < lpRect->left )
        xOrg = lpRect->left;
      if( xOrg + xExt > lpRect->right )
        xExt = lpRect->right - xOrg;

      if( yOrg >= lpRect->bottom )
        yOrg = lpRect->bottom - yExt;
      if( yOrg < lpRect->top )
        yOrg = lpRect->top;
      if( yOrg + yExt > lpRect->bottom )
        yExt = lpRect->bottom - yOrg;
      }

    // Copy the bitmap to the screen.
    BitBlt( hdc, xOrg, yOrg, xExt, yExt, hdcMem, 0, 0, SRCCOPY );

    DeleteDC( hdcMem );
    }

/****************************************************************************
 * DrawTransparentBitmap
 *
 * Purpose:
 *  Draws a transparent bitmap starting at the passed in x/y origin.
 *  Transparent meaning that the background color of the bitmap is changed
 *  to match the background color of the destination.
 *
 * Parameters:
 *  HDC           hDC         - dc to draw to.
 *  HBITMAP       hBitmap     - handle to bitmap
 *  int           xOrg        - x coord to upper left corner of destination.
 *  int           yOrg        - y coord to upper left corner of destination.
 *  int           xTransPxl   - x position of pixel which has transparent color.
 *  int           yTransPxl   - y position of pixel which has transparent color.
 *  LPRECT        lpRect      - bounding rectangle
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID NEAR DrawTransparentBitmap( HDC hdc, HBITMAP hBitmap, int xOrg, int yOrg,
                                 int xTransPxl, int yTransPxl, LPRECT lpRect )
    {
    BITMAP   bm;
    COLORREF cColor;
    COLORREF cTransparentColor;
    HBITMAP  hbmAndBack, hbmAndObject, hbmAndMem, hbmSave;
    HBITMAP  hbmBackOld, hbmObjectOld, hbmMemOld, hbmSaveOld;
    HDC      hdcMem, hdcBack, hdcObject, hdcTemp, hdcSave;
    POINT    ptSize;
    int      xExt, yExt;

    hdcTemp = CreateCompatibleDC( hdc );
    SelectObject( hdcTemp, hBitmap );   // Select the bitmap

    GetObject( hBitmap, sizeof(BITMAP), (LPSTR)&bm );
    ptSize.x = bm.bmWidth;            // Get width of bitmap
    ptSize.y = bm.bmHeight;           // Get height of bitmap
    DPtoLP( hdcTemp, &ptSize, 1 );    // Convert from device to logical points

    // Create some DCs to hold temporary data.
    hdcBack   = CreateCompatibleDC( hdc );
    hdcObject = CreateCompatibleDC( hdc );
    hdcMem    = CreateCompatibleDC( hdc );
    hdcSave   = CreateCompatibleDC( hdc );

    // Create a bitmap for each DC.

    // Monochrome DC
    hbmAndBack   = CreateBitmap( ptSize.x, ptSize.y, 1, 1, NULL );

    // Monochrome DC
    hbmAndObject = CreateBitmap( ptSize.x, ptSize.y, 1, 1, NULL );

    hbmAndMem    = CreateCompatibleBitmap( hdc, ptSize.x, ptSize.y );
    hbmSave      = CreateCompatibleBitmap( hdc, ptSize.x, ptSize.y );

    // Each DC must select a bitmap object to store pixel data.
    hbmBackOld   = SelectObject( hdcBack, hbmAndBack );
    hbmObjectOld = SelectObject( hdcObject, hbmAndObject );
    hbmMemOld    = SelectObject( hdcMem, hbmAndMem );
    hbmSaveOld   = SelectObject( hdcSave, hbmSave );

    // Set proper mapping mode.
    SetMapMode( hdcTemp, GetMapMode( hdc ) );

    // Save the bitmap sent here, because it will be overwritten.
    BitBlt( hdcSave, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCCOPY );

    // Get the color designated to be the transparent color from the bitmap.
    // Set the background color of the source DC to the color
    // contained in the parts of the bitmap that should be transparent
    cTransparentColor = GetPixel( hdcTemp, xTransPxl, yTransPxl );
    cColor = SetBkColor( hdcTemp, cTransparentColor );

    // Create the object mask for the bitmap by performing a BitBlt
    // from the source bitmap to a monochrome bitmap.
    BitBlt( hdcObject, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCCOPY );

    // Set the bkgrnd color of the source DC back to the original color.
    SetBkColor( hdcTemp, cColor );

    // Create the inverse of the object mask.
    BitBlt( hdcBack, 0, 0, ptSize.x, ptSize.y, hdcObject, 0, 0, NOTSRCCOPY );

    // Copy the background of the main DC to the destination.
    BitBlt( hdcMem, 0, 0, ptSize.x, ptSize.y, hdc, xOrg, yOrg, SRCCOPY );

    // Mask out the places where the bitmap will be placed.
    BitBlt( hdcMem, 0, 0, ptSize.x, ptSize.y, hdcObject, 0, 0, SRCAND );

    // Mask out the transparent colored pixels on the bitmap.
    BitBlt( hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcBack, 0, 0, SRCAND );

    // XOR the bitmap with the background on the destination DC.
    BitBlt( hdcMem, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCPAINT );

    // Check the extents against the clipping rect and adjust if necessary.
    xExt = ptSize.x;
    yExt = ptSize.y;

    if( lpRect )
      {
      if( xOrg >= lpRect->right )
        xOrg = lpRect->right - xExt;
      if( xOrg < lpRect->left )
        xOrg = lpRect->left;
      if( xOrg + xExt > lpRect->right )
        xExt = lpRect->right - xOrg;

      if( yOrg >= lpRect->bottom )
        yOrg = lpRect->bottom - yExt;
      if( yOrg < lpRect->top )
        yOrg = lpRect->top;
      if( yOrg + yExt > lpRect->bottom )
        yExt = lpRect->bottom - yOrg;
      }

    // Copy the transparent bitmap to the screen.
    BitBlt( hdc, xOrg, yOrg, xExt, yExt, hdcMem, 0, 0, SRCCOPY );

    // Place the original bitmap back into the bitmap sent here.
    BitBlt( hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcSave, 0, 0, SRCCOPY );

    // Delete the memory bitmaps.
    DeleteObject( SelectObject( hdcBack,   hbmBackOld   ) );
    DeleteObject( SelectObject( hdcObject, hbmObjectOld ) );
    DeleteObject( SelectObject( hdcMem,    hbmMemOld    ) );
    DeleteObject( SelectObject( hdcSave,   hbmSaveOld   ) );

    // Delete the memory DCs.
    DeleteDC( hdcMem    );
    DeleteDC( hdcBack   );
    DeleteDC( hdcObject );
    DeleteDC( hdcSave   );
    DeleteDC( hdcTemp   );
    }

/****************************************************************************
 * DrawDisabledBitmap
 *
 * Purpose:
 *  Creates a disabled bitmap ("tinfoil" effect) using the specified bitmap.
 *  The source bitmap can be any color format, or monochrome.  This
 *  function will essentially map the color bitmap down to a few shades
 *  of gray, black, and white which represents the original bitmap in a
 *  raised, 3-D effect.  Borders of contrasting colors in the original 
 *  bitmap will show up as lines of "shadow" in the resulting bitmap.  
 *
 *  This routine does not destroy the original bitmap.
 *
 * History:
 *  This function came with a sample bitmap utility on the MSDN CD.
 *  The original author is Bryan Woodruff. It was modified for use in
 *  this DLL by MWC.
 *
 * Parameters:
 *  HDC           hDC         - DC to draw to
 *  HBITMAP       hBitmap     - the source bitmap to "disable"
 *                              (it must not be currently selected into a DC)
 *  int           xOrg        - x coord to upper left corner of destination.
 *  int           yOrg        - y coord to upper left corner of destination.
 *  COLORREF      crFace      - color for the face
 *  COLORREF      crHigh      - color for the highlight
 *  COLORREF      crShadow    - color for the shadow
 *  LPRECT        lpRect      - bounding rectangle
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID NEAR DrawDisabledBitmap( HDC hDC, HBITMAP hBitmap, int xOrg, int yOrg,
                              COLORREF crFace, COLORREF crHigh, COLORREF crShadow,
                              LPRECT lpRect )
    {
    HDC     hdcColor, hdcMono;
    BITMAP  bmInfo;
    HBITMAP hbmOld, hbmShadow, hbmHighlight;
    HBITMAP hbmDisable=NULL;
    HBRUSH  hbrPat;
    int     xExt, yExt;

    // We essentially need to create 2 monochrome bitmaps:  one defining
    // the "highlight" area on the bitmap, and one defining the "shadow"
    // area, which is how we make the resulting bitmap look 3-D.
    // Both the highlight and shadow bitmaps are created by offsetting
    // calls to BitBlt() using various ROP codes.  
    // hbmDisable is our result bitmap.

    hdcMono  = CreateCompatibleDC( hDC );
    hdcColor = CreateCompatibleDC( hDC );

    if( !hdcMono || !hdcColor )
      return;

    // Create the monochrome and color bitmaps and necessary DCs.
    GetObject( hBitmap, sizeof( BITMAP ), (LPSTR) &bmInfo );
    xExt = bmInfo.bmWidth;
    yExt = bmInfo.bmHeight;

    hbmShadow    = CreateBitmap( xExt, yExt, 1, 1, NULL );
    hbmHighlight = CreateBitmap( xExt, yExt, 1, 1, NULL );
    hbmDisable   = CreateCompatibleBitmap( hDC, xExt, yExt );

    hbmOld = SelectObject( hdcColor, hBitmap );

    // Set background color of bitmap for mono conversion.
    SetBkColor( hdcColor, GetPixel( hdcColor, 0, 0 ) );

    // Create the shadow bitmap.
    hbmShadow = SelectObject( hdcMono, (HGDIOBJ) hbmShadow );
    PatBlt( hdcMono, 0, 0, xExt, yExt, WHITENESS );
    BitBlt( hdcMono, 0, 0, xExt-1, yExt-1, hdcColor, 1, 1, SRCCOPY );
    BitBlt( hdcMono, 0, 0, xExt, yExt, hdcColor, 0, 0, MERGEPAINT );

    hbmShadow = SelectObject( hdcMono, (HGDIOBJ) hbmShadow );

    // Create the highlight bitmap.
    hbmHighlight = SelectObject( hdcMono, (HGDIOBJ) hbmHighlight );
    BitBlt( hdcMono, 0, 0, xExt, yExt, hdcColor, 0, 0, SRCCOPY );
    BitBlt( hdcMono, 0, 0, xExt-1, yExt-1, hdcColor, 1, 1, MERGEPAINT );

    hbmHighlight = SelectObject( hdcMono, (HGDIOBJ) hbmHighlight );

    // Select old bitmap into color DC.
    SelectObject( hdcColor, hbmOld );

    // Clear the background for the disabled bitmap.
    SelectObject( hdcColor, hbmDisable );

    hbrPat = CreateSolidBrush( crFace );
    hbrPat = SelectObject( hdcColor, hbrPat );
    PatBlt( hdcColor, 0, 0, xExt, yExt, PATCOPY );
    DeleteObject( SelectObject( hdcColor, hbrPat ) );
    SetBkColor( hdcColor, RGB_WHITE );
    SetTextColor( hdcColor, RGB_BLACK );

    // Blt the highlight edge.
    hbrPat = CreateSolidBrush( crHigh );
    hbrPat = SelectObject( hdcColor, hbrPat );
    hbmHighlight = SelectObject( hdcMono, (HGDIOBJ) hbmHighlight );
    BitBlt( hdcColor, 0, 0, xExt, yExt, hdcMono, 0, 0, PSDPxax );
    DeleteObject( SelectObject( hdcColor, hbrPat ) );
    hbmHighlight = SelectObject( hdcMono, (HGDIOBJ) hbmHighlight );

    // Blt the shadow edge.
    hbrPat = CreateSolidBrush( crShadow );
    hbrPat = SelectObject( hdcColor, hbrPat );
    hbmShadow = SelectObject( hdcMono, (HGDIOBJ) hbmShadow );

    // hdcColor now contains the "tinfoil'd" bitmap.
    BitBlt( hdcColor, 0, 0, xExt, yExt, hdcMono, 0, 0, PSDPxax );
    DeleteObject( SelectObject( hdcColor, hbrPat ) );
    hbmShadow = SelectObject( hdcMono, (HGDIOBJ) hbmShadow );

    // Check the extents against the clipping rect and adjust if necessary.
    if( lpRect )
      {
      if( xOrg >= lpRect->right )
        xOrg = lpRect->right - xExt;
      if( xOrg < lpRect->left )
        xOrg = lpRect->left;
      if( xOrg + xExt > lpRect->right )
        xExt = lpRect->right - xOrg;
   
    if( yOrg >= lpRect->bottom )
        yOrg = lpRect->bottom - yExt;
      if( yOrg < lpRect->top )
        yOrg = lpRect->top;
      if( yOrg + yExt > lpRect->bottom )
        yExt = lpRect->bottom - yOrg;
      }
   
    // Copy the "disabled" bitmap to the screen.
    BitBlt( hDC, xOrg, yOrg, xExt, yExt, hdcColor, 0, 0, SRCCOPY );
   
    // Select old bitmap into color DC.
    SelectObject( hdcColor, hbmOld );

    // Delete the memory bitmaps.
    DeleteObject( (HGDIOBJ) hbmShadow );
    DeleteObject( (HGDIOBJ) hbmHighlight );
    DeleteObject( (HGDIOBJ) hbmDisable );

    // Delete the memory DCs.
    DeleteDC( hdcMono );
    DeleteDC( hdcColor );

    return;
    }

/****************************************************************************
 * GetBitmapExt
 *
 * Purpose:
 *  Gets the x/y extents of a bitmap.
 *
 * Parameters:
 *  HDC           hDC         - dc to draw to
 *  HBITMAP       hBitmap     - handle to bitmap
 *  int           xExt        - pointer to returned bitmap width
 *  int           yExt        - pointer to returned bitmap height
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID NEAR GetBitmapExt( HDC hdc, HBITMAP hBitmap, LPINT xExt, LPINT yExt )
    {
    BITMAP  bm;
    POINT   ptSize;    

    GetObject( hBitmap, sizeof(BITMAP), (LPSTR) &bm );

    ptSize.x = bm.bmWidth;    
    ptSize.y = bm.bmHeight;    
    DPtoLP( hdc, &ptSize, 1 );

    *xExt = ptSize.x;
    *yExt = ptSize.y;

    return;
    }

/****************************************************************************
 * SpinColorManager
 *
 * Purpose:
 *  Initializes, updates, or deletes the spin button's color list
 *  and the associated pens.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the control
 *  LPSPIN        lpSpin      - control data pointer.
 *  UINT          uAction     - type of action to execute
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI SpinColorManager( HWND hWnd, LPSPIN lpSpin, UINT uAction )
    {  
    HDC hDC;
    int iColor;

    if( uAction == CLR_UPDATE || uAction == CLR_DELETE )
      {
      hDC = GetDC( hWnd );

      // Select this so we don't try to delete a selected pen.
      SelectObject( hDC, GetStockObject( BLACK_PEN ) );

      // Delete this spin's pens.
      for( iColor=0; iColor < SPINCOLORS; iColor++ )
        DeleteObject( lpSpin->hPen[iColor] );

      ReleaseDC( hWnd, hDC );
      }

    if( uAction == CLR_INIT )
      {
      // Load up the system default colors.
      for( iColor=0; iColor < SPINCOLORS; iColor++ )
        lpSpin->crSpin[iColor] = GetSysColor( crDefColor[iColor] );
      }

    if( uAction == CLR_INIT || uAction == CLR_UPDATE )
      {
      // Create all the spin pens.  
      for( iColor=0; iColor < SPINCOLORS; iColor++ )
        lpSpin->hPen[iColor] = CreatePen( PS_SOLID, 1, lpSpin->crSpin[iColor] );
      }

    return;
    }
