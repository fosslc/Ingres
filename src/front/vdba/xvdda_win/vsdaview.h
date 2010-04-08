/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vsdaview.h : interface of the CvSda class
**    Project  : INGRES II/ Visual Schema Diff Control (vdda.exe).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Frame/View/Doc Architecture (view)
**
** History:
**
** 23-Sep-2002 (uk$so01)
**    Created
** 23-Sep-2004 (uk$so01)
**    BUG #113114 
**    Force the SendMessageToDescendants(WM_SYSCOLORCHANGE) to reflect 
**    immediately the system color changed.
** 12-Oct-2004 (uk$so01)
**    BUG #113215 / ISSUE 13720075,
**    F1-key should invoke always the topic Overview if no comparison's result.
**    Otherwise F1-key should invoke always the ocx.s help.
*/


#if !defined(AFX_VSDAVIEW_H__2F5E535D_CF0B_11D6_87E7_00C04F1F754A__INCLUDED_)
#define AFX_VSDAVIEW_H__2F5E535D_CF0B_11D6_87E7_00C04F1F754A__INCLUDED_
#include "dlgmain.h"
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CuVddaControl;
class CvSda : public CView
{
protected: // create from serialization only
	CvSda();
	DECLARE_DYNCREATE(CvSda)

public:
	CdSda* GetDocument();
	CuVddaControl* GetVsdaControl();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CvSda)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CvSda();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	CuDlgMain* m_pDlgMain;

	// Generated message map functions
protected:
	//{{AFX_MSG(CvSda)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnSysColorChange();
};

#ifndef _DEBUG  // debug version in vsdaview.cpp
inline CdSda* CvSda::GetDocument()
   { return (CdSda*)m_pDocument; }
#endif


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VSDAVIEW_H__2F5E535D_CF0B_11D6_87E7_00C04F1F754A__INCLUDED_)
