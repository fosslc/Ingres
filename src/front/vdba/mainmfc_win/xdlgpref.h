/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xdlgpref.h, Header File 
**    Project  : INGRES II / VDBA 
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Dialog for Preference Settings 
**
** History:
** 
** xx-Dec-1998 (uk$so01)
**    Created.
** 19-Feb-2002 (uk$so01)
**    SIR #106648, Integrate SQL Test ActiveX Control
** 05-Aug-2003 (uk$so01)
**    SIR #106648, Remove printer setting
*/

#if !defined(AFX_XDLGPREF_H__896F7B33_8601_11D2_A2B0_00609725DDAF__INCLUDED_)
#define AFX_XDLGPREF_H__896F7B33_8601_11D2_A2B0_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CxDlgPreferences : public CDialog
{
public:
	CxDlgPreferences(CWnd* pParent = NULL);
	void OnOK();
	// Dialog Data
	//{{AFX_DATA(CxDlgPreferences)
	enum { IDD = IDD_XPREFERENCES };
	CListCtrl	m_cListCtrl;
	CString	m_strDescription;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgPreferences)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	CImageList m_ImageListBig;
	CImageList m_ImageListSmall;
	int  m_nViewMode;
	int  m_nSortColumn;
	BOOL m_bAsc;
	BOOL m_bPropertyChange;

	void InitializeItems();
	void ChangeViewMode (int nMode);
	void OnSort(int nSubItem);
	void OnExecuteItem();

	// Generated message map functions
	//{{AFX_MSG(CxDlgPreferences)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnDblclkList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnclickList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnClose();
	//}}AFX_MSG
	afx_msg void OnSettingSessions();
	afx_msg void OnSettingDOM();
	afx_msg void OnSettingSQLTest();
	afx_msg void OnSettingRefresh();
	afx_msg void OnSettingNodes();
	afx_msg void OnSettingMonitor();
	afx_msg void OnSettingGeneralDisplay();
	afx_msg void OnSettingClose();

	afx_msg void OnDisplayBigIcon();
	afx_msg void OnDisplaySmallIcon();
	afx_msg void OnDisplayReport();
	afx_msg void OnDisplayList();

	DECLARE_MESSAGE_MAP()

	afx_msg void OnPropertyChange();
	DECLARE_EVENTSINK_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XDLGPREF_H__896F7B33_8601_11D2_A2B0_00609725DDAF__INCLUDED_)
