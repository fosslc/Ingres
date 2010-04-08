/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : ijactppg.h , Header File
**    Project  : Ingres Journal Analyser
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Implementation of the CIjaCtrlPropPage property page class 

** History:
**
** 22-Dec-1999 (uk$so01)
**    Created
**
**/

#if !defined(AFX_IJACTRLPPG_H__C92B8437_B176_11D3_A322_00C04F1F754A__INCLUDED_)
#define AFX_IJACTRLPPG_H__C92B8437_B176_11D3_A322_00C04F1F754A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// IjaCtrlPpg.h : Declaration of the CIjaCtrlPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CIjaCtrlPropPage : See IjaCtrlPpg.cpp.cpp for implementation.

class CIjaCtrlPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CIjaCtrlPropPage)
	DECLARE_OLECREATE_EX(CIjaCtrlPropPage)

public:
	CIjaCtrlPropPage();

	// Dialog Data
	//{{AFX_DATA(CIjaCtrlPropPage)
	enum { IDD = IDD_PROPPAGE_IJACTRL };
	BOOL	m_bPaintForegroundTransaction;
	//}}AFX_DATA
	

	// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	// Message maps
protected:
	//{{AFX_MSG(CIjaCtrlPropPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IJACTRLPPG_H__C92B8437_B176_11D3_A322_00C04F1F754A__INCLUDED)
