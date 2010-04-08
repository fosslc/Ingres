#if !defined(AFX_COUNTERCREATION_H__D3A65AA1_8AEB_11D3_B877_AA000400CF10__INCLUDED_)
#define AFX_COUNTERCREATION_H__D3A65AA1_8AEB_11D3_B877_AA000400CF10__INCLUDED_

#include "256bmp.h"

#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

/*
**
**  Name: CounterCreation.h
**
**  Description:
**	This is the header file for the CounterCreation.cpp file.
**
**  History:
**	25-oct-1999 (somsa01)
**	    Created.
*/

/*
** CCounterCreation dialog
*/
class CCounterCreation : public CPropertyPage
{
    DECLARE_DYNCREATE(CCounterCreation)

/* Construction */
public:
	void InitListViewControl();
	LPCTSTR MakeShortString(CDC *pDC, LPCTSTR lpszLong, int nColumnLen, int nOffset);
    C256bmp m_image;
    CCounterCreation();
    CCounterCreation(CPropertySheet *ps);
    ~CCounterCreation();

/* Dialog Data */
    //{{AFX_DATA(CCounterCreation)
	enum { IDD = IDD_COUNTER_CREATION_PAGE };
	CListCtrl	m_CounterList;
	CComboBox	m_ObjectList;
	//}}AFX_DATA

/* Overrides */
    /* ClassWizard generate virtual function overrides */
    //{{AFX_VIRTUAL(CCounterCreation)
	public:
	virtual BOOL OnSetActive();
	protected:
    virtual void DoDataExchange(CDataExchange* pDX);    /* DDX/DDV support */
	//}}AFX_VIRTUAL

/* Implementation */
protected:
    CPropertySheet * m_propertysheet;
    /* Generated message map functions */
    //{{AFX_MSG(CCounterCreation)
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeComboGroups();
	afx_msg void OnButtonCreateCounter();
	afx_msg void OnButtonEditCounter();
	afx_msg void OnClickPersonalCounterList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkPersonalCounterList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKeydownPersonalCounterList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonDeleteCounter();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COUNTERCREATION_H__D3A65AA1_8AEB_11D3_B877_AA000400CF10__INCLUDED_)
