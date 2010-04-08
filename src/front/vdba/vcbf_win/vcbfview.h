/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Project : Ingres Configuration Manager
**
**  Source   : vcbfview.h
**
**  interface of the CVcbfView class
**
**  22-Dec-2000 (noifr01)
**   (SIR 103548) added the m_csii_installation member variable ,where the 
**   value of II_INSTALLATION will be stored (in order to avoid reloading it
**   several times)
** 05-Apr-2001 (uk$so01)
**    (SIR 103548) only allow one vcbf per installation ID
**
*/

#include "mtabdlg.h"
#include "vcbfdoc.h"

class CVcbfView : public CView
{
protected: // create from serialization only
	CVcbfView();
	DECLARE_DYNCREATE(CVcbfView)

// Attributes
public:
	CVcbfDoc* GetDocument();
	CMainTabDlg * m_pMainTabDlg;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVcbfView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CVcbfView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CVcbfView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in vcbfview.cpp
inline CVcbfDoc* CVcbfView::GetDocument()
   { return (CVcbfDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////
