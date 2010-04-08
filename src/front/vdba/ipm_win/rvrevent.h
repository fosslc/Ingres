/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : rvrevent.h, header file 
**    Project  : CA-OpenIngres/Replicator.
**    Author   : UK Sotheavut
**    Purpose  : Page of a Replication Server Item  (Raise Event)
**
** History:
**
** xx-Dec-1997 (uk$so01)
**    Created
**
*/

#if !defined(AFX_RVREVENT_H__360829A9_6A76_11D1_A22D_00609725DDAF__INCLUDED_)
#define AFX_RVREVENT_H__360829A9_6A76_11D1_A22D_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "calsctrl.h"
#include "repevent.h"

class CuDlgReplicationServerPageRaiseEvent : public CDialog
{
public:
	CuDlgReplicationServerPageRaiseEvent(CWnd* pParent = NULL); 
	void OnOK(){return;}
	void OnCancel(){return;}
	void AddItem (CaReplicationRaiseDbevent* pItem);
	void EnableButtons();

	// Dialog Data
	//{{AFX_DATA(CuDlgReplicationServerPageRaiseEvent)
	enum { IDD = IDD_REPSERVER_PAGE_RAISEEVENT };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
	CuListCtrl m_cListCtrl;


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgReplicationServerPageRaiseEvent)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	LPREPLICSERVERDATAMIN m_pSvrDta;
	CaReplicRaiseDbeventList m_EventsList;

	void Cleanup();
	// Generated message map functions
	//{{AFX_MSG(CuDlgReplicationServerPageRaiseEvent)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnButtonRaiseEvent();
	//}}AFX_MSG
	afx_msg void OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
	afx_msg long OnPropertiesChange(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RVREVENT_H__360829A9_6A76_11D1_A22D_00609725DDAF__INCLUDED_)
