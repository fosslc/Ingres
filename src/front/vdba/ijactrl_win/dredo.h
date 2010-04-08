/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dredo.h , Header File
**    Project  : Ingres Journal Analyser
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Implementation of Redo Dialog Box
**
** History:
**
** 29-Dec-1999 (uk$so01)
**    created
** 14-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management and put string into the resource files
** 05-Sep-2003 (uk$so01)
**    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
**/

#if !defined(AFX_DREDO_H__554E1D52_BDDE_11D3_A328_00C04F1F754A__INCLUDED_)
#define AFX_DREDO_H__554E1D52_BDDE_11D3_A328_00C04F1F754A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "logadata.h"
#include "rcrdtool.h"

class CxDlgRedo : public CDialog
{
public:
	CxDlgRedo(CWnd* pParent = NULL);

	void SetQueryTransactionInfo (CaQueryTransactionInfo* pInfo) {m_pQueryTransactionInfo = pInfo;}
	CTypedPtrList < CObList, CaBaseTransactionItemData* >& GetListTransaction(){return m_listTransaction;}
	void SetInitialSelection (UINT* arrayItem, int nArraySize)
	{
		m_pArrayItem = arrayItem;
		m_nArraySize = nArraySize;
	}

	// Dialog Data
	//{{AFX_DATA(CxDlgRedo)
	enum { IDD = IDD_XREDO };
	CButton	m_cButtonInitialOrder;
	CButton	m_cButtonInitialSelection;
	CTabCtrl	m_cTab1;
	CButton	m_cButtonDown;
	CButton	m_cButtonUp;
	CComboBox	m_cComboDatabase;
	CComboBox	m_cComboNode;
	BOOL	m_bNoRule;
	CString	m_strNode;
	CString	m_strDatabase;
	BOOL	m_bImpersonateUser;
	BOOL	m_bErrorOnAffectedRowCount;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgRedo)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void FillComboNode();
	void FillComboDatabase(LPCTSTR lpszNode);
	void EnableButtons();
	BOOL PrepareParam (CaRedoParam& param);
	BOOL IncludesCommitBetweenTransactions(CaRedoParam& param);


	CuListCtrl m_cListCtrl;
	CImageList m_ImageList;
	CTypedPtrList < CObList, CaBaseTransactionItemData* > m_listTransaction;
	CaQueryTransactionInfo* m_pQueryTransactionInfo;
	UINT* m_pArrayItem;
	int   m_nArraySize;
	CuDlgListHeader* m_pDlgListRules;
	CuDlgListHeader* m_pDlgListIntegrities;
	CuDlgListHeader* m_pDlgListConstraints;
	CuDlgListHeader* m_pCurrentPage;
	CfMiniFrame* m_pPropFrame;
	CToolTipCtrl m_toolTip;

	//
	// Maintaint the list of rules and constraints to be destroyed later:
	CTypedPtrList < CObList, CaDBObject* > m_listRule;
	CTypedPtrList < CObList, CaDBObject* > m_listConstraint;
	CTypedPtrList < CObList, CaDBObject* > m_listIntegrity;

	// Generated message map functions
	//{{AFX_MSG(CxDlgRedo)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeComboNode();
	afx_msg void OnButtonUp();
	afx_msg void OnButtonDown();
	afx_msg void OnButtonInitialSelection();
	afx_msg void OnButtonInitialOrder();
	afx_msg void OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonRedoNow();
	afx_msg void OnButtonGenerateScript();
	afx_msg void OnRadioWholeTransaction();
	afx_msg void OnButtonRedoNowScript();
	afx_msg void OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	afx_msg long OnDisplayProperty (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DREDO_H__554E1D52_BDDE_11D3_A328_00C04F1F754A__INCLUDED_)
