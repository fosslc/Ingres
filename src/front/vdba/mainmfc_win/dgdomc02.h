/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dgdomc02.h
**    Project  : Ingres VisualDBA
**
**  27-Mar-2001 (noifr01)
**   (sir 104270) removal of code for managing Ingres/Desktop
** 30-May-2001 (uk$so01)
**    SIR #104736, Integrate IIA and Cleanup old code of populate.
** 21-Mar-2002 (uk$so01)
**    SIR #106648, Integrate ipm.ocx.
** 02-Apr-2003 (schph01)
**    SIR #107523, new functions for Sequences.
**/

#ifndef DGDOMTAB_HEADER
#define DGDOMTAB_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DgDomC02.h : header file
//

#include "ipmdisp.h"
#include "dompginf.h"
#include "titlebmp.h" 

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomTabCtrl dialog

#define NUMBEROFDOMPAGES   129 // number of Modeless Dialog boxes for dom right pane

typedef struct
{
    UINT    nIDD;
    CWnd*   pPage;
} DOMDETAILDLGSTRUCT;

enum {GETDIALOGDATA = 0L, GETDIALOGDATAWITHDUP};

class CuDlgDomTabCtrl : public CDialog
{
// Construction
public:
	CuDlgDomTabCtrl(CWnd* pParent = NULL);   // standard constructor
  void DisplayPage (CuDomPageInformation* pPageInfo, BOOL bUpdate = TRUE, int initialSel = 0);
  void LoadPage    (CuDomProperty* pCurrentProperty);
  void NotifyPageSelChanging(BOOL bUpdate);

  // Changed from protected to public
  CWnd* GetPage       (UINT nIDD);     // Create the page
  CWnd* GetCreatedPage(UINT nIDD);     // Return NULL if the page has not been created.

  CuIpmPropertyData* GetDialogData();
  CuIpmPropertyData* GetDialogDataWithDup();  // for new window/tear out
  CuDomProperty* GetCurrentProperty (){return m_pCurrentProperty;} 
  UINT GetCurrentPageID();
  UINT GetCurrentHelpID();
  void UpdateBitmapTitle(HICON hIcon);

  // Utility for construction of scrollable property page
  CWnd* DomPropPage_GenericWithScrollbars(UINT nIDD);

  // Construction of the property pages
  CWnd* DomPropPage_Table(UINT nIDD);
  CWnd* DomPropPage_Location(UINT nIDD);
  CWnd* DomPropPage_User(UINT nIDD);
  CWnd* DomPropPage_Role(UINT nIDD);
  CWnd* DomPropPage_Profile(UINT nIDD);
  CWnd* DomPropPage_Index(UINT nIDD);
  CWnd* DomPropPage_Integrity(UINT nIDD);
  CWnd* DomPropPage_Rule(UINT nIDD);
  CWnd* DomPropPage_Procedure(UINT nIDD);
  CWnd* DomPropPage_Sequence(UINT nIDD);
  CWnd* DomPropPage_Group(UINT nIDD);
  CWnd* DomPropPage_DbAlarms(UINT nIDD);
  CWnd* DomPropPage_TableAlarms(UINT nIDD);
  CWnd* DomPropPage_TableGrantees(UINT nIDD);
  CWnd* DomPropPage_DbGrantees(UINT nIDD);
  CWnd* DomPropPage_Db(UINT nIDD);
  CWnd* DomPropPage_ViewGrantees(UINT nIDD);
  CWnd* DomPropPage_View(UINT nIDD);
  CWnd* DomPropPage_Cdds(UINT nIDD);
  CWnd* DomPropPage_CddsTables(UINT nIDD);
  CWnd* DomPropPage_LocationSpace(UINT nIDD);
  CWnd* DomPropPage_LocationDbs(UINT nIDD);
  CWnd* DomPropPage_DbEventGrantees(UINT nIDD);
  CWnd* DomPropPage_ProcGrantees(UINT nIDD);
  CWnd* DomPropPage_SeqGrantees(UINT nIDD);
  CWnd* DomPropPage_TableColumns(UINT nIDD);

  CWnd* DomPropPage_ReplicCdds(UINT nIDD);
  CWnd* DomPropPage_ReplicConn(UINT nIDD);
  CWnd* DomPropPage_ReplicMailU(UINT nIDD);
  CWnd* DomPropPage_ReplicRegTbl(UINT nIDD);

  CWnd* DomPropPage_DbTbls(UINT nIDD);
  CWnd* DomPropPage_DbViews(UINT nIDD);
  CWnd* DomPropPage_DbProcs(UINT nIDD);
  CWnd* DomPropPage_DbSeqs(UINT nIDD);
  CWnd* DomPropPage_DbSchemas(UINT nIDD);
  CWnd* DomPropPage_DbSynonyms(UINT nIDD);
  CWnd* DomPropPage_DbEvents(UINT nIDD);

  CWnd* DomPropPage_TableInteg(UINT nIDD);
  CWnd* DomPropPage_TableRule(UINT nIDD);
  CWnd* DomPropPage_TableIdx(UINT nIDD);
  CWnd* DomPropPage_TableLoc(UINT nIDD);
  CWnd* DomPropPage_TableSyn(UINT nIDD);
  CWnd* DomPropPage_TableRows(UINT nIDD);
  CWnd* DomPropPage_TableStatistic(UINT nIDD);

  CWnd* DomPropPage_UserXAlarms(UINT nIDD);

  CWnd* DomPropPage_UserXGrantedDb(UINT nIDD);
  CWnd* DomPropPage_UserXGrantedTbl(UINT nIDD);
  CWnd* DomPropPage_UserXGrantedView(UINT nIDD);
  CWnd* DomPropPage_UserXGrantedEvent(UINT nIDD);
  CWnd* DomPropPage_UserXGrantedProc(UINT nIDD);
  CWnd* DomPropPage_UserXGrantedRole(UINT nIDD);
  CWnd* DomPropPage_UserXGrantedSeq(UINT nIDD);

  CWnd* DomPropPage_GroupXGrantedDb(UINT nIDD);
  CWnd* DomPropPage_GroupXGrantedTbl(UINT nIDD);
  CWnd* DomPropPage_GroupXGrantedView(UINT nIDD);
  CWnd* DomPropPage_GroupXGrantedEvent(UINT nIDD);
  CWnd* DomPropPage_GroupXGrantedProc(UINT nIDD);
  CWnd* DomPropPage_GroupXGrantedSeq(UINT nIDD);

  CWnd* DomPropPage_RoleXGrantedDb(UINT nIDD);
  CWnd* DomPropPage_RoleXGrantedTbl(UINT nIDD);
  CWnd* DomPropPage_RoleXGrantedView(UINT nIDD);
  CWnd* DomPropPage_RoleXGrantedEvent(UINT nIDD);
  CWnd* DomPropPage_RoleXGrantedProc(UINT nIDD);
  CWnd* DomPropPage_RoleXGrantedSeq(UINT nIDD);

  CWnd* DomPropPage_Locations(UINT nIDD);

  CWnd* DomPropPage_Connection(UINT nIDD);

  CWnd* DomPropPage_DDb(UINT nIDD);
  CWnd* DomPropPage_DDbTbls(UINT nIDD);
  CWnd* DomPropPage_DDbViews(UINT nIDD);
  CWnd* DomPropPage_DDbProcs(UINT nIDD);

  CWnd* DomPropPage_StarProcLink(UINT nIDD);
  CWnd* DomPropPage_StarViewLink(UINT nIDD);
  CWnd* DomPropPage_StarViewNative(UINT nIDD);
  CWnd* DomPropPage_StarTableLinkIndexes(UINT nIDD);

  CWnd* DomPropPage_SchemaTbls(UINT nIDD);
  CWnd* DomPropPage_SchemaViews(UINT nIDD);
  CWnd* DomPropPage_SchemaProcs(UINT nIDD);

  CWnd* DomPropPage_TablePages(UINT nIDD);
  CWnd* DomPropPage_IndexPages(UINT nIDD);

  CWnd* DomPropPage_StarTableLink(UINT nIDD);
  CWnd* DomPropPage_StarDbComponent(UINT nIDD);

  CWnd* DomPropPage_StarIndexLinkSource(UINT nIDD);
  CWnd* DomPropPage_StarProcLinkSource(UINT nIDD);
  CWnd* DomPropPage_StarViewLinkSource(UINT nIDD);
  CWnd* DomPropPage_StarViewNativeSource(UINT nIDD);

  CWnd* DomPropPage_TableRowsNA(UINT nIDD);

  // Static root items
  CWnd* DomPropPage_StaticDb(UINT nIDD);
  CWnd* DomPropPage_StaticProfile(UINT nIDD);
  CWnd* DomPropPage_StaticUser(UINT nIDD);
  CWnd* DomPropPage_StaticGroup(UINT nIDD);
  CWnd* DomPropPage_StaticRole(UINT nIDD);
  CWnd* DomPropPage_StaticLocation(UINT nIDD);
  CWnd* DomPropPage_StaticGwDb(UINT nIDD);

  CWnd* DomPropPage_IceDbconnection(UINT nIDD);
  CWnd* DomPropPage_IceDbuser(UINT nIDD);
  CWnd* DomPropPage_IceProfile(UINT nIDD);
  CWnd* DomPropPage_IceRole(UINT nIDD);
  CWnd* DomPropPage_IceWebuser(UINT nIDD);
  CWnd* DomPropPage_IceLocation(UINT nIDD);
  CWnd* DomPropPage_IceVariable(UINT nIDD);
  CWnd* DomPropPage_IceBunit(UINT nIDD);
  CWnd* DomPropPage_IceSecRoles(UINT nIDD);
  CWnd* DomPropPage_IceSecDbusers(UINT nIDD);
  CWnd* DomPropPage_IceSecDbconns(UINT nIDD);
  CWnd* DomPropPage_IceSecWebusers(UINT nIDD);
  CWnd* DomPropPage_IceSecProfiles(UINT nIDD);
  CWnd* DomPropPage_IceSecWebuserRoles(UINT nIDD);
  CWnd* DomPropPage_IceSecWebuserDbconns(UINT nIDD);
  CWnd* DomPropPage_IceSecProfileRoles(UINT nIDD);
  CWnd* DomPropPage_IceSecProfileDbconns(UINT nIDD);
  CWnd* DomPropPage_IceServerApplications(UINT nIDD);
  CWnd* DomPropPage_IceServerLocations(UINT nIDD);
  CWnd* DomPropPage_IceServerVariables(UINT nIDD);

  CWnd* DomPropPage_InstallLevelAditAllUsers(UINT nIDD);
  CWnd* DomPropPage_InstallLevelEnabledLevel(UINT nIDD);
  CWnd* DomPropPage_InstallLevelAuditLog    (UINT nIDD);

  CWnd* DomPropPage_IceBunitFacetAndPage(UINT nIDD);

  CWnd* DomPropPage_IceBunitAccessDef(UINT nIDD);
  CWnd* DomPropPage_IceBunits(UINT nIDD);

  CWnd* DomPropPage_IceBunitRole(UINT nIDD);
  CWnd* DomPropPage_IceBunitDbuser(UINT nIDD);

  CWnd* DomPropPage_IceFacets(UINT nIDD);
  CWnd* DomPropPage_IcePages(UINT nIDD);
  CWnd* DomPropPage_IceLocations(UINT nIDD);

  CWnd* DomPropPage_IceBunitRoles(UINT nIDD);
  CWnd* DomPropPage_IceBunitUsers(UINT nIDD);

  CWnd* DomPropPage_IceFacetNPageRoles(UINT nIDD);
  CWnd* DomPropPage_IceFacetNPageUsers(UINT nIDD);

  CWnd* DomPropPage_IcePageHtml(UINT nIDD);
  CWnd* DomPropPage_IceFacetGif(UINT nIDD);

// Dialog Data
	//{{AFX_DATA(CuDlgDomTabCtrl)
	enum { IDD = IDD_DOMDETAIL };
		// NOTE: the ClassWizard will add data members here
    CTabCtrl    m_cTab1;
	//}}AFX_DATA

public:
  CuTitleBitmap m_staticHeader;
  CWnd*   m_pCurrentPage;
  void OnCancel() {return;}
  void OnOK()     {return;}


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomTabCtrl)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
protected:
  BOOL m_bIsLoading;
  DOMDETAILDLGSTRUCT m_arrayDlg [NUMBEROFDOMPAGES];
  CWnd* ConstructPage (UINT nIDD);     // Called by GetPage

protected:
  CuDomProperty* m_pCurrentProperty;

	// Generated message map functions
	//{{AFX_MSG(CuDlgDomTabCtrl)
  virtual BOOL OnInitDialog();
  afx_msg void OnSize(UINT nType, int cx, int cy);
  afx_msg void OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult);
  afx_msg void OnDestroy();
	afx_msg void OnSelchangingTab1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

  afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg long OnQueryOpenCursor(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif  // ifndef DGDOMTAB_HEADER
