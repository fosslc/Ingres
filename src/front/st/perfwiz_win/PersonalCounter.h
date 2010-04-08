#if !defined(AFX_PERSONALCOUNTER_H__B5D37087_5FCD_11D3_B86D_AA000400CF10__INCLUDED_)
#define AFX_PERSONALCOUNTER_H__B5D37087_5FCD_11D3_B86D_AA000400CF10__INCLUDED_

#include "256bmp.h"

#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

/*
**
**  Name: PersonalCounter.h
**
**  Description:
**	This is the header file for the PersonalCounter.cpp file.
**
**  History:
**	06-sep-1999 (somsa01)
**	    Created.
*/

/*
** CPersonalCounter dialog
*/
class CPersonalCounter : public CPropertyPage
{
    DECLARE_DYNCREATE(CPersonalCounter)

/* Construction */
public:
    LPCTSTR MakeShortString(CDC* pDC, LPCTSTR lpszLong, int nColumnLen, int nOffset);
    void InitCounterLists();
    void InitListViewControls();
    C256bmp m_image;
    CPersonalCounter();
    CPersonalCounter(CPropertySheet *ps);
    ~CPersonalCounter();

/* Dialog Data */
    //{{AFX_DATA(CPersonalCounter)
    enum { IDD = IDD_PERSONAL_COUNTER_PAGE };
    CListCtrl	m_PersonalCtrList;
    CListCtrl	m_CounterList;
    CComboBox	m_ObjectList;
    CComboBox	m_GroupList;
    //}}AFX_DATA

/* Overrides */
    /* ClassWizard generate virtual function overrides */
    //{{AFX_VIRTUAL(CPersonalCounter)
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
    //{{AFX_MSG(CPersonalCounter)
    virtual BOOL OnInitDialog();
    afx_msg void OnSelchangeComboObjects();
    afx_msg void OnSelchangeComboGroups();
    afx_msg void OnButtonAdd();
    afx_msg void OnButtonDelete();
    afx_msg void OnClickCounterList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnClickPersonalCounterList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
    afx_msg void OnItemchangedCounterList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnItemchangedPersonalCounterList(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PERSONALCOUNTER_H__B5D37087_5FCD_11D3_B86D_AA000400CF10__INCLUDED_)
