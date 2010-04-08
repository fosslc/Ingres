#pragma once
/*
**  Copyright (c) 2001, 2006 Ingres Corporation.
*/

/*
**  Name: ChooseInstallCode.h
**
**  Description:
**	Header file for ChooseInstallCode dialog.
**
**  History:
**  15-Nov-2006 (drivi01)
**		SIR 116599
**		Enhanced pre-installer in effort to improve installer usability.
*/



class ChooseInstallCode : public CPropertyPage
{
	DECLARE_DYNAMIC(ChooseInstallCode)

public:
	ChooseInstallCode();
	virtual ~ChooseInstallCode();

// Dialog Data
	enum { IDD = IDD_CHOOSEINSTALLCODE };
	CButton m_default;
	CButton m_custom;
	CButton m_info;
	CEdit m_code;
	CStatic m_stat_id;
	CFont m_font_bold;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnSetActive();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnEnChangeEditId();
	afx_msg BOOL OnWizardFinish();
	afx_msg LRESULT OnWizardNext();
	afx_msg void OnBnClickedRadioCustom();
	afx_msg void OnBnClickedRadioDefault();
	afx_msg void OnBnClickedButtonInfo();
};
