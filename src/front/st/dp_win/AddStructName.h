#if !defined(AFX_ADDSTRUCTNAME_H__C72537FB_34ED_11D3_B857_AA000400CF10__INCLUDED_)
#define AFX_ADDSTRUCTNAME_H__C72537FB_34ED_11D3_B857_AA000400CF10__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

/*
**
**  Name: AddStructName.h
**
**  Description:
**	This is the header file for the AddStructName.cpp file.
**
**  History:
**	10-jul-1999 (somsa01)
**	    Created.
*/

/*
** AddStructName dialog
*/
class AddStructName : public CDialog
{
/* Construction */
public:
    AddStructName(CWnd* pParent = NULL);   /* standard constructor */

/* Dialog Data */
    //{{AFX_DATA(AddStructName)
    enum { IDD = IDD_DIALOG_TBNAME };
    //}}AFX_DATA

/* Overrides */
    /* ClassWizard generated virtual function overrides */
    //{{AFX_VIRTUAL(AddStructName)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX); /* DDX/DDV support */
    //}}AFX_VIRTUAL

/* Implementation */
protected:
    /* Generated message map functions */
    //{{AFX_MSG(AddStructName)
    virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDSTRUCTNAME_H__C72537FB_34ED_11D3_B857_AA000400CF10__INCLUDED_)
