/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : xtbpkey.h, Header File                                                //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II./ VDBA                                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : Popup Dialog Box for Creating a Primary Key of Table                  //
****************************************************************************************/
#if !defined(AFX_XTBPKEY_H__58F93382_61E4_11D2_A2A7_00609725DDAF__INCLUDED_)
#define AFX_XTBPKEY_H__58F93382_61E4_11D2_A2A7_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "calsctrl.h"
extern "C"
{
#include "dbaset.h"
}


class CxDlgTablePrimaryKey : public CDialog
{
public:
	CxDlgTablePrimaryKey(CWnd* pParent = NULL);
	void SetAlter(){m_bAlter = TRUE;}
	void SetPrimaryKeyParam(PRIMARYKEYSTRUCT* pKeyParam)
	{
		m_pKeyParam  = pKeyParam;
		m_strKeyName = pKeyParam->tchszPKeyName;
	}
	void SetTableParams(LPVOID pTable){m_pTable = pTable;}

	// Dialog Data
	//{{AFX_DATA(CxDlgTablePrimaryKey)
	enum { IDD = IDD_XTABLE_PRIMARY_KEY };
	CButton	m_cButtonDeleteColumn;
	CButton	m_cButtonAddColumn;
	CEdit	m_cEditKeyName;
	CButton	m_cButtonIndex;
	CButton	m_cButtonDeleteCascade;
	CButton	m_cButtonDelete;
	CListBox	m_cListKey;
	CString	m_strKeyName;
	//}}AFX_DATA
	CString m_strDatabase;

	//
	// For altering the table:
	CString m_strTable;
	CString m_strTableOwner;


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgTablePrimaryKey)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	CImageList m_ImageCheck;
	CuListCtrl m_cListCtrl;
	CBitmapButton m_cButtonUp;
	CBitmapButton m_cButtonDown;

	PRIMARYKEYSTRUCT* m_pKeyParam;
	BOOL m_bAlter;
	int  m_nDelMode;
	LPVOID m_pTable; // Must be casted to LPTABLEPARAMS

	void AddTableColumn (LPCTSTR lpszColumn, BOOL bNullable);
	void SetDisplayMode();
	void EnableControls();

	// Generated message map functions
	//{{AFX_MSG(CxDlgTablePrimaryKey)
	afx_msg void OnButtonDelete();
	afx_msg void OnButtonDeleteCascade();
	afx_msg void OnButtonIndex();
	afx_msg void OnAddColumn();
	afx_msg void OnButtonDropColumn();
	afx_msg void OnButtonUpColumn();
	afx_msg void OnButtonDownColumn();
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	afx_msg void OnItemchangedMfcList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkMfcList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkMfcList2();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XTBPKEY_H__58F93382_61E4_11D2_A2A7_00609725DDAF__INCLUDED_)
