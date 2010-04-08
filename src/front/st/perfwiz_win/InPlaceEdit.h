#if !defined(AFX_INPLACEEDIT_H__B50669A7_D299_11D2_B45E_24CA7D000000__INCLUDED_)
#define AFX_INPLACEEDIT_H__B50669A7_D299_11D2_B45E_24CA7D000000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

/*
**
**  Name: InPlaceEdit.h
**
**  Description:
**	This is the header file for the InPlaceEdit.cpp file.
**
**  History:
**	05-mar-1999 (somsa01)
**	    Created.
*/

/*
** CInPlaceEdit window
*/
class CInPlaceEdit : public CEdit
{
/* Construction */
public:
    CInPlaceEdit(int iItem, int iSubItem, CString sInitText, BOOL NeedSpin);

/* Attributes */
public:

/* Operations */
public:

/* Overrides */
    /* ClassWizard generated virtual function overrides */
    //{{AFX_VIRTUAL(CInPlaceEdit)
    public:
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    //}}AFX_VIRTUAL

/* Implementation */
public:
    virtual ~CInPlaceEdit();

    /* Generated message map functions */
protected:
    //{{AFX_MSG(CInPlaceEdit)
    afx_msg void OnKillFocus(CWnd* pNewWnd);
    afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnNcDestroy();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
private:
    BOOL m_bESC;
    BOOL m_NeedSpin;
    CString m_sInitText;
    int m_iSubItem;
    int m_iItem;
    CSpinButtonCtrl *pSpin;
};

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INPLACEEDIT_H__B50669A7_D299_11D2_B45E_24CA7D000000__INCLUDED_)
