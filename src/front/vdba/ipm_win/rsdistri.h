/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : rsdistri.h, header file  
**    Project  : CA-OpenIngres/Replicator.
**    Author   : UK Sotheavut
**    Purpose  : Page of a static item Replication.  (Distrib)
**
** History:
**
** xx-Dec-1997 (uk$so01)
**    Created
**
*/

#if !defined(AFX_RSDISTRIB_H__360829A3_6A76_11D1_A22D_00609725DDAF__INCLUDED_)
#define AFX_RSDISTRIB_H__360829A3_6A76_11D1_A22D_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CuDlgReplicationStaticPageDistrib : public CDialog
{
public:
	CuDlgReplicationStaticPageDistrib(CWnd* pParent = NULL); 
	void OnOK(){return;}
	void OnCancel(){return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgReplicationStaticPageDistrib)
	enum { IDD = IDD_REPSTATIC_PAGE_DISTRIB };
	CEdit	m_cEdit1;
	CString	m_strEdit1;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgReplicationStaticPageDistrib)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CuDlgReplicationStaticPageDistrib)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RSDISTRIB_H__360829A3_6A76_11D1_A22D_00609725DDAF__INCLUDED_)
