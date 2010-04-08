/******************************************************************************
**
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**	01-Sept-2004 (kodse01)
**		Changed #include "..\HDR\DBASET.H" to #include "DBASET.H"
******************************************************************************/
#if !defined(AFX_MODIFYSIZE_H__A3857F66_697C_11D5_B618_00C04F1790C3__INCLUDED_)
#define AFX_MODIFYSIZE_H__A3857F66_697C_11D5_B618_00C04F1790C3__INCLUDED_

#include "DBASET.H"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ModifySize.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CxDlgModifyPageSize dialog

class CxDlgModifyPageSize : public CDialog
{
// Construction
public:
	//DWORD m_PageSize[6];
	CxDlgModifyPageSize(CWnd* pParent = NULL);   // standard constructor
	void EnableDisableOKButton();
	int FillPageSize();
	LPTABLEPARAMS m_lpTblParams;
//	DWORD m_pPageSize={2048,4096,8192,16384,32768,65536};
// Dialog Data
	//{{AFX_DATA(CxDlgModifyPageSize)
	enum { IDD = IDD_MODIFY_PAGE_SIZE };
	CButton	m_ctrlOKButton;
	CStatic	m_ChangeDBMS_PageSize;
	CListBox	m_ctrlLstPageSize;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgModifyPageSize)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CxDlgModifyPageSize)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeListPageSize();
	afx_msg void OnKillfocusListPageSize();
	afx_msg void OnSetfocusListPageSize();
	afx_msg void OnSelcancelListPageSize();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MODIFYSIZE_H__A3857F66_697C_11D5_B618_00C04F1790C3__INCLUDED_)
