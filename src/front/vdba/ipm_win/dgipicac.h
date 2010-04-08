/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgipicac.h, Header file.
**    Project  : INGRES II/ Ice Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control: The Ice Active User Page 
**
** History:
**
** xx-Oct-1998 (uk$so01)
**    Created
*/

#ifndef DGIPICAC_HEADER
#define DGIPICAC_HEADER
#include "calsctrl.h"


class CuDlgIpmIcePageActiveUser : public CDialog
{
public:
	CuDlgIpmIcePageActiveUser(CWnd* pParent = NULL);  
	void OnCancel() {return;}
	void OnOK() {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgIpmIcePageActiveUser)
	enum { IDD = IDD_IPMICEPAGE_ACTIVEUSER };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
	CuListCtrl m_cListCtrl;
	CImageList m_ImageList;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmIcePageActiveUser)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX); // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void AddItem (LPCTSTR lpszItem, ICE_ACTIVE_USERDATAMIN* pData);
	void DisplayItems();
	void ResetDisplay();

	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmIcePageActiveUser)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDblclkMfcList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeleteitemMfcList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
	afx_msg long OnPropertiesChange(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};
#endif
