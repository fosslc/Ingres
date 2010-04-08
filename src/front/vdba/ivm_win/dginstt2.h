/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : dginstt2.h , Header File
**
**  Project  : Ingres II / Visual Manager
**
**  Author   : Sotheavut UK (uk$so01)
**
**  Purpose  : Status Page for Installation branch  (Tab 2)
**
**  History:
** 09-Feb-1999 (uk$so01)
**    Created.
** 11-Feb-2000 (somsa01)
**    Removed m_fileStatus, created global m_LSSfileStatus.
** 01-Mar-2000 (uk$so01)
**    SIR #100635, Add the Functionality of Find operation.
** 20-Jun-2001 (noifr01)
**    (bug 105065) restored m_fileStatus, and added m_csCurOutput
**    for keeping in memory the previous contents that had been
**    read in the file
**
**/

#if !defined(AFX_DGINSTT2_H__DAD0E9E3_BF6A_11D2_A2CF_00609725DDAF__INCLUDED_)
#define AFX_DGINSTT2_H__DAD0E9E3_BF6A_11D2_A2CF_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// dginstt2.h : header file
//

class CuDlgStatusInstallationTab2 : public CDialog
{
public:
	CuDlgStatusInstallationTab2(CWnd* pParent = NULL);
	void OnOK() {return;}
	void OnCancel() {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgStatusInstallationTab2)
	enum { IDD = IDD_STATUS_INSTALLATION_TAB2 };
	CEdit	m_cEdit1;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgStatusInstallationTab2)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CFileStatus m_fileStatus;
	CString m_csCurOutput;
	// Generated message map functions
	//{{AFX_MSG(CuDlgStatusInstallationTab2)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnFind (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DGINSTT2_H__DAD0E9E3_BF6A_11D2_A2CF_00609725DDAF__INCLUDED_)
