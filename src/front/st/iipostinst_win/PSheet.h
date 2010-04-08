/*
** Copyright (c) 2001 Ingres Corporation
*/

/*
**
**  Name: PSheet.h
**
**  Description:
**      This is the header file for the PSheet.cpp file.
**
**  History:
**      10-apr-2001 (somsa01)
**          Created.
**	13-jul-2001 (somsa01)
**	    Removed obsolete function FindPage(), and
**	    removed OnClose() and OnCommand().
**  15-Nov-2006 (drivi01)
**      SIR 116599
**      Enhanced post-installer in effort to improve installer usability.
*/

/*
** CPropSheet
*/
class CPropSheet : public CPropertySheet
{
    DECLARE_DYNAMIC(CPropSheet)

/* Construction */
public:
    CPropSheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

/* Attributes */
public:

/* Operations */
public:

/* Overrides */
    /* ClassWizard generated virtual function overrides */
    //{{AFX_VIRTUAL(CPropSheet)
    public:
    virtual BOOL OnInitDialog();
    protected:
    //}}AFX_VIRTUAL

/* Implementation */
public:
    void HideButtons();
    virtual ~CPropSheet();
    /* Generated message map functions */
protected:
    //{{AFX_MSG(CPropSheet)
    afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
	afx_msg BOOL OnCommand(WPARAM, LPARAM);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
private:

};
