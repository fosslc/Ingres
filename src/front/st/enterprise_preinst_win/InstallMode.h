#pragma once
/*
**  Copyright (c) 2001, 2006 Ingres Corporation.
*/

/*
**  Name: InstallMode.h
**
**  Description:
**	Header file for InstallMode dialog.
**
**  History:
**  15-Nov-2006 (drivi01)
**		SIR 116599
**		Enhanced pre-installer in effort to improve installer usability.
*/



class InstallMode : public CPropertyPage//public CPropertyPage
{
	DECLARE_DYNCREATE(InstallMode)

public:
	InstallMode();   // standard constructor
	virtual ~InstallMode();

// Dialog Data
	//{{AFX_DATA(InstallMode)
	enum { IDD = IDD_INSTALLMODE };
	C256bmp	m_image_top;
	CButton m_express;
	CButton m_advanced;
	CFont m_font_bold;
	//}}AFX_DATA

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(InstallMode)
	public:
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnSetActive();
	//}}AFX_VIRTUAL

protected:
	//virtual LRESULT OnWizardNext();
	//{{AFX_VIRTUAL(InstallMode)
	protected:
	virtual BOOL OnInitDialog();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnWizardBack();
	//}}AFX_VIRTUAL
	DECLARE_MESSAGE_MAP()



public:

};
