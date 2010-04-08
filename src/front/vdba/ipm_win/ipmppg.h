/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : ipmppg.h , Header File 
**    Project  : INGRESII/Monitoring.
**    Author   : UK Sotheavut
**    Purpose  : Declaration of the CIpmPropPage property page class
**
** History:
**
** 12-Nov-2001 (uk$so01)
**    Created
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 07-Apr-2004 (uk$so01)
**    BUG #112109 / ISSUE 13342826, The min of frequency is 10 when unit is Second(s)
**    The min of frequency should be 1 if unit is other than Second(s).
*/

#if !defined(AFX_IPMPPG_H__AB474694_E577_11D5_878C_00C04F1F754A__INCLUDED_)
#define AFX_IPMPPG_H__AB474694_E577_11D5_878C_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CIpmPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CIpmPropPage)
	DECLARE_OLECREATE_EX(CIpmPropPage)

public:
	CIpmPropPage();

	// Dialog Data
	//{{AFX_DATA(CIpmPropPage)
	enum { IDD = IDD_PROPPAGE_IPM };
	CComboBox	m_cComboUnit;
	CSpinButtonCtrl	m_cSpinMaxSession;
	CButton	m_cCheckActivateBackroundRefresh;
	CEdit	m_cEditMaxSession;
	CSpinButtonCtrl	m_cSpinFrequency;
	CSpinButtonCtrl	m_cSpinTimeOut;
	CEdit	m_cEditTimeOut;
	CEdit	m_cEditFrequency;
	long	m_nFrequency;
	long	m_nTimeOut;
	BOOL	m_bActivateRefresh;
	BOOL	m_bShowGrid;
	int		m_nUnit;
	long	m_nMaxSession;
	//}}AFX_DATA
	BOOL OnHelp(LPCTSTR lpszHelpDir);

	// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	void EnableBkRefreshParam();

	// Message maps
protected:
	//{{AFX_MSG(CIpmPropPage)
	afx_msg void OnKillfocusEditTimeout();
	afx_msg void OnKillfocusEditFrequency();
	virtual BOOL OnInitDialog();
	afx_msg void OnKillfocusEditMaxsession();
	afx_msg void OnCheckActivaterefresh();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnCbnSelchangeCombo1();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IPMPPG_H__AB474694_E577_11D5_878C_00C04F1F754A__INCLUDED)
