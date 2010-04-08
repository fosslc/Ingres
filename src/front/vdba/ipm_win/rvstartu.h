/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : rvstartu.h, header file
**    Project  : CA-OpenIngres/Replicator.
**    Author   : UK Sotheavut
**    Purpose  : Page of a Replication Server Item  (Startup Setting)
**
** History:
**
** xx-Dec-1997 (uk$so01)
**    Created
*/

#if !defined(AFX_RVSTARTU_H__360829A8_6A76_11D1_A22D_00609725DDAF__INCLUDED_)
#define AFX_RVSTARTU_H__360829A8_6A76_11D1_A22D_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "editlsgn.h"
#include "repitem.h" 


class CuDlgReplicationServerPageStartupSetting : public CDialog
{
public:
	CuDlgReplicationServerPageStartupSetting(CWnd* pParent = NULL);   // standard constructor
	void OnOK(){return;}
	void OnCancel(){return;}
	void AddItem(CaReplicationItem* pItem);
	void EnableButtons();
	CString m_csDBAOwner;
	// Dialog Data
	//{{AFX_DATA(CuDlgReplicationServerPageStartupSetting)
	enum { IDD = IDD_REPSERVER_PAGE_STARTSETTING };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
	CuEditableListCtrlStartupSetting m_cListCtrl;
	CImageList m_ImageList;
	CImageList m_ImageCheck;
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgReplicationServerPageStartupSetting)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL
	
	//
	// Implementation
protected:
	//int m_nNodeHandle;
	LPREPLICSERVERDATAMIN m_pSvrDta;
	CaReplicationItemList m_FlagsList;

	void Cleanup();
	// Generated message map functions
	//{{AFX_MSG(CuDlgReplicationServerPageStartupSetting)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnButtonSave();
	afx_msg void OnButtonEditValue();
	afx_msg void OnButtonRestore();
	afx_msg void OnButtonRestoreAll();
	//}}AFX_MSG
	afx_msg void OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	afx_msg LONG OnUpdateData   (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad         (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData      (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnHideProperty (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLeavingPage  (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnDataChanged  (WPARAM wParam, LPARAM lParam);
	afx_msg long OnPropertiesChange(WPARAM wParam, LPARAM lParam);

DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RVSTARTU_H__360829A8_6A76_11D1_A22D_00609725DDAF__INCLUDED_)
