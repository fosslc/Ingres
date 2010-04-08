/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : rvstatus.h, header file
**    Project  : CA-OpenIngres/Replicator.
**    Author   : UK Sotheavut
**    Purpose  : Page of a Replication Server Item  (Status)
**
** History:
**
** xx-Dec-1997 (uk$so01)
**    Created
*/


#if !defined(AFX_RVSTATUS_H__360829A7_6A76_11D1_A22D_00609725DDAF__INCLUDED_)
#define AFX_RVSTATUS_H__360829A7_6A76_11D1_A22D_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
class CuDlgReplicationServerPageStatus : public CDialog
{
public:
	int GetOwnerOfDatabase(CString& rcsTempOwner);
	CuDlgReplicationServerPageStatus(CWnd* pParent = NULL);  
	void OnOK(){return;}
	void OnCancel(){return;}
	void EnableButtons();

	// Dialog Data
	//{{AFX_DATA(CuDlgReplicationServerPageStatus)
	enum { IDD = IDD_REPSERVER_PAGE_STATUS };
	CButton	m_cButtonStop;
	CButton	m_cButtonStart;
	CEdit	m_cEdit2;
	CEdit	m_cEdit1;
	CString	m_strEdit1;
	CString	m_strEdit2;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgReplicationServerPageStatus)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL


	//
	// Implementation
protected:
	REPLICSERVERDATAMIN m_SvrDta;

	// Generated message map functions
	//{{AFX_MSG(CuDlgReplicationServerPageStatus)
	afx_msg void OnButtonStart();
	afx_msg void OnButtonStop();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnButtonViewLog();
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLeavingPage  (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
private:
	bool m_bDisplayFirstTime;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RVSTATUS_H__360829A7_6A76_11D1_A22D_00609725DDAF__INCLUDED_)
