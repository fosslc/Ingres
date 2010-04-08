#if !defined(AFX_ADDFILENAME_H__8EF334F5_345E_11D3_B857_AA000400CF10__INCLUDED_)
#define AFX_ADDFILENAME_H__8EF334F5_345E_11D3_B857_AA000400CF10__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

/*
**
**  Name: AddFileName.h
**
**  Description:
**	This is the header file for the AddFileName.cpp file.
**
**  History:
**	10-jul-1999 (somsa01)
**	    Created.
*/

/*
** AddFileName dialog
*/
class AddFileName : public CDialog
{
/* Construction */
public:
    AddFileName(CWnd* pParent = NULL);   /* standard constructor */

/* Dialog Data */
    //{{AFX_DATA(AddFileName)
    enum { IDD = IDD_DIALOG_FILENAME };
    //}}AFX_DATA

/* Overrides */
    /* ClassWizard generated virtual function overrides */
    //{{AFX_VIRTUAL(AddFileName)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX); /* DDX/DDV support */
    //}}AFX_VIRTUAL

/* Implementation */
protected:
    /* Generated message map functions */
    //{{AFX_MSG(AddFileName)
    virtual void OnOK();
    afx_msg void OnButtonBrowse();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDFILENAME_H__8EF334F5_345E_11D3_B857_AA000400CF10__INCLUDED_)
