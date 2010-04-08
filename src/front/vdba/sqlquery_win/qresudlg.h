/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qresudlg.h, Header file    (Modeless Dialog)
**    Project  : CA-OpenIngres Visual DBA
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : It contains a CTab Control to manage displaying the different pages 
**               of the result (history of query result).
**
** History:
** xx-Oct-1997 (uk$so01)
**   Created.
** 31-Jan-2000 (uk$so01)
**    (Bug #100235)
**    Special SQL Test Setting when running on Context.
** 05-Jul-2001 (uk$so01)
**    SIR #105199. Integrate & Generate XML.
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
*/

#if !defined(AFX_QRESUDLG_H__21BF2679_4A04_11D1_A20C_00609725DDAF__INCLUDED_)
#define AFX_QRESUDLG_H__21BF2679_4A04_11D1_A20C_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "qpageraw.h"

class CaSqlQueryProperty;
class CdSqlQueryRichEditDoc;
class CuDlgSqlQueryResult : public CDialog
{
public:
	CuDlgSqlQueryResult(CWnd* pParent = NULL);
	void OnOK (){return;}
	void OnCancel(){return;}

	void DisplayRawPage (BOOL bDisplay = TRUE);
	void DisplayPage (CWnd* pPage = NULL, BOOL bLoaded = FALSE, int nImage = -1);
	void DisplayPageHeader (int nStart = 0);
	void GetData ();
	void UpdatePage(int nPage);
	void CloseCursor();
	void UpdateTabBitmaps();
	int  GetOpenedCursorCount();
	CWnd* GetCurrentPage(){return m_pCurrentPage;}
	CuDlgSqlQueryPageRaw* GetRawPage();
	CaQueryPageData* GetCurrentPageData();
	void SelectRawPage();

	//
	// When the select statement calls this function to display the information about TRACE
	// it should pass the cursor's name in the 'lpszCursorName' to identify the caller.
	// For the non select statement this parameter 'lpszCursorName' should be null.
	void DisplayTraceLine (LPCTSTR strStatement, LPCTSTR lpszTrace, LPCTSTR lpszCursorName = NULL);

	void Cleanup();
	void LoadDocument();
	void SettingChange(UINT nMask, CaSqlQueryProperty* pProperty);

	// Dialog Data
	//{{AFX_DATA(CuDlgSqlQueryResult)
	enum { IDD = IDD_SQLQUERY_RESULT };
	CTabCtrl m_cTab1;
	//}}AFX_DATA
	CImageList m_ImageList;
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgSqlQueryResult)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	BOOL  m_bMaxPageReached;
	CWnd* m_pCurrentPage;
	CuDlgSqlQueryPageRaw* m_pRawPage;
	BOOL  m_bInitialCreated;

	void ChangeTab(BOOL bLoaded = FALSE);
	void DestroyTab(CdSqlQueryRichEditDoc* pDoc, BOOL bAdded=TRUE);
	// Generated message map functions
	//{{AFX_MSG(CuDlgSqlQueryResult)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LONG OnSQLStatementChange  (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnExecuteSQLStatement (WPARAM wParam, LPARAM lParam);
	
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QRESUDLG_H__21BF2679_4A04_11D1_A20C_00609725DDAF__INCLUDED_)
