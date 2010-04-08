/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : ipmctl.h : Header File.
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Declaration of the CIpmCtrl ActiveX Control class
**
** History:
**
** 12-Nov-2001 (uk$so01)
**    Created
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 22-Sep-2004 (uk$so01)
**    BUG #113104 / ISSUE 13690527, 
**    Add method: short GetConnected(LPCTSTR lpszNode, LPCTSTR lpszDatabase)
**    that return 1 if there is an SQL Session.
*/

#if !defined(AFX_IPMCTL_H__AB474692_E577_11D5_878C_00C04F1F754A__INCLUDED_)
#define AFX_IPMCTL_H__AB474692_E577_11D5_878C_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
class CfIpmFrame;

class CIpmCtrl : public COleControl
{
	DECLARE_DYNCREATE(CIpmCtrl)

	// Constructor
public:
	CIpmCtrl();
	void ContainerNotifySelChange(){FireIpmTreeSelChange();}

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CIpmCtrl)
	public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	virtual void OnFontChanged();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CfIpmFrame* m_pIpmFrame;
	~CIpmCtrl();
	void ConstructPropertySet(CaIpmProperty& property);
	void NotifySettingChange(UINT nMask);

	DECLARE_OLECREATE_EX(CIpmCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CIpmCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CIpmCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CIpmCtrl)		// Type name and misc status

	// Message maps
	//{{AFX_MSG(CIpmCtrl)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg long OnTrackerNotify (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

	// Dispatch maps
	//{{AFX_DISPATCH(CIpmCtrl)
	long m_timeOut;
	afx_msg void OnTimeOutChanged();
	long m_refreshFrequency;
	afx_msg void OnRefreshFrequencyChanged();
	BOOL m_activateRefresh;
	afx_msg void OnActivateRefreshChanged();
	BOOL m_showGrid;
	afx_msg void OnShowGridChanged();
	long m_unit;
	afx_msg void OnUnitChanged();
	long m_maxSession;
	afx_msg void OnMaxSessionChanged();
	afx_msg BOOL Initiate(LPCTSTR lpszNode, LPCTSTR lpszServer, LPCTSTR lpszUser, LPCTSTR lpszOption);
	afx_msg void ExpandBranch();
	afx_msg void ExpandAll();
	afx_msg void CollapseBranch();
	afx_msg void CollapseAll();
	afx_msg void ExpandOne();
	afx_msg void CollapseOne();
	afx_msg BOOL ExpressRefresh();
	afx_msg BOOL ItemShutdown();
	afx_msg BOOL ForceRefresh();
	afx_msg BOOL OpenServer();
	afx_msg BOOL CloseServer();
	afx_msg BOOL ReplicationServerChangeNode();
	afx_msg BOOL ResourceTypeChange(short nResType);
	afx_msg BOOL NullResource(short bSet);
	afx_msg BOOL InternalSession(short bSet);
	afx_msg BOOL SystemLockList(short bSet);
	afx_msg BOOL InactiveTransaction(short bSet);
	afx_msg BOOL IsEnabledShutdown();
	afx_msg BOOL IsEnabledOpenServer();
	afx_msg BOOL IsEnabledCloseServer();
	afx_msg BOOL IsEnabledReplicationChangeNode();
	afx_msg SCODE Loading(LPUNKNOWN pStream);
	afx_msg SCODE Storing(LPUNKNOWN FAR* ppStream);
	afx_msg void ProhibitActionOnTreeCtrl(short nYes);
	afx_msg BOOL UpdateFilters(short FAR* arrayFilter, short nArraySize);
	afx_msg void SearchItem();
	afx_msg void SetSessionStart(long nStart);
	afx_msg void SetSessionDescription(LPCTSTR lpszSessionDescription);
	afx_msg void StartExpressRefresh(long nSeconds);
	afx_msg BOOL SelectItem(LPCTSTR lpszNode, LPCTSTR lpszServer, LPCTSTR lpszUser, LPCTSTR lpszKey, VARIANT FAR* pArrayItem, short nShowTree);
	afx_msg void SetErrorlogFile(LPCTSTR lpszFullPathFile);
	afx_msg short FindAndSelectTreeItem(LPCTSTR lpszText, long nMode);
	afx_msg long GetMonitorObjectsCount();
	afx_msg long GetHelpID();
	afx_msg void SetHelpFile(LPCTSTR lpszFileWithoutPath);
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

	// Event maps
	//{{AFX_EVENT(CIpmCtrl)
	void FireIpmTreeSelChange()
		{FireEvent(eventidIpmTreeSelChange,EVENT_PARAM(VTS_NONE));}
	void FirePropertyChange()
		{FireEvent(eventidPropertyChange,EVENT_PARAM(VTS_NONE));}
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

	// Dispatch and event IDs
public:
	enum {
		dispidGetConnected = 43L,	//{{AFX_DISP_ID(CIpmCtrl)
	dispidTimeOut = 1L,
	dispidRefreshFrequency = 2L,
	dispidActivateRefresh = 3L,
	dispidShowGrid = 4L,
	dispidUnit = 5L,
	dispidMaxSession = 6L,
	dispidInitiate = 7L,
	dispidExpandBranch = 8L,
	dispidExpandAll = 9L,
	dispidCollapseBranch = 10L,
	dispidCollapseAll = 11L,
	dispidExpandOne = 12L,
	dispidCollapseOne = 13L,
	dispidExpressRefresh = 14L,
	dispidItemShutdown = 15L,
	dispidForceRefresh = 16L,
	dispidOpenServer = 17L,
	dispidCloseServer = 18L,
	dispidReplicationServerChangeNode = 19L,
	dispidResourceTypeChange = 20L,
	dispidNullResource = 21L,
	dispidInternalSession = 22L,
	dispidSystemLockList = 23L,
	dispidInactiveTransaction = 24L,
	dispidIsEnabledShutdown = 25L,
	dispidIsEnabledOpenServer = 26L,
	dispidIsEnabledCloseServer = 27L,
	dispidIsEnabledReplicationChangeNode = 28L,
	dispidLoading = 29L,
	dispidStoring = 30L,
	dispidProhibitActionOnTreeCtrl = 31L,
	dispidUpdateFilters = 32L,
	dispidSearchItem = 33L,
	dispidSetSessionStart = 34L,
	dispidSetSessionDescription = 35L,
	dispidStartExpressRefresh = 36L,
	dispidSelectItem = 37L,
	dispidSetErrorlogFile = 38L,
	dispidFindAndSelectTreeItem = 39L,
	dispidGetMonitorObjectsCount = 40L,
	dispidGetHelpID = 41L,
	dispidSetHelpFile = 42L,
	eventidIpmTreeSelChange = 1L,
	eventidPropertyChange = 2L,
	//}}AFX_DISP_ID
	};
protected:
	SHORT GetConnected(LPCTSTR lpszNode, LPCTSTR lpszDatabase);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IPMCTL_H__AB474692_E577_11D5_878C_00C04F1F754A__INCLUDED)
