/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : mainfrm.h  : Header File.
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : interface of the Frame CfMainFrame class  (MFC frame/view/doc).
**
** History:
**
** 10-Dec-2001 (uk$so01)
**    Created
** 26-Feb-2003 (uk$so01)
**    SIR #106648, conform to the change of DDS VDBA split
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 16-Mar-2004 (uk$so01)
**    BUG #111965 / ISSUE 13296981 The Vdbamon's menu item "Help Topic" is disabled.
** 04-Jun-2004 (uk$so01)
**    SIR #111701, Connect Help to History of SQL Statement error.
*/


#if !defined(AFX_MAINFRM_H__AE712EF8_E8C7_11D5_8792_00C04F1F754A__INCLUDED_)
#define AFX_MAINFRM_H__AE712EF8_E8C7_11D5_8792_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "dmlbase.h"
class CdIpm;

typedef struct tagFRAMEDATASTRUCT
{
	short* arrayFilter;
	int nDim;
} FRAMEDATASTRUCT;

class CfMainFrame : public CFrameWnd
{
	
protected: 
	CfMainFrame();
	DECLARE_DYNCREATE(CfMainFrame)

	// Operations
public:
	CdIpm* GetIpmDoc(){return m_pIpmDoc;}
	void SetIpmDoc(CdIpm* pDoc){m_pIpmDoc = pDoc;}
	void GetFilters(short* arrayFilter, int nSize);
	void DisableExpresRefresh();
	void PropertiesChange(BOOL bChanged = TRUE){m_bPropertyChange = bChanged;}

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

protected:  // control bar embedded members
	CdIpm*       m_pIpmDoc;
	CStatusBar   m_wndStatusBar;
	CToolBar     m_wndToolBar;
	CToolBar     m_wndToolBarCmd;
	CReBar       m_wndReBar;
	CDialogBar   m_wndDlgBar;
	CDialogBar   m_wndDlgConnectBar;
	CImageList   m_ImageListNode;
	CImageList   m_ImageListServer;
	CImageList   m_ImageListUser;
	CComboBoxEx* m_pComboNode;
	CComboBoxEx* m_pComboServer;
	CComboBoxEx* m_pComboUser;

	CString m_strDefaultServer;
	CString m_strDefaultUser;
	UINT    m_nAlreadyRefresh;
	BOOL    m_bPropertyChange;
	BOOL    m_bSavePreferenceAsDefault;
	BOOL    m_bCommandLine;
	BOOL    m_bActivateApply;

	CaDBObject* m_pDefaultServer;
	CaDBObject* m_pDefaultUser;
	CTypedPtrList< CObList, CaDBObject* > m_lNode;
	CTypedPtrList< CObList, CaDBObject* > m_lServer;
	CTypedPtrList< CObList, CaDBObject* > m_lUser;

	void UpdateLoadedData(CdIpm* pDoc, FRAMEDATASTRUCT* pFrameData);
	void ShowIpmControl(BOOL bShow);
	BOOL ShouldEnable();
	int  SetDefaultServer(BOOL bCleanCombo = FALSE);
	int  SetDefaultUser(BOOL bCleanCombo = FALSE);
	void SaveWorkingFile(CString& strFullName);
	void PermanentState(BOOL bLoad);
	void HandleCommandLine(CdIpm* pDoc);
	BOOL QueryNode();
	BOOL QueryServer();
	BOOL QueryUser();

	// Generated message map functions
protected:
	//{{AFX_MSG(CfMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnGo();
	afx_msg void OnForceRefresh();
	afx_msg void OnShutDown();
	afx_msg void OnExpandBranch();
	afx_msg void OnExpandAll();
	afx_msg void OnCollapseBranch();
	afx_msg void OnCollapseAll();
	afx_msg void OnDestroy();
	afx_msg void OnUpdateExpandBranch(CCmdUI* pCmdUI);
	afx_msg void OnUpdateExpandAll(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCollapseBranch(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCollapseAll(CCmdUI* pCmdUI);
	afx_msg void OnUpdateShutDown(CCmdUI* pCmdUI);
	afx_msg void OnUpdateForceRefresh(CCmdUI* pCmdUI);
	afx_msg void OnFileOpen();
	afx_msg void OnFileSave();
	afx_msg void OnUpdateFileOpen(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileSave(CCmdUI* pCmdUI);
	afx_msg void OnViewToolbar();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPreference();
	afx_msg void OnUpdatePreference(CCmdUI* pCmdUI);
	afx_msg void OnUpdatePreferenceSave(CCmdUI* pCmdUI);
	afx_msg void OnPreferenceSave();
	afx_msg void OnClose();
	afx_msg void OnUpdateGo(CCmdUI* pCmdUI);
	afx_msg void OnHistoricSqlError();
	afx_msg void OnUpdateHistoricSqlError(CCmdUI* pCmdUI);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	afx_msg long OnExpressRefresh(WPARAM wParam, LPARAM lParam);

	afx_msg void OnDropDownNode(); 
	afx_msg void OnSelChangeNode();
	afx_msg void OnDropDownServer();
	afx_msg void OnSelChangeServer();
	afx_msg void OnDropDownUser();
	afx_msg void OnSelChangeUser();

	afx_msg void OnSelChangeResourceType();
	afx_msg void OnCheckNullResource();
	afx_msg void OnCheckInternalSession();
	afx_msg void OnCheckInactiveTransaction();
	afx_msg void OnCheckSystemLockList();
	afx_msg void OnCheckExpresRefresh();
	afx_msg void OnEditChangeFrequency();

	afx_msg void OnUpdateResourceType(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNullResource(CCmdUI* pCmdUI);
	afx_msg void OnUpdateInternalSession(CCmdUI* pCmdUI);
	afx_msg void OnUpdateInactiveTransaction(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSystemLockList(CCmdUI* pCmdUI);
	afx_msg void OnUpdateExpresRefresh(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditFrequency(CCmdUI* pCmdUI);


	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnDefaultHelp();
};
BOOL APP_HelpInfo(HWND hWnd, UINT nHelpID);


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__AE712EF8_E8C7_11D5_8792_00C04F1F754A__INCLUDED_)
