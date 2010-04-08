#if !defined(AFX_WAITDLG_H__993E27F1_AB28_11D2_9FE8_006008924264__INCLUDED_)
#define AFX_WAITDLG_H__993E27F1_AB28_11D2_9FE8_006008924264__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif /* _MSC_VER >= 1000 */

/*
**
**  Name: WaitDlg.h
**
**  Description:
**	This is the header file for the WaitDlg.cpp file.
**
**  History:
**	30-mar-1999 (somsa01)
**	    Created.
*/

/*
** CWaitDlg dialog
*/
class CWaitDlg : public CDialog
{
/* Construction */
public:
    CWaitDlg(CWnd* pParent = NULL);   /* standard constructor */

/* Dialog Data */
    //{{AFX_DATA(CWaitDlg)
    enum { IDD = IDD_WAIT_DIALOG };
    //}}AFX_DATA

/* Overrides */
    //{{AFX_VIRTUAL(CWaitDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    /* DDX/DDV support */
    //}}AFX_VIRTUAL

/* Implementation */
protected:
    //{{AFX_MSG(CWaitDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnClose();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WAITDLG_H__993E27F1_AB28_11D2_9FE8_006008924264__INCLUDED_)
