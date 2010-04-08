/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : stainsta.h , Header File
**    Project  : Ingres II / Visual Manager 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Status Page for Installation branch
**
** History:
**
** 16-Dec-1998 (uk$so01)
**    created
** 01-Mar-2000 (uk$so01)
**    SIR #100635, Add the Functionality of Find operation.
** 16-Sep-2004 (uk$so01)
**    BUG #113060, IVM's checkbox “Service Mode” stays disable for a while (about 15 seconds) 
**    even if the Installation has been completely stopped.
**    Add a timer to monitor this checkbox every second, instead of waiting until the 
**    OnUpdateData has been called.
**/

#if !defined(AFX_STAINSTA_H__5D002661_94F4_11D2_A2B7_00609725DDAF__INCLUDED_)
#define AFX_STAINSTA_H__5D002661_94F4_11D2_A2B7_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "dginstt1.h"
#include "dginstt2.h"

class CuDlgStatusInstallation : public CDialog
{
public:
	CuDlgStatusInstallation(CWnd* pParent = NULL);
	void OnOK(){return;}
	void OnCancel(){return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgStatusInstallation)
	enum { IDD = IDD_STATUS_INSTALLATION };
	CButton	m_cCheckAsService;
	CTabCtrl	m_cTab1;
	BOOL	m_bUnreadCriticalEvent;
	BOOL	m_bStarted;
	BOOL	m_bStartedAll;
	BOOL	m_bStartedMore;
	BOOL	m_bAsService;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgStatusInstallation)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void DisplayPage();
	void ServiceMode();

	CuDlgStatusInstallationTab1* m_pPage1;
	CuDlgStatusInstallationTab2* m_pPage2;
	CWnd* m_pCurrentPage;
	UINT m_nTimer1;

	// Generated message map functions
	//{{AFX_MSG(CuDlgStatusInstallation)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonSysInformation();
	afx_msg void OnCheckAsService();
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnComponentChanged (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnNewEvents (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnFindAccepted (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnFind (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnTimer(UINT nIDEvent);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STAINSTA_H__5D002661_94F4_11D2_A2B7_00609725DDAF__INCLUDED_)
