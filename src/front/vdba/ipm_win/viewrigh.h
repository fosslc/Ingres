/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : viewrigh.h, Header File
**    Project  : INGRESII / Monitoring.
**    Author   : UK Sotheavut (old code from ipmview2.cpp)
**    Purpose  : The right pane for the Monitor Window.
**               It is responsible for displaying the Modeless Dialog of the Tree Item.
**
** History:
**
** 12-Nov-2001 (uk$so01)
**    Created
**
*/


#ifndef IPM_VIEWRIGHT_HEADER
#define IPM_VIEWRIGHT_HEADER
#include "dgipmc02.h"
#include "ipmdisp.h"
#include "ctltree.h"

class CvIpmRight : public CView
{
protected:
	CvIpmRight();
	DECLARE_DYNCREATE(CvIpmRight)


	// Operations
public:
	CuDlgIpmTabCtrl* m_pIpmTabDialog;
	void DisplayProperty (CuPageInformation* pPageInfo);
	UINT GetHelpID() {return m_pIpmTabDialog? m_pIpmTabDialog->GetCurrentPageID(): 0;}
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CvIpmRight)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC); // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	//}}AFX_VIRTUAL

	// Implementation
protected:
	virtual ~CvIpmRight();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	CuTreeCtrlInvisible m_treeCtrl; // Create this control only if we has only one view (no splitter):

	// Generated message map functions
protected:
	//{{AFX_MSG(CvIpmRight)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // IPM_VIEWRIGHT_HEADER
