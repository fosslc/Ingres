/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : xdlgpref.h , Header File                                              //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II / Visual Manager                                            //
//    Author   : Sotheavut UK (uk$so01)                                                //
//                                                                                     //
//                                                                                     //
//    Purpose  : The Preferences setting of Visual Manager                             //
****************************************************************************************/
/*
** History:
**
** 05-Feb-1999 (uk$so01)
**     Created
** 27-Jan-2000 (uk$so01)
**     (Bug #100157): Activate Help Topic
**
**/

#if !defined(AFX_XDLGPREF_H__58793FC2_B9D3_11D2_A2CE_00609725DDAF__INCLUDED_)
#define AFX_XDLGPREF_H__58793FC2_B9D3_11D2_A2CE_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "dlgepref.h"

class CxDlgPropertySheetPreference : public CPropertySheet
{
	DECLARE_DYNAMIC(CxDlgPropertySheetPreference)

public:
	CxDlgPropertySheetPreference(CWnd* pWndParent = NULL);
	UINT GetHelpID() {return 0;}


//	CuDlgPropertyPageEventDisplay m_Page1;
	CuDlgPropertyPageEventNotify m_Page2;

public:

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgPropertySheetPreference)
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CxDlgPropertySheetPreference();

	// Generated message map functions
protected:
	//{{AFX_MSG(CxDlgPropertySheetPreference)
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XDLGPREF_H__58793FC2_B9D3_11D2_A2CE_00609725DDAF__INCLUDED_)
