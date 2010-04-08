#if !defined(AFX_SERVERSELECT_H__2C7F9D46_5BEE_11D3_B867_AA000400CF10__INCLUDED_)
#define AFX_SERVERSELECT_H__2C7F9D46_5BEE_11D3_B867_AA000400CF10__INCLUDED_

#include "256Bmp.h"

#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

/*
**
**  Name: ServerSelect.h
**
**  Description:
**	This is the header file for the ServerSelect.cpp file.
**
**  History:
**	06-sep-1999 (somsa01)
**	    Created.
**	15-oct-1999 (somsa01)
**	    The Server selection page is now the final page.
*/

/*
** CServerSelect dialog
*/
class CServerSelect : public CPropertyPage
{
    DECLARE_DYNCREATE(CServerSelect)

/* Construction */
public:
    void FillServers(HTREEITEM hParent);
    BOOL FillNodes(HTREEITEM hParent);
    void FillNodeRoot();
    CString m_ServerID;
    CImageList m_ImageList;
    C256bmp m_image;
    CServerSelect();
    CServerSelect(CPropertySheet *ps);
    ~CServerSelect();

/* Dialog Data */
    //{{AFX_DATA(CServerSelect)
    enum { IDD = IDD_SERVER_PAGE };
    CTreeCtrl	m_ServerList;
    //}}AFX_DATA

/* Overrides */
    /* ClassWizard generate virtual function overrides */
    //{{AFX_VIRTUAL(CServerSelect)
    public:
    virtual BOOL OnSetActive();
    virtual BOOL OnWizardFinish();
    virtual LRESULT OnWizardBack();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    /* DDX/DDV support */
    //}}AFX_VIRTUAL

/* Implementation */
protected:
    CPropertySheet *m_propertysheet;
    /* Generated message map functions */
    //{{AFX_MSG(CServerSelect)
    afx_msg void OnButtonRefresh();
    virtual BOOL OnInitDialog();
    afx_msg void OnItemexpandingTreeServers(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSelchangedTreeServers(NMHDR* pNMHDR, LRESULT* pResult);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SERVERSELECT_H__2C7F9D46_5BEE_11D3_B867_AA000400CF10__INCLUDED_)
