/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgipltbl.h, Header file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control: The Locked Table page
**
** History:
**
** xx-Feb-1997 (uk$so01)
**    Created
*/

#ifndef DGIPLTBL_HEADER
#define DGIPLTBL_HEADER
#include "calsctrl.h"

class CuDlgIpmPageLockTables : public CDialog
{
public:
	CuDlgIpmPageLockTables(CWnd* pParent = NULL);  
	void AddLockTable(
		LPCTSTR strID,
		LPCTSTR strGR, 
		LPCTSTR strCV,
		LPCTSTR strTable);
	void OnCancel() {return;}
	void OnOK() {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgIpmPageLockTables)
	enum { IDD = IDD_IPMPAGE_LOCKEDTABLES };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
	CuListCtrl m_cListCtrl;
	CImageList m_ImageList;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmPageLockTables)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmPageLockTables)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
	afx_msg long OnPropertiesChange(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};
#endif
