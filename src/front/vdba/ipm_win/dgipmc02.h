/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgipmc02.h, Header file 
**    Project  : CA-OpenIngres/Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Modeless Dialog Box that will appear at the right pane of the 
**               Ipm Frame.
**               It contains a TabCtrl and maintains the pages (Modeless Dialogs)
**               that will appeare at each tab. It also keeps track of the current 
**               object being displayed. When we get the new object to display,
**               we check if it is the same Class of the Current Object for the 
**               purpose of just updating the data of the current page.
** History:
**
** xx-Feb-1997 (uk$so01)
**    Created
**/

#ifndef DGIPMTAB_HEADER
#define DGIPMTAB_HEADER
#include "ipmdisp.h"
#include "titlebmp.h" 

#define NUMBEROFPAGES   (38+11) // 49 Pages (38 Modeless Dialog boxes and 11 FormView);
typedef struct tagDLGSTRUCT
{
	UINT    nIDD;
	CWnd*   pPage;
} DLGSTRUCT;

class CuDlgIpmTabCtrl : public CDialog
{
public:
	CuDlgIpmTabCtrl(CWnd* pParent = NULL);
	void DisplayPage (CuPageInformation* pPageInfo, LPCTSTR lpszHeader = NULL, int nSel = -1);
	void LoadPage    (CuIpmProperty* pCurrentProperty);
	CuIpmPropertyData* GetDialogData();
	CuIpmProperty* GetCurrentProperty (){return m_pCurrentProperty;} 
	UINT GetCurrentPageID();
	CWnd* GetPage (UINT nIDD);     // Create the page
	void NotifyPageSelChanging();  // Notify page of selchanging in left tree
	void NotifyPageDocClose();     // Notify page of document closing
	CWnd* GetCurrentPage(){return m_pCurrentPage;}
	void SelectPage (int nPage);
	// Dialog Data
	//{{AFX_DATA(CuDlgIpmTabCtrl)
	enum { IDD = IDD_IPMCAT02 };
	CTabCtrl    m_cTab1;
	//}}AFX_DATA
	CuTitleBitmap m_staticHeader;
	CWnd*   m_pCurrentPage;
	
	void OnCancel() {return;}
	void OnOK()     {return;}
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmTabCtrl)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL
	
	// Implementation
private:
	BOOL m_bIsLoading;
	DLGSTRUCT m_arrayDlg [NUMBEROFPAGES];
	CWnd* ConstructPage (UINT nIDD);     // Called by GetPage
	//
	// Construct the Individual Page 

	CWnd* Page_ActiveDB          (UINT nIDD);
	CWnd* Page_LockedResources   (UINT nIDD);
	CWnd* Page_LockedTables      (UINT nIDD);
	CWnd* Page_LockLists         (UINT nIDD);
	CWnd* Page_Locks             (UINT nIDD);
	CWnd* Page_LogFile           (UINT nIDD);
	CWnd* Page_Processes         (UINT nIDD);
	CWnd* Page_Sessions          (UINT nIDD);
	CWnd* Page_Transactions      (UINT nIDD);
	
	CWnd* Page_RepStaticActivity (UINT nIDD);
	CWnd* Page_RepStaticEvent    (UINT nIDD);
	CWnd* Page_RepStaticDistrib  (UINT nIDD);
	CWnd* Page_RepStaticCollision(UINT nIDD);
	CWnd* Page_RepStaticServer   (UINT nIDD);
	CWnd* Page_RepStaticIntegerity(UINT nIDD);
	
	CWnd* Page_RepServerStatus   (UINT nIDD);
	CWnd* Page_RepServerStartupSetting(UINT nIDD);
	CWnd* Page_RepServerEvent    (UINT nIDD);
	CWnd* Page_RepServerAssignment(UINT nIDD);

	// Pages on root static items
	CWnd* Page_StaticServers(UINT nIDD);
	CWnd* Page_StaticDatabases(UINT nIDD);
	CWnd* Page_StaticActiveusers(UINT nIDD);
	CWnd* Page_StaticReplication(UINT nIDD);

	//
	// Ice (Pages + Detail)
	CWnd* IcePage_ActiveUser    (UINT nIDD);
	CWnd* IcePage_CachePage     (UINT nIDD);
	CWnd* IcePage_ConnectedUser (UINT nIDD);
	CWnd* IcePage_Cursor        (UINT nIDD);
	CWnd* IcePage_DatabaseConnection (UINT nIDD);
	CWnd* IcePage_HttpServerConnection (UINT nIDD);
	CWnd* IcePage_Transaction   (UINT nIDD);

protected:
	CWnd* CreateFormViewPage (UINT nIDD);
	CuIpmProperty* m_pCurrentProperty;
	BOOL ChangeTab();


	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmTabCtrl)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	afx_msg void OnSelchangingMfcTab1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData   (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnHideProperty (WPARAM wParam, LPARAM lParam);
	afx_msg long OnSettingChange(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};
#endif
