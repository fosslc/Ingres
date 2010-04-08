/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : ipmdoc.h, Header File 
**    Project  : INGRESII/Monitoring.
**    Author   : UK Sotheavut
**    Purpose  : The Document Data for the IPM Window.
**
** History:
**
** 12-Nov-2001 (uk$so01)
**    Created
** 17-Jul-2003 (uk$so01)
**    SIR #106648, Vdba-Split, ensure that the ingres sessions 
**    have the descriptions.
*/

#ifndef IPMDOC_HEADER
#define IPMDOC_HEADER
#include "qryinfo.h"
#include "ipm.h"

class CdIpmDoc;
typedef struct tagTHREADSCANDBEPARAM
{
	CdIpmDoc* pIpmDoc;
	HWND   hWndFrame;
	HANDLE hEvent;
}THREADSCANDBEPARAM;

class CuIpmProperty;
class CuDlgIpmTabCtrl;
class CTreeGlobalData;
class CTreeItem;
class CdIpmDoc : public CDocument
{
	DECLARE_SERIAL(CdIpmDoc)
public:
	CdIpmDoc();
	//
	// lpszKey    : "SERVER"|"SESSION"|"LOGINFO"|"REPLICSERVER".
	// pArrayItem : item to be selected. It must be a VT_ARRAY.
	//              For item that has no parent, the array must have only one lelement.
	//              Otherwise the first element is parent0, the second is parent1, ... and the last is element itself.
	//              EX: e1 = "ii\ingres\158" for lpszKey = "SERVER"
	//                  e1 = "ii\ingres\158" and e2 = "0ad5aa00" for lpszKey = "SESSION"
	//                  e1 = "db1" and e2 = "1" for lpszKey = "REPLICSERVER"
	CdIpmDoc(LPCTSTR lpszKey, VARIANT* pArrayItem, BOOL bShowTree);
	void SetTabDialog (CuDlgIpmTabCtrl* pTabDlg){m_pTabDialog = pTabDlg;}
	CuDlgIpmTabCtrl* GetTabDialog() {return m_pTabDialog;}
	CTreeItem* GetSelectedTreeItem();
	HANDLE GetEventRaisedDBEvents() {return m_hEventRaisedDBEvents;}

	void SetNodeHandle (int nNodeHandle) {m_hNode = nNodeHandle;}
	void SetDbmsVersion(int nDbmsVersion){m_nOIVers = nDbmsVersion;}
	BOOL ManageMonSpecialState(); // returns TRUE if refresh has occured

	int GetNodeHandle(){return m_hNode;}
	int GetNodeHandleReplication(){return m_hReplMonHandle;}
	int SetNodeHandleReplication(int nHnodeRep){m_hReplMonHandle = nHnodeRep; return m_hReplMonHandle;}
	int GetDbmsVersion(){return m_nOIVers;}
	CuIpmProperty* GetCurrentProperty() {return m_pCurrentProperty;}
	SFILTER* GetFilter(){return &m_sFilter;}
	LPSFILTER GetLpFilter() { return &m_sFilter; }
	CTreeGlobalData* GetPTreeGD() { return m_pTreeGD; }
	CaConnectInfo& GetConnectionInfo(){return m_connectedInfo;}
	CaIpmProperty& GetProperty() {return m_property;}

	BOOL IsLoadedDoc() { return m_bLoaded; }
	BOOL InitializeReplicator(LPCTSTR lpszDatabase);
	BOOL TerminateReplicator();

	BOOL UpdateDisplay();
	BOOL IsInitiated(){return m_bInitiate;}
	void BkRefresh();
	//
	// Select item: 
	CString GetSelectKey(){return m_strSelectKey;}
	CStringArray& GetItemPath(){return m_arrItemPath;}
	BOOL GetShowTree(){return m_bShowTree;}

	//
	// Interfaces:
	BOOL Initiate(LPCTSTR lpszNode, LPCTSTR lpszServerClass, LPCTSTR lpszUser, LPCTSTR lpszOption);
	void ItemExpandBranch();
	void ItemExpandAll();
	void ItemCollapseBranch();
	void ItemCollapseAll();
	void ItemExpandOne();
	void ItemCollapseOne();
	BOOL ExpresRefresh();
	BOOL ItemShutdown();
	BOOL ForceRefresh();
	BOOL FilterChange(FilterCause nCause, BOOL bSet);
	BOOL ResourceTypeChange(int nResourceType); // nResourceType == RESTYPE_XXX
	BOOL ItemOpenServer();
	BOOL ItemCloseServer();
	BOOL ItemReplicationServerChangeNode();
	BOOL GetData (IStream** ppStream);
	BOOL SetData (IStream* pStream);
	void ProhibitActionOnTreeCtrl(BOOL bProhibit);
	void SearchItem();
	void SetSessionStart(long nStart);
	void StartExpressRefresh(long nSeconds); // nSeconds = 0 => stop.

	BOOL IsEnabledItemShutdown();
	BOOL IsEnabledItemOpenServer();
	BOOL IsEnabledItemCloseServer();
	BOOL IsEnabledItemReplicationServerChangeNode();


protected:
	CaConnectInfo m_connectedInfo;
	//
	// Backward compatibilities:
	int m_nOIVers;
	int m_hNode;
	int m_hReplMonHandle;
	SFILTER m_sFilter;
	CuIpmProperty* m_pCurrentProperty; // This member is only constructed from the serialization
	int m_cxCur; // splitter
	int m_cxMin; // splitter

	CWinThread* m_pThreadScanDBEvents;
	HANDLE      m_hEventRaisedDBEvents;
	THREADSCANDBEPARAM m_ThreadParam;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CdIpmDoc)
	public:
	virtual void Serialize(CArchive& ar); // overridden for document i/o
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CdIpmDoc();
	void SetLoadDoc(BOOL bLoaded){m_bLoaded = bLoaded;}
	void SetLockDisplayRightPane(BOOL bLock){m_bLockDisplayRightPane = bLock;}
	BOOL GetLockDisplayRightPane(){return m_bLockDisplayRightPane;}
	void GetSpliterSize(int& ncxCur, int& ncxMin){ncxCur = m_cxCur; ncxMin = m_cxMin;}
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
	void DisplayDoc();
#endif

private:
	void Initialize();

	CTreeGlobalData* m_pTreeGD;
	CuDlgIpmTabCtrl* m_pTabDialog;
	BOOL  m_bLoaded;
	BOOL  m_bInitiate;
	CaIpmProperty m_property;

	CString m_strSelectKey;
	BOOL    m_bShowTree;
	BOOL    m_bLockDisplayRightPane;
	CStringArray m_arrItemPath;

	// Generated message map functions
protected:
	//{{AFX_MSG(CdIpmDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // IPMDOC_HEADER
