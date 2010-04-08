/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : dginstt1.h , Header File
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Status Page for Installation branch  (Tab 1)
**
** History:
**
** 09-Feb-1999 (uk$so01)
**    created
** 01-Mar-2000 (uk$so01)
**    SIR #100635, Add the Functionality of Find operation.
** 06-Mar-2003 (uk$so01)
**    SIR #109773, Add property frame for displaying the description 
**    of ingres messages.
**/

#if !defined(AFX_DGINSTT1_H__DAD0E9E2_BF6A_11D2_A2CF_00609725DDAF__INCLUDED_)
#define AFX_DGINSTT1_H__DAD0E9E2_BF6A_11D2_A2CF_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "calsctrl.h"

class CaLoggedEvent;
class CuDlgStatusInstallationTab1 : public CDialog
{
public:
	CuDlgStatusInstallationTab1(CWnd* pParent = NULL);
	void OnOK() {return;}
	void OnCancel() {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgStatusInstallationTab1)
	enum { IDD = IDD_STATUS_INSTALLATION_TAB1 };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgStatusInstallationTab1)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CuListCtrl m_cListCtrl;
	CImageList m_ImageList;

	void AddEvent (CaLoggedEvent* pEvent);
	void ShowMessageDescriptionFrame(int nSel);
	// Generated message map functions
	//{{AFX_MSG(CuDlgStatusInstallationTab1)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkList1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnFind (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DGINSTT1_H__DAD0E9E2_BF6A_11D2_A2CF_00609725DDAF__INCLUDED_)
