#pragma once
/*
**  Copyright (c) 2001, 2006 Ingres Corporation.
*/

/*
**  Name: InstallType.h
**
**  Description:
**	Header file for InstallType dialog.
**
**  History:
**  15-Nov-2006 (drivi01)
**		SIR 116599
**		Enhanced pre-installer in effort to improve installer usability.
*/


class InstallType : public CPropertyPage
{
	DECLARE_DYNAMIC(InstallType)

public:
	InstallType();
	virtual ~InstallType();


// Dialog Data
	enum { IDD = IDD_INSTALLTYPE };
	CButton m_upgrade;
	CButton m_modify;
	CButton m_installnew;
	HBITMAP m_image;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	afx_msg HBRUSH OnCtlColor(CDC *pDC, CWnd *pWnd, UINT nCtlColor);
	afx_msg BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
public:
};
