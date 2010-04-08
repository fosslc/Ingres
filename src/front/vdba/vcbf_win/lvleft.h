/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : lvleft.h, header file                                                 //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : View (left-pane of the page Configure)                                //
//               See the CONCEPT.DOC for more detail                                   //
****************************************************************************************/

#if !defined(AFX_LVLEFT_H__53F32ED1_2DDF_11D1_A1F6_00609725DDAF__INCLUDED_)
#define AFX_LVLEFT_H__53F32ED1_2DDF_11D1_A1F6_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// lvleft.h : header file
//
#include "cleftdlg.h"

/////////////////////////////////////////////////////////////////////////////
// CConfListViewLeft view

class CConfListViewLeft : public CView
{
protected:
	CConfListViewLeft();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CConfListViewLeft)

	// Attributes
public:
	CConfLeftDlg* GetConfLeftDlg() { return m_pDialog; }
	// Operations
public:
	CDialog* GetCConfLeftDlg(){return m_pDialog;}
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfListViewLeft)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	CConfLeftDlg* m_pDialog;

	virtual ~CConfListViewLeft();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CConfListViewLeft)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LVLEFT_H__53F32ED1_2DDF_11D1_A1F6_00609725DDAF__INCLUDED_)
