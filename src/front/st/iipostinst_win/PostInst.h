/*
** Copyright (c) 1998, 2001 Ingres Corporation
*/

#if !defined(AFX_POSTINST_H__00583ED1_E911_11D0_B7E1_00805FEAA579__INCLUDED_)
#define AFX_POSTINST_H__00583ED1_E911_11D0_B7E1_00805FEAA579__INCLUDED_

#include "256Bmp.h"

#if _MSC_VER >= 1000
#pragma once
#endif /* _MSC_VER >= 1000 */

/*
**
**  Name: PostInst.h
**
**  Description:
**      This is the header file for the PostInst.cpp file.
**
**  History:
**      30-jul-1998 (mcgem01)
**          Created.
**	10-apr-2001 (somsa01)
**	    Removed anything not needed by the post installation DLL.
**	15-jun-2001 (somsa01)
**	    Re-added the running of the clock.
**	06-mar-2002 (penga03)
**	    Added m_msg.
**  15-Nov-2006 (drivi01)
**      SIR 116599
**      Enhanced post-installer in effort to improve installer usability.
*/

/*
** CPostInst dialog
*/
class CPostInst : public CPropertyPage
{
    DECLARE_DYNCREATE(CPostInst)
	
/* Construction */
public:
    UINT m_uiAVI_Resource;
    CPostInst();
    ~CPostInst();
    
    /* Dialog Data */
    //{{AFX_DATA(CPostInst)
    enum { IDD = IDD_POSTINSTALLATION };
    CStatic	m_output;
    C256bmp	m_image;
	CStatic	m_msg;
    //}}AFX_DATA

    /* Overrides */
    /* ClassWizard generate virtual function overrides */
    //{{AFX_VIRTUAL(CPostInst)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    /* DDX/DDV support */
    //}}AFX_VIRTUAL
    
/* Implementation */
protected:
	/* Generated message map functions */
    //{{AFX_MSG(CPostInst)
    virtual BOOL OnInitDialog();
    afx_msg void OnTimer(UINT nIDEvent);
	afx_msg LRESULT OnWizardNext();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
	
private:
    void RemoveCAInstallerCrap();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_POSTINST_H__00583ED1_E911_11D0_B7E1_00805FEAA579__INCLUDED_)
