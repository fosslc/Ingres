#if !defined(AFX_OPTIONS3PAGE_H__7CEB7034_36E1_11D3_B4D5_C85956000000__INCLUDED_)
#define AFX_OPTIONS3PAGE_H__7CEB7034_36E1_11D3_B4D5_C85956000000__INCLUDED_

#include "256Bmp.h"
#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

/*
**
**  Name: Options3Page.h
**
**  Description:
**	This is the header file for the Options3Page.cpp file.
**
**  History:
**	10-jul-1999 (somsa01)
**	    Created.
*/

/*
** COptions3Page dialog
*/
class COptions3Page : public CPropertyPage
{
    DECLARE_DYNCREATE(COptions3Page)

/* Construction */
public:
    C256bmp m_image;
    COptions3Page();
    ~COptions3Page();
    COptions3Page(CPropertySheet *ps);

/* Dialog Data */
    //{{AFX_DATA(COptions3Page)
    enum { IDD = IDD_OPTIONS3_PAGE };
	// NOTE - ClassWizard will add data members here.
	//    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_DATA

/* Overrides */
    /* ClassWizard generate virtual function overrides */
    //{{AFX_VIRTUAL(COptions3Page)
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
    //{{AFX_MSG(COptions3Page)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OPTIONS3PAGE_H__7CEB7034_36E1_11D3_B4D5_C85956000000__INCLUDED_)
