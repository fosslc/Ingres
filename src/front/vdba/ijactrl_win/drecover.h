/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : drecover.h , Header File
**    Project  : Ingres Journal Analyser
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Implementation of Recover Dialog Box
**
** History:
**
** 28-Dec-1999 (uk$so01)
**    created
** 14-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management and put string into the resource files
** 05-Sep-2003 (uk$so01)
**    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
**/

#if !defined(AFX_DRECOVER_H__51862792_B9DC_11D3_A326_00C04F1F754A__INCLUDED_)
#define AFX_DRECOVER_H__51862792_B9DC_11D3_A326_00C04F1F754A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "logadata.h"
#include "rcrdtool.h"

class CxDlgRecover : public CDialog
{
public:
	CxDlgRecover(CWnd* pParent = NULL);

	void SetQueryTransactionInfo (CaQueryTransactionInfo* pInfo) {m_pQueryTransactionInfo = pInfo;}
	CTypedPtrList < CObList, CaBaseTransactionItemData* >& GetListTransaction(){return m_listTransaction;}
	void SetInitialSelection (UINT* arrayItem, int nArraySize)
	{
		m_pArrayItem = arrayItem;
		m_nArraySize = nArraySize;
	}


	// Dialog Data
	//{{AFX_DATA(CxDlgRecover)
	enum { IDD = IDD_XRECOVER };
	CButton	m_cButtonInitialOrder;
	CButton	m_cButtonInitialSelection;
	CTabCtrl	m_cTab1;
	CButton	m_cButtonDown;
	CButton	m_cButtonUp;
	BOOL	m_bCheckScanJournal;
	BOOL	m_bNoRule;
	BOOL	m_bImpersonateUser;
	BOOL	m_bErrorOnAffectedRowCount;
	//}}AFX_DATA
	

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgRecover)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void EnableButtons();
	BOOL PrepareParam (CaRecoverParam& param);
	BOOL ScanToTheEnd(CaRecoverParam& param);
	BOOL IncludesCommitBetweenTransactions(CaRecoverParam& param);
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

	CString m_strActionDelete;
	CString m_strActionUpdate;
	CString m_strActionAlter;
	CString m_strActionDrop;
	CString m_strActionMore;

	// Generated message map functions
	//{{AFX_MSG(CxDlgRecover)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonInitialSelection();
	afx_msg void OnButtonInitialOrder();
	afx_msg void OnButtonUp();
	afx_msg void OnButtonDown();
	afx_msg void OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRadioWholeTransaction();
	afx_msg void OnButtonRecoverNow();
	afx_msg void OnButtonGenerateScript();
	afx_msg void OnButtonRecoverNowScript();
	afx_msg void OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	afx_msg long OnDisplayProperty (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DRECOVER_H__51862792_B9DC_11D3_A326_00C04F1F754A__INCLUDED_)
