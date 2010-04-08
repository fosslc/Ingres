/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xdgabout.h : header file
**    Project  : DLL (About Box). 
**    Author   : Sotheavut UK (uk$so01)
**
**
** History:
**
** 06-Nov-2001 (uk$so01)
**    created
** 08-Nov-2001 (uk$so01)
**    SIR #106290 (Handle the 255 palette)
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 11-Jan-2002 (schph01 for uk$so01)
**    (SIR 106290) allow the caller to pass the bitmap
** 30-Jul-2004 (uk$so01)
**    SIR #112708, Change the AboutBox due to UI Standard.
** 29-Apr-2008 (drivi01)
**    Move things around in about box.  Separate version from an application name.
*/



#if !defined(AFX_XDGABOUT_H__295E5521_D297_11D5_8787_00C04F1F754A__INCLUDED_)
#define AFX_XDGABOUT_H__295E5521_D297_11D5_8787_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "256bmp.h"
#include "transbmp.h"
#include "afxwin.h"
class CxDlgAbout : public CDialog
{
public:
	CxDlgAbout(CWnd* pParent = NULL); 
	void SetTitle(LPCTSTR lpszTitle){m_strTitle = lpszTitle;}
	void SetVersion(LPCTSTR lpszProductVersion){m_strProductVersion = lpszProductVersion;}
	void SetYear (short nYear){m_nYear = nYear;}
	void SetExternalBitmap (HINSTANCE hExternalInstance, UINT nBitmap);

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	void SetShowMask (UINT nMask) {m_nShowMask = nMask;}

	// Dialog Data
	//{{AFX_DATA(CxDlgAbout)
	enum { IDD = IDD_XABOUT };
	CButton	m_cButtonOK;
	//}}AFX_DATA
	C256bmp m_bitmap;


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgAbout)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX); 
	//}}AFX_VIRTUAL

	// Implementation
private:
	COLORREF m_rgbBkColor;
	COLORREF m_rgbFgColor;
	CBrush*  m_pEditBkBrush;
	UINT     m_nShowMask;

protected:
	CToolTipCtrl m_tooltip;
	CFont   m_fontBold;
	CFont   m_font;
	CFont   m_font2;
	HICON   m_hIcon;
	BOOL    m_bChangeCursor;
	CString m_strTitle;
	CString m_strProductVersion;
	short   m_nYear;
	CStatic m_cStatic1;
	CStatic m_cStatic1_5;
	CStatic m_cStatic2;
	CStatic m_cStatic3;
	CStaticTransparentBitmap m_cStaticHyperLink;
	HCURSOR m_hcArrow;
	HCURSOR m_hcHand;
	CBitmap m_bmpHyperlink;


	// Generated message map functions
	//{{AFX_MSG(CxDlgAbout)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	CButton m_cButton3Party;
	CButton m_cButtonSystemInfo;
	CButton m_cButtonTechSupport;
public:
	afx_msg void OnBnClickedButton3Party();
	afx_msg void OnBnClickedButtonSystemInfo();
	afx_msg void OnBnClickedButtonTechSupport();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XDGABOUT_H__295E5521_D297_11D5_8787_00C04F1F754A__INCLUDED_)
