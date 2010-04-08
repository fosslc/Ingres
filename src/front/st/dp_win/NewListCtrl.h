#if !defined(AFX_NEWLISTCTRL_H__39305DF5_D7F4_11D2_B465_6E95B8000000__INCLUDED_)
#define AFX_NEWLISTCTRL_H__39305DF5_D7F4_11D2_B465_6E95B8000000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

/*
**
**  Name: NewListCtrl.h
**
**  Description:
**	This is the header file for the NewListCtrl.cpp file.
**
**  History:
**	10-mar-1999 (somsa01)
**	    Created.
**	13-apr-1999 (somsa01)
**	    Added HitTestEx() to act like SubItemHitTest.
*/

/*
** CNewListCtrl window
*/
class CNewListCtrl : public CListCtrl
{
/* Construction */
public:
    CNewListCtrl();

/* Attributes */
public:

/* Operations */
public:

/* Overrides */
    /* ClassWizard generated virtual function overrides */
    //{{AFX_VIRTUAL(CNewListCtrl)
    //}}AFX_VIRTUAL

/* Implementation */
public:
    virtual ~CNewListCtrl();
    CEdit *EditSubLabel(int nItem, int nCol);
    int HitTestEx(CPoint &point, int *col);

    /* Generated message map functions */
protected:
    //{{AFX_MSG(CNewListCtrl)
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnEndlabeledit(NMHDR* pNMHDR, LRESULT* pResult);
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
private:

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NEWLISTCTRL_H__39305DF5_D7F4_11D2_B465_6E95B8000000__INCLUDED_)
