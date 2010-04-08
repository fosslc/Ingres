/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : ipmctl.cpp : Implementation File.
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Implementation of the CIpmCtrl ActiveX Control class.
**
** History:
**
** 12-Nov-2001 (uk$so01)
**    Created
** 19-May-2003 (uk$so01)
**    SIR #110269, Add Network trafic monitoring.
** 17-Jul-2003 (uk$so01)
**    SIR #106648, Vdba-Split, ensure that the ingres sessions 
**    have the descriptions.
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 22-Sep-2004 (uk$so01)
**    BUG #113104 / ISSUE 13690527, 
**    Add method: short GetConnected(LPCTSTR lpszNode, LPCTSTR lpszDatabase)
**    that return 1 if there is an SQL Session.
** 19-Nov-2004 (uk$so01)
**    BUG #113500 / ISSUE #13755297 (Vdba / monitor, references such as background 
**    refresh, or the number of sessions, are not taken into account in vdbamon nor 
**    in the VDBA Monitor functionality)
** 19-Nov-2004 (uk$so01)
**    BUG #113500 / ISSUE #13755297, additional fixes
*/

#include "stdafx.h"
#include "ipm.h"
#include "ipmppg.h"
#include "ipmdoc.h"
#include "ipmframe.h"
#include "cmddrive.h"
#include "wmusrmsg.h"
#include ".\ipmctl.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CIpmCtrl, COleControl)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CIpmCtrl, COleControl)
	//{{AFX_MSG_MAP(CIpmCtrl)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
	ON_MESSAGE (WMUSRMSG_UPDATEDATA,    OnTrackerNotify)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CIpmCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CIpmCtrl)
	DISP_PROPERTY_NOTIFY(CIpmCtrl, "TimeOut", m_timeOut, OnTimeOutChanged, VT_I4)
	DISP_PROPERTY_NOTIFY(CIpmCtrl, "RefreshFrequency", m_refreshFrequency, OnRefreshFrequencyChanged, VT_I4)
	DISP_PROPERTY_NOTIFY(CIpmCtrl, "ActivateRefresh", m_activateRefresh, OnActivateRefreshChanged, VT_BOOL)
	DISP_PROPERTY_NOTIFY(CIpmCtrl, "ShowGrid", m_showGrid, OnShowGridChanged, VT_BOOL)
	DISP_PROPERTY_NOTIFY(CIpmCtrl, "Unit", m_unit, OnUnitChanged, VT_I4)
	DISP_PROPERTY_NOTIFY(CIpmCtrl, "MaxSession", m_maxSession, OnMaxSessionChanged, VT_I4)
	DISP_FUNCTION(CIpmCtrl, "Initiate", Initiate, VT_BOOL, VTS_BSTR VTS_BSTR VTS_BSTR VTS_BSTR)
	DISP_FUNCTION(CIpmCtrl, "ExpandBranch", ExpandBranch, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CIpmCtrl, "ExpandAll", ExpandAll, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CIpmCtrl, "CollapseBranch", CollapseBranch, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CIpmCtrl, "CollapseAll", CollapseAll, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CIpmCtrl, "ExpandOne", ExpandOne, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CIpmCtrl, "CollapseOne", CollapseOne, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CIpmCtrl, "ExpressRefresh", ExpressRefresh, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(CIpmCtrl, "ItemShutdown", ItemShutdown, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(CIpmCtrl, "ForceRefresh", ForceRefresh, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(CIpmCtrl, "OpenServer", OpenServer, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(CIpmCtrl, "CloseServer", CloseServer, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(CIpmCtrl, "ReplicationServerChangeNode", ReplicationServerChangeNode, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(CIpmCtrl, "ResourceTypeChange", ResourceTypeChange, VT_BOOL, VTS_I2)
	DISP_FUNCTION(CIpmCtrl, "NullResource", NullResource, VT_BOOL, VTS_I2)
	DISP_FUNCTION(CIpmCtrl, "InternalSession", InternalSession, VT_BOOL, VTS_I2)
	DISP_FUNCTION(CIpmCtrl, "SystemLockList", SystemLockList, VT_BOOL, VTS_I2)
	DISP_FUNCTION(CIpmCtrl, "InactiveTransaction", InactiveTransaction, VT_BOOL, VTS_I2)
	DISP_FUNCTION(CIpmCtrl, "IsEnabledShutdown", IsEnabledShutdown, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(CIpmCtrl, "IsEnabledOpenServer", IsEnabledOpenServer, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(CIpmCtrl, "IsEnabledCloseServer", IsEnabledCloseServer, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(CIpmCtrl, "IsEnabledReplicationChangeNode", IsEnabledReplicationChangeNode, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(CIpmCtrl, "Loading", Loading, VT_ERROR, VTS_UNKNOWN)
	DISP_FUNCTION(CIpmCtrl, "Storing", Storing, VT_ERROR, VTS_PUNKNOWN)
	DISP_FUNCTION(CIpmCtrl, "ProhibitActionOnTreeCtrl", ProhibitActionOnTreeCtrl, VT_EMPTY, VTS_I2)
	DISP_FUNCTION(CIpmCtrl, "UpdateFilters", UpdateFilters, VT_BOOL, VTS_PI2 VTS_I2)
	DISP_FUNCTION(CIpmCtrl, "SearchItem", SearchItem, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CIpmCtrl, "SetSessionStart", SetSessionStart, VT_EMPTY, VTS_I4)
	DISP_FUNCTION(CIpmCtrl, "SetSessionDescription", SetSessionDescription, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION(CIpmCtrl, "StartExpressRefresh", StartExpressRefresh, VT_EMPTY, VTS_I4)
	DISP_FUNCTION(CIpmCtrl, "SelectItem", SelectItem, VT_BOOL, VTS_BSTR VTS_BSTR VTS_BSTR VTS_BSTR VTS_PVARIANT VTS_I2)
	DISP_FUNCTION(CIpmCtrl, "SetErrorlogFile", SetErrorlogFile, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION(CIpmCtrl, "FindAndSelectTreeItem", FindAndSelectTreeItem, VT_I2, VTS_BSTR VTS_I4)
	DISP_FUNCTION(CIpmCtrl, "GetMonitorObjectsCount", GetMonitorObjectsCount, VT_I4, VTS_NONE)
	DISP_FUNCTION(CIpmCtrl, "GetHelpID", GetHelpID, VT_I4, VTS_NONE)
	DISP_FUNCTION(CIpmCtrl, "SetHelpFile", SetHelpFile, VT_EMPTY, VTS_BSTR)
	DISP_STOCKPROP_FONT()
	//}}AFX_DISPATCH_MAP
	DISP_FUNCTION_ID(CIpmCtrl, "GetConnected", dispidGetConnected, GetConnected, VT_I2, VTS_BSTR VTS_BSTR)
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CIpmCtrl, COleControl)
	//{{AFX_EVENT_MAP(CIpmCtrl)
	EVENT_CUSTOM("IpmTreeSelChange", FireIpmTreeSelChange, VTS_NONE)
	EVENT_CUSTOM("PropertyChange", FirePropertyChange, VTS_NONE)
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

BEGIN_PROPPAGEIDS(CIpmCtrl, 2)
	PROPPAGEID(CIpmPropPage::guid)
	PROPPAGEID(CLSID_CFontPropPage)
END_PROPPAGEIDS(CIpmCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CIpmCtrl, "IPM.IpmCtrl.1",
	0xab474686, 0xe577, 0x11d5, 0x87, 0x8c, 0, 0xc0, 0x4f, 0x1f, 0x75, 0x4a)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CIpmCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DIpm =
		{ 0xab474684, 0xe577, 0x11d5, { 0x87, 0x8c, 0, 0xc0, 0x4f, 0x1f, 0x75, 0x4a } };
const IID BASED_CODE IID_DIpmEvents =
		{ 0xab474685, 0xe577, 0x11d5, { 0x87, 0x8c, 0, 0xc0, 0x4f, 0x1f, 0x75, 0x4a } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwIpmOleMisc =
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CIpmCtrl, IDS_IPM, _dwIpmOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CIpmCtrl::CIpmCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CIpmCtrl

BOOL CIpmCtrl::CIpmCtrlFactory::UpdateRegistry(BOOL bRegister)
{
	// TODO: Verify that your control follows apartment-model threading rules.
	// Refer to MFC TechNote 64 for more information.
	// If your control does not conform to the apartment-model rules, then
	// you must modify the code below, changing the 6th parameter from
	// afxRegApartmentThreading to 0.

	if (bRegister)
		return AfxOleRegisterControlClass(
			AfxGetInstanceHandle(),
			m_clsid,
			m_lpszProgID,
			IDS_IPM,
			IDB_IPM,
			afxRegApartmentThreading,
			_dwIpmOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


/////////////////////////////////////////////////////////////////////////////
// CIpmCtrl::CIpmCtrl - Constructor

CIpmCtrl::CIpmCtrl()
{
	InitializeIIDs(&IID_DIpm, &IID_DIpmEvents);
	m_pIpmFrame = NULL;
}


/////////////////////////////////////////////////////////////////////////////
// CIpmCtrl::~CIpmCtrl - Destructor

CIpmCtrl::~CIpmCtrl()
{
}


/////////////////////////////////////////////////////////////////////////////
// CIpmCtrl::OnDraw - Drawing function

void CIpmCtrl::OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{
	if (m_pIpmFrame)
	{
		m_pIpmFrame->MoveWindow(rcBounds);
	}
}


/////////////////////////////////////////////////////////////////////////////
// CIpmCtrl::DoPropExchange - Persistence support

void CIpmCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

	PX_Long (pPX, _T("TimeOut"),          m_timeOut,           DEFAULT_TIMEOUT);
	PX_Long (pPX, _T("RefreshFrequency"), m_refreshFrequency,  DEFAULT_REFRESHFREQUENCY);
	PX_Bool (pPX, _T("ActivateRefresh"),  m_activateRefresh,   DEFAULT_ACTIVATEREFRESH);
	PX_Bool (pPX, _T("ShowGrid"),         m_showGrid,          DEFAULT_GRID);
	PX_Long (pPX, _T("Unit"),             m_unit,              DEFAULT_UNIT);
	PX_Long (pPX, _T("MaxSession"),       m_maxSession,        DEFAULT_MAXSESSION);

	if (pPX->IsLoading() && m_pIpmFrame)
	{
		CdIpmDoc* pDoc = m_pIpmFrame->GetIpmDoc();
		ASSERT(pDoc);
		if (pDoc)
		{
			CaIpmProperty& property = pDoc->GetProperty();
			ConstructPropertySet (property);
		}
	}
}


/////////////////////////////////////////////////////////////////////////////
// CIpmCtrl::OnResetState - Reset control to default state

void CIpmCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange
}


/////////////////////////////////////////////////////////////////////////////
// CIpmCtrl message handlers

int CIpmCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (COleControl::OnCreate(lpCreateStruct) == -1)
		return -1;
	theApp.GetControlTracker().Add(m_hWnd);
	theApp.StartDatachangeNotificationThread();
	return 0;
}

BOOL CIpmCtrl::Initiate(LPCTSTR lpszNode, LPCTSTR lpszServer, LPCTSTR lpszUser, LPCTSTR lpszOption) 
{
	BOOL bOK = FALSE;
	if (m_pIpmFrame)
	{
		m_pIpmFrame->ShowWindow (SW_HIDE);
		m_pIpmFrame->DestroyWindow();
		m_pIpmFrame = NULL;
	}

	CWnd* pParent = this;
	CRect r;
	GetClientRect (r);
	m_pIpmFrame = new CfIpmFrame();
	m_pIpmFrame->Create (
		NULL,
		NULL,
		WS_CHILD,
		r,
		pParent);
	m_pIpmFrame->InitialUpdateFrame(NULL, TRUE);
	m_pIpmFrame->ShowWindow(SW_HIDE);

	CdIpmDoc* pDoc = m_pIpmFrame->GetIpmDoc();
	ASSERT(pDoc);
	if (pDoc)
	{
		CaIpmProperty& property = pDoc->GetProperty();
		ConstructPropertySet (property);
		UINT nMask = IPMMASK_TIMEOUT|IPMMASK_BKREFRESH|IPMMASK_SHOWGRID|IPMMASK_FONT|IPMMASK_MAXSESSION;
		IPM_PropertiesChange((WPARAM)nMask, (LPARAM)&property);
		theApp.SetRefreshFrequency(m_refreshFrequency);
		theApp.SetActivateBkRefresh(m_activateRefresh);
		theApp.StartBackgroundRefreshThread();
	}

	ASSERT(m_pIpmFrame); // Must create the window first
	if (!m_pIpmFrame)
		return FALSE;
	if (pDoc)
	{
		SFILTER* pFilter = theApp.GetFilter();
		SFILTER* pDocFilter = pDoc->GetFilter();
		memcpy (pDocFilter, pFilter, sizeof(SFILTER));
		bOK = pDoc->Initiate(lpszNode, lpszServer, lpszUser, lpszOption);
	}
	if (!bOK)
	{
		m_pIpmFrame->ShowWindow (SW_HIDE);
		m_pIpmFrame->DestroyWindow();
		m_pIpmFrame = NULL;
		return bOK;
	}
	m_pIpmFrame->ShowWindow(SW_SHOW);
	return bOK;
}

void CIpmCtrl::ExpandBranch() 
{
	ASSERT(m_pIpmFrame); // Must create the window first
	if (!m_pIpmFrame)
		return;
	CdIpmDoc* pDoc = m_pIpmFrame->GetIpmDoc();
	ASSERT(pDoc);
	if (pDoc)
		pDoc->ItemExpandBranch();
}

void CIpmCtrl::ExpandAll() 
{
	ASSERT(m_pIpmFrame); // Must create the window first
	if (!m_pIpmFrame)
		return;
	CdIpmDoc* pDoc = m_pIpmFrame->GetIpmDoc();
	ASSERT(pDoc);
	if (pDoc)
		pDoc->ItemExpandAll();
}

void CIpmCtrl::CollapseBranch() 
{
	ASSERT(m_pIpmFrame); // Must create the window first
	if (!m_pIpmFrame)
		return;
	CdIpmDoc* pDoc = m_pIpmFrame->GetIpmDoc();
	ASSERT(pDoc);
	if (pDoc)
		pDoc->ItemCollapseBranch();
}

void CIpmCtrl::CollapseAll() 
{
	ASSERT(m_pIpmFrame); // Must create the window first
	if (!m_pIpmFrame)
		return;
	CdIpmDoc* pDoc = m_pIpmFrame->GetIpmDoc();
	ASSERT(pDoc);
	if (pDoc)
		pDoc->ItemCollapseAll();
}

void CIpmCtrl::ExpandOne() 
{
	ASSERT(m_pIpmFrame); // Must create the window first
	if (!m_pIpmFrame)
		return;
	CdIpmDoc* pDoc = m_pIpmFrame->GetIpmDoc();
	ASSERT(pDoc);
	if (pDoc)
		pDoc->ItemExpandOne();
}

void CIpmCtrl::CollapseOne() 
{
	ASSERT(m_pIpmFrame); // Must create the window first
	if (!m_pIpmFrame)
		return;
	CdIpmDoc* pDoc = m_pIpmFrame->GetIpmDoc();
	ASSERT(pDoc);
	if (pDoc)
		pDoc->ItemCollapseOne();
}

BOOL CIpmCtrl::ExpressRefresh() 
{
	ASSERT(m_pIpmFrame); // Must create the window first
	if (!m_pIpmFrame)
		return FALSE;
	CdIpmDoc* pDoc = m_pIpmFrame->GetIpmDoc();
	ASSERT(pDoc);
	if (!pDoc)
		return FALSE;
	return pDoc->ExpresRefresh();
}

BOOL CIpmCtrl::ItemShutdown() 
{
	ASSERT(m_pIpmFrame); // Must create the window first
	if (!m_pIpmFrame)
		return FALSE;
	CdIpmDoc* pDoc = m_pIpmFrame->GetIpmDoc();
	ASSERT(pDoc);
	if (!pDoc)
		return FALSE;
	if (!pDoc->IsEnabledItemShutdown())
		return FALSE;
	return pDoc->ItemShutdown();
}

BOOL CIpmCtrl::ForceRefresh() 
{
	ASSERT(m_pIpmFrame); // Must create the window first
	if (!m_pIpmFrame)
		return FALSE;
	CdIpmDoc* pDoc = m_pIpmFrame->GetIpmDoc();
	ASSERT(pDoc);
	if (!pDoc)
		return FALSE;
	return pDoc->ForceRefresh();
}

BOOL CIpmCtrl::OpenServer() 
{
	ASSERT(m_pIpmFrame); // Must create the window first
	if (!m_pIpmFrame)
		return FALSE;
	CdIpmDoc* pDoc = m_pIpmFrame->GetIpmDoc();
	ASSERT(pDoc);
	if (!pDoc)
		return FALSE;
	if (!pDoc->IsEnabledItemOpenServer())
		return FALSE;
	return pDoc->ItemOpenServer();
}

BOOL CIpmCtrl::CloseServer() 
{
	ASSERT(m_pIpmFrame); // Must create the window first
	if (!m_pIpmFrame)
		return FALSE;
	CdIpmDoc* pDoc = m_pIpmFrame->GetIpmDoc();
	ASSERT(pDoc);
	if (!pDoc)
		return FALSE;
	if (!pDoc->IsEnabledItemCloseServer())
		return FALSE;
	return pDoc->ItemCloseServer();
}

BOOL CIpmCtrl::ReplicationServerChangeNode() 
{
	ASSERT(m_pIpmFrame); // Must create the window first
	if (!m_pIpmFrame)
		return FALSE;
	CdIpmDoc* pDoc = m_pIpmFrame->GetIpmDoc();
	ASSERT(pDoc);
	if (!pDoc)
		return FALSE;
	if (!pDoc->IsEnabledItemReplicationServerChangeNode())
		return FALSE;
	return pDoc->ItemReplicationServerChangeNode();
}

BOOL CIpmCtrl::ResourceTypeChange(short nResType) 
{
	ASSERT(m_pIpmFrame); // Must create the window first
	if (!m_pIpmFrame)
		return FALSE;
	CdIpmDoc* pDoc = m_pIpmFrame->GetIpmDoc();
	ASSERT(pDoc);
	if (!pDoc)
		return FALSE;
	return pDoc->ResourceTypeChange(nResType);
}

BOOL CIpmCtrl::NullResource(short bSet) 
{
	ASSERT(m_pIpmFrame); // Must create the window first
	if (!m_pIpmFrame)
		return FALSE;
	CdIpmDoc* pDoc = m_pIpmFrame->GetIpmDoc();
	ASSERT(pDoc);
	if (!pDoc)
		return FALSE;
	return pDoc->FilterChange(FILTER_NULL_RESOURCES, (BOOL)bSet);
}

BOOL CIpmCtrl::InternalSession(short bSet) 
{
	ASSERT(m_pIpmFrame); // Must create the window first
	if (!m_pIpmFrame)
		return FALSE;
	CdIpmDoc* pDoc = m_pIpmFrame->GetIpmDoc();
	ASSERT(pDoc);
	if (!pDoc)
		return FALSE;
	return pDoc->FilterChange(FILTER_INTERNAL_SESSIONS, (BOOL)bSet);
}

BOOL CIpmCtrl::SystemLockList(short bSet) 
{
	ASSERT(m_pIpmFrame); // Must create the window first
	if (!m_pIpmFrame)
		return FALSE;
	CdIpmDoc* pDoc = m_pIpmFrame->GetIpmDoc();
	ASSERT(pDoc);
	if (!pDoc)
		return FALSE;
	return pDoc->FilterChange(FILTER_SYSTEM_LOCK_LISTS, (BOOL)bSet);
}

BOOL CIpmCtrl::InactiveTransaction(short bSet) 
{
	ASSERT(m_pIpmFrame); // Must create the window first
	if (!m_pIpmFrame)
		return FALSE;
	CdIpmDoc* pDoc = m_pIpmFrame->GetIpmDoc();
	ASSERT(pDoc);
	if (!pDoc)
		return FALSE;
	return pDoc->FilterChange(FILTER_INACTIVE_TRANSACTIONS, (BOOL)bSet);
}

BOOL CIpmCtrl::IsEnabledShutdown() 
{
	ASSERT(m_pIpmFrame); // Must create the window first
	if (!m_pIpmFrame)
		return FALSE;
	CdIpmDoc* pDoc = m_pIpmFrame->GetIpmDoc();
	ASSERT(pDoc);
	if (!pDoc)
		return FALSE;
	return pDoc->IsEnabledItemShutdown();
}

BOOL CIpmCtrl::IsEnabledOpenServer() 
{
	ASSERT(m_pIpmFrame); // Must create the window first
	if (!m_pIpmFrame)
		return FALSE;
	CdIpmDoc* pDoc = m_pIpmFrame->GetIpmDoc();
	ASSERT(pDoc);
	if (!pDoc)
		return FALSE;
	return pDoc->IsEnabledItemOpenServer();
}

BOOL CIpmCtrl::IsEnabledCloseServer() 
{
	ASSERT(m_pIpmFrame); // Must create the window first
	if (!m_pIpmFrame)
		return FALSE;
	CdIpmDoc* pDoc = m_pIpmFrame->GetIpmDoc();
	ASSERT(pDoc);
	if (!pDoc)
		return FALSE;
	return pDoc->IsEnabledItemCloseServer();
}

BOOL CIpmCtrl::IsEnabledReplicationChangeNode() 
{
	ASSERT(m_pIpmFrame); // Must create the window first
	if (!m_pIpmFrame)
		return FALSE;
	CdIpmDoc* pDoc = m_pIpmFrame->GetIpmDoc();
	ASSERT(pDoc);
	if (!pDoc)
		return FALSE;
	return pDoc->IsEnabledItemReplicationServerChangeNode();
}

SCODE CIpmCtrl::Loading(LPUNKNOWN pStream) 
{
	//
	// 1) Destroy the old frame first:
	//    m_pIpmFrame->ShowWindow (SW_HIDE)
	//    m_pIpmFrame->DestroyWindow();
	//    delete m_pIpmFrame ?
	// 2) Construct the Document from Stream.
	//    CdIpmDoc* pLoadedDoc = new CdIpmDoc();
	//    pLoadedDoc->SetData(pSream);
	// 3) Create new Frame associated with the new Doc
	//    m_pIpmFrame = new CfIpmFrame (pLoadedDoc);
	//    m_pIpmFrame->Create (...)
	//    ...
	//    m_pIpmFrame->ShowWindow (SW_SHOW)
	if (m_pIpmFrame && IsWindow(m_pIpmFrame->m_hWnd))
	{
		m_pIpmFrame->ShowWindow (SW_HIDE);
		m_pIpmFrame->DestroyWindow();
		m_pIpmFrame = NULL;
	}

	CdIpmDoc* pDoc = new CdIpmDoc();
	IStream* pSream = (IStream*)pStream;
	BOOL bOK = pDoc->SetData(pSream);
	if (!bOK)
	{
		delete pDoc;
		return S_FALSE;
	}

	CaIpmProperty& property = pDoc->GetProperty();
	ConstructPropertySet (property);

	CWnd* pParent = this;
	CRect r;
	GetWindowRect (r);
	m_pIpmFrame = new CfIpmFrame(pDoc);
	m_pIpmFrame->Create (
		NULL,
		NULL,
		WS_CHILD,
		r,
		pParent);
	m_pIpmFrame->InitialUpdateFrame(pDoc, TRUE);
	m_pIpmFrame->ShowWindow(SW_SHOW);

	return bOK? S_OK: S_FALSE;
}

SCODE CIpmCtrl::Storing(LPUNKNOWN FAR* ppStream) 
{
	ASSERT(m_pIpmFrame); // Must create the window first
	if (!m_pIpmFrame)
		return S_FALSE;
	CdIpmDoc* pDoc = m_pIpmFrame->GetIpmDoc();
	ASSERT(pDoc);
	if (!pDoc)
		return S_FALSE;
	IStream* pSream = NULL;
	BOOL bOK = pDoc->GetData(&pSream);
	*ppStream = (LPUNKNOWN)pSream;
	return bOK? S_OK: S_FALSE;
}

void CIpmCtrl::OnDestroy() 
{
	theApp.GetControlTracker().Remove(m_hWnd);

	COleControl::OnDestroy();
	if (m_pIpmFrame)
	{
		m_pIpmFrame->DestroyWindow();
	}
}

void CIpmCtrl::ProhibitActionOnTreeCtrl(short nYes) 
{
	BOOL bProhibit = (BOOL)nYes;
	ASSERT(m_pIpmFrame); // Must create the window first
	if (!m_pIpmFrame)
		return;
	CdIpmDoc* pDoc = m_pIpmFrame->GetIpmDoc();
	ASSERT(pDoc);
	if (!pDoc)
		return;
	pDoc->ProhibitActionOnTreeCtrl(bProhibit);
}

BOOL CIpmCtrl::UpdateFilters(short FAR* arrayFilter, short nArraySize) 
{
	SFILTER* pFilter = theApp.GetFilter();
	ASSERT(nArraySize >= 5);
	int nPos = 0;
	pFilter->bNullResources = (BOOL)arrayFilter[nPos]; nPos++;
	pFilter->bInternalSessions = (BOOL)arrayFilter[nPos]; nPos++;
	pFilter->bSystemLockLists = (BOOL)arrayFilter[nPos]; nPos++;
	pFilter->bInactiveTransactions = (BOOL)arrayFilter[nPos]; nPos++;
	pFilter->ResourceType = arrayFilter[nPos]; nPos++;

	if (!m_pIpmFrame)
		return FALSE;
	CdIpmDoc* pDoc = m_pIpmFrame->GetIpmDoc();
	ASSERT(pDoc);
	if (!pDoc)
		return FALSE;

	SFILTER* pDocFilter = pDoc->GetFilter();
	memcpy (pDocFilter, pFilter, sizeof(SFILTER));

	return TRUE;
}

void CIpmCtrl::SearchItem() 
{
	ASSERT(m_pIpmFrame); // Must create the window first
	if (!m_pIpmFrame)
		return;
	CdIpmDoc* pDoc = m_pIpmFrame->GetIpmDoc();
	ASSERT(pDoc);
	if (!pDoc)
		return;
	pDoc->SearchItem();
}

void CIpmCtrl::SetSessionStart(long nStart) 
{
	IPM_SetSessionStart(nStart);
}

void CIpmCtrl::SetSessionDescription(LPCTSTR lpszSessionDescription) 
{
	IPM_SetSessionDescription(lpszSessionDescription);
}


LONG CIpmCtrl::OnTrackerNotify (WPARAM wParam, LPARAM lParam)
{
	if (!m_pIpmFrame)
		return 0;
	int nMode = (int)wParam;
	CdIpmDoc* pDoc = m_pIpmFrame->GetIpmDoc();
	ASSERT(pDoc);
	if (!pDoc)
		return 0;
	if (pDoc->IsLoadedDoc())
		return 0;
	if (nMode == 0)
	{
		pDoc->UpdateDisplay();
	}
	else
	if (nMode == 1)
	{
		pDoc->BkRefresh();
	}
	return 0;
}

void CIpmCtrl::ConstructPropertySet(CaIpmProperty& property)
{
	property.SetTimeout(m_timeOut); 
	property.SetRefreshFrequency(m_refreshFrequency);
	property.SetRefreshActivated(m_activateRefresh);
	property.SetGrid(m_showGrid);
	property.SetUnit(m_unit);
	property.SetMaxSession(m_maxSession);
	CFontHolder& fontHolder = InternalGetFont();
	property.SetFont(fontHolder.GetFontHandle()); 
}

void CIpmCtrl::NotifySettingChange(UINT nMask)
{
	FirePropertyChange();
	if (m_pIpmFrame && IsWindow (m_pIpmFrame->m_hWnd))
	{
		CdIpmDoc* pDoc = m_pIpmFrame->GetIpmDoc();
		ASSERT(pDoc);
		if (pDoc)
		{
			CaIpmProperty& property = pDoc->GetProperty();
			ConstructPropertySet (property);
			m_pIpmFrame->PostMessage(WMUSRMSG_CHANGE_SETTING, (WPARAM)nMask, (LPARAM)&property);
		}
		return;
	}
	CaIpmProperty property;
	ConstructPropertySet (property);
	IPM_PropertiesChange((WPARAM)nMask, (LPARAM)&property);
}

void CIpmCtrl::OnTimeOutChanged() 
{
	NotifySettingChange(IPMMASK_TIMEOUT);
	SetModifiedFlag();
}

void CIpmCtrl::OnRefreshFrequencyChanged() 
{
	theApp.SetRefreshFrequency(m_refreshFrequency);
	NotifySettingChange(IPMMASK_BKREFRESH);
	SetModifiedFlag();
}

void CIpmCtrl::OnActivateRefreshChanged() 
{
	theApp.SetActivateBkRefresh(m_activateRefresh);
	theApp.StartBackgroundRefreshThread();
	NotifySettingChange(IPMMASK_BKREFRESH);
	SetModifiedFlag();
}

void CIpmCtrl::OnShowGridChanged() 
{
	NotifySettingChange(IPMMASK_SHOWGRID);
	SetModifiedFlag();
}

void CIpmCtrl::OnFontChanged() 
{
	NotifySettingChange(IPMMASK_FONT);
	COleControl::OnFontChanged();
}

void CIpmCtrl::OnUnitChanged() 
{
	NotifySettingChange(IPMMASK_BKREFRESH);
	SetModifiedFlag();
}

void CIpmCtrl::OnMaxSessionChanged() 
{
	NotifySettingChange(IPMMASK_MAXSESSION);
	SetModifiedFlag();
}

void CIpmCtrl::StartExpressRefresh(long nSeconds) 
{
	if (!m_pIpmFrame)
		return;
	m_pIpmFrame->StartExpressRefresh(nSeconds);
}

BOOL CIpmCtrl::SelectItem(LPCTSTR lpszNode, LPCTSTR lpszServer, LPCTSTR lpszUser, LPCTSTR lpszKey, VARIANT FAR* pArrayItem, short nShowTree)
{
	class CaLockDisplayRightPaneProperty
	{
	public:
		CaLockDisplayRightPaneProperty(CdIpmDoc* pDoc):m_pDoc(pDoc), m_bLock(FALSE){}
		~CaLockDisplayRightPaneProperty(){UnLock();}

		void Lock(){m_pDoc->SetLockDisplayRightPane(TRUE);}
		void UnLock(){m_pDoc->SetLockDisplayRightPane(FALSE);}

	protected:
		BOOL m_bLock;
		CdIpmDoc* m_pDoc;
	};

	CWaitCursor doWaitCursor ;
	BOOL bOK = FALSE;
	try 
	{
		CdIpmDoc* pDoc = new CdIpmDoc(lpszKey, pArrayItem, (BOOL)nShowTree);

		if (m_pIpmFrame)
		{
			m_pIpmFrame->ShowWindow (SW_HIDE);
			m_pIpmFrame->DestroyWindow();
			m_pIpmFrame = NULL;
		}

		CaIpmProperty& property = pDoc->GetProperty();
		ConstructPropertySet (property);

		SFILTER* pFilter = theApp.GetFilter();
		SFILTER* pDocFilter = pDoc->GetFilter();
		memcpy (pDocFilter, pFilter, sizeof(SFILTER));
		CaLockDisplayRightPaneProperty lock(pDoc);
		if (nShowTree == 1)
		{
			//
			// During the process of expanding tree up to search item,
			// prohibit the display of the properties of the selected tree item
			lock.Lock();
		}

		CWnd* pParent = this;
		CRect r;
		GetClientRect (r);
		m_pIpmFrame = new CfIpmFrame(pDoc);
		m_pIpmFrame->Create (
			NULL,
			NULL,
			WS_CHILD,
			r,
			pParent);
		if (!m_pIpmFrame)
			return FALSE;

		m_pIpmFrame->InitialUpdateFrame(NULL, TRUE);
		m_pIpmFrame->ShowWindow(SW_SHOW);
		bOK = pDoc->Initiate(lpszNode, lpszServer, lpszUser, NULL);
		if (!bOK)
			return FALSE;

		CTypedPtrList<CObList, CuIpmTreeFastItem*> listItemPath;
		bOK = IPM_BuildItemPath (pDoc, listItemPath);
		if (!bOK)
		{
			while (!listItemPath.IsEmpty())
				delete listItemPath.RemoveHead();
			return FALSE;
		}
		ExpandUpToSearchedItem(m_pIpmFrame, listItemPath, TRUE);
		lock.UnLock();
		//
		// Cleanup:
		while (!listItemPath.IsEmpty())
			delete listItemPath.RemoveHead();

		//
		// Fetch the selected tree item and display the corresponding right pane:
		IPMUPDATEPARAMS ups;
		CTreeGlobalData* pGD = pDoc->GetPTreeGD();
		ASSERT (pGD);
		if (!pGD)
			return FALSE;
		CTreeCtrl* pTree = pGD->GetPTree();
		ASSERT (pTree);
		if (!pTree)
			return FALSE;
		CuDlgIpmTabCtrl* pTabDlg  = (CuDlgIpmTabCtrl*)m_pIpmFrame->GetTabDialog();
		if (!pTabDlg)
			return FALSE;
		HTREEITEM hSelected = pTree->GetSelectedItem();
		if (!hSelected)
			return FALSE;
		//
		// Notify the container of sel change:
		ContainerNotifySelChange();
		memset (&ups, 0, sizeof (ups));
		CTreeItem* pItem = (CTreeItem*)pTree->GetItemData (hSelected);
		if (pItem->IsNoItem() || pItem->IsErrorItem())
		{
			pTabDlg->DisplayPage (NULL);
			return TRUE;
		}
		if (pItem->ItemDisplaysNoPage()) 
		{
			CString caption = pItem->ItemNoPageCaption();
			pTabDlg->DisplayPage (NULL, caption);
			return TRUE;
		}

		if (pItem->HasReplicMonitor())
		{
			pDoc->InitializeReplicator(pItem->GetDBName());
		}

		int nImage = -1, nSelectedImage = -1;
		CImageList* pImageList = pTree->GetImageList (TVSIL_NORMAL);
		HICON hIcon = NULL;
		int nImageCount = pImageList? pImageList->GetImageCount(): 0;
		if (pImageList && pTree->GetItemImage(hSelected, nImage, nSelectedImage))
		{
			if (nImage < nImageCount)
				hIcon = pImageList->ExtractIcon(nImage);
		}
		CuPageInformation* pPageInfo = pItem->GetPageInformation();
		ASSERT(pPageInfo);
		if (!pPageInfo)
			return FALSE;
		int nSelectTab = GetDefaultSelectTab (pDoc);

		CString strItem = pItem->GetRightPaneTitle();
		pItem->UpdateDataWhenSelChange(); // Has an effect only if the class has specialied the method.
		ups.nType   = pItem->GetType();
		ups.pStruct = pItem->GetPTreeItemData()? pItem->GetPTreeItemData()->GetDataPtr(): NULL;
		ups.pSFilter= pDoc->GetPTreeGD()->GetPSFilter();
		pPageInfo->SetUpdateParam (&ups);
		pPageInfo->SetTitle ((LPCTSTR)strItem, pItem, pDoc);
		pPageInfo->SetImage  (hIcon);
		pTabDlg->DisplayPage (pPageInfo, NULL, nSelectTab);

		return TRUE;
	}
	catch(CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch(CResourceException* e)
	{
		AfxMessageBox (IDS_E_LOAD_DLG);
		e->Delete();
	}
	catch(...)
	{
		AfxMessageBox (IDS_E_CONSTRUCT_PROPERTY);
	}
	return FALSE;
}

void CIpmCtrl::SetErrorlogFile(LPCTSTR lpszFullPathFile) 
{
	IPM_ErrorLogFile(lpszFullPathFile);
}

short CIpmCtrl::FindAndSelectTreeItem(LPCTSTR lpszText, long nMode) 
{
	if (m_pIpmFrame && IsWindow (m_pIpmFrame->m_hWnd))
	{
		return m_pIpmFrame->FindItem(lpszText, (UINT)nMode);
	}
	return 0;
}

long CIpmCtrl::GetMonitorObjectsCount() 
{
	if (m_pIpmFrame && IsWindow (m_pIpmFrame->m_hWnd))
	{
		return m_pIpmFrame->GetMonitorObjectsCount();
	}
	return -1;
}

long CIpmCtrl::GetHelpID() 
{
	if (m_pIpmFrame && IsWindow (m_pIpmFrame->m_hWnd))
	{
		return m_pIpmFrame->GetHelpID();
	}
	return 0;
}

void CIpmCtrl::SetHelpFile(LPCTSTR lpszFileWithoutPath) 
{
	theApp.m_strHelpFile = lpszFileWithoutPath;
}

SHORT CIpmCtrl::GetConnected(LPCTSTR lpszNode, LPCTSTR lpszDatabase)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if (DBACloseNodeSessions((LPUCHAR)lpszNode, TRUE,TRUE) != RES_SUCCESS)
		return 1;

	return 0;
}
