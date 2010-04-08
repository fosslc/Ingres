/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vlegend.h, Header file
**    Project  : VDBA
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Legend of pie/chart control
**
** History:
**
** 03-Dec-2001 (uk$so01)
**    Created
*/

#if !defined(AFX_VLEGEND_H__BB588220_E59D_11D5_878E_00C04F1F754A__INCLUDED_)
#define AFX_VLEGEND_H__BB588220_E59D_11D5_878E_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "statfrm.h"

class CvLegend : public CView
{
protected:
	CvLegend();
	DECLARE_DYNCREATE(CvLegend)

public:
	CuStatisticBarLegendList* GetListBoxLegend(){return &m_legendListBox;};

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CvLegend)
	protected:
	virtual void OnDraw(CDC* pDC); 
	//}}AFX_VIRTUAL

	// Implementation
protected:
	virtual ~CvLegend();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	CuStatisticBarLegendList m_legendListBox;


	// Generated message map functions
protected:
	//{{AFX_MSG(CvLegend)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VLEGEND_H__BB588220_E59D_11D5_878E_00C04F1F754A__INCLUDED_)
