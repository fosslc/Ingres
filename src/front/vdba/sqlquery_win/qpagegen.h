/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qpagegen.h, Header file    (Modeless Dialog) 
**    Project  : CA-OpenIngres Visual DBA. 
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : It contains an edit to show a non-select statement.
**               This dialog is displayed upon the setting.
** 
** History:
** xx-Oct-1997 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
*/

#if !defined(AFX_QPAGEGEN_H__2F0A8BF1_4AB9_11D1_A20D_00609725DDAF__INCLUDED_)
#define AFX_QPAGEGEN_H__2F0A8BF1_4AB9_11D1_A20D_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// qpagegen.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CuDlgSqlQueryPageGeneric dialog

class CuDlgSqlQueryPageGeneric : public CDialog
{
public:
	CuDlgSqlQueryPageGeneric(CWnd* pParent = NULL);   // standard constructor
	void OnOK (){return;}
	void OnCancel(){return;}
	void NotifyLoad (CaQueryNonSelectPageData* pData);

	// Dialog Data
	//{{AFX_DATA(CuDlgSqlQueryPageGeneric)
	enum { IDD = IDD_SQLQUERY_PAGEGENERIC };
	CEdit   m_cEdit1;
	CString m_strEdit1;
	//}}AFX_DATA
	CString	m_strDatabase;
	LONG m_nStart;
	LONG m_nEnd;


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgSqlQueryPageGeneric)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	BOOL m_bShowStatement;

	void ResizeControls();
	// Generated message map functions
	//{{AFX_MSG(CuDlgSqlQueryPageGeneric)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LONG OnDisplayHeader      (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnHighlightStatement (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData            (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetTabBitmap       (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QPAGEGEN_H__2F0A8BF1_4AB9_11D1_A20D_00609725DDAF__INCLUDED_)
