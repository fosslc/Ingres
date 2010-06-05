/*
**  Copyright (C) 2004 Ingres Corporation
*/

/*
**
**  Name: Splash.cpp
**
**  Description:
**      This file contains the routines to implement the splash screen dialog.
**
**  History:
**	29-apr-2004 (wonst02)
**	    Created; cloned from similar enterprise_preinst directory.
**	06-Apr-2010 (drivi01)
**	    Update OnTimer to take UINT_PTR as a parameter as UINT_PTR
**	    will be unsigned int on x86 and unsigned int64 on x64.
**	    This will cleanup warnings.
*/

#ifndef _SPLASH_SCRN_
#define _SPLASH_SCRN_

/*
** Splash Screen class
*/
class CSplashWnd : public CWnd
{
/* Construction */

/* Attributes: */
public:
    CSplashWnd();
    CBitmap m_bitmap;
    static CFont m_font;
    static CStatic* m_pStatic;

/* Operations */
public:
    static void EnableLicense(BOOL bEnable = TRUE);
    static void ShowSplashScreen(CWnd* pParentWnd = NULL);

/* Overrides */
    /* ClassWizard generated virtual function overrides */
    //{{AFX_VIRTUAL(CSplashWnd)
    //}}AFX_VIRTUAL

/* Implementation */
public:
    ~CSplashWnd();
    virtual void PostNcDestroy();

protected:
    BOOL Create(CWnd* pParentWnd = NULL);
    void HideSplashScreen();
    static BOOL c_bShowLicense;
    static CSplashWnd* c_pSplashWnd;

/* Generated message map functions */
protected:
    //{{AFX_MSG(CSplashWnd)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnPaint();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnKillFocus(CWnd* pNewWnd);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


#endif
