/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Project : Ingres Visual DBA
**
**    MainVi2.h : header file for right pane of DOM
**
**  26-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
** 30-May-2001 (uk$so01)
**    SIR #104736, Integrate IIA and Cleanup old code of populate.
** 28-Apr-2004 (uk$so01)
**    BUG #112221 / ISSUE 13403162
**    VC7 (.Net) does not call OnInitialUpdate first for this Frame/View/Doc but it does call
**    OnUpdate. So I write void InitUpdate(); that is called once either in OnUpdate or
**    OnInitialUpdate() to resolve this problem.
*/


#ifndef DOMVIEW2_HEADER
#define DOMVIEW2_HEADER

#include "ipmdisp.h"    // CuPageInformation

extern "C" {
#include "main.h"
#include "dom.h"
#include "tree.h"
}

#include "dgdomc02.h"

/////////////////////////////////////////////////////////////////////////////
// C++ Utility functions
CuDomPageInformation* GetDomItemPageInfo(int iobjecttype, LPTREERECORD pItemData, int ingresVer, int hDomNode);

/////////////////////////////////////////////////////////////////////////////
// CMainMfcView2 view

class CMainMfcView2 : public CView
{
protected:
	CMainMfcView2();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CMainMfcView2)

// Attributes
public:

// Operations
public:
  CuDlgDomTabCtrl* m_pDomTabDialog;

  UINT GetHelpID();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainMfcView2)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CMainMfcView2();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	BOOL m_bAlreadyInit;
	void InitUpdate(); 

	// Generated message map functions
protected:
	//{{AFX_MSG(CMainMfcView2)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg long OnQueryOpenCursor(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#endif  // DOMVIEW2_HEADER
