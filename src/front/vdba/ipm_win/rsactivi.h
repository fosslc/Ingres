/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/


/*
**  Project  : Visual DBA
**  Source   : rsactivi.h, header file 
**  Author   : UK Sotheavut 
**  Purpose  : Page of a static item Replication.  (Activity)
**
** History:
**
** xx-Dec-1997 (uk$so01)
**    Created.
*/

#if !defined(AFX_RSACTIVI_H__360829A1_6A76_11D1_A22D_00609725DDAF__INCLUDED_)
#define AFX_RSACTIVI_H__360829A1_6A76_11D1_A22D_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "calsctrl.h"
#include "rsactidb.h"
#include "rsactitb.h"

class CuDlgReplicationStaticPageActivity : public CDialog
{
public:
	CuDlgReplicationStaticPageActivity(CWnd* pParent = NULL);  
	void OnOK(){return;}
	void OnCancel(){return;}

	void DisplayPage(int nPage = -1);
	// Dialog Data
	//{{AFX_DATA(CuDlgReplicationStaticPageActivity)
	enum { IDD = IDD_REPSTATIC_PAGE_ACTIVITY };
	CButton	m_cButtonNow;
	CTabCtrl	m_cTab1;
	CString	m_strInputQueue;
	CString	m_strDistributionQueue;
	CString	m_strStartingTime;
	//}}AFX_DATA

	enum {MODE_OUTGOING, MODE_INCOMING, MODE_TOTAL};
	CuListCtrl m_cListCtrl;
	CImageList m_ImageList;
	CImageList m_tabImageList;
	CWnd*      m_pCurrentPage;

	CuDlgReplicationStaticPageActivityPerDatabase* m_pDatabaseOutgoing;
	CuDlgReplicationStaticPageActivityPerDatabase* m_pDatabaseIncoming;
	CuDlgReplicationStaticPageActivityPerDatabase* m_pDatabaseTotal;

	CuDlgReplicationStaticPageActivityPerTable*    m_pTableOutgoing;
	CuDlgReplicationStaticPageActivityPerTable*    m_pTableIncoming;
	CuDlgReplicationStaticPageActivityPerTable*    m_pTableTotal;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgReplicationStaticPageActivity)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	void UpdateOutgoing (LPRAISEDREPLICDBE pData);
	void UpdateIncoming (LPRAISEDREPLICDBE pData);
	void UpdateSummary    (int iColumn, int nMode);
	void UpdateDatabase   (int iColumn, LPRAISEDREPLICDBE pData, int nMode);
	void UpdateTable      (int iColumn, LPRAISEDREPLICDBE pData, int nMode);
	void UpdateDbTotal    (int iColumn, LPRAISEDREPLICDBE pData);
	void UpdateTbTotal    (int iColumn, LPRAISEDREPLICDBE pData);


	// Generated message map functions
	//{{AFX_MSG(CuDlgReplicationStaticPageActivity)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnButtonNow();
	afx_msg void OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnDbeventIncoming (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLeavingPage(WPARAM wParam, LPARAM lParam);
	afx_msg long OnPropertiesChange(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RSACTIVI_H__360829A1_6A76_11D1_A22D_00609725DDAF__INCLUDED_)
