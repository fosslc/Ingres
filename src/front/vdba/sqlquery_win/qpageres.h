/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qpageres.h, Header file    (Modeless Dialog)
**    Project  : CA-OpenIngres Visual DBA.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : It contains a list of rows resulting from the SELECT statement.
**
** History:
**
** xx-Oct-1997 (uk$so01)
**    Created.
** 30-May-2001 (uk$so01)
**    SIR #104736, Integrate IIA and Cleanup old code of populate.
** 05-Jul-2001 (uk$so01)
**    SIR #105199. Integrate & Generate XML.
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 11-Apr-2005 (komve01)
**    SIR #114284/Issue#14055289, Added handler for KeyDown message 
**    (OnKeyDownResultsList) for Results List, to enable copy the 
**    contents of the result list.
*/

#if !defined(AFX_QPAGERES_H__21BF267B_4A04_11D1_A20C_00609725DDAF__INCLUDED_)
#define AFX_QPAGERES_H__21BF267B_4A04_11D1_A20C_00609725DDAF__INCLUDED_
#define __OPING_CURSOR__
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "lsselres.h"
#include "qresurow.h"
#include "rcdepend.h"
#include "calsctrl.h"
#include "qryinfo.h"

class CaCursor;
class CaQuerySelectPageData;
class CaExecParamQueryRow;
class CaSqlQueryProperty;
class CuDlgSqlQueryPageResult : public CDialog
{
public:
	CuDlgSqlQueryPageResult(CWnd* pParent = NULL);
	void OnOK (){return;}
	void OnCancel(){return;}
	void NotifyLoad (CaQuerySelectPageData* pData);
	void BrokenCursor();
	void SetQueryRowParam(CaExecParamQueryRows* pParam);

	void SetStandAlone () {m_bStandAlone = TRUE;}
	void ExcuteSelectStatement (CaConnectInfo* pConnectInfo, LPCTSTR lpszStatement, CaSqlQueryProperty* pProperty);

	// Dialog Data
	//{{AFX_DATA(CuDlgSqlQueryPageResult)
	enum { IDD = IDD_SQLQUERY_PAGERESULT };
	CStatic	m_cStat1;
	CEdit	m_cEdit1;
	CString	m_strEdit1;
	//}}AFX_DATA
	CStatic  m_cStaticTotalFetchedRows;
	CStatic  m_cStat2;
	CEdit    m_cEdit2;
	CString  m_strDatabase;
	LONG     m_nStart;
	LONG     m_nEnd;

	CString m_strNoMoreAvailable;
	BOOL m_bBrokenCursor; // When cursor is close and there are more rows.
	BOOL m_bAllRows;      // When all rows are fetched at one time;

	CuListCtrlSelectResult m_ListCtrl;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgSqlQueryPageResult)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
private:
	BOOL m_bStandAlone; // Stand alone page (not concerned in Frame/View/Doc)
protected:
	CaExecParamQueryRows* m_pQueryRowParam;

protected:
	BOOL m_bShowStatement;
	CImageList m_ImageList;
	BOOL m_bLoaded;
	int  m_nBitmap;

	void AddLineItem (int iSubItem, LPCTSTR lpValue);
	void ResizeControls();
	void AboutFecthing (int nRow, BOOL bEnd = FALSE);
	void InfoTrace (int nFetchRows);

	// Generated message map functions
	//{{AFX_MSG(CuDlgSqlQueryPageResult)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnOutofmemoryList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnclickMfcList1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LONG OnDisplayHeader      (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnHighlightStatement (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnDisplayResult      (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData            (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnFetchRows          (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnCloseCursor        (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetTabBitmap       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnChangeSetting      (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetSqlFont         (WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnKeyDownResultsList(NMHDR *pNMHDR, LRESULT *pResult);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QPAGERES_H__21BF267B_4A04_11D1_A20C_00609725DDAF__INCLUDED_)
