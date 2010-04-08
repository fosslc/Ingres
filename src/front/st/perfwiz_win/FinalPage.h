#if !defined(AFX_FINALPAGE_H__2C7F9D47_5BEE_11D3_B867_AA000400CF10__INCLUDED_)
#define AFX_FINALPAGE_H__2C7F9D47_5BEE_11D3_B867_AA000400CF10__INCLUDED_

#include "256Bmp.h"
#include "winperf.h"
#include "perfchart.h"

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
**	06-sep-1999 (somsa01)
**	    Created.
**	15-oct-1999 (somsa01)
**	    Moved selection of new chart file name to here from the Intro
**	    Property Page.
*/

/*
** CFinalPage dialog
*/
class CFinalPage : public CPropertyPage
{
    DECLARE_DYNCREATE(CFinalPage)

/* Construction */
public:
    void ListViewInsert(char *CounterName, bool selected);
    void InitCounterList();
    CImageList * m_StateImageList;
    void InitListViewControl();
    LPCTSTR MakeShortString(CDC* pDC, LPCTSTR lpszLong, int nColumnLen, int nOffset);
    C256bmp m_image;
    CFinalPage();
    CFinalPage(CPropertySheet *ps);
    ~CFinalPage();

/* Dialog Data */
    //{{AFX_DATA(CFinalPage)
    enum { IDD = IDD_FINAL_PAGE };
    CComboBox	m_ChartLoc;
    CComboBox	m_ObjectList;
    CListCtrl	m_CounterList;
    //}}AFX_DATA

/* Overrides */
    /* ClassWizard generate virtual function overrides */
    //{{AFX_VIRTUAL(CFinalPage)
	public:
    virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	protected:
    virtual void DoDataExchange(CDataExchange* pDX);    /* DDX/DDV support */
	//}}AFX_VIRTUAL

/* Implementation */
protected:
    CPropertySheet * m_propertysheet;
    /* Generated message map functions */
    //{{AFX_MSG(CFinalPage)
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
    virtual BOOL OnInitDialog();
    afx_msg void OnSelchangeObjectsCombo();
    afx_msg void OnClickCounterList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKeydownCounterList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnItemchangedCounterList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnButtonSelect();
    afx_msg void OnButtonDeselect();
    afx_msg void OnButtonBrowse2();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FINALPAGE_H__2C7F9D47_5BEE_11D3_B867_AA000400CF10__INCLUDED_)
