/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**
**  Name: VtreeCtl.h
**
**  Description:
**	This header file declares the CVtreeCtrl class.
**
**  History:
**	15-feb-2000 (somsa01)
**	    Properly commented "VTREECTRL_HEADER".
*/

#ifndef VTREECTRL_HEADER
#define VTREECTRL_HEADER

/////////////////////////////////////////////////////////////////////////////
// CVtreeCtrl window

class CVtreeCtrl : public CTreeCtrl
{
// Construction
public:
	CVtreeCtrl();

// Attributes
public:

// Operations
public:

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

/////////////////////////////////////////////////////////////////////////////

#endif /* VTREECTRL_HEADER */
