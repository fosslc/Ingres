/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qboxleaf.h, header file 
**    Project  : OpenIngres Visual DBA 
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Node of Qep tree. (Leaf)
**
** History:
**
** xx-Dec-1997 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
*/

#if !defined(AFX_QBOXLEAF_H__84DE24D1_7628_11D1_A232_00609725DDAF__INCLUDED_)
#define AFX_QBOXLEAF_H__84DE24D1_7628_11D1_A232_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "transbmp.h"

class CaQepNodeInformation;
class CuDlgQueryExecutionPlanBoxLeaf : public CDialog
{
public:
	CuDlgQueryExecutionPlanBoxLeaf(CWnd* pParent = NULL);
	CuDlgQueryExecutionPlanBoxLeaf(CWnd* pParent, BOOL bPopupInfo);
	void OnOK() {return;}
	void OnCancel() {return;}
	void SetPopupInfo() {m_bPopupInfo = TRUE;}
	void SetupData(CaQepNodeInformation* pData);

	// Dialog Data
	//{{AFX_DATA(CuDlgQueryExecutionPlanBoxLeaf)
	enum { IDD = IDD_QEPBOX_LEAF };
	CString	m_strHeader;
	CString	m_strPage;
	CString	m_strTuple;
	//}}AFX_DATA
	enum { IDD_POPUPINFO = IDD_QEPBOX_LEAF_OVERLAPPED };


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgQueryExecutionPlanBoxLeaf)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	BOOL m_bPopupInfo;
	CBitmap m_bmpPage;
	CBitmap m_bmpTuple;

	CStaticTransparentBitmap m_cBmpPage;
	CStaticTransparentBitmap m_cBmpTuple;

	// Generated message map functions
	//{{AFX_MSG(CuDlgQueryExecutionPlanBoxLeaf)
	virtual BOOL OnInitDialog();
	afx_msg void OnMove(int x, int y);
	afx_msg void OnNcPaint();
	//}}AFX_MSG
	afx_msg LONG OnUpdateData    (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QBOXLEAF_H__84DE24D1_7628_11D1_A232_00609725DDAF__INCLUDED_)
