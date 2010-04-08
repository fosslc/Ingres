#if !defined(AFX_OPTIONSPAGE_H__FBEF5669_FD1C_11D2_B842_AA000400CF10__INCLUDED_)
#define AFX_OPTIONSPAGE_H__FBEF5669_FD1C_11D2_B842_AA000400CF10__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

/*
**
**  Name: OptionsPage.h
**
**  Description:
**	This is the header file for the OptionsPage.cpp file.
**
**  History:
**	28-apr-1999 (somsa01)
**	    Created.
**	22-jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	03-apr-2008 (drivi01)
**	    Put spatial objects support back.
*/

#include "256bmp.h"

/*
** COptionsPage dialog
*/
class COptionsPage : public CPropertyPage
{
    DECLARE_DYNCREATE(COptionsPage)

/* Construction */
public:
    C256bmp m_image;
    BOOL DemosChecked;
    COptionsPage();
    COptionsPage(CPropertySheet *ps);
    ~COptionsPage();

/* Dialog Data */
    //{{AFX_DATA(COptionsPage)
    enum { IDD = IDD_OPTIONS_PAGE };
    //}}AFX_DATA

/* Overrides */
    /* ClassWizard generate virtual function overrides */
    //{{AFX_VIRTUAL(COptionsPage)
    public:
    virtual BOOL OnSetActive();
    virtual LRESULT OnWizardNext();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    /* DDX/DDV support */
    //}}AFX_VIRTUAL

/* Implementation */
protected:
    CPropertySheet * m_propertysheet;
    /* Generated message map functions */
    //{{AFX_MSG(COptionsPage)
    afx_msg void OnCheckSpatial();
    afx_msg void OnCheckUserUdt();
    virtual BOOL OnInitDialog();
    afx_msg void OnButtonLoc();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OPTIONSPAGE_H__FBEF5669_FD1C_11D2_B842_AA000400CF10__INCLUDED_)
