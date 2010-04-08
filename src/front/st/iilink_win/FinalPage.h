#if !defined(AFX_FINALPAGE_H__CDED577D_F990_11D2_B842_AA000400CF10__INCLUDED_)
#define AFX_FINALPAGE_H__CDED577D_F990_11D2_B842_AA000400CF10__INCLUDED_

#include "256Bmp.h"
#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

/*
**
**  Name: FinalPage.h
**
**  Description:
**	This is the header file for the FinalPage.cpp file.
**
**  History:
**	23-apr-1999 (somsa01)
**	    Created.
*/

/*
** FinalPage dialog
*/
class FinalPage : public CPropertyPage
{
    DECLARE_DYNCREATE(FinalPage)

/* Construction */
public:
    CFont EditFont;
    C256bmp m_image;
    FinalPage();
    FinalPage(CPropertySheet *ps);
    ~FinalPage();

/* Dialog Data */
    //{{AFX_DATA(FinalPage)
	enum { IDD = IDD_FINAL_PAGE };
	CEdit	m_LinkStmt;
	CEdit	m_CompileStmt;
	//}}AFX_DATA

/* Overrides */
    /* ClassWizard generate virtual function overrides */
    //{{AFX_VIRTUAL(FinalPage)
    public:
    virtual BOOL OnSetActive();
    virtual LRESULT OnWizardBack();
    virtual BOOL OnWizardFinish();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    /* DDX/DDV support */
    //}}AFX_VIRTUAL

/* Implementation */
protected:
    CPropertySheet * m_propertysheet;
    /* Generated message map functions */
    //{{AFX_MSG(FinalPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FINALPAGE_H__CDED577D_F990_11D2_B842_AA000400CF10__INCLUDED_)
