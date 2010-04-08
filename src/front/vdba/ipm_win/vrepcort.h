/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vrepcort.h, Header file
**    Project  : INGRESII / Monitoring.
**    Author   : UK Sotheavut
**    Purpose  : The view containing the modeless dialog to display the conflict rows 
**
** History:
**
** xx-May-1998 (uk$so01)
**    Created
*/

#if !defined(AFX_VREPCORT_H__09BFF513_EB0A_11D1_A27C_00609725DDAF__INCLUDED_)
#define AFX_VREPCORT_H__09BFF513_EB0A_11D1_A27C_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "dgrepcor.h"
#include "dgrepco2.h"

class CvReplicationPageCollisionViewRight : public CView
{
protected:
	CvReplicationPageCollisionViewRight();
	DECLARE_DYNCREATE(CvReplicationPageCollisionViewRight)


	//
	// Operations
public:
	CuDlgReplicationPageCollisionRight*  m_pDlg;
	CuDlgReplicationPageCollisionRight2* m_pDlg2;
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CvReplicationPageCollisionViewRight)
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	BOOL m_bTransaction; // TRUE: The current dialog is 'm_pDlg2' otherwise  he current dialog is 'm_pDlg'

	virtual ~CvReplicationPageCollisionViewRight();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CvReplicationPageCollisionViewRight)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg long OnPropertiesChange(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VREPCORT_H__09BFF513_EB0A_11D1_A27C_00609725DDAF__INCLUDED_)
