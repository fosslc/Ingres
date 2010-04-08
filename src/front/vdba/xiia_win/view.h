// view.h : interface of the CvChildView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_VIEW_H__4E271B4A_FCE9_11D4_873E_00C04F1F754A__INCLUDED_)
#define AFX_VIEW_H__4E271B4A_FCE9_11D4_873E_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CvChildView : public CWnd
{
public:
	CvChildView();


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CvChildView)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CvChildView();

	// Generated message map functions
protected:
	//{{AFX_MSG(CvChildView)
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VIEW_H__4E271B4A_FCE9_11D4_873E_00C04F1F754A__INCLUDED_)
