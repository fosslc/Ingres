/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : mtabdlg.h, header file                                                //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut,                                                         //
//                                                                                     //
//    Purpose  : Modeless Dialog holds 3 Tabs:                                         //
//               (History of Changes, Configure, Preferences)                          //
//               See the CONCEPT.DOC for more detail                                   //
****************************************************************************************/

#ifndef MTABDLG_HEADER
#define MTABDLG_HEADER
#include "histdlg.h"
#include "prefdlg.h"
#include "cconfdlg.h"


class CMainTabDlg : public CDialog
{
public:
	enum {PANE_CONFIG, PANE_HISTORY, PANE_PREFERENCES};
	CMainTabDlg(CWnd* pParent = NULL);   
	void OnOK() {return;};
	void OnCancel() {return;};
	void SetHeader (LPCTSTR lpszInstall, LPCTSTR lpszHost, LPCTSTR lpszIISyst);
	UINT GetHelpID();

	// Dialog Data
	//{{AFX_DATA(CMainTabDlg)
	enum { IDD = IDD_MAINTAB_DLG };
	CString	m_strHeader;
	//}}AFX_DATA


	//
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainTabDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	CDialog * m_pCurDlg;

	CConfigDlg* m_pConfigDlg;
	CHistDlg*   m_pHistDlg;
	CPrefDlg*   m_pPrefDlg;
	
	CSize       m_dlgPadding;
	CSize       m_buttonPadding;

	// Generated message map functions
	//{{AFX_MSG(CMainTabDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	afx_msg void OnButtonOK();
	afx_msg void OnButtonCancel();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif //#ifndef MTABDLG_HEADER
