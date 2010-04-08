/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/* 
**    Source   : qepboxdg.h, header file 
**    Project  : OpenIngres Visual DBA 
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Node of Qep tree.
**
** History:
**
** xx-Oct-1997 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
*/

#if !defined(AFX_QEPBOXDG_H__DC4E1097_46D7_11D1_A20A_00609725DDAF__INCLUDED_)
#define AFX_QEPBOXDG_H__DC4E1097_46D7_11D1_A20A_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "qstatind.h"
#include "transbmp.h"

class CaQepNodeInformation;
class CuDlgQueryExecutionPlanBox : public CDialog
{
public:
	CuDlgQueryExecutionPlanBox(CWnd* pParent = NULL);
	CuDlgQueryExecutionPlanBox(CWnd* pParent, BOOL bPopupInfo);
	void OnOK() {return;}
	void OnCancel() {return;}
	void SetPopupInfo() {m_bPopupInfo = TRUE;}
	void SetupData(CaQepNodeInformation* pData);

	// Dialog Data
	//{{AFX_DATA(CuDlgQueryExecutionPlanBox)
	enum { IDD = IDD_QEPBOX };
	CString	m_strC;
	CString	m_strD;
	CString	m_strHeader;
	CString	m_strPage;
	CString	m_strTuple;
	//}}AFX_DATA
	enum { IDD_POPUPINFO = IDD_QEPBOX_OVERLAPPED };
	BOOL m_bRoot;
	CString m_strName;
	CRect m_rcRect;
	BOOL m_bInitialize;
	CuStaticIndicatorWnd m_Indicator1;
	CuStaticIndicatorWnd m_Indicator2;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgQueryExecutionPlanBox)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
	//
protected:
	BOOL m_bPopupInfo;
	CBitmap m_bmpPage;
	CBitmap m_bmpTuple;
	CBitmap m_bmpD;
	CBitmap m_bmpC;

	CStaticTransparentBitmap m_cBmpPage;
	CStaticTransparentBitmap m_cBmpTuple;
	CStaticTransparentBitmap m_cBmpD;
	CStaticTransparentBitmap m_cBmpC;

	// Generated message map functions
	//{{AFX_MSG(CuDlgQueryExecutionPlanBox)
	afx_msg void OnMove(int x, int y);
	virtual BOOL OnInitDialog();
	afx_msg void OnNcPaint();
	//}}AFX_MSG
	afx_msg LONG OnUpdateData    (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QEPBOXDG_H__DC4E1097_46D7_11D1_A20A_00609725DDAF__INCLUDED_)
