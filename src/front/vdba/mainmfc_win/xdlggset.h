/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : xdlggset.h, Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : INGRES II / VDBA                                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Dialog for General Display Settings                  .                //
****************************************************************************************/
#if !defined(AFX_XDLGGSET_H__896F7B31_8601_11D2_A2B0_00609725DDAF__INCLUDED_)
#define AFX_XDLGGSET_H__896F7B31_8601_11D2_A2B0_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class CxDlgGeneralDisplaySetting : public CDialog
{
public:
	CxDlgGeneralDisplaySetting(CWnd* pParent = NULL); 

	// Dialog Data
	//{{AFX_DATA(CxDlgGeneralDisplaySetting)
	enum { IDD = IDD_XGENERAL_DISPLAY_SETTING };
	CEdit	m_cEditDelayValue;
	CSpinButtonCtrl	m_cSpinDelayValue;
	BOOL	m_bUseGrid;
	CString	m_strDelayValue;
	BOOL	m_bMaximizeDocs;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgGeneralDisplaySetting)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CxDlgGeneralDisplaySetting)
	virtual void OnOK();
	afx_msg void OnKillfocusEditDelayValue();
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XDLGGSET_H__896F7B31_8601_11D2_A2B0_00609725DDAF__INCLUDED_)
