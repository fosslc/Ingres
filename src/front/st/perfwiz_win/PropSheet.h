#if !defined(AFX_PROPSHEET1_H__C1EF42F2_5BEC_11D3_B867_AA000400CF10__INCLUDED_)
#define AFX_PROPSHEET1_H__C1EF42F2_5BEC_11D3_B867_AA000400CF10__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

/*
**
**  Name: PropSheet.h
**
**  Description:
**	This is the header file for the PropSheet.cpp file.
**
**  History:
**	06-sep-1999 (somsa01)
**	    Created.
*/

/*
** PropSheet
*/
class PropSheet : public CPropertySheet
{
    DECLARE_DYNAMIC(PropSheet)

/* Construction */
public:
    PropSheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
    PropSheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

/* Attributes */
public:

/* Operations */
public:

/* Overrides */
    /* ClassWizard generated virtual function overrides */
    //{{AFX_VIRTUAL(PropSheet)
    //}}AFX_VIRTUAL

/* Implementation */
public:
    virtual ~PropSheet();

    /* Generated message map functions */
protected:
    //{{AFX_MSG(PropSheet)
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPSHEET1_H__C1EF42F2_5BEC_11D3_B867_AA000400CF10__INCLUDED_)
