/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : netrgdlg.h : header file
**    Project  : Configuration Manager 
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Modeless Dialog, Page (Registry protocol) of Net
**
** History:
**
** 20-jun-2003 (uk$so01)
**    created.
**    SIR #110389 / INGDBA210
**    Add functionalities for configuring the GCF Security and
**    the security mechanisms.
*/

#if !defined(AFX_NETRGDLG_H__17EC0063_94D9_11D7_8839_00C04F1F754A__INCLUDED_)
#define AFX_NETRGDLG_H__17EC0063_94D9_11D7_8839_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include  "editlnet.h"  // custom list control for net

class CuDlgNetRegistry : public CDialog
{
public:
	CuDlgNetRegistry(CWnd* pParent = NULL); 
	void OnOK() {return;}
	void OnCancel() {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgNetRegistry)
	enum { IDD = IDD_NET_PAGE_REGISTRY };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgNetRegistry)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CuEditableListCtrlNet m_cListCtrl;
	CImageList  m_ImageList;
	CImageList  m_ImageListCheck;

	void ResetList();
	BOOL AddProtocol(void* lpProtocol);

	// Generated message map functions
	//{{AFX_MSG(CuDlgNetRegistry)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnValidateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnEditListenAddress (UINT wParam, LONG lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NETRGDLG_H__17EC0063_94D9_11D7_8839_00C04F1F754A__INCLUDED_)
