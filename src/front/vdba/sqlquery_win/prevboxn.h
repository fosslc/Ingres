/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : prevboxn.h, header file
**    Project  : OpenIngres Visual DBA 
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Node of Qep tree.(mode preview)
**
** History:
**
** xx-Oct-1997 (uk$so01)
**    Created
** 05-Jul-2001 (uk$so01)
**    SIR #105199. Integrate & Generate XML.
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
*/

#if !defined(PREVBOXN_HEADER)
#define PREVBOXN_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "qstatind.h"
#include "colorind.h"

class CuDlgQueryExecutionPlanBoxPreview : public CDialog
{
public:
	CuDlgQueryExecutionPlanBoxPreview(CWnd* pParent = NULL);   
	void OnOK() {return;}
	void OnCancel() {return;}
	void SetDisplayIndicator (BOOL bDisplay = FALSE){m_bDisplayIndicator = bDisplay;}

	// Dialog Data
	//{{AFX_DATA(CuDlgQueryExecutionPlanBoxPreview)
	enum { IDD = IDD_QEPBOX_PREVIEW };
	//}}AFX_DATA

	BOOL m_bRoot;
	CString m_strName;
	CRect m_rcRect;
	BOOL m_bInitialize;
	CuStaticIndicatorWnd m_Indicator1;
	CuStaticIndicatorWnd m_Indicator2;
	CuStaticColorWnd     m_Color;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgQueryExecutionPlanBoxPreview)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
	//
protected:
	BOOL m_bDisplayIndicator;
	// Generated message map functions
	//{{AFX_MSG(CuDlgQueryExecutionPlanBoxPreview)
	afx_msg void OnMove(int x, int y);
	virtual BOOL OnInitDialog();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData    (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QEPBOXDG_H__DC4E1097_46D7_11D1_A20A_00609725DDAF__INCLUDED_)
