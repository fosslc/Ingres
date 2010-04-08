/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ijatable.h , Header File 
**    Project  : Ingres Journal Analyser
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Modeless Dialog represents the Table Pane
**
** History:
**
** 22-Dec-1999 (uk$so01)
**    Created
** 05-Sep-2003 (uk$so01)
**    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
** 02-Dec-2003 (schph01)
**    SIR #111389 Implement new button for displayed the checkpoint list.
**/

#if !defined(AFX_IJATABLE_H__98E91762_B3BE_11D3_A323_00C04F1F754A__INCLUDED_)
#define AFX_IJATABLE_H__98E91762_B3BE_11D3_A323_00C04F1F754A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "logadata.h"

class CuDlgIjaTable : public CDialog
{
public:
	CuDlgIjaTable(CWnd* pParent = NULL);
	void OnOK() {return;}
	void OnCancel() {return;}

	long AddUser(LPCTSTR strUser);
	void CleanUser();
	void RefreshPaneTable(LPCTSTR lpszNode, LPCTSTR lpszDatabase, LPCTSTR lpszDatabaseOwner, LPCTSTR lpszTable, LPCTSTR lpszTableOwner);
	void EnableRecoverRedo(BOOL bEnable){m_bEnableRecoverRedo = bEnable;}

	// Dialog Data
	//{{AFX_DATA(CuDlgIjaTable)
	enum { IDD = IDD_TABLE };
	CButton	m_ButtonCheckpointList;
	CButton	m_cCheckAfterCheckPoint;
	CEdit	m_cEditAfterCheckPoint;
	CDateTimeCtrl	m_cDatePickerBefore;
	CDateTimeCtrl	m_cDatePickerAfter;
	CButton	m_cButtonSelectAll;
	CButton	m_cButtonRedo;
	CButton	m_cButtonRecover;
	CComboBox	m_cComboUser;
	BOOL	m_bInconsistent;
	BOOL	m_bAfterCheckPoint;
	BOOL	m_bWait;
	CString	m_strUser;
	CString	m_strCheckPointNo;
	BOOL	m_bAbortedTransaction;
	BOOL	m_bCommittedTransaction;
	CTime	m_timeAfter;
	CTime	m_timeBefore;
	//}}AFX_DATA
	CString m_strAfter;
	CString m_strBefore;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIjaTable)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
private:
	CTypedPtrList < CObList, CaTableTransactionItemData* > m_listMainTrans;
	CString m_strInitialUser;
	void DisplayMainTransaction();

protected:
	CaQueryTransactionInfo m_queryTransactionInfo;
	CuListCtrl m_cListCtrl;
	CImageList m_ImageList;
	CImageList m_ImageListOrder;

	CRect m_rcInitialListCtrl;
	CRect m_rcInitialButtonRecover;
	CRect m_rcInitialButtonRedo;
	CSize m_sizeMargin;
	BOOL  m_bColumnHeaderOK;
	int   m_nColumnHeaderCount;
	SORTPARAMS m_sortparam;
	BOOL m_bViewButtonClicked;
	CTypedPtrList<CObList, CaColumn*> m_listColumn;
	BOOL m_bEnableRecoverRedo;

	void EnableButtons();
	void EnableCheckPoint();
	void ResizeControls();
	void CleanListCtrl();
	void GetSelectedTransactions (CTypedPtrList < CObList, CaBaseTransactionItemData* >& ltr);
	void GetSelectedItems (UINT*& pArraySelectedItem, int& nSelectedCount);
	void ChangeSortIndicator(CListCtrl* pListCtrl, SORTPARAMS* pSp);
	void OnInputParamChange(int idct);

	// Generated message map functions
	//{{AFX_MSG(CuDlgIjaTable)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnButtonView();
	afx_msg void OnButtonRecover();
	afx_msg void OnButtonRedo();
	afx_msg void OnDestroy();
	afx_msg void OnButtonSelectAll();
	afx_msg void OnColumnclickList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonMoveLog2Journals();
	afx_msg void OnCheckAfterCheckPoint();
	afx_msg void OnCheckAbortedTransaction();
	afx_msg void OnCheckCommittedTransaction();
	afx_msg void OnSelchangeComboUser();
	afx_msg void OnDatetimechangeBefore(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDatetimechangeAfter(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnChangeEditAfterCheckPoint();
	afx_msg void OnCheckWait();
	afx_msg void OnKillfocusEditAfterCheckPoint();
	afx_msg void OnButtonCheckpointList();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IJATABLE_H__98E91762_B3BE_11D3_A323_00C04F1F754A__INCLUDED_)
