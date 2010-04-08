/****************************************************************************
 * Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * CNTPAINT.C
 *
 * Contains common functions used by all the views to handle WM_PAINT logic 
 ****************************************************************************/
#include <windows.h>
#include "cntdll.h"


/****************************************************************************
 * WithinRect
 *
 * Purpose:
 *  determines whether rect1 is within or intersects rect2
 *
 * Parameters:
 *  LPRECT  lpRect1  - the first rectangle
 *  LPRECT  lpRect2  - the second rectangle
 *
 * Return Value:
 *  TRUE if rect1 contained by rect2 or they intersect, otherwise FALSE
 ****************************************************************************/
BOOL WithinRect( LPRECT lpRect1, LPRECT lpRect2 )
{
    if (lpRect1->right  > lpRect2->left && lpRect1->left < lpRect2->right &&
        lpRect1->bottom > lpRect2->top  && lpRect1->top  < lpRect2->bottom)
      return TRUE;
    else 
      return FALSE;
}

/****************************************************************************
 * Draw3DFrame
 *
 * Purpose:
 *  Draws the 3D button look within a given rectangle.
 *  The rect will be drawn with a one pixel black border.
 *
 * Parameters:
 *  DC            hDC         - to draw to.
 *  HPEN          hPenHigh    - highlight color pen.
 *  HPEN          hPenShadow  - shadow color pen.
 *  HPEN          hPenBorder  - border color pen.
 *  int           x1          - Upper left corner x.
 *  int           y1          - Upper left corner y.
 *  int           x2          - Lower right corner x.
 *  int           y2          - Lower right corner y.
 *
 * Return Value:
 *  VOID
 ****************************************************************************/
VOID Draw3DFrame( HDC hDC, HPEN hPenHigh, HPEN hPenShadow, HPEN hPenBorder,
                       int x1, int y1, int x2, int y2, BOOL bPressed )
{
    HPEN  hPenOrg;

    // First draw the border.
    hPenOrg = SelectObject( hDC, hPenBorder );
    MoveToEx( hDC, x1, y1, NULL );
    LineTo( hDC, x1, y2 );
    LineTo( hDC, x2, y2 );
    LineTo( hDC, x2, y1 );
    LineTo( hDC, x1, y1 );

    // Now shrink the rectangle to fit inside the black border.
    x1 += 1;
    x2 -= 1;
    y1 += 1;
    y2 -= 1;

    SelectObject( hDC, hPenShadow );

    if( bPressed )
      {
      //Shadow on left and top edge when pressed.
      MoveToEx( hDC, x1, y2, NULL );
      LineTo( hDC, x1,   y1 );
      LineTo( hDC, x2+1, y1 );
      }
    else
      {
      //Lowest shadow line.
      MoveToEx( hDC, x1, y2, NULL );
      LineTo( hDC, x2, y2   );
      LineTo( hDC, x2, y1-1 );
  
      //Upper shadow line.
      MoveToEx( hDC, x1+1, y2-1, NULL );
      LineTo( hDC, x2-1, y2-1 );
      LineTo( hDC, x2-1, y1   );
  
      SelectObject( hDC, hPenHigh );
  
      //Upper highlight line.
      MoveToEx( hDC, x1,   y2-1, NULL );
      LineTo( hDC, x1,   y1   );
      LineTo( hDC, x2,   y1   );
      }

    SelectObject( hDC, hPenOrg );
    return;
}

/****************************************************************************
 * PaintInUseRect
 *
 * Purpose:
 *  Paints a rectangle with the CRA_INUSE emphasis.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window.
 *  LPCONTAINER   lpCnt       - pointer to container's CONTAINER structure.
 *  DC            hDC         - container window DC
 *  LPRECT        lpRect      - pointer to rectangle to be painted
 *
 * Return Value:
 *  VOID
 ****************************************************************************/
VOID PaintInUseRect( HWND hWnd, LPCONTAINER lpCnt, HDC hDC, LPRECT lpRect )
{
    HBRUSH    hbr, hbrOld;
    POINT     pt;
#ifdef WIN32
    POINT     ptPrev;
#endif

    // If a pattern or hatch brush is being used in a scrollable area
    // you must maintain brush alignment by resetting the brush origin
    // in accordance with the container's scrolling positions.
    // Maintaining pattern alignment in Windows is documented in a 
    // confusing, inconsistent manner. Specifically, the documentation
    // for SetBrushOrg seems to be wrong (or at least misleading). 
    // It says the x/y values must be between 0 and 7 but the following 
    // code works just fine and makes sense, too.

    // Get the current container scrolling positions and calculate where
    // the horizontal and vertical brush origin should be set.
    // It will always be 0,0 or some negative offset in x and/or y.
    pt.x = -lpCnt->nHscrollPos * lpCnt->lpCI->cxChar;
    pt.y = -lpCnt->nVscrollPos * lpCnt->lpCI->cyRow;
    ClientToScreen( hWnd, &pt );

    // Create a hatch brush and set its origin in screen coordinates. 
    hbr = CreateHatchBrush( HS_DIAGCROSS, lpCnt->crColor[CNTCOLOR_INUSE] );
    UnrealizeObject( hbr );
#ifdef WIN32
    SetBrushOrgEx( hDC, pt.x, pt.y, &ptPrev );
#else
    SetBrushOrg( hDC, pt.x, pt.y );
#endif

    // Select the brush into the container's DC and fill the rectangle.
    hbrOld = SelectObject( hDC, hbr );
    FillRect( hDC, lpRect, hbr );

    // Restore the original brush and destroy the hatch brush.
    SelectObject( hDC, hbrOld );
    DeleteObject( hbr );

    return;
}

/****************************************************************************
 * GetBitmapExt
 *
 * Purpose:
 *  Gets the x/y extents of a bitmap.
 *
 * Parameters:
 *  DC            hDC         - DC to draw to
 *  HBITMAP       hBitmap     - handle to bitmap
 *  int           xExt        - pointer to returned bitmap width
 *  int           yExt        - pointer to returned bitmap height
 *
 * Return Value:
 *  VOID
 ****************************************************************************/
VOID GetBitmapExt( HDC hdc, HBITMAP hBitmap, LPINT xExt, LPINT yExt )
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
 * DrawBitmap
 *
 * Purpose:
 *  Draws a bitmap starting at the passed in x/y origin.
 *
 * Parameters:
 *  DC            hDC         - DC to draw to
 *  HBITMAP       hBitmap     - handle to bitmap
 *  int           xOrg        - x coord to upper left corner of destination
 *  int           yOrg        - y coord to upper left corner of destination
 *  LPRECT        lpRect      - pointer to clip rect
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID DrawBitmap( HDC hdc, HBITMAP hBitmap, int xOrg, int yOrg, LPRECT lpRect )
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
 *  DC            hDC         - DC to draw to
 *  HBITMAP       hBitmap     - handle to bitmap
 *  int           xOrg        - x coord to upper left corner of destination
 *  int           yOrg        - y coord to upper left corner of destination
 *  int           xTransPxl   - x position of pixel which has transparent color
 *  int           yTransPxl   - y position of pixel which has transparent color
 *  LPRECT        lpRect      - pointer to clip rect
 *
 * Return Value:
 *  VOID
 ****************************************************************************/
VOID DrawTransparentBitmap(HDC hdc, HBITMAP hBitmap, int xOrg, int yOrg,
                           int xTransPxl, int yTransPxl, LPRECT lpRect )
{
    BITMAP     bm;
    COLORREF   cColor;
    COLORREF   cTransparentColor;
    HBITMAP    hbmMask, hbmMem;
    HBITMAP    hbmMaskOld, hbmMemOld;
    HDC        hdcMem, hdcTemp, hdcMask;
    POINT      ptSize;
    int     xExt, yExt;

    hdcTemp = CreateCompatibleDC( hdc );
    SelectObject( hdcTemp, hBitmap );   // Select the bitmap

    GetObject( hBitmap, sizeof(BITMAP), (LPSTR)&bm );
    ptSize.x = bm.bmWidth;            // Get width of bitmap
    ptSize.y = bm.bmHeight;           // Get height of bitmap
    DPtoLP( hdcTemp, &ptSize, 1 );    // Convert from device to logical points

    // Set proper mapping mode.
    SetMapMode( hdcTemp, GetMapMode( hdc ) );

    // Monochrome DC - used for the mask
    hdcMask    = CreateCompatibleDC( hdc );
    hbmMask    = CreateBitmap( ptSize.x, ptSize.y, 1, 1, NULL );
    hbmMaskOld = SelectObject( hdcMask, hbmMask );

    // Get the color designated to be the transparent color from the bitmap.
    // Set the background color of the source DC to the color
    // contained in the parts of the bitmap that should be transparent
    cTransparentColor = GetPixel( hdcTemp, xTransPxl, yTransPxl );
    cColor = SetBkColor( hdcTemp, cTransparentColor );

    // Create the object mask for the bitmap by performing a BitBlt
    // from the source bitmap to a monochrome bitmap.
    BitBlt( hdcMask, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCCOPY );

    // Set the bkgrnd color of the source DC back to the original color.
    SetBkColor( hdcTemp, cColor );

    // Create scratch bitmap
    hdcMem    = CreateCompatibleDC( hdc );
    hbmMem    = CreateCompatibleBitmap( hdc, ptSize.x, ptSize.y );
    hbmMemOld = SelectObject( hdcMem, hbmMem );

    // create the transparent bitmap
    BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdc, xOrg, yOrg, SRCCOPY);
    BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCINVERT);
    BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcMask, 0, 0, SRCAND);
    BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCINVERT);
  
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

    // Delete the memory bitmaps.
    DeleteObject( SelectObject( hdcMask, hbmMaskOld ) );
    DeleteObject( SelectObject( hdcMem, hbmMemOld ) );

    // Delete the memory DCs.
    DeleteDC( hdcMem );
    DeleteDC( hdcMask );
    DeleteDC( hdcTemp );
}

#if 0
// this is old way of drawing transparent bitmaps.  I developed a new algorithm that
// runs a bit faster (see above).  If you use MaskBlt(), you can increase the speed
// even more (this is Win32 dependent)
VOID DrawTransparentBitmap(HDC hdc, HBITMAP hBitmap, int xOrg, int yOrg,
                                int xTransPxl, int yTransPxl, LPRECT lpRect )
{
    BITMAP     bm;
    COLORREF   cColor;
    COLORREF   cTransparentColor;
    HBITMAP    bmAndBack, bmAndObject, bmAndMem, bmSave;
    HBITMAP    bmBackOld, bmObjectOld, bmMemOld, bmSaveOld;
    HDC        hdcMem, hdcBack, hdcObject, hdcTemp, hdcSave;
    POINT      ptSize;
    int     xExt, yExt;

    hdcTemp = CreateCompatibleDC( hdc );
    SelectObject( hdcTemp, hBitmap );   // Select the bitmap

    GetObject( hBitmap, sizeof(BITMAP), (LPSTR)&bm );
    ptSize.x = bm.bmWidth;            // Get width of bitmap
    ptSize.y = bm.bmHeight;           // Get height of bitmap
    DPtoLP( hdcTemp, &ptSize, 1 );      // Convert from device to logical points

    // Create some DCs to hold temporary data.
    hdcBack   = CreateCompatibleDC( hdc );
    hdcObject = CreateCompatibleDC( hdc );
    hdcMem    = CreateCompatibleDC( hdc );
    hdcSave   = CreateCompatibleDC( hdc );

    // Create a bitmap for each DC. DCs are required for a number of
    // GDI functions.

    // Monochrome DC
    bmAndBack   = CreateBitmap( ptSize.x, ptSize.y, 1, 1, NULL );

    // Monochrome DC
    bmAndObject = CreateBitmap( ptSize.x, ptSize.y, 1, 1, NULL );

    bmAndMem    = CreateCompatibleBitmap( hdc, ptSize.x, ptSize.y );
    bmSave      = CreateCompatibleBitmap( hdc, ptSize.x, ptSize.y );

    // Each DC must select a bitmap object to store pixel data.
    bmBackOld   = SelectObject( hdcBack, bmAndBack );
    bmObjectOld = SelectObject( hdcObject, bmAndObject );
    bmMemOld    = SelectObject( hdcMem, bmAndMem );
    bmSaveOld   = SelectObject( hdcSave, bmSave );

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
    DeleteObject( SelectObject( hdcBack, bmBackOld ) );
    DeleteObject( SelectObject( hdcObject, bmObjectOld ) );
    DeleteObject( SelectObject( hdcMem, bmMemOld ) );
    DeleteObject( SelectObject( hdcSave, bmSaveOld ) );

    // Delete the memory DCs.
    DeleteDC( hdcMem );
    DeleteDC( hdcBack );
    DeleteDC( hdcObject );
    DeleteDC( hdcSave );
    DeleteDC( hdcTemp );
}
#endif
#if 0
// this code deals with drawing transparent bitmaps - I was trying to speed up the
// performance of the painting of bitmaps in name view.  For Win32, you can use
// MaskBlt() - the limitation being you have to supply a mask in the form of a bitmap.
// There are also other algorithms which may perform transparency better than
// our current one 
VOID DrawTransparentBitmap1(HDC hdc, HBITMAP hBitmap, HBITMAP hMask, int xOrg, int yOrg,
                           int xTransPxl, int yTransPxl, LPRECT lpRect )
{
    BITMAP     bm;
    COLORREF   cColor;
    COLORREF   cTransparentColor;
    HBITMAP    hbmMask, hbmMem, bmSave;
    HBITMAP    bmBackOld, hbmMaskOld, hbmMemOld, bmSaveOld;
    HDC        hdcMem, hdcBack, hdcObject, hdcTemp, hdcSave, hdcMask;
    POINT      ptSize;
    int     xExt, yExt;

    hdcTemp = CreateCompatibleDC( hdc );
    SelectObject( hdcTemp, hBitmap );   // Select the bitmap

    GetObject( hBitmap, sizeof(BITMAP), (LPSTR)&bm );
    ptSize.x = bm.bmWidth;            // Get width of bitmap
    ptSize.y = bm.bmHeight;           // Get height of bitmap
    DPtoLP( hdcTemp, &ptSize, 1 );    // Convert from device to logical points

    // Set proper mapping mode.
    SetMapMode( hdcTemp, GetMapMode( hdc ) );

    MaskBlt(hdc,       // handle of target DC
            xOrg,      // x coord, upper-left corner of target rectangle
            yOrg,      // y coord, upper-left corner of target rectangle
            ptSize.x,  // width of source and target rectangles
            ptSize.y,  // height of source and target rectangles
            hdcTemp,   // handle of source DC
            0,         // x coord, upper-left corner of source rectangle
            0,         // y coord, upper-left corner of source rectangle
            hMask,     // handle of monochrome bit-mask
            0,         // x coord, upper-left corner of mask rectangle
            0,         // y coord, upper-left corner of mask rectangle
            0xCDAA0000 // raster-operation (ROP) code
           );
  
    DeleteDC( hdcTemp );
    return;
}
#endif
