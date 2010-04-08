#if !defined(AFX_EDITOBJECT_H__ABD2B5E0_5E70_11D3_B867_AA000400CF10__INCLUDED_)
#define AFX_EDITOBJECT_H__ABD2B5E0_5E70_11D3_B867_AA000400CF10__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

/*
**
**  Name: EditObject.h
**
**  Description:
**	This is the header file for the EditObject.cpp file.
**
**  History:
**	06-sep-1999 (somsa01)
**	    Created.
*/

/*
** CEditObject dialog
*/
class CEditObject : public CDialog
{
/* Construction */
public:
    CString m_ObjectDescription;
    CString m_ObjectName;
    CEditObject(CWnd* pParent = NULL);   /* standard constructor */

/* Dialog Data */
    //{{AFX_DATA(CEditObject)
    enum { IDD = IDD_EDIT_OBJ_DIALOG };
    //}}AFX_DATA

/* Overrides */
    /* ClassWizard generated virtual function overrides */
    //{{AFX_VIRTUAL(CEditObject)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    /* DDX/DDV support */
    //}}AFX_VIRTUAL

/* Implementation */
protected:
    /* Generated message map functions */
    //{{AFX_MSG(CEditObject)
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EDITOBJECT_H__ABD2B5E0_5E70_11D3_B867_AA000400CF10__INCLUDED_)
