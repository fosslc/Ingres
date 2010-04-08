/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   :  ijactrlc.h , Header File
**    Project  : Ingres Journal Analyser
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Implementation of the CIjaCtrl ActiveX Control class 
**
** History:
**
** 22-Dec-1999 (uk$so01)
**    Created
** 14-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management and put string into the resource files
** 29-Jun-2001 (hanje04)
**    Removed ' from end of defn of enu as it causes build errors on 
**    solaris.
** 05-Sep-2003 (uk$so01)
**    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
*/

#if !defined(AFX_IJACTRLCTL_H__C92B8435_B176_11D3_A322_00C04F1F754A__INCLUDED_)
#define AFX_IJACTRLCTL_H__C92B8435_B176_11D3_A322_00C04F1F754A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "ijadbase.h"
#include "ijatable.h"


class CIjaCtrl : public COleControl
{
	DECLARE_DYNCREATE(CIjaCtrl)

public:
	CIjaCtrl();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CIjaCtrl)
	public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	~CIjaCtrl();
	short m_nMode; 
	BOOL  m_bEnableRecoverRedo; 


	CuDlgIjaDatabase* m_mDlgDatabase;
	CuDlgIjaTable*    m_mDlgTable;

	DECLARE_OLECREATE_EX(CIjaCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CIjaCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CIjaCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CIjaCtrl)      // Type name and misc status

	// Message maps
	//{{AFX_MSG(CIjaCtrl)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	// Dispatch maps
	//{{AFX_DISPATCH(CIjaCtrl)
	OLE_COLOR m_transactionColorDelete;
	afx_msg void OnColorTransactionDeleteChanged();
	OLE_COLOR m_transactionColorInsert;
	afx_msg void OnColorTransactionInsertChanged();
	OLE_COLOR m_transactionColorAfterUpdate;
	afx_msg void OnColorTransactionAfterUpdateChanged();
	OLE_COLOR m_transactionColorBeforeUpdate;
	afx_msg void OnColorTransactionBeforeUpdateChanged();
	OLE_COLOR m_transactionColorOther;
	afx_msg void OnColorTransactionOtherChanged();
	BOOL m_bPaintForegroundTransaction;
	afx_msg void OnPaintForegroundTransactionChanged();
	afx_msg void SetMode(short nMode);
	afx_msg long AddUser(LPCTSTR strUser);
	afx_msg void CleanUser();
	afx_msg void RefreshPaneDatabase(LPCTSTR lpszNode, LPCTSTR lpszDatabase, LPCTSTR lpszDatabaseOwner);
	afx_msg void RefreshPaneTable(LPCTSTR lpszNode, LPCTSTR lpszDatabase, LPCTSTR lpszDatabaseOwner, LPCTSTR lpszTable, LPCTSTR lpszTableOwner);
	afx_msg void SetUserPos(long nPos);
	afx_msg void SetUserString(LPCTSTR lpszUser);
	afx_msg void SetAllUserString(LPCTSTR lpszAllUser);
	afx_msg void AddNode(LPCTSTR lpszNode);
	afx_msg void RemoveNode(LPCTSTR lpszNode);
	afx_msg void CleanNode();
	afx_msg short GetMode();
	afx_msg void EnableRecoverRedo(BOOL bEnable);
	afx_msg BOOL GetEnableRecoverRedo();
	afx_msg long ShowHelp();
	afx_msg void SetHelpFilePath(LPCTSTR lpszPath);
	afx_msg void SetSessionStart(long nStart);
	afx_msg void SetSessionDescription(LPCTSTR lpszDescription);
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

	afx_msg void AboutBox();

// Event maps
	//{{AFX_EVENT(CIjaCtrl)
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

	// Dispatch and event IDs
public:
	enum {
	//{{AFX_DISP_ID(CIjaCtrl)
	dispidTransactionDelete = 1L,
	dispidTransactionInsert = 2L,
	dispidTransactionAfterUpdate = 3L,
	dispidTransactionBeforeUpdate = 4L,
	dispidTransactionOther = 5L,
	dispidPaintForegroundTransaction = 6L,
	dispidSetMode = 7L,
	dispidAddUser = 8L,
	dispidCleanUser = 9L,
	dispidRefreshPaneDatabase = 10L,
	dispidRefreshPaneTable = 11L,
	dispidSetUserPos = 12L,
	dispidSetUserString = 13L,
	dispidSetAllUserString = 14L,
	dispidAddNode = 15L,
	dispidRemoveNode = 16L,
	dispidCleanNode = 17L,
	dispidGetMode = 18L,
	dispidEnableRecoverRedo = 19L,
	dispidGetEnableRecoverRedo = 20L,
	dispidShowHelp = 21L,
	dispidSetHelpFilePath = 22L,
	dispidSetSessionStart = 23L,
	dispidSetSessionDescription = 24L
	//}}AFX_DISP_ID
	};
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IJACTRLCTL_H__C92B8435_B176_11D3_A322_00C04F1F754A__INCLUDED)
