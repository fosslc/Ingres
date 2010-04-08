/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vcdaview.h : interface of the CvCda class
**    Project  : VCDA (Container)
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : frame/view/doc architecture (view)
**
** History:
**
** 01-Oct-2002 (uk$so01)
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
**    Created
** 23-Sep-2004 (uk$so01)
**    BUG #113114 
**    Force the SendMessageToDescendants(WM_SYSCOLORCHANGE) to reflect 
**    immediately the system color changed.
**/

#if !defined(AFX_VCDAVIEW_H__EAF345DD_D514_11D6_87EA_00C04F1F754A__INCLUDED_)
#define AFX_VCDAVIEW_H__EAF345DD_D514_11D6_87EA_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "udlgmain.h"

class CdCda;
class CvCda : public CView
{
protected: // create from serialization only
	CvCda();
	DECLARE_DYNCREATE(CvCda)

public:
	CdCda* GetDocument();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CvCda)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CvCda();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	CuDlgMain* m_pDlgMain;


	// Generated message map functions
protected:
	//{{AFX_MSG(CvCda)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnSysColorChange();
};

#ifndef _DEBUG  // debug version in vcdaview.cpp
inline CdCda* CvCda::GetDocument()
   { return (CdCda*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VCDAVIEW_H__EAF345DD_D514_11D6_87EA_00C04F1F754A__INCLUDED_)
