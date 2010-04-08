/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**
**  Name: vtreectl.h
**  Author   : Emmanuel Blattes 
**  Description: This header file declares the CVtreeCtrl class.
**
** History:
**
** xx-Xxx-1997 (Emmanuel Blattes)
**    Created
** 15-feb-2000 (somsa01)
**    Properly commented "VTREECTRL_HEADER".
*/

#ifndef VTREECTRL_HEADER
#define VTREECTRL_HEADER


class CVtreeCtrl : public CTreeCtrl
{
public:
	CVtreeCtrl();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVtreeCtrl)
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CVtreeCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CVtreeCtrl)
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};


#endif /* VTREECTRL_HEADER */
