/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qupview.h, Header file 
**    Project  : CA-OpenIngres Visual DBA.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Upper view of the sql frame. It contains a serie of static control 
**               to display time taken to execute the queries.
**
** History:
**
** xx-Oct-1997 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
*/


#if !defined(AFX_QUPVIEW_H__21BF266F_4A04_11D1_A20C_00609725DDAF__INCLUDED_)
#define AFX_QUPVIEW_H__21BF266F_4A04_11D1_A20C_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "qredoc.h"
#include "qstmtdlg.h"

class CvSqlQueryUpperView : public CView
{
protected: 
	CvSqlQueryUpperView();
	DECLARE_DYNCREATE(CvSqlQueryUpperView)

public:

	CuDlgSqlQueryStatement* GetDlgSqlQueryStatement () {return m_pStatementDlg;}
	CdSqlQueryRichEditDoc* GetDocument();


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CvSqlQueryUpperView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

	//
	// Implementation
public:
	virtual ~CvSqlQueryUpperView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	CuDlgSqlQueryStatement* m_pStatementDlg;

	//
	// Generated message map functions
protected:
	//{{AFX_MSG(CvSqlQueryUpperView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in qupview.cpp
inline CdSqlQueryRichEditDoc* CvSqlQueryUpperView::GetDocument()
	{ return (CdSqlQueryRichEditDoc*)m_pDocument; }
#endif


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QUPVIEW_H__21BF266F_4A04_11D1_A20C_00609725DDAF__INCLUDED_)
