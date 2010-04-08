/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : ipmframe.h, Header file (Manual Created Frame).
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Frame for monitor windows
**
** History:
**
** 12-Nov-2001 (uk$so01)
**    Created
*/


#ifndef IPMFRAME_HEADER
#define IPMFRAME_HEADER
#include "dgipmc02.h"
#include "vtree.h"

class CdIpmDoc;
class CuIpmTreeFastItem;

class CfIpmFrame : public CFrameWnd
{
	DECLARE_DYNCREATE(CfIpmFrame)
public:
	CfIpmFrame();
	CfIpmFrame(CdIpmDoc* pLoadedDoc);

	CdIpmDoc* GetIpmDoc();
	CWnd* GetSplitterWnd(){return &m_WndSplitter;};
	CWnd* GetLeftPane();
	CWnd* GetRightPane ();
	BOOL IsAllViewCreated() {return m_bAllViewCreated;}
	CuDlgIpmTabCtrl* GetTabDialog ();   
	CTreeCtrl* GetPTree();
	HTREEITEM  GetSelectedItem();
	HTREEITEM  FindSearchedItem(CuIpmTreeFastItem* pItemPath, HTREEITEM hAnchorItem);
	void SelectItem(HTREEITEM hItem);
	void StartExpressRefresh(long nSeconds);
	short FindItem(LPCTSTR lpszText, UINT mode); // return 0: not found, 1: no other occurence, 2: found and selected.
	long GetMonitorObjectsCount();
	//
	// Help management Interface.
	long GetHelpID();      // Return the Dialog Box ID of the current page of Tab Ctrl.

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CfIpmFrame)
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

	// Implementation
protected:
	virtual ~CfIpmFrame();

	BOOL m_bAllViewCreated;
	CSplitterWnd  m_WndSplitter;
	CdIpmDoc* m_pIpmDoc;
	BOOL m_bHasSplitter; // TRUE (default)


	// Generated message map functions
	//{{AFX_MSG(CfIpmFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	afx_msg LONG OnDbeventIncoming (WPARAM wParam, LPARAM lParam);
	afx_msg long OnPropertiesChange(WPARAM wParam, LPARAM lParam);
	afx_msg long OnExpressRefresh(WPARAM wParam, LPARAM lParam);

	// tree item management
	CTreeItem *GetCurSelTreeItem();

	DECLARE_MESSAGE_MAP()

	// Fast access management
private:
	HTREEITEM  GetNextSiblingItem(HTREEITEM hItem);
	HTREEITEM  GetChildItem(HTREEITEM hItem);
	HTREEITEM  GetRootItem();
	BOOL       ExpandNotCurSelItem(HTREEITEM hItem);
	void Init();
	CTreeGlobalData* GetPTreeGD();

	UINT m_arrayExpressRefeshParam[3];
	CWinThread* m_pThreadExpressRefresh;
};

#endif // IPMFRAME_HEADER
