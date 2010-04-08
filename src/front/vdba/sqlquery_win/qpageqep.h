/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qpageqep.h, Header file    (Modeless Dialog) 
**    Project  : CA-OpenIngres Visual DBA.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : It contains a view for drawing the binary tree of boxes of QEP.
**               It contains a document which itself contains a list of qep's trees.
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

#if !defined(AFX_QPAGEQEP_H__21BF267A_4A04_11D1_A20C_00609725DDAF__INCLUDED_)
#define AFX_QPAGEQEP_H__21BF267A_4A04_11D1_A20C_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "qepdoc.h"
#include "qpageqin.h"

class CaQueryQepPageData;
class CuDlgSqlQueryPageQep : public CDialog
{
public:
	CuDlgSqlQueryPageQep(CWnd* pParent = NULL);
	void OnOK (){return;}
	void OnCancel(){return;}
	void NotifyLoad (CaQueryQepPageData* pData);

	void DisplayQepTree (int nTab = 0);
	void SetQepDoc (CdQueryExecutionPlanDoc* pDoc) {m_pDoc = pDoc;}
	// Dialog Data
	//{{AFX_DATA(CuDlgSqlQueryPageQep)
	enum { IDD = IDD_SQLQUERY_PAGEQEP };
	CTabCtrl	m_cTab1;
	CStatic	m_cStat1;
	CEdit   m_cEdit1;
	CString m_strEdit1;
	//}}AFX_DATA
	CEdit   m_cEdit2; 
	CStatic	m_cStat3; // Database
	CString	m_strDatabase;
	LONG m_nStart;
	LONG m_nEnd;
	CdQueryExecutionPlanDoc* m_pDoc;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgSqlQueryPageQep)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	BOOL m_bShowStatement;
	CWnd* m_pCurrentPage;

	void ResizeControls();
	// Generated message map functions
	//{{AFX_MSG(CuDlgSqlQueryPageQep)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSelchangingTab1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData    (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnDisplayHeader (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnHighlightStatement (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData            (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetTabBitmap       (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QPAGEQEP_H__21BF267A_4A04_11D1_A20C_00609725DDAF__INCLUDED_)
