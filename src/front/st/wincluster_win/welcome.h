/*
**  Copyright (c) 2004 Ingres Corporation
*/

/*
**  Name: Welcome.h : header file
**
**  History:
**
**	29-apr-2004 (wonst02)
**	    Created; cloned from similar enterprise_preinst directory.
*/

#if !defined(AFX_WELCOME_H__769F61B2_E772_42FC_9B83_A0E5A0F01E5D__INCLUDED_)
#define AFX_WELCOME_H__769F61B2_E772_42FC_9B83_A0E5A0F01E5D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


/////////////////////////////////////////////////////////////////////////////
// WcWelcome dialog

class WcWelcome : public CPropertyPage
{
	DECLARE_DYNCREATE(WcWelcome)

// Construction
public:
	WcWelcome();
	~WcWelcome();

// Dialog Data
	//{{AFX_DATA(WcWelcome)
	enum { IDD = IDD_WELCOMEDLG };
	C256bmp	m_image;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(WcWelcome)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(WcWelcome)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WELCOME_H__769F61B2_E772_42FC_9B83_A0E5A0F01E5D__INCLUDED_)
