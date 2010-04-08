/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ipmaxfrm.h  : Header File.
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : interface of the Frame CfIpm class  (MFC frame/view/doc).
**
** History:
**
** 20-Fev-2002 (uk$so01)
**    Created
**    SIR #106648, Integrate ipm.ocx.
*/


#if !defined(AFX_IPMFRAME_H__AE712EF8_E8C7_11D5_8792_00C04F1F754A__INCLUDED_)
#define AFX_IPMFRAME_H__AE712EF8_E8C7_11D5_8792_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "dmlbase.h"
#include "sbipmbar.h"

class CdIpm;
class CaIpmFrameFilter;
class CuIpmTreeFastItem;

class CfIpm : public CMDIChildWnd
{
	
protected: 
	CfIpm();
	DECLARE_DYNCREATE(CfIpm)

	// Operations
public:
	CdIpm* GetIpmDoc(){return m_pIpmDoc;}
	void SetIpmDoc(CdIpm* pDoc){m_pIpmDoc = pDoc;}
	void GetFilters(short* arrayFilter, int nSize);
	void DisableExpresRefresh();
	CuIpmDlgBar& GetToolBar(){return m_wndDlgBar;}
	void GetFilter(CaIpmFrameFilter* pFrameData);
	long GetHelpID();
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CfIpm)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CfIpm();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	BOOL m_bAllCreated;
	CdIpm* m_pIpmDoc;
	CuIpmDlgBar m_wndDlgBar;

	void UpdateLoadedData(CdIpm* pDoc);
	void ShowIpmControl(BOOL bShow);
	BOOL ShouldEnable();
	// Generated message map functions
protected:
	//{{AFX_MSG(CfIpm)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd);
	//}}AFX_MSG
	afx_msg void OnUpdateButtonFind(CCmdUI* pCmdUI);

	afx_msg void OnForceRefresh();
	afx_msg void OnShutDown();
	afx_msg void OnExpandBranch();
	afx_msg void OnExpandAll();
	afx_msg void OnCollapseBranch();
	afx_msg void OnCollapseAll();
	afx_msg void OnSelChangeResourceType();
	afx_msg void OnCheckNullResource();
	afx_msg void OnCheckInternalSession();
	afx_msg void OnCheckInactiveTransaction();
	afx_msg void OnCheckSystemLockList();
	afx_msg void OnCheckExpresRefresh();
	afx_msg void OnEditChangeFrequency();
	afx_msg void OnEditKillfocusFrequency();

	afx_msg void OnUpdateExpandBranch(CCmdUI* pCmdUI);
	afx_msg void OnUpdateExpandAll(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCollapseBranch(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCollapseAll(CCmdUI* pCmdUI);
	afx_msg void OnUpdateShutDown(CCmdUI* pCmdUI);
	afx_msg void OnUpdateForceRefresh(CCmdUI* pCmdUI);
	afx_msg void OnUpdateResourceType(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNullResource(CCmdUI* pCmdUI);
	afx_msg void OnUpdateInternalSession(CCmdUI* pCmdUI);
	afx_msg void OnUpdateInactiveTransaction(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSystemLockList(CCmdUI* pCmdUI);
	afx_msg void OnUpdateExpresRefresh(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditFrequency(CCmdUI* pCmdUI);

	afx_msg long OnGetDocumentType(WPARAM wParam, LPARAM lParam);
	afx_msg long OnFind(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IPMFRAME_H__AE712EF8_E8C7_11D5_8792_00C04F1F754A__INCLUDED_)
