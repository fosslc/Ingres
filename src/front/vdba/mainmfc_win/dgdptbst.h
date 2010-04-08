/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dgdptbst.h, Header file    (Modeless Dialog)
**    Project  : CA-OpenIngres Visual DBA.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Table's statistic pane of DOM's right pane.
** 
** History after 02-May-2001:
** xx-Mar-1998 (uk$so01):
**    Created.
** 02-May-2001 (uk$so01)
**    SIR #102678
**    Support the composite histogram of base table and secondary index.
**
*/

#if !defined(AFX_DGDPTBST_H__0540BB01_C4C6_11D1_A271_00609725DDAF__INCLUDED_)
#define AFX_DGDPTBST_H__0540BB01_C4C6_11D1_A271_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// dgdptbst.h : header file
//
#include "lstbstat.h"
#include "domseri.h"
/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropTableStatistic dialog

class CuDlgDomPropTableStatistic : public CDialog
{
public:
	CuDlgDomPropTableStatistic(CWnd* pParent = NULL); 
	void OnOK (){return;}
	void OnCancel(){return;}
	BOOL QueryStatColumns();
	void DrawStatistic();

	// Dialog Data
	//{{AFX_DATA(CuDlgDomPropTableStatistic)
	enum { IDD = IDD_DOMPROP_TABLE_STATISTIC };
	CButton	m_cButtonRemove;
	CButton	m_cButtonGenerate;
	BOOL	m_bUniqueFlag;
	BOOL	m_bCompleteFlag;
	CString	m_strUniqueValues;
	CString	m_strRepetitionFactors;
	//}}AFX_DATA
	CuListCtrl m_cListColumn;
	CuListCtrlTableStatistic m_cListStatItem;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropTableStatistic)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	BOOL m_bCleaned;    // Use to Assert if data has been cleaned on destroy window
	BOOL m_bLoad;       // Use to prevent the calling OnItemchangedList1 when Load
	BOOL m_bExecuted;   // Use to check if we have already updated when selchange in the right pane
	CImageList m_ImageList;
	CImageList m_ImageCheck;
	CaTableStatistic m_statisticData;
	int m_nOT;          // The current object type (OT_TABLE or OT_INDEX) being used. Initialize to -1;

	void ResetDisplay();
	void CleanStatisticItem();
	void CleanListColumns();
	void UpdateDisplayList1();
	void EnableButtons();
	int Find (LPCTSTR lpszColumn);
	void InitializeStatisticHeader(int nOT);

	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropTableStatistic)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnOutofmemoryList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnOutofmemoryList2(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnGenerate();
	afx_msg void OnRemove();
	afx_msg void OnColumnclickList1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData    (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad          (WPARAM wParam, LPARAM lParam);
	//
	// OnUpdateQuery only changes the hint to indicate the OnUpdateData that
	// the selection is occured in the right pane:
	afx_msg LONG OnUpdateQuery   (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnChangeProperty(WPARAM wParam, LPARAM lParam);

	afx_msg LONG OnQueryType  (WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DGDPTBST_H__0540BB01_C4C6_11D1_A271_00609725DDAF__INCLUDED_)
