#if !defined(AFX_EDITCOUNTER_H__36EE45F3_8B3C_11D3_B52B_A087A5000000__INCLUDED_)
#define AFX_EDITCOUNTER_H__36EE45F3_8B3C_11D3_B52B_A087A5000000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

/*
**
**  Name: EditCounter.h
**
**  Description:
**	This is the header file for the EditCounter.cpp file.
**
**  History:
**	25-oct-1999 (somsa01)
**	    Created.
*/

/*
** CEditCounter dialog
*/
class CEditCounter : public CDialog
{
/* Construction */
public:
    CString m_CounterDescription;
    CString m_CounterQuery;
    CString m_CounterDBName;
    int m_CounterScale;
    CString m_CounterName;
    CEditCounter(CWnd* pParent = NULL);   /* standard constructor */

/* Dialog Data */
    //{{AFX_DATA(CEditCounter)
    enum { IDD = IDD_EDIT_COUNTER_DIALOG };
    //}}AFX_DATA

/* Overrides */
    /* ClassWizard generated virtual function overrides */
    //{{AFX_VIRTUAL(CEditCounter)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);  /* DDX/DDV support */
    //}}AFX_VIRTUAL

/* Implementation */
protected:
    /* Generated message map functions */
    //{{AFX_MSG(CEditCounter)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EDITCOUNTER_H__36EE45F3_8B3C_11D3_B52B_A087A5000000__INCLUDED_)
