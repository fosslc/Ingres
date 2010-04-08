/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : ppagedit.h : header file
**  Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
**  Author   : UK Sotheavut (uk$so01)
**  Purpose  : CuPPageEdit is the property page of the sheet CuPpropertySheet.
**             It has an edit control.
**
** History :
**
** 06-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**    created
**/

#if !defined (PROPERTYPAGE_EDIT_PAGE_HEADER)
#define PROPERTYPAGE_EDIT_PAGE_HEADER
#include "rcdepend.h"

class CuPPageEdit : public CPropertyPage
{
	DECLARE_DYNCREATE(CuPPageEdit)

public:
	CuPPageEdit();
	~CuPPageEdit();
	void SetText (LPCTSTR lpszText);

	// Dialog Data
	//{{AFX_DATA(CuPPageEdit)
	enum { IDD = IDD_RCT_PROPPAGE1_TEXT };
	CEdit m_cEdit1;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CuPPageEdit)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CuPPageEdit)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};



#endif // PROPERTYPAGE_EDIT_PAGE_HEADER
