#if !defined(AFX_CHOOSEDIR_H__2F20D7F1_0298_11D1_B7EA_00805FEAA579__INCLUDED_)
#define AFX_CHOOSEDIR_H__2F20D7F1_0298_11D1_B7EA_00805FEAA579__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif /*_MSC_VER >= 1000 */

/*
**
**  Name: ChooseDir.h
**
**  Description:
**	This is the header file for the ChooseDir.cpp file.
**
**  History:
**	23-apr-1999 (somsa01)
**	    Created from Enterprise Installer.
*/

/*
** CChooseDir dialog
*/
class CChooseDir : public CDialog
{
/* Construction */
public:
    void FillDrives();
    ~CChooseDir();
    CImageList m_imageList;
    BOOL HasSubDirs(HTREEITEM hitem);
    void GetCurPath(CString & path,HTREEITEM hitem);
    void FillDir(HTREEITEM hparent);
    CString m_path;
    CString m_title;
    CChooseDir(CString &s,CWnd* pParent = NULL);   // standard constructor
    HTREEITEM FindInTree(HTREEITEM hRootItem, char *szFindText);

/* Dialog Data */
    //{{AFX_DATA(CChooseDir)
    enum { IDD = IDD_CHOOSEDIRS };
    CTreeCtrl	m_dirs;
    CEdit	m_edit;
    //}}AFX_DATA

/* Overrides */
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CChooseDir)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

/* Implementation */
protected:
    /* Generated message map functions */
    //{{AFX_MSG(CChooseDir)
    virtual BOOL OnInitDialog();
    afx_msg void OnItemexpandingDirs(NMHDR* pNMHDR, LRESULT* pResult);
    virtual void OnOK();
    afx_msg void OnSelchangedDirs(NMHDR* pNMHDR, LRESULT* pResult);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CHOOSEDIR_H__2F20D7F1_0298_11D1_B7EA_00805FEAA579__INCLUDED_)
