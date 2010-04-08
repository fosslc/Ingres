/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : rsrevent.h, header file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of a static item Replication.  (Raise Event)
**
** History:
**
** xx-Dec-1997 (uk$so01)
**    Created
*/

#if !defined(AFX_RSREVENT_H__360829A5_6A76_11D1_A22D_00609725DDAF__INCLUDED_)
#define AFX_RSREVENT_H__360829A5_6A76_11D1_A22D_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "calsctrl.h"
#include "repevent.h"


class CuDlgReplicationStaticPageRaiseEvent : public CDialog
{
public:
	CuDlgReplicationStaticPageRaiseEvent(CWnd* pParent = NULL);   
	void OnOK(){return;}
	void OnCancel(){return;}
	void EnableButtons();
	void AddItem (CaReplicationRaiseDbevent* pItem);

	// Dialog Data
	//{{AFX_DATA(CuDlgReplicationStaticPageRaiseEvent)
	enum { IDD = IDD_REPSTATIC_PAGE_RAISEEVENT };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
	CuListCtrl m_cListCtrl;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgReplicationStaticPageRaiseEvent)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	LPRESOURCEDATAMIN m_pSvrDta;
	CaReplicRaiseDbeventList m_EventsList;

	void Cleanup();
	// Generated message map functions
	//{{AFX_MSG(CuDlgReplicationStaticPageRaiseEvent)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnButtonRaiseEventAllServer();
	afx_msg void OnItemchangedMfcList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
	afx_msg long OnPropertiesChange(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RSREVENT_H__360829A5_6A76_11D1_A22D_00609725DDAF__INCLUDED_)
