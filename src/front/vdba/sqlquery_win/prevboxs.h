/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : prevboxs.h, header file
**    Project  : OpenIngres Visual DBA
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Node of Qep tree. (Star mode preview)
**
** History:
**
** xx-Feb-1998 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
*/

#if !defined(PREVBOXS_HEADER)
#define PREVBOXS_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "qstatind.h"
#include "colorind.h"

class CuDlgQueryExecutionPlanBoxStarPreview : public CDialog
{
public:
	CuDlgQueryExecutionPlanBoxStarPreview(CWnd* pParent = NULL); 
	void OnOK() {return;}
	void OnCancel() {return;}
	void SetDisplayIndicator (BOOL bDisplay = FALSE){m_bDisplayIndicator = bDisplay;}

	// Dialog Data
	//{{AFX_DATA(CuDlgQueryExecutionPlanBoxStarPreview)
	enum { IDD = IDD_QEPBOX_STAR_PREVIEW };

	//}}AFX_DATA
	CuStaticIndicatorWnd m_Indicator1;
	CuStaticIndicatorWnd m_Indicator2;
	CuStaticIndicatorWnd m_Indicator3;
	CuStaticColorWnd     m_Color;


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgQueryExecutionPlanBoxStarPreview)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	BOOL m_bDisplayIndicator;
	// Generated message map functions
	//{{AFX_MSG(CuDlgQueryExecutionPlanBoxStarPreview)
	virtual BOOL OnInitDialog();
	afx_msg void OnMove(int x, int y);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData    (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QBOXSTAR_H__84DE24D2_7628_11D1_A232_00609725DDAF__INCLUDED_)
