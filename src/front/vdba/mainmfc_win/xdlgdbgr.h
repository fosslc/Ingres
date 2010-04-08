/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : xdlgdbgr.h, Header file                                               //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II/VDBA                                                        //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Create Rule Dialog Box                                                //
****************************************************************************************/
#if !defined(AFX_XDLGDBGR_H__42CE3E24_722D_11D2_A2AA_00609725DDAF__INCLUDED_)
#define AFX_XDLGDBGR_H__42CE3E24_722D_11D2_A2AA_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "edlsdbgr.h"
#include "uchklbox.h"

/////////////////////////////////////////////////////////////////////////////
// CxDlgGrantDatabase dialog

class CxDlgGrantDatabase : public CDialog
{
public:
	CxDlgGrantDatabase(CWnd* pParent = NULL);
	void SetParam (LPVOID pParam){m_pParam = pParam;}

	// Dialog Data
	//{{AFX_DATA(CxDlgGrantDatabase)
	enum { IDD = IDD_XGRANTDB };
	CButton	m_cButtonOK;
	CStatic	m_cStatic1;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgGrantDatabase)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	int m_nNodeHandle;
	LPVOID m_pParam;
	CuCheckListBox m_cCheckListBoxGrantee;
	CuCheckListBox m_cCheckListBoxDatabase;
	CuEditableListCtrlGrantDatabase m_cListPrivilege;
	CImageList m_ImageCheck;

	void InitializeFillPrivileges ();
	void Cleanup();
	BOOL InitializeDatabase();
	BOOL InitializeGrantee(int nGranteeType = -1/*public*/);
	void CheckPrevileges  (int nPrivNoPriv);
	BOOL ValidateInputData(CString& strMsg);
	void EnableButtonOK();

	// Generated message map functions
	//{{AFX_MSG(CxDlgGrantDatabase)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	virtual void OnOK();
	afx_msg void OnRadioUser();
	afx_msg void OnRadioRole();
	afx_msg void OnRadioGroup();
	afx_msg void OnRadioPublic();
	//}}AFX_MSG
	afx_msg LONG OnPrivilegeChange (UINT wParam, LONG lParam);
	afx_msg void OnCheckChangeGrantee();
	afx_msg void OnCheckChangeDatabase();
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XDLGDBGR_H__42CE3E24_722D_11D2_A2AA_00609725DDAF__INCLUDED_)
