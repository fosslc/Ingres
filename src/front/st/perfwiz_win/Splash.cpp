/*
**  Copyright (C) 1998, 2001 Ingres Corporation
*/

/*
**
**  Name: Splash.cpp
**
**  Description:
**      This file contains the routines to implement the splash screen dialog.
**
**  History:
**      ??-jul-1998 (mcgem01)
**          Created.
**	06-nov-2001 (somsa01)
**	    Modified to conform to the new standard as per CorpQA.
**	07-nov-2001 (somsa01)
**	    Modified the new splash generation to be "color-conscious".
**	07-nov-2001 (somsa01)
**	    Modified the ending of the splash screen to just hide it.
**	23-Jun-2004 (drivi01)
**	    Removed licensing.
**	22-oct-2004 (penga03)
**	    Updated the splash screen.
** 	29-Apr-2008 (drivi01)
**    	    Update the location of copyright notice on the splash screen.
*/

#include "stdafx.h"
#include "perfwiz.h"
#include "Splash.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/*
** Splash Screen class
** Developer only needs to change here if the splash screen size is different.
*/
#define SPLASH_WIDTH	550	/* Splash screen width */
#define SPLASH_HEIGHT	370	/* Splash screen height */
#define CP_X		 10	/* X coordination of coyright box */
#define CP_Y		335	/* Y coordination of coyright box */

CSplashWnd*	CSplashWnd::c_pSplashWnd;
CFont		CSplashWnd::m_font;
CStatic*	CSplashWnd::m_pStatic;

CSplashWnd::CSplashWnd()
{
}

CSplashWnd::~CSplashWnd()
{
    /* Clear the static window pointer. */
    c_pSplashWnd = NULL;

    /* Delete the Font object. */
    m_font.DeleteObject();	

    /* Delete the background bitmap object. */
    m_bitmap.DeleteObject();

    /* Delete the Copyright Static control */
    delete m_pStatic;
}

BEGIN_MESSAGE_MAP(CSplashWnd, CWnd)
    //{{AFX_MSG_MAP(CSplashWnd)
    ON_WM_CREATE()
    ON_WM_PAINT()
    ON_WM_TIMER()
    ON_WM_CTLCOLOR()
    ON_WM_LBUTTONUP()
    ON_WM_KILLFOCUS()
    ON_WM_KEYDOWN()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


void
CSplashWnd::ShowSplashScreen(CWnd* pParentWnd)
{
    if (c_pSplashWnd != NULL)
    {
	c_pSplashWnd->RedrawWindow();
	return;
    }

    /* Allocate a new splash screen, and create the window. */
    c_pSplashWnd = new CSplashWnd;
    if (!c_pSplashWnd->Create(pParentWnd))
    {
	delete c_pSplashWnd;
	return;
    }
    else
	c_pSplashWnd->UpdateWindow();

    m_pStatic = new CStatic;
    /*
    ** Here we assume the bitmap size is 485x440
    ** 72dpi is the standard use here.
    ** 1/8 dpi = 9 pixels.
    ** 1/2 dpi = 36 pixels.
    */

    CString sSiteID , sOrg;
    sSiteID.LoadString(IDS_UNKNOWN);
    sOrg.LoadString(IDS_UNKNOWN);
 
    /* Put the copyright information with or without license information. */
    CString s, sCopyright;
 
    s.LoadString(IDS_COPYRIGHT);
    sCopyright += s;

    /* Create the coyright static control. */
    m_pStatic->Create(sCopyright,WS_CHILD|WS_VISIBLE, 
		      CRect(CP_X, CP_Y,
			    SPLASH_WIDTH -CP_X-CP_X/2 /*Reserve a bigger width for the coyright*/,
			    SPLASH_HEIGHT-4), 
		      c_pSplashWnd, 125);

    /* Use the font to paint a control. */
    m_pStatic->SetFont(&m_font);
    m_pStatic->ShowWindow(SW_SHOW);

    /* Run the Window as a ModalLoop */
    c_pSplashWnd->RunModalLoop();
}

BOOL
CSplashWnd::Create(CWnd* pParentWnd)
{
    BITMAP bm;

    if (!m_bitmap.LoadBitmap(IDB_SPLASH256))
	return FALSE;

    m_bitmap.GetBitmap(&bm);

    /*
    ** Create "Verdana 8" font for the copyright information.
    */
    LOGFONT lf;				/* Used to create the CFont. */
    memset(&lf, 0, sizeof(LOGFONT));	/* Clear out structure. */
    lf.lfHeight = -9;			/* Request a 8-pixel-high font */
    strcpy(lf.lfFaceName, "Verdana");	/* with face name "Verdana". */
    m_font.CreateFontIndirect(&lf);	/* Create the font. */

    return CreateEx(0,
		    AfxRegisterWndClass(0, AfxGetApp()->LoadStandardCursor(IDC_ARROW)),
		    NULL, WS_POPUP | WS_VISIBLE, 0, 0, bm.bmWidth, bm.bmHeight,
		    pParentWnd->GetSafeHwnd(), NULL);
}

void
CSplashWnd::HideSplashScreen()
{
    c_pSplashWnd->EndModalLoop(FALSE);
    c_pSplashWnd->ShowWindow(SW_HIDE);
    c_pSplashWnd->EnableWindow(FALSE);
}

void CSplashWnd::PostNcDestroy()
{
    /* Free the C++ class. */
    delete this;
}

int CSplashWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWnd::OnCreate(lpCreateStruct) == -1)
	    return -1;

    /* Center the window. */
    CenterWindow();

    /* Set a timer to destroy the splash screen. */
    SetTimer(1, SPLASH_DURATION, NULL);

    return 0;
}

static WORD
DibNumColors (VOID FAR * pv)
{
    INT 		bits;
    LPBITMAPINFOHEADER	lpbi = ((LPBITMAPINFOHEADER)pv);
    LPBITMAPCOREHEADER	lpbc = ((LPBITMAPCOREHEADER)pv);
    
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

static WORD
PaletteSize (VOID FAR * pv)
{
	
    LPBITMAPINFOHEADER	lpbi = (LPBITMAPINFOHEADER)pv;
    WORD		NumColors = DibNumColors(lpbi);

    if (lpbi->biSize == sizeof(BITMAPCOREHEADER))
	return (WORD)(NumColors * sizeof(RGBTRIPLE));
    else
	return (WORD)(NumColors * sizeof(RGBQUAD));
}

static HPALETTE
CreateBIPalette (LPBITMAPINFOHEADER lpbi)
{
    LOGPALETTE	*pPal;
    HPALETTE	hpal = NULL;
    WORD	nNumColors;
    BYTE	red;
    BYTE	green;
    BYTE	blue;
    WORD	i;
    RGBQUAD	FAR *pRgb;

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
	pPal = (LOGPALETTE*)LocalAlloc(LPTR,
				       sizeof(LOGPALETTE) + nNumColors * sizeof(PALETTEENTRY));
	if (!pPal)
	    return NULL;

	pPal->palNumEntries = nNumColors;
	pPal->palVersion    = 0x300;

	/*
	** Fill in the palette entries from the DIB color table and
	** create a logical color palette.
	*/
	for (i = 0; i < nNumColors; i++)
	{
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
	/*
	** A 24 bitcount DIB has no color table entries so, set the number of
	** to the maximum value (256).
	*/
	nNumColors = 256;
	pPal = (LOGPALETTE*)LocalAlloc(LPTR,
				       sizeof(LOGPALETTE) + nNumColors * sizeof(PALETTEENTRY));
	if (!pPal)
	    return NULL;

	pPal->palNumEntries = nNumColors;
	pPal->palVersion    = 0x300;
	red = green = blue = 0;
	for (i = 0; i < pPal->palNumEntries; i++)
	{
	    pPal->palPalEntry[i].peRed	 = red;
	    pPal->palPalEntry[i].peGreen = green;
	    pPal->palPalEntry[i].peBlue  = blue;
	    pPal->palPalEntry[i].peFlags = (BYTE)0;
	    if (!(red += 32))
	    {
		if (!(green += 32))
		    blue += 64;
	    }
	}
	hpal = CreatePalette(pPal);
	LocalFree((HANDLE)pPal);
    }
    return hpal;
}

void
CSplashWnd::OnPaint()
{
    CPaintDC dc(this);
    HRSRC hrsrc = FindResource(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_SPLASH256) ,RT_BITMAP);
    if (hrsrc)
    {
	HGLOBAL h = LoadResource(AfxGetInstanceHandle(), hrsrc);
	BITMAPINFO *pb = (BITMAPINFO *)LockResource(h);
	if (pb)
	{
	    BITMAPINFOHEADER *pbh = (BITMAPINFOHEADER*)pb;
	    HPALETTE hpal = CreateBIPalette(pbh);
	    LPSTR pbits = (LPSTR)pbh + (WORD)pbh->biSize + PaletteSize(pbh);
	    HPALETTE holdpal = ::SelectPalette(dc.m_hDC,hpal,FALSE);
	    if (!hSystemPalette)
		hSystemPalette=holdpal;
	    ::RealizePalette(dc.m_hDC);
	    ::SetDIBitsToDevice(dc.m_hDC, 0, 0, pbh->biWidth, pbh->biHeight, 0, 0, 0,
				pbh->biHeight, pbits, pb, DIB_RGB_COLORS);
	    ::SelectPalette(dc.m_hDC,holdpal,FALSE);
	    ::DeleteObject(hpal);
	}
    }
    ValidateRect(0);
}

void
CSplashWnd::OnTimer(UINT nIDEvent)
{
    /* Destroy the splash screen window. */
    HideSplashScreen();
}

HBRUSH
CSplashWnd::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
    HBRUSH hbr = CWnd::OnCtlColor(pDC, pWnd, nCtlColor);
		
    /* Sets the background mode transparent. */
    pDC->SetBkMode(TRANSPARENT);
    return (HBRUSH)::GetStockObject(NULL_BRUSH);
       	
    /* Return a different brush if the default is not desired */
    return hbr;
}

void
CSplashWnd::OnLButtonUp(UINT nFlags, CPoint point) 
{
    /* When user clicks on left button of mouse. Splash will disappear. */
    c_pSplashWnd->HideSplashScreen();
}

void
CSplashWnd::OnKillFocus(CWnd* pNewWnd) 
{
    CWnd::OnKillFocus(pNewWnd);
    
    if (pNewWnd == NULL)
	SetFocus();
}

void
CSplashWnd::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
    /* If the user clicks any key, destroy the splash screen window. */
    c_pSplashWnd->HideSplashScreen();
    CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}
