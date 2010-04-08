/*
**  Copyright (c) 2001 Ingres Corporation
*/

/*
**  Name: Welcome.h : header file
**
**  History:
**
**	05-Jun-2001 (penga03)
**	    Created.
**  15-Nov-2006 (drivi01)
**		SIR 116599
**		Enhanced pre-installer in effort to improve installer usability.
*/

#if !defined(AFX_WELCOME_H__769F61B2_E772_42FC_9B83_A0E5A0F01E5D__INCLUDED_)
#define AFX_WELCOME_H__769F61B2_E772_42FC_9B83_A0E5A0F01E5D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


/////////////////////////////////////////////////////////////////////////////
// CWelcome dialog

class CWelcome : public CPropertyPage
{
	DECLARE_DYNCREATE(CWelcome)

// Construction
public:
	CWelcome();
	~CWelcome();

// Dialog Data
	//{{AFX_DATA(CWelcome)
	enum { IDD = IDD_WELCOMEDLG };
	C256bmp	m_image;
	CStatic m_title;
	CStatic m_text;
	CFont m_font_bold;
	CFont m_font;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CWelcome)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CWelcome)
	virtual BOOL OnInitDialog();
	virtual LRESULT OnWizardNext();
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	
	
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WELCOME_H__769F61B2_E772_42FC_9B83_A0E5A0F01E5D__INCLUDED_)
