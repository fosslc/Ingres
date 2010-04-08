/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : ipmdoc.cpp, Implementation File  
**    Project  : INGRESII/Monitoring.
**    Author   : UK Sotheavut
**    Purpose  : The Document Data for the IPM Window.
**
** History:
**
** 12-Nov-2001 (uk$so01)
**    Created
** 19-May-2003 (uk$so01)
**    SIR #110269, Add Network trafic monitoring.
** 22-May-2003 (uk$so01)
**    SIR #106648
**    Change the serialize machanism. Do not serialize the HTREEITEM
**    because the stored HTREEITEM may different of when the tree is
**    reconstructed from the serialize.
** 17-Jul-2003 (uk$so01)
**    SIR #106648, Vdba-Split, ensure that the ingres sessions 
**    have the descriptions.
** 20-Aug-2008 (whiro01)
**    Moved <afx...> include to "stdafx.h"
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "ipmdoc.h"
#include "ipmframe.h"
#include "vtreeglb.h"
#include "viewleft.h"
#include "a2stream.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static UINT ThreadControlScanDBEvents(LPVOID pParam)
{
	THREADSCANDBEPARAM* p = (THREADSCANDBEPARAM*)pParam;
	CdIpmDoc* pIpmDoc = (CdIpmDoc*)p->pIpmDoc;
	int nFrequency = 5;
	BOOL bHasEvent = FALSE;
	BOOL bUpdateTree = FALSE;
	while (1)
	{
		DWORD dwWait = WaitForSingleObject(p->hEvent, 60*1000);
		if (dwWait == WAIT_OBJECT_0)
		{
			::PostMessage(p->hWndFrame, WM_USER_DBEVENT_TRACE_INCOMING, (WPARAM)0, (LPARAM)0);
		}
		Sleep (1000 * nFrequency);
	}
	return 0;
}

IMPLEMENT_SERIAL(CdIpmDoc, CDocument, 1)
CdIpmDoc::CdIpmDoc()
{
	Initialize();
}

CdIpmDoc::CdIpmDoc(LPCTSTR lpszKey, VARIANT* pArrayItem, BOOL bShowTree)
{
	USES_CONVERSION;
	Initialize();
	m_strSelectKey = lpszKey;
	m_bShowTree    = bShowTree;

	LONG dwSLBound = 0;
	LONG dwSUBound = 0;
	if (!pArrayItem)
		return;
	if (!pArrayItem->parray)
		return;
	long lDim = SafeArrayGetDim (pArrayItem->parray);
	ASSERT (lDim == 1);
	if (lDim != 1)
		return;

	HRESULT hr = SafeArrayGetLBound(pArrayItem->parray, 1, &dwSLBound);
	if(FAILED(hr))
		return;
	if(dwSLBound != 0)
		return;
	hr = SafeArrayGetUBound(pArrayItem->parray, 1, &dwSUBound);
	if(FAILED(hr))
		return;

	for (long i=dwSLBound; i<=dwSUBound; i++)
	{
		VARIANT obj;
		VariantInit(&obj);
		SafeArrayGetElement(pArrayItem->parray, &i, &obj);
		m_arrItemPath.Add(W2T(obj.bstrVal));
		VariantClear(&obj);
	}
}

void CdIpmDoc::Initialize()
{
	m_bInitiate = FALSE;
	m_bLoaded = FALSE;
	m_hNode   = -1;
	m_nOIVers = -1;
	m_hReplMonHandle = -1;
	memset (&m_sFilter, 0, sizeof(SFILTER));

	m_sFilter.bNullResources = FALSE;
	m_sFilter.bInternalSessions = FALSE;
	m_sFilter.bSystemLockLists = FALSE;
	m_sFilter.bInactiveTransactions = FALSE;
	m_sFilter.ResourceType = RESTYPE_ALL;
	m_pTreeGD = new CTreeGlobalData (NULL, &m_sFilter);
	ASSERT (m_pTreeGD);
	m_pTreeGD->SetDocument(this);

	m_cxCur = 200;
	m_cxMin = 10;


	// NEED TO CALL BEFORE VIEW ARE CREATED:
	// - SetHNode() or serialization load mechanism
	// - AND SetPTree() in all cases
	m_pCurrentProperty = NULL;
	m_pTabDialog = NULL;

	m_pThreadScanDBEvents = NULL;
	memset (&m_ThreadParam, 0, sizeof(m_ThreadParam));
	m_hEventRaisedDBEvents = CreateEvent(NULL, TRUE, TRUE, NULL);

	m_strSelectKey = _T("");
	m_bShowTree    = TRUE;
	m_bLockDisplayRightPane = FALSE;
}


CdIpmDoc::~CdIpmDoc()
{
	SetEvent(m_hEventRaisedDBEvents);
	if (m_pThreadScanDBEvents)
	{
		TerminateThread (m_pThreadScanDBEvents->m_hThread, 0);
		delete m_pThreadScanDBEvents;
		m_pThreadScanDBEvents = NULL;
	}
	CloseHandle(m_hEventRaisedDBEvents);

	TerminateReplicator();
	IPM_TerminateDocument(this);
	if (m_pTreeGD)
		delete m_pTreeGD;
	if (m_pCurrentProperty)
		delete m_pCurrentProperty;
	m_arrItemPath.RemoveAll();
}


BEGIN_MESSAGE_MAP(CdIpmDoc, CDocument)
	//{{AFX_MSG_MAP(CdIpmDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CdIpmDoc diagnostics

#ifdef _DEBUG
void CdIpmDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CdIpmDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CdIpmDoc serialization

void CdIpmDoc::Serialize(CArchive& ar)
{
	CString caption;
	m_connectedInfo.Serialize(ar);

	if (ar.IsStoring())
	{
		ar << m_hNode;
		ar << m_hReplMonHandle;
		ar << m_nOIVers;

		// filter structure
		ar.Write(&m_sFilter, sizeof(m_sFilter));

		// splitbar
		if (m_bShowTree )
		{
			POSITION pos = GetFirstViewPosition();
			CView *pView = GetNextView(pos);
			ASSERT (pView);
			CSplitterWnd *pSplit = (CSplitterWnd *)pView->GetParent();
			ASSERT (pSplit);
			pSplit->GetColumnInfo(0, m_cxCur, m_cxMin);
		}
		ar << m_cxCur;
		ar << m_cxMin;

		ASSERT (m_pTabDialog);
		CuIpmPropertyData* pData = m_pTabDialog->GetDialogData();
		CuIpmProperty* pCurrentProperty = m_pTabDialog->GetCurrentProperty();
		if (pCurrentProperty) 
		{
			pCurrentProperty->SetPropertyData (pData);
			ar << pCurrentProperty;
		}

		ar << m_strSelectKey;
		ar << m_bShowTree;
	}
	else
	{
		m_bLoaded = TRUE;

		ar >> m_hNode; // Note : OpenNodeStruct() MUST NOT BE CALLED!
		ar >> m_hReplMonHandle;
		ar >> m_nOIVers;
		IPM_SetDbmsVersion(m_nOIVers);

		// filter structure
		ar.Read(&m_sFilter, sizeof(m_sFilter));

		// splitbar
		ar >> m_cxCur;
		ar >> m_cxMin;

		if (m_pCurrentProperty)
		{
			delete m_pCurrentProperty;
			m_pCurrentProperty = NULL;
		}
		ar >> m_pCurrentProperty;
		ar >> m_strSelectKey;
		ar >> m_bShowTree;
	}
	m_arrItemPath.Serialize(ar);
	IPM_SerializeChache(ar);

	// global data for the tree, including tree lines
	// ex - m_pTreeGD->Serialize(ar);
	if (ar.IsStoring())
	{
		ar << m_pTreeGD;
	}
	else
	{
		delete m_pTreeGD; // memory leak otherwise
		ar >> m_pTreeGD;
		m_pTreeGD->SetPSFilter(&m_sFilter);
		m_pTreeGD->SetDocument(this);
	}
}

BOOL CdIpmDoc::ManageMonSpecialState()
{
	if (m_hNode != -1) 
	{
		if (IPM_IsFirstAccessDataAfterLoad(this))
		{
			CWaitCursor doWaitCursor;
			IPMUPDATEPARAMS ups;
			memset (&ups, 0, sizeof(ups));
			ups.nType = 0;
			ups.pStruct = NULL;
			CaIpmQueryInfo info (this, OT_MON_ALL, &ups);
			IPM_UpdateInfo (&info);

			BOOL bFirstAccess = FALSE;
			IPM_SetFirstAccessDataAfterLoad(this, bFirstAccess);
			UpdateDisplay();

			// _T("Monitor Information has been refreshed, (according to previous message when loading an environment), and the selected item may have changed.");
			AfxMessageBox(IDS_MONITOR_INFO);
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CdIpmDoc::InitializeReplicator(LPCTSTR lpszDatabase)
{
	return IPM_InitializeReplicator (this, lpszDatabase);
}

BOOL CdIpmDoc::TerminateReplicator()
{
	return IPM_TerminateReplicator(this);
}

BOOL CdIpmDoc::UpdateDisplay()
{
	TRACE0("CdIpmDoc::UpdateDisplay()\n");
	try
	{
		m_pTreeGD->RefreshAllTreeBranches(); // refresh left pane
		
		IPMUPDATEPARAMS ups;
		memset (&ups, 0, sizeof (ups));
		CTreeItem* pItem = NULL;
		CTreeCtrl* pTree = m_pTreeGD->GetPTree();
		ASSERT (pTree);
		HTREEITEM hSelectedItem = pTree->GetSelectedItem();
		if (!hSelectedItem)
			return FALSE;
		pItem = (CTreeItem*)pTree->GetItemData(hSelectedItem);
		ASSERT (pItem);
		if (pItem->IsNoItem() || pItem->IsErrorItem())
		{
			m_pTabDialog->DisplayPage (NULL);
		}
		else 
		if (pItem->ItemDisplaysNoPage()) 
		{
			CString caption = pItem->ItemNoPageCaption();
			m_pTabDialog->DisplayPage (NULL, caption);
		}
		else 
		{
			int nImage = -1, nSelectedImage = -1;
			CImageList* pImageList = pTree->GetImageList (TVSIL_NORMAL);
			HICON hIcon = NULL;
			int nImageCount = pImageList? pImageList->GetImageCount(): 0;
			if (pImageList && pTree->GetItemImage(hSelectedItem, nImage, nSelectedImage))
			{
				if (nImage < nImageCount)
					hIcon = pImageList->ExtractIcon(nImage);
			}
			CuPageInformation* pPageInfo = pItem->GetPageInformation();
			CString strItem = pItem->GetRightPaneTitle();

			ups.nType   = pItem->GetType();
			ups.pStruct = pItem->GetPTreeItemData()? pItem->GetPTreeItemData()->GetDataPtr(): NULL;
			ups.pSFilter= m_pTreeGD->GetPSFilter();
			pPageInfo->SetUpdateParam (&ups);
			pPageInfo->SetTitle ((LPCTSTR)strItem, pItem, this);

			pPageInfo->SetImage  (hIcon);
			m_pTabDialog->DisplayPage (pPageInfo);
		}

		// Replicator Monitor special management
		// ASSUMES we did not change the current item!
		BOOL bReplMonWasOn = (m_hReplMonHandle != -1) ? TRUE: FALSE;
		BOOL bReplMonToBeOn = pItem->HasReplicMonitor();
		// 3 cases :
		//    - on to off ---> replicator has been uninstalled
		//    - off to on ---> replicator has been installed
		//    - on to on, or off to off: state has not changed
		if (bReplMonWasOn && !bReplMonToBeOn) 
		{
			BOOL bOK = TerminateReplicator();
			ASSERT (bOK);
		}
		else 
		if (!bReplMonWasOn && bReplMonToBeOn)
		{
			CString csDbName = pItem->GetDBName();
			BOOL bOK = InitializeReplicator(csDbName);
			ASSERT (bOK);
		}
		//
		// Refresh right pane
		CfIpmFrame* pIpmFrame = (CfIpmFrame*)pTree->GetParentFrame();
		UpdateAllViews((CView *)pIpmFrame->GetLeftPane());
	}
	catch(CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
		return FALSE;
	}
	catch(CResourceException* e)
	{
		//_T("Cannot load dialog box");
		AfxMessageBox (IDS_E_LOAD_DLG);
		e->Delete();
		return FALSE;
	}
	catch(...)
	{
		//_T("Cannot construct the property pane");
		AfxMessageBox (IDS_E_CONSTRUCT_PROPERTY);
		return FALSE;
	}

	return TRUE;
}

BOOL CdIpmDoc::Initiate(LPCTSTR lpszNode, LPCTSTR lpszServerClass, LPCTSTR lpszUser, LPCTSTR lpszOption)
{
	try
	{
		m_connectedInfo.SetNode(lpszNode);
		if (lpszServerClass)
			m_connectedInfo.SetServerClass(lpszServerClass);
		if (lpszUser)
			m_connectedInfo.SetUser(lpszUser);

		BOOL bOK = IPM_Initiate(this);
		if (bOK)
		{
			// 
			// LPARAM = 1 (Instruct the view that the function initiate has been invoked)
			UpdateAllViews(NULL, (LPARAM)1);
			m_bInitiate = TRUE;
			//
			// Create a Thread that periodically scans the DBEvents in order to
			// update the tree and the right pane:
			if (!m_pThreadScanDBEvents)
			{
				CTreeCtrl* pTree = m_pTreeGD->GetPTree();
				ASSERT (pTree);
				if (pTree)
				{
					CfIpmFrame* pIpmFrame = (CfIpmFrame*)pTree->GetParentFrame();
					m_ThreadParam.hWndFrame = pIpmFrame? pIpmFrame->m_hWnd: NULL;
				}
				m_ThreadParam.hEvent  = m_hEventRaisedDBEvents;
				m_ThreadParam.pIpmDoc = this;

				m_pThreadScanDBEvents = AfxBeginThread(
					(AFX_THREADPROC)ThreadControlScanDBEvents, 
					&m_ThreadParam, 
					THREAD_PRIORITY_NORMAL,
					0,
					CREATE_SUSPENDED);
				if (m_pThreadScanDBEvents)
					ResumeThread(m_pThreadScanDBEvents->m_hThread);
			}
		}
		return bOK;
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		TRACE0("Exception in: CdIpmDoc::Initiate\n");
	}
	return FALSE;
}

BOOL CdIpmDoc::ItemShutdown()
{
	try
	{
		CTreeCtrl* pTree = m_pTreeGD->GetPTree();
		HTREEITEM hItem  = pTree->GetSelectedItem();
		if (hItem) 
		{
			CTreeItem* pItem = (CTreeItem*)pTree->GetItemData(hItem);
			ASSERT (pItem);
			if (pItem)
				return pItem->TreeKillShutdown();
		}
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		TRACE0("Exception in: CdIpmDoc::ItemShutdown\n");
	}
	return FALSE;
}

BOOL CdIpmDoc::ForceRefresh()
{
	try
	{
		CTreeCtrl* pTree = m_pTreeGD->GetPTree();

		CWaitCursor hourGlass;
		IPMUPDATEPARAMS ups;
		memset (&ups, 0, sizeof(ups));
		ups.nType = 0;
		ups.pStruct = NULL;
		CaIpmQueryInfo queryInfo (this, OT_MON_ALL, &ups);
		IPM_UpdateInfo(&queryInfo);
		IPM_SetNormalState(this);
		if (IsLoadedDoc())
		{
			UpdateDisplay();
			m_bLoaded = FALSE;
		}
		return TRUE;
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		TRACE0("Exception in: CdIpmDoc::ForceRefresh\n");
	}
	return FALSE;
}

BOOL CdIpmDoc::ExpresRefresh ()
{
	try
	{
		CuDlgIpmTabCtrl* pTab = m_pTabDialog;
		if (pTab && IsWindow (pTab->m_hWnd))
		{
			IPMUPDATEPARAMS ups;
			memset (&ups, 0, sizeof (ups));
			ups.nIpmHint = FILTER_IPM_EXPRESS_REFRESH; // 0 if for every pages
			CWnd* pCurrentActivePage = pTab->GetCurrentPage();
			if (pCurrentActivePage)
				pCurrentActivePage->SendMessage (WM_USER_IPMPAGE_UPDATEING, (WPARAM)this, (LPARAM)&ups);
			return TRUE;
		}
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		TRACE0("Exception in: CdIpmDoc::ExpresRefresh\n");
	}
	return FALSE;
}

BOOL CdIpmDoc::ResourceTypeChange(int nResourceType)
{
	try
	{
		BOOL bIgnore = FALSE;
		if (m_sFilter.ResourceType == nResourceType)
			return TRUE;
		m_sFilter.ResourceType = nResourceType;
		BOOL bOK = FilterChange(FILTER_RESOURCE_TYPE, bIgnore);

		return bOK;
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		TRACE0("Exception in: CdIpmDoc::ResourceTypeChange\n");
	}
	return FALSE;
}

BOOL CdIpmDoc::FilterChange(FilterCause nCause, BOOL bSet)
{
	try
	{
		switch (nCause)
		{
		case FILTER_NOTHING:
			return TRUE;
		case FILTER_NULL_RESOURCES:
			if (m_sFilter.bNullResources == bSet)
				return TRUE;
			m_sFilter.bNullResources = bSet;
			break;
		case FILTER_INTERNAL_SESSIONS:
			if (m_sFilter.bInternalSessions == bSet)
				return TRUE;
			m_sFilter.bInternalSessions = bSet;
			break;
		case FILTER_SYSTEM_LOCK_LISTS:
			if (m_sFilter.bSystemLockLists == bSet)
				return TRUE;
			m_sFilter.bSystemLockLists = bSet;
			break;
		case FILTER_INACTIVE_TRANSACTIONS:
			if (m_sFilter.bInactiveTransactions == bSet)
				return TRUE;
			m_sFilter.bInactiveTransactions = bSet;
			break;
		case FILTER_RESOURCE_TYPE:
			break;
		default:
			ASSERT(FALSE);
			return TRUE;
		}

		BOOL bOK = m_pTreeGD->FilterAllDisplayedItemsLists(nCause);
		if (bOK)
		{
			POSITION pos = GetFirstViewPosition();
			while (pos != NULL)
			{
				CView* pView = GetNextView(pos);
				if (pView->IsKindOf(RUNTIME_CLASS(CvIpmLeft)))
				{
					UpdateAllViews(pView, (LPARAM)(int)nCause); // will only update right pane
					break;
				}
			}
		}
		return bOK;
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		TRACE0("Exception in: CdIpmDoc::FilterChange\n");
	}
	return FALSE;
}


BOOL CdIpmDoc::ItemOpenServer()
{
	try
	{
		CTreeItem* pItem = GetSelectedTreeItem();
		if (pItem)
			return pItem->TreeOpenServer();
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		TRACE0("Exception in: CdIpmDoc::ItemOpenServer\n");
	}
	return FALSE;
}

BOOL CdIpmDoc::ItemCloseServer()
{
	try
	{
		CTreeItem* pItem = GetSelectedTreeItem();
		if (pItem)
			return pItem->TreeCloseServer();
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		TRACE0("Exception in: CdIpmDoc::ItemCloseServer\n");
	}
	return FALSE;
}

BOOL CdIpmDoc::ItemReplicationServerChangeNode()
{
	try
	{
		return IPM_ReplicationServerChangeRunNode(this);
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		TRACE0("Exception in: CdIpmDoc::ItemReplicationServerChangeNode\n");
	}
	return FALSE;
}

void CdIpmDoc::ItemExpandBranch()
{
	try
	{
		m_pTreeGD->ExpandBranch();
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		TRACE0("Exception in: CdIpmDoc::ItemExpandBranch\n");
	}
}

void CdIpmDoc::ItemExpandAll()
{
	try
	{
		m_pTreeGD->ExpandAll();
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		TRACE0("Exception in: CdIpmDoc::ItemExpandAll\n");
	}
}

void CdIpmDoc::ItemCollapseBranch()
{
	try
	{
		m_pTreeGD->CollapseBranch();
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		TRACE0("Exception in: CdIpmDoc::ItemCollapseBranch\n");
	}
}

void CdIpmDoc::ItemCollapseAll()
{
	try
	{
		m_pTreeGD->CollapseAll();
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		TRACE0("Exception in: CdIpmDoc::ItemCollapseAll\n");
	}
}

void CdIpmDoc::ItemExpandOne()
{
	try
	{
		CTreeCtrl* pTree = m_pTreeGD->GetPTree();
		HTREEITEM hItem  = pTree->GetSelectedItem();
		if (hItem) 
		{
			pTree->Expand(hItem, TVE_EXPAND);
		}
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		TRACE0("Exception in: CdIpmDoc::ItemExpandOne\n");
	}
}

void CdIpmDoc::ItemCollapseOne()
{
	try
	{
		CTreeCtrl* pTree = m_pTreeGD->GetPTree();
		HTREEITEM hItem  = pTree->GetSelectedItem();
		if (hItem) 
		{
			pTree->Expand(hItem, TVE_COLLAPSE);
		}
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		TRACE0("Exception in: CdIpmDoc::ItemCollapseOne\n");
	}
}


CTreeItem* CdIpmDoc::GetSelectedTreeItem()
{
	CTreeCtrl* pTree = m_pTreeGD->GetPTree();
	HTREEITEM hItem  = pTree->GetSelectedItem();
	if (hItem) 
	{
		CTreeItem* pItem = (CTreeItem*)pTree->GetItemData(hItem);
		return pItem;
	}
	return NULL;
}


BOOL CdIpmDoc::GetData (IStream** ppStream)
{
	try
	{
		BOOL bOk = CObjectToIStream (this, ppStream);
		if (!bOk)
		{
			*ppStream = NULL;
		}
		return TRUE;
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		TRACE0("Exception in: CdIpmDoc::GetData\n");
	}
	return FALSE;
}

BOOL CdIpmDoc::SetData (IStream* pStream)
{
	try
	{
		BOOL bOk = CObjectFromIStream(this, pStream);
		if (bOk)
		{
			m_bLoaded = TRUE;
		}
		return TRUE;
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (CArchiveException* e)
	{
		e->ReportError();
		e->Delete();
	}
	catch (CFileException* e)
	{
		e->ReportError();
		e->Delete();
	}
	catch (...)
	{
		TRACE0("Exception in: CdIpmDoc::SetData\n");
	}
	return FALSE;
}


BOOL CdIpmDoc::IsEnabledItemShutdown() 
{
	try
	{
		CTreeItem* pSelected = GetSelectedTreeItem();
		BOOL bEnable = pSelected? pSelected->IsEnabled(IDM_IPMBAR_SHUTDOWN): FALSE;
		return bEnable; 
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		TRACE0("Exception in: CdIpmDoc::IsEnabledItemShutdown\n");
	}
	return FALSE;
}

BOOL CdIpmDoc::IsEnabledItemOpenServer()
{
	try
	{
		CTreeItem* pSelected = GetSelectedTreeItem();
		BOOL bEnable = pSelected? pSelected->IsEnabled(IDM_IPMBAR_OPENSVR): FALSE;
		return bEnable; 
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		TRACE0("Exception in: CdIpmDoc::IsEnabledItemOpenServer\n");
	}
	return FALSE;
}

BOOL CdIpmDoc::IsEnabledItemCloseServer()
{
	try
	{
		CTreeItem* pSelected = GetSelectedTreeItem();
		BOOL bEnable = pSelected? pSelected->IsEnabled(IDM_IPMBAR_CLOSESVR): FALSE;
		return bEnable; 
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		TRACE0("Exception in: CdIpmDoc::IsEnabledItemCloseServer\n");
	}
	return FALSE;
}

BOOL  CdIpmDoc::IsEnabledItemReplicationServerChangeNode()
{
	try
	{
		CTreeItem* pSelected = GetSelectedTreeItem();
		BOOL bEnable = pSelected? pSelected->IsEnabled(IDM_IPMBAR_REPLICSVR_CHANGERUNNODE): FALSE;
		return bEnable; 
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		TRACE0("Exception in: CdIpmDoc::IsEnabledItemReplicationServerChangeNode\n");
	}
	return FALSE;
}


void CdIpmDoc::ProhibitActionOnTreeCtrl(BOOL bProhibit)
{
	CTreeCtrl* pTree = m_pTreeGD->GetPTree();
	ASSERT (pTree);
	if (!pTree)
		return;
	CfIpmFrame* pIpmFrame = (CfIpmFrame*)pTree->GetParentFrame();
	CvIpmLeft* pView = (CvIpmLeft*)pIpmFrame->GetLeftPane();
	if (!pView)
		return;
	pView->ProhibitActionOnTreeCtrl(bProhibit);
}


void CdIpmDoc::SearchItem()
{
	CTreeCtrl* pTree = m_pTreeGD->GetPTree();
	ASSERT (pTree);
	if (!pTree)
		return;
	CfIpmFrame* pIpmFrame = (CfIpmFrame*)pTree->GetParentFrame();
	CvIpmLeft* pView = (CvIpmLeft*)pIpmFrame->GetLeftPane();
	if (!pView)
		return;
	pView->SearchItem();
}

void CdIpmDoc::SetSessionStart(long nStart) 
{
	IPM_SetSessionStart(nStart);
}

void CdIpmDoc::BkRefresh()
{
	if (IPM_NeedBkRefresh(this))
	{
		CWaitCursor hourGlass;
		IPMUPDATEPARAMS ups;
		memset (&ups, 0, sizeof(ups));
		ups.nType = 0;
		ups.pStruct = NULL;
		CaIpmQueryInfo queryInfo (this, OT_MON_ALL, &ups);
		IPM_UpdateInfo(&queryInfo);
	}
}

