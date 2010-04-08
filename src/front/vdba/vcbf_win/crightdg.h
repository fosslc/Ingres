/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/* 
**    Source   : crightdg.h, Header file
**    Project  : OpenIngres Configuration Manager (Origin IPM Project)
**    Author   : UK Sotheavut 
**               Blatte Emannuel (Adds additional pages, Custom Implementations) 
**    Purpose  : Modeless Dialog Box that will appear at the right pane of the 
**               page Configure 
**               It contains a TabCtrl and maintains the pages (Modeless Dialogs)
**               that will appeare at each tab. It also keeps track of the current
**               object being displayed. When we get the new object to display,
**               we check if it is the same Class of the Current Object for the 
**               purpose of just updating the data of the current page.
**
** History:
** xx-Sep-1997 (uk$so01)
**    Created.
** 01-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
** 20-jun-2003 (uk$so01)
**    SIR #110389 / INGDBA210
**    Add functionalities for configuring the GCF Security and
**    the security mechanisms.
** 25-Sep-2003 (uk$so01)
**    SIR #105781 of cbf
** 03-Nov-2003 (noifr01)
**     fixed errors from massive ingres30->main gui tools source propagation
** 12-Mar-2004 (uk$so01)
**    SIR #110013 (Handle the DAS Server)
** 06-May-2007 (drivi01)
**    SIR #118462
**    Increase number of pages to +1 (50->51)
**    due to the fact that we added IDD_GW_DB2UDB_PAGE_PARAMETER
**    to the list of dialogs.
*/

#if !defined(AFX_CRIGHTDG_H__05328421_2516_11D1_A1EE_00609725DDAF__INCLUDED_)
#define AFX_CRIGHTDG_H__05328421_2516_11D1_A1EE_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// crightdg.h : header file
//
#include "cbfdisp.h"
#include "dlgvfrm.h"
#define NUMBEROFPAGES   51  // (Form View + Modeless Dialog);

typedef struct tagDLGSTRUCT
{
	UINT    nIDD;
	CWnd*   pPage;
} DLGSTRUCT;


/////////////////////////////////////////////////////////////////////////////
// CConfRightDlg dialog

class CConfRightDlg : public CDialog
{
public:
	CConfRightDlg(CWnd* pParent = NULL);   // standard constructor
	void OnOK() {return;};
	void OnCancel() {return;};

	void DisplayPage (CuPageInformation* pPageInfo);
	void LoadPage    (CuCbfProperty* pCurrentProperty);
	CuCbfPropertyData* GetDialogData();
	CuCbfProperty* GetCurrentProperty (){return m_pCurrentProperty;} 
	UINT  GetCurrentPageID();
	CWnd* GetCurrentPage(){return m_pCurrentPage;};

	// Dialog Data
	//{{AFX_DATA(CConfRightDlg)
	enum { IDD = IDD_CONFIG_RIGHT };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	CTabCtrl    m_cTab1;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfRightDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

//
// Implementation
//
private:
	BOOL m_bIsLoading;
	DLGSTRUCT m_arrayDlg [NUMBEROFPAGES];
	CWnd* GetPage       (UINT nIDD);     // Create the page
	CWnd* ConstructPage (UINT nIDD);     // Called by GetPage
	CWnd* FormViewPage  (UINT nIDD);

protected:
	CWnd*   m_pCurrentPage;
	CuCbfProperty* m_pCurrentProperty;

	// Generated message map functions
	//{{AFX_MSG(CConfRightDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButton1();
	afx_msg void OnButton2();
	afx_msg void OnButton3();
	afx_msg void OnButton4();
	afx_msg void OnButton5();
	//}}AFX_MSG
	afx_msg LONG OnUpdateData   (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnValidateData (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CRIGHTDG_H__05328421_2516_11D1_A1EE_00609725DDAF__INCLUDED_)




