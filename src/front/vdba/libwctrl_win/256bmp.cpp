/*
**  Copyright (C) 2005-2006 Ingres Corporation.
*/

/*
**    Source   : 256Bmp.cpp : implementation file
**    Project  : IMPORT ASSISTANT 
**    Author   : 
**    Purpose  : STEP 1 of Import assistant
**
** History:
**
** 24-April-1999
**    Created for vdba sql assistant
** 07-Fev-2000 (uk$so01)
**    Move this class in a library 'libwctrl.lib' to be reusable.
**    Add a function SetBitmpapId() that set the bitmap ID after it has been constructed.
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 30-Jan-2002 (uk$so01)
**    SIR #106648, Enhance the position (center vertical & horizontal) of bitmap.
**/

#include "stdafx.h"
#include "256bmp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

WORD DibNumColors (VOID FAR * pv)
{
	INT bits;
	LPBITMAPINFOHEADER lpbi= ((LPBITMAPINFOHEADER)pv);
	LPBITMAPCOREHEADER lpbc= ((LPBITMAPCOREHEADER)pv);
	
	if (lpbi->biSize != sizeof(BITMAPCOREHEADER))
	{
		if (lpbi->biClrUsed != 0)
			return (WORD)lpbi->biClrUsed;
		bits = lpbi->biBitCount;
	}
	else
		bits = lpbc->bcBitCount;

	switch (bits)
	{
	case 1:
		return 2;
	case 4:
		return 16;
	case 8:
		return 256;
	default:
		return 0;
	}
}



WORD PaletteSize (VOID FAR * pv)
{
	
	LPBITMAPINFOHEADER lpbi= (LPBITMAPINFOHEADER)pv;
	WORD  NumColors=DibNumColors(lpbi);
	if (lpbi->biSize == sizeof(BITMAPCOREHEADER))
		return (WORD)(NumColors * sizeof(RGBTRIPLE));
	else
		return (WORD)(NumColors * sizeof(RGBQUAD));
}


HPALETTE CreateBIPalette (LPBITMAPINFOHEADER lpbi)
{
	LOGPALETTE *pPal;
	HPALETTE hpal = NULL;
	WORD nNumColors;
	BYTE red;
	BYTE green;
	BYTE blue;
	WORD i;
	RGBQUAD FAR *pRgb;
	if (!lpbi)
		return NULL;
	
	if (lpbi->biSize != sizeof(BITMAPINFOHEADER))
		return NULL;
	/* Get a pointer to the color table and the number of colors in it */
	pRgb = (RGBQUAD FAR *)((LPSTR)lpbi + (WORD)lpbi->biSize);
	nNumColors = DibNumColors(lpbi);
	
	if (nNumColors)
	{
		/* Allocate for the logical palette structure */
		pPal = (LOGPALETTE*)LocalAlloc(LPTR,sizeof(LOGPALETTE) + nNumColors * sizeof(PALETTEENTRY));
		if (!pPal)
			return NULL;
		pPal->palNumEntries = nNumColors;
		pPal->palVersion	= 0x300;
		/* Fill in the palette entries from the DIB color table and
		* create a logical color palette.
		*/
		for (i = 0; i < nNumColors; i++){
			pPal->palPalEntry[i].peRed	 = pRgb[i].rgbRed;
			pPal->palPalEntry[i].peGreen = pRgb[i].rgbGreen;
			pPal->palPalEntry[i].peBlue  = pRgb[i].rgbBlue;
			pPal->palPalEntry[i].peFlags = (BYTE)0;
		}
		hpal = CreatePalette(pPal);
		LocalFree((HANDLE)pPal);
	}
	else if (lpbi->biBitCount == 24)
	{
		/* A 24 bitcount DIB has no color table entries so, set the number of
		* to the maximum value (256).
		*/
		nNumColors = 256;
		pPal = (LOGPALETTE*)LocalAlloc(LPTR,sizeof(LOGPALETTE) + nNumColors * sizeof(PALETTEENTRY));
		if (!pPal)
			return NULL;
		pPal->palNumEntries = nNumColors;
		pPal->palVersion	= 0x300;
		red = green = blue = 0;
		for (i = 0; i < pPal->palNumEntries; i++){
			pPal->palPalEntry[i].peRed	 = red;
			pPal->palPalEntry[i].peGreen = green;
			pPal->palPalEntry[i].peBlue  = blue;
			pPal->palPalEntry[i].peFlags = (BYTE)0;
			if (!(red += 32))
				if (!(green += 32))
					blue += 64;
		}
		hpal = CreatePalette(pPal);
		LocalFree((HANDLE)pPal);
	}
	return hpal;
}

/////////////////////////////////////////////////////////////////////////////
// C256bmp

C256bmp::C256bmp()
{
	m_hModule = NULL;
	m_hExternalInstance = NULL;
	m_nResourceID = 0;

	m_bCenterX = FALSE;
	m_bCenterY = FALSE;
}

C256bmp::~C256bmp()
{
}


BEGIN_MESSAGE_MAP(C256bmp, CStatic)
	//{{AFX_MSG_MAP(C256bmp)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// C256bmp message handlers

void C256bmp::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	BITMAPINFO *pb=GetBitmap();
	if (pb)
	{
		BITMAPINFOHEADER *pbh=( BITMAPINFOHEADER*)pb;
		HPALETTE hpal=CreateBIPalette (pbh);
		LPSTR pbits = (LPSTR)pbh + (WORD)pbh->biSize + PaletteSize(pbh);
		HPALETTE holdpal=::SelectPalette(dc.m_hDC,hpal,FALSE);
		::RealizePalette(dc.m_hDC);

		int nXdest = 0;
		int nYdest = 0;
		if (m_bCenterY)
		{
			CRect r;
			GetClientRect (r);
			nYdest = (r.Height() - pbh->biHeight) / 2;
			if (nYdest < 0)
				nYdest = 0;
		}
		if (m_bCenterX)
		{
			CRect r;
			GetClientRect (r);
			nXdest = (r.Width() - pbh->biWidth) / 2;
			if (nXdest < 0)
				nXdest = 0;
		}

		::SetDIBitsToDevice(
			dc.m_hDC,
			nXdest,
			nYdest,
			pbh->biWidth,
			pbh->biHeight,
			0,
			0,
			0,
			pbh->biHeight,
			pbits,
			pb,
			DIB_RGB_COLORS);
		::SelectPalette(dc.m_hDC,holdpal,FALSE);
		::DeleteObject(hpal);
	}
	ValidateRect(0);
}

void C256bmp::GetBitmapDimension(CPoint & pt)
{
	BITMAPINFOHEADER *pbh=( BITMAPINFOHEADER*)GetBitmap();
	if (pbh)
	{
		pt.x=pbh->biWidth;
		pt.y=pbh->biHeight;
	}
	else
	{
		pt.x=pt.y=0;
	}
}


BOOL C256bmp::OnEraseBkgnd(CDC* pDC) 
{
	return TRUE;
}

BITMAPINFO * C256bmp::GetBitmap()
{
	BITMAPINFO *pb = NULL;
	HRSRC hrsrc = NULL;
	int bitmapNum = 0;
	if (m_nResourceID == 0)
	{
		CString s; GetWindowText(s); 
		bitmapNum=_ttoi(s);
	}
	else
	{
		bitmapNum = m_nResourceID;
	}

	if (m_hExternalInstance)
		m_hModule = m_hExternalInstance;
	else
	if (!m_hModule)
		m_hModule = AfxGetInstanceHandle();
	hrsrc=FindResource(m_hModule, MAKEINTRESOURCE(bitmapNum), RT_BITMAP);
	if (hrsrc)
	{
		HGLOBAL h=LoadResource(m_hModule, hrsrc);
		if (h)
		{
			pb=(BITMAPINFO *) LockResource(h);
		}
	}
	return pb;
}
