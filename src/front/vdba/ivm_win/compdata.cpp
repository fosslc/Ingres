/*
**  Copyright (C) 2005-2010 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : compdata.cpp , Implementation File 
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Manipulation Ingres Visual Manager Data
**
** History:
**
** 15-Dec-1998 (uk$so01)
**    Created
** 03-Mar-2000 (uk$so01)
**    SIR #100706. Provide the different context help id.
** 03-Mar-2000 (noifr01)
**    bug 100714. a terminating \0 was not set at the right place in
**    a date formatting function
** 08-Mar-2000 (uk$so01)
**    SIR #100706. Provide the different context help id.
**    Use the specific defined help ID instead of calculating.
**    Handle the generic folder of gateways, 
** 31-May-2000 (uk$so01)
**    bug 99242 Handle DBCS
** 18-Jan-2001 (noifr01)
**    (SIR 103727) manage JDBC server
** 18-Jan-2001 (uk$so01)
**    SIR #103727 (Make IVM support JDBC Server)
**    Add help management.
** 12-Fev-2001 (noifr01)
**    bug 102974 (manage multiline errlog.log messages)
** 28-May-2001 (uk$so01)
**    SIR #103727 (Activate Help for JDBC)
** 14-Aug-2001 (uk$so01)
**    SIR #105383., Remove meber CaTreeComponentItemData::MakeActiveTab() and
**    CaTreeComponentInstallationItemData::MakeActiveTab()
** 10-nov-2001 (somsa01)
**    Cleaned up 64-bit compiler warnings.
** 07-Jun-2002 (uk$so01)
**    SIR #107951, Display date format according to the LOCALE.
** 02-Oct-2002 (noifr01)
**    (SIR 107587) have IVM manage DB2UDB gateway (mapping the GUI to
**    oracle gateway code until this is moved to generic management)
** 04-Oct-2002 (noifr01)
**    (bug 108868) display "gateways" instead of "gateway" for the gateways
**    parent branch. put that string into the resource file
** 21-Nov-2002 (noifr01)
**    (bug 109143) management of gateway messages
** 19-Dec-2002 (schph01)
**    bug 109330 Implement ExistInstance() method, used in derived class to
**    update the "Stop" option in popup menu.
** 07-May-2003 (uk$so01)
**    BUG #110195, issue #11179264. Ivm show the alerted messagebox to indicate that
**    there are alterted messages but the are not displayed in the right pane
**    according to the preferences
** 02-Feb-2004 (noifr01)
**    removed any unneeded reference to iostream libraries, including now
**    unused internal test code 
** 09-Mar-2004 (uk$so01)
**    SIR #110013 (Handle the DAS Server)
** 25-Feb-2007 (bonro01)
**    Remove JDBC package.
** 25-Jul-2005 (drivi01)
**	  For Vista, update code to use winstart to start/stop servers
**	  whenever start/stop menu is activated.  IVM will run as a user
**	  application and will elevate privileges to start/stop by
**	  calling ShellExecuteEx function, since ShellExecuteEx can not
**	  subscribe to standard input/output, we must use winstart to
**	  stop/start ingres.
** 18-Aug-2010 (lunbr01)  bug 124216
**    Remove code that depended on the local server listen addresses
**    being in the format used for named pipes, and instead use the
**    code for Unix that doesn't depend on the format of the listen
**    addresses. This is needed on Windows to support using protocol
**    "tcp_ip" as the local IPC rather than the default named pipes.
**    IVM code should not depend on the format of the listen addresses.
**    "#ifdef MAINWIN" had been used to generate the Unix version of
**    the code.  Hence, the fix is to keep the MAINWIN code and strip
**    out the non-MAINWIN code that was used to detect and parse pipe
**    names.
**/

#include "stdafx.h"
#include "compdata.h"
#include "compcat.h"
#include "ivmdisp.h"
#include "ivm.h"
#include "ll.h"
#include <compat.h>
#include <gvcl.h>


#include "ivmdml.h"
#include "evtcats.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static HTREEITEM IVM_TreeAddItem (LPCTSTR lpszItem, CTreeCtrl* pTree, HTREEITEM hParent, HTREEITEM hInsertAfter, int nImage, DWORD_PTR dwData)
{
	TV_ITEM tvi; 
	TV_INSERTSTRUCT tvins; 
	
	tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	//
	// Set the text of the item. 
	tvi.pszText = (LPTSTR)lpszItem; 
	tvi.cchTextMax = lstrlen(lpszItem); 
	
	tvi.iImage = nImage;
	tvi.iSelectedImage = nImage; 
	tvins.item = tvi; 
	tvins.hInsertAfter = hInsertAfter;
	tvins.hParent = hParent;
	
	HTREEITEM hItem = pTree->InsertItem(&tvins);
	if (hItem)
		pTree->SetItemData (hItem, dwData);
	return hItem; 
}


/////////////////////////////////////////////////////////////////////////////
// CaTreeComponentItemData

CaTreeComponentItemData::CaTreeComponentItemData()
{
	m_bFolder= FALSE;
	m_nOrder = 0;
	m_bAllowDuplicate = FALSE;
	m_strComponentTypes= _T("");
	m_strComponentType = _T("");
	m_strComponentName = _T("");
	m_strComponentInstance = _T("");
	m_pPageInfo = NULL;

	m_nStartupCount = 0;
	m_nComponentType = -1;

	m_bDisplayInstance = TRUE;
	//
	// List of Instances:
	m_pListInstance = NULL;
	m_nItemStatus = COMP_NORMAL;
	m_bQueryInstance = FALSE;

	m_strStart = _T("");
	m_strStop = _T("");

	m_bIsInstance = FALSE;
}

CaTreeComponentItemData::~CaTreeComponentItemData()
{
	if (m_pPageInfo)
		delete m_pPageInfo;
	if (m_pListInstance)
	{
		while (!m_pListInstance->IsEmpty())
			delete m_pListInstance->RemoveHead();
		delete m_pListInstance;
		m_pListInstance = NULL;
	}
}


void CaTreeComponentItemData::Display (int& nTreeChanged, CTreeCtrl* pTree, HTREEITEM hParent, int nMode)
{
	HTREEITEM hItem;
	CString strDisplay = (nMode == 0)? m_strComponentTypes: m_strComponentName;
	hItem = m_treeCtrlData.GetTreeItem();
	CaTreeCtrlData::ItemState state = m_treeCtrlData.GetState();
	//
	// Delete Old Item:
	if (m_treeCtrlData.GetState() == CaTreeCtrlData::ITEM_DELETE && hItem != NULL)
	{
		pTree->DeleteItem (hItem);
		nTreeChanged |= COMPONENT_REMOVED;
		return;
	}

	//
	// Modify exiting item (its intances ...):
	if (state == CaTreeCtrlData::ITEM_EXIST && hItem != NULL)
	{
		if (m_bDisplayInstance && m_pListInstance && !m_pListInstance->IsEmpty())
		{
			CaTreeComponentItemData* pItemInstance = NULL;
			POSITION pos = m_pListInstance->GetHeadPosition();
			while (pos != NULL)
			{
				pItemInstance = m_pListInstance->GetNext (pos);
				pItemInstance->SetDisplayInstance (m_bDisplayInstance);
				pItemInstance->Display (nTreeChanged, pTree, hItem, 1);
			}
		}
	}
	
	//
	// Display New Item:
	if (state != CaTreeCtrlData::ITEM_DELETE && hItem == NULL)
	{	
		hItem = IVM_TreeAddItem (strDisplay, pTree, hParent, TVI_SORT, m_treeCtrlData.GetImage(), (DWORD_PTR)this);
		m_treeCtrlData.SetState (CaTreeCtrlData::ITEM_EXIST);
		m_treeCtrlData.SetTreeItem (hItem);
		nTreeChanged |= COMPONENT_ADDED;
		//
		// Display Instances of item if any:
		if (m_pListInstance && !m_pListInstance->IsEmpty())
		{
			CaTreeComponentItemData* pItemInstance = NULL;
			POSITION pos = m_pListInstance->GetHeadPosition();
			while (pos != NULL)
			{
				pItemInstance = m_pListInstance->GetNext (pos);
				pItemInstance->SetDisplayInstance (m_bDisplayInstance);
				pItemInstance->Display (nTreeChanged, pTree, hItem, 1);
			}
		}
	}
	
	//
	// Set the component state of this component:
	UINT nState = 0;
	if (m_nStartupCount == 0)
	{
		if (!m_pListInstance || (m_pListInstance && m_pListInstance->GetCount() == 0))
			nState = COMP_NORMAL;
		else
		if (m_pListInstance->GetCount() > m_nStartupCount)
			nState = COMP_STARTMORE;
	}
	else
	{
		if (!m_pListInstance)
			nState = COMP_STOP;
		else
		if (m_pListInstance->GetCount() == 0)
			nState = COMP_STOP;
		else
		if (m_pListInstance->GetCount() == m_nStartupCount)
			nState = COMP_START;
		else
		if (m_pListInstance->GetCount() > m_nStartupCount)
			nState = COMP_STARTMORE;
		else
		if (m_pListInstance->GetCount() < m_nStartupCount)
			nState = COMP_HALFSTART;
	}
	
	m_treeCtrlData.SetComponentState (nState);
	UpdateIcon(pTree);
}

BOOL CaTreeComponentItemData::AddFolder(CaIvmTree* pTreeData, int& nChange)
{
	CaTreeComponentItemData* pExistItem = pTreeData->FindFolder(m_strComponentType);
	ASSERT (pTreeData);
	ASSERT (m_bFolder == FALSE);
	if (m_bFolder)
		return FALSE;
	if (!pTreeData)
		return FALSE;

	//
	// Add new Component:
	if (!pExistItem)
	{
		pTreeData->m_listFolder.AddTail (this);
		return TRUE;
	}
	else
	{
		ASSERT (pExistItem->IsFolder() == m_bFolder);
		if (pExistItem->IsFolder() != m_bFolder)
			return FALSE;
		//
		// Modify the startup count:
		if (pExistItem->m_nStartupCount != m_nStartupCount)
		{
			nChange |= COMPONENT_STARTUPCOUNT;
			pExistItem->m_nStartupCount = m_nStartupCount;
		}
		pExistItem->m_treeCtrlData.SetState (CaTreeCtrlData::ITEM_EXIST);
		//
		// Modify the list of Instance(s) of a Non-Duplicatable Item (Folder)
		if (IsQueryInstance())
			pExistItem->AddInstance (m_pListInstance);
		return FALSE;
	}
	return TRUE;
}


BOOL CaTreeComponentItemData::AddInstance (CTypedPtrList<CObList, CaTreeComponentInstanceItemData*>* pListInstance)
{
	CaTreeComponentInstanceItemData* pInstance = NULL;
	CaTreeComponentInstanceItemData* pExistInstance = NULL;
	if (!pListInstance)
	{
		if (!m_pListInstance)
			return TRUE;
		POSITION pos = m_pListInstance->GetHeadPosition();
		while (pos != NULL)
		{
			pInstance = m_pListInstance->GetNext (pos);
			pInstance->m_treeCtrlData.SetState (CaTreeCtrlData::ITEM_DELETE);
		}
		return TRUE;
	}

	POSITION p, pos = pListInstance->GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		pInstance = pListInstance->GetNext (pos);
		pExistInstance = FindInstance (pInstance);
		if (pExistInstance)
			pExistInstance->m_treeCtrlData.SetState (CaTreeCtrlData::ITEM_EXIST);
		else
		{
			if (!m_pListInstance)
				m_pListInstance = new CTypedPtrList<CObList, CaTreeComponentInstanceItemData*>;
			pInstance->m_strComponentType = m_strComponentType;
			pInstance->m_strComponentName = m_strComponentName;
			m_pListInstance->AddTail (pInstance);
			pListInstance->RemoveAt(p);
		}
	}
	return TRUE;
}


CaTreeComponentInstanceItemData* CaTreeComponentItemData::FindInstance (CaTreeComponentInstanceItemData* pInstance)
{
	CaTreeComponentInstanceItemData* pExist = NULL;
	if (!m_pListInstance)
		return NULL;
	POSITION pos = m_pListInstance->GetHeadPosition();
	while (pos != NULL)
	{
		pExist = m_pListInstance->GetNext (pos);
		if (pExist->m_strComponentInstance.CompareNoCase (pInstance->m_strComponentInstance) == 0)
			return pExist;
	}

	return NULL;
}


BOOL CaTreeComponentItemData::QueryInstance()
{
	CTypedPtrList<CObList, CaTreeComponentItemData*> listInstance;
	if (IVM_llQueryInstance (this, listInstance) && !listInstance.IsEmpty())
	{
		CaTreeComponentInstanceItemData* pItem = NULL;
		m_pListInstance = new CTypedPtrList<CObList, CaTreeComponentInstanceItemData*>;
		while (!listInstance.IsEmpty())
		{
			pItem = (CaTreeComponentInstanceItemData*)listInstance.RemoveHead();
			pItem->m_treeCtrlData.SetImage(GetInstaneImageRead(), GetInstaneImageNotRead());
			m_pListInstance->AddTail (pItem);
		}
	}
	SetQueryInstance(TRUE);
	return TRUE;
}


void CaTreeComponentItemData::ResetState (CaTreeCtrlData::ItemState state, BOOL bInstance)
{
	//
	// This Item:
	m_treeCtrlData.SetState (state);
	//
	// List of Instances:
	CaTreeComponentInstanceItemData* pItem = NULL;
	if (!m_pListInstance)
		return;
	if (!bInstance)
		return;
	POSITION  pos = m_pListInstance->GetHeadPosition();
	while (pos != NULL)
	{
		pItem = m_pListInstance->GetNext(pos);
		pItem->m_treeCtrlData.SetState (state);
	}
}

void CaTreeComponentItemData::UpdateTreeData()
{
	//
	// Update list of Instances:
	CaTreeComponentInstanceItemData* pItem = NULL;
	if (!m_pListInstance)
		return;
	POSITION p, pos = m_pListInstance->GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		pItem = m_pListInstance->GetNext(pos);
		if (pItem->m_treeCtrlData.GetState () == CaTreeCtrlData::ITEM_DELETE)
		{
			m_pListInstance->RemoveAt (p);
			delete pItem;
			if (m_pListInstance->IsEmpty())
			{
				delete m_pListInstance;
				m_pListInstance = NULL;
				return;
			}
		}
	}
}


BOOL CaTreeComponentItemData::AlertNew (CTreeCtrl* pTree, CaLoggedEvent* e)
{
	CaTreeComponentInstanceItemData* pInstance = NULL;
	if (m_strComponentType.CompareNoCase (e->m_strComponentType) != 0)
		return FALSE;
	//
	// Mark the current folder (component category) as alert:
	m_treeCtrlData.AlertInc();

	//
	// Try to alert on the sub-branch instance:
	if (m_pListInstance && !e->m_strInstance.IsEmpty())
	{
		POSITION pos = m_pListInstance->GetHeadPosition();
		while (pos != NULL)
		{
			pInstance = m_pListInstance->GetNext (pos);
			if (pInstance->m_strComponentInstance.CompareNoCase (e->m_strInstance) == 0)
			{
				pInstance->m_treeCtrlData.AlertInc();
				pInstance->m_treeCtrlData.AlertMarkUpdate(pTree, TRUE);
				return TRUE;
			}
		}
	}
	return TRUE;
}


BOOL CaTreeComponentItemData::AlertRead (CTreeCtrl* pTree, CaLoggedEvent* e)
{
	CaTreeComponentInstanceItemData* pInstance = NULL;
	if (e == NULL)
	{
		//
		// The whole branch:
		m_treeCtrlData.AlertZero();
		if (m_pListInstance)
		{
			POSITION pos = m_pListInstance->GetHeadPosition();
			while (pos != NULL)
			{
				pInstance = m_pListInstance->GetNext (pos);
				pInstance->m_treeCtrlData.AlertZero();
			}
		}
		return TRUE;
	}

	if (m_strComponentType.CompareNoCase (e->m_strComponentType) != 0)
		return FALSE;
	//
	// UnMark the current folder (component category) as not alert:
	m_treeCtrlData.AlertDec();

	//
	// Try to unalert on the sub-branch instance:
	if (m_pListInstance && !e->m_strInstance.IsEmpty())
	{
		POSITION pos = m_pListInstance->GetHeadPosition();
		while (pos != NULL)
		{
			pInstance = m_pListInstance->GetNext (pos);
			if (pInstance->m_strComponentInstance.CompareNoCase (e->m_strInstance) == 0)
			{
				pInstance->m_treeCtrlData.AlertDec();
				return TRUE;
			}
		}
	}
	return TRUE;
}


void CaTreeComponentItemData::ExpandAll(CTreeCtrl* pTree)
{
	HTREEITEM hItem = m_treeCtrlData.GetTreeItem();
	if (hItem && !m_treeCtrlData.IsExpanded())
	{
		pTree->Expand (hItem, TVE_EXPAND);
		m_treeCtrlData.SetExpand (TRUE);
	}
}

void CaTreeComponentItemData::CollapseAll(CTreeCtrl* pTree)
{
	HTREEITEM hItem = m_treeCtrlData.GetTreeItem();
	if (hItem && m_treeCtrlData.IsExpanded())
	{
		pTree->Expand (hItem, TVE_COLLAPSE);
		m_treeCtrlData.SetExpand (FALSE);
	}
}

CaTreeComponentItemData* CaTreeComponentItemData::MatchInstance (LPCTSTR lpszInstance)
{
	CaTreeComponentInstanceItemData* pInstance = NULL;
	if (!m_pListInstance)
		return NULL;
	if (m_pListInstance->IsEmpty())
		return NULL;
	POSITION pos = m_pListInstance->GetHeadPosition();
	while (pos != NULL)
	{
		pInstance = m_pListInstance->GetNext (pos);
		if (pInstance->m_strComponentInstance.CompareNoCase (lpszInstance) == 0)
			return pInstance;
	}
	return NULL;
}

BOOL CaTreeComponentItemData::IsMultipleInstancesRunning()
{
	CaTreeComponentInstanceItemData* pInstance = NULL;
	if (!m_pListInstance)
		return FALSE;
	if (m_pListInstance->IsEmpty())
		return FALSE;

	if (m_pListInstance->GetCount() > m_nStartupCount)
		return TRUE;
	return FALSE;
}


int CaTreeComponentItemData::GetItemStatus ()
{
	UINT nState = m_treeCtrlData.GetComponentState();
	if (nState == 0 || nState == COMP_NORMAL)
	{
		//
		// NORMAL:
		return COMP_NORMAL;
	}
	else
	if (nState == COMP_STOP || nState == (COMP_NORMAL|COMP_STOP))
	{
		//
		// STOPPED:
		return COMP_STOP;
	}
	else
	if (nState == COMP_START || nState == (COMP_NORMAL|COMP_START))
	{
		//
		// STARTED:
		return COMP_START;
	}
	else
	if (nState&COMP_STARTMORE && !(nState&COMP_STOP) && !(nState&COMP_HALFSTART))
	{
		//
		// STARTED MORE:
		return COMP_STARTMORE;
	}
	else
	{
		//
		// HALF STARTED:
		return COMP_HALFSTART;
	}
}


void CaTreeComponentItemData::UpdateState()
{

}

BOOL CaTreeComponentItemData::ExistInstance()
{
	if (!m_pListInstance)
		return FALSE;
	if (m_pListInstance->GetCount() == 0)
		return FALSE;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CaTreeComponentInstanceItemData

CString CaTreeComponentInstanceItemData::GetStartStopString(BOOL bStart)
{
	CString strProcess = _T("???");
	switch (m_nComponentType)
	{
	case COMP_TYPE_NAME:
		if (GVvista())
			strProcess = _T("winstart /stop /param=\"-iigcn\"");
		else
			strProcess = _T("ingstop -iigcn");
		break;
	case COMP_TYPE_DBMS:
		if (GVvista())
		{
		strProcess = _T("winstart /stop /param=\"-iidbms=");
		strProcess+= m_strComponentInstance;
		strProcess+= _T("\"");
		}
		else
		{
		strProcess = _T("ingstop -iidbms=");
		strProcess+= m_strComponentInstance;
		}
		break;
	case COMP_TYPE_INTERNET_COM:
		// ICE Server
		if (GVvista())
			strProcess = _T("winstart /stop /param=\"-icesvr\"");
		else
			strProcess = _T("ingstop -icesvr");
		break;
	case COMP_TYPE_NET:
		if (GVvista())
		{
		strProcess = _T("winstart /stop /param=\"-iigcc=");
		strProcess+= m_strComponentInstance;
		strProcess+= _T("\"");
		}
		else
		{
		strProcess = _T("ingstop -iigcc=");
		strProcess+= m_strComponentInstance;
		}
		break;
	case COMP_TYPE_JDBC:
		if (GVvista())
		{
		strProcess = _T("winstart /stop /param=\"-iijdbc=");
		strProcess+= m_strComponentInstance;
		strProcess+= _T("\"");
		}
		else
		{
		strProcess = _T("ingstop -iijdbc=");
		strProcess+= m_strComponentInstance;
		}
		break;
	case COMP_TYPE_DASVR:
		if (GVvista())
		{
		strProcess = _T("winstart /stop /param=\"-iigcd=");
		strProcess+= m_strComponentInstance;
		strProcess+= _T("\"");
		}
		else
		{
		strProcess = _T("ingstop -iigcd=");
		strProcess+= m_strComponentInstance;
		}
		break;
	case COMP_TYPE_BRIDGE:
		if (GVvista())
		{
			strProcess = _T("winstart /stop /param=\"-iigcb=");
			strProcess+= m_strComponentInstance;
			strProcess+= _T("\"");
		}
		else
		{
		strProcess = _T("ingstop -iigcb=");
		strProcess+= m_strComponentInstance;
		}
		break;
	case COMP_TYPE_STAR:
		if (GVvista())
		{
			strProcess = _T("winstart /stop /param=\"-iistar=");
			strProcess+= m_strComponentInstance;
			strProcess+= _T("\"");
		}
		else
		{
		strProcess = _T("ingstop -iistar=");
		strProcess+= m_strComponentInstance;
		}
		break;

	case COMP_TYPE_SECURITY:
	case COMP_TYPE_LOCKING_SYSTEM:
	case COMP_TYPE_LOGGING_SYSTEM:
	case COMP_TYPE_TRANSACTION_LOG:
		// Not specified to be able to stop.
		ASSERT (FALSE);
		break;

	case COMP_TYPE_RECOVERY:
		if(GVvista())
			strProcess = _T("winstart /stop /param=\"-dmfrcp\"");
		else
			strProcess = _T("ingstop -dmfrcp");
		break;
	case COMP_TYPE_ARCHIVER:
		if (GVvista())
			strProcess = _T("winstart /stop /param=\"-dmfacp\"");
		else
			strProcess = _T("ingstop -dmfacp");
		break;

	case COMP_TYPE_OIDSK_DBMS:
		// Desktop DBMS
		ASSERT (FALSE);
		break;

	case COMP_TYPE_RMCMD:
		if (GVvista())
			strProcess = _T("winstart /stop /param=\"-rmcmd\"");
		else
			strProcess = _T("ingstop -rmcmd");
		break;

	case COMP_TYPE_GW_ORACLE:
		if (GVvista())
			strProcess = _T("winstart /stop /param=\"-oracle\"");
		else
		strProcess = _T("ingstop -oracle");
		break;
	case COMP_TYPE_GW_DB2UDB:
		if (GVvista())
			strProcess = _T("winstart /stop /param=\"-db2udb\"");
		else
			strProcess = _T("ingstop -db2udb");
		break;
	case COMP_TYPE_GW_INFORMIX:
		if (GVvista())
			strProcess = _T("winstart /stop /param=\"-informix\"");
		else
		strProcess = _T("ingstop -informix");
		break;
	case COMP_TYPE_GW_SYBASE:
		if (GVvista())
			strProcess = _T("winstart /stop /param=\"-sybase\"");
		else
		strProcess = _T("ingstop -sybase");
		break;
	case COMP_TYPE_GW_MSSQL:
		if (GVvista())
			strProcess = _T("winstart /stop /param=\"-mssql\"");
		else
		strProcess = _T("ingstop -mssql");
		break;
	case COMP_TYPE_GW_ODBC:
		if (GVvista())
			strProcess = _T("winstart /stop /param=\"-odbc\"");
		else
		strProcess = _T("ingstop -odbc");
		break;
	default:
		//
		// Type of component is unknown !
		ASSERT (FALSE);
	}

	return strProcess;
}

CaPageInformation* CaTreeComponentInstanceItemData::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [3] = 
	{
		IDS_TAB_STATUS,
		IDS_TAB_LOGEVENT,
		IDS_TAB_STATISTIC
	};
	UINT nDlgID [3] = 
	{
		IDD_STATUS_INSTANCE,
		IDD_LOGEVENT_GENERIC,
		IDD_MESSAGE_STATISTIC
	};

	m_pPageInfo = new CaPageInformation (_T("INSTANCE"), 3, nTabID, nDlgID);
	CaEventFilter& filter = m_pPageInfo->GetEventFilter();
	//
	// Category and Name are not set for the 'Instance' item.
	filter.SetFilterFlag (FILER_DATE_BEGIN|FILER_DATE_END);
	return m_pPageInfo;
}

void CaTreeComponentInstanceItemData::Display (int& nTreeChanged, CTreeCtrl* pTree, HTREEITEM hParent, int nMode)
{
	HTREEITEM hItem;
	CString strDisplay = m_strComponentInstance;
	hItem = m_treeCtrlData.GetTreeItem();
	CaTreeCtrlData::ItemState state = m_treeCtrlData.GetState();
	//
	// Delete Old Item:
	if (m_treeCtrlData.GetState() == CaTreeCtrlData::ITEM_DELETE && hItem != NULL)
	{
		pTree->DeleteItem (hItem);
		nTreeChanged |= COMPONENT_REMOVED;
		return;
	}

	//
	// Modify exiting item:
	if (state == CaTreeCtrlData::ITEM_EXIST && hItem != NULL)
	{
		m_treeCtrlData.AlertMarkUpdate(pTree, TRUE);
		return;
	}

	//
	// Display New Item:
	ASSERT (hItem == NULL);
	if (m_bDisplayInstance)
	{
		hItem = IVM_TreeAddItem (strDisplay, pTree, hParent, TVI_SORT, m_treeCtrlData.GetImage(), (DWORD_PTR)this);
		m_treeCtrlData.SetTreeItem (hItem);
	}
	m_treeCtrlData.SetState (CaTreeCtrlData::ITEM_EXIST);
	nTreeChanged |= COMPONENT_ADDED;
}


BOOL CaTreeComponentFolderItemData::AddFolder(CaIvmTree* pTreeData, int& nChange)
{
	CaTreeComponentItemData* pExistItem = pTreeData->FindFolder(m_strComponentType);
	//
	// Add New Folder of a Duplicatable Item + new Component
	if (!pExistItem)
	{
		CaTreeComponentDuplicatableFolder* pFolder = new CaTreeComponentDuplicatableFolder(m_strComponentType);
		pFolder->m_strComponentTypes = m_strComponentTypes;
		pFolder->m_nComponentType = m_nComponentType;

		//
		// Component of the Folder:
		pFolder->AddComponent (this, nChange);
		pFolder->SetOrder(m_nOrder);
		//
		// Add The folder that holds the component:
		pTreeData->m_listFolder.AddTail (pFolder);
		return TRUE;
	}

	ASSERT(pExistItem->IsFolder() == TRUE);
	if (!pExistItem->IsFolder())
		return FALSE;
	BOOL bAdd = pExistItem->AddComponent (this, nChange);
	return bAdd;
}


void CaTreeComponentFolderItemData::Display (int& nTreeChanged, CTreeCtrl* pTree, HTREEITEM hParent, int nMode)
{
	HTREEITEM hItem;
	CaTreeComponentInstanceItemData* pInstance = NULL;
	CString strDisplay = m_strComponentName;
	hItem = m_treeCtrlData.GetTreeItem();
	CaTreeCtrlData::ItemState state = m_treeCtrlData.GetState();
	//
	// Delete Old Item:
	if (m_treeCtrlData.GetState() == CaTreeCtrlData::ITEM_DELETE && hItem != NULL)
	{
		pTree->DeleteItem (hItem);
		nTreeChanged |= COMPONENT_REMOVED;
		return;
	}

	//
	// Modify exiting item / Display its instances:
	if (state == CaTreeCtrlData::ITEM_EXIST && hItem != NULL)
	{
		if (m_pListInstance)
		{
			POSITION pos = m_pListInstance->GetHeadPosition();
			while (pos != NULL)
			{
				pInstance = m_pListInstance->GetNext (pos);
				pInstance->Display (nTreeChanged, pTree, hItem, 1);
			}
		}
	}

	//
	// Display New Item (Component Name):
	if (state != CaTreeCtrlData::ITEM_DELETE && hItem == NULL)
	{
		hItem = IVM_TreeAddItem (strDisplay, pTree, hParent, TVI_SORT, m_treeCtrlData.GetImage(), (DWORD_PTR)this);
		m_treeCtrlData.SetState (CaTreeCtrlData::ITEM_EXIST);
		m_treeCtrlData.SetTreeItem (hItem);
		nTreeChanged |= COMPONENT_ADDED;
		//
		// Display Instances of item if any:
		if (m_pListInstance && !m_pListInstance->IsEmpty())
		{
			POSITION pos = m_pListInstance->GetHeadPosition();
			while (pos != NULL)
			{
				pInstance = m_pListInstance->GetNext (pos);
				pInstance->Display (nTreeChanged, pTree, hItem, 1);
			}
		}
	}
	//
	// Set the component state of this component:
	UINT nState = 0;
	if (m_nStartupCount == 0)
	{
		if (!m_pListInstance || (m_pListInstance && m_pListInstance->GetCount() == 0))
			nState = COMP_NORMAL;
		else
		if (m_pListInstance->GetCount() > m_nStartupCount)
			nState = COMP_STARTMORE;
	}
	else
	{
		if (!m_pListInstance)
			nState = COMP_STOP;
		else
		if (m_pListInstance->GetCount() == 0)
			nState = COMP_STOP;
		else
		if (m_pListInstance->GetCount() == m_nStartupCount)
			nState = COMP_START;
		else
		if (m_pListInstance->GetCount() > m_nStartupCount)
			nState = COMP_STARTMORE;
		else
		if (m_pListInstance->GetCount() < m_nStartupCount)
			nState = COMP_HALFSTART;
	}
	
	m_treeCtrlData.SetComponentState (nState);
	UpdateIcon(pTree);
}


BOOL CaTreeComponentFolderItemData::AlertNew (CTreeCtrl* pTree, CaLoggedEvent* e)
{
	CaTreeComponentInstanceItemData* pInstance = NULL;
	if (m_strComponentName.CompareNoCase (e->m_strComponentName) != 0)
		return FALSE;
	//
	// Mark the current folder (component name) as alert:
	m_treeCtrlData.AlertInc();

	//
	// Try to alert on the sub-branch instance:
	if (m_pListInstance && !e->m_strInstance.IsEmpty())
	{
		POSITION pos = m_pListInstance->GetHeadPosition();
		while (pos != NULL)
		{
			pInstance = m_pListInstance->GetNext (pos);
			if (pInstance->m_strComponentInstance.CompareNoCase (e->m_strInstance) == 0)
			{
				pInstance->m_treeCtrlData.AlertInc();
				pInstance->m_treeCtrlData.AlertMarkUpdate(pTree, TRUE);
				return TRUE;
			}
		}
	}
	return TRUE;
}


BOOL CaTreeComponentFolderItemData::AlertRead (CTreeCtrl* pTree, CaLoggedEvent* e)
{
	CaTreeComponentInstanceItemData* pInstance = NULL;
	if (e == NULL)
	{
		//
		// The whole branch:
		m_treeCtrlData.AlertZero();
		if (m_pListInstance)
		{
			POSITION pos = m_pListInstance->GetHeadPosition();
			while (pos != NULL)
			{
				pInstance = m_pListInstance->GetNext (pos);
				pInstance->m_treeCtrlData.AlertZero();
			}
		}
		return TRUE;
	}

	if (m_strComponentName.CompareNoCase (e->m_strComponentName) != 0)
		return FALSE;
	//
	// UnMark the current folder (component name) as not alert:
	m_treeCtrlData.AlertDec();

	//
	// Try to alert on the sub-branch instance:
	if (m_pListInstance && !e->m_strInstance.IsEmpty())
	{
		POSITION pos = m_pListInstance->GetHeadPosition();
		while (pos != NULL)
		{
			pInstance = m_pListInstance->GetNext (pos);
			if (pInstance->m_strComponentInstance.CompareNoCase (e->m_strInstance) == 0)
			{
				pInstance->m_treeCtrlData.AlertDec();
				return TRUE;
			}
		}
	}
	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CaTreeComponentGatewayItemData


BOOL CaTreeComponentGatewayItemData::AddFolder(CaIvmTree* pTreeData, int& nChange)
{
	CString cstemp;
	cstemp.LoadString(IDS_GW_PARENTBRANCH);
	LPCTSTR pGWBranchName = (LPCTSTR) cstemp;
	CaTreeComponentItemData* pExistItem = pTreeData->FindFolder(oneGW());
	//
	// Add New Folder of a Gateway Item + new Component
	if (!pExistItem)
	{
		CaTreeComponentDuplicatableFolder* pFolder = new CaTreeComponentDuplicatableFolder(oneGW());
		pFolder->m_strComponentTypes = pGWBranchName;
		pFolder->m_nComponentType = COMP_TYPE_GW_FOLDER;
		//
		// Component of the Folder:
		pFolder->AddComponent (this, nChange);
		pFolder->SetOrder(m_nOrder);
		//
		// Add The folder that holds the component:
		pTreeData->m_listFolder.AddTail (pFolder);
		return TRUE;
	}

	ASSERT(pExistItem->IsFolder() == TRUE);
	if (!pExistItem->IsFolder())
		return FALSE;
	BOOL bAdd = pExistItem->AddComponent (this, nChange);
	return bAdd;
}


void CaTreeComponentGatewayItemData::Display (int& nTreeChanged, CTreeCtrl* pTree, HTREEITEM hParent, int nMode)
{
	CaTreeComponentItemData::Display(nTreeChanged, pTree, hParent, 1);
}


void CaTreeComponentGatewayItemData::UpdateIcon(CTreeCtrl* pTree)
{
	int nImageIcon = -1;
	BOOL bExpanded = m_treeCtrlData.IsExpanded();
	BOOL bAlert    = (m_treeCtrlData.AlertGetCount()>0)? TRUE: FALSE;
	UINT nState = m_treeCtrlData.GetComponentState();
	ASSERT (pTree);
	if (!pTree)
		return;

	if (nState == 0 || nState == COMP_NORMAL)
	{
		//
		// NORMAL:
		nImageIcon = bAlert? IM_GATEWAY_N_NORMAL: IM_GATEWAY_R_NORMAL;
	}
	else
	if (nState == COMP_STOP || nState == (COMP_NORMAL|COMP_STOP))
	{
		//
		// STOPPED:
		nImageIcon = bAlert? IM_GATEWAY_N_STOPPED: IM_GATEWAY_R_STOPPED;
	}
	else
	if (nState == COMP_START || nState == (COMP_NORMAL|COMP_START))
	{
		//
		// STARTED:
		nImageIcon = bAlert? IM_GATEWAY_N_STARTED: IM_GATEWAY_N_STARTED;
	}
	else
	if (nState&COMP_STARTMORE && !(nState&COMP_STOP) && !(nState&COMP_HALFSTART))
	{
		//
		// STARTED MORE:
		nImageIcon = bAlert? IM_GATEWAY_N_STARTEDMORE: IM_GATEWAY_R_STARTEDMORE;
	}
	else
	{
		//
		// HALF STARTED:
		nImageIcon = bAlert? IM_GATEWAY_N_HALFSTARTED: IM_GATEWAY_N_HALFSTARTED;
	}
	pTree->SetItemImage (m_treeCtrlData.GetTreeItem(), nImageIcon, nImageIcon);
}

//
// CaTreeComponentDuplicatableFolder:
// *********************************
CaTreeComponentDuplicatableFolder::CaTreeComponentDuplicatableFolder():CaTreeComponentItemData()
{
	m_bFolder = TRUE;
	m_treeCtrlData.SetImage(IM_FOLDER_CLOSED_R_NORMAL, IM_FOLDER_CLOSED_R_NORMAL);
}

CaTreeComponentDuplicatableFolder::CaTreeComponentDuplicatableFolder(LPCTSTR lpszFolderName):CaTreeComponentItemData()
{
	m_bFolder = TRUE;
	m_strComponentType = lpszFolderName;
	m_treeCtrlData.SetImage(IM_FOLDER_CLOSED_R_NORMAL, IM_FOLDER_CLOSED_R_NORMAL);
}

CaTreeComponentDuplicatableFolder::~CaTreeComponentDuplicatableFolder()
{
	while (!m_listComponent.IsEmpty())
		delete m_listComponent.RemoveTail();
}

CaPageInformation* CaTreeComponentDuplicatableFolder::GetPageInformation()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [3] = 
	{
		IDS_TAB_STATUS,
		IDS_TAB_LOGEVENT,
		IDS_TAB_STATISTIC
	};
	UINT nDlgID [3] = 
	{
		IDD_STATUS_DUPLICATABLE_COMPONENT,
		IDD_LOGEVENT_GENERIC,
		IDD_MESSAGE_STATISTIC
	};

	m_pPageInfo = new CaPageInformation (_T("FOLDER"), 3, nTabID, nDlgID);
	CaEventFilter& filter = m_pPageInfo->GetEventFilter();
	//
	// Category is not set for the 'Folder' item.
	filter.SetFilterFlag (FILER_DATE_BEGIN|FILER_DATE_END|FILER_NAME|FILER_INSTANCE);
	return m_pPageInfo;
}



BOOL CaTreeComponentDuplicatableFolder::AddComponent(CaTreeComponentItemData* pComponent, int& nChange)
{
	CaTreeComponentItemData* pExistComponent = NULL;
	POSITION pos = m_listComponent.GetHeadPosition();
	while (pos != NULL)
	{
		pExistComponent = m_listComponent.GetNext (pos);
		if (pExistComponent->m_strComponentType.CompareNoCase (pComponent->m_strComponentType) == 0 &&
		    pExistComponent->m_strComponentName.CompareNoCase (pComponent->m_strComponentName) == 0)
		{
			//
			// Child in this folder:
			pExistComponent->m_treeCtrlData.SetState (CaTreeCtrlData::ITEM_EXIST);
			//
			// Modify the startup count:
			if (pExistComponent->m_nStartupCount != pComponent->m_nStartupCount)
			{
				nChange |= COMPONENT_STARTUPCOUNT;
				pExistComponent->m_nStartupCount = pComponent->m_nStartupCount;
			}
			//
			// Modify the list of Instances of the existing component:
			if (pComponent->IsQueryInstance())
				pExistComponent->AddInstance (pComponent->GetInstances());
			//
			// The folder itself:
			m_treeCtrlData.SetState(CaTreeCtrlData::ITEM_EXIST);
			return FALSE;
		}
	}
	m_listComponent.AddTail (pComponent);
	return TRUE;
}

void CaTreeComponentDuplicatableFolder::Display (int& nTreeChanged, CTreeCtrl* pTree, HTREEITEM hParent, int nMode)
{
	HTREEITEM hItem;
	CaTreeComponentItemData* pComponent = NULL;
	hItem = m_treeCtrlData.GetTreeItem();
	CaTreeCtrlData::ItemState state = m_treeCtrlData.GetState();
	UINT nState = 0;
	CString strDisplay = m_strComponentTypes;

	//
	// Delete Old Item:
	if (m_treeCtrlData.GetState() == CaTreeCtrlData::ITEM_DELETE && hItem != NULL)
	{
		pTree->DeleteItem (hItem);
		nTreeChanged |= COMPONENT_REMOVED;
		return;
	}

	//
	// Modify exiting item / Display its components:
	if (state == CaTreeCtrlData::ITEM_EXIST && hItem != NULL)
	{
		POSITION pos = m_listComponent.GetHeadPosition();
		while (pos != NULL)
		{
			pComponent = m_listComponent.GetNext (pos);
			pComponent->Display (nTreeChanged, pTree, hItem, 1);
			CaTreeCtrlData& nodeInfo = pComponent->GetNodeInformation();
			if (pComponent->GetNodeInformation().GetState() != CaTreeCtrlData::ITEM_DELETE)
				nState |= nodeInfo.GetComponentState();
		}
	}

	//
	// Display New Item (Folder Name + Component Name):
	if (state != CaTreeCtrlData::ITEM_DELETE && hItem == NULL)
	{
		hItem = IVM_TreeAddItem (strDisplay, pTree, hParent, TVI_LAST, m_treeCtrlData.GetImage(), (DWORD_PTR)this);
		m_treeCtrlData.SetState (CaTreeCtrlData::ITEM_EXIST);
		m_treeCtrlData.SetTreeItem (hItem);
		nTreeChanged |= COMPONENT_ADDED;
		//
		// Display Components of item if any:
		POSITION pos = m_listComponent.GetHeadPosition();
		while (pos != NULL)
		{
			pComponent = m_listComponent.GetNext (pos);
			pComponent->Display (nTreeChanged, pTree, hItem, 1);
			CaTreeCtrlData& nodeInfo = pComponent->GetNodeInformation();
			if (pComponent->GetNodeInformation().GetState() != CaTreeCtrlData::ITEM_DELETE)
				nState |= nodeInfo.GetComponentState();
		}
	}
	//
	// Set the component state of this component:
	m_treeCtrlData.SetComponentState (nState);
	UpdateIcon(pTree);
}



void CaTreeComponentDuplicatableFolder::ResetState (CaTreeCtrlData::ItemState state, BOOL bInstance)
{
	//
	// This Item:
	m_treeCtrlData.SetState (state);
	//
	// List of Components:
	CaTreeComponentItemData* pComponent = NULL;
	POSITION  pos = m_listComponent.GetHeadPosition();
	while (pos != NULL)
	{
		pComponent = m_listComponent.GetNext(pos);
		pComponent->ResetState (state, bInstance);
	}
}

void CaTreeComponentDuplicatableFolder::UpdateTreeData()
{
	//
	// Update list of Components:
	CaTreeComponentItemData* pComponent = NULL;
	POSITION p, pos = m_listComponent.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		pComponent = m_listComponent.GetNext(pos);
		if (pComponent->m_treeCtrlData.GetState () == CaTreeCtrlData::ITEM_DELETE)
		{
			m_listComponent.RemoveAt (p);
			delete pComponent;
		}
		else
			pComponent->UpdateTreeData();
	}
}


BOOL CaTreeComponentDuplicatableFolder::AlertNew (CTreeCtrl* pTree, CaLoggedEvent* e)
{
	CaTreeComponentItemData* pComponent = NULL;
	if (m_strComponentType.CompareNoCase (e->m_strComponentType) != 0)
		return FALSE;
	//
	// Whatever important event arrived, we mark an alert on the concerned
	// branch and ALL ITS PARENT BRANCH:
	m_treeCtrlData.AlertInc();

	POSITION pos = m_listComponent.GetHeadPosition();
	while (pos != NULL)
	{
		pComponent = m_listComponent.GetNext(pos);
		if (pComponent->m_strComponentName.CompareNoCase (e->m_strComponentName) == 0)
		{
			pComponent->AlertNew (pTree, e);
			return TRUE;
		}
	}
	return TRUE;
}


BOOL CaTreeComponentDuplicatableFolder::AlertRead (CTreeCtrl* pTree, CaLoggedEvent* e)
{
	CaTreeComponentItemData* pComponent = NULL;
	if (e == NULL)
	{
		//
		// The whole branch:
		m_treeCtrlData.AlertZero();
		POSITION pos = m_listComponent.GetHeadPosition();
		while (pos != NULL)
		{
			pComponent = m_listComponent.GetNext(pos);
			pComponent->AlertRead (pTree, NULL);
		}
		return TRUE;
	}

	if (m_strComponentType.CompareNoCase (e->m_strComponentType) != 0)
		return FALSE;
	//
	// Whatever important event is read, we unmark an alert on the concerned
	// branch and ALL ITS PARENT BRANCH:
	m_treeCtrlData.AlertDec();

	POSITION pos = m_listComponent.GetHeadPosition();
	while (pos != NULL)
	{
		pComponent = m_listComponent.GetNext(pos);
		if (pComponent->m_strComponentName.CompareNoCase (e->m_strComponentName) == 0)
		{
			pComponent->AlertRead (pTree, e);
			return TRUE;
		}
	}
	return TRUE;
}


void CaTreeComponentDuplicatableFolder::ExpandAll(CTreeCtrl* pTree)
{
	HTREEITEM hItem = m_treeCtrlData.GetTreeItem();
	if (hItem && !m_treeCtrlData.IsExpanded())
	{
		pTree->Expand (hItem, TVE_EXPAND);
		m_treeCtrlData.SetExpand (TRUE);
	}

	CaTreeComponentItemData* pComponent = NULL;
	POSITION pos = m_listComponent.GetHeadPosition();
	while (pos != NULL)
	{
		pComponent = m_listComponent.GetNext(pos);
		pComponent->ExpandAll(pTree);
	}
}

void CaTreeComponentDuplicatableFolder::CollapseAll(CTreeCtrl* pTree)
{
	HTREEITEM hItem = m_treeCtrlData.GetTreeItem();
	if (hItem && m_treeCtrlData.IsExpanded())
	{
		pTree->Expand (hItem, TVE_COLLAPSE);
		m_treeCtrlData.SetExpand (FALSE);
	}

	CaTreeComponentItemData* pComponent = NULL;
	POSITION pos = m_listComponent.GetHeadPosition();
	while (pos != NULL)
	{
		pComponent = m_listComponent.GetNext(pos);
		pComponent->CollapseAll(pTree);
	}
}



void CaTreeComponentDuplicatableFolder::SetExpandImages (CTreeCtrl* pTree, BOOL bExpand)
{
	UpdateIcon(pTree);
}


CaTreeComponentItemData* CaTreeComponentDuplicatableFolder::MatchInstance (LPCTSTR lpszInstance)
{
	CaTreeComponentItemData* pComponent = NULL;
	CaTreeComponentItemData* pCompFound = NULL;

	POSITION pos = m_listComponent.GetHeadPosition();
	while (pos != NULL)
	{
		pComponent = m_listComponent.GetNext(pos);
		pCompFound = pComponent->MatchInstance(lpszInstance);
		if (pCompFound)
			return pCompFound;
	}
	return NULL;
}

BOOL CaTreeComponentDuplicatableFolder::IsMultipleInstancesRunning()
{
	CaTreeComponentItemData* pComponent = NULL;

	POSITION pos = m_listComponent.GetHeadPosition();
	while (pos != NULL)
	{
		pComponent = m_listComponent.GetNext(pos);
		if (pComponent->IsMultipleInstancesRunning())
			return TRUE;
	}
	return FALSE;
}


void CaTreeComponentDuplicatableFolder::UpdateIcon(CTreeCtrl* pTree)
{
	int nImageIcon = -1;
	BOOL bExpanded = m_treeCtrlData.IsExpanded();
	BOOL bAlert    = (m_treeCtrlData.AlertGetCount()>0)? TRUE: FALSE;
	UINT nState = m_treeCtrlData.GetComponentState();
	ASSERT (pTree);
	if (!pTree)
		return;

	if (nState == 0 || nState == COMP_NORMAL)
	{
		//
		// NORMAL:
		if (bExpanded)
			nImageIcon = bAlert? IM_FOLDER_OPENED_N_NORMAL: IM_FOLDER_OPENED_R_NORMAL;
		else
			nImageIcon = bAlert? IM_FOLDER_CLOSED_N_NORMAL: IM_FOLDER_CLOSED_R_NORMAL;
	}
	else
	if (nState == COMP_STOP || nState == (COMP_NORMAL|COMP_STOP))
	{
		//
		// STOPPED:
		if (bExpanded)
			nImageIcon = bAlert? IM_FOLDER_OPENED_N_STOPPED: IM_FOLDER_OPENED_R_STOPPED;
		else
			nImageIcon = bAlert? IM_FOLDER_CLOSED_N_STOPPED: IM_FOLDER_CLOSED_R_STOPPED;
	}
	else
	if (nState == COMP_START || nState == (COMP_NORMAL|COMP_START))
	{
		//
		// STARTED:
		if (bExpanded)
			nImageIcon = bAlert? IM_FOLDER_OPENED_N_STARTED: IM_FOLDER_OPENED_R_STARTED;
		else
			nImageIcon = bAlert? IM_FOLDER_CLOSED_N_STARTED: IM_FOLDER_CLOSED_R_STARTED;
	}
	else
	if (nState&COMP_STARTMORE && !(nState&COMP_STOP) && !(nState&COMP_HALFSTART))
	{
		//
		// STARTED MORE:
		if (bExpanded)
			nImageIcon = bAlert? IM_FOLDER_OPENED_N_STARTEDMORE: IM_FOLDER_OPENED_R_STARTEDMORE;
		else
			nImageIcon = bAlert? IM_FOLDER_CLOSED_N_STARTEDMORE: IM_FOLDER_CLOSED_R_STARTEDMORE;
	}
	else
	{
		//
		// HALF STARTED:
		if (bExpanded)
			nImageIcon = bAlert? IM_FOLDER_OPENED_N_HALFSTARTED: IM_FOLDER_OPENED_R_HALFSTARTED;
		else
			nImageIcon = bAlert? IM_FOLDER_CLOSED_N_HALFSTARTED: IM_FOLDER_CLOSED_R_HALFSTARTED;
	}
	pTree->SetItemImage (m_treeCtrlData.GetTreeItem(), nImageIcon, nImageIcon);
}

//
// CaTreeComponentInstallationItemData:
// ************************************

CaTreeComponentInstallationItemData::CaTreeComponentInstallationItemData():CaTreeComponentItemData(), m_strFolderName(_T("Ingres Installation"))
{
#if defined (MAINWIN)
	if (GVvista())
	{
	m_strStart = _T("winstart /start");
	m_strStop  = _T("winstart /stop");
	}
	else
	{
	m_strStart = _T("ingstart");
	m_strStop  = _T("ingstop");
	}
#else
	if (GVvista())
	{
	m_strStart = _T("winstart.exe /start");
	m_strStop  = _T("winstart.exe /stop");
	}
	else
	{
	m_strStart = _T("ingstart.exe");
	m_strStop  = _T("ingstop.exe");
	}
#endif
	m_treeCtrlData.SetImage (IM_INSTALLATION_R_NORMAL, IM_INSTALLATION_R_NORMAL);
}

CaTreeComponentInstallationItemData::~CaTreeComponentInstallationItemData()
{
}

CaPageInformation* CaTreeComponentInstallationItemData::GetPageInformation()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [4] = 
	{
		IDS_TAB_STATUS,
		IDS_TAB_PARAMETER,
		IDS_TAB_LOGEVENT,
		IDS_TAB_STATISTIC
	};
	UINT nDlgID [4] = 
	{
		IDD_STATUS_INSTALLATION,
		IDD_PARAMETER_INSTALLATION,
		IDD_LOGEVENT_GENERIC,
		IDD_MESSAGE_STATISTIC
	};
		
	m_pPageInfo = new CaPageInformation (_T("INSTALLATION"), 4, nTabID, nDlgID);
	return m_pPageInfo;
}

BOOL CaTreeComponentInstallationItemData::Start()
{
	BOOL bOK = IVM_StartComponent(m_strStart);
	return bOK;
}

BOOL CaTreeComponentInstallationItemData::Stop()
{
	BOOL bOK = IVM_StopComponent(m_strStop);
	return bOK;
}

BOOL CaTreeComponentInstallationItemData::IsStartEnabled()
{
	return TRUE;
}

BOOL CaTreeComponentInstallationItemData::IsStopEnabled ()
{

	return TRUE;
}


void CaTreeComponentInstallationItemData::UpdateIcon(CTreeCtrl* pTree)
{
	int nImageIcon = -1;
	BOOL bExpanded = m_treeCtrlData.IsExpanded();
	BOOL bAlert    = (m_treeCtrlData.AlertGetCount()>0)? TRUE: FALSE;
	UINT nState = m_treeCtrlData.GetComponentState();
	ASSERT (pTree);
	if (!pTree)
		return;

	if (nState == 0 || nState == COMP_NORMAL)
	{
		//
		// NORMAL:
		nImageIcon = bAlert? IM_INSTALLATION_N_NORMAL: IM_INSTALLATION_R_NORMAL;
	}
	else
	if (nState == COMP_STOP || nState == (COMP_NORMAL|COMP_STOP))
	{
		//
		// STOPPED:
		nImageIcon = bAlert? IM_INSTALLATION_N_STOPPED: IM_INSTALLATION_R_STOPPED;
	}
	else
	if (nState == COMP_START || nState == (COMP_NORMAL|COMP_START))
	{
		//
		// STARTED:
		nImageIcon = bAlert? IM_INSTALLATION_N_STARTED: IM_INSTALLATION_R_STARTED;
	}
	else
	if (nState&COMP_STARTMORE && !(nState&COMP_STOP) && !(nState&COMP_HALFSTART))
	{
		//
		// STARTED MORE:
		nImageIcon = bAlert? IM_INSTALLATION_N_STARTEDMORE: IM_INSTALLATION_R_STARTEDMORE;
	}
	else
	{
		//
		// HALF STARTED:
		nImageIcon = bAlert? IM_INSTALLATION_N_HALFSTARTED: IM_INSTALLATION_R_HALFSTARTED;
	}
	pTree->SetItemImage (m_treeCtrlData.GetTreeItem(), nImageIcon, nImageIcon);
}



//
// CaIvmTree:
// **********
void CaIvmTree::CleanUp()
{
	while (!m_listFolder.IsEmpty())
		delete m_listFolder.RemoveHead();
}

CaIvmTree::~CaIvmTree()
{
	CleanUp();
}

CaTreeComponentItemData* CaIvmTree::FindFolder(LPCTSTR lpszFolderName)
{
	CaTreeComponentItemData* pFolder = NULL;
	POSITION pos = m_listFolder.GetHeadPosition();

	while (pos != NULL)
	{
		pFolder = m_listFolder.GetNext (pos);
		if (pFolder->m_strComponentType.CompareNoCase (lpszFolderName) == 0)
			return pFolder;
	}
	return NULL;
}



BOOL CaIvmTree::Initialize(CTypedPtrList<CObList, CaTreeComponentItemData*>* pListComponent, BOOL bInstance, int& nChange)
{
	CaTreeComponentItemData* pItem = NULL;
	POSITION p, pos = pListComponent? pListComponent->GetHeadPosition(): NULL;
	while (pos != NULL)
	{
		p = pos;
		pItem = pListComponent->GetNext(pos);

		if (!pItem->AddFolder (this, nChange))
		{
			pListComponent->RemoveAt (p);
			delete pItem;
			pItem = NULL;
		}
		else
		{
			pListComponent->RemoveAt (p);
		}
	}

	return TRUE;
}


void CaIvmTree::Display(CTreeCtrl* pTree, int& nChange)
{
	BOOL b2Expand = FALSE;
	//
	// nState is a Union of states of all the children branches
	UINT nState = 0;
	if (!m_hRootInstallation)
	{
		if (!m_bLoadString)
		{
			m_bLoadString = TRUE;
			if (!m_installationBranch.m_strFolderName.LoadString(IDS_INSTALLATION))
				m_installationBranch.m_strFolderName = _T("Ingres Installation");
		}
		//
		// Create the main root item:
		m_hRootInstallation  = pTree->InsertItem (
			m_installationBranch.m_strFolderName, 
			IM_INSTALLATION_R_NORMAL, 
			IM_INSTALLATION_R_NORMAL, 
			TVI_ROOT, 
			TVI_FIRST);
		m_installationBranch.m_treeCtrlData.SetTreeItem(m_hRootInstallation);
		pTree->SetItemData (m_hRootInstallation, (DWORD_PTR)&m_installationBranch);
		b2Expand = TRUE;
	}

	//
	// Display the children (sub items) of the main item:
	CaTreeComponentItemData* pItem = NULL;
	POSITION pos = m_listFolder.GetHeadPosition();
	while (pos != NULL)
	{
		pItem = m_listFolder.GetNext(pos);
		pItem->Display (nChange, pTree, m_hRootInstallation);
		CaTreeCtrlData& nodeInfo = pItem->GetNodeInformation();
		if (pItem->GetNodeInformation().GetState() != CaTreeCtrlData::ITEM_DELETE)
			nState |= nodeInfo.GetComponentState();
	}
	//
	// Update the icon of the main item:
	CaTreeCtrlData& nodeInfo = m_installationBranch.GetNodeInformation();
	nodeInfo.SetComponentState(nState);
	m_installationBranch.UpdateIcon(pTree);

	//
	// Sort the children of the main item:
	SortFolders (pTree, m_hRootInstallation);
	//
	// Expand the main item:
	if (b2Expand)
		pTree->Expand(m_hRootInstallation, TVE_EXPAND);
}

void CaIvmTree::Update(CTypedPtrList<CObList, CaTreeComponentItemData*>* pLc, CTreeCtrl* pTree, BOOL bInstance, int& nChange)
{
	ResetState (CaTreeCtrlData::ITEM_DELETE, bInstance);
	Initialize (pLc, bInstance, nChange);
	if (pTree && IsWindow (pTree->m_hWnd))
		Display(pTree, nChange);
	UpdateTreeData();
}

static int CALLBACK fnCompareComponent(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	CaTreeComponentItemData* pComponent1 = (CaTreeComponentItemData*)lParam1;
	CaTreeComponentItemData* pComponent2 = (CaTreeComponentItemData*)lParam2;
	if (pComponent1 == NULL || pComponent2 == NULL)
		return 0;
	return (pComponent1->GetOrder() > pComponent2->GetOrder())? 1: -1;
}

void CaIvmTree::SortFolders (CTreeCtrl* pTree, HTREEITEM hRootInstallation)
{
	TV_SORTCB sortcb;

	sortcb.hParent = hRootInstallation;
	sortcb.lpfnCompare = fnCompareComponent;
	sortcb.lParam = NULL;

	pTree->SortChildrenCB (&sortcb);
}

void CaIvmTree::UpdateTreeData()
{
	CaTreeComponentItemData* pItem = NULL;
	POSITION p, pos = m_listFolder.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		pItem = m_listFolder.GetNext(pos);
		if (pItem->m_treeCtrlData.GetState () == CaTreeCtrlData::ITEM_DELETE)
		{
			m_listFolder.RemoveAt (p);
			delete pItem;
		}
		else
			pItem->UpdateTreeData();
	}
	UpdateState();
}

void CaIvmTree::UpdateState()
{
	CaTreeComponentItemData* pItem = NULL;
	POSITION pos = m_listFolder.GetHeadPosition();
	while (pos != NULL)
	{
		pItem = m_listFolder.GetNext(pos);
		pItem->UpdateState();
	}
}

void CaIvmTree::ResetState (CaTreeCtrlData::ItemState state, BOOL bInstance)
{
	CaTreeComponentItemData* pItem = NULL;
	POSITION pos = m_listFolder.GetHeadPosition();
	while (pos != NULL)
	{
		pItem = m_listFolder.GetNext(pos);
		pItem->ResetState (state, bInstance);
	}
}



void CaIvmTree::AlertNew  (CTreeCtrl* pTree, CaLoggedEvent* e)
{
	CaTreeComponentItemData* pItem = NULL;
	if (!e->IsAlert())
		return;
	//
	// Whatever important event arrived, we mark an alert on the concerned
	// branch and ALL ITS PARENT BRANCH:
	m_installationBranch.m_treeCtrlData.AlertInc();

	//
	// Find the folder of concerned event:
	POSITION pos = m_listFolder.GetHeadPosition();
	while (pos != NULL)
	{
		pItem = m_listFolder.GetNext(pos);
		//
		// The folder is found by category:
		if (pItem->m_strComponentType.CompareNoCase(e->m_strComponentType) == 0)
		{
			//
			// Mark the alert in the folder:
			pItem->m_treeCtrlData.AlertInc();

			//
			// Try to mark the alert in the sub-branch, in which the event
			// is supposed to be:
			if (pItem->AlertNew (pTree, e))
				return;
		}
	}
}

void CaIvmTree::AlertRead (CTreeCtrl* pTree, CaLoggedEvent* e)
{
	CaTreeComponentItemData* pItem = NULL;
	
	if (e == NULL)
	{
		//
		// Unalert the whole tree:
		m_installationBranch.m_treeCtrlData.AlertZero();

		POSITION pos = m_listFolder.GetHeadPosition();
		while (pos != NULL)
		{
			pItem = m_listFolder.GetNext(pos);
			// Unmark the alert in the folder:
			pItem->m_treeCtrlData.AlertZero();
			//
			// Try to unmark the alert in the sub-branch:
			pItem->AlertRead (pTree, NULL);
		}
		return;
	}


	if (!e->IsAlert())
		return;
	//
	// Whatever important event is read, we unmark an alert on the concerned
	// branch and ALL ITS PARENT BRANCH:
	m_installationBranch.m_treeCtrlData.AlertDec();

	//
	// Find the folder of concerned event:
	POSITION pos = m_listFolder.GetHeadPosition();
	while (pos != NULL)
	{
		pItem = m_listFolder.GetNext(pos);
		//
		// The folder is found by category:
		if (pItem->m_strComponentType.CompareNoCase(e->m_strComponentType) == 0)
		{
			//
			// Unmark the alert in the folder:
			pItem->m_treeCtrlData.AlertDec();
			//
			// Try to unmark the alert in the sub-branch, in which the event
			// is supposed to be:
			if (pItem->AlertRead (pTree, e))
				return;
		}
	}
}

void CaIvmTree::UpdateIcon(CTreeCtrl* pTree)
{
	int nChange = 0;
	Display(pTree, nChange);
}


void CaIvmTree::ExpandAll(CTreeCtrl* pTree)
{
	//
	// Expand the Installation branch:
	HTREEITEM hItem = m_installationBranch.m_treeCtrlData.GetTreeItem();
	if (hItem && !m_installationBranch.m_treeCtrlData.IsExpanded())
	{
		pTree->Expand (hItem, TVE_EXPAND);
		m_installationBranch.m_treeCtrlData.SetExpand (TRUE);
	}
	//
	// Expand the sub-branch of Installation's branch:
	CaTreeComponentItemData* pItem = NULL;
	POSITION pos = m_listFolder.GetHeadPosition();
	while (pos != NULL)
	{
		pItem = m_listFolder.GetNext(pos);
		pItem->ExpandAll (pTree);
	}
}

void CaIvmTree::CollapseAll(CTreeCtrl* pTree)
{
	//
	// Collapse the Installation branch:
	HTREEITEM hItem = m_installationBranch.m_treeCtrlData.GetTreeItem();
	if (hItem && m_installationBranch.m_treeCtrlData.IsExpanded())
	{
		pTree->Expand (hItem, TVE_COLLAPSE);
		m_installationBranch.m_treeCtrlData.SetExpand (FALSE);
	}
	//
	// Collapse the sub-branch of Installation's branch:
	CaTreeComponentItemData* pItem = NULL;
	POSITION pos = m_listFolder.GetHeadPosition();
	while (pos != NULL)
	{
		pItem = m_listFolder.GetNext(pos);
		pItem->CollapseAll (pTree);
	}
}

CaTreeComponentItemData* CaIvmTree::MatchInstance (LPCTSTR lpszInstance)
{
	CaTreeComponentItemData* pItem = NULL;
	CaTreeComponentItemData* pItemFound = NULL;
	POSITION pos = m_listFolder.GetHeadPosition();
	while (pos != NULL)
	{
		pItem = m_listFolder.GetNext(pos);
		pItemFound = pItem->MatchInstance(lpszInstance);
		if (pItemFound)
			return pItemFound;
	}
	return NULL;
}

BOOL CaIvmTree::IsMultipleInstancesRunning()
{
	CaTreeComponentItemData* pItem = NULL;
	POSITION pos = m_listFolder.GetHeadPosition();
	while (pos != NULL)
	{
		pItem = m_listFolder.GetNext(pos);
		if (pItem->IsMultipleInstancesRunning())
			return TRUE;
	}
	return FALSE;
}


//
// Logged events:
// **************
static void LocalTimeStamp(CString& strLocalTimeStamp)
{
	CTime t = CTime::GetCurrentTime();
	strLocalTimeStamp = t.Format (_T("%Y%m%d%H%M%S"));
}

IMPLEMENT_SERIAL (CaLoggedEvent, CObject, 1)
CaLoggedEvent::CaLoggedEvent()
	:m_strDate(_T("")), m_strTime(_T("")), m_strComponent(_T("")), m_strCategory(_T("")), m_strText(_T(""))
{
	m_eventClass = EVENT_NEW;
	m_strComponentType = _T("");
	m_strComponentName = _T("");
	m_strInstance = _T("");

	m_bAlert = FALSE;
	m_bRead = FALSE;
	m_bAltertNotify = FALSE;
	m_nIdentifier = 0;
	m_lCatType = CLASS_OTHERS;
	m_lCode = MSG_NOCODE;
	m_nClass = IMSG_NOTIFY;
	m_bClassified = FALSE;
	m_bNotFirstLine = FALSE;
	
	m_nCount = 1;
}

CaLoggedEvent::CaLoggedEvent(LPCTSTR lpszDate, LPCTSTR lpszTime, LPCTSTR lpszComponent, LPCTSTR lpszCategoty, LPCTSTR lpszText)
{
	m_strDate = lpszDate? lpszDate: _T("");
	m_strTime = lpszTime? lpszTime: _T("");
	m_strComponent = lpszComponent? lpszComponent: _T("");
	m_strCategory = lpszCategoty? lpszCategoty: _T("");
	m_strText = lpszText? lpszText: _T("");

	m_strComponentType = _T("");
	m_strComponentName = _T("");
	m_strInstance = _T("");
	m_eventClass = EVENT_NEW;

	m_bAlert = FALSE;
	m_bRead = FALSE;
	m_bAltertNotify = FALSE;
	m_nIdentifier = 0;
	m_lCatType = CLASS_OTHERS;
	m_lCode = MSG_NOCODE;
	m_nClass = IMSG_NOTIFY;
	m_bClassified = FALSE;
	m_bNotFirstLine = FALSE;

	m_nCount = 1;
}

CaLoggedEvent* CaLoggedEvent::Clone()
{
	CaLoggedEvent* pNewItem = new CaLoggedEvent(m_strDate, m_strTime, m_strComponent, m_strCategory, m_strText);
	pNewItem->m_strComponentType = m_strComponentType;
	pNewItem->m_strComponentName = m_strComponentName;
	pNewItem->m_strInstance = m_strInstance;
	pNewItem->m_eventClass = m_eventClass;
	pNewItem->SetAlert (m_bAlert);
	pNewItem->SetIdentifier (m_nIdentifier);
	pNewItem->SetRead (m_bRead);
	pNewItem->SetCatType(m_lCatType);
	pNewItem->SetCode(m_lCode);
	pNewItem->SetClass(m_nClass);
	pNewItem->SetClassify (m_bClassified);
	pNewItem->SetNotFirstLine (m_bNotFirstLine);
	return pNewItem;
}


BOOL CaLoggedEvent::MatchFilter (CaEventFilter* pFilter)
{
	if (!pFilter)
		return TRUE;
	else
	{
		if (pFilter->m_strCategory.CompareNoCase (theApp.m_strAll) == 0)
			return TRUE;

		if (pFilter->m_strCategory.CompareNoCase (m_strComponentType) != 0) {
			// for logging and locking system, the criteria doesn't follow the logic on
			// component type/name/instance, but a special criteria (LG / LK messages)
			if (IVM_IsLoggingSystemMessage(GetCatType(), GetCode()) &&
				pFilter->m_strCategory.CompareNoCase (GetLoggingSystemCompName()) == 0) {
				// (test in this order for performance reasons)
				return TRUE;
			}
			if (IVM_IsLockingSystemMessage(GetCatType(), GetCode()) &&
				pFilter->m_strCategory.CompareNoCase (GetLockingSystemCompName()) == 0) {
				// (test in this order for performance reasons)
					return TRUE;
			}
			// regular component types, and non-matching logging/locking system messages
			return FALSE;
		}
		if (pFilter->m_strName.CompareNoCase (theApp.m_strAll) == 0)
			return TRUE;
		if (pFilter->m_strName.CompareNoCase (m_strComponentName) != 0)
			return FALSE;

		if (pFilter->m_strInstance.CompareNoCase (theApp.m_strAll) == 0)
			return TRUE;
		if (pFilter->m_strInstance.CompareNoCase (m_strInstance) != 0)
			return FALSE;
		return TRUE;
	}
	return TRUE;
}

UINT CaLoggedEvent::GetSize()
{
	UINT nSize = sizeof (CaLoggedEvent);
	nSize+= m_strDate.GetLength();
	nSize+= m_strTime.GetLength();
	nSize+= m_strComponent.GetLength();
	nSize+= m_strCategory.GetLength();
	nSize+= m_strText.GetLength();
		
	nSize+= m_strComponentType.GetLength();
	nSize+= m_strComponentName.GetLength();
	nSize+= m_strInstance.GetLength();
	return nSize;
}

CString CaLoggedEvent::GetIvmFormatDate()
{
	try
	{
		CTime t = GetCTime();
		return t.Format (theApp.m_strTimeFormat);
	}
	catch (...)
	{
		return m_strDate;
	}
}

CTime CaLoggedEvent::GetCTime()
{
	TCHAR tchsz[6][5];
#if defined (_DEBUG) && defined (SIMULATION)
	ASSERT (m_strDate.GetLength() >= 19);
	if (m_strDate.GetLength() < 19)
		throw (int)100;
	// Year:
	tchsz[0][0] = m_strDate.GetAt(6);
	tchsz[0][1] = m_strDate.GetAt(7);
	tchsz[0][2] = m_strDate.GetAt(8);
	tchsz[0][3] = m_strDate.GetAt(9);
	tchsz[0][5] = _T('\0');
	// Month:
	tchsz[1][0] = m_strDate.GetAt(3);
	tchsz[1][1] = m_strDate.GetAt(4);
	tchsz[1][2] = _T('\0');
	// Day:
	tchsz[2][0] = m_strDate.GetAt(0);
	tchsz[2][1] = m_strDate.GetAt(1);
	tchsz[2][2] = _T('\0');
	// Hour:
	tchsz[3][0] = m_strDate.GetAt(11);
	tchsz[3][1] = m_strDate.GetAt(12);
	tchsz[3][2] = _T('\0');
	// Minute:
	tchsz[4][0] = m_strDate.GetAt(14);
	tchsz[4][1] = m_strDate.GetAt(15);
	tchsz[4][2] = _T('\0');
	// Second:
	tchsz[5][0] = m_strDate.GetAt(17);
	tchsz[5][1] = m_strDate.GetAt(18);
	tchsz[5][2] = _T('\0');
#else
	//
	// Actual format date in errlog.log is: ddd mmm dd hh:nn:ss yyyy
	// ex: Wed May 19 11:40:47 1999
	// eg: %a %b %d %H:%M:%s %Y
	
	//
	// In this conversion, we assume that the date format is always in ENGLISH
	// Otherwise this conversion will not work 
	int i;
	TCHAR tchszMonth  [12][4] = 
	{
		_T("Jan"),
		_T("Feb"),
		_T("Mar"),
		_T("Apr"),
		_T("May"),
		_T("Jun"),
		_T("Jul"),
		_T("Aug"),
		_T("Sep"),
		_T("Oct"),
		_T("Nov"),
		_T("Dec")
	};


	if (m_strDate.GetLength() != 24)
	{
		TRACE0 ("CaLoggedEvent::GetIvmFormatDate: m_strDate has incompatible format or empty\n");
		throw (int)100;
	}

	// Year:
	tchsz[0][0] = m_strDate.GetAt(20);
	tchsz[0][1] = m_strDate.GetAt(21);
	tchsz[0][2] = m_strDate.GetAt(22);
	tchsz[0][3] = m_strDate.GetAt(23);
	tchsz[0][4] = _T('\0');
	// Month:
	tchsz[1][0] = m_strDate.GetAt(4);
	tchsz[1][1] = m_strDate.GetAt(5);
	tchsz[1][2] = m_strDate.GetAt(6);
	tchsz[1][3] = _T('\0');
	BOOL bMonthOK = FALSE;
	for (i=0; i<12; i++)
	{
		if (lstrcmpi (tchsz[1], tchszMonth[i]) == 0)
		{
			wsprintf (tchsz[1], _T("%02d"), i+1);
			bMonthOK = TRUE;
			break;
		}
	}
	if (!bMonthOK)
	{
		// Default to the current month:
		CTime t = CTime::GetCurrentTime();
		wsprintf (tchsz[1], _T("%02d"), t.GetMonth());
	}
	// Day:
	tchsz[2][0] = m_strDate.GetAt(8);
	tchsz[2][1] = m_strDate.GetAt(9);
	tchsz[2][2] = _T('\0');
	// Hour:
	tchsz[3][0] = m_strDate.GetAt(11);
	tchsz[3][1] = m_strDate.GetAt(12);
	tchsz[3][2] = _T('\0');
	// Minute:
	tchsz[4][0] = m_strDate.GetAt(14);
	tchsz[4][1] = m_strDate.GetAt(15);
	tchsz[4][2] = _T('\0');
	// Second:
	tchsz[5][0] = m_strDate.GetAt(17);
	tchsz[5][1] = m_strDate.GetAt(18);
	tchsz[5][2] = _T('\0');
#endif
	CTime t(_ttoi (tchsz[0]), _ttoi (tchsz[1]), _ttoi (tchsz[2]), _ttoi (tchsz[3]), _ttoi (tchsz[4]), _ttoi (tchsz[5]));

	return t;
}

CString CaLoggedEvent::GetDateLocale()
{
	try
	{
		CTime t = GetCTime();
		return t.Format(_T("%c"));
	}
	catch (...)
	{
		return m_strDate;
	}
}


//
// CaIvmEvent:
// ------------------------------------

IMPLEMENT_SERIAL (CaIvmEvent, CObject, 1)

void CaIvmEvent::CleanUp()
{
	while (!m_listEvent.IsEmpty())
		delete m_listEvent.RemoveHead();
}

CaIvmEvent::~CaIvmEvent()
{
	CleanUp();
}

BOOL CaIvmEvent::Get (CTypedPtrList<CObList, CaLoggedEvent*>& listEvent, CaEventFilter* pFilter, BOOL bNew)
{
	CaLoggedEvent* pEvent = NULL;
	POSITION pos = m_listEvent.GetHeadPosition();
	while (pos != NULL)
	{
		pEvent = m_listEvent.GetNext (pos);
		if (bNew && pEvent->GetEventClass() != CaLoggedEvent::EVENT_NEW)
			continue;
		if (pEvent->MatchFilter (pFilter))
			listEvent.AddTail (pEvent);
	}

	return TRUE;
}

BOOL CaIvmEvent::GetAll (CTypedPtrList<CObList, CaLoggedEvent*>& listEvent)
{
	CaLoggedEvent* pEvent = NULL;
	POSITION pos = m_listEvent.GetHeadPosition();
	while (pos != NULL)
	{
		pEvent = m_listEvent.GetNext (pos);
		listEvent.AddTail (pEvent);
	}

	return TRUE;
}


UINT CaIvmEvent::GetEventSize()
{
	if (m_listEvent.GetCount() == 0)
		return 0;
	UINT nSize = 0;

	CaLoggedEvent* pEvent = NULL;
	POSITION pos = m_listEvent.GetHeadPosition();
	while (pos != NULL)
	{
		pEvent = m_listEvent.GetNext (pos);
		nSize += pEvent->GetSize();
	}
	return nSize;
}

CaIvmEventStartStop::CaIvmEventStartStop()
{

}

CaIvmEventStartStop::~CaIvmEventStartStop()
{
	CleanUp();
}

void CaIvmEventStartStop::CleanUp()
{
	while (!m_listEvent.IsEmpty())
		delete m_listEvent.RemoveHead();
}

void CaIvmEventStartStop::Add (CaLoggedEvent* pEvent)
{
	m_listEvent.AddTail (pEvent);
}



CaTreeDataMutex::CaTreeDataMutex():CaIvmDataMutex()
{
	m_bStatus = FALSE;
	HANDLE hMutex = theApp.GetMutexTreeData();
	if (!hMutex)
		return;
	DWORD dwWaitResult = WaitForSingleObject (hMutex, 15*60*1000);
	switch (dwWaitResult)
	{
	case WAIT_OBJECT_0:
		m_bStatus = TRUE;
		break;
	default:
		break;
	}
}

CaTreeDataMutex::~CaTreeDataMutex()
{
	HANDLE hMutex = theApp.GetMutexTreeData();
	if (!hMutex)
		return;
	if (m_bStatus)
		ReleaseMutex (hMutex);
}


CaEventDataMutex::CaEventDataMutex(long lmsWait):CaIvmDataMutex()
{
	long lWait = (lmsWait == 0L)? 30*1000: lmsWait;
	m_bStatus = FALSE;
	HANDLE hMutex = theApp.GetMutexEventData();
	if (!hMutex)
		return;
	DWORD dwWaitResult = WaitForSingleObject (hMutex, lWait);
	switch (dwWaitResult)
	{
	case WAIT_OBJECT_0:
		m_bStatus = TRUE;
		break;
	default:
		break;
	}
}

CaEventDataMutex::~CaEventDataMutex()
{
	HANDLE hMutex = theApp.GetMutexEventData();
	if (!hMutex)
		return;
	if (m_bStatus)
		ReleaseMutex (hMutex);
}


CaMutexInstance::CaMutexInstance():CaIvmDataMutex()
{
	m_bStatus = FALSE;
	HANDLE hMutex = theApp.GetMutexInstanceData();
	if (!hMutex)
		return;
	DWORD dwWaitResult = WaitForSingleObject (hMutex, 5*60*1000);
	switch (dwWaitResult)
	{
	case WAIT_OBJECT_0:
		m_bStatus = TRUE;
		break;
	default:
		break;
	}
}

CaMutexInstance::~CaMutexInstance()
{
	HANDLE hMutex = theApp.GetMutexInstanceData();
	if (!hMutex)
		return;
	if (m_bStatus)
		ReleaseMutex (hMutex);
}


void CaParseSpecialInstancesInfo::CleanUp()
{
	while (!m_listSpecialInstances.IsEmpty())
		delete m_listSpecialInstances.RemoveHead();

}

BOOL IsSpecialInstance(char * pinstancename, char * bufResType,char * bufResName)
{
	CaParseInstanceInfo * pInstance = NULL;
	POSITION  pos = theApp.GetSpecialInstanceInfo().GetSpecialInstancesList().GetHeadPosition();
	while (pos != NULL)
	{
		pInstance = theApp.GetSpecialInstanceInfo().GetSpecialInstancesList().GetNext(pos);
		if (pInstance->m_csInstance.CompareNoCase((LPCTSTR)pinstancename)==0 ) {
			strcpy(bufResName, (char *) (LPCTSTR)pInstance ->m_csConfigName);
			switch (pInstance->m_iservertype) {
				case COMP_TYPE_DBMS:
					strcpy(bufResType, GetDBMSCompName());
					break;
				case COMP_TYPE_STAR:
					strcpy(bufResType, GetStarServCompName());
					break;
				case COMP_TYPE_NET:
					strcpy(bufResType, GetNetSvrCompName());
					break;
				case COMP_TYPE_DASVR:
					strcpy(bufResType, GetDASVRSvrCompName());
					break;
				case COMP_TYPE_BRIDGE:
					strcpy(bufResType, GetBridgeSvrCompName());
					break;
				case COMP_TYPE_INTERNET_COM:
					strcpy(bufResType, GetICESvrCompName());
					break;
				case COMP_TYPE_NAME:
					strcpy(bufResType, GetNameSvrCompName());
					break;
				default :
					AfxMessageBox(_T("Internal Error / Msg Type"));
					strcpy(bufResType, "");
					break;
			}
			return TRUE;
		}
	}
	return FALSE;
}



//
// **************************** BEGIN SECTION OF HELP MANAGEMENT ****************************

static int HelpNAMEServer (TreeBranchType nTreType, UINT nDialogID)
{
	if (nTreType == TREE_FOLDER)
	{
		//
		// Name has no folder:
		ASSERT (FALSE);
		return 0;
	}

	if (nTreType == TREE_COMPONENTNAME)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_C_NAME_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_C_NAME_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_C_NAME_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}

	if (nTreType == TREE_INSTANCE)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_I_NAME_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_I_NAME_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_I_NAME_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}
	//
	// Invalid help request:
	ASSERT (FALSE);
	return 0;
}

static int HelpDBMSServer (TreeBranchType nTreType, UINT nDialogID)
{
	if (nTreType == TREE_FOLDER)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_F_DBMS_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_F_DBMS_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_F_DBMS_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}

	if (nTreType == TREE_COMPONENTNAME)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_C_DBMS_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_C_DBMS_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_C_DBMS_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}

	if (nTreType == TREE_INSTANCE)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_I_DBMS_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_I_DBMS_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_I_DBMS_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}
	//
	// Invalid help request:
	ASSERT (FALSE);
	return 0;
}

static int HelpICEServer (TreeBranchType nTreType, UINT nDialogID)
{
	if (nTreType == TREE_FOLDER)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_F_ICE_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_F_ICE_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_F_ICE_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}

	if (nTreType == TREE_COMPONENTNAME)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_C_ICE_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_C_ICE_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_C_ICE_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}

	if (nTreType == TREE_INSTANCE)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_I_ICE_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_I_ICE_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_I_ICE_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}
	//
	// Invalid help request:
	ASSERT (FALSE);
	return 0;
}


static int HelpNETServer (TreeBranchType nTreType, UINT nDialogID)
{
	if (nTreType == TREE_FOLDER)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_F_NET_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_F_NET_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_F_NET_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}

	if (nTreType == TREE_COMPONENTNAME)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_C_NET_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_C_NET_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_C_NET_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}

	if (nTreType == TREE_INSTANCE)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_I_NET_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_I_NET_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_I_NET_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}
	//
	// Invalid help request:
	ASSERT (FALSE);
	return 0;
}


static int HelpSTARServer (TreeBranchType nTreType, UINT nDialogID)
{
	if (nTreType == TREE_FOLDER)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_F_STAR_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_F_STAR_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_F_STAR_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}

	if (nTreType == TREE_COMPONENTNAME)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_C_STAR_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_C_STAR_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_C_STAR_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}

	if (nTreType == TREE_INSTANCE)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_I_STAR_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_I_STAR_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_I_STAR_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}
	//
	// Invalid help request:
	ASSERT (FALSE);
	return 0;
}

static int HelpJDBCServer (TreeBranchType nTreType, UINT nDialogID)
{
	if (nTreType == TREE_FOLDER)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_F_JDBC_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_F_JDBC_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_F_JDBC_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}

	if (nTreType == TREE_COMPONENTNAME)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_C_JDBC_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_C_JDBC_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_C_JDBC_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}

	if (nTreType == TREE_INSTANCE)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_I_JDBC_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_I_JDBC_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_I_JDBC_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}
	//
	// Invalid help request:
	ASSERT (FALSE);
	return 0;
}

static int HelpDASVRServer (TreeBranchType nTreType, UINT nDialogID)
{
	if (nTreType == TREE_FOLDER)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_F_DASVR_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_F_DASVR_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_F_DASVR_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}

	if (nTreType == TREE_COMPONENTNAME)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_C_DASVR_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_C_DASVR_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_C_DASVR_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}

	if (nTreType == TREE_INSTANCE)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_I_DASVR_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_I_DASVR_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_I_DASVR_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}
	//
	// Invalid help request:
	ASSERT (FALSE);
	return 0;
}

static int HelpBRIDGEServer (TreeBranchType nTreType, UINT nDialogID)
{
	if (nTreType == TREE_FOLDER)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_F_BRIDGE_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_F_BRIDGE_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_F_BRIDGE_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}

	if (nTreType == TREE_COMPONENTNAME)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_C_BRIDGE_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_C_BRIDGE_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_C_BRIDGE_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}

	if (nTreType == TREE_INSTANCE)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_I_BRIDGE_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_I_BRIDGE_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_I_BRIDGE_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}
	//
	// Invalid help request:
	ASSERT (FALSE);
	return 0;
}

static int HelpSECURITYServer (TreeBranchType nTreType, UINT nDialogID)
{
	if (nTreType == TREE_FOLDER)
	{
		// 
		// No Folder for Security:
		ASSERT (FALSE);

		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_F_SECURITY_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_F_SECURITY_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_F_SECURITY_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}

	if (nTreType == TREE_COMPONENTNAME)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_C_SECURITY_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_C_SECURITY_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_C_SECURITY_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}

	if (nTreType == TREE_INSTANCE)
	{
		//
		// No Instance for Security:
		ASSERT (FALSE);

		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_I_SECURITY_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_I_SECURITY_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_I_SECURITY_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}
	//
	// Invalid help request:
	ASSERT (FALSE);
	return 0;
}

static int HelpLOCKINGServer (TreeBranchType nTreType, UINT nDialogID)
{
	if (nTreType == TREE_FOLDER)
	{
		//
		// No Folder for Locking:
		ASSERT (FALSE);

		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_F_LOCKING_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_F_LOCKING_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_F_LOCKING_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}

	if (nTreType == TREE_COMPONENTNAME)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_C_LOCKING_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_C_LOCKING_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_C_LOCKING_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}

	if (nTreType == TREE_INSTANCE)
	{
		//
		// No Instance of Locking:
		ASSERT (FALSE);

		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_I_LOCKING_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_I_LOCKING_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_I_LOCKING_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}
	//
	// Invalid help request:
	ASSERT (FALSE);
	return 0;
}


static int HelpLOGGINGServer (TreeBranchType nTreType, UINT nDialogID)
{
	if (nTreType == TREE_FOLDER)
	{
		//
		// No Folder for Logging:
		ASSERT (FALSE);

		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_F_LOGGING_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_F_LOGGING_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_F_LOGGING_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}

	if (nTreType == TREE_COMPONENTNAME)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_C_LOGGING_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_C_LOGGING_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_C_LOGGING_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}

	if (nTreType == TREE_INSTANCE)
	{
		//
		// No Instance for Logging:
		ASSERT (FALSE);

		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_I_LOGGING_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_I_LOGGING_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_I_LOGGING_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}
	//
	// Invalid help request:
	ASSERT (FALSE);
	return 0;
}

static int HelpLOGFILEServer (TreeBranchType nTreType, UINT nDialogID)
{
	if (nTreType == TREE_FOLDER)
	{
		//
		// No Folder of Logfile 
		ASSERT (FALSE);

		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_F_LOGFILE_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_F_LOGFILE_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_F_LOGFILE_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}

	if (nTreType == TREE_COMPONENTNAME)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_C_LOGFILE_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_C_LOGFILE_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_C_LOGFILE_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}

	if (nTreType == TREE_INSTANCE)
	{
		//
		// No Instance of Logfile
		ASSERT (FALSE);

		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_I_LOGFILE_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_I_LOGFILE_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_I_LOGFILE_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}
	//
	// Invalid help request:
	ASSERT (FALSE);
	return 0;
}

static int HelpRECOVERYServer (TreeBranchType nTreType, UINT nDialogID)
{
	if (nTreType == TREE_FOLDER)
	{
		//
		// Actually, no Folder for Recovery:
		ASSERT (FALSE);

		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_F_RECOVER_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_F_RECOVER_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_F_RECOVER_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}

	if (nTreType == TREE_COMPONENTNAME)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_C_RECOVER_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_C_RECOVER_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_C_RECOVER_STATEVENT;
		case IDD_LOGEVENT_ACPRCP:
			return HELPBASE_C_RECOVER_LOGFILE;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}

	if (nTreType == TREE_INSTANCE)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_I_RECOVER_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_I_RECOVER_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_I_RECOVER_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}
	//
	// Invalid help request:
	ASSERT (FALSE);
	return 0;
}


static int HelpARCHIVERServer (TreeBranchType nTreType, UINT nDialogID)
{
	if (nTreType == TREE_FOLDER)
	{
		//
		// Actually, no Folder for Archiver:
		ASSERT (FALSE);

		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_F_ARCHIVER_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_F_ARCHIVER_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_F_ARCHIVER_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}

	if (nTreType == TREE_COMPONENTNAME)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_C_ARCHIVER_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_C_ARCHIVER_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_C_ARCHIVER_STATEVENT;
		case IDD_LOGEVENT_ACPRCP:
			return HELPBASE_C_ARCHIVER_LOGFILE;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}

	if (nTreType == TREE_INSTANCE)
	{
		//
		// Actually, no instance of Archiver:
		ASSERT (FALSE);

		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_I_ARCHIVER_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_I_ARCHIVER_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_I_ARCHIVER_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}
	//
	// Invalid help request:
	ASSERT (FALSE);
	return 0;
}

static int HelpRMCMDServer (TreeBranchType nTreType, UINT nDialogID)
{
	if (nTreType == TREE_FOLDER)
	{
		//
		// Actually, no Folder for rmcmd:
		ASSERT (FALSE);

		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_F_RMCMD_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_F_RMCMD_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_F_RMCMD_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}

	if (nTreType == TREE_COMPONENTNAME)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_C_RMCMD_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_C_RMCMD_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_C_RMCMD_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}

	if (nTreType == TREE_INSTANCE)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			return HELPBASE_I_RMCMD_STATUS;
		case IDD_LOGEVENT_GENERIC:
			return HELPBASE_I_RMCMD_EVENT;
		case IDD_MESSAGE_STATISTIC:
			return HELPBASE_I_RMCMD_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
	}
	//
	// Invalid help request:
	ASSERT (FALSE);
	return 0;
}

static int HelpGATEWAYServer (TreeBranchType nTreType, int nComponentType, UINT nDialogID)
{
	if (nTreType == TREE_FOLDER)
	{
		//
		// Must be a folder:
		ASSERT (nComponentType == COMP_TYPE_GW_FOLDER);
		if (nComponentType == COMP_TYPE_GW_FOLDER)
		{
			switch (nDialogID)
			{
			case IDD_STATUS_AUTOSTART_COMPONENT:
			case IDD_STATUS_DUPLICATABLE_COMPONENT:
			case IDD_STATUS_INSTANCE:
			case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
				return HELPBASE_F_GATEWAY_STATUS;
			case IDD_LOGEVENT_GENERIC:
				return HELPBASE_F_GATEWAY_EVENT;
			case IDD_MESSAGE_STATISTIC:
				return HELPBASE_F_GATEWAY_STATEVENT;
			default:
				ASSERT (FALSE);
				return 0;
			}
		}
		//
		// Invalid Hep Request:
		ASSERT (FALSE);
		return 0;
	}

	if (nTreType == TREE_COMPONENTNAME)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			switch (nComponentType)
			{
			case COMP_TYPE_GW_ORACLE:
				return HELPBASE_C_ORACLE_STATUS;
			case COMP_TYPE_GW_INFORMIX:
				return HELPBASE_C_INFORMIX_STATUS;
			case COMP_TYPE_GW_SYBASE:
				return HELPBASE_C_SYBASE_STATUS;
			case COMP_TYPE_GW_MSSQL:
				return HELPBASE_C_MSSQL_STATUS;
			case COMP_TYPE_GW_ODBC:
				return HELPBASE_C_ODBC_STATUS;
			default:
				//
				// Invalid help request:
				ASSERT (FALSE);
				return 0;
			}
			break;

		case IDD_LOGEVENT_GENERIC:
			switch (nComponentType)
			{
			case COMP_TYPE_GW_ORACLE:
				return HELPBASE_C_ORACLE_EVENT;
			case COMP_TYPE_GW_INFORMIX:
				return HELPBASE_C_INFORMIX_EVENT;
			case COMP_TYPE_GW_SYBASE:
				return HELPBASE_C_SYBASE_EVENT;
			case COMP_TYPE_GW_MSSQL:
				return HELPBASE_C_MSSQL_EVENT;
			case COMP_TYPE_GW_ODBC:
				return HELPBASE_C_ODBC_EVENT;
			default:
				//
				// Invalid help request:
				ASSERT (FALSE);
				return 0;
			}
			break;

		case IDD_MESSAGE_STATISTIC:
			switch (nComponentType)
			{
			case COMP_TYPE_GW_ORACLE:
				return HELPBASE_C_ORACLE_STATEVENT;
			case COMP_TYPE_GW_INFORMIX:
				return HELPBASE_C_INFORMIX_STATEVENT;
			case COMP_TYPE_GW_SYBASE:
				return HELPBASE_C_SYBASE_STATEVENT;
			case COMP_TYPE_GW_MSSQL:
				return HELPBASE_C_MSSQL_STATEVENT;
			case COMP_TYPE_GW_ODBC:
				return HELPBASE_C_ODBC_STATEVENT;
			default:
				//
				// Invalid help request:
				ASSERT (FALSE);
				return 0;
			}
			break;

		default:
			ASSERT (FALSE);
			return 0;
		}
	}

	if (nTreType == TREE_INSTANCE)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_AUTOSTART_COMPONENT:
		case IDD_STATUS_DUPLICATABLE_COMPONENT:
		case IDD_STATUS_INSTANCE:
		case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
			switch (nComponentType)
			{
			case COMP_TYPE_GW_ORACLE:
				return HELPBASE_I_ORACLE_STATUS;
			case COMP_TYPE_GW_INFORMIX:
				return HELPBASE_I_INFORMIX_STATUS;
			case COMP_TYPE_GW_SYBASE:
				return HELPBASE_I_SYBASE_STATUS;
			case COMP_TYPE_GW_MSSQL:
				return HELPBASE_I_MSSQL_STATUS;
			case COMP_TYPE_GW_ODBC:
				return HELPBASE_I_ODBC_STATUS;
			default:
				//
				// Invalid help request:
				ASSERT (FALSE);
				return 0;
			}
			break;

		case IDD_LOGEVENT_GENERIC:
			switch (nComponentType)
			{
			case COMP_TYPE_GW_ORACLE:
				return HELPBASE_I_ORACLE_EVENT;
			case COMP_TYPE_GW_INFORMIX:
				return HELPBASE_I_INFORMIX_EVENT;
			case COMP_TYPE_GW_SYBASE:
				return HELPBASE_I_SYBASE_EVENT;
			case COMP_TYPE_GW_MSSQL:
				return HELPBASE_I_MSSQL_EVENT;
			case COMP_TYPE_GW_ODBC:
				return HELPBASE_I_ODBC_EVENT;
			default:
				//
				// Invalid help request:
				ASSERT (FALSE);
				return 0;
			}
			break;

		case IDD_MESSAGE_STATISTIC:
			switch (nComponentType)
			{
			case COMP_TYPE_GW_ORACLE:
				return HELPBASE_I_ORACLE_STATEVENT;
			case COMP_TYPE_GW_INFORMIX:
				return HELPBASE_I_INFORMIX_STATEVENT;
			case COMP_TYPE_GW_SYBASE:
				return HELPBASE_I_SYBASE_STATEVENT;
			case COMP_TYPE_GW_MSSQL:
				return HELPBASE_I_MSSQL_STATEVENT;
			case COMP_TYPE_GW_ODBC:
				return HELPBASE_I_ODBC_STATEVENT;
			default:
				//
				// Invalid help request:
				ASSERT (FALSE);
				return 0;
			}
			break;

		default:
			ASSERT (FALSE);
			return 0;
		}
	}
	//
	// Invalid Hep Request:
	ASSERT (FALSE);
	return 0;
}


//
// This function return the value of HELPBASE_F_XX, HELPBASE_C_XX, HELPBASE_I_XX
// depending on 'nTreType' and 'nComponentType'
int IVM_HelpBase(TreeBranchType nTreType, int nComponentType, UINT nDialogID)
{
	if (nComponentType == -1 && nTreType == TREE_INSTALLATION)
	{
		switch (nDialogID)
		{
		case IDD_STATUS_INSTALLATION:     return HELPBASE_INSTALLATION_STATUS;
		case IDD_PARAMETER_INSTALLATION:  return HELPBASE_INSTALLATION_PARAMETER;
		case IDD_LOGEVENT_GENERIC:        return HELPBASE_INSTALLATION_EVENT;
		case IDD_MESSAGE_STATISTIC:       return HELPBASE_INSTALLATION_STATEVENT;
		default:
			ASSERT (FALSE);
			return 0;
		}
		return 0;
	}

	switch (nComponentType)
	{
	case COMP_TYPE_NAME:
		return HelpNAMEServer(nTreType, nDialogID);
		break;
	case COMP_TYPE_DBMS:
		return HelpDBMSServer(nTreType, nDialogID);
	case COMP_TYPE_INTERNET_COM:
		return HelpICEServer(nTreType, nDialogID);
	case COMP_TYPE_NET:
		return HelpNETServer(nTreType, nDialogID);
	case COMP_TYPE_JDBC:
		return HelpJDBCServer(nTreType, nDialogID);
	case COMP_TYPE_DASVR:
		return HelpDASVRServer(nTreType, nDialogID);
	case COMP_TYPE_BRIDGE:
		return HelpBRIDGEServer(nTreType, nDialogID);
	case COMP_TYPE_STAR:
		return HelpSTARServer(nTreType, nDialogID);
	case COMP_TYPE_SECURITY:
		return HelpSECURITYServer(nTreType, nDialogID);
	case COMP_TYPE_LOCKING_SYSTEM:
		return HelpLOCKINGServer(nTreType, nDialogID);
	case COMP_TYPE_LOGGING_SYSTEM:
		return HelpLOGGINGServer(nTreType, nDialogID);
	case COMP_TYPE_TRANSACTION_LOG:
		return HelpLOGFILEServer(nTreType, nDialogID);
	case COMP_TYPE_RECOVERY:
		return HelpRECOVERYServer(nTreType, nDialogID);
	case COMP_TYPE_ARCHIVER:
		return HelpARCHIVERServer(nTreType, nDialogID);
	case COMP_TYPE_RMCMD:
		return HelpRMCMDServer(nTreType, nDialogID);
	case COMP_TYPE_GW_ORACLE:
	case COMP_TYPE_GW_DB2UDB:
	case COMP_TYPE_GW_INFORMIX:
	case COMP_TYPE_GW_SYBASE:
	case COMP_TYPE_GW_MSSQL:
	case COMP_TYPE_GW_ODBC:
	case COMP_TYPE_GW_FOLDER:
		return HelpGATEWAYServer(nTreType, nComponentType, nDialogID);
	case COMP_TYPE_OIDSK_DBMS:
		//
		// Ivm component has not been implement for DESTOP:
		ASSERT (FALSE);
		return 0;
	default:
		break;
	}

	//
	// Invalid base:
	return 0;
}

//
// **************************** END SECTION OF HELP MANAGEMENT ****************************

