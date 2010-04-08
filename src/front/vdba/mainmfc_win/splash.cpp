/*
** CG: This file was added by the Splash Screen component.
** Splash.cpp : implementation file
**
** 06-Nov-2001 (noifr01)
** (SIR 106290) update for following the new guidelines for
**  splash screens
** 08-Nov-2001 (noifr01)
** (sir 106290) increased the duration of the splash screen to 
**  3 seconds (+ VDBA initialization time )
** 06-Jan-2002) (noifr01)
**   (additional change for SIR 106290) the product name  now
**   appears in the bitmap rather than in the text.
** 29-Jan-2003 (noifr01)
**      (SIR 108139) get the version year by parsing information 
**       from the gv.h file
** 03-feb-2004 (somsa01)
**	Include compat.h when including gv.h.
** 10-Feb-2004 (schph01)
**   (SIR 108139)  additional change for retrieved the year by parsing information 
**   from the gv.h file.
** 23-Jun-2004 (schph01)
**   (SIR 112464) remove CA licensing references in splash screen.
** 20-July-2004 (schph01)
**   (SIR 112708) Change the bitmap used by VDBA splash screen.
** 20-Oct-2004 (uk$so01)
**    BUG #113271 / ISSUE #13751480 -- The string GV_VER does not contain 
**    information about Year anymore. So hardcode Year to 2004.
** 19-Jan-2005 (komve01)
**    BUG #113768 / ISSUE 13913697: 
**	  GUI tools display incorrect year in the Copyright Information.
** 14-Feb-2006 (drivi01)
**    Update the year to 2006.
** 08-Jan-2007 (drivi01)
**    Update the year to 2007.
** 07-Jan-2008 (drivi01)
**    Created copyright.h header for Visual DBA suite.
**    Redefine copyright year as constant defined in new
**    header file.  This will ensure single point of update
**    for variable year.
** 29-Apr-2008 (drivi01)
**    Update the location of copyright notice on the splash screen.
** 20-Mar-2009 (smeke01) b121832
**    Add global function INGRESII_BuildVersionInfo() (copied from libingll) 
**    to return product version and year. Used in this file to get product 
**    year for copyright message on splash screen.
*/

#include "stdafx.h"  // e. g. stdafx.h
#include "resmfc.h"  // e.g. resource.h

#include "splash.h"  // e.g. splash.h
#include "prodname.h"
#include "copyright.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

extern "C"
{
#include <compat.h>
#include <gv.h>
}

/////////////////////////////////////////////////////////////////////////////
// Utility functions

HPALETTE CreateDIBPalette (LPBITMAPINFO lpbmi, int& riNumColors)
{ 
  LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER)lpbmi;

  // pick number of colors
  if (lpbi->biBitCount <= 8)
    riNumColors = (1 << lpbi->biBitCount);
  else
    riNumColors = 0;  // No palette needed for 24 BPP DIB
  if (lpbi->biClrUsed > 0)
    riNumColors = lpbi->biClrUsed;  // Use biClrUsed

  // No palette if no colors
  if (riNumColors == 0)
    return NULL;

  // Build palette from bitmap info
  UINT nSize = sizeof(LOGPALETTE) + (sizeof(PALETTEENTRY) * (riNumColors-1));
  LPLOGPALETTE lpPal = (LPLOGPALETTE) new BYTE[nSize];
  lpPal->palVersion    = 0x300;
  lpPal->palNumEntries = riNumColors;
  for (int cpt=0; cpt < riNumColors;  cpt++) {
    lpPal->palPalEntry[cpt].peRed   = lpbmi->bmiColors[cpt].rgbRed;
    lpPal->palPalEntry[cpt].peGreen = lpbmi->bmiColors[cpt].rgbGreen;
    lpPal->palPalEntry[cpt].peBlue  = lpbmi->bmiColors[cpt].rgbBlue;
    lpPal->palPalEntry[cpt].peFlags = 0;
  }
  HPALETTE hPal = CreatePalette(lpPal);
  delete[] lpPal;

  return hPal;
} 

HBITMAP LoadResourceBitmap(HINSTANCE hInstance, LPCTSTR lpString, HPALETTE& rhPalette)
{
  // Pre initialize in case of error
  rhPalette = NULL;

  HRSRC hRsrc = FindResource(hInstance, lpString, RT_BITMAP);
  if (hRsrc == NULL)
    return NULL;
  HGLOBAL hGlobal = LoadResource(hInstance, hRsrc);
  if (hGlobal == NULL)
    return NULL;
  LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER)LockResource(hGlobal);
  if (lpbi == NULL)
    return NULL;

  HDC hdc = GetDC(NULL);
  int iNumColors;
  rhPalette =  CreateDIBPalette ((LPBITMAPINFO)lpbi, iNumColors);
  if (rhPalette) {
    SelectPalette(hdc, rhPalette, FALSE);
    RealizePalette(hdc);
  }
  HBITMAP hBitmapFinal = CreateDIBitmap(hdc,
                                        (LPBITMAPINFOHEADER)lpbi,
                                        (LONG)CBM_INIT,
                                        (LPSTR)lpbi + lpbi->biSize + iNumColors * sizeof(RGBQUAD), 
                                        (LPBITMAPINFO)lpbi,
                                        DIB_RGB_COLORS );
  ReleaseDC(NULL,hdc);
  
  return (hBitmapFinal);
} 

#define SPLASH_WIDTH 550	// Splash screen width
#define SPLASH_HEIGHT 370	// Splash screen height
#define CP_X	10		// X coordination of coyright box
#define CP_Y	335		// Y coordination of coyright box

/////////////////////////////////////////////////////////////////////////////
//   Splash Screen class

BOOL CSplashWnd::c_bShowSplashWnd;
CSplashWnd* CSplashWnd::c_pSplashWnd;
CFont		CSplashWnd::m_font;
CStatic*	CSplashWnd::m_pStatic;

CSplashWnd::CSplashWnd()
{
  m_bSupportPalette = FALSE;
  m_bHasPalette = FALSE;
}

CSplashWnd::~CSplashWnd()
{
	// Clear the static window pointer.
	ASSERT(c_pSplashWnd == this);
	c_pSplashWnd = NULL;

	// Delete the Font object.
	m_font.DeleteObject();	

	// Delete the Copyright Static control
	delete m_pStatic;


}

BEGIN_MESSAGE_MAP(CSplashWnd, CWnd)
	//{{AFX_MSG_MAP(CSplashWnd)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CSplashWnd::EnableSplashScreen(BOOL bEnable /*= TRUE*/)
{
	c_bShowSplashWnd = bEnable;
}

void CSplashWnd::ShowSplashScreen(CWnd* pParentWnd /*= NULL*/)
{
	if (!c_bShowSplashWnd)
		return;
	if (c_pSplashWnd != NULL)
	{
		c_pSplashWnd->RedrawWindow();
		return;
	}


	// Allocate a new splash screen, and create the window.
	c_pSplashWnd = new CSplashWnd;
	if (!c_pSplashWnd->Create(pParentWnd)) {
		delete c_pSplashWnd;
		return;
	}
	else
		c_pSplashWnd->UpdateWindow();

	m_pStatic = new CStatic;
	// Here we assume the bitmap size is 485x440
	// 72dpi is the standard use here.
	// 1/8 dpi = 9 pixels.
	// 1/2 dpi = 36 pixels.

	CString strVer;
	int year;
	
	/* get year information (here strVer is just to keep the function happy) */
	INGRESII_BuildVersionInfo (strVer, year);
	
	// Put the copyright information.
	CString s, sCopyright;
	//sCopyright = _T("\r\r\r\r");
	s.Format(IDS_COPYRIGHT_NOTICE,year);
	sCopyright += s;

	// Create the coyright static control.
	m_pStatic->Create(sCopyright,WS_CHILD|WS_VISIBLE, 
		CRect(CP_X,CP_Y,SPLASH_WIDTH -CP_X-CP_X/2 /*Reserve a bigger width for the coyright*/,SPLASH_HEIGHT-4),
		c_pSplashWnd, 125);


	// Use the font to paint a control.
	m_pStatic->SetFont(&m_font);
	m_pStatic->ShowWindow(SW_SHOW);	

	Sleep(3000);
}

void CSplashWnd::VdbaHideSplashScreen()
{
	if (!c_bShowSplashWnd)
		return;
  if (c_pSplashWnd == NULL)   // already deleted
    return;
  c_pSplashWnd->ShowWindow(SW_HIDE);
}

BOOL CSplashWnd::PreTranslateAppMessage(MSG* pMsg)
{
	if (c_pSplashWnd == NULL)
		return FALSE;

  // VDBA SPECIFIC : IGNORE SHORTCUT MESSAGES
  /*
	// If we get a keyboard or mouse message, hide the splash screen.
	if (pMsg->message == WM_KEYDOWN ||
	    pMsg->message == WM_SYSKEYDOWN ||
	    pMsg->message == WM_LBUTTONDOWN ||
	    pMsg->message == WM_RBUTTONDOWN ||
	    pMsg->message == WM_MBUTTONDOWN ||
	    pMsg->message == WM_NCLBUTTONDOWN ||
	    pMsg->message == WM_NCRBUTTONDOWN ||
	    pMsg->message == WM_NCMBUTTONDOWN)
	{
		c_pSplashWnd->HideSplashScreen();
		return TRUE;	// message handled here
	}
  */

	return FALSE;	// message not handled
}

BOOL CSplashWnd::Create(CWnd* pParentWnd /*= NULL*/)
{
  // check whether device supports palettes
  CClientDC dc(AfxGetMainWnd());
  if ((dc.GetDeviceCaps(RASTERCAPS) & RC_PALETTE) != 0)
    m_bSupportPalette = TRUE;
  else
    m_bSupportPalette = FALSE;

  // load from resources according to palette support presence
  if (m_bSupportPalette) {
    HBITMAP hBitmap;
    HPALETTE hPalette;
    hBitmap = LoadResourceBitmap(NULL,
                                 (LPCTSTR)MAKELONG((WORD)VDBA_GetSplashScreen_Bitmap_id(), 0),
                                 hPalette);
    if (hBitmap == NULL)
      return FALSE;
    if (hPalette == NULL) {
      // No palette: try to load as a DDB
    	if (!m_bitmap.LoadBitmap(VDBA_GetSplashScreen_Bitmap_id()))
  	  	return FALSE;
    }
    else {
      m_bHasPalette = TRUE;
      m_bitmap.Attach(hBitmap);
      m_palette.Attach(hPalette);
    }
  }
  else {
    // load as a DDB
  	if (!m_bitmap.LoadBitmap(VDBA_GetSplashScreen_Bitmap_id()))
	  	return FALSE;
  }

  // Get bitmap width/height, for window create
	BITMAP bm;
	m_bitmap.GetBitmap(&bm);

	// Create "Verdana 8" font for the copyright information.
	LOGFONT lf;                        // Used to create the CFont.
	memset(&lf, 0, sizeof(LOGFONT));   // Clear out structure.
	lf.lfHeight = -9;                  // Request a 8-pixel-high font
	strcpy(lf.lfFaceName, "Verdana");  // with face name "Verdana".
	m_font.CreateFontIndirect(&lf);    // Create the font.

	BOOL bSuccess;
	bSuccess = CreateEx(0,
		AfxRegisterWndClass(0, AfxGetApp()->LoadStandardCursor(IDC_ARROW)),
		NULL, WS_POPUP | WS_VISIBLE, 0, 0, bm.bmWidth, bm.bmHeight, pParentWnd->GetSafeHwnd(), NULL);
  return bSuccess;
}

void CSplashWnd::HideSplashScreen()
{
	// Destroy the window, and update the mainframe.
	DestroyWindow();
	AfxGetMainWnd()->UpdateWindow();
}

void CSplashWnd::PostNcDestroy()
{
  // liberate memory
  if ((HBITMAP)m_bitmap != NULL)
    m_bitmap.DeleteObject();
  if ((HPALETTE)m_palette != NULL)
    m_palette.DeleteObject();

	// Free the C++ class.
	delete this;
}

int CSplashWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// Center the window.
	CenterWindow();

	// Set a timer to destroy the splash screen.
	SetTimer(1, 10000, NULL);     // Up to 10 seconds, Original value 750

	return 0;
}

void CSplashWnd::OnPaint()
{
	CPaintDC dc(this);

  if (m_bHasPalette) {
    TRACE0 ("Splash window OnPaint() - has palette\n");
    ASSERT (m_bSupportPalette);
    ASSERT ((HPALETTE)m_palette != NULL);
    CPalette* pOldPalette = dc.SelectPalette(&m_palette, FALSE);
    dc.RealizePalette();

    DIBSECTION ds;
    memset (&ds, 0, sizeof(ds));
    int val = m_bitmap.GetObject(sizeof(DIBSECTION), &ds);
    ASSERT (val);

  	CDC memDC;
	  if (!memDC.CreateCompatibleDC(&dc))
		  return;
    CBitmap* pOldBitmap = memDC.SelectObject(&m_bitmap);
    dc.BitBlt(0, 0, ds.dsBm.bmWidth, ds.dsBm.bmHeight, &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject (pOldBitmap);
    dc.SelectPalette(pOldPalette, FALSE);
  }
  else {
    // No palette in image, or no palette support: bitblit "as is"
    TRACE0 ("Splash window OnPaint() - has no palette\n");
  	CDC dcImage;
	  if (!dcImage.CreateCompatibleDC(&dc))
		  return;
	  CBitmap* pOldBitmap = dcImage.SelectObject(&m_bitmap);
	  BITMAP bm;
	  m_bitmap.GetBitmap(&bm);
	  dc.BitBlt(0, 0, bm.bmWidth, bm.bmHeight, &dcImage, 0, 0, SRCCOPY);
	  dcImage.SelectObject(pOldBitmap);
  }
}

void CSplashWnd::OnTimer(UINT nIDEvent)
{
	// Destroy the splash screen window.
	HideSplashScreen();
}

HBRUSH CSplashWnd::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CWnd::OnCtlColor(pDC, pWnd, nCtlColor);

	// Sets the background mode transparent.
	pDC->SetBkMode(TRANSPARENT);
	return (HBRUSH)::GetStockObject(NULL_BRUSH);

	// Return a different brush if the default is not desired
	return hbr;
}

/* previous code when loaded from file
BOOL CSplashWnd::Create(CWnd* pParentWnd)
{
  // check whether device supports palettes
  CClientDC dc(AfxGetMainWnd());
  if (dc.GetDeviceCaps(RASTERCAPS) & RC_PALETTE != 0)
    m_bSupportPalette = TRUE;
  else
    m_bSupportPalette = FALSE;

  HBITMAP hBitmap;
  hBitmap = (HBITMAP)::LoadImage (AfxGetInstanceHandle(),
                                  (LPCTSTR)"c:\\vdba\\vdbadomsplitbar\\mainmfc\\res\\mire.bmp",
                                  IMAGE_BITMAP,
                                  0, 0,
                                  LR_LOADFROMFILE | LR_CREATEDIBSECTION);
  if (hBitmap == NULL) {
    ASSERT (FALSE);
    return FALSE;     // FAILURE
  }
  m_bitmap.Attach(hBitmap);
  // if palette supported, check whether the loaded bitmap contains a palette
  if (m_bSupportPalette) {
    DIBSECTION ds;
    memset (&ds, 0, sizeof(ds));
    int val = m_bitmap.GetObject(sizeof(DIBSECTION), &ds);
    if (val == 0)
      m_bHasPalette = FALSE;
    else {
      m_bHasPalette = TRUE;
      int nColors;
      if (ds.dsBmih.biClrUsed != 0)
        nColors = ds.dsBmih.biClrUsed;
      else
        nColors = 1 << ds.dsBmih.biBitCount;
      // halftone palette if more than 256 colors
      if (nColors > 256)
        m_palette.CreateHalftonePalette(&dc);
      else {
        // create palette on the fly:

        // Pick dib color table
        RGBQUAD* pRGB = new RGBQUAD[nColors];
        CDC memDC;
        memDC.CreateCompatibleDC(&dc);
        CBitmap* pOldBitmap = memDC.SelectObject (&m_bitmap);
        ::GetDIBColorTable ((HDC)memDC, 0, nColors, pRGB);
        memDC.SelectObject(pOldBitmap);

        // allocate palette structure
        UINT nSize = sizeof(LOGPALETTE) + (sizeof(PALETTEENTRY) * (nColors-1));
        LOGPALETTE* pLP = (LOGPALETTE*) new BYTE[nSize];

        // duplicate dib color table into palette structure
        pLP->palVersion = 0x300;
        pLP->palNumEntries = nColors;
        for (int cpt=0; cpt<nColors; cpt++) {
          pLP->palPalEntry[cpt].peRed = pRGB[cpt].rgbRed;
          pLP->palPalEntry[cpt].peGreen = pRGB[cpt].rgbGreen;
          pLP->palPalEntry[cpt].peBlue = pRGB[cpt].rgbBlue;
          pLP->palPalEntry[cpt].peFlags = 0;
        }

        // create palette, then free objects
        m_palette.CreatePalette(pLP);
        delete[] pLP;
        delete[] pRGB;
      }
    }
  }

  // Get bitmap width/height, for window create
	BITMAP bm;
	m_bitmap.GetBitmap(&bm);

  BOOL bSuccess;
	bSuccess = CreateEx(0,
		AfxRegisterWndClass(0, AfxGetApp()->LoadStandardCursor(IDC_ARROW)),
		NULL, WS_POPUP | WS_VISIBLE, 0, 0, bm.bmWidth, bm.bmHeight, pParentWnd->GetSafeHwnd(), NULL);
  return bSuccess;
}
*/

void INGRESII_BuildVersionInfo (CString& strVersion, int& nYear)
{
	nYear = COPYRIGHT_YEAR;

	// Parse "II 9.x.x (int.w32/xxx)" style string from gv.h to generate version string.
	CString strVer = (LPCTSTR) GV_VER;

	int ipos = strVer.Find(_T('('));
	if (ipos>=0)
		strVer=strVer.Left(ipos);
	strVer.TrimRight();
	while ( (ipos = strVer.Find(_T(' ')) ) >=0)
		strVer=strVer.Mid(ipos+1);
	
	strVersion = strVer;
}
