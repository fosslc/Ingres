/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : maintab.h , Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II / Visual Manager                                            //
//    Author   : Sotheavut UK (uk$so01)                                                //
//                                                                                     //
//                                                                                     //
//    Purpose  : Tab Control that contain the pages of Right View                      //
****************************************************************************************/
/* History:
* --------
* uk$so01: (15-Dec-1998 created)
*
*
*/

#if !defined(AFX_MAINTAB_H__63A2E056_936D_11D2_A2B5_00609725DDAF__INCLUDED_)
#define AFX_MAINTAB_H__63A2E056_936D_11D2_A2B5_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "ivmdisp.h"

#define NUMBEROFPAGES   9  // (Modeless Dialog);

typedef struct tagDLGSTRUCT
{
	UINT  nIDD;
	CWnd* pPage;
} DLGSTRUCT;


class CuMainTabCtrl : public CTabCtrl
{
public:
	CuMainTabCtrl();
	UINT GetHelpID();
	void DisplayPage (CaPageInformation* pPageInfo = NULL);
	CWnd* IsPageCreated (UINT nIDD);

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuMainTabCtrl)
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CuMainTabCtrl();
	CWnd* GetCurrentPage(){return m_pCurrentPage;};
	CaIvmProperty* GetCurrentProperty (){return m_pCurrentProperty;} 

protected:
	CWnd* m_pCurrentPage;
	CaIvmProperty* m_pCurrentProperty;

	void LoadPage    (CaIvmProperty* pCurrentProperty);

private:
	DLGSTRUCT m_arrayDlg [NUMBEROFPAGES];
	CWnd* GetPage       (UINT nIDD);     // Create the page
	CWnd* ConstructPage (UINT nIDD);     // Called by GetPage
	CWnd* FormViewPage  (UINT nIDD);

	//
	// Generated message map functions
protected:
	//{{AFX_MSG(CuMainTabCtrl)
	afx_msg void OnSelchange(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	afx_msg LONG OnRemoveEvent(WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnComponentChanged (WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINTAB_H__63A2E056_936D_11D2_A2B5_00609725DDAF__INCLUDED_)
