#if !defined(AFX_MAINPAGE_H__AF316AF0_F98B_11D2_B842_AA000400CF10__INCLUDED_)
#define AFX_MAINPAGE_H__AF316AF0_F98B_11D2_B842_AA000400CF10__INCLUDED_

#include "256Bmp.h"
#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

/*
**
**  Name: MainPage.h
**
**  Description:
**	This is the header file for the MainPage.cpp file.
**
**  History:
**	23-apr-1999 (somsa01)
**	    Created.
*/

/*
** MainPage dialog
*/
class MainPage : public CPropertyPage
{
    DECLARE_DYNCREATE(MainPage)

/* Construction */
public:
    C256bmp m_image;
    MainPage();
    MainPage(CPropertySheet *ps);
    ~MainPage();

/* Dialog Data */
    //{{AFX_DATA(MainPage)
    enum { IDD = IDD_MAIN_PAGE };
    //}}AFX_DATA


/* Overrides */
    /* ClassWizard generate virtual function overrides */
    //{{AFX_VIRTUAL(MainPage)
    public:
    virtual BOOL OnSetActive();
    virtual LRESULT OnWizardNext();
    virtual BOOL OnWizardFinish();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    /* DDX/DDV support */
    //}}AFX_VIRTUAL

/* Implementation */
protected:
    CPropertySheet * m_propertysheet;
    /* Generated message map functions */
    //{{AFX_MSG(MainPage)
    virtual BOOL OnInitDialog();
    afx_msg void OnCheckBackup();
    afx_msg void OnCheckRestore();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINPAGE_H__AF316AF0_F98B_11D2_B842_AA000400CF10__INCLUDED_)
