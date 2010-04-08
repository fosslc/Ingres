/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Project : Ingres Visual Manager
**
**  Source   : mainview.h
**  Author   : Sotheavut UK (uk$so01)
**
**  View that holds a main dialog
**
** History:
** 14-Dec-1998 (uk$so01)
**    created
** 22-Dec-2000 (noifr01)
**    (SIR 103548) added the m_csii_installation member variable, where the 
**    value of II_INSTALLATION will be stored (in order to avoid reloading it
**    several times)
** 05-Apr-2001 (uk$so01)
**    Move the code of getting installation ID to ivmdml (.h, .cpp) to be more 
**    generic and reusable.
***/


#if !defined(AFX_MAINVIEW_H__63A2E04D_936D_11D2_A2B5_00609725DDAF__INCLUDED_)
#define AFX_MAINVIEW_H__63A2E04D_936D_11D2_A2B5_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "dlgmain.h" // Main Dialog: CuDlgMain

class CvMainView : public CView
{
protected: // create from serialization only
	CvMainView();
	DECLARE_DYNCREATE(CvMainView)

	//
	// Attributes
public:
	CdMainDoc* GetDocument();

	//
	// Operations
public:

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CvMainView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo = NULL);
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnPrint(CDC* pDC, CPrintInfo* pInfo);
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CvMainView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	CuDlgMain* m_pDlgMain;

protected:
	UINT m_nPage;
	CString m_strPrintTime;

	void PrintTitlePage(CDC* pDC, CPrintInfo* pInfo);
	void PrintPageHeader(CDC* pDC, CPrintInfo* pInfo, CString& strHeader);
	void PrintPageNumber(CDC* pDC, CPrintInfo* pInfo);

// Generated message map functions
protected:
	//{{AFX_MSG(CvMainView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in mainview.cpp
inline CdMainDoc* CvMainView::GetDocument()
	{ return (CdMainDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINVIEW_H__63A2E04D_936D_11D2_A2B5_00609725DDAF__INCLUDED_)
