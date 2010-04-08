/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ppmsgdes.h : header file)
**    Project  : Visual Manager
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Property page of message description
**
** History:
**
** 06-Mar-2003 (uk$so01)
**    created
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
**/


#if !defined(AFX_PPMSGDES_H__514FE9C2_4D5D_11D7_880A_00C04F1F754A__INCLUDED_)
#define AFX_PPMSGDES_H__514FE9C2_4D5D_11D7_880A_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#define MSG_ALREADY_QUERIED    0x00000001

class CuPropertyPageMessageDescription : public CPropertyPage
{
	DECLARE_DYNCREATE(CuPropertyPageMessageDescription)

public:
	CuPropertyPageMessageDescription();
	~CuPropertyPageMessageDescription();

	// Dialog Data
	//{{AFX_DATA(CuPropertyPageMessageDescription)
	enum { IDD = IDD_MESSAGE_DESCRIPTION };
	CComboBox	m_cComboMessage;
	CComboBox	m_cComboCategory;
	CString	m_strParameter;
	CString	m_strExplanation;
	CString	m_strSystemStatus;
	CString	m_strRecommendation;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CuPropertyPageMessageDescription)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	//
	// m_nMsgAlreadyQueried
	// OnUpdateData and OnSelchangeComboCategory set m_nMsgAlreadyQueried to 0.
	// OnDropdownComboMessage queries messages and sets (if m_nMsgAlreadyQueried = 0)
	// m_uMsgAlreadyQueried to MSG_ALREADY_QUERIED.
	UINT m_uMsgAlreadyQueried;

	// Generated message map functions
	//{{AFX_MSG(CuPropertyPageMessageDescription)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeComboCategory();
	afx_msg void OnDropdownComboMessage();
	afx_msg void OnSelchangeComboMessage();
	afx_msg void OnButtonFind();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PPMSGDES_H__514FE9C2_4D5D_11D7_880A_00C04F1F754A__INCLUDED_)
