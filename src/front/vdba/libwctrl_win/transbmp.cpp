/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : parse.h, Header File 
**    Project  : Visual DBA.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Bitmap assumed to have RGB(255,0,255) pixels to be 
**               drawn as transparent, an be enhanced by adding a 
**               new member with RGB to be drawn as transparent.
**
** History:
**
** 24-Jul-2001 (uk$so01)
**    Created. Split the old code from VDBA (mainmfc\parse.cpp)
** 03-Sep-2001 (uk$so01)
**    Transform code to be an ActiveX Control
*/


#include "stdafx.h"
#include "transbmp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Utility functions
void DrawTransparentBitmap(HDC hdc, HBITMAP hBitmap, short xStart, short yStart, COLORREF cTransparentColor)
{
	BITMAP     bm;
	COLORREF   cColor;
	HBITMAP    bmAndBack, bmAndObject, bmAndMem, bmSave;
	HBITMAP    bmBackOld, bmObjectOld, bmMemOld, bmSaveOld;
	HDC        hdcMem, hdcBack, hdcObject, hdcTemp, hdcSave;
	POINT      ptSize;

	hdcTemp = CreateCompatibleDC(hdc);
	SelectObject(hdcTemp, hBitmap);   // Select the bitmap

	GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
	ptSize.x = bm.bmWidth;            // Get width of bitmap
	ptSize.y = bm.bmHeight;           // Get height of bitmap
	DPtoLP(hdcTemp, &ptSize, 1);      // Convert from device to logical points

	// Create some DCs to hold temporary data.
	hdcBack   = CreateCompatibleDC(hdc);
	hdcObject = CreateCompatibleDC(hdc);
	hdcMem    = CreateCompatibleDC(hdc);
	hdcSave   = CreateCompatibleDC(hdc);

	// Create a bitmap for each DC. DCs are required for a number of
	// GDI functions.

	// Monochrome DC
	bmAndBack   = CreateBitmap(ptSize.x, ptSize.y, 1, 1, NULL);

	// Monochrome DC
	bmAndObject = CreateBitmap(ptSize.x, ptSize.y, 1, 1, NULL);

	bmAndMem    = CreateCompatibleBitmap(hdc, ptSize.x, ptSize.y);
	bmSave      = CreateCompatibleBitmap(hdc, ptSize.x, ptSize.y);

	// Each DC must select a bitmap object to store pixel data.
	bmBackOld   = (HBITMAP)SelectObject(hdcBack, bmAndBack);
	bmObjectOld = (HBITMAP)SelectObject(hdcObject, bmAndObject);
	bmMemOld    = (HBITMAP)SelectObject(hdcMem, bmAndMem);
	bmSaveOld   = (HBITMAP)SelectObject(hdcSave, bmSave);

	// Set proper mapping mode.
	SetMapMode(hdcTemp, GetMapMode(hdc));

	// Save the bitmap sent here, because it will be overwritten.
	BitBlt(hdcSave, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCCOPY);

	// Set the background color of the source DC to the color.
	// contained in the parts of the bitmap that should be transparent
	cColor = SetBkColor(hdcTemp, cTransparentColor);

	// Create the object mask for the bitmap by performing a BitBlt
	// from the source bitmap to a monochrome bitmap.
	BitBlt(hdcObject, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCCOPY);

	// Set the background color of the source DC back to the original
	// color.
	SetBkColor(hdcTemp, cColor);

	// Create the inverse of the object mask.
	BitBlt(hdcBack, 0, 0, ptSize.x, ptSize.y, hdcObject, 0, 0, NOTSRCCOPY);

	// Copy the background of the main DC to the destination.
	BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdc, xStart, yStart, SRCCOPY);

	// Mask out the places where the bitmap will be placed.
	BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcObject, 0, 0, SRCAND);

	// Mask out the transparent colored pixels on the bitmap.
	BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcBack, 0, 0, SRCAND);

	// XOR the bitmap with the background on the destination DC.
	BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCPAINT);

	// Copy the destination to the screen.
	BitBlt(hdc, xStart, yStart, ptSize.x, ptSize.y, hdcMem, 0, 0, SRCCOPY);

	// Place the original bitmap back into the bitmap sent here.
	BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcSave, 0, 0, SRCCOPY);

	// Delete the memory bitmaps.
	DeleteObject(SelectObject(hdcBack, bmBackOld));
	DeleteObject(SelectObject(hdcObject, bmObjectOld));
	DeleteObject(SelectObject(hdcMem, bmMemOld));
	DeleteObject(SelectObject(hdcSave, bmSaveOld));

	// Delete the memory DCs.
	DeleteDC(hdcMem);
	DeleteDC(hdcBack);
	DeleteDC(hdcObject);
	DeleteDC(hdcSave);
	DeleteDC(hdcTemp);
}


CStaticTransparentBitmap::CStaticTransparentBitmap()
{
}

CStaticTransparentBitmap::~CStaticTransparentBitmap()
{
}


BEGIN_MESSAGE_MAP(CStaticTransparentBitmap, CStatic)
	//{{AFX_MSG_MAP(CStaticTransparentBitmap)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStaticTransparentBitmap message handlers

void CStaticTransparentBitmap::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	DrawTransparentBitmap(
		dc.m_hDC,           // The destination DC
		m_hBmp,             // The bitmap to be drawn
		0,                 // X coordinate
		0,                 // Y coordinate
		RGB(255, 0, 255)   // color for transparent pixels
		);
	// Do not call CStatic::OnPaint() for painting messages
}

void CStaticTransparentBitmap::SetBitmap (HBITMAP hBmp)
{
	m_hBmp = hBmp;
	if (IsWindow(m_hWnd))
		InvalidateRect(NULL, TRUE); // True erases
}

BOOL CStaticTransparentBitmap::OnEraseBkgnd(CDC* pDC) 
{
	COLORREF colorBk = ::GetSysColor (COLOR_BTNFACE);   // COLOR_APPWORKSPACE
	CBrush bkBrush;
	bkBrush.CreateSolidBrush(colorBk);
	CRect rect;
	GetClientRect(&rect);
	pDC->FillRect(&rect, &bkBrush);

	return TRUE;    // I have processed!
}
