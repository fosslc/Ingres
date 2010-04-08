/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/


/*
**  Project  : Visual DBA
**  Source   : rsactitb.h, header file
**  Author   : UK Sotheavut (uk$so01)
**  Purpose  : Page of a static item Replication.(Sub page of Activity: Per Table)
**             Outgoing, Incoming, Total 
** History:
**
** xx-Dec-1997 (uk$so01)
**    Created.
*/

#if !defined(AFX_RSACTITB_H__360829AC_6A76_11D1_A22D_00609725DDAF__INCLUDED_)
#define AFX_RSACTITB_H__360829AC_6A76_11D1_A22D_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "calsctrl.h"


class CuDlgReplicationStaticPageActivityPerTable : public CDialog
{
public:
	CuDlgReplicationStaticPageActivityPerTable(CWnd* pParent = NULL);
	CuDlgReplicationStaticPageActivityPerTable(int nMode, CWnd* pParent = NULL);  
	void OnOK(){return;}
	void OnCancel(){return;}
	void GetAllColumnWidth (CArray < int, int >& cxArray);
	void SetAllColumnWidth (CArray < int, int >& cxArray);
	void SetScrollPosListCtrl (CSize& size);
	CSize GetScrollPosListCtrl();

	// Dialog Data
	//{{AFX_DATA(CuDlgReplicationStaticPageActivityPerTable)
	enum { IDD = IDD_REPSTATIC_PAGE_ACTIVITY_TAB_TABLE };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
	CuListCtrl m_cListCtrl;
	enum {MODE_OUTGOING = 1, MODE_INCOMING, MODE_TOTAL};


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgReplicationStaticPageActivityPerTable)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	UINT m_nMode; // (1: Outgoing, 2: Incoming, 3: Total)

	// Generated message map functions
	//{{AFX_MSG(CuDlgReplicationStaticPageActivityPerTable)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RSACTITB_H__360829AC_6A76_11D1_A22D_00609725DDAF__INCLUDED_)
