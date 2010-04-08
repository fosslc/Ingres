/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : psheet.h : header file
**  Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
**  Author   : UK Sotheavut (uk$so01)
**  Purpose  : CuPropertySheet is a modeless property sheet that is 
**             created once and not destroyed until the owner is being destroyed.
**             It is initialized and controlled from CfMiniFrame.
**
** History :
**
** 06-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**    created
** 07-Mar-2003 (uk$so01)
**    SIR #109773, Add property frame for displaying the description 
**    of ingres messages.
**/

#if !defined (MINIFRAME_PROPERTY_SHEET_HEADER)
#define MINIFRAME_PROPERTY_SHEET_HEADER
#include "ppgedit.h"


class CuPropertySheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CuPropertySheet)

public:
	CuPropertySheet(CWnd* pWndParent = NULL);
	CuPropertySheet(CWnd* pWndParent, CTypedPtrList < CObList, CPropertyPage* >& listPages);

	CuPPageEdit m_Page1;
	CStatic m_cMsg;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuPropertySheet)
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CuPropertySheet();
	virtual void PostNcDestroy();

	// Generated message map functions
protected:
	//{{AFX_MSG(CuPropertySheet)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	afx_msg long OnUpdateData(WPARAM w, LPARAM l);
	DECLARE_MESSAGE_MAP()
};


#endif // MINIFRAME_PROPERTY_SHEET_HEADER
