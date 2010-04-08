/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vsdappg.h : Declaration of the CVsdaPropPage property page class.
**    Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Property Page
**
** History:
**
** 26-Aug-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
*/

#if !defined(AFX_VSDAPPG_H__CC2DA2C5_B8F1_11D6_87D8_00C04F1F754A__INCLUDED_)
#define AFX_VSDAPPG_H__CC2DA2C5_B8F1_11D6_87D8_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CVsdaPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CVsdaPropPage)
	DECLARE_OLECREATE_EX(CVsdaPropPage)

	// Constructor
public:
	CVsdaPropPage();

	// Dialog Data
	//{{AFX_DATA(CVsdaPropPage)
	enum { IDD = IDD_PROPPAGE_VSDA };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

	// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	// Message maps
protected:
	//{{AFX_MSG(CVsdaPropPage)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VSDAPPG_H__CC2DA2C5_B8F1_11D6_87D8_00C04F1F754A__INCLUDED)
