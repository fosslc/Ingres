/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : viewrigh.h : header file
**    Project  : INGRESII / Visual Schema Diff Control (vsda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : The right pane for the Vsda Control Window.
**               It is responsible for displaying the Modeless Dialog of the Tree Item.
**
** History:
**
** 26-Aug-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
*/


#if !defined(AFX_VIEWRIGH_H__CC2DA2C9_B8F1_11D6_87D8_00C04F1F754A__INCLUDED_)
#define AFX_VIEWRIGH_H__CC2DA2C9_B8F1_11D6_87D8_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "dgdiffls.h"

class CvSdaRight : public CView
{
protected:
	CvSdaRight(); 
	DECLARE_DYNCREATE(CvSdaRight)

	// Operations
	public:

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CvSdaRight)
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

	// Implementation
protected:
	virtual ~CvSdaRight();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	CuDlgPageDifferentList* m_pPageList;

	// Generated message map functions
protected:
	//{{AFX_MSG(CvSdaRight)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VIEWRIGH_H__CC2DA2C9_B8F1_11D6_87D8_00C04F1F754A__INCLUDED_)
