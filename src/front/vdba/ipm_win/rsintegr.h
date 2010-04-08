/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : rsintegr.h, header file
**    Project  : INGRES II/ Monitoring
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of a static item Replication.  (Integrity)
**
** History:
**
** xx-Dec-1997 (uk$so01)
**    Created
*/


#if !defined(AFX_RSINTEGR_H__360829A6_6A76_11D1_A22D_00609725DDAF__INCLUDED_)
#define AFX_RSINTEGR_H__360829A6_6A76_11D1_A22D_00609725DDAF__INCLUDED_
#include "repinteg.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class CuDlgReplicationStaticPageIntegrity : public CDialog
{
public:
	void GetOrderColumns(CString *Tempo);
	bool m_bColumnsOrderModifyByUser;
	void EnableButton();
	CuDlgReplicationStaticPageIntegrity(CWnd* pParent = NULL);   
	void OnOK(){return;}
	void OnCancel(){return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgReplicationStaticPageIntegrity)
	enum { IDD = IDD_REPSTATIC_PAGE_INTEGRITY };
	CButton	m_cButtonUp;
	CButton	m_cButtonDown;
	CButton	m_cButtonGo;
	CEdit	m_cEdit4;
	CComboBox	m_cComboDatabase2;
	CComboBox	m_cComboDatabase1;
	CComboBox	m_cComboCdds;
	CComboBox	m_cComboTable;
	CString	m_strTimeBegin;
	CString	m_strTimeEnd;
	CString	m_strResult;
	CString	m_strComboTable;
	CString	m_strComboCdds;
	CString	m_strComboDatabase1;
	CString	m_strComboDatabase2;
	//}}AFX_DATA
	CaReplicCommonList            m_ColList;
	CaReplicIntegrityRegTableList m_RegTableList;
	CListBox m_cListColumnOrder;


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgReplicationStaticPageIntegrity)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CuDlgReplicationStaticPageIntegrity)
	afx_msg void OnButtonGo();
	afx_msg void OnButtonUp();
	afx_msg void OnButtonDown();
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSelchangeRegTblName();
	afx_msg void OnSelchangeComboCDDS();
	afx_msg void OnSelchangeComboDB1();
	afx_msg void OnSelchangeComboDb2();
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RSINTEGR_H__360829A6_6A76_11D1_A22D_00609725DDAF__INCLUDED_)
