#if !defined(AFX_PERSONALHELP_H__E0FA50EF_6063_11D3_B86D_AA000400CF10__INCLUDED_)
#define AFX_PERSONALHELP_H__E0FA50EF_6063_11D3_B86D_AA000400CF10__INCLUDED_

#include "256bmp.h"
#include "NewListCtrl.h"

#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

/*
**
**  Name: PersonalHelp.h
**
**  Description:
**	This is the header file for the PersonalHelp.cpp file.
**
**  History:
**	06-sep-1999 (somsa01)
**	    Created.
**	15-oct-1999 (somsa01)
**	    Removed OnItemChangedPcList() function.
*/

/*
** CPersonalHelp dialog
*/
class CPersonalHelp : public CPropertyPage
{
    DECLARE_DYNCREATE(CPersonalHelp)

/* Construction */
public:
    void ListViewInsert();
    void InitListViewControl();
    LPCTSTR MakeShortString(CDC *pDC, LPCTSTR lpszLong, int nColumnLen, int nOffset);
    C256bmp m_image;
    CPersonalHelp();
    CPersonalHelp(CPropertySheet *ps);
    ~CPersonalHelp();

/* Dialog Data */
    //{{AFX_DATA(CPersonalHelp)
    enum { IDD = IDD_PERSONAL_HELP_PAGE };
    CComboBox	m_GroupList;
    CNewListCtrl	m_PersonalCtrList;
    //}}AFX_DATA

/* Overrides */
    /* ClassWizard generate virtual function overrides */
    //{{AFX_VIRTUAL(CPersonalHelp)
    public:
    virtual BOOL OnSetActive();
    virtual LRESULT OnWizardNext();
    virtual LRESULT OnWizardBack();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    /* DDX/DDV support */
    //}}AFX_VIRTUAL

/* Implementation */
protected:
    CPropertySheet *m_propertysheet;
    /* Generated message map functions */
    //{{AFX_MSG(CPersonalHelp)
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
    virtual BOOL OnInitDialog();
    afx_msg void OnDblclkPcList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnClickPcList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSelchangePgList();
    afx_msg void OnButtonSaveHelp();
    afx_msg void OnUpdateEditHelp();
    afx_msg void OnButtonReset();
    afx_msg	LRESULT OnUserMessage(WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PERSONALHELP_H__E0FA50EF_6063_11D3_B86D_AA000400CF10__INCLUDED_)
