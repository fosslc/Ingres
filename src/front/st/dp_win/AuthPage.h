#if !defined(AFX_AUTHPAGE_H__576351BE_33A4_11D3_B855_AA000400CF10__INCLUDED_)
#define AFX_AUTHPAGE_H__576351BE_33A4_11D3_B855_AA000400CF10__INCLUDED_

#include "256Bmp.h"
#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

/*
**
**  Name: AuthPage.h
**
**  Description:
**	This is the header file for the AuthPage.cpp file.
**
**  History:
**	10-jul-1999 (somsa01)
**	    Created.
*/

/*
** AuthPage dialog
*/
class AuthPage : public CPropertyPage
{
    DECLARE_DYNCREATE(AuthPage)

/* Construction */
public:
    C256bmp m_image;
    AuthPage();
    AuthPage(CPropertySheet *ps);
    ~AuthPage();

/* Dialog Data */
    //{{AFX_DATA(AuthPage)
    enum { IDD = IDD_AUTH_PAGE };
    // NOTE - ClassWizard will add data members here.
    //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_DATA

/* Overrides */
    /* ClassWizard generate virtual function overrides */
    //{{AFX_VIRTUAL(AuthPage)
	public:
    virtual BOOL OnSetActive();
    virtual LRESULT OnWizardNext();
	protected:
    virtual void DoDataExchange(CDataExchange* pDX); /* DDX/DDV support */
	//}}AFX_VIRTUAL

/* Implementation */
protected:
    CPropertySheet * m_propertysheet;
    /* Generated message map functions */
    //{{AFX_MSG(AuthPage)
    virtual BOOL OnInitDialog();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AUTHPAGE_H__576351BE_33A4_11D3_B855_AA000400CF10__INCLUDED_)
