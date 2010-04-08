#if !defined(AFX_FAILDLG_H__BEEBFAFD_FC57_11D2_B842_AA000400CF10__INCLUDED_)
#define AFX_FAILDLG_H__BEEBFAFD_FC57_11D2_B842_AA000400CF10__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

/*
**
**  Name: FailDlg.h
**
**  Description:
**	This is the header file for the FailDlg.cpp file.
**
**  History:
**	27-apr-1999 (somsa01)
**	    Created.
*/

/*
** FailDlg dialog
*/
class FailDlg : public CDialog
{
/* Construction */
public:
    FailDlg(CWnd* pParent = NULL);   /* standard constructor */

/* Dialog Data */
    //{{AFX_DATA(FailDlg)
    enum { IDD = IDD_FAIL_DIALOG };
    //}}AFX_DATA

/* Overrides */
    /* ClassWizard generated virtual function overrides */
    //{{AFX_VIRTUAL(FailDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    /* DDX/DDV support */
    //}}AFX_VIRTUAL

/* Implementation */
protected:
    /* Generated message map functions */
    //{{AFX_MSG(FailDlg)
    afx_msg void OnButtonSeeErrs();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FAILDLG_H__BEEBFAFD_FC57_11D2_B842_AA000400CF10__INCLUDED_)
