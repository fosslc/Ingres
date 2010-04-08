/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlpgdis.h : header file
**    Project  : SqlTest ActiveX.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Property page for Display
**
** History:
**
** 06-Mar-2002 (uk$so01)
**    Created
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 26-Feb-2003 (uk$so01)
**    SIR #106648, conform to the change of DDS VDBA split
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
*/

#if !defined(AFX_SQLPGDIS_H__FC40B05C_2CEE_11D6_87A5_00C04F1F754A__INCLUDED_)
#define AFX_SQLPGDIS_H__FC40B05C_2CEE_11D6_87A5_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CSqlqueryPropPageDisplay : public COlePropertyPage
{
	DECLARE_DYNCREATE(CSqlqueryPropPageDisplay)
	DECLARE_OLECREATE_EX(CSqlqueryPropPageDisplay)

public:
	CSqlqueryPropPageDisplay();

	// Dialog Data
	//{{AFX_DATA(CSqlqueryPropPageDisplay)
	enum { IDD = IDD_PROPPAGE_DISPLAY };
	CEdit	m_cEditF4Scale;
	CEdit	m_cEditF4Precision;
	CEdit	m_cEditF8Scale;
	CEdit	m_cEditF8Precision;
	CSpinButtonCtrl	m_cSpinF8Scale;
	CSpinButtonCtrl	m_cSpinF8Precision;
	CSpinButtonCtrl	m_cSpinF4Scale;
	CSpinButtonCtrl	m_cSpinF4Precision;
	long	m_nF4Scale;
	long	m_nF4Width;
	BOOL	m_bF4Exponential;
	long	m_nF8Scale;
	long	m_nF8Width;
	BOOL	m_bF8Exponential;
	BOOL	m_bShowGrid;
	//}}AFX_DATA
	int m_nQepDisplayMode;
	int m_nXmlDisplayMode;

	BOOL OnHelp(LPCTSTR lpszHelpDir);
	// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);        // DDX/DDV support

	// Message maps
protected:
	//{{AFX_MSG(CSqlqueryPropPageDisplay)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditF8width();
	afx_msg void OnChangeEditF8scale();
	afx_msg void OnChangeEditF4width();
	afx_msg void OnChangeEditF4scale();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SQLPGDIS_H__FC40B05C_2CEE_11D6_87A5_00C04F1F754A__INCLUDED_)
