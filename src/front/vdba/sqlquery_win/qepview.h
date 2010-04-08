/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qepview.h, Header file 
**    Project  : CA-OpenIngres Visual DBA.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : The view for drawing the QEP's Tree.
**
** History:
**
** xx-Oct-1997 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
*/

#if !defined(AFX_QEPVIEW_H__DC4E108E_46D7_11D1_A20A_00609725DDAF__INCLUDED_)
#define AFX_QEPVIEW_H__DC4E108E_46D7_11D1_A20A_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "qepboxdg.h"
#include "qepbtree.h"

class CvQueryExecutionPlanView : public CScrollView
{
protected: // create from serialization only
	CvQueryExecutionPlanView();
	DECLARE_DYNCREATE(CvQueryExecutionPlanView)

	void DrawChildStart (CaSqlQueryExecutionPlanBinaryTree* pQepBinaryTree, CDC* pDC, BOOL bPreview = FALSE);
	void DrawQepBoxes   (CaSqlQueryExecutionPlanBinaryTree* pQepBinaryTree, CDC* pDC, BOOL bPreview = FALSE);
	void GetStartingPoint (CWnd* pBox, CPoint& p, int nSon);
	void GetEndingPoint   (CWnd* pBox, CPoint& p);

	void CalculateDepth (CaSqlQueryExecutionPlanBinaryTree* pQepBinaryTree, int& nDepth, int level = 0);


public:
	CdQueryExecutionPlanDoc* GetDocument();
	void InitialDrawing ();
	void SetMode (BOOL bPreview);



	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CvQueryExecutionPlanView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	//}}AFX_VIRTUAL

	//
	// Implementation
public:
	virtual ~CvQueryExecutionPlanView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	POSITION m_nSequence;
	BOOL m_bPreview;
	BOOL m_bCalPositionPreview;
	BOOL m_bCalPosition;      // It concerns only the mode normal.  
	                          // (This variable indicates if we should calculate the position)
	BOOL m_bDrawingFailed;    // No painting/drawing will be taken if this variable is TRUE
	BOOL m_bCalSizeBox;       // Indicate if the size of qep box (normal) is calculated
	BOOL m_bCalSizeBoxPreview;// Indicate if the size of qep box (preview) is calculated
	CWnd* m_pPopupInfoWnd;    // Pointer to the Qep's box. (Popup info on LButtonDow of Preview Box)
	CRect m_rcPopupInfo;
	CuDlgQueryExecutionPlanBox*         m_pPopupInfo;
	CuDlgQueryExecutionPlanBoxLeaf*     m_pPopupInfoLeaf;
	CuDlgQueryExecutionPlanBoxStar*     m_pPopupInfoStar;
	CuDlgQueryExecutionPlanBoxStarLeaf* m_pPopupInfoStarLeaf;

	void CalcQepBoxSize (CaSqlQueryExecutionPlanData* pTreeData);
	//
	// Generated message map functions
protected:
	//{{AFX_MSG(CvQueryExecutionPlanView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//}}AFX_MSG
	afx_msg LONG OnRedrawLinks   (UINT wParam, LONG lParam);
	afx_msg LONG OnOpenPopupInfo (UINT wParam, LONG lParam);
	afx_msg LONG OnClosePopupInfo(UINT wParam, LONG lParam);
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in qepview.cpp
inline CdQueryExecutionPlanDoc* CvQueryExecutionPlanView::GetDocument()
	{ return (CdQueryExecutionPlanDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QEPVIEW_H__DC4E108E_46D7_11D1_A20A_00609725DDAF__INCLUDED_)
