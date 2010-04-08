/*
**  Copyright (c) 2006 Ingres Corporation. All Rights Reserved.

/*
**  Name: Welcome.h : header file
**
**  History:
**
**	05-Jun-2001 (penga03)
**	    Created.
**	01-Mar-2006 (drivi01)
**	    Reused for MSI Patch Installer on Windows.
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
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WELCOME_H__769F61B2_E772_42FC_9B83_A0E5A0F01E5D__INCLUDED_)
