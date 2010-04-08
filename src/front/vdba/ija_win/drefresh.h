/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : drefresh.h, Header File
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Purpose  :  Popup Dialog proposing the option of the Refresh data
**
** History:
**
** 23-Dec-1999 (uk$so01)
**    Created
** 21-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management.
**
**/

#if !defined(AFX_DREFRESH_H__2E67EEF2_B940_11D3_A326_00C04F1F754A__INCLUDED_)
#define AFX_DREFRESH_H__2E67EEF2_B940_11D3_A326_00C04F1F754A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CxDlgRefresh : public CDialog
{
public:
	CxDlgRefresh(CWnd* pParent = NULL);

	// Dialog Data
	//{{AFX_DATA(CxDlgRefresh)
	enum { IDD = IDD_XREFRESH_MODE };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
	int m_nRadioChecked;


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgRefresh)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CxDlgRefresh)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DREFRESH_H__2E67EEF2_B940_11D3_A326_00C04F1F754A__INCLUDED_)
