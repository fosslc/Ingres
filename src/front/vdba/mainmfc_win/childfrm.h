/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : childfrm.h : interface of the CChildFrame class 
**    Project  : IngresII / VDBA.
**    Author   : Emmanuel Blattes
**
** History (after 26-Sep-2000)
**
** 26-Sep-2000 (uk$so01)
**    SIR #102711: Callable in context command line improvement (for Manage-IT)
**
**  16-Nov-2000 (schph01)
**    SIR 102821 Comment on table and columns.
**  11-Apr-2001 (schph01)
**    (sir 103292) add menu for 'usermod' command.
**  27-Apr-2001 (schph01)
**    (sir 103299) new menu management for 'copydb' command.
** 03-Feb-2004 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX or COM:
**    Integrate Export assistant into vdba
** 10-Mar-2010 (drivi01) SIR 123397
**    Add functionality to bring up "Generate Statistics" dialog 
**    from command line by providing proper vnode/database/table flags
**    to Visual DBA utility.
**    example: vdba.exe /c dom (local) table junk/mytable1 GENSTATANDEXIT
** 11-May-2010 (drivi01)
**    Added 2 functions OnUpdateButtonCreateidx and OnButtonCreateidx.
**    These functions will add functionality behind newly added
**    "Create Index" file menu.
**/

//
/////////////////////////////////////////////////////////////////////////////

#ifndef CHILDFRM_INCLUDED
#define CHILDFRM_INCLUDED

#include "combobar.h"
#include "chboxbar.h"

class CuDomTreeFastItem;    // cannot include "domfast.h" abruptly
class CuDomObjDesc;

class CChildFrame : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CChildFrame)
public:
	CChildFrame();
	BOOL SpecialCmdAddAlterDrop(CuDomObjDesc* pObjDesc);

	//
	// Add/Alter/Drop object from command line:
	void ObjectAdd(){OnButtonAddobject();}
	void ObjectDrop(){OnButtonDropobject();}
	void ObjectAlter(){OnButtonAlterobject();}
	void ObjectOptimize(){OnButtonGenstat();}

// Attributes
public:
  BOOL GetDesktopItemState(BOOL bOnlyIfDb);
  void RelayCommand(UINT itemId);
  void OnIdleGlobStatusUpdate();
  //
  // Help management Interface.
  DWORD GetHelpID();      // Return the Dialog Box ID of the current page of Tab Ctrl.
  CWnd* GetRightPane () {return m_pWndSplitter->GetPane (0, 1);};
  //
  // Return the image of the current selected tree's branch
  static HICON DOMTreeGetCurrentIcon(HWND hwndDOMTree);

  // Fast expand
  DWORD GetTreeCurSel();
  DWORD FindSearchedItem(CuDomTreeFastItem* pFastItem, DWORD dwZeSel);
  void  SelectItem(DWORD dwZeSel);

protected:
    CToolBar        m_wndToolBar;
    CuComboToolBar  m_ComboOwnerFilter;
    CuComboToolBar  m_ComboOwnerFilter2;
    CuCheckBoxBar   m_CheckBoxSysObject;

private:
    CSplitterWnd*   m_pWndSplitter; // splitter window
    CRuntimeClass*  m_pDomView2;    // view in right pane
    BOOL  m_bAllViewCreated;
    BOOL  m_bActive;                // Active document ?
    BOOL  m_bInside;                // mouse cursor inside doc ?

    BOOL  m_bRegularToolbar;        // for custom gray

    BOOL GetDbItemState();
    BOOL GetReplicGenericItemState(UINT nId);

public:
    BOOL IsAllViewCreated() {return m_bAllViewCreated;}

// Operations
public:
    afx_msg void OnCheckBoxSysObjClicked();
    afx_msg void OnComboOwnerDropdown();
    afx_msg void OnComboOwnerSelchange();
    afx_msg void OnComboOtherDropdown();
    afx_msg void OnComboOtherSelchange();

    CuComboToolBar& GetComboOwner()     { return m_ComboOwnerFilter; }
    CuComboToolBar& GetComboOwner2()    { return m_ComboOwnerFilter2; }
    CuCheckBoxBar&  GetCheckBoxSysObj() { return m_CheckBoxSysObject; }

    afx_msg void OnInitMenuPopup( CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu );
//    afx_msg void OnSysCommand( UINT nID, LPARAM lParam );

    void RelayerOnButtonFastload();
    void RelayerOnButtonInfodb();
    void RelayerOnButtonDuplicateDb();
    void RelayerOnButtonSecurityAudit();
    void RelayerOnButtonExpireDate();
    void RelayerOnButtonComment();
    void RelayerOnButtonUsermod();
    void RelayerOnButtonJournaling();
    void RelayerOnButtonCopyDB();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CChildFrame)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CChildFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(CChildFrame)
	afx_msg void OnUpdateButtonSpacecalc(CCmdUI* pCmdUI);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd);
	afx_msg void OnUpdateButtonAddobject(CCmdUI* pCmdUI);
	afx_msg void OnButtonAddobject();
	afx_msg void OnUpdateButtonAlterobject(CCmdUI* pCmdUI);
	afx_msg void OnButtonAlterobject();
	afx_msg void OnUpdateButtonDropobject(CCmdUI* pCmdUI);
	afx_msg void OnButtonDropobject();
	afx_msg void OnUpdateButtonPrint(CCmdUI* pCmdUI);
	afx_msg void OnButtonPrint();
	afx_msg void OnUpdateButtonFind(CCmdUI* pCmdUI);
	afx_msg void OnButtonFind();
	afx_msg void OnUpdateButtonClassbExpandone(CCmdUI* pCmdUI);
	afx_msg void OnButtonClassbExpandone();
	afx_msg void OnUpdateButtonClassbExpandbranch(CCmdUI* pCmdUI);
	afx_msg void OnButtonClassbExpandbranch();
	afx_msg void OnUpdateButtonClassbExpandall(CCmdUI* pCmdUI);
	afx_msg void OnButtonClassbExpandall();
	afx_msg void OnUpdateButtonClassbCollapseone(CCmdUI* pCmdUI);
	afx_msg void OnButtonClassbCollapseone();
	afx_msg void OnUpdateButtonClassbCollapsebranch(CCmdUI* pCmdUI);
	afx_msg void OnButtonClassbCollapsebranch();
	afx_msg void OnUpdateButtonClassbCollapseall(CCmdUI* pCmdUI);
	afx_msg void OnButtonClassbCollapseall();
	afx_msg void OnUpdateButtonShowSystem(CCmdUI* pCmdUI);
	afx_msg void OnButtonShowSystem();
	afx_msg void OnUpdateButtonFilter(CCmdUI* pCmdUI);
	afx_msg void OnButtonFilter();
	afx_msg void OnUpdateButtonProperties(CCmdUI* pCmdUI);
	afx_msg void OnButtonProperties();
	afx_msg void OnUpdateButtonReplicInstall(CCmdUI* pCmdUI);
	afx_msg void OnButtonReplicInstall();
	afx_msg void OnUpdateButtonReplicPropag(CCmdUI* pCmdUI);
	afx_msg void OnButtonReplicPropag();
	afx_msg void OnUpdateButtonReplicBuildsrv(CCmdUI* pCmdUI);
	afx_msg void OnButtonReplicBuildsrv();
	afx_msg void OnUpdateButtonReplicReconcil(CCmdUI* pCmdUI);
	afx_msg void OnButtonReplicReconcil();
	afx_msg void OnUpdateButtonReplicDereplic(CCmdUI* pCmdUI);
	afx_msg void OnButtonReplicDereplic();
	afx_msg void OnUpdateButtonReplicMobile(CCmdUI* pCmdUI);
	afx_msg void OnButtonReplicMobile();
	afx_msg void OnUpdateButtonReplicArcclean(CCmdUI* pCmdUI);
	afx_msg void OnButtonReplicArcclean();
	afx_msg void OnUpdateButtonReplicRepmod(CCmdUI* pCmdUI);
	afx_msg void OnButtonReplicRepmod();
	afx_msg void OnUpdateButtonReplicCreateKeys(CCmdUI* pCmdUI);
	afx_msg void OnButtonReplicCreateKeys();
	afx_msg void OnUpdateButtonReplicActivate(CCmdUI* pCmdUI);
	afx_msg void OnButtonReplicActivate();
	afx_msg void OnUpdateButtonReplicDeactivate(CCmdUI* pCmdUI);
	afx_msg void OnButtonReplicDeactivate();
	afx_msg void OnUpdateButtonNewwindow(CCmdUI* pCmdUI);
	afx_msg void OnButtonNewwindow();
	afx_msg void OnUpdateButtonTearout(CCmdUI* pCmdUI);
	afx_msg void OnButtonTearout();
	afx_msg void OnUpdateButtonRestartfrompos(CCmdUI* pCmdUI);
	afx_msg void OnButtonRestartfrompos();
	afx_msg void OnUpdateButtonLocate(CCmdUI* pCmdUI);
	afx_msg void OnButtonLocate();
	afx_msg void OnUpdateButtonRefresh(CCmdUI* pCmdUI);
	afx_msg void OnButtonRefresh();
	afx_msg void OnUpdateEditCut(CCmdUI* pCmdUI);
	afx_msg void OnEditCut();
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
	afx_msg void OnEditCopy();
	afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI);
	afx_msg void OnEditPaste();
	afx_msg void OnUpdateButtonAlterdb(CCmdUI* pCmdUI);
	afx_msg void OnButtonAlterdb();
	afx_msg void OnUpdateButtonSysmod(CCmdUI* pCmdUI);
	afx_msg void OnButtonSysmod();
	afx_msg void OnUpdateButtonAudit(CCmdUI* pCmdUI);
	afx_msg void OnButtonAudit();
	afx_msg void OnUpdateButtonGenstat(CCmdUI* pCmdUI);
	afx_msg void OnButtonGenstat();
	afx_msg void OnUpdateButtonDispstat(CCmdUI* pCmdUI);
	afx_msg void OnButtonDispstat();
	afx_msg void OnUpdateButtonCheckpoint(CCmdUI* pCmdUI);
	afx_msg void OnButtonCheckpoint();
	afx_msg void OnUpdateButtonRollforward(CCmdUI* pCmdUI);
	afx_msg void OnButtonRollforward();
	afx_msg void OnUpdateButtonVerifydb(CCmdUI* pCmdUI);
	afx_msg void OnButtonVerifydb();
	afx_msg void OnUpdateButtonUnloaddb(CCmdUI* pCmdUI);
	afx_msg void OnButtonUnloaddb();
	afx_msg void OnUpdateButtonCopydb(CCmdUI* pCmdUI);
	afx_msg void OnButtonCopydb();
	afx_msg void OnUpdateButtonPopulate(CCmdUI* pCmdUI);
	afx_msg void OnButtonPopulate();
	afx_msg void OnUpdateButtonGrant(CCmdUI* pCmdUI);
	afx_msg void OnButtonGrant();
	afx_msg void OnUpdateButtonRevoke(CCmdUI* pCmdUI);
	afx_msg void OnButtonRevoke();
	afx_msg void OnUpdateButtonModifystruct(CCmdUI* pCmdUI);
	afx_msg void OnButtonModifystruct();
	afx_msg void OnUpdateButtonCreateidx(CCmdUI* pCmdUI);
	afx_msg void OnButtonCreateidx();
	afx_msg void OnUpdateButtonLoad(CCmdUI* pCmdUI);
	afx_msg void OnButtonLoad();
	afx_msg void OnUpdateButtonUnload(CCmdUI* pCmdUI);
	afx_msg void OnButtonUnload();
	afx_msg void OnUpdateButtonDownload(CCmdUI* pCmdUI);
	afx_msg void OnButtonDownload();
	afx_msg void OnUpdateButtonRunscripts(CCmdUI* pCmdUI);
	afx_msg void OnButtonRunscripts();
	afx_msg void OnUpdateButtonUpdstat(CCmdUI* pCmdUI);
	afx_msg void OnButtonUpdstat();
	afx_msg void OnButtonRegisteraslink();
	afx_msg void OnUpdateButtonRegisteraslink(CCmdUI* pCmdUI);
	afx_msg void OnButtonRefreshlink();
	afx_msg void OnUpdateButtonRefreshlink(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonRemoveObject(CCmdUI* pCmdUI);
	afx_msg void OnButtonRemoveObject();
	afx_msg void OnDestroy();
	afx_msg void OnButtonFastload();
	afx_msg void OnUpdateButtonFastload(CCmdUI* pCmdUI);
	afx_msg void OnButtonInfodb();
	afx_msg void OnUpdateButtonInfodb(CCmdUI* pCmdUI);
	afx_msg void OnButtonDuplicateDb();
	afx_msg void OnUpdateButtonDuplicateDb(CCmdUI* pCmdUI);
	afx_msg void OnButtonExpireDate();
	afx_msg void OnButtonComment();
	afx_msg void OnUpdateButtonExpireDate(CCmdUI* pCmdUI);
	afx_msg void OnButtonSecurityAudit();
	afx_msg void OnUpdateButtonSecurityAudit(CCmdUI* pCmdUI);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnButtonJournaling();
	afx_msg void OnUpdateButtonJournaling(CCmdUI* pCmdUI);
	afx_msg void OnClose();
	afx_msg void OnUpdateButtonComment(CCmdUI* pCmdUI);
	afx_msg void OnButtonUsermod();
	afx_msg void OnUpdateButtonUsermod(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonExport(CCmdUI* pCmdUI);
	afx_msg void OnButtonExport();
	//}}AFX_MSG

  // toolbar management
  afx_msg LONG OnHasToolbar       (WPARAM wParam, LPARAM lParam);
  afx_msg LONG OnIsToolbarVisible (WPARAM wParam, LPARAM lParam);
  afx_msg LONG OnSetToolbarState  (WPARAM wParam, LPARAM lParam);
  afx_msg LONG OnSetToolbarCaption(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#endif // CHILDFRM_INCLUDED
