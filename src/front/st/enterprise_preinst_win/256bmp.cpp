/*
**  Copyright (c) 2001 Ingres Corporation
*/

/*
**  Name: 256bmp.cpp : implementation file
**
**  History:
**
**	05-Jun-2001 (penga03)
**	    Created.
**	01-Mar-2007 (drivi01)
**		Modified code to overwrite painting of banner at the top
**	    of the wizard.
*/

#include "stdafx.h"
#include "enterprise.h"
#include "256bmp.h"
#include "dibpal.h"

#define PADWIDTH(x)  (((x)*8 + 31)  & (~31))/8

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
	
	LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER)pv;
	WORD NumColors = DibNumColors(lpbi);
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
		pPal->palVersion = 0x300;
		/* 
		Fill in the palette entries from the DIB color table and create a logical color palette.
		*/
		for (i = 0; i < nNumColors; i++)
		{
			pPal->palPalEntry[i].peRed = pRgb[i].rgbRed;
			pPal->palPalEntry[i].peGreen = pRgb[i].rgbGreen;
			pPal->palPalEntry[i].peBlue  = pRgb[i].rgbBlue;
			pPal->palPalEntry[i].peFlags = (BYTE)0;
		}
		hpal = CreatePalette(pPal);
		LocalFree((HANDLE)pPal);
	}
	else if (lpbi->biBitCount == 24)
	{
		/* 
		A 24 bitcount DIB has no color table entries so, set the number of to the maximum value (256).
		*/
		nNumColors = 256;
		pPal = (LOGPALETTE*)LocalAlloc(LPTR,sizeof(LOGPALETTE) + nNumColors * sizeof(PALETTEENTRY));
		if (!pPal)
			return NULL;
		pPal->palNumEntries = nNumColors;
		pPal->palVersion = 0x300;
		red = green = blue = 0;
		for (i = 0; i < pPal->palNumEntries; i++)
		{
			pPal->palPalEntry[i].peRed = red;
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
}

C256bmp::~C256bmp()
{
}


BEGIN_MESSAGE_MAP(C256bmp, CStatic)
//{{AFX_MSG_MAP(C256bmp)
ON_WM_ERASEBKGND()
ON_WM_PAINT()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// C256bmp message handlers

BOOL C256bmp::OnEraseBkgnd(CDC* pDC) 
{
	return CStatic::OnEraseBkgnd(pDC);
}

void C256bmp::OnPaint() 
{
	CPaintDC dc(this); /* device context for painting */
	
	BITMAPINFO *pb=GetBitmap();
	if (pb)
	{
		BITMAPINFOHEADER *pbh=( BITMAPINFOHEADER*)pb;
		HPALETTE hpal=CreateBIPalette (pbh);
		LPSTR pbits = (LPSTR)pbh + (WORD)pbh->biSize + PaletteSize(pbh);
		HPALETTE holdpal=::SelectPalette(dc.m_hDC,hpal,FALSE);
		if (!hSystemPalette) hSystemPalette=holdpal;
		::RealizePalette(dc.m_hDC);
		::SetDIBitsToDevice(dc.m_hDC,0,0,pbh->biWidth,pbh->biHeight,0,0,0,pbh->biHeight,pbits,pb,DIB_RGB_COLORS);
		::SelectPalette(dc.m_hDC,holdpal,FALSE);
		::DeleteObject(hpal);
	}
	ValidateRect(0);
}

BOOL C256bmp :: CreateFromBitmap( CDC * pDC, CBitmap * pSrcBitmap ) 
{
	ASSERT_VALID(pSrcBitmap);
	ASSERT_VALID(pDC);

	try {
		BITMAP bmHdr;

		// Get the pSrcBitmap info
		pSrcBitmap->GetObject(sizeof(BITMAP), &bmHdr);

		// Reallocate space for the image data
		if( m_pPixels ) 
		{
			delete [] m_pPixels;
			m_pPixels = 0;
		}

		DWORD dwWidth;
		if (bmHdr.bmBitsPixel > 8)
			dwWidth = PADWIDTH(bmHdr.bmWidth * 3);
		else
			dwWidth = PADWIDTH(bmHdr.bmWidth);

		m_pPixels = new BYTE[dwWidth*bmHdr.bmHeight];
		if( !m_pPixels )
			throw TEXT("could not allocate data storage\n");

		// Set the appropriate number of colors base on BITMAP structure info
		WORD wColors;
		switch( bmHdr.bmBitsPixel ) 
		{
			case 1 : 
				wColors = 2;
				break;
			case 4 :
				wColors = 16;
				break;
			case 8 :
				wColors = 256;
				break;
			default :
				wColors = 0;
				break;
		}

		// Re-allocate and populate BITMAPINFO structure
		if( m_pInfo ) 
		{
			delete [] (BYTE*)m_pInfo;
			m_pInfo = 0;
		}

		m_pInfo = (BITMAPINFO*)new BYTE[sizeof(BITMAPINFOHEADER) + wColors*sizeof(RGBQUAD)];
		if( !m_pInfo )
			throw TEXT("could not allocate BITMAPINFO struct\n");

		// Populate BITMAPINFO header info
		m_pInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		m_pInfo->bmiHeader.biWidth = bmHdr.bmWidth;
		m_pInfo->bmiHeader.biHeight = bmHdr.bmHeight;
		m_pInfo->bmiHeader.biPlanes = bmHdr.bmPlanes;
		
		
		if( bmHdr.bmBitsPixel > 8 )
			m_pInfo->bmiHeader.biBitCount = 24;
		else
			m_pInfo->bmiHeader.biBitCount = bmHdr.bmBitsPixel;

		m_pInfo->bmiHeader.biCompression = BI_RGB;
		m_pInfo->bmiHeader.biSizeImage = ((((bmHdr.bmWidth * bmHdr.bmBitsPixel) + 31) & ~31) >> 3) * bmHdr.bmHeight;
		m_pInfo->bmiHeader.biXPelsPerMeter = 0;
		m_pInfo->bmiHeader.biYPelsPerMeter = 0;
		m_pInfo->bmiHeader.biClrUsed = 0;
		m_pInfo->bmiHeader.biClrImportant = 0;

		// Now actually get the bits
		int test = ::GetDIBits(pDC->GetSafeHdc(), (HBITMAP)pSrcBitmap->GetSafeHandle(),
	 		0, (WORD)bmHdr.bmHeight, m_pPixels, m_pInfo, DIB_RGB_COLORS);

		// check that we scanned in the correct number of bitmap lines
		if( test != (int)bmHdr.bmHeight )
			throw TEXT("call to GetDIBits did not return full number of requested scan lines\n");
		
		CreatePalette();
		m_bIsPadded = FALSE;
#ifdef _DEBUG
	} 
	catch( TCHAR * psz ) 
	{
		TRACE1("C256bmp::CreateFromBitmap(): %s\n", psz);
#else
	} 
	catch( TCHAR * ) 
	{
#endif
		if( m_pPixels ) 
		{
			delete [] m_pPixels;
			m_pPixels = 0;
		}
		if( m_pInfo ) 
		{
			delete [] (BYTE*) m_pInfo;
			m_pInfo = 0;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL C256bmp :: LoadOneResource(LPCTSTR pszID) 
{
	HINSTANCE hInst = ::AfxFindResourceHandle(pszID, RT_BITMAP);
   
	if (hInst == NULL)
	{
		return FALSE;
	}
	HBITMAP hBmp = (HBITMAP)::LoadImage(hInst,
										pszID,
										IMAGE_BITMAP,
										0,
										0,
										LR_CREATEDIBSECTION);

	if( hBmp == 0 ) 
		return FALSE;

	CBitmap bmp;
	bmp.Attach(hBmp);
	CClientDC cdc( CWnd::GetDesktopWindow() );
	BOOL bRet = CreateFromBitmap( &cdc, &bmp );
	bmp.DeleteObject();
	return bRet;
}

BOOL C256bmp :: CreatePalette() 
{
	if( m_pPal )
		delete m_pPal;
	m_pPal = 0;
	ASSERT( m_pInfo );
	// We only need a palette, if there are <= 256 colors.
	// otherwise we would bomb the memory.
	if( m_pInfo->bmiHeader.biBitCount <= 8 )
		m_pPal = new CBmpPalette(this);
	return m_pPal ? TRUE : FALSE;
}

void C256bmp :: ClearPalette() 
{
	if( m_pPal )
		delete m_pPal;
	m_pPal = 0;
}


void C256bmp :: DrawDIB( CDC* pDC, int x, int y ) 
{
	DrawDIB( pDC, x, y, GetWidth(), GetHeight() );
}

//
// DrawDib uses StretchDIBits to display the bitmap.
void C256bmp :: DrawDIB( CDC* pDC, int x, int y, int width, int height ) 
{
    ASSERT( pDC );
    HDC     hdc = pDC->GetSafeHdc();

	CPalette * pOldPal = 0;

	if( m_pPal ) 
	{
		pOldPal = pDC->SelectPalette( m_pPal, FALSE );
		pDC->RealizePalette();
		// Make sure to use the stretching mode best for color pictures
		pDC->SetStretchBltMode(COLORONCOLOR);
	}

    if( m_pInfo )
        StretchDIBits( hdc,
                       x,
                       y,
                       width,
                       height,
                       0,
                       0,
                       GetWidth(),
                       GetHeight(),
                       GetPixelPtr(),
                       GetHeaderPtr(),
                       DIB_RGB_COLORS,
                       SRCCOPY );
	
	if( m_pPal )
		pDC->SelectPalette( pOldPal, FALSE );
}

int C256bmp :: DrawDIB( CDC * pDC, CRect & rectDC, CRect & rectDIB ) 
{
    ASSERT( pDC );
    HDC     hdc = pDC->GetSafeHdc();

	CPalette * pOldPal = 0;

	if( m_pPal ) 
	{
		pOldPal = pDC->SelectPalette( m_pPal, FALSE );
		pDC->RealizePalette();
		// Make sure to use the stretching mode best for color pictures
		pDC->SetStretchBltMode(COLORONCOLOR);
	}

	int nRet = 0;

    if( m_pInfo )
		nRet =	SetDIBitsToDevice(
					hdc,					// device
					rectDC.left,			// DestX
					rectDC.top,				// DestY
					rectDC.Width(),			// DestWidth
					rectDC.Height(),		// DestHeight
					rectDIB.left,			// SrcX
					GetHeight() -
						rectDIB.top -
						rectDIB.Height(),	// SrcY
					0,						// StartScan
					GetHeight(),			// NumScans
					GetPixelPtr(),			// color data
					GetHeaderPtr(),			// header data
					DIB_RGB_COLORS			// color usage
				);
	
	if( m_pPal )
		pDC->SelectPalette( pOldPal, FALSE );

	return nRet;
}

BITMAPINFO * C256bmp :: GetHeaderPtr() const 
{
    ASSERT( m_pInfo );
    ASSERT( m_pPixels );
    return m_pInfo;
}

RGBQUAD * C256bmp :: GetColorTablePtr() const 
{
    ASSERT( m_pInfo );
    ASSERT( m_pPixels );
    RGBQUAD* pColorTable = 0;
    if( m_pInfo != 0 ) 
	{
        int cOffset = sizeof(BITMAPINFOHEADER);
        pColorTable = (RGBQUAD*)(((BYTE*)(m_pInfo)) + cOffset);
    }
    return pColorTable;
}


BYTE * C256bmp :: GetPixelPtr() const 
{
    ASSERT( m_pInfo );
    ASSERT( m_pPixels );
    return m_pPixels;
}

int C256bmp :: GetWidth() const 
{
    ASSERT( m_pInfo );
    return m_pInfo->bmiHeader.biWidth;
}

int C256bmp :: GetHeight() const 
{
    ASSERT( m_pInfo );
    return m_pInfo->bmiHeader.biHeight;
}

int C256bmp :: GetPalEntries() const 
{
    ASSERT( m_pInfo );
    return GetPalEntries( *(BITMAPINFOHEADER*)m_pInfo );
}

int C256bmp :: GetPalEntries( BITMAPINFOHEADER& infoHeader ) const 
{
	int nReturn;
	if( infoHeader.biClrUsed == 0 )
		nReturn = ( 1 << infoHeader.biBitCount );
	else
		nReturn = infoHeader.biClrUsed;

	return nReturn;
}


BITMAPINFO * C256bmp::GetBitmap()
{
	BITMAPINFO *pb = NULL;
	CString s; GetWindowText(s); 
	int bitmapNum=atoi(s);
	HRSRC hrsrc=FindResource(AfxGetInstanceHandle(),MAKEINTRESOURCE(bitmapNum),RT_BITMAP);
	if (hrsrc)
	{
		HGLOBAL h=LoadResource(AfxGetInstanceHandle(),hrsrc);
		if (h)
		{
			pb=(BITMAPINFO *) LockResource(h);
		}
	}
	return pb;
}

void C256bmp::GetBitmapDimension(CPoint &pt)
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






