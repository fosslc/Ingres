#if !defined(AFX_SUCCESSDLG_H__C2083238_3819_11D3_B85A_AA000400CF10__INCLUDED_)
#define AFX_SUCCESSDLG_H__C2083238_3819_11D3_B85A_AA000400CF10__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

/*
**
**  Name: SuccessDlg.h
**
**  Description:
**	This is the header file for the SuccessDlg.cpp file.
**
**  History:
**	10-jul-1999 (somsa01)
**	    Created.
*/

/*
** CSuccessDlg dialog
*/
class CSuccessDlg : public CDialog
{
/* Construction */
public:
    CSuccessDlg(CWnd* pParent = NULL);   /* standard constructor */

/* Dialog Data */
    //{{AFX_DATA(CSuccessDlg)
    enum { IDD = IDD_SUCCESS_DIALOG };
	    // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA

/* Overrides */
    /* ClassWizard generated virtual function overrides */
    //{{AFX_VIRTUAL(CSuccessDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX); /* DDX/DDV support */
    //}}AFX_VIRTUAL

/* Implementation */
protected:
    /* Generated message map functions */
    //{{AFX_MSG(CSuccessDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnButtonOutfile();
    afx_msg void OnButtonErrfile();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SUCCESSDLG_H__C2083238_3819_11D3_B85A_AA000400CF10__INCLUDED_)
