/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : cachevi1.h, header file                                               //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Left pane of DBMS Cache page                                          //
****************************************************************************************/

#if !defined(AFX_CACHEVI1_H__6722F083_3B35_11D1_A201_00609725DDAF__INCLUDED_)
#define AFX_CACHEVI1_H__6722F083_3B35_11D1_A201_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// cachevi1.h : header file
//
#include "cachelst.h" 
/////////////////////////////////////////////////////////////////////////////
// CvDbmsCacheViewLeft view

class CvDbmsCacheViewLeft : public CView
{
protected:
	CvDbmsCacheViewLeft();  
	DECLARE_DYNCREATE(CvDbmsCacheViewLeft)


public:
	CuCacheCheckListCtrl* GetCacheListCtrl() {return m_pDlgCacheList->GetCacheListCtrl();}

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CvDbmsCacheViewLeft)
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	CuDlgCacheList* m_pDlgCacheList;
	virtual ~CvDbmsCacheViewLeft();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CvDbmsCacheViewLeft)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
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

#endif // !defined(AFX_CACHEVI1_H__6722F083_3B35_11D1_A201_00609725DDAF__INCLUDED_)
