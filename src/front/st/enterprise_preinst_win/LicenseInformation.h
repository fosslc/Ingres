/*
**  Copyright (c) 2009 Ingres Corporation
*/
#pragma once
/*
**  Name: LicenseInformation.h : header file
**
**  History:
**
**	02-Nov-2009 (drivi01)
**		Created.
*/


class LicenseInformation : public CPropertyPage
{
	DECLARE_DYNAMIC(LicenseInformation)

public:
	LicenseInformation();
	virtual ~LicenseInformation();
	virtual BOOL OnInitDialog();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	CFont m_font;

// Dialog Data
	enum { IDD = IDD_LICENSEINFORMATION };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
