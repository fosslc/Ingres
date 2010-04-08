#if !defined(AFX_TABLELIST_H__619F7649_D56E_11D2_B83F_AA000400CF10__INCLUDED_)
#define AFX_TABLELIST_H__619F7649_D56E_11D2_B83F_AA000400CF10__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

/*
**
**  Name: UserUdts.h
**
**  Description:
**	This is the header file for the UserUdts.cpp file.
**
**  History:
**	23-apr-1999 (somsa01)
**	    Created.
*/

#include "256bmp.h"

/*
** UserUdts dialog
*/
class UserUdts : public CPropertyPage
{
    DECLARE_DYNCREATE(UserUdts)

/* Construction */
public:
    CImageList *m_StateImageList;
    C256bmp m_image;
    UserUdts();
    UserUdts(CPropertySheet *ps);
    ~UserUdts();
    void InitListViewControl();
    int ListViewInsert();
    LPCTSTR MakeShortString(CDC* pDC, LPCTSTR lpszLong, int nColumnLen, int nOffset);

/* Dialog Data */
    //{{AFX_DATA(UserUdts)
    enum { IDD = IDD_USERUDT_PAGE };
    CListCtrl	m_FileSelectList;
    //}}AFX_DATA

/* Overrides */
    /* ClassWizard generate virtual function overrides */
    //{{AFX_VIRTUAL(UserUdts)
    public:
    virtual BOOL OnSetActive();
    virtual LRESULT OnWizardNext();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    /* DDX/DDV support */
    //}}AFX_VIRTUAL

/* Implementation */
protected:
    CPropertySheet *m_propertysheet;
    /* Generated message map functions */
    //{{AFX_MSG(UserUdts)
    virtual BOOL OnInitDialog();
    afx_msg void OnClickListFiles(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnButtonSelect();
    afx_msg void OnButtonDeselect();
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
    afx_msg void OnButtonViewsrc();
    afx_msg void OnKeydownListFiles(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDblclkListFiles(NMHDR* pNMHDR, LRESULT* pResult);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TABLELIST_H__619F7649_D56E_11D2_B83F_AA000400CF10__INCLUDED_)
