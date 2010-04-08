/*
**  Copyright (c) 2001 Ingres Corporation
*/

/*
**  Name: PropSheet.h : header file
**
**  History:
**
**	05-Jun-2001 (penga03)
**	    Created.
**  15-Nov-2006 (drivi01)
**		SIR 116599
**		Enhanced pre-installer in effort to improve installer usability.
**	01-Mar-2007 (drivi01)
**		Modified code to overwrite painting of banner at the top
**	    of the wizard.
*/

#if !defined(AFX_PROPSHEET_H__0E578486_9B8A_4863_91B1_582307DE0B59__INCLUDED_)
#define AFX_PROPSHEET_H__0E578486_9B8A_4863_91B1_582307DE0B59__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CPropSheet

class CPropSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CPropSheet)

// Construction
public:
	CPropSheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	BITMAPINFO m_Info;

// Attributes
public:
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPropSheet)
	public:
	virtual BOOL OnInitDialog();
	protected:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CPropSheet();

	// Generated message map functions
protected:
	//{{AFX_MSG(CPropSheet)
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnClose();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg BOOL OnEraseBkgnd(CDC *pDC);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg BOOL OnQueryNewPalette(void);
	afx_msg void OnPaletteChanged(CWnd* pFocusWnd);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPSHEET_H__0E578486_9B8A_4863_91B1_582307DE0B59__INCLUDED_)
