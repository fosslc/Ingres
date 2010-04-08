/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgipiccu.h, Header file
**    Project  : INGRES II/ Ice Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control: The Ice Cursor Page
**
** History:
**
** xx-Oct-1998 (uk$so01)
**    Created
*/

#ifndef DGIPICCU_HEADER
#define DGIPICCU_HEADER
#include "calsctrl.h"

class CuDlgIpmIcePageCursor : public CDialog
{
public:
	CuDlgIpmIcePageCursor(CWnd* pParent = NULL);  
	void OnCancel() {return;}
	void OnOK() {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgIpmIcePageCursor)
	enum { IDD = IDD_IPMICEPAGE_CURSOR };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
	CuListCtrl m_cListCtrl;
	CImageList m_ImageList;
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmIcePageCursor)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX); // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void AddItem (LPCTSTR lpszItem, ICE_CURSORDATAMIN* pData);
	void DisplayItems();
	void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmIcePageCursor)
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
