/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : fformat3.h : header file
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Page of Tab (CSV format)
**
** History:
**
** 26-Dec-2000 (uk$so01)
**    Created
**/


#if !defined(AFX_FFORMAT3_H__1A885E3A_D691_11D4_8739_00C04F1F754A__INCLUDED_)
#define AFX_FFORMAT3_H__1A885E3A_D691_11D4_8739_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "lsctlhug.h"


class CaIIAInfo;
class CaSeparatorSet;
class CaSeparator;


class CuDlgPageCsv : public CDialog
{
public:
	CuDlgPageCsv(CWnd* pParent = NULL);
	void SetFormat (CaIIAInfo* pData, CaSeparatorSet* pSet, CaSeparator* pSeparator);
	void GetFormat (CaSeparatorSet*& pSet, CaSeparator*& pSeparator)
	{
		pSet = m_pSeparatorSet;
		pSeparator = m_pSeparator;
	}

	// Dialog Data
	//{{AFX_DATA(CuDlgPageCsv)
	enum { IDD = IDD_PROPPAGE_FORMAT_CSV };
	CButton	m_cCheckIgnoreTrailingSeparator;
	CButton	m_cCheckConfirmChoice;
	CStatic	m_cStatic1;
	CComboBox	m_cComboTextQualifier;
	CButton	m_cCheckConsecutiveSeparatorAsOne;
	CEdit	m_cEditSeparator;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgPageCsv)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CaIIAInfo* m_pData;
	CaSeparatorSet* m_pSeparatorSet;
	CaSeparator* m_pSeparator;

	CFont m_font;
	SIZE m_sizeText;
	int  m_nCoumnCount;
	BOOL m_bFirstUpdate;
	CuListCtrlHuge m_listCtrlHuge;

	void DisplaySeparatorSetInfo(CaSeparatorSet* pSeparatorSet, CaSeparator* pSeparator);
	// Generated message map functions
	//{{AFX_MSG(CuDlgPageCsv)
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

#endif // !defined(AFX_FFORMAT3_H__1A885E3A_D691_11D4_8739_00C04F1F754A__INCLUDED_)
