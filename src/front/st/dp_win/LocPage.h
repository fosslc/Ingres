#if !defined(AFX_LOCPAGE_H__3ECC5927_33C4_11D3_B857_AA000400CF10__INCLUDED_)
#define AFX_LOCPAGE_H__3ECC5927_33C4_11D3_B857_AA000400CF10__INCLUDED_

#include "256Bmp.h"
#include "NewListCtrl.h"
#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

/*
**
**  Name: LocPage.h
**
**  Description:
**	This is the header file for the LocPage.cpp file.
**
**  History:
**	10-jul-1999 (somsa01)
**	    Created.
*/

/*
** LocPage dialog
*/
class LocPage : public CPropertyPage
{
    DECLARE_DYNCREATE(LocPage)

/* Construction */
public:
    C256bmp m_image;
    LocPage();
    LocPage(CPropertySheet *ps);
    ~LocPage();
    void InitListViewControl();
    void InitListViewControl2();
    void ListViewInsert(CString value);
    void ListViewInsert2(CString value);
    LPCTSTR MakeShortString(CDC* pDC, LPCTSTR lpszLong, int nColumnLen,
			    int nOffset);

/* Dialog Data */
    //{{AFX_DATA(LocPage)
    enum { IDD = IDD_LOC_PAGE };
    CNewListCtrl	m_TableList;
    CNewListCtrl	m_FileList;
    //}}AFX_DATA

/* Overrides */
    /* ClassWizard generate virtual function overrides */
    //{{AFX_VIRTUAL(LocPage)
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
    //{{AFX_MSG(LocPage)
    afx_msg void OnRadioConnect();
    afx_msg void OnRadioFile();
    virtual BOOL OnInitDialog();
    afx_msg void OnKeydownListFiles(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnButtonDelete();
    afx_msg void OnButtonAdd();
    afx_msg void OnClickListFiles(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDblclkListFiles(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnItemchangedListFiles(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
    afx_msg void OnButtonAdd2();
    afx_msg void OnButtonDelete2();
    afx_msg void OnClickListTables(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDblclkListTables(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKeydownListTables(NMHDR* pNMHDR, LRESULT* pResult);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOCPAGE_H__3ECC5927_33C4_11D3_B857_AA000400CF10__INCLUDED_)
