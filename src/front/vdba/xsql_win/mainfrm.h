/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : mainfrm.h : interface of the CfMainFrame class
**    Project  : SQL/Test (stand alone)
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Main frame.
**
** History:
**
** 10-Dec-2001 (uk$so01)
**    Created
** 26-Feb-2003 (uk$so01)
**    SIR #106648, conform to the change of DDS VDBA split
** 03-Oct-2003 (uk$so01)
**    SIR #106648, Vdba-Split, Additional fix for GATEWAY Enhancement 
** 15-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
** 17-Oct-2003 (uk$so01)
**    SIR #106648, Additional change to change #464605 (role/password)
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 01-Sep-2004 (schph01)
**    SIR #106648 add message map function for the 'History of SQL Statements
**    Errors' dialog 
*/


#if !defined(AFX_MAINFRM_H__1D04F618_EFC9_11D5_8795_00C04F1F754A__INCLUDED_)
#define AFX_MAINFRM_H__1D04F618_EFC9_11D5_8795_00C04F1F754A__INCLUDED_
#include "dmlbase.h"
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CuSqlQueryControl;
class CdSqlQuery;
class CfMainFrame : public CFrameWnd
{
	
protected: // create from serialization only
	CfMainFrame();
	DECLARE_DYNCREATE(CfMainFrame)


	// Operations
public:
	CdSqlQuery* GetSqlDoc(){return m_pSqlDoc;}
	void SetSqlDoc(CdSqlQuery* pDoc){m_pSqlDoc = pDoc;}

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CfMainFrame)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CfMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	void PropertyChange(){m_bPropertyChange = TRUE;}

protected:  // control bar embedded members
	CdSqlQuery* m_pSqlDoc;
	CStatusBar  m_wndStatusBar;
	CToolBar    m_wndToolBar;
	CToolBar    m_wndToolBarConnect;
	CDialogBar  m_wndDlgBar;
	CReBar      m_wndReBar;

	CComboBoxEx* m_pComboNode;
	CComboBoxEx* m_pComboServer;
	CComboBoxEx* m_pComboUser;
	CComboBoxEx* m_pComboDatabase;
	CImageList   m_ImageListNode;
	CImageList   m_ImageListServer;
	CImageList   m_ImageListUser;
	CImageList   m_ImageListDatabase;

	CString m_strDefaultServer;
	CString m_strDefaultUser;
	CString m_strDefaultDatabase;
	UINT    m_nAlreadyRefresh;
	BOOL    m_bUpdateLoading;
	BOOL    m_bPropertyChange;
	BOOL    m_bSavePreferenceAsDefault;

	CaDBObject* m_pDefaultServer;
	CaDBObject* m_pDefaultUser;
	CaDBObject* m_pDefaultDatabase;

	CTypedPtrList< CObList, CaDBObject* > m_lNode;
	CTypedPtrList< CObList, CaDBObject* > m_lServer;
	CTypedPtrList< CObList, CaDBObject* > m_lUser;
	CTypedPtrList< CObList, CaDBObject* > m_lDatabase;

	CuSqlQueryControl* GetSqlqueryControl();
	int  SetDefaultDatabase(BOOL bCleanCombo = FALSE);
	int  SetDefaultServer(BOOL bCleanCombo = FALSE);
	int  SetDefaultUser(BOOL bCleanCombo = FALSE);
	void SetIdicatorAutocomit(int nOnOff);
	void SetIdicatorReadLock(int nOnOff);
	void CommitOldSession();
	void UpdateLoadedData(CdSqlQuery* pDoc);
	void SaveWorkingFile(CString& strFullName);
	void PermanentState(BOOL bLoad);
	BOOL QueryNode(LPCTSTR lpszCmdNode = NULL);
	BOOL QueryServer(LPCTSTR lpszCmdServer = NULL);
	BOOL QueryUser();
	BOOL QueryDatabase(LPCTSTR lpszCmdDatabase = NULL);

	virtual BOOL SetConnection(BOOL bDisplayMessage = TRUE);
	virtual UINT HandleCommandLine(CdSqlQuery* pDoc);

#if defined (_OPTION_GROUPxROLE)
	CImageList   m_ImageListGroup;
	CImageList   m_ImageListRole;
	CComboBoxEx* m_pComboGroup;
	CComboBoxEx* m_pComboRole;
	CString      m_strDefaultGroup;
	CString      m_strDefaultRole;
	CaDBObject*  m_pDefaultGroup;
	CaDBObject*  m_pDefaultRole;
	CTypedPtrList< CObList, CaDBObject* > m_lGroup;
	CTypedPtrList< CObList, CaDBObject* > m_lRole;
	BOOL QueryGroup();
	BOOL QueryRole();
	int  SetDefaultGroup(BOOL bCleanCombo = FALSE);
	int  SetDefaultRole(BOOL bCleanCombo = FALSE);
	BOOL CheckRolePassword();
	CString RequestRolePassword(LPCTSTR lpszRole);
#endif
	UINT m_uSetArg;
	UINT m_nIDHotImage;

	// Generated message map functions
protected:
	//{{AFX_MSG(CfMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClear();
	afx_msg void OnOpenQuery();
	afx_msg void OnSaveQuery();
	afx_msg void OnAssistant();
	afx_msg void OnRun();
	afx_msg void OnQep();
	afx_msg void OnXml();
	afx_msg void OnTrace();
	afx_msg void OnSetting();
	afx_msg void OnUpdateClear(CCmdUI* pCmdUI);
	afx_msg void OnUpdateOpenQuery(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSaveQuery(CCmdUI* pCmdUI);
	afx_msg void OnUpdateAssistant(CCmdUI* pCmdUI);
	afx_msg void OnUpdateRun(CCmdUI* pCmdUI);
	afx_msg void OnUpdateQep(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTrace(CCmdUI* pCmdUI);
	afx_msg void OnUpdateXml(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSetting(CCmdUI* pCmdUI);
	afx_msg void OnRefresh();
	afx_msg void OnOpenEnvironment();
	afx_msg void OnUpdateOpenEnvironment(CCmdUI* pCmdUI);
	afx_msg void OnSaveEnvironment();
	afx_msg void OnUpdateSaveEnvironment(CCmdUI* pCmdUI);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnCommit();
	afx_msg void OnUpdateCommit(CCmdUI* pCmdUI);
	afx_msg void OnClose();
	afx_msg void OnPreferenceSave();
	afx_msg void OnNew();
	afx_msg void OnUpdateNew(CCmdUI* pCmdUI);
	afx_msg void OnDefaultHelp();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG

	afx_msg void OnDropDownNode(); 
	afx_msg void OnSelChangeNode();
	afx_msg void OnDropDownServer();
	afx_msg void OnSelChangeServer();
	afx_msg void OnDropDownUser();
	afx_msg void OnSelChangeUser();
	afx_msg void OnDropDownDatabase();
	afx_msg void OnSelChangeDatabase();
#if defined (_OPTION_GROUPxROLE)
	afx_msg void OnDropDownGroup();
	afx_msg void OnSelChangeGroup();
	afx_msg void OnDropDownRole();
	afx_msg void OnSelChangeRole();
#endif

	DECLARE_MESSAGE_MAP()
	afx_msg void OnHistoricSqlError();
	afx_msg void OnUpdateHistoricSqlError(CCmdUI *pCmdUI);
};

BOOL APP_HelpInfo(HWND hWnd, UINT nHelpID);


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__1D04F618_EFC9_11D5_8795_00C04F1F754A__INCLUDED_)
