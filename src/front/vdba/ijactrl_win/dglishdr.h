/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : dglishdr.h, header file
**    Project  : IJA 
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Modeless dialog that contains the list control 
**               with header (generic)
**               The caller must initialize the following member
**               before creating the dialog:
**               + m_nHeaderCount: number of header in the list control:
**               + m_pArrayHeader: array of headers
**               + m_lpfnDisplay : the funtion that display the item in the list control.
**               + m_pWndMessageHandler: to which the list control will send message to.
**
** History :
**
** 16-May-2000 (uk$so01)
**    created
**
**/

#if !defined(AFX_DGLISHDR_H__218D74B4_2B06_11D4_A35F_00C04F1F754A__INCLUDED_)
#define AFX_DGLISHDR_H__218D74B4_2B06_11D4_A35F_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "calsctrl.h"

//
// l1: point to the item data of the list control:
// l2: additional info how to display the data
typedef void (CALLBACK* PFNLISTCTRLDISPLAY)(CListCtrl* pList, LPARAM l1, LPARAM l2);

class CuDlgListHeader : public CDialog
{
public:
	CuDlgListHeader(CWnd* pParent = NULL); 
	void OnOK() {return;}
	void OnCancel() {return;}
	void AddItem (LPARAM lItem);

	// Dialog Data
	//{{AFX_DATA(CuDlgListHeader)
	enum { IDD = IDD_LISTHEADER };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
	int m_nHeaderCount;
	LSCTRLHEADERPARAMS2* m_pArrayHeader;
	PFNLISTCTRLDISPLAY   m_lpfnDisplay;
	CWnd* m_pWndMessageHandler;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgListHeader)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CuListCtrl m_cListCtrl;
	CImageList m_cImageList;

	// Generated message map functions
	//{{AFX_MSG(CuDlgListHeader)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDblclkList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnClickList1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DGLISHDR_H__218D74B4_2B06_11D4_A35F_00C04F1F754A__INCLUDED_)
