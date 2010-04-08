/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : mainfrm.h
**    Project  : CA-Visual DBA,
**
** History: (after 24-Feb-2000)
**
** 24-Feb-2000 (uk$so01)
**    SIR #100558. 
**    Add a menu item in the main menu to open the hisory of error dialog box.
** 19-Feb-2002 (schph01)
**    SIR #107122 enable the "space calculation" icon even if no DOM is open.
** 04-Apr-2002 (uk$so01)
**    SIR #106648, Integrate (ipm and sqlquery).ocx.
** 31-Oct-2003 (noifr01)
**    fixed propagation error in massive propagation of ingres30 gui tools 
**    sources into the main codeline
** 02-feb-2004 (somsa01)
**    Updated OnActivateApp()'s second parameter to match its definition: DWORD.
** 09-May-2007 (karye01)
**    SIR #118282, added function OnUpdateButtonOnlineSupport.
**/

// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////
#if !defined (MAINFRM_HEADER)
#define MAINFRM_HEADER
#include "sbdlgbar.h"

#include "vtree2.h"     // CTreeItemNodes
#include "saveload.h"

// global special functions defined in mainfrm.cpp
void ForgetDelayedUpdates();
void AcceptDelayedUpdates();
BOOL DelayedUpdatesAccepted();
void DelayUpdateOnNode(char *nodeName);

class CMainFrame : public CMDIFrameWnd
{
	DECLARE_DYNAMIC(CMainFrame)
public:
	BOOL UpdateNodeDlgItemButton(UINT idMenu);
	CMainFrame();
	CTreeCtrl* GetVNodeTreeCtrl () {return (CTreeCtrl*)m_wndVirtualNode.GetDlgItem (IDC_TREE1);};
	void RelayCommand  (WPARAM wParam, LPARAM lParam);
	BOOL GetButtonState(UINT nID);
	CuResizableDlgBar* GetVNodeDlgBar () {return &m_wndVirtualNode;};
	void VDBASetAppTitle(char * lptext) { SetWindowText(lptext);GetWindowText(m_strTitle);}

// Attributes
private:
	void UpdateOpenWindowsNodeDisplay();
	int m_IpmSeqNum;
	int m_DbeventSeqNum;
	int m_SqlactSeqNum;
	BOOL m_bAcceptDelay;
	BOOL m_bNoSaveOnExit;

// Attributes
public:

// Operations
public:
	int GetNextIpmSeqNum()      { return ++m_IpmSeqNum; }
	int GetNextDbeventSeqNum()  { return ++m_DbeventSeqNum; }
	int GetNextSqlactSeqNum()   { return ++m_SqlactSeqNum; }

	// for load time:
	void UpdateIpmSeqNum(int val);
	void UpdateDbeventSeqNum(int val);
	void UpdateSqlactSeqNum(int val);

	void ForgetDelayedUpdates()   { ASSERT(m_bAcceptDelay); m_bAcceptDelay = FALSE; }
	void AcceptDelayedUpdates()   { ASSERT(!m_bAcceptDelay); m_bAcceptDelay = TRUE; }
	BOOL DelayedUpdatesAccepted() { return m_bAcceptDelay; }

	// For users list update in node window after add/alter/drop/forcerefresh/bkrefresh in a dom
	void UpdateNodeUsersList(CString csNodeName);

	// For environment load at startup time (specified on command line)
	void OnInitialLoadEnvironment(CString csCmdLine);

	// Dor "no save on exit" context-driven option
	void SetNoSaveOnExit() { m_bNoSaveOnExit = TRUE; }
	BOOL NoSaveOnExit() { return m_bNoSaveOnExit; }

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CStatusBar  m_wndStatusBar;
	CToolBar    m_wndToolBar;

	// for virtual nodes
	BOOL        m_bNodesVisible;
	BOOL        m_bNodesToolbarMode;
	CuResizableDlgBar m_wndVirtualNode;     // in Toolbar mode
	CDocument  *m_pVirtualNodeMdiDoc;       // in MDI doc mode

public:
	HWND OnCreateDom();
	HWND OnCreateSqlAct();
	UINT ConvertCommandId(UINT cppId, BOOL bContextHelp = FALSE);

	CStatusBar *     GetMainFrmStatusBar() { return &m_wndStatusBar; };
	CTreeItemNodes * GetSelectedVirtNodeItem();
	CToolBar *       GetPMainToolBar() { return &m_wndToolBar; }

	// for virtual nodes
	void             SetNodesVisibilityFlag(BOOL bVisible)  { m_bNodesVisible = bVisible; }
	void             SetPVirtualNodeMdiDoc(CDocument *pDoc) { m_pVirtualNodeMdiDoc = NULL; }
	void             UpdateVirtualNodesVariables();   // Post-Serialization
	BOOL             IsNodesWindowVisible() { return m_wndVirtualNode.IsVisible(); }

	// for menu select
	void ManageMainOnMenuSelect( UINT nItemID, UINT nFlags, HMENU hSysMenu );

protected:
	void PermanentState(BOOL bLoad);

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnUpdateButtonConnect(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonDisconnect(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonPreferences(CCmdUI* pCmdUI);
	afx_msg void OnUpdateWindowTileVert(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonAddobject(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonAlterobject(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonDropobject(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonProperties(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonRefresh(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonFilter1(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonSysobject(CCmdUI* pCmdUI);
	afx_msg void OnButtonAddobject();
	afx_msg void OnButtonAlterobject();
	afx_msg void OnButtonDropobject();
	afx_msg void OnButtonProperties();
	afx_msg void OnButtonRefresh();
	afx_msg void OnButtonFilter1();
	afx_msg void OnButtonSysobject();
	afx_msg void OnUpdateButtonSqlact(CCmdUI* pCmdUI);
	afx_msg LRESULT OnSetupMessage(WPARAM wParam, LPARAM lParam);
	afx_msg void OnUpdateButtonOpen(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonNew(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonSave(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonSaveAs(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonPrint(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonExit(CCmdUI* pCmdUI);
	afx_msg void OnUpdateWindowTileHorz(CCmdUI* pCmdUI);
	afx_msg void OnUpdateWindowCascade(CCmdUI* pCmdUI);
	afx_msg void OnUpdateWindowArrange(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonSelectwindow(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonWindowcloseall(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonAbout(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonHelpindex(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonHelpsearch(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonContexthelp(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonOnlineSupport(CCmdUI* pCmdUI);
	afx_msg void OnClose();
	afx_msg void OnUpdateButtonChmodrefresh(CCmdUI* pCmdUI);
	afx_msg void OnButtonChmodrefresh();
	afx_msg LRESULT OnF1DownMessage(WPARAM wParam, LPARAM lParam);
	afx_msg void OnMenuSelect( UINT nItemID, UINT nFlags, HMENU hSysMenu );
	afx_msg void OnUpdateServersViewnodeswindow(CCmdUI* pCmdUI);
	afx_msg void OnServersViewnodeswindow();
	afx_msg void OnToolbars();
	afx_msg void OnServersHidenodeswindow();
	afx_msg void OnUpdateServersHidenodeswindow(CCmdUI* pCmdUI);
	afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
	afx_msg void OnSysColorChange();
	afx_msg BOOL OnQueryEndSession();
	afx_msg void OnEndSession(BOOL bEnding);
	afx_msg void OnActivateApp(BOOL bActive, DWORD hTask);
	afx_msg void OnDestroy();
	afx_msg void OnPaletteChanged(CWnd* pFocusWnd);
	afx_msg BOOL OnQueryNewPalette();
	afx_msg void OnPaletteIsChanging(CWnd* pRealizeWnd);
	afx_msg void OnButtonHistoryerror();
	afx_msg void OnUpdateButtonSpacecalc(CCmdUI* pCmdUI);
	afx_msg void OnPreferenceSave();
	//}}AFX_MSG

	afx_msg void OnButton01();  // Connect DOM Window                     
	afx_msg void OnButton02();  // Open SQL Test Window                   
	afx_msg void OnButton03();  // Open Monitor Window                    
	afx_msg void OnButton04();  // Open DBEvent Tree Window               
	afx_msg void OnButton05();  // Disconnect Virtual Node                
	afx_msg void OnButton06();  // Close the Selected Window              
	afx_msg void OnButton07();  // Activate the Select Window             
	afx_msg void OnButton08();  // Add                                    
	afx_msg void OnButton09();  // Alter                                  
	afx_msg void OnButton10();  // Drop                                   
	afx_msg void OnButton11();  // Force Refresh of the List of Virtual Nodes
	afx_msg void OnButton12();  // Enable/Disable the Docking view mode.
	afx_msg void OnButtonScratchpad();
	afx_msg void OnUpdateButton01 (CCmdUI* pCmdUI);
	afx_msg void OnUpdateButton02 (CCmdUI* pCmdUI);
	afx_msg void OnUpdateButton03 (CCmdUI* pCmdUI);
	afx_msg void OnUpdateButton04 (CCmdUI* pCmdUI);
	afx_msg void OnUpdateButton05 (CCmdUI* pCmdUI);
	afx_msg void OnUpdateButton06 (CCmdUI* pCmdUI);
	afx_msg void OnUpdateButton07 (CCmdUI* pCmdUI);
	afx_msg void OnUpdateButton08 (CCmdUI* pCmdUI);
	afx_msg void OnUpdateButton09 (CCmdUI* pCmdUI);
	afx_msg void OnUpdateButton10 (CCmdUI* pCmdUI);
	afx_msg void OnUpdateButton11 (CCmdUI* pCmdUI);
	afx_msg void OnUpdateButton12 (CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonScratchpad(CCmdUI* pCmdUI);

	afx_msg void OnButtonTestNode();  // Test node is up
	afx_msg void OnUpdateButtonTestNode(CCmdUI* pCmdUI);

	afx_msg void OnButtonInstallPassword();  // installation password
	afx_msg void OnUpdateButtonInstallPassword(CCmdUI* pCmdUI);

	afx_msg LONG OnUpdateOpenwin(WPARAM wParam, LPARAM lParam);

	afx_msg LONG OnActionThreadAnimate(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
private:
};

#endif

/////////////////////////////////////////////////////////////////////////////
