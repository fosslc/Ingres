/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : bkgdpref.h, Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : INGRES II / VDBA                                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Dialog for Preference Settings for Background Refresh                 //
****************************************************************************************/
#if !defined(AFX_BKGDPREF_H__21012B22_89C5_11D2_A2B1_00609725DDAF__INCLUDED_)
#define AFX_BKGDPREF_H__21012B22_89C5_11D2_A2B1_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CxDlgPreferencesBackgroundRefresh : public CDialog
{
public:
	CxDlgPreferencesBackgroundRefresh(CWnd* pParent = NULL);

	// Dialog Data
	//{{AFX_DATA(CxDlgPreferencesBackgroundRefresh)
	enum { IDD = IDD_XPREFERENCES_BACKGROUND_REFRESH };
	CButton	m_cCheckRefreshOnLoad;
	CButton	m_cCheckSychronizeParent;
	CSpinButtonCtrl	m_cSpin1;
	CEdit	m_cEditFrequency;
	CComboBox	m_cComboUnit;
	CComboBox	m_cComboObjectType;
	BOOL	m_bSaveAsDefault;
	BOOL	m_bSynchronizeAmongObjects;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgPreferencesBackgroundRefresh)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CString m_strAllObject;

	void FillObjectType ();
	void FillUnit();
	void ShowCharacteristics (int nSel);
	void SetDefaultMessageOption();
	void SaveAsDefaults();
	//
	// nCheckBox = 0: Synchronize with Parent.
	// nCheckBox = 1: Refresh on Load
	void UpdateCharacteristics(int nCheckBox);
	void UpdateFrequencyUnit(BOOL bFrequencyKillFocus = FALSE);
	int  GetSynchronizeParent(); 
	int  GetRefreshOnLoad(); 
	//
	// nTest = 0: Frequency and Unit
	// nTest = 1: Synchronize on Parent
	// nTest = 2: Refresh on Load
	BOOL IsEqualAllObjectType(int nTest);


	// Generated message map functions
	//{{AFX_MSG(CxDlgPreferencesBackgroundRefresh)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnSelchangeComboObjectType();
	afx_msg void OnSelchangeComboUnit();
	afx_msg void OnKillfocusEditFrequency();
	afx_msg void OnButtonLoadDefault();
	afx_msg void OnChangeEditFrequency();
	virtual void OnOK();
	afx_msg void OnCheckSynchonizeParent();
	afx_msg void OnCheckRefreshOnLoad();
	afx_msg void OnKillfocusComboUnit();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BKGDPREF_H__21012B22_89C5_11D2_A2B1_00609725DDAF__INCLUDED_)
