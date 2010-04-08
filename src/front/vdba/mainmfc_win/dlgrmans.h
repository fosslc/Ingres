/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : dlgrmans.h, Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II/Vdba.                                                       //
//    Author   : UK Sotheavut                       .                                  //
//                                                                                     //
//    Purpose  : Popup Dialog Box for the RMCMD Send Answer                            //
****************************************************************************************/
#if !defined(AFX_DLGRMANS_H__6DD3BF63_7B13_11D2_A2AF_00609725DDAF__INCLUDED_)
#define AFX_DLGRMANS_H__6DD3BF63_7B13_11D2_A2AF_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CxDlgRemoteCommandInterface : public CDialog
{
public:
	CxDlgRemoteCommandInterface(CWnd* pParent = NULL);

	// Dialog Data
	//{{AFX_DATA(CxDlgRemoteCommandInterface)
	enum { IDD = IDD_XRMCMD_SEND_ANSWER };
	CButton	m_cButtonOK;
	CEdit	m_cEdit1;
	CString	m_strEdit1;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgRemoteCommandInterface)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CxDlgRemoteCommandInterface)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGRMANS_H__6DD3BF63_7B13_11D2_A2AF_00609725DDAF__INCLUDED_)
