/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qboxstar.h, header file 
**    Project  : OpenIngres Visual DBA 
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Node of Qep tree. (Star)
**
** History:
**
** xx-Oct-1997 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
*/

#if !defined(AFX_QBOXSTAR_H__84DE24D2_7628_11D1_A232_00609725DDAF__INCLUDED_)
#define AFX_QBOXSTAR_H__84DE24D2_7628_11D1_A232_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "qstatind.h"
#include "transbmp.h"

class CaQepNodeInformationStar;
class CuDlgQueryExecutionPlanBoxStar : public CDialog
{
public:
	CuDlgQueryExecutionPlanBoxStar(CWnd* pParent = NULL); 
	CuDlgQueryExecutionPlanBoxStar(CWnd* pParent, BOOL bPopupInfo); 
	void OnOK() {return;}
	void OnCancel() {return;}
	void SetPopupInfo() {m_bPopupInfo = TRUE;}
	void SetupData(CaQepNodeInformationStar* pData);

	// Dialog Data
	//{{AFX_DATA(CuDlgQueryExecutionPlanBoxStar)
	enum { IDD = IDD_QEPBOX_STAR };
	CString	m_strC;
	CString	m_strD;
	CString	m_strN;
	CString	m_strDB;
	CString	m_strHeader;
	CString	m_strNode;
	CString	m_strPage;
	CString	m_strTuple;
	//}}AFX_DATA
	CuStaticIndicatorWnd m_Indicator1;
	CuStaticIndicatorWnd m_Indicator2;
	CuStaticIndicatorWnd m_Indicator3;

	enum { IDD_POPUPINFO = IDD_QEPBOX_STAR_OVERLAPPED };


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgQueryExecutionPlanBoxStar)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	BOOL m_bPopupInfo;
	CBitmap m_bmpDatabase;
	CBitmap m_bmpPage;
	CBitmap m_bmpTuple;
	CBitmap m_bmpD;
	CBitmap m_bmpC;
	CBitmap m_bmpN;

	CStaticTransparentBitmap m_cBmpDatabase;
	CStaticTransparentBitmap m_cBmpPage;
	CStaticTransparentBitmap m_cBmpTuple;
	CStaticTransparentBitmap m_cBmpD;
	CStaticTransparentBitmap m_cBmpC;
	CStaticTransparentBitmap m_cBmpN;

	// Generated message map functions
	//{{AFX_MSG(CuDlgQueryExecutionPlanBoxStar)
	virtual BOOL OnInitDialog();
	afx_msg void OnMove(int x, int y);
	afx_msg void OnNcPaint();
	//}}AFX_MSG
	afx_msg LONG OnUpdateData    (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QBOXSTAR_H__84DE24D2_7628_11D1_A232_00609725DDAF__INCLUDED_)
