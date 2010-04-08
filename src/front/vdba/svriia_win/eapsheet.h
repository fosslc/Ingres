/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : eapsheet.h : header file
**    Project  : EXPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : This class defines custom modal property sheet 
**                CuIEAPropertySheet.
**
** History:
**
** 15-Oct-2001 (uk$so01)
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
**    Created
**/


#if !defined (EAPSHEET_HEADER)
#define EAPSHEET_HEADER
#include "svriia.h"
#include "eappage1.h"
#include "eappage2.h" // CSV
#include "eappagc2.h" // All others
#include "impexpas.h"



class CuIEAPropertySheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CuIEAPropertySheet)

public:
	CuIEAPropertySheet(CWnd* pWndParent = NULL);
	CaIEAInfo& GetData(){return m_data;}
	IEASTRUCT* GetInputParam(){return m_pStruct;}
public:
	CuIeaPPage1 m_Page1;
	CuIeaPPage2 m_Page2Csv;
	CuIeaPPage2Common m_Page2Common;

	virtual BOOL OnInitDialog();
	int  DoModal();
	void SetPreviousPage (int nPage){m_PreviousPage = nPage;}
	int  GetPreviousPage (){return m_PreviousPage;}

	void SetInputParam(IEASTRUCT* pStruct){m_pStruct = pStruct;}
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuIEAPropertySheet)
	protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	IEASTRUCT* m_pStruct;
	CaIEAInfo m_data;
	int  m_PreviousPage;

public:
	virtual ~CuIEAPropertySheet();

	//
	// Generated message map functions
protected:
	//{{AFX_MSG(CuIEAPropertySheet)
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg void OnHelp();
	DECLARE_MESSAGE_MAP()
};


#endif // EAPSHEET_HEADER
