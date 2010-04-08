/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dgstarco.cpp, Implementation file 
**    Project  : Ingres II/Vdba.
**    Author   : UK Sotheavut
**    Purpose  : Page of Star Database (Component Pane)
**
** History:
** xx-May-1998 (uk$so01)
**    created.
** 21-Fev-2002 (uk$so01)
**    SIR #106648, Integrate ipm.ocx.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgstarco.h"
#include "wmusrmsg.h"

extern "C" 
{
#include "domdata.h"
#include "main.h"
#include "dom.h"
#include "dbaginfo.h"
#include "monitor.h" //IPMUPDATEPARAMS
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define IMG_VNODE     0
#define IMG_VNODE_CX  1 // Connected
#define IMG_SERVER    3
#define IMG_SERVER_CX 4 // Connected
#define IMG_DATABASE  5
#define IMG_TABLE     6
#define IMG_VIEW      7
#define IMG_PROCEDURE 8
#define IMG_INDEX     9


IMPLEMENT_SERIAL (CaStarDbComponent, CObject, 1)
void CaStarDbComponent::Copy (const CaStarDbComponent& c)
{
	m_nType          = c.m_nType ;
	m_strVNode       = c.m_strVNode;
	m_strDatabase    = c.m_strDatabase;
	m_strServer      = c.m_strServer;
	m_strObject      = c.m_strObject;
	m_strObjectOwner = c.m_strObjectOwner;
}

CaStarDbComponent::CaStarDbComponent(STARDBCOMPONENTPARAMS* pParams)
{
	m_nType          = pParams->itype ;
	m_strVNode       = pParams->NodeName;
	m_strDatabase    = pParams->DBName;
	m_strServer      = pParams->ServerClass;
	m_strObject      = pParams->ObjectName;
	m_strObjectOwner = pParams->ObjectOwner;
}

CaStarDbComponent::CaStarDbComponent(const CaStarDbComponent& c)
{
	Copy (c);
}

CaStarDbComponent CaStarDbComponent::operator = (const CaStarDbComponent& c)
{
	if (this != &c)
	{
		Copy (c);
	}
	return *this;
}

void CaStarDbComponent::Serialize (CArchive& ar)
{
	if (ar.IsStoring()) 
	{
		ar << m_nType ;
		ar << m_strVNode;
		ar << m_strDatabase;
		ar << m_strServer;
		ar << m_strObject;
		ar << m_strObjectOwner;
	}
	else 
	{
		ar >> m_nType ;
		ar >> m_strVNode;
		ar >> m_strDatabase;
		ar >> m_strServer;
		ar >> m_strObject;
		ar >> m_strObjectOwner;
	}
}



IMPLEMENT_SERIAL (CaDomPropDataStarDb, CuIpmPropertyData, 1)

CaDomPropDataStarDb::CaDomPropDataStarDb(const CaDomPropDataStarDb& c)
	:CuIpmPropertyData(_T("CaDomPropDataStarDb"))
{
	Copy (c);
}

CaDomPropDataStarDb CaDomPropDataStarDb::operator = (const CaDomPropDataStarDb& c)
{
	if (this != &c)
	{
		Copy (c);
	}
	return *this;
}

void CaDomPropDataStarDb::Copy (const CaDomPropDataStarDb& c)
{
	CStringList* pStrList = NULL;
	CaStarDbComponent* pComponent;
	CaStarDbComponent* pC = NULL;
	POSITION p, pos = c.m_listComponent.GetHeadPosition();
	while (pos != NULL)
	{
		pComponent = c.m_listComponent.GetNext (pos);
		pC = new CaStarDbComponent();
		*pC=*pComponent;
		m_listComponent.AddTail (pC);
	}
	
	pos = c.m_listExpandedNode.GetHeadPosition();
	while (pos != NULL)
	{
		pStrList = c.m_listExpandedNode.GetNext(pos);
		CStringList* pNewStrList = new CStringList();
		p = pStrList->GetHeadPosition();
		while (p != NULL)
		{
			CString strItem = pStrList->GetNext (p);
			pNewStrList->AddTail (strItem);
		}
		m_listExpandedNode.AddTail (pNewStrList);
	}
}

CaDomPropDataStarDb::~CaDomPropDataStarDb()
{
	Cleanup();
}

void CaDomPropDataStarDb::Cleanup()
{
	while (!m_listComponent.IsEmpty())
		delete m_listComponent.RemoveHead();
	while (!m_listExpandedNode.IsEmpty())
	{
		CStringList* pStrList = m_listExpandedNode.RemoveHead();
		delete pStrList;
	}
}

void CaDomPropDataStarDb::Serialize (CArchive& ar)
{
	CuIpmPropertyData::Serialize(ar);
	m_listComponent.Serialize (ar);
	m_listExpandedNode.Serialize (ar);
}

void CaDomPropDataStarDb::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropStarDbComponent dialog


CuDlgDomPropStarDbComponent::CuDlgDomPropStarDbComponent(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgDomPropStarDbComponent::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgDomPropStarDbComponent)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_nHNode = -1;
	m_pData = new CaDomPropDataStarDb();
}


void CuDlgDomPropStarDbComponent::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDomPropStarDbComponent)
	DDX_Control(pDX, IDC_MFC_TREE1, m_cTree1);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropStarDbComponent, CDialog)
	//{{AFX_MSG_MAP(CuDlgDomPropStarDbComponent)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_NOTIFY(TVN_ITEMEXPANDED, IDC_MFC_TREE1, OnItemexpandedTree1)
	ON_NOTIFY(NM_OUTOFMEMORY, IDC_MFC_TREE1, OnOutofmemoryTree1)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,     OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING,   OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,     OnGetData)
	ON_MESSAGE (WM_CHANGE_FONT,   OnChangeFont)
END_MESSAGE_MAP()

LONG CuDlgDomPropStarDbComponent::OnChangeFont (WPARAM wParam, LPARAM lParam)
{
	if (lParam)
		m_cTree1.SetFont (CFont::FromHandle((HFONT)lParam));
	return 0;
}

LONG CuDlgDomPropStarDbComponent::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	int iret = RES_ERR;
	STARDBCOMPONENTPARAMS stParams;
	CaStarDbComponent* pComponent = NULL;

	CWaitCursor waitCursor;
	BOOL bCheck = FALSE;
	m_nHNode = (int)wParam;
	LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;
	ASSERT (m_nHNode != -1);
	ASSERT (pUps);
	if (m_nHNode == -1 || pUps == NULL)
		return 0L;
	switch (pUps->nIpmHint)
	{
	case 0:
		break;
	default:
		//
		// nothing to change on the display
		return 0L;
	}

	LPUCHAR       aparents[3];
	LPTREERECORD  pRecord = (LPTREERECORD)pUps->pStruct;
	ASSERT (pRecord);
	if (!pRecord)
		return 0L;

	aparents[0] = (LPUCHAR)pRecord->objName;
	aparents[1] = NULL;
	aparents[2] = NULL;

	try
	{
		LPCTSTR lpszVNode = (LPCTSTR)GetVirtNodeName(m_nHNode);
		ASSERT (lpszVNode);
		if (!lpszVNode)
			return 0L;
		if (!m_pData)
			m_pData = new CaDomPropDataStarDb();
		else
			m_pData->Cleanup();
		ResetDisplay();

		iret = DBAGetFirstObject ((LPUCHAR)lpszVNode, OT_STARDB_COMPONENT, 1, aparents, TRUE, (LPUCHAR)&stParams, NULL, NULL);
		while (iret == RES_SUCCESS)
		{
			pComponent = new CaStarDbComponent(&stParams);
			m_pData->m_listComponent.AddTail (pComponent);
			iret = DBAGetNextObject ((LPUCHAR)&stParams, NULL, NULL);
		}
		UpdateTree ();
	}
	catch (...)
	{
		TRACE0 ("Critical error: CuDlgDomPropStarDbComponent::OnUpdateData\n");
	}

	return 0L;
}

LONG CuDlgDomPropStarDbComponent::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CaDomPropDataStarDb* pData;
	ASSERT (m_pData);
	try
	{
		if (!m_pData)
			m_pData = new CaDomPropDataStarDb(); // Empty Data
		POSITION pos = m_pData->m_listExpandedNode.GetHeadPosition();
		while (pos != NULL)
		{
			CStringList* pStrList = m_pData->m_listExpandedNode.GetNext(pos);
			delete pStrList;
		}
		StoreExpandedState();
	}
	catch (...)
	{
		//CString strMsg = _T("Cannot save Component pane of Database Star");
		AfxMessageBox (VDBA_MfcResourceString(IDS_E_SAVE_COMPONENT));
		return NULL;
	}

	pData = m_pData;
	m_pData = NULL;
	return (LONG)pData;
}

LONG CuDlgDomPropStarDbComponent::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ASSERT (lstrcmp (pClass, "CaDomPropDataStarDb") == 0);
	CaDomPropDataStarDb* pData = (CaDomPropDataStarDb*)lParam;

	ASSERT (pData);
	if (!pData)
		return 0L;
	if (m_pData)
		delete m_pData;

	try
	{
		ResetDisplay();
		m_pData = new CaDomPropDataStarDb();
		*m_pData = *pData;
	}
	catch (...)
	{
		//CString strMsg = _T("Cannot Load Component pane of Database Star");
		AfxMessageBox (VDBA_MfcResourceString(IDS_E_LOAD_COMPONENT));
		return NULL;
	}

	UpdateTree ();
	return 0L;
}



/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropStarDbComponent message handlers

void CuDlgDomPropStarDbComponent::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

void CuDlgDomPropStarDbComponent::OnDestroy() 
{
	if (m_pData)
		delete m_pData;
	m_pData = NULL;
	CDialog::OnDestroy();
}

BOOL CuDlgDomPropStarDbComponent::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_ImageList.Create (IDB_STAR_DATABASE_COMPONENT, 16, 0, RGB(255,0,255));
	m_cTree1.SetImageList (&m_ImageList, TVSIL_NORMAL);
	if (hFontBrowsers)
	{
		m_cTree1.SetFont (CFont::FromHandle(hFontBrowsers));
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropStarDbComponent::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (IsWindow (m_cTree1.m_hWnd))
	{
		CRect r;
		GetClientRect (r);
		m_cTree1.MoveWindow (r);
	}
}

void CuDlgDomPropStarDbComponent::OnItemexpandedTree1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	
	*pResult = 0;
}

void CuDlgDomPropStarDbComponent::OnOutofmemoryTree1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	VDBA_OutOfMemoryMessage ();
	*pResult = 1;
}

void CuDlgDomPropStarDbComponent::UpdateTree ()
{
	StoreExpandedState();
	m_cTree1.DeleteAllItems();
	CaStarDbComponent* pComponent = NULL;
	ASSERT (m_pData);
	if (!m_pData)
		return;
	POSITION pos = m_pData->m_listComponent.GetHeadPosition();
	while (pos != NULL)
	{
		pComponent = m_pData->m_listComponent.GetNext (pos);
		AddItem (pComponent);
	}
	RestoreExpandedState();
}

void CuDlgDomPropStarDbComponent::AddItem (CaStarDbComponent* pC)
{
	HTREEITEM hRootServer   = NULL;
	HTREEITEM hRootDatabase = NULL;
	HTREEITEM hRootObject   = NULL;
	HTREEITEM hRoot         = m_cTree1.GetRootItem();
	HTREEITEM hRootNode     = FindNode (pC->m_strVNode, hRoot);

	CString strItem = _T("");
	CString strObject;
	strObject.Format (_T("%s.%s"), (LPCTSTR)pC->m_strObjectOwner, (LPCTSTR)pC->m_strObject);
	int nObjImage = -1;
	switch (pC->m_nType)
	{
	case OT_TABLE:
		nObjImage = IMG_TABLE;
		break;
	case OT_VIEW:
		nObjImage = IMG_VIEW;
		break;
	case OT_PROCEDURE:
		nObjImage = IMG_PROCEDURE;
		break;
	case OT_INDEX:
		nObjImage = IMG_INDEX;
		break;
	default:
		break;
	}

	if (!hRootNode)
	{
		hRootNode     = m_cTree1.InsertItem (pC->m_strVNode,   IMG_VNODE, IMG_VNODE, TVI_ROOT,      TVI_LAST);
		hRootServer   = m_cTree1.InsertItem (pC->m_strServer,  IMG_SERVER, IMG_SERVER, hRootNode,     TVI_LAST);
		hRootDatabase = m_cTree1.InsertItem (pC->m_strDatabase,IMG_DATABASE, IMG_DATABASE, hRootServer,   TVI_LAST);
		hRootObject   = m_cTree1.InsertItem (strObject,        nObjImage, nObjImage, hRootDatabase, TVI_LAST);
	}
	else
	{
		hRootServer = FindObject (pC->m_strServer, hRootNode);
		if (!hRootServer)
		{
			hRootServer   = m_cTree1.InsertItem (pC->m_strServer,   IMG_SERVER, IMG_SERVER, hRootNode, TVI_LAST);
			hRootDatabase = m_cTree1.InsertItem (pC->m_strDatabase, IMG_DATABASE, IMG_DATABASE, hRootServer, TVI_LAST);
			hRootObject   = m_cTree1.InsertItem (strObject,         nObjImage, nObjImage, hRootDatabase, TVI_LAST);
		}
		else
		{
			hRootDatabase = FindObject (pC->m_strDatabase, hRootServer);
			if (!hRootDatabase)
			{
				hRootDatabase = m_cTree1.InsertItem (pC->m_strDatabase, IMG_DATABASE, IMG_DATABASE, hRootServer, TVI_LAST);
				hRootObject   = m_cTree1.InsertItem (strObject,         nObjImage, nObjImage, hRootDatabase, TVI_LAST);
			}
			else
			{
				hRootObject = FindObject (strObject, hRootDatabase);
				if (!hRootObject)
				{
					hRootObject = m_cTree1.InsertItem (strObject, nObjImage, nObjImage, hRootDatabase, TVI_LAST);
				}
			}
		}
	}
}



HTREEITEM CuDlgDomPropStarDbComponent::FindNode (LPCTSTR lpszNode,HTREEITEM hRoot)
{
	CString strItem;
	while (hRoot)
	{
		strItem = m_cTree1.GetItemText (hRoot);
		if (strItem.CompareNoCase (lpszNode) == 0)
			return hRoot;
		hRoot = m_cTree1.GetNextSiblingItem (hRoot);
	}
	return NULL;
}


HTREEITEM CuDlgDomPropStarDbComponent::FindObject  (LPCTSTR lpszObject,   HTREEITEM hRootDatabase)
{
	CString strItem;
	HTREEITEM hChild = m_cTree1.GetChildItem (hRootDatabase);
	while (hChild)
	{
		strItem = m_cTree1.GetItemText (hChild);
		if (strItem.CompareNoCase (lpszObject) == 0)
			return hChild;
		hChild = m_cTree1.GetNextSiblingItem (hChild);
	}
	return NULL;
}



void CuDlgDomPropStarDbComponent::StoreExpandedState()
{
	CStringList* pStrList = NULL;
	CString strItem;
	CString strParent1 = _T("");
	CString strParent2 = _T("");
	CString strParent3 = _T("");
	UINT uState = 0;
	HTREEITEM hRoot = m_cTree1.GetRootItem();
	ASSERT (m_pData);
	if (!m_pData)
		return;
	//
	// Virtual Node Level:
	while (hRoot)
	{
		strParent1 = "";
		strItem = m_cTree1.GetItemText (hRoot);
		uState = m_cTree1.GetItemState (hRoot, TVIS_EXPANDED);
		if (uState & TVIS_EXPANDED)
		{
			strParent1 = strItem;
			TRACE1 ("1: %s\n", strParent1);

			pStrList = new CStringList();
			pStrList->AddTail (strParent1);
			m_pData->m_listExpandedNode.AddTail (pStrList);
		}
		else
		{
			//
			// Skip this Virtual Node because it is collapsed.
			hRoot = m_cTree1.GetNextSiblingItem (hRoot);
			continue;
		}
		//
		// Server Level:
		HTREEITEM hChildServer = m_cTree1.GetChildItem (hRoot);
		while (hChildServer)
		{
			strItem = m_cTree1.GetItemText (hChildServer);
			uState = m_cTree1.GetItemState (hChildServer, TVIS_EXPANDED);
			if (uState & TVIS_EXPANDED)
			{
				strParent2 = strItem;
				TRACE2 ("2: %s, %s\n", strParent1, strParent2);

				pStrList = new CStringList();
				pStrList->AddTail (strParent1);
				pStrList->AddTail (strParent2);
				m_pData->m_listExpandedNode.AddTail (pStrList);
			}
			else
			{
				//
				// Skip this Server because it is collapsed.
				hChildServer = m_cTree1.GetNextSiblingItem (hChildServer);
				continue;
			}

			//
			// Database Level:
			HTREEITEM hChildDatabase = m_cTree1.GetChildItem (hChildServer);
			while (hChildDatabase)
			{
				strItem = m_cTree1.GetItemText (hChildDatabase);
				uState = m_cTree1.GetItemState (hChildDatabase, TVIS_EXPANDED);
				if (uState & TVIS_EXPANDED)
				{
					strParent3 = strItem;
					TRACE3 ("3: %s, %s, %s\n", strParent1, strParent2, strParent3);

					pStrList = new CStringList();
					pStrList->AddTail (strParent1);
					pStrList->AddTail (strParent2);
					pStrList->AddTail (strParent3);
					m_pData->m_listExpandedNode.AddTail (pStrList);
				}
				else
				{
					//
					// Skip this Database because it is collapsed.
					hChildDatabase = m_cTree1.GetNextSiblingItem (hChildDatabase);
					continue;
				}
				hChildDatabase = m_cTree1.GetNextSiblingItem (hChildDatabase);
			}
			hChildServer = m_cTree1.GetNextSiblingItem (hChildServer);
		}
		hRoot = m_cTree1.GetNextSiblingItem (hRoot);
	}
}

void CuDlgDomPropStarDbComponent::RestoreExpandedState()
{
	CStringList* pStrList;
	HTREEITEM hItem = NULL;
	ASSERT (m_pData);
	if (!m_pData)
		return;
	POSITION pos = m_pData->m_listExpandedNode.GetHeadPosition();
	while (pos != NULL)
	{
		pStrList = m_pData->m_listExpandedNode.GetNext (pos);
		hItem = FindItem (pStrList);
		if (hItem)
			m_cTree1.Expand (hItem, TVE_EXPAND);
	}
}


HTREEITEM CuDlgDomPropStarDbComponent::FindItem (CStringList* pStrList)
{
	CString strItem = _T("");
	CString strParent1;
	CString strParent2;
	CString strParent3;

	POSITION pos = NULL;
	HTREEITEM hRoot = m_cTree1.GetRootItem();
	int nCount = pStrList->GetCount();
	//
	// Virtual Node Level:
	while (hRoot)
	{
		pos = pStrList->GetHeadPosition();
		if (pos != NULL)
			strParent1 = pStrList->GetNext (pos);
		else
			strParent1 = _T("");
		strItem = m_cTree1.GetItemText (hRoot);
		if (strItem.CompareNoCase (strParent1) !=0)
		{
			//
			// Find the next node
			hRoot = m_cTree1.GetNextSiblingItem (hRoot);
			continue;
		}
		if (nCount == 1)
			return hRoot;
		//
		// Server Level:
		HTREEITEM hChildServer = m_cTree1.GetChildItem (hRoot);
		while (hChildServer && nCount > 1)
		{
			pos = pStrList->GetHeadPosition();
			pStrList->GetNext (pos);
			if (pos)
				strParent2 = pStrList->GetNext (pos);
			else
			strParent2 = _T("");

			strItem = m_cTree1.GetItemText (hChildServer);
			if (strItem.CompareNoCase (strParent2) !=0)
			{
				//
				// Find the next Server
				hChildServer = m_cTree1.GetNextSiblingItem (hChildServer);
				continue;
			}
			if (nCount == 2)
				return hChildServer;
			//
			// Database Level:
			HTREEITEM hChildDatabase = m_cTree1.GetChildItem (hChildServer);
			while (hChildDatabase && nCount > 2)
			{
				pos = pStrList->GetHeadPosition();
				pStrList->GetNext (pos);
				pStrList->GetNext (pos);
				if (pos != NULL)
					strParent3 = pStrList->GetNext (pos);
				else
					strParent3 = _T("");

				strItem = m_cTree1.GetItemText (hChildDatabase);
				if (strItem.CompareNoCase (strParent3) !=0)
				{
					//
					// Find the next Database
					hChildDatabase = m_cTree1.GetNextSiblingItem (hChildDatabase);
					continue;
				}
				if (nCount == 3)
					return hChildDatabase;

				hChildDatabase = m_cTree1.GetNextSiblingItem (hChildDatabase);
			}
			hChildServer = m_cTree1.GetNextSiblingItem (hChildServer);
		}
		hRoot = m_cTree1.GetNextSiblingItem (hRoot);
	}
	return NULL;
}

void CuDlgDomPropStarDbComponent::ResetDisplay()
{
	//
	// Nothing to do !
}