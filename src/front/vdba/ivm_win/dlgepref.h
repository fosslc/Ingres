/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : dlgepref.h , Header File                                              //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II / Visual Manager                                            //
//    Author   : Sotheavut UK (uk$so01)                                                //
//                                                                                     //
//                                                                                     //
//    Purpose  : The Preferences setting of Visual Manager                             //
****************************************************************************************/
/* History:
* --------
* uk$so01: (05-Feb-1999 created)
*
*
*/

#ifndef __DLGEPREF_H__
#define __DLGEPREF_H__



/////////////////////////////////////////////////////////////////////////////
// CuDlgPropertyPageEventNotify dialog

class CuDlgPropertyPageEventNotify : public CPropertyPage
{
	DECLARE_DYNCREATE(CuDlgPropertyPageEventNotify)

public:
	CuDlgPropertyPageEventNotify();
	~CuDlgPropertyPageEventNotify();
	//
	// bOut = TRUE=>
	//   Retrieve the current data and update global class
	//   CaGeneralSetting.
	// bOut = FALSE=>
	//   Retrieve the current data from global class
	//   CaGeneralSetting and update to this dialog

	void UpdateSetting(BOOL bOut);

	// Dialog Data
	//{{AFX_DATA(CuDlgPropertyPageEventNotify)
	enum { IDD = IDD_PROPPAGE_EVENT_NOTIFY };
	BOOL	m_bSendingMail;
	BOOL	m_bMessageBox;
	BOOL	m_bSoundBeep;
	CEdit	m_cMaxEvent;
	CEdit	m_cMaxMemory;
	CEdit	m_cSinceDays;
	CString	m_strMaxMemory;
	CString	m_strMaxEvent;
	CString	m_strSinceDays;
	//}}AFX_DATA
	int m_nEventMaxType;

	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CuDlgPropertyPageEventNotify)
	public:
	virtual void OnOK();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	BOOL m_bInitDialog; // TRUE if OnInitDialog has been called;
	void EnableDisableControl();

	// Generated message map functions
	//{{AFX_MSG(CuDlgPropertyPageEventNotify)
	virtual BOOL OnInitDialog();
	afx_msg void OnRadioMaxMemory();
	afx_msg void OnRadioMaxEvent();
	afx_msg void OnRadioSinceDays();
	afx_msg void OnRadioSinceLastNameStarted();
	afx_msg void OnRadioNoLimit();
	afx_msg void OnKillfocusEditMaxMemory();
	afx_msg void OnKillfocusEditMaxEvent();
	afx_msg void OnKillfocusEditSinceDays();
	//}}AFX_MSG
	afx_msg void OnCheckChange();
	DECLARE_MESSAGE_MAP()

};



#endif // __DLGEPREF_H__
