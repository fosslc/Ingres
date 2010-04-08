/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : mgrvleft.h , Header File 
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Tree View that contains the components 
**
** History:
** 
** 15-Dec-1998 (uk$so01)
**    Created
** 20-Apr-2000 (uk$so01)
**    SIR #101252
**    When user start the VCBF, and if IVM is already running
**    then request IVM the start VCBF
** 03-May-2000 (uk$so01)
**    SIR #101402
**    Start ingres (or ingres component) is in progress ...:
**    If start the Name Server of the whole installation, then just sleep an elapse
**    of time before querying the instance. (15 secs or less if process terminated ealier).
**    Change the accelerate phase of refresh instances from 200ms to 1600ms.
** 14-Aug-2001 (uk$so01)
**    SIR #105383,
**    When changing the selection in a tree, resulting in the right panes to be different, 
**    the selected right pane for the new selected tree item should be the same (that with the same name) 
**    as that of the previous selection in the tree, if there is one with the same name.
** 17-Sep-2001 (uk$so01)
**    SIR #105345, Add menu item "Start Client".
** 01-nov-2001 (somsa01)
**    Cleaned up 64-bit compiler warnings.
** 11-Mar-2003 (uk$so01)
**    SIR #109775, Provide access to all the GUI tools through toolbar and shell icon menu
** 20-Aug-2008 (whiro01)
**    Move <afx....> include to "stdafx.h"
**/

#if !defined(AFX_MGRVLEFT_H__63A2E057_936D_11D2_A2B5_00609725DDAF__INCLUDED_)
#define AFX_MGRVLEFT_H__63A2E057_936D_11D2_A2B5_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "compdata.h"

class CvManagerViewLeft : public CTreeView
{
protected:
	CvManagerViewLeft(); // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CvManagerViewLeft)


public:
	//
	// Call 'InitializeData' when the tree is first created:
	void InitializeData();
	//
	// Call 'Refresh' when need to refresh the Tree:
	void Refresh(int& nChange);

	void AlertRead (CaLoggedEvent* pEvent);
	UINT ThreadProcControlS1 (LPVOID pParam); // Vcbf
	UINT ThreadProcControlS3 (LPVOID pParam, BOOL bStartName = FALSE); // Start / Stop
	UINT ThreadProcControlS4 (LPVOID pParam, BOOL bStart = FALSE);     // Service


	CaTreeComponentItemData* GetComponentItem(){return m_pItem;}
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CvManagerViewLeft)
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	virtual ~CvManagerViewLeft();
	void ActivateTabLastStartStop();
	void OnAppAbout();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	BOOL m_bPopupOpen;
	BOOL m_bAlreadyCreateImageList;
	CImageList m_ImageList;
	CaTreeComponentItemData* m_pInstallationData;
	CWinThread* m_pThread1;
	CWinThread* m_pThread2;
	CWinThread* m_pThread3;
	//
	// If the current selected tree item has the right pane then m_nPreviousPageID = 0
	// else m_nPreviousPageID = PageID of the last right pane (if exist).
	UINT m_nPreviousPageID;

private:
	CaTreeComponentItemData* m_pItem;

	//
	// Generated message map functions
protected:
	//{{AFX_MSG(CvManagerViewLeft)
	afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnItemexpanded(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnStart();
	afx_msg void OnStartClient();
	afx_msg void OnStop();
	afx_msg void OnConfigure();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG_PTR OnGetItemData(WPARAM wParam, LPARAM lParam);
	
	afx_msg LONG OnUpdateComponent(WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnUpdateComponentIcon(WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnUpdateAlert(WPARAM wParam, LPARAM lParam);
	//
	// wParam = 0 (Update Unalert event by event)
	// wParam = 1 (Update Unalert all events)
	afx_msg LONG OnUpdateUnalert(WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnReactivateSelection(WPARAM wParam, LPARAM lParam);

	afx_msg LONG OnGetState(WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnShutDownIngres(WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnErrorOccurred(WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnRunProgram(WPARAM wParam, LPARAM lParam);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MGRVLEFT_H__63A2E057_936D_11D2_A2B5_00609725DDAF__INCLUDED_)
