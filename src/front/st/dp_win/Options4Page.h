#if !defined(AFX_OPTIONS4PAGE_H__7CEB7035_36E1_11D3_B4D5_C85956000000__INCLUDED_)
#define AFX_OPTIONS4PAGE_H__7CEB7035_36E1_11D3_B4D5_C85956000000__INCLUDED_

#include "256Bmp.h"
#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

/*
**
**  Name: Options4Page.h
**
**  Description:
**	This is the header file for the Options4Page.cpp file.
**
**  History:
**	10-jul-1999 (somsa01)
**	    Created.
*/

/*
** COptions4Page dialog
*/
class COptions4Page : public CPropertyPage
{
    DECLARE_DYNCREATE(COptions4Page)

/* Construction */
public:
    C256bmp m_image;
    COptions4Page();
    ~COptions4Page();
    COptions4Page(CPropertySheet *ps);

/* Dialog Data */
    //{{AFX_DATA(COptions4Page)
    enum { IDD = IDD_OPTIONS4_PAGE };
	// NOTE - ClassWizard will add data members here.
	//    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_DATA

/* Overrides */
    /* ClassWizard generate virtual function overrides */
    //{{AFX_VIRTUAL(COptions4Page)
    public:
    virtual BOOL OnSetActive();
    virtual BOOL OnWizardFinish();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX); /* DDX/DDV support */
    //}}AFX_VIRTUAL

/* Implementation */
protected:
    CPropertySheet * m_propertysheet;
    /* Generated message map functions */
    //{{AFX_MSG(COptions4Page)
    virtual BOOL OnInitDialog();
    afx_msg void OnCheckOutfile();
    afx_msg void OnCheckErrfile();
    afx_msg void OnCheckOptfile();
    afx_msg void OnCheckSqlfile();
    afx_msg void OnButtonOutfile();
    afx_msg void OnButtonErrfile();
    afx_msg void OnButtonOptfile();
    afx_msg void OnButtonSqlfile();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OPTIONS4PAGE_H__7CEB7035_36E1_11D3_B4D5_C85956000000__INCLUDED_)
