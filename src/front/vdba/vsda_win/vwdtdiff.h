/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vwdtdiff.h : header file
**    Project  : INGRESII / Visual Schema Diff Control (vsda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : View for detail difference page
**
** History:
**
** 10-Dec-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
** 17-May-2004 (uk$so01)
**    SIR #109220, ISSUE 13407277: additional change to put the header
**    in the pop-up for an item's detail of difference.
*/

#if !defined(AFX_VWDTDIFF_H__6F33C6A4_0B84_11D7_87F9_00C04F1F754A__INCLUDED_)
#define AFX_VWDTDIFF_H__6F33C6A4_0B84_11D7_87F9_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "dgdiffpg.h"
class CvDifferenceDetail : public CView
{
protected:
	CvDifferenceDetail(); // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CvDifferenceDetail)

public:
	CuDlgDifferenceDetailPage* GetDetailPage(){return m_pDetailPage;}
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CvDifferenceDetail)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CuDlgDifferenceDetailPage* m_pDetailPage;
	virtual ~CvDifferenceDetail();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CvDifferenceDetail)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VWDTDIFF_H__6F33C6A4_0B84_11D7_87F9_00C04F1F754A__INCLUDED_)
