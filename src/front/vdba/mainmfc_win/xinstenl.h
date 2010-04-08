/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : xinstenl.h, Header file                                               //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II/VDBA                                                        //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Enable/Disable of Install Security Auditing Level ( IISECURITY_STATE) //
****************************************************************************************/
#if !defined(AFX_XINSTENL_H__42CE3E21_722D_11D2_A2AA_00609725DDAF__INCLUDED_)
#define AFX_XINSTENL_H__42CE3E21_722D_11D2_A2AA_00609725DDAF__INCLUDED_
#include "calsctrl.h"
#include "domseri.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CxDlgInstallLevelSecurityEnableLevel : public CDialog
{
public:
	CxDlgInstallLevelSecurityEnableLevel(CWnd* pParent = NULL);

	// Dialog Data
	//{{AFX_DATA(CxDlgInstallLevelSecurityEnableLevel)
	enum { IDD = IDD_XENABLE_SECURITY_LEVEL };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgInstallLevelSecurityEnableLevel)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CuListCtrl m_cListCtrl;
	CImageList m_ImageList;
	CImageList m_ImageCheck;

	void AddItem (LPCTSTR lpszItem, CaAuditEnabledLevel* pData);
	void DisplayItems();
	// Generated message map functions
	//{{AFX_MSG(CxDlgInstallLevelSecurityEnableLevel)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XINSTENL_H__42CE3E21_722D_11D2_A2AA_00609725DDAF__INCLUDED_)
