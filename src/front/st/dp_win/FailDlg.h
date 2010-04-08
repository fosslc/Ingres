#if !defined(AFX_FAILDLG_H__C2083237_3819_11D3_B85A_AA000400CF10__INCLUDED_)
#define AFX_FAILDLG_H__C2083237_3819_11D3_B85A_AA000400CF10__INCLUDED_

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
**	10-jul-1999 (somsa01)
**	    Created.
*/

/*
** CFailDlg dialog
*/
class CFailDlg : public CDialog
{
/* Construction */
public:
    CFailDlg(CWnd* pParent = NULL);   /* standard constructor */

/* Dialog Data */
    //{{AFX_DATA(CFailDlg)
    enum { IDD = IDD_FAIL_DIALOG };
    //}}AFX_DATA

/* Overrides */
    /* ClassWizard generated virtual function overrides */
    //{{AFX_VIRTUAL(CFailDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX); /* DDX/DDV support */
    //}}AFX_VIRTUAL

/* Implementation */
protected:
    /* Generated message map functions */
    //{{AFX_MSG(CFailDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnButtonErrfile();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FAILDLG_H__C2083237_3819_11D3_B85A_AA000400CF10__INCLUDED_)
