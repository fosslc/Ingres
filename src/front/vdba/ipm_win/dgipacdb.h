/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   :dgipacdb.h, Header file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control: The Active Database page
**
** History:
**
** xx-Feb-1997 (uk$so01)
**    Created
*/

#ifndef DGIPACDB_HEADER
#define DGIPACDB_HEADER
#include "calsctrl.h"

class CuDlgIpmPageActiveDatabases : public CDialog
{
public:
	CuDlgIpmPageActiveDatabases(CWnd* pParent = NULL);
	void AddActiveDB (
		LPCTSTR strDB,
		LPCTSTR strStatus, 
		LPCTSTR strTXCnt,
		LPCTSTR strBegin,
		LPCTSTR strEnd,
		LPCTSTR strRead,
		LPCTSTR strWrite,
		void* pStruct);
	void OnCancel() {return;}
	void OnOK() {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgIpmPageActiveDatabases)
	enum { IDD = IDD_IPMPAGE_ACTIVEDB };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
	CuListCtrl m_cListCtrl;
	CImageList m_ImageList;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmPageActiveDatabases)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmPageActiveDatabases)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDblclkMfcList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeleteitemMfcList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad  (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData (WPARAM wParam, LPARAM lParam);
	afx_msg long OnPropertiesChange(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};
#endif
