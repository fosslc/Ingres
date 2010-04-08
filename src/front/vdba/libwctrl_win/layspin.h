/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : layspin.h, Header File
**    Project  : OpenIngres Configuration Manager
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Edit Box with Spin to be used with Editable Owner Draw ListCtrl
**
** History:
**
** xx-Sep-1997 (uk$so01)
**    created
** 28-Feb-2000 (uk$so01)
**    Position the member variable 'm_bReadOnly' before using the control
**/

#if !defined(AFX_LAYSPIN_H__53F32ED2_2DDF_11D1_A1F6_00609725DDAF__INCLUDED_)
#define AFX_LAYSPIN_H__53F32ED2_2DDF_11D1_A1F6_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "peditctl.h"   // Special Parse Edit

/////////////////////////////////////////////////////////////////////////////
// CuLayoutSpinDlg dialog

class CuLayoutSpinDlg : public CDialog
{
public:
	CuLayoutSpinDlg(CWnd* pParent = NULL);   // standard constructor
	CuLayoutSpinDlg(CWnd* pParent, CString strText, int iItem, int iSubItem);  
	CString GetText ();
	void SetData    (LPCTSTR lpszText, int iItem, int iSubItem);
	void GetText    (CString& strText);
	void GetEditItem(int& iItem, int& iSubItem){iItem = m_iItem; iSubItem = m_iSubItem;};
	void SetFocus();
	void OnOK();
	void OnCancel();
	void SetRange (int  Min, int  Max);
	void GetRange (int& Min, int& Max){Min = m_Min; Max = m_Max;}
	void SetUseSpin (BOOL bUseSpin = TRUE){m_bUseSpin = bUseSpin;}
	void SetReadOnly (BOOL bReadOnly = FALSE);

	void SetSpecialParse (BOOL bSpecialParse = TRUE);
	void SetParseStyle   (WORD wParseStyle   = PES_INTEGER){m_wParseStyle = wParseStyle;}

	// Dialog Data
	//{{AFX_DATA(CuLayoutSpinDlg)
	enum { IDD = IDD_LAYOUTCTRLSPIN };
	CString	m_strValue;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuLayoutSpinDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	WORD m_wParseStyle;
	BOOL m_bUseSpecialParse;
	CuParseEditCtrl m_cEdit1;

	CString m_strItemText;
	int     m_iItem;
	int     m_iSubItem;
	int     m_Min;
	int     m_Max;
	BOOL    m_bUseSpin;
	BOOL    m_bReadOnly;

	void Init();
	// Generated message map functions
	//{{AFX_MSG(CuLayoutSpinDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnChangeEdit1();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LAYSPIN_H__53F32ED2_2DDF_11D1_A1F6_00609725DDAF__INCLUDED_)
