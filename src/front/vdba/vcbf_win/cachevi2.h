/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : cachevi2.h, header file                                               //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Right pane of DBMS Cache page                                         //
****************************************************************************************/

#if !defined(AFX_CACHEVI2_H__6722F084_3B35_11D1_A201_00609725DDAF__INCLUDED_)
#define AFX_CACHEVI2_H__6722F084_3B35_11D1_A201_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// cachevi2.h : header file
//
#include "cachetab.h"

/////////////////////////////////////////////////////////////////////////////
// CvDbmsCacheViewRight view

class CvDbmsCacheViewRight : public CView
{
protected:
	CvDbmsCacheViewRight();     
	DECLARE_DYNCREATE(CvDbmsCacheViewRight)

public:
	CuDlgCacheTab* GetCacheTabDlg() {return m_pDlgCacheTab;};
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CvDbmsCacheViewRight)
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	CuDlgCacheTab* m_pDlgCacheTab;

	virtual ~CvDbmsCacheViewRight();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CvDbmsCacheViewRight)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnValidateData (WPARAM wParam, LPARAM lParam);
	//
	// Generic button: You must spesify the action of what you expected !!!
	afx_msg LONG OnButton1 (UINT wParam, LONG lParam);
	afx_msg LONG OnButton2 (UINT wParam, LONG lParam);
	afx_msg LONG OnButton3 (UINT wParam, LONG lParam);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CACHEVI2_H__6722F084_3B35_11D1_A201_00609725DDAF__INCLUDED_)
