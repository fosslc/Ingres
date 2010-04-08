#if !defined(AFX_SPLASH_H__14827DEE_E8AB_11D0_B7E1_00805FEAA579__INCLUDED_)
#define AFX_SPLASH_H__14827DEE_E8AB_11D0_B7E1_00805FEAA579__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif /* _MSC_VER >= 1000 */

/*
**
**  Name: Splash.h
**
**  Description:
**	This is the header file for the Splash.cpp file.
**
**  History:
**	??-???-???? (mcgem01)
**	    Created.
*/

#include "256bmp.h"

/*
** CSplash dialog
*/
class CSplash : public CDialog
{
/* Construction */
public:
    CSplash(CWnd* pParent = NULL);   /* standard constructor */

/* Dialog Data */
    //{{AFX_DATA(CSplash)
    enum { IDD = IDD_SPLASH };
    C256bmp	m_image;
    //}}AFX_DATA

/* Overrides */
    /* ClassWizard generated virtual function overrides */
    //{{AFX_VIRTUAL(CSplash)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    /* DDX/DDV support */
    //}}AFX_VIRTUAL

/* Implementation */
protected:
    /* Generated message map functions */
    //{{AFX_MSG(CSplash)
    virtual BOOL OnInitDialog();
    afx_msg void OnTimer(UINT nIDEvent);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPLASH_H__14827DEE_E8AB_11D0_B7E1_00805FEAA579__INCLUDED_)
