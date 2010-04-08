#if !defined(AFX_PERSONALGROUP_H__ABD2B5DF_5E70_11D3_B867_AA000400CF10__INCLUDED_)
#define AFX_PERSONALGROUP_H__ABD2B5DF_5E70_11D3_B867_AA000400CF10__INCLUDED_

#include "256bmp.h"

#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

/*
**
**  Name: PersonalGroup.h
**
**  Description:
**	This is the header file for the PersonalGroup.cpp file.
**
**  History:
**	06-sep-1999 (somsa01)
**	    Created.
*/

/*
** CPersonalGroup dialog
*/
class CPersonalGroup : public CPropertyPage
{
    DECLARE_DYNCREATE(CPersonalGroup)

/* Construction */
public:
    LPCTSTR MakeShortString(CDC* pDC, LPCTSTR lpszLong, int nColumnLen,
			    int nOffset);
    void InitListViewControl();
    C256bmp m_image;
    CPersonalGroup();
    CPersonalGroup(CPropertySheet *ps);
    ~CPersonalGroup();

/* Dialog Data */
    //{{AFX_DATA(CPersonalGroup)
    enum { IDD = IDD_PERSONAL_GROUP_PAGE };
    CListCtrl	m_PersonalGroupList;
    //}}AFX_DATA

/* Overrides */
    /* ClassWizard generate virtual function overrides */
    //{{AFX_VIRTUAL(CPersonalGroup)
    public:
    virtual BOOL OnSetActive();
    virtual LRESULT OnWizardBack();
    virtual LRESULT OnWizardNext();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    /* DDX/DDV support */
    //}}AFX_VIRTUAL

/* Implementation */
protected:
    CPropertySheet * m_propertysheet;
    /* Generated message map functions */
    //{{AFX_MSG(CPersonalGroup)
    virtual BOOL OnInitDialog();
    afx_msg void OnButtonAddGroup();
    afx_msg void OnButtonDeleteGroup();
    afx_msg void OnButtonEditGroup();
    afx_msg void OnKeydownPersonalGroupList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDblclkPersonalGroupList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
    afx_msg void OnClickPersonalGroupList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnItemchangedPersonalGroupList(NMHDR* pNMHDR, LRESULT* pResult);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PERSONALGROUP_H__ABD2B5DF_5E70_11D3_B867_AA000400CF10__INCLUDED_)
