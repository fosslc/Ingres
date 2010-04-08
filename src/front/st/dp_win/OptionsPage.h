#if !defined(AFX_OPTIONSPAGE_H__BD46DE45_34CE_11D3_B4D3_5A39AD000000__INCLUDED_)
#define AFX_OPTIONSPAGE_H__BD46DE45_34CE_11D3_B4D3_5A39AD000000__INCLUDED_

#include "256Bmp.h"
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
**	10-jul-1999 (somsa01)
**	    Created.
*/

/*
** COptionsPage dialog
*/
class COptionsPage : public CPropertyPage
{
    DECLARE_DYNCREATE(COptionsPage)

/* Construction */
public:
    C256bmp m_image;
    COptionsPage();
    ~COptionsPage();
    COptionsPage(CPropertySheet *ps);

/* Dialog Data */
    //{{AFX_DATA(COptionsPage)
    enum { IDD = IDD_OPTIONS_PAGE };
    CComboBox	m_II;
    CComboBox	m_Pagesize;
    CComboBox	m_Version;
    //}}AFX_DATA

/* Overrides */
    /* ClassWizard generate virtual function overrides */
    //{{AFX_VIRTUAL(COptionsPage)
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
    //{{AFX_MSG(COptionsPage)
    virtual BOOL OnInitDialog();
    afx_msg void OnCheckPb();
    afx_msg void OnCheckPe();
    afx_msg void OnKillfocusEditModpages();
    afx_msg void OnKillfocusEditNumpages();
    afx_msg void OnKillfocusEditNumbytes();
    afx_msg void OnCheckRe();
    afx_msg void OnCheckPn();
    afx_msg void OnCheckIi();
    afx_msg void OnCheckTs();
    afx_msg void OnCheckRw();
    afx_msg void OnCheckLb();
    afx_msg void OnCheckPs();
    afx_msg void OnCheckTp();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OPTIONSPAGE_H__BD46DE45_34CE_11D3_B4D3_5A39AD000000__INCLUDED_)
