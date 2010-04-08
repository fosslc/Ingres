/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : cbfitem.cpp, Implementation file 
**    Project  : OpenIngres Configuration Manager
**    Author   : UK Sotheavut 
**               Blattes Emannuel 
**
**    Purpose  : Item data to store information about item (Components) in the List
**               Control of the Lef-pane of Tab Configure.
**   
** History:
**
** xx-Sep-1997 (uk$so01)
**    Created
** 22-nov-98 (cucjo01)
**    Added startup count tabs
** 09-dec-98 (cucjo01)
**    Added ICE Server Page
**    Added Gateway Pages for Oracle, Informix, Sybase, MSSQL, and ODBC
** 16-may-00 (cucjo01)
**    Added JDBC Server Page.
** 06-Jun-2000: (uk$so01) 
**    (BUG #99242)
**    Cleanup code for DBCS compliant
** 16-Oct-2001 (uk$so01)
**    BUG #106053, free the unused memory
** 31-Oct-2001 (hanje04)
**    Move declaration of i in LowLevelDeactivation to outside for loop to 
**    stop non-ANSI compiler errors on Linux.
** 02-Oct-2002 (noifr01)
**    (SIR 107587) have VCBF manage DB2UDB gateway (mapping it to
**    oracle gateway code until this is moved to generic management)
** 01-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
** 20-jun-2003 (uk$so01)
**    SIR #110389 / INGDBA210
**    Add functionalities for configuring the GCF Security and
**    the security mechanisms.
** 07-Jul-2003 (schph01)
**    (bug 106858) consistently with CBF change 455479
**    Now the "Security" branch is always displayed, but with Ingres installation Client-only,
**    only two tabs are displayed "System" and "Mechanisms"
** 25-Sep-2003 (uk$so01)
**    SIR #105781 of cbf
** 12-Mar-2004 (uk$so01)
**    SIR #110013 (Handle the DAS Server)
** 06-May-2007 (drivi01)
**    SIR #118462
**    Add Item data for DB2 UDB Gateway.  Previously it used Oracle 
**    item data.    
*/


#include "stdafx.h"
#include "resource.h"
#include "cbfitem.h"
#include "ll.h"
#include "cleftdlg.h"
#include "vcbf.h"   // CeVcbfException

IMPLEMENT_SERIAL (CuCbfListViewItem, CObject, 1)

// constructor for static item
CuCbfListViewItem::CuCbfListViewItem()
{
	m_pPageInfo = NULL;
	m_pLeftDlg  = NULL;
	m_ActivationCount = 0;
}


CuCbfListViewItem::~CuCbfListViewItem()
{
	if (m_pPageInfo) 
		delete m_pPageInfo;
}

CuCbfListViewItem* CuCbfListViewItem::Copy()
{
	CuCbfListViewItem* pObj = new CuCbfListViewItem();
	return pObj;
}

void CuCbfListViewItem::Serialize (CArchive& ar)
{

	if (ar.IsStoring()) 
	{
		ar << m_pPageInfo;
	}
	else 
	{
		ar >> m_pPageInfo;
	}
}


BOOL CuCbfListViewItem::LowLevelActivation()
{
	return VCBFllInitGenericPane(&m_componentInfo);
}

BOOL CuCbfListViewItem::LowLevelDeactivation()
{
	return VCBFllOnMainComponentExit(&m_componentInfo);
}

BOOL CuCbfListViewItem::LowLevelActivationWithCheck()
{
  // Only one activation at a time
  ASSERT (m_ActivationCount >= 0);
  if (m_ActivationCount >= 1)
    return TRUE;
  m_ActivationCount++;
  BOOL bRet;
  try
  {
    bRet = LowLevelActivation();
  } // end of try
	catch (CeVcbfException e)
	{
		TRACE1 ("LowLevelActivationWithCheck has caught exception: %s\n", e.m_strReason);
		CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
		pMain->CloseApplication (FALSE);
	}
	catch (...)
	{
		TRACE0 ("Other error occured ...\n");
	}
  return bRet;
}

BOOL CuCbfListViewItem::LowLevelDeactivationWithCheck()
{
  // No deactivation if already done (only case: exit the product when active pane is not components list)
  ASSERT (m_ActivationCount >= 0);
  if (m_ActivationCount == 0)
    return TRUE;
  m_ActivationCount--;
  BOOL bRet;
  try
  {
    bRet = LowLevelDeactivation();
  } // end of try
	catch (CeVcbfException e)
	{
		TRACE1 ("LowLevelDeactivationWithCheck has caught exception: %s\n", e.m_strReason);
		CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
		pMain->CloseApplication (FALSE);
	}
	catch (...)
	{
		TRACE0 ("Other error occured ...\n");
	}
  return bRet;
}


//
// NAME Item data
// --------------

IMPLEMENT_SERIAL (CuNAMEItemData, CuCbfListViewItem, 1)
CuNAMEItemData::CuNAMEItemData():CuCbfListViewItem()
{

}

CuCbfListViewItem* CuNAMEItemData::Copy()
{
	ASSERT (FALSE);
	return NULL;
	CuNAMEItemData*	pObj = new CuNAMEItemData();
	return (CuCbfListViewItem*)pObj;
}

CuPageInformation* CuNAMEItemData::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [2] = 
	{
		IDS_TAB_CONFRIGHT_PARAMETERS,
		IDS_TAB_CONFRIGHT_STARTUP_COUNT
	};
	UINT nDlgID [2] = 
	{
		IDD_NAME_PAGE_PARAMETER,
		IDD_GENERIC_PAGE_STARTUP_COUNT
	};
	try
	{
		m_pPageInfo = new CuPageInformation ((LPCTSTR)"NAME", 2, nTabID, nDlgID);
	}
	catch (...)
	{
		throw;
	}
	return m_pPageInfo;
}


//
// DBMS Item data
// --------------
IMPLEMENT_SERIAL (CuDBMSItemData, CuCbfListViewItem, 1)
CuDBMSItemData::CuDBMSItemData():CuCbfListViewItem()
{
  // for databases management
  // because of calls to VCBFll_dblist_init() and VCBFOnDBListExit()
  m_bDbListModified   = FALSE;
  m_initialDbList     = NULL;
  m_resultDbList      = "";
  m_bMustFillListbox  = FALSE;
}

CuCbfListViewItem* CuDBMSItemData::Copy()
{
	CuDBMSItemData*	pObj = new CuDBMSItemData();
	return (CuCbfListViewItem*)pObj;
}


CuPageInformation* CuDBMSItemData::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [5] = 
	{
		IDS_TAB_CONFRIGHT_PARAMETERS,
		IDS_TAB_CONFRIGHT_DATABASES,
		IDS_TAB_CONFRIGHT_CACHE,
		IDS_TAB_CONFRIGHT_DERIVED,
		IDS_TAB_CONFRIGHT_STARTUP_COUNT
	};
	UINT nDlgID [5] = 
	{
		IDD_DBMS_PAGE_PARAMETER,
		IDD_DBMS_PAGE_DATABASE,
		IDD_DBMS_PAGE_CACHE,
		IDD_DBMS_PAGE_DERIVED,
		IDD_GENERIC_PAGE_STARTUP_COUNT
	};
	try
	{
		m_pPageInfo = new CuPageInformation ((LPCTSTR)"DBMS", 5, nTabID, nDlgID);
	}
	catch (...)
	{
		throw;
	}
	return m_pPageInfo;
}

BOOL CuDBMSItemData::LowLevelActivation()
{
  BOOL bSuccess = VCBFllInitGenericPane(&m_componentInfo);
  m_initialDbList = VCBFll_dblist_init();
  m_resultDbList = _T("");
  m_bDbListModified = FALSE;
  m_bMustFillListbox = TRUE;
  return bSuccess;
}

BOOL CuDBMSItemData::LowLevelDeactivation()
{
	BOOL bFlag = VCBFllOnDBListExit((LPTSTR)(LPCTSTR)m_resultDbList, m_bDbListModified);

	BOOL bDontCare = VCBFllOnMainComponentExit(&m_componentInfo);
	return bDontCare;
}

//
// ICE Item data
// --------------

IMPLEMENT_SERIAL (CuICEItemData, CuCbfListViewItem, 1)
CuICEItemData::CuICEItemData():CuCbfListViewItem()
{

}

CuCbfListViewItem* CuICEItemData::Copy()
{
	ASSERT (FALSE);
	return NULL;
	CuICEItemData*	pObj = new CuICEItemData();
	return (CuCbfListViewItem*)pObj;
}

CuPageInformation* CuICEItemData::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [2] = 
	{
		IDS_TAB_CONFRIGHT_PARAMETERS,
		IDS_TAB_CONFRIGHT_STARTUP_COUNT
	};
	UINT nDlgID [2] = 
	{
		IDD_INTERNET_PAGE_PARAMETER,
		IDD_GENERIC_PAGE_STARTUP_COUNT
	};
	try
	{
		m_pPageInfo = new CuPageInformation ((LPCTSTR)"ICESVR", 2, nTabID, nDlgID);
	}
	catch (...)
	{
		throw;
	}
	return m_pPageInfo;
}

//
// NET Item data
// --------------

IMPLEMENT_SERIAL (CuNETItemData, CuCbfListViewItem, 1)
CuNETItemData::CuNETItemData():CuCbfListViewItem()
{

}

CuCbfListViewItem* CuNETItemData::Copy()
{
	CuNETItemData*	pObj = new CuNETItemData();
	return (CuCbfListViewItem*)pObj;
}

CuPageInformation* CuNETItemData::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	const int cCount = 4;
	UINT nTabID [cCount] = 
	{
		IDS_TAB_CONFRIGHT_PARAMETERS,
		IDS_TAB_CONFRIGHT_PROTOCOL,
		IDS_TAB_CONFRIGHT_REGISTRY,
		IDS_TAB_CONFRIGHT_STARTUP_COUNT
	};
	UINT nDlgID [cCount] = 
	{
		IDD_NET_PAGE_PARAMETER,
		IDD_NET_PAGE_PROTOCOL,
		IDD_NET_PAGE_REGISTRY,
		IDD_GENERIC_PAGE_STARTUP_COUNT
	};
	try
	{
		m_pPageInfo = new CuPageInformation (_T("NET"), cCount, nTabID, nDlgID);
	}
	catch (...)
	{
		throw;
	}
	return m_pPageInfo;
}

//
// JDBC Item data
// --------------

IMPLEMENT_SERIAL (CuJDBCItemData, CuCbfListViewItem, 1)
CuJDBCItemData::CuJDBCItemData():CuCbfListViewItem()
{

}

CuCbfListViewItem* CuJDBCItemData::Copy()
{
	CuJDBCItemData*	pObj = new CuJDBCItemData();
	return (CuCbfListViewItem*)pObj;
}

CuPageInformation* CuJDBCItemData::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [2] = 
	{
		IDS_TAB_CONFRIGHT_PARAMETERS,
		IDS_TAB_CONFRIGHT_STARTUP_COUNT
	};
	UINT nDlgID [2] = 
	{
		IDD_JDBC_PAGE_PARAMETER,
		IDD_GENERIC_PAGE_STARTUP_COUNT
	};
	try
	{
		m_pPageInfo = new CuPageInformation ((LPCTSTR)"JDBC", 2, nTabID, nDlgID);
	}
	catch (...)
	{
		throw;
	}
	return m_pPageInfo;
}

//
// DASVR Item data
// --------------
IMPLEMENT_SERIAL (CuDASVRItemData, CuCbfListViewItem, 1)
CuDASVRItemData::CuDASVRItemData():CuCbfListViewItem()
{

}

CuCbfListViewItem* CuDASVRItemData::Copy()
{
	CuDASVRItemData*	pObj = new CuDASVRItemData();
	return (CuCbfListViewItem*)pObj;
}

CuPageInformation* CuDASVRItemData::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [3] = 
	{
		IDS_TAB_CONFRIGHT_PARAMETERS,
		IDS_TAB_CONFRIGHT_PROTOCOL,
		IDS_TAB_CONFRIGHT_STARTUP_COUNT
	};
	UINT nDlgID [3] = 
	{
		IDD_DASVR_PAGE_PARAMETER,
		IDD_DASVR_PAGE_PROTOCOL,
		IDD_GENERIC_PAGE_STARTUP_COUNT
	};
	
	m_pPageInfo = new CuPageInformation (_T("DASVR"), 3, nTabID, nDlgID);
		return m_pPageInfo;
}

//
// BRIDGE Item data
// --------------

IMPLEMENT_SERIAL (CuBRIDGEItemData, CuCbfListViewItem, 1)
CuBRIDGEItemData::CuBRIDGEItemData():CuCbfListViewItem()
{

}

CuCbfListViewItem* CuBRIDGEItemData::Copy()
{
	CuBRIDGEItemData*	pObj = new CuBRIDGEItemData();
	return (CuCbfListViewItem*)pObj;
}

CuPageInformation* CuBRIDGEItemData::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [4] = 
	{
		IDS_TAB_CONFRIGHT_PARAMETERS,
		IDS_TAB_CONFRIGHT_PROTOCOL,
		IDS_TAB_CONFRIGHT_VNODE,
		IDS_TAB_CONFRIGHT_STARTUP_COUNT
	};
	UINT nDlgID [4] = 
	{
		IDD_BRIDGE_PAGE_PARAMETER,
		IDD_BRIDGE_PAGE_PROTOCOL,
		IDD_BRIDGE_PAGE_VNODE,
		IDD_GENERIC_PAGE_STARTUP_COUNT
	};
	try
	{
		m_pPageInfo = new CuPageInformation (_T("BRIDGE"), 4, nTabID, nDlgID);
	}
	catch (...)
	{
		throw;
	}
	return m_pPageInfo;
}

BOOL CuBRIDGEItemData::LowLevelActivation()
{
	return CuCbfListViewItem::LowLevelActivation();
}

BOOL CuBRIDGEItemData::LowLevelDeactivation()
{
	return CuCbfListViewItem::LowLevelDeactivation();
}

//
// STAR Item data
// --------------

IMPLEMENT_SERIAL (CuSTARItemData, CuCbfListViewItem, 1)
CuSTARItemData::CuSTARItemData():CuCbfListViewItem()
{

}

CuCbfListViewItem* CuSTARItemData::Copy()
{
	CuSTARItemData*	pObj = new CuSTARItemData();
	return (CuCbfListViewItem*)pObj;
}

CuPageInformation* CuSTARItemData::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [3] = 
	{
		IDS_TAB_CONFRIGHT_PARAMETERS,
		IDS_TAB_CONFRIGHT_DERIVED,
		IDS_TAB_CONFRIGHT_STARTUP_COUNT
	};
	UINT nDlgID [3] = 
	{
		IDD_STAR_PAGE_PARAMETER,
		IDD_STAR_PAGE_DERIVED,
		IDD_GENERIC_PAGE_STARTUP_COUNT
	};

	try
	{
		m_pPageInfo = new CuPageInformation ((LPCTSTR)"STAR", 3, nTabID, nDlgID);
	}
	catch (...)
	{
		throw;
	}
	return m_pPageInfo;
}

//
// SECURE Item data
// --------------

IMPLEMENT_SERIAL (CuSECUREItemData, CuCbfListViewItem, 1)
CuSECUREItemData::CuSECUREItemData():CuCbfListViewItem()
{
	// Since active property page will be the leftmost, indicate left subset of property pages
	m_ispecialtype = GEN_FORM_SECURE_SECURE;
	m_bDisplayAllTabs = TRUE;
}

CuSECUREItemData::CuSECUREItemData(BOOL bDisp):CuCbfListViewItem()
{
	if (!bDisp)
		m_ispecialtype = GEN_FORM_SECURE_GCF;
	else
		m_ispecialtype = GEN_FORM_SECURE_SECURE;

	m_bDisplayAllTabs = bDisp;
}

CuCbfListViewItem* CuSECUREItemData::Copy()
{
	ASSERT (FALSE);
	return NULL;
	CuSECUREItemData* pObj = new CuSECUREItemData();
	return (CuCbfListViewItem*)pObj;
}

CuPageInformation* CuSECUREItemData::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	const int nTab = 8;
	const int nTab1 = 2;
	UINT nTabID [nTab] = 
	{
		IDS_TAB_CONFRIGHT_OPTION,
		IDS_TAB_CONFRIGHT_SYSTEM,
		IDS_TAB_CONFRIGHT_MECHANISM,
		IDS_TAB_CONFRIGHT_DERIVED,
		IDS_TAB_CONFRIGHT_AUDITING,
		IDS_TAB_CONFRIGHT_LOGFILE,
		IDS_TAB_CONFRIGHT_AUDITDERIVED,
		IDS_TAB_CONFRIGHT_STARTUP_COUNT
	};
	UINT nDlgID [nTab] = 
	{
		IDD_SECURITY_PAGE_OPTION,
		IDD_SECURITY_PAGE_SYSTEM,
		IDD_SECURITY_PAGE_MECHANISM,
		IDD_SECURITY_PAGE_DERIVED,
		IDD_SECURITY_PAGE_AUDITING,
		IDD_SECURITY_PAGE_LOGFILE,
		IDD_SECURITY_PAGE_AUDITDERIVED,
		IDD_GENERIC_PAGE_STARTUP_COUNT
	};
	UINT nTabID1 [nTab1] = 
	{
		IDS_TAB_CONFRIGHT_SYSTEM,
		IDS_TAB_CONFRIGHT_MECHANISM,
	};
	UINT nDlgID1 [nTab1] = 
	{
		IDD_SECURITY_PAGE_SYSTEM,
		IDD_SECURITY_PAGE_MECHANISM,
	};
	try
	{
		if (m_bDisplayAllTabs)
			m_pPageInfo = new CuPageInformation (_T("SECURE"), nTab, nTabID, nDlgID);
		else
			m_pPageInfo = new CuPageInformation (_T("SECURE"), nTab1, nTabID1, nDlgID1);
	}
	catch (...)
	{
		throw;
	}
	return m_pPageInfo;
}

BOOL CuSECUREItemData::LowLevelActivation()
{
	m_componentInfo.ispecialtype = m_ispecialtype;
	return VCBFllInitGenericPane(&m_componentInfo);
}

BOOL CuSECUREItemData::LowLevelDeactivation()
{
	// use the "current" ispecialtype, which says in which subset of property pages we are
	ASSERT (m_ispecialtype == GEN_FORM_SECURE_SECURE ||
	        m_ispecialtype == GEN_FORM_SECURE_C2 || 
	        m_ispecialtype == GEN_FORM_SECURE_GCF); 

	m_componentInfo.ispecialtype = m_ispecialtype;
	return VCBFllOnMainComponentExit(&m_componentInfo);
}

void CuSECUREItemData::SetSpecialType(int type)
{
	ASSERT (type == GEN_FORM_SECURE_SECURE ||
	        type == GEN_FORM_SECURE_C2 || 
	        type == GEN_FORM_SECURE_GCF); 
	m_ispecialtype = type;
}

//
// LOCK Item data
// --------------

IMPLEMENT_SERIAL (CuLOCKItemData, CuCbfListViewItem, 1)
CuLOCKItemData::CuLOCKItemData():CuCbfListViewItem()
{

}

CuCbfListViewItem* CuLOCKItemData::Copy()
{
	ASSERT (FALSE);
	return NULL;
	CuLOCKItemData* pObj = new CuLOCKItemData();
	return (CuCbfListViewItem*)pObj;
}

CuPageInformation* CuLOCKItemData::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [3] = 
	{
		IDS_TAB_CONFRIGHT_PARAMETERS,
		IDS_TAB_CONFRIGHT_DERIVED,
		IDS_TAB_CONFRIGHT_STARTUP_COUNT
	};
	UINT nDlgID [3] = 
	{
		IDD_LOCKINGSYS_PAGE_PARAMETER,
		IDD_LOCKINGSYS_PAGE_DERIVED,
		IDD_GENERIC_PAGE_STARTUP_COUNT
	};
	try
	{
		m_pPageInfo = new CuPageInformation ((LPCTSTR)"LOCK", 3, nTabID, nDlgID);
	}
	catch (...)
	{
		throw;
	}
	return m_pPageInfo;
}

//
// LOG Item data
// --------------

IMPLEMENT_SERIAL (CuLOGItemData, CuCbfListViewItem, 1)
CuLOGItemData::CuLOGItemData():CuCbfListViewItem()
{

}

CuCbfListViewItem* CuLOGItemData::Copy()
{
	ASSERT (FALSE);
	return NULL;
	CuLOGItemData*	pObj = new CuLOGItemData();
	return (CuCbfListViewItem*)pObj;
}

CuPageInformation* CuLOGItemData::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [3] = 
	{
		IDS_TAB_CONFRIGHT_PARAMETERS,
		IDS_TAB_CONFRIGHT_DERIVED,
		IDS_TAB_CONFRIGHT_STARTUP_COUNT
	};
	UINT nDlgID [3] = 
	{
		IDD_LOGGINGSYS_PAGE_PARAMETER,
		IDD_LOGGINGSYS_PAGE_DERIVED,
		IDD_GENERIC_PAGE_STARTUP_COUNT
	};
	try
	{
		m_pPageInfo = new CuPageInformation ((LPCTSTR)"LOG", 3, nTabID, nDlgID);
	}
	catch (...)
	{
		throw;
	}
	return m_pPageInfo;
}

//
// LOGFILE Item data
// --------------

IMPLEMENT_SERIAL (CuLOGFILEItemData, CuCbfListViewItem, 1)
CuLOGFILEItemData::CuLOGFILEItemData():CuCbfListViewItem()
{
  memset (&m_transactionInfo, 0, sizeof(TRANSACTINFO));
}

CuCbfListViewItem* CuLOGFILEItemData::Copy()
{
	ASSERT (FALSE);
	return NULL;
	CuLOGFILEItemData*	pObj = new CuLOGFILEItemData();
	return (CuCbfListViewItem*)pObj;
}

CuPageInformation* CuLOGFILEItemData::GetPageInformation ()
{
	if (m_pPageInfo)
	{
		if (m_transactionInfo.bStartOnFirstOnInit)
			m_pPageInfo->SetDefaultPage();  // Primary Log File
		else
			m_pPageInfo->SetDefaultPage(1); // Secondary Log File
		return m_pPageInfo;
	}
	UINT nTabID [3] = 
	{
		IDS_TAB_CONFRIGHT_PRIMARYLOG,
		IDS_TAB_CONFRIGHT_SECONDARYLOG,
		IDS_TAB_CONFRIGHT_STARTUP_COUNT
	};
	UINT nDlgID [3] = 
	{
		IDD_LOGFILE_PAGE_CONFIGURE,
		IDD_LOGFILE_PAGE_SECONDARY,
		IDD_GENERIC_PAGE_STARTUP_COUNT
	};
	try
	{
		m_pPageInfo = new CuPageInformation ((LPCTSTR)"LOGFILE", 3, nTabID, nDlgID);
		if (m_transactionInfo.bStartOnFirstOnInit)
			m_pPageInfo->SetDefaultPage();  // Primary Log File
		else
			m_pPageInfo->SetDefaultPage(1); // Secondary Log File
	}
	catch (...)
	{
		throw;
	}
	return m_pPageInfo;
}

BOOL CuLOGFILEItemData::LowLevelActivation()
{
	return VCBFll_tx_init(&m_componentInfo, &m_transactionInfo);
}

BOOL CuLOGFILEItemData::LowLevelDeactivation()
{
  CConfLeftDlg *pLeftDlg = m_pLeftDlg;
  ASSERT (m_pLeftDlg);
  LPCOMPONENTINFO lpLog1=NULL,lpLog2=NULL;
  CuCbfListViewItem*  pOtherData;
  int nCount = pLeftDlg->m_clistctrl.GetItemCount();
  int i = 0;
  for (i=0; i<nCount; i++){
    pOtherData = (CuCbfListViewItem*)pLeftDlg->m_clistctrl.GetItemData (i);
    ASSERT (pOtherData);
    if (pOtherData->m_componentInfo.blog1)
        lpLog1=&(pOtherData->m_componentInfo);
    if (pOtherData->m_componentInfo.blog2)
        lpLog2=&(pOtherData->m_componentInfo);
  }
  ASSERT (lpLog1);
  ASSERT (lpLog2);
  if (!lpLog1 || !lpLog2)
        return FALSE;
  BOOL bRet = VCBFll_tx_OnExit(&m_transactionInfo, lpLog1, lpLog2);

  // Update the 2 lines counts in left pane
  for (i=0; i<nCount; i++){
    pOtherData = (CuCbfListViewItem*)pLeftDlg->m_clistctrl.GetItemData (i);
    ASSERT (pOtherData);
    if (pOtherData->m_componentInfo.itype == COMP_TYPE_TRANSACTION_LOG)
    	pLeftDlg->m_clistctrl.SetItemText (i, 2, (LPCTSTR)pOtherData->m_componentInfo.szcount);
  }

  return bRet;
}

//
// RECOVER Item data
// --------------

IMPLEMENT_SERIAL (CuRECOVERItemData, CuCbfListViewItem, 1)
CuRECOVERItemData::CuRECOVERItemData():CuCbfListViewItem()
{

}

CuCbfListViewItem* CuRECOVERItemData::Copy()
{
	ASSERT (FALSE);
	return NULL;
	CuRECOVERItemData*	pObj = new CuRECOVERItemData();
	return (CuCbfListViewItem*)pObj;
}

CuPageInformation* CuRECOVERItemData::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;

	UINT nTabID [3] = 
	{
		IDS_TAB_CONFRIGHT_PARAMETERS,
		IDS_TAB_CONFRIGHT_DERIVED,
		IDS_TAB_CONFRIGHT_STARTUP_COUNT
	};
	UINT nDlgID [3] = 
	{
		IDD_RECOVERY_PAGE_PARAMETER,
		IDD_RECOVERY_PAGE_DERIVED,
		IDD_GENERIC_PAGE_STARTUP_COUNT
	};

        try
	{
		m_pPageInfo = new CuPageInformation ((LPCTSTR)"RECOVER", 3, nTabID, nDlgID);
	}
	catch (...)
	{
		throw;
	}

	return m_pPageInfo;
}

//
// ARCHIVE Item data
// --------------

IMPLEMENT_SERIAL (CuARCHIVEItemData, CuCbfListViewItem, 1)
CuARCHIVEItemData::CuARCHIVEItemData():CuCbfListViewItem()
{

}

CuCbfListViewItem* CuARCHIVEItemData::Copy()
{
	ASSERT (FALSE);
	return NULL;
	CuARCHIVEItemData*	pObj = new CuARCHIVEItemData();
	return (CuCbfListViewItem*)pObj;
}

CuPageInformation* CuARCHIVEItemData::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	try
	{
		m_pPageInfo = new CuPageInformation ((LPCTSTR)"ARCHIVE", 0, NULL, NULL);
	}
	catch (...)
	{
		throw;
	}
	return m_pPageInfo;
}

//
// RMCMD Item data
// --------------

IMPLEMENT_SERIAL (CuRMCMDItemData, CuCbfListViewItem, 1)
CuRMCMDItemData::CuRMCMDItemData():CuCbfListViewItem()
{

}

CuCbfListViewItem* CuRMCMDItemData::Copy()
{
	ASSERT (FALSE);
	return NULL;
	CuRMCMDItemData*	pObj = new CuRMCMDItemData();
	return (CuCbfListViewItem*)pObj;
}

CuPageInformation* CuRMCMDItemData::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	try
	{
		m_pPageInfo = new CuPageInformation ((LPCTSTR)"RMCMD", 0, NULL, NULL);
	}
	catch (...)
	{
		throw;
	}
	return m_pPageInfo;
}

//
// ORACLE Item data
// --------------

IMPLEMENT_SERIAL (CuGW_ORACLEItemData, CuCbfListViewItem, 1)
CuGW_ORACLEItemData::CuGW_ORACLEItemData():CuCbfListViewItem()
{
}

CuCbfListViewItem* CuGW_ORACLEItemData::Copy()
{
	ASSERT (FALSE);
	return NULL;
	CuGW_ORACLEItemData*	pObj = new CuGW_ORACLEItemData();
	return (CuCbfListViewItem*)pObj;
}

CuPageInformation* CuGW_ORACLEItemData::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;

	UINT nTabID [2] = 
	{
		IDS_TAB_CONFRIGHT_PARAMETERS,
		IDS_TAB_CONFRIGHT_STARTUP_COUNT
	};
	UINT nDlgID [2] = 
	{
		IDD_GW_ORACLE_PAGE_PARAMETER,
		IDD_GENERIC_PAGE_STARTUP_COUNT
	};

    try
	{
		m_pPageInfo = new CuPageInformation ((LPCTSTR)"GW_GENERIC", 2, nTabID, nDlgID);
	}
	catch (...)
	{
		throw;
	}

	return m_pPageInfo;
}

//
// DB2 UDB Item data
// --------------

IMPLEMENT_SERIAL (CuGW_DB2UDBItemData, CuCbfListViewItem, 1)
CuGW_DB2UDBItemData::CuGW_DB2UDBItemData():CuCbfListViewItem()
{
}

CuCbfListViewItem* CuGW_DB2UDBItemData::Copy()
{
	ASSERT (FALSE);
	return NULL;
	CuGW_DB2UDBItemData*	pObj = new CuGW_DB2UDBItemData();
	return (CuCbfListViewItem*)pObj;
}

CuPageInformation* CuGW_DB2UDBItemData::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;

	UINT nTabID [2] = 
	{
		IDS_TAB_CONFRIGHT_PARAMETERS,
		IDS_TAB_CONFRIGHT_STARTUP_COUNT
	};
	UINT nDlgID [2] = 
	{
		IDD_GW_DB2UDB_PAGE_PARAMETER,
		IDD_GENERIC_PAGE_STARTUP_COUNT
	};

    try
	{
		m_pPageInfo = new CuPageInformation ((LPCTSTR)"GW_GENERIC", 2, nTabID, nDlgID);
	}
	catch (...)
	{
		throw;
	}

	return m_pPageInfo;
}


//
// INFORMIX Item data
// --------------

IMPLEMENT_SERIAL (CuGW_INFORMIXItemData, CuCbfListViewItem, 1)
CuGW_INFORMIXItemData::CuGW_INFORMIXItemData():CuCbfListViewItem()
{
}

CuCbfListViewItem* CuGW_INFORMIXItemData::Copy()
{
	ASSERT (FALSE);
	return NULL;
	CuGW_INFORMIXItemData*	pObj = new CuGW_INFORMIXItemData();
	return (CuCbfListViewItem*)pObj;
}

CuPageInformation* CuGW_INFORMIXItemData::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;

	UINT nTabID [2] = 
	{
		IDS_TAB_CONFRIGHT_PARAMETERS,
		IDS_TAB_CONFRIGHT_STARTUP_COUNT
	};
	UINT nDlgID [2] = 
	{
		IDD_GW_INFORMIX_PAGE_PARAMETER,
		IDD_GENERIC_PAGE_STARTUP_COUNT
	};

    try
	{
		m_pPageInfo = new CuPageInformation ((LPCTSTR)"GW_INFORMIX", 2, nTabID, nDlgID);
	}
	catch (...)
	{
		throw;
	}

	return m_pPageInfo;
}

//
// SYBASE Item data
// --------------

IMPLEMENT_SERIAL (CuGW_SYBASEItemData, CuCbfListViewItem, 1)
CuGW_SYBASEItemData::CuGW_SYBASEItemData():CuCbfListViewItem()
{
}

CuCbfListViewItem* CuGW_SYBASEItemData::Copy()
{
	ASSERT (FALSE);
	return NULL;
	CuGW_SYBASEItemData*	pObj = new CuGW_SYBASEItemData();
	return (CuCbfListViewItem*)pObj;
}

CuPageInformation* CuGW_SYBASEItemData::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;

	UINT nTabID [2] = 
	{
		IDS_TAB_CONFRIGHT_PARAMETERS,
		IDS_TAB_CONFRIGHT_STARTUP_COUNT
	};
	UINT nDlgID [2] = 
	{
		IDD_GW_SYBASE_PAGE_PARAMETER,
		IDD_GENERIC_PAGE_STARTUP_COUNT
	};

    try
	{
		m_pPageInfo = new CuPageInformation ((LPCTSTR)"GW_SYBASE", 2, nTabID, nDlgID);
	}
	catch (...)
	{
		throw;
	}

	return m_pPageInfo;
}

//
// MSSQL Item data
// --------------

IMPLEMENT_SERIAL (CuGW_MSSQLItemData, CuCbfListViewItem, 1)
CuGW_MSSQLItemData::CuGW_MSSQLItemData():CuCbfListViewItem()
{
}

CuCbfListViewItem* CuGW_MSSQLItemData::Copy()
{
	ASSERT (FALSE);
	return NULL;
	CuGW_MSSQLItemData*	pObj = new CuGW_MSSQLItemData();
	return (CuCbfListViewItem*)pObj;
}

CuPageInformation* CuGW_MSSQLItemData::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;

	UINT nTabID [2] = 
	{
		IDS_TAB_CONFRIGHT_PARAMETERS,
		IDS_TAB_CONFRIGHT_STARTUP_COUNT
	};
	UINT nDlgID [2] = 
	{
		IDD_GW_MSSQL_PAGE_PARAMETER,
		IDD_GENERIC_PAGE_STARTUP_COUNT
	};

    try
	{
		m_pPageInfo = new CuPageInformation ((LPCTSTR)"GW_MSSQL", 2, nTabID, nDlgID);
	}
	catch (...)
	{
		throw;
	}

	return m_pPageInfo;
}

//
// ODBC Item data
// --------------

IMPLEMENT_SERIAL (CuGW_ODBCItemData, CuCbfListViewItem, 1)
CuGW_ODBCItemData::CuGW_ODBCItemData():CuCbfListViewItem()
{
}

CuCbfListViewItem* CuGW_ODBCItemData::Copy()
{
	ASSERT (FALSE);
	return NULL;
	CuGW_ODBCItemData*	pObj = new CuGW_ODBCItemData();
	return (CuCbfListViewItem*)pObj;
}

CuPageInformation* CuGW_ODBCItemData::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;

	UINT nTabID [2] = 
	{
		IDS_TAB_CONFRIGHT_PARAMETERS,
		IDS_TAB_CONFRIGHT_STARTUP_COUNT
	};
	UINT nDlgID [2] = 
	{
		IDD_GW_ODBC_PAGE_PARAMETER,
		IDD_GENERIC_PAGE_STARTUP_COUNT
	};

    try
	{
		m_pPageInfo = new CuPageInformation ((LPCTSTR)"GW_ODBC", 2, nTabID, nDlgID);
	}
	catch (...)
	{
		throw;
	}

	return m_pPageInfo;
}
