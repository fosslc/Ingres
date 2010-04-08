/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : msgstat.h, Header file    (Modeless Dialog)                           **
**    Project  : Visual Manager.                                                       **
**    Author   : UK Sotheavut                                                          **
**    Purpose  : Statistic pane of IVM.                                                **
**
** History:
**
** 08-Jul-1999 (uk$so01)
**    created
** 01-Mar-2000 (uk$so01)
**    SIR #100635, Add the Functionality of Find operation.
**
**/

#if !defined(MESSAGE_STATISTIC)
#define MESSAGE_STATISTIC

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "lmsgstat.h"

class CuDlgMessageStatistic : public CDialog
{
public:
	CuDlgMessageStatistic(CWnd* pParent = NULL); 
	void OnOK (){return;}
	void OnCancel(){return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgMessageStatistic)
	enum { IDD = IDD_MESSAGE_STATISTIC };
	//}}AFX_DATA
	CuListCtrlMessageStatistic m_cListStatItem;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgMessageStatistic)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	CImageList m_ImageList;
	SORTPARAMS m_sort;
	void CleanStatisticItem();
	void DrawStatistic (CTypedPtrList<CObList, CaMessageStatisticItemData*>& listMsg);
	void ShowMessageDescriptionFrame(int nSel);

	// Generated message map functions
	//{{AFX_MSG(CuDlgMessageStatistic)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnColumnclickList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData    (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnNewEvents (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnFindAccepted (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnFind (WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(MESSAGE_STATISTIC)
