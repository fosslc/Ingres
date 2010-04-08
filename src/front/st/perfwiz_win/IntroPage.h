#if !defined(AFX_INTROPAGE_H__2C7F9D45_5BEE_11D3_B867_AA000400CF10__INCLUDED_)
#define AFX_INTROPAGE_H__2C7F9D45_5BEE_11D3_B867_AA000400CF10__INCLUDED_

#include "256Bmp.h"

#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

/*
**
**  Name: IntroPage.h
**
**  Description:
**	This is the header file for the IntroPage.cpp file.
**
**  History:
**	06-sep-1999 (somsa01)
**	    Created.
**	15-oct-1999 (somsa01)
**	    Moved selection of new chart file name to the Final Property
**	    Page.
*/

/*
** CIntroPage dialog
*/
class CIntroPage : public CPropertyPage
{
    DECLARE_DYNCREATE(CIntroPage)

/* Construction */
public:
    C256bmp m_image;
    CIntroPage();
    CIntroPage(CPropertySheet *ps);
    ~CIntroPage();

/* Dialog Data */
    //{{AFX_DATA(CIntroPage)
    enum { IDD = IDD_INTRO_PAGE };
    CComboBox	m_ChartLoc;
    //}}AFX_DATA

/* Overrides */
    /* ClassWizard generate virtual function overrides */
    //{{AFX_VIRTUAL(CIntroPage)
    public:
    virtual BOOL OnSetActive();
    virtual LRESULT OnWizardNext();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    /* DDX/DDV support */
    //}}AFX_VIRTUAL

/* Implementation */
protected:
    CPropertySheet * m_propertysheet;
    // Generated message map functions
    //{{AFX_MSG(CIntroPage)
    virtual BOOL OnInitDialog();
    afx_msg void OnCheckGroupModify();
    afx_msg void OnButtonBrowse();
    afx_msg void OnCheckPersonal();
    afx_msg void OnCheckRemove();
    afx_msg void OnRadioCreate();
    afx_msg void OnRadioUseExisting();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INTROPAGE_H__2C7F9D45_5BEE_11D3_B867_AA000400CF10__INCLUDED_)
