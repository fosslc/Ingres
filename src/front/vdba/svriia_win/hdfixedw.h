/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : hdfixedw.h : Header file
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Header control.
**
** History:
**
** 19-Dec-2000 (uk$so01)
**    Created
**/

#if !defined(AFX_HDFIXEDW_H__8D741560_DBD9_11D4_8739_00C04F1F754A__INCLUDED_)
#define AFX_HDFIXEDW_H__8D741560_DBD9_11D4_8739_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CuHeaderFixedWidth : public CHeaderCtrl
{
public:
	CuHeaderFixedWidth();

public:

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuHeaderFixedWidth)
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CuHeaderFixedWidth();

	// Generated message map functions
protected:
	//{{AFX_MSG(CuHeaderFixedWidth)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HDFIXEDW_H__8D741560_DBD9_11D4_8739_00C04F1F754A__INCLUDED_)
