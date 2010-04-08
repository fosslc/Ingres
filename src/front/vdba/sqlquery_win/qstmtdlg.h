/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qstmtdlg.h, Header file    (Modeless Dialog)
**    Project  : CA-OpenIngres Visual DBA
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : It contains an Edit for SQL statement and other static controls that 
**               hold the information about execute time, elap time, 
** 
** History:
**
** xx-Oct-1997 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
*/

#if !defined(AFX_QSTMTDLG_H__21BF2678_4A04_11D1_A20C_00609725DDAF__INCLUDED_)
#define AFX_QSTMTDLG_H__21BF2678_4A04_11D1_A20C_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
class CaSqlQueryMeasureData;

class CuDlgSqlQueryStatement : public CDialog
{
public:
	CuDlgSqlQueryStatement(CWnd* pParent = NULL);
	void OnOK (){return;}
	void OnCancel(){return;}
	void DisplayStatistic (CaSqlQueryMeasureData* pData);
	void LoadDocument();

	// Dialog Data
	//{{AFX_DATA(CuDlgSqlQueryStatement)
	enum { IDD = IDD_SQLQUERY_STATEMENT };
	CString m_strCom;
	CString m_strExec;
	CString m_strElap;
	CString m_strCost;
	//}}AFX_DATA

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgSqlQueryStatement)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CuDlgSqlQueryStatement)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QSTMTDLG_H__21BF2678_4A04_11D1_A20C_00609725DDAF__INCLUDED_)
