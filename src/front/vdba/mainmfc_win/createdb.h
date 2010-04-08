/****************************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**  Project  : Visual DBA
**
**  createdb.h
**
**  History after 01-May-2002:
**
**  14-May-2002 (uk$so01)
**   (bug 107773) don't refuse any more international characters (such as
**   accented characters) for the database name
**  26-May-2004 (schph01)
**  SIR #112460 add "Catalogs Page Size" control in createdb dialog box.
**  06-Sep-2005) (drivi01)
**  Bug #115173 Updated createdb dialogs and alterdb dialogs to contain
**  two available unicode normalization options, added group box with
**  two checkboxes corresponding to each normalization, including all
**  expected functionlity.
**  09-Mar-2010 (drivi01) 
**  SIR #123397 Update the dialog to be more user friendly.  Minimize the amount
**  of controls exposed to the user when dialog is initially displayed.
**  Add "Show/Hide Advanced" button to see more/less options in the 
**  dialog. Take out DBD from the list of fe-clients.  Put all the options
**  into a tab control.
**
*/
#if !defined(AFX_CREATEDB_H__A434A702_43ED_11D2_9718_00609725DBF9__INCLUDED_)
#define AFX_CREATEDB_H__A434A702_43ED_11D2_9718_00609725DBF9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// createdb.h : header file
//
#include "uchklbox.h"
//UKS #include "editobjn.h"
#include "createadv.h"
#include "localter.h"
#include "location.h"
extern "C" {
	#include "dbaset.h"
	#include "dll.h"
	#include <nm.h>
	#include <er.h>
}
#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////
// CxDlgCreateAlterDatabase dialog
class CxDlgCreateAlterDatabase : public CDialogEx
{
  // property panes declared as friend so they can call OnOK/OnCancel handlers
  friend class CuDlgcreateDBAlternateLoc;
  friend class CuDlgcreateDBLocation;
  friend class CuDlgcreateDBAdvanced;

// Construction
public:
	CxDlgCreateAlterDatabase(CWnd* pParent = NULL);   // standard constructor
	LPCREATEDB m_StructDatabaseInfo;

// Dialog Data
	//{{AFX_DATA(CxDlgCreateAlterDatabase)
	enum { IDD = IDD_DOM_CREATE_DB };
	CButton	m_ctrlUnicodeEnable;
	CButton	m_ctrlUnicodeEnable2;
	CButton m_ctrlEnableUnicode;
	CTabCtrl	m_cTabLoc;
	CButton m_ctrlMoreOptions;
	//}}AFX_DATA

	CEdit m_ctrlDatabaseName;
	BOOL bExpanded;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgCreateAlterDatabase)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
protected:
	CuDlgcreateDBLocation* m_pDlgcreateDBLocation;
	CuDlgcreateDBAdvanced* m_pDlgcreateDBAdvanced;
	CuDlgcreateDBAlternateLoc* m_pDlgcreateDBAlternateLoc;

	// Generated message map functions
	//{{AFX_MSG(CxDlgCreateAlterDatabase)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	virtual void OnOK();
	afx_msg void OnChangeDbname();
	afx_msg void OnSelchangeTabLocation(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnReadOnly();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void ChangeGlobalAccessStatus( BOOL bPublic);
	void ChangeExtendedLocationStatus();
	void ExecuteRemoteCommand();
	void SelectDefaultLocation();
	void FillStructureWithCtrlInfo();
	BOOL OccupyLocationsControl();
	void InitialiseEditControls();
	BOOL IsW4GLInLocalInstall();
	BOOL OccupyProdutsControl();
	LPDB_EXTENSIONS FindExtendLoc(char *szLocName,int iTypeLoc);
	int VerifyOneWorkLocExist();
	int IsDatabaseLocked();
	void ChangeCatalogPageSize(long lNewPageSize );

	CStatic m_CtrlStaticCatPageSize;
public:
	afx_msg void OnBnClickedShowMoreOptions();
	afx_msg void OnBnClickedEnableUnicode();
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CREATEDB_H__A434A702_43ED_11D2_9718_00609725DBF9__INCLUDED_)
