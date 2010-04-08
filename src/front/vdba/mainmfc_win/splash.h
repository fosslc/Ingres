// CG: This file was added by the Splash Screen component.

// 06-Nov-2001 (noifr01)
// (SIR 106290) update for following the new guidelines for
//  splash screens
// 20-Mar-2009 (smeke01) b121832
//    Add global function INGRESII_BuildVersionInfo() (copied from libingll).

#ifndef _SPLASH_SCRN_
#define _SPLASH_SCRN_

// Splash.h : header file
//

/////////////////////////////////////////////////////////////////////////////
//   Splash Screen class

class CSplashWnd : public CWnd
{
// Construction
protected:
	CSplashWnd();

// Attributes:
private:
  BOOL m_bSupportPalette;
  BOOL m_bHasPalette;
  CPalette m_palette;
	CBitmap m_bitmap;
public:
	static CFont	m_font;
	static CStatic* m_pStatic;

// Operations
public:
	static void EnableSplashScreen(BOOL bEnable = TRUE);
	static void ShowSplashScreen(CWnd* pParentWnd = NULL);
  static void VdbaHideSplashScreen();
	static BOOL PreTranslateAppMessage(MSG* pMsg);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSplashWnd)
	//}}AFX_VIRTUAL

// Implementation
public:
	~CSplashWnd();
	virtual void PostNcDestroy();

protected:
	BOOL Create(CWnd* pParentWnd = NULL);
	void HideSplashScreen();
	static BOOL c_bShowSplashWnd;
	static CSplashWnd* c_pSplashWnd;

// Generated message map functions
protected:
	//{{AFX_MSG(CSplashWnd)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT nIDEvent);
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


// Used by splash screen and help about logic to get product build and year information
extern "C" void INGRESII_BuildVersionInfo (CString& strVersion, int& nYear);

#endif
