/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : fformat1.h: header file
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Page of Tab (DBF format)
**
** History:
**
** 26-Dec-2000 (uk$so01)
**    Created
**/


#if !defined(AFX_FFORMAT1_H__AE456E26_B3C5_11D4_8727_00C04F1F754A__INCLUDED_)
#define AFX_FFORMAT1_H__AE456E26_B3C5_11D4_8727_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "lsctlhug.h"
#include "formdata.h"


class CuDlgPageDbf : public CDialog
{
public:
	CuDlgPageDbf(CWnd* pParent = NULL);
	void OnOK() {return;}
	void OnCancel() {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgPageDbf)
	enum { IDD = IDD_PROPPAGE_FORMAT_DBF };
	CButton	m_cCheckConfirmChoice;
	CStatic	m_cStatic1;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgPageDbf)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CaIIAInfo* m_pData;
	CFont m_font;
	SIZE m_sizeText;
	int  m_nCoumnCount;
	BOOL m_bFirstUpdate;
	CuListCtrlHuge m_listCtrlHuge;

	// Generated message map functions
	//{{AFX_MSG(CuDlgPageDbf)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnCheckConfirmChoice();
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnCleanData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FFORMAT1_H__AE456E26_B3C5_11D4_8727_00C04F1F754A__INCLUDED_)
