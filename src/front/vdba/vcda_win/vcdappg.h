/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vcdappg.h : Declaration of the CuVcdaPropPage property page class
**    Project  : Visual Config Diff 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Property page
**
** History:
**
** 02-Oct-2002 (uk$so01)
**    SIR #109221, created
**    Created
**/

#if !defined(AFX_VCDAPPG_H__EAF345F9_D514_11D6_87EA_00C04F1F754A__INCLUDED_)
#define AFX_VCDAPPG_H__EAF345F9_D514_11D6_87EA_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CuVcdaPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CuVcdaPropPage)
	DECLARE_OLECREATE_EX(CuVcdaPropPage)

public:
	CuVcdaPropPage();

	// Dialog Data
	//{{AFX_DATA(CuVcdaPropPage)
	enum { IDD = IDD_PROPPAGE_VCDA };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

	// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	// Message maps
protected:
	//{{AFX_MSG(CuVcdaPropPage)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VCDAPPG_H__EAF345F9_D514_11D6_87EA_00C04F1F754A__INCLUDED)
