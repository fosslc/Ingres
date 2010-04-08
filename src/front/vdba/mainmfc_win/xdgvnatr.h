/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : xdgvnatr.h, Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II./ VDBA                                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : Popup Dialog Box for Creating a VNode Attribute                       //
//               The Advanced concept of Virtual Node.                                 //
****************************************************************************************/
#if !defined(AFX_XDGVNATR_H__A4ADB042_594A_11D2_A2A5_00609725DDAF__INCLUDED_)
#define AFX_XDGVNATR_H__A4ADB042_594A_11D2_A2A5_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class CxDlgVirtualNodeAttribute : public CDialog
{
public:
	CxDlgVirtualNodeAttribute(CWnd* pParent = NULL);
	void SetAlter(){m_bAlter = TRUE;}

	// Dialog Data
	//{{AFX_DATA(CxDlgVirtualNodeAttribute)
	enum { IDD = IDD_XVNODE_ATTRIBUTE };
	CComboBox	m_cComboAttribute;
	CString	m_strAttributeName;
	CString	m_strAttributeValue;
	BOOL	m_bPrivate;
	//}}AFX_DATA
	CString m_strVNodeName;


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgVirtualNodeAttribute)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	BOOL m_bAlter;
	CString m_strOldAttributeName;
	CString m_strOldAttributeValue;
	BOOL m_bOldPrivate;

	// Generated message map functions
	//{{AFX_MSG(CxDlgVirtualNodeAttribute)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XDGVNATR_H__A4ADB042_594A_11D2_A2A5_00609725DDAF__INCLUDED_)
