/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : compcat.cpp , Implementation File 
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Manipulation Ingres Visual Manager Data
**
** History:
**
** 13-Jan-1999: (uk$so01) 
**    created
** 06-Jun-2000: (uk$so01) 
**    (BUG #99242)
**    Cleanup code for DBCS compliant
** 18-Jan-2001 (uk$so01)
**    SIR #103727 (Make IVM support JDBC Server)
**    Add the tree item data 'CaItemDataJdbc'
** 14-Aug-2001 (uk$so01)
**    SIR #105383., Remove meber CaItemDataRecovery::MakeActiveTab() and
**    CaItemDataArchiver::MakeActiveTab()
** 20-Mar-2002 (noifr01)
**    (buf 107366) the icon used for transaction log branches was not the
**    right one
** 03-Oct-2002 (noifr01)
**    (SIR 107587) have IVM manage DB2UDB gateway (mapping the GUI to
**    oracle gateway code until this is moved to generic management)
** 09-Mar-2004 (uk$so01)
**    SIR #110013 (Handle the DAS Server)
** 10-Nov-2004 (noifr01)
**    (bug 113412) replaced usage of hardcoded strings (such as "(default)"
**    for a configuration name), with that of data from the message file
** 25-Jul-2005 (drivi01)
**	  For Vista, update code to use winstart to start/stop servers
**	  whenever start/stop menu is activated.  IVM will run as a user
**	  application and will elevate privileges to start/stop by
**	  calling ShellExecuteEx function, since ShellExecuteEx can not
**	  subscribe to standard input/output, we must use winstart to
**	  stop/start ingres.
**/


#include "stdafx.h"
#include "ivm.h"
#include "ivmdisp.h"
#include "compdata.h"
#include "compcat.h"
#include "ivmdml.h"
#include "ll.h"
#include <compat.h>
#include <gvcl.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// NAME Item data
// --------------
CaItemDataName::CaItemDataName():CaTreeComponentItemData()
{
	m_nOrder = ORDER_NAME;
	m_treeCtrlData.SetImage (IM_NAME_R_NORMAL, IM_NAME_R_NORMAL);
	if (GVvista())
	{
	m_strStart = _T("winstart /start /param=\"-iigcn\"");
	m_strStop = _T("winstart /stop /param=\"-iigcn\"");
	}
	else
	{
	m_strStart = _T("ingstart -iigcn");
	m_strStop = _T("ingstop -iigcn");
	}
}

CaPageInformation* CaItemDataName::GetPageInformation ()
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
		IDD_STATUS_SINGLEINSTANCE_COMPONENT,
		IDD_LOGEVENT_GENERIC,
		IDD_MESSAGE_STATISTIC
	};

	m_pPageInfo = new CaPageInformation (_T("NAME"), 3, nTabID, nDlgID);
	CaEventFilter& filter = m_pPageInfo->GetEventFilter();
	//
	// Category and Name are not set for this item.
	filter.SetFilterFlag (FILER_DATE_BEGIN|FILER_DATE_END|FILER_INSTANCE);
	return m_pPageInfo;
}


void CaItemDataName::UpdateIcon(CTreeCtrl* pTree)
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
		nImageIcon = bAlert? IM_NAME_N_NORMAL: IM_NAME_R_NORMAL;
	}
	else
	if (nState == COMP_STOP || nState == (COMP_NORMAL|COMP_STOP))
	{
		//
		// STOPPED:
		nImageIcon = bAlert? IM_NAME_N_STOPPED: IM_NAME_R_STOPPED;
	}
	else
	if (nState == COMP_START || nState == (COMP_NORMAL|COMP_START))
	{
		//
		// STARTED:
		nImageIcon = bAlert? IM_NAME_N_STARTED: IM_NAME_R_STARTED;
	}
	else
	if (nState&COMP_STARTMORE && !(nState&COMP_STOP) && !(nState&COMP_HALFSTART))
	{
		//
		// STARTED MORE:
		nImageIcon = bAlert? IM_NAME_N_STARTEDMORE: IM_NAME_R_STARTEDMORE;
	}
	else
	{
		//
		// HALF STARTED:
		nImageIcon = bAlert? IM_NAME_N_HALFSTARTED: IM_NAME_R_HALFSTARTED;
	}
	m_treeCtrlData.SetImage (nImageIcon, nImageIcon);
	pTree->SetItemImage (m_treeCtrlData.GetTreeItem(), nImageIcon, nImageIcon);
}


//
// DBMS Item data
// --------------
CaItemDataDbms::CaItemDataDbms():CaTreeComponentFolderItemData()
{
	m_nOrder = ORDER_DBMS;
	m_treeCtrlData.SetImage (IM_DBMS_R_NORMAL, IM_DBMS_R_NORMAL);
	if (GVvista())
	{
	m_strStart = _T("winstart /start /param=\"-iidbms\"");
	m_strStop = _T("winstart /stop /param=\"-iidbms\"");
	}
	else
	{
	m_strStart = _T("ingstart -iidbms");
	m_strStop = _T("ingstop -iidbms");
	}
}

CString CaItemDataDbms::GetStartStopString(BOOL bStart)
{
	CString strProcess = _T("");
	if (bStart)
	{
		//
		// If component's name = (default) then run <ingstart -iidbms>
		// else run <ingstart -iidbms=<component's name>>
		strProcess = m_strStart;
		if (m_strComponentName.Find (GetLocDefConfName()) == -1)
		{
			if (GVvista())
			{
				strProcess = _T("winstart /start /param=\"iidbms=");
				strProcess += m_strComponentName;
				strProcess += _T("\"");
			}
			else
			{
			strProcess  = _T("ingstart -iidbms=");
			strProcess += m_strComponentName;
			}
		}
	}
	else
	{
		//
		// If component's name = (default) then run <ingstop -iidbms>
		// else run <ingstop -iidbms=<component's name>>
		strProcess = m_strStop;
		if (m_strComponentName.Find (GetLocDefConfName()) == -1)
		{
			if (GVvista())
			{
				strProcess = _T("winstart /stop /param=\"-iidbms=");
				strProcess += m_strComponentName;
				strProcess += _T("\"");
			}
			else
			{
			strProcess  = _T("ingstop -iidbms=");
			strProcess += m_strComponentName;
			}
		}
	}
	return strProcess;
}

CaPageInformation* CaItemDataDbms::GetPageInformation ()
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
	
	m_pPageInfo = new CaPageInformation (_T("DBMS"), 3, nTabID, nDlgID);
	CaEventFilter& filter = m_pPageInfo->GetEventFilter();
	//
	// Category and Name are not set for this item.
	filter.SetFilterFlag (FILER_DATE_BEGIN|FILER_DATE_END|FILER_INSTANCE);
	return m_pPageInfo;
}

void CaItemDataDbms::UpdateIcon(CTreeCtrl* pTree)
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
		nImageIcon = bAlert? IM_DBMS_N_NORMAL: IM_DBMS_R_NORMAL;
	}
	else
	if (nState == COMP_STOP || nState == (COMP_NORMAL|COMP_STOP))
	{
		//
		// STOPPED:
		nImageIcon = bAlert? IM_DBMS_N_STOPPED: IM_DBMS_R_STOPPED;
	}
	else
	if (nState == COMP_START || nState == (COMP_NORMAL|COMP_START))
	{
		//
		// STARTED:
		nImageIcon = bAlert? IM_DBMS_N_STARTED: IM_DBMS_R_STARTED;
	}
	else
	if (nState&COMP_STARTMORE && !(nState&COMP_STOP) && !(nState&COMP_HALFSTART))
	{
		//
		// STARTED MORE:
		nImageIcon = bAlert? IM_DBMS_N_STARTEDMORE: IM_DBMS_R_STARTEDMORE;
	}
	else
	{
		//
		// HALF STARTED:
		nImageIcon = bAlert? IM_DBMS_N_HALFSTARTED: IM_DBMS_R_HALFSTARTED;
	}
	pTree->SetItemImage (m_treeCtrlData.GetTreeItem(), nImageIcon, nImageIcon);
}



//
// ICE Item data
// --------------
CaItemDataIce::CaItemDataIce():CaTreeComponentItemData()
{
	m_nOrder = ORDER_ICE;
	m_treeCtrlData.SetImage (IM_ICE_R_NORMAL, IM_ICE_R_NORMAL);
	if (GVvista())
	{
		m_strStart = _T("winstart /start /param=\"-icesvr\"");
		m_strStop = _T("winstart /stop /param=\"-icesvr\"");
	}
	else
	{
	m_strStart = _T("ingstart -icesvr");
	m_strStop = _T("ingstop -icesvr");
	}
}



CaPageInformation* CaItemDataIce::GetPageInformation ()
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

	m_pPageInfo = new CaPageInformation (_T("ICE"), 3, nTabID, nDlgID);
	CaEventFilter& filter = m_pPageInfo->GetEventFilter();
	//
	// Category and Name are not set for this item.
	filter.SetFilterFlag (FILER_DATE_BEGIN|FILER_DATE_END|FILER_INSTANCE);
	return m_pPageInfo;
}


void CaItemDataIce::UpdateIcon(CTreeCtrl* pTree)
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
		nImageIcon = bAlert? IM_ICE_N_NORMAL: IM_ICE_R_NORMAL;
	}
	else
	if (nState == COMP_STOP || nState == (COMP_NORMAL|COMP_STOP))
	{
		//
		// STOPPED:
		nImageIcon = bAlert? IM_ICE_N_STOPPED: IM_ICE_R_STOPPED;
	}
	else
	if (nState == COMP_START || nState == (COMP_NORMAL|COMP_START))
	{
		//
		// STARTED:
		nImageIcon = bAlert? IM_ICE_N_STARTED: IM_ICE_R_STARTED;
	}
	else
	if (nState&COMP_STARTMORE && !(nState&COMP_STOP) && !(nState&COMP_HALFSTART))
	{
		//
		// STARTED MORE:
		nImageIcon = bAlert? IM_ICE_N_STARTEDMORE: IM_ICE_R_STARTEDMORE;
	}
	else
	{
		//
		// HALF STARTED:
		nImageIcon = bAlert? IM_ICE_N_HALFSTARTED: IM_ICE_R_HALFSTARTED;
	}
	pTree->SetItemImage (m_treeCtrlData.GetTreeItem(), nImageIcon, nImageIcon);
}



//
// NET Item data
// --------------
CaItemDataNet::CaItemDataNet():CaTreeComponentFolderItemData()
{
	m_nOrder = ORDER_NET;
	m_treeCtrlData.SetImage (IM_NET_R_NORMAL, IM_NET_R_NORMAL);
	if (GVvista())
	{
		m_strStart = _T("winstart /start /param=\"-iigcc\"");
		m_strStop = _T("winstart /stop /param=\"-iigcc\"");
	}
	else
	{
	m_strStart = _T("ingstart -iigcc");
	m_strStop = _T("ingstop -iigcc");
	}
}

CString CaItemDataNet::GetStartStopString(BOOL bStart)
{
	CString strProcess = _T("");
	if (bStart)
	{
		//
		// If component's name = (default) then run <ingstart -iigcc>
		// else run <ingstart -iigcc=<component's name>>
		strProcess = m_strStart;
		if (m_strComponentName.Find (GetLocDefConfName()) == -1)
		{
			if(GVvista())
			{
				strProcess  = _T("winstart /start /param=\"-iigcc=");
				strProcess += m_strComponentName;
				strProcess += _T("\"");
			}
			else
			{
			strProcess  = _T("ingstart -iigcc=");
			strProcess += m_strComponentName;
			}
		}
	}
	else
	{
		//
		// If component's name = (default) then run <ingstop -iigcc>
		// else run <ingstart -iigcc=<component's name>>
		strProcess = m_strStop;
		if (m_strComponentName.Find (GetLocDefConfName()) == -1)
		{
			if (GVvista())
			{
				strProcess = _T("winstart /stop /param=\"-iigcc=");
				strProcess += m_strComponentName;
				strProcess += _T("\"");
			}
			else
			{
			strProcess  = _T("ingstop -iigcc=");
			strProcess += m_strComponentName;
			}
		}
	}
	return strProcess;
}

CaPageInformation* CaItemDataNet::GetPageInformation ()
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

	m_pPageInfo = new CaPageInformation (_T("NET"), 3, nTabID, nDlgID);
	CaEventFilter& filter = m_pPageInfo->GetEventFilter();
	//
	// Category and Name are not set for this item.
	filter.SetFilterFlag (FILER_DATE_BEGIN|FILER_DATE_END|FILER_INSTANCE);
	return m_pPageInfo;
}

void CaItemDataNet::UpdateIcon(CTreeCtrl* pTree)
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
		nImageIcon = bAlert? IM_NET_N_NORMAL: IM_NET_R_NORMAL;
	}
	else
	if (nState == COMP_STOP || nState == (COMP_NORMAL|COMP_STOP))
	{
		//
		// STOPPED:
		nImageIcon = bAlert? IM_NET_N_STOPPED: IM_NET_R_STOPPED;
	}
	else
	if (nState == COMP_START || nState == (COMP_NORMAL|COMP_START))
	{
		//
		// STARTED:
		nImageIcon = bAlert? IM_NET_N_STARTED: IM_NET_R_STARTED;
	}
	else
	if (nState&COMP_STARTMORE && !(nState&COMP_STOP) && !(nState&COMP_HALFSTART))
	{
		//
		// STARTED MORE:
		nImageIcon = bAlert? IM_NET_N_STARTEDMORE: IM_NET_R_STARTEDMORE;
	}
	else
	{
		//
		// HALF STARTED:
		nImageIcon = bAlert? IM_NET_N_HALFSTARTED: IM_NET_R_HALFSTARTED;
	}
	pTree->SetItemImage (m_treeCtrlData.GetTreeItem(), nImageIcon, nImageIcon);
}


//
// JDBC Item data
// --------------
CaItemDataJdbc::CaItemDataJdbc():CaTreeComponentFolderItemData()
{
	m_nOrder = ORDER_JDBC;
	m_treeCtrlData.SetImage (IM_JDBC_R_NORMAL, IM_JDBC_R_NORMAL);
	if (GVvista())
	{
		m_strStart = _T("winstart /start /param=\"-iijdbc\"");
		m_strStop = _T("winstart /stop /param=\"-iijdbc\"");
	}
	else
	{
	m_strStart = _T("ingstart -iijdbc");
	m_strStop = _T("ingstop -iijdbc");
	}
}

CString CaItemDataJdbc::GetStartStopString(BOOL bStart)
{
	CString strProcess = _T("");
	if (bStart)
	{
		//
		// If component's name = (default) then run <ingstart -iijdbc>
		// else run <ingstart -iijdbc=<component's name>>
		strProcess = m_strStart;
		if (m_strComponentName.Find (GetLocDefConfName()) == -1)
		{
			if (GVvista())
			{
			strProcess  = _T("winstart /start /param=\"-iijdbc=");
			strProcess += m_strComponentName;
			strProcess += _T("\"");
			}
			else
			{
			strProcess  = _T("ingstart -iijdbc=");
			strProcess += m_strComponentName;
			}
		}
	}
	else
	{
		//
		// If component's name = (default) then run <ingstop -iijdbc>
		// else run <ingstart -iijdbc=<component's name>>
		strProcess = m_strStop;
		if (m_strComponentName.Find (GetLocDefConfName()) == -1)
		{
			if (GVvista())
			{
				strProcess  = _T("winstart /stop /param=\"-iijdbc=");
				strProcess += m_strComponentName;
				strProcess += _T("\"");
			}
			else
			{
			strProcess  = _T("ingstop -iijdbc=");
			strProcess += m_strComponentName;
			}
		}
	}
	return strProcess;
}

CaPageInformation* CaItemDataJdbc::GetPageInformation ()
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

	m_pPageInfo = new CaPageInformation (_T("JDBC"), 3, nTabID, nDlgID);
	CaEventFilter& filter = m_pPageInfo->GetEventFilter();
	//
	// Category and Name are not set for this item.
	filter.SetFilterFlag (FILER_DATE_BEGIN|FILER_DATE_END|FILER_INSTANCE);
	return m_pPageInfo;
}

void CaItemDataJdbc::UpdateIcon(CTreeCtrl* pTree)
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
		nImageIcon = bAlert? IM_JDBC_N_NORMAL: IM_JDBC_R_NORMAL;
	}
	else
	if (nState == COMP_STOP || nState == (COMP_NORMAL|COMP_STOP))
	{
		//
		// STOPPED:
		nImageIcon = bAlert? IM_JDBC_N_STOPPED: IM_JDBC_R_STOPPED;
	}
	else
	if (nState == COMP_START || nState == (COMP_NORMAL|COMP_START))
	{
		//
		// STARTED:
		nImageIcon = bAlert? IM_JDBC_N_STARTED: IM_JDBC_R_STARTED;
	}
	else
	if (nState&COMP_STARTMORE && !(nState&COMP_STOP) && !(nState&COMP_HALFSTART))
	{
		//
		// STARTED MORE:
		nImageIcon = bAlert? IM_JDBC_N_STARTEDMORE: IM_JDBC_R_STARTEDMORE;
	}
	else
	{
		//
		// HALF STARTED:
		nImageIcon = bAlert? IM_JDBC_N_HALFSTARTED: IM_JDBC_R_HALFSTARTED;
	}
	pTree->SetItemImage (m_treeCtrlData.GetTreeItem(), nImageIcon, nImageIcon);
}


//
// DASVR Item data
// --------------
CaItemDataDasvr::CaItemDataDasvr():CaTreeComponentFolderItemData()
{
	m_nOrder = ORDER_DAS;
	m_treeCtrlData.SetImage (IM_DASVR_R_NORMAL, IM_DASVR_R_NORMAL);
	if (GVvista())
	{
		m_strStart = _T("winstart /start /param=\"-iigcd\"");
		m_strStop = _T("winstart /stop /param=\"-iigcd\"");
	}
	else
	{
	m_strStart = _T("ingstart -iigcd");
	m_strStop = _T("ingstop -iigcd");
	}
}

CString CaItemDataDasvr::GetStartStopString(BOOL bStart)
{
	CString strProcess = _T("");
	if (bStart)
	{
		//
		// If component's name = (default) then run <ingstart -iigcd>
		// else run <ingstart -iigcd=<component's name>>
		strProcess = m_strStart;
		if (m_strComponentName.Find (GetLocDefConfName()) == -1)
		{
			if (GVvista())
			{
			strProcess  = _T("winstart /start /param=\"-iigcd=");
			strProcess += m_strComponentName;
			strProcess += _T("\"");
			}
			else
			{
			strProcess  = _T("ingstart -iigcd=");
			strProcess += m_strComponentName;
			}
		}
	}
	else
	{
		//
		// If component's name = (default) then run <ingstop -iigcd>
		// else run <ingstart -iigcd=<component's name>>
		strProcess = m_strStop;
		if (m_strComponentName.Find (GetLocDefConfName()) == -1)
		{
			if (GVvista())
			{
			strProcess  = _T("winstart /stop /param=\"-iigcd=");
			strProcess += m_strComponentName;
			strProcess += _T("\"");
			}
			else
			{
			strProcess  = _T("ingstop -iigcd=");
			strProcess += m_strComponentName;
			}
		}
	}
	return strProcess;
}

CaPageInformation* CaItemDataDasvr::GetPageInformation ()
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

	m_pPageInfo = new CaPageInformation (_T("DASVR"), 3, nTabID, nDlgID);
	CaEventFilter& filter = m_pPageInfo->GetEventFilter();
	//
	// Category and Name are not set for this item.
	filter.SetFilterFlag (FILER_DATE_BEGIN|FILER_DATE_END|FILER_INSTANCE);
	return m_pPageInfo;
}

void CaItemDataDasvr::UpdateIcon(CTreeCtrl* pTree)
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
		nImageIcon = bAlert? IM_DASVR_N_NORMAL: IM_DASVR_R_NORMAL;
	}
	else
	if (nState == COMP_STOP || nState == (COMP_NORMAL|COMP_STOP))
	{
		//
		// STOPPED:
		nImageIcon = bAlert? IM_DASVR_N_STOPPED: IM_DASVR_R_STOPPED;
	}
	else
	if (nState == COMP_START || nState == (COMP_NORMAL|COMP_START))
	{
		//
		// STARTED:
		nImageIcon = bAlert? IM_DASVR_N_STARTED: IM_DASVR_R_STARTED;
	}
	else
	if (nState&COMP_STARTMORE && !(nState&COMP_STOP) && !(nState&COMP_HALFSTART))
	{
		//
		// STARTED MORE:
		nImageIcon = bAlert? IM_DASVR_N_STARTEDMORE: IM_DASVR_R_STARTEDMORE;
	}
	else
	{
		//
		// HALF STARTED:
		nImageIcon = bAlert? IM_DASVR_N_HALFSTARTED: IM_DASVR_R_HALFSTARTED;
	}
	pTree->SetItemImage (m_treeCtrlData.GetTreeItem(), nImageIcon, nImageIcon);
}


//
// BRIDGE Item data
// --------------
CaItemDataBridge::CaItemDataBridge():CaTreeComponentFolderItemData()
{
	m_nOrder = ORDER_BRIDGE;
	m_treeCtrlData.SetImage (IM_BRIDGE_R_NORMAL, IM_BRIDGE_R_NORMAL);
	if (GVvista())
	{
	m_strStart = _T("winstart /start /param=\"-iigcb\"");
	m_strStop = _T("winstart /stop /param=\"-iigcb\"");
	}
	else
	{
	m_strStart = _T("ingstart -iigcb");
	m_strStop = _T("ingstop -iigcb");
	}
}

CString CaItemDataBridge::GetStartStopString(BOOL bStart)
{
	CString strProcess = _T("");
	if (bStart)
	{
		//
		// If component's name = (default) then run <ingstart -iigcb>
		// else run <ingstart -iigcb=<component's name>>
		strProcess = m_strStart;
		if (m_strComponentName.Find (GetLocDefConfName()) == -1)
		{
			if (GVvista())
			{
				strProcess  = _T("winstart /start /param=\"-iigcb=");
				strProcess += m_strComponentName;
				strProcess += _T("\"");
			}
			else
			{
			strProcess  = _T("ingstart -iigcb=");
			strProcess += m_strComponentName;
			}
		}
	}
	else
	{
		//
		// If component's name = (default) then run <ingstop -iigcb>
		// else run <ingstop -iigcb=<component's name>>
		strProcess = m_strStop;
		if (m_strComponentName.Find (GetLocDefConfName()) == -1)
		{
			if (GVvista())
			{
				strProcess  = _T("winstart /stop /param=\"-iigcb=\"");
				strProcess += m_strComponentName;
				strProcess += _T("\"");
			}
			else
			{
			strProcess  = _T("ingstop -iigcb=");
			strProcess += m_strComponentName;
			}
		}
	}
	return strProcess;
}


CaPageInformation* CaItemDataBridge::GetPageInformation ()
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
		
	m_pPageInfo = new CaPageInformation (_T("BRIDGE"), 3, nTabID, nDlgID);
	CaEventFilter& filter = m_pPageInfo->GetEventFilter();
	//
	// Category and Name are not set for this item.
	filter.SetFilterFlag (FILER_DATE_BEGIN|FILER_DATE_END|FILER_INSTANCE);
	return m_pPageInfo;
}


void CaItemDataBridge::UpdateIcon(CTreeCtrl* pTree)
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
		nImageIcon = bAlert? IM_BRIDGE_N_NORMAL: IM_BRIDGE_R_NORMAL;
	}
	else
	if (nState == COMP_STOP || nState == (COMP_NORMAL|COMP_STOP))
	{
		//
		// STOPPED:
		nImageIcon = bAlert? IM_BRIDGE_N_STOPPED: IM_BRIDGE_R_STOPPED;
	}
	else
	if (nState == COMP_START || nState == (COMP_NORMAL|COMP_START))
	{
		//
		// STARTED:
		nImageIcon = bAlert? IM_BRIDGE_N_STARTED: IM_BRIDGE_R_STARTED;
	}
	else
	if (nState&COMP_STARTMORE && !(nState&COMP_STOP) && !(nState&COMP_HALFSTART))
	{
		//
		// STARTED MORE:
		nImageIcon = bAlert? IM_BRIDGE_N_STARTEDMORE: IM_BRIDGE_R_STARTEDMORE;
	}
	else
	{
		//
		// HALF STARTED:
		nImageIcon = bAlert? IM_BRIDGE_N_HALFSTARTED: IM_BRIDGE_R_HALFSTARTED;
	}
	pTree->SetItemImage (m_treeCtrlData.GetTreeItem(), nImageIcon, nImageIcon);
}


//
// STAR Item data
// --------------
CaItemDataStar::CaItemDataStar():CaTreeComponentFolderItemData()
{
	m_nOrder = ORDER_STAR;
	m_treeCtrlData.SetImage (IM_STAR_R_NORMAL, IM_STAR_R_NORMAL);
	if (GVvista())
	{
		m_strStart = _T("winstart /start /param=\"-iistar\"");
		m_strStop = _T("winstart /stop /param=\"-iistar\"");
	}
	else
	{
	m_strStart = _T("ingstart -iistar");
	m_strStop = _T("ingstop -iistar");
	}
}

CString CaItemDataStar::GetStartStopString(BOOL bStart)
{
	CString strProcess = _T("");
	if (bStart)
	{
		//
		// If component's name = (default) then run <ingstart -iistar>
		// else run <ingstart -iistar=<component's name>>
		strProcess = m_strStart;
		if (m_strComponentName.Find (GetLocDefConfName()) == -1)
		{
			if (GVvista())
			{
				strProcess  = _T("winstart /start /param=\"-iistar=");
				strProcess += m_strComponentName;
				strProcess += _T("\"");
			}
			else
			{
			strProcess  = _T("ingstart -iistar=");
			strProcess += m_strComponentName;
			}
		}
	}
	else
	{
		//
		// If component's name = (default) then run <ingstop -iidbms>
		// else run <ingstop -iistar=<component's name>>
		strProcess = m_strStop;
		if (m_strComponentName.Find (GetLocDefConfName()) == -1)
		{
			if (GVvista())
			{
				strProcess  = _T("winstart /stop /param=\"-iistar=\"");
				strProcess += m_strComponentName;
				strProcess += _T("\"");
			}
			else
			{
			strProcess  = _T("ingstop -iistar=");
			strProcess += m_strComponentName;
			}
		}
	}
	return strProcess;
}


CaPageInformation* CaItemDataStar::GetPageInformation ()
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

	m_pPageInfo = new CaPageInformation (_T("STAR"), 3, nTabID, nDlgID);
	CaEventFilter& filter = m_pPageInfo->GetEventFilter();
	//
	// Category and Name are not set for this item.
	filter.SetFilterFlag (FILER_DATE_BEGIN|FILER_DATE_END|FILER_INSTANCE);
	return m_pPageInfo;
}


void CaItemDataStar::UpdateIcon(CTreeCtrl* pTree)
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
		nImageIcon = bAlert? IM_STAR_N_NORMAL: IM_STAR_R_NORMAL;
	}
	else
	if (nState == COMP_STOP || nState == (COMP_NORMAL|COMP_STOP))
	{
		//
		// STOPPED:
		nImageIcon = bAlert? IM_STAR_N_STOPPED: IM_STAR_R_STOPPED;
	}
	else
	if (nState == COMP_START || nState == (COMP_NORMAL|COMP_START))
	{
		//
		// STARTED:
		nImageIcon = bAlert? IM_STAR_N_STARTED: IM_STAR_R_STARTED;
	}
	else
	if (nState&COMP_STARTMORE && !(nState&COMP_STOP) && !(nState&COMP_HALFSTART))
	{
		//
		// STARTED MORE:
		nImageIcon = bAlert? IM_STAR_N_STARTEDMORE: IM_STAR_R_STARTEDMORE;
	}
	else
	{
		//
		// HALF STARTED:
		nImageIcon = bAlert? IM_STAR_N_HALFSTARTED: IM_STAR_R_HALFSTARTED;
	}
	pTree->SetItemImage (m_treeCtrlData.GetTreeItem(), nImageIcon, nImageIcon);
}




//
// SECURE Item data
// --------------
CaItemDataSecurity::CaItemDataSecurity():CaTreeComponentItemData()
{
	m_bDisplayInstance = FALSE;
	m_nOrder = ORDER_SECURITY;
	m_treeCtrlData.SetImage (IM_SECURITY_R_NORMAL, IM_SECURITY_R_NORMAL);
}


CaPageInformation* CaItemDataSecurity::GetPageInformation ()
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

	m_pPageInfo = new CaPageInformation (_T("SECURITY"), 3, nTabID, nDlgID);
	CaEventFilter& filter = m_pPageInfo->GetEventFilter();
	//
	// Category and Name are not set for this item.
	filter.SetFilterFlag (FILER_DATE_BEGIN|FILER_DATE_END|FILER_INSTANCE);
	return m_pPageInfo;
}

void CaItemDataSecurity::UpdateIcon(CTreeCtrl* pTree)
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
		nImageIcon = bAlert? IM_SECURITY_N_NORMAL: IM_SECURITY_R_NORMAL;
	}
	else
	if (nState == COMP_STOP || nState == (COMP_NORMAL|COMP_STOP))
	{
		//
		// STOPPED:
		nImageIcon = bAlert? IM_SECURITY_N_STOPPED: IM_SECURITY_R_STOPPED;
	}
	else
	if (nState == COMP_START || nState == (COMP_NORMAL|COMP_START))
	{
		//
		// STARTED:
		nImageIcon = bAlert? IM_SECURITY_N_STARTED: IM_SECURITY_R_STARTED;
	}
	else
	if (nState&COMP_STARTMORE && !(nState&COMP_STOP) && !(nState&COMP_HALFSTART))
	{
		//
		// STARTED MORE:
		nImageIcon = bAlert? IM_SECURITY_N_STARTEDMORE: IM_SECURITY_R_STARTEDMORE;
	}
	else
	{
		//
		// HALF STARTED:
		nImageIcon = bAlert? IM_SECURITY_N_HALFSTARTED: IM_SECURITY_R_HALFSTARTED;
	}
	pTree->SetItemImage (m_treeCtrlData.GetTreeItem(), nImageIcon, nImageIcon);
}


//
// LOCK Item data
// --------------
CaItemDataLocking::CaItemDataLocking():CaTreeComponentItemData()
{
	m_bDisplayInstance = FALSE;
	m_nOrder = ORDER_LOCKING;
	m_treeCtrlData.SetImage (IM_LOCKING_R_NORMAL, IM_LOCKING_R_NORMAL);
}


CaPageInformation* CaItemDataLocking::GetPageInformation ()
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
		IDD_STATUS_SINGLEINSTANCE_COMPONENT,
		IDD_LOGEVENT_GENERIC,
		IDD_MESSAGE_STATISTIC
	};

	m_pPageInfo = new CaPageInformation (_T("LOCK"), 3, nTabID, nDlgID);
	CaEventFilter& filter = m_pPageInfo->GetEventFilter();
	//
	// Category and Name are not set for this item.
	filter.SetFilterFlag (FILER_DATE_BEGIN|FILER_DATE_END|FILER_INSTANCE);
	return m_pPageInfo;
}

void CaItemDataLocking::UpdateIcon(CTreeCtrl* pTree)
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
		nImageIcon = bAlert? IM_LOCKING_N_NORMAL: IM_LOCKING_R_NORMAL;
	}
	else
	if (nState == COMP_STOP || nState == (COMP_NORMAL|COMP_STOP))
	{
		//
		// STOPPED:
		nImageIcon = bAlert? IM_LOCKING_N_STOPPED: IM_LOCKING_R_STOPPED;
	}
	else
	if (nState == COMP_START || nState == (COMP_NORMAL|COMP_START))
	{
		//
		// STARTED:
		nImageIcon = bAlert? IM_LOCKING_N_STARTED: IM_LOCKING_R_STARTED;
	}
	else
	if (nState&COMP_STARTMORE && !(nState&COMP_STOP) && !(nState&COMP_HALFSTART))
	{
		//
		// STARTED MORE:
		nImageIcon = bAlert? IM_LOCKING_N_STARTEDMORE: IM_LOCKING_R_STARTEDMORE;
	}
	else
	{
		//
		// HALF STARTED:
		nImageIcon = bAlert? IM_LOCKING_N_HALFSTARTED: IM_LOCKING_R_HALFSTARTED;
	}
	pTree->SetItemImage (m_treeCtrlData.GetTreeItem(), nImageIcon, nImageIcon);
}


//
// LOG Item data
// --------------
CaItemDataLogging::CaItemDataLogging():CaTreeComponentItemData()
{
	m_bDisplayInstance = FALSE;
	m_nOrder = ORDER_LOGGING;
	m_treeCtrlData.SetImage (IM_LOGGING_R_NORMAL, IM_LOGGING_R_NORMAL);
}



CaPageInformation* CaItemDataLogging::GetPageInformation ()
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
		IDD_STATUS_SINGLEINSTANCE_COMPONENT,
		IDD_LOGEVENT_GENERIC,
		IDD_MESSAGE_STATISTIC
	};

	m_pPageInfo = new CaPageInformation (_T("LOG"), 3, nTabID, nDlgID);
	CaEventFilter& filter = m_pPageInfo->GetEventFilter();
	//
	// Category and Name are not set for this item.
	filter.SetFilterFlag (FILER_DATE_BEGIN|FILER_DATE_END|FILER_INSTANCE);
	return m_pPageInfo;
}


void CaItemDataLogging::UpdateIcon(CTreeCtrl* pTree)
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
		nImageIcon = bAlert? IM_LOGGING_N_NORMAL: IM_LOGGING_R_NORMAL;
	}
	else
	if (nState == COMP_STOP || nState == (COMP_NORMAL|COMP_STOP))
	{
		//
		// STOPPED:
		nImageIcon = bAlert? IM_LOGGING_N_STOPPED: IM_LOGGING_R_STOPPED;
	}
	else
	if (nState == COMP_START || nState == (COMP_NORMAL|COMP_START))
	{
		//
		// STARTED:
		nImageIcon = bAlert? IM_LOGGING_N_STARTED: IM_LOGGING_R_STARTED;
	}
	else
	if (nState&COMP_STARTMORE && !(nState&COMP_STOP) && !(nState&COMP_HALFSTART))
	{
		//
		// STARTED MORE:
		nImageIcon = bAlert? IM_LOGGING_N_STARTEDMORE: IM_LOGGING_R_STARTEDMORE;
	}
	else
	{
		//
		// HALF STARTED:
		nImageIcon = bAlert? IM_LOGGING_N_HALFSTARTED: IM_LOGGING_R_HALFSTARTED;
	}
	pTree->SetItemImage (m_treeCtrlData.GetTreeItem(), nImageIcon, nImageIcon);
}

//
// LOGFILE Item data
// --------------
CaItemDataLogFile::CaItemDataLogFile():CaTreeComponentItemData()
{
	m_bDisplayInstance = FALSE;
	m_treeCtrlData.SetImage (IM_TRANSACTION_R_NORMAL, IM_TRANSACTION_R_NORMAL);
}


CaPageInformation* CaItemDataLogFile::GetPageInformation ()
{
	//
	// Remove all tabs: (sept-21-1999:12:26)
	return NULL;

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
		IDD_STATUS_SINGLEINSTANCE_COMPONENT,
		IDD_LOGEVENT_GENERIC,
		IDD_MESSAGE_STATISTIC
	};

	m_pPageInfo = new CaPageInformation (_T("LOGFILE"), 3, nTabID, nDlgID);
	CaEventFilter& filter = m_pPageInfo->GetEventFilter();
	//
	// Category and Name are not set for this item.
	filter.SetFilterFlag (FILER_DATE_BEGIN|FILER_DATE_END|FILER_INSTANCE);
	return m_pPageInfo;
}


void CaItemDataLogFile::UpdateIcon(CTreeCtrl* pTree)
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
		nImageIcon = bAlert? IM_TRANSACTION_N_NORMAL: IM_TRANSACTION_R_NORMAL;
	}
	else
	if (nState == COMP_STOP || nState == (COMP_NORMAL|COMP_STOP))
	{
		//
		// STOPPED:
		nImageIcon = bAlert? IM_TRANSACTION_N_STOPPED: IM_TRANSACTION_R_STOPPED;
	}
	else
	if (nState == COMP_START || nState == (COMP_NORMAL|COMP_START))
	{
		//
		// STARTED:
		nImageIcon = bAlert? IM_TRANSACTION_N_STARTED: IM_TRANSACTION_R_STARTED;
	}
	else
	if (nState&COMP_STARTMORE && !(nState&COMP_STOP) && !(nState&COMP_HALFSTART))
	{
		//
		// STARTED MORE:
		nImageIcon = bAlert? IM_TRANSACTION_N_STARTEDMORE: IM_TRANSACTION_R_STARTEDMORE;
	}
	else
	{
		//
		// HALF STARTED:
		nImageIcon = bAlert? IM_TRANSACTION_N_HALFSTARTED: IM_TRANSACTION_R_HALFSTARTED;
	}
	pTree->SetItemImage (m_treeCtrlData.GetTreeItem(), nImageIcon, nImageIcon);
}


//
// RECOVER Item data
// --------------
CaItemDataRecovery::CaItemDataRecovery():CaTreeComponentItemData()
{
	m_nOrder = ORDER_RECOVERY;
	m_treeCtrlData.SetImage (IM_RECOVERY_R_NORMAL, IM_RECOVERY_R_NORMAL);
	if (GVvista())
	{
		m_strStart = _T("winstart /start /param=\"-dmfrcp\"");
		m_strStop = _T("winstart /stop /param=\"-dmfrcp\"");
	}
	else
	{
	m_strStart = _T("ingstart -dmfrcp");
	m_strStop = _T("ingstop -dmfrcp");
	}
}


CaPageInformation* CaItemDataRecovery::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [4] = 
	{
		IDS_TAB_STATUS,
		IDS_TAB_RECOVERY_LOG,
		IDS_TAB_LOGEVENT,
		IDS_TAB_STATISTIC,
	};
	UINT nDlgID [4] = 
	{
		IDD_STATUS_SINGLEINSTANCE_COMPONENT,
		IDD_LOGEVENT_ACPRCP,
		IDD_LOGEVENT_GENERIC,
		IDD_MESSAGE_STATISTIC,
	};
		
	m_pPageInfo = new CaPageInformation (_T("RECOVERY"), 4, nTabID, nDlgID);
	CaEventFilter& filter = m_pPageInfo->GetEventFilter();
	//
	// Category and Name are not set for this item.
	filter.SetFilterFlag (FILER_DATE_BEGIN|FILER_DATE_END|FILER_INSTANCE);
	return m_pPageInfo;
}


void CaItemDataRecovery::UpdateIcon(CTreeCtrl* pTree)
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
		nImageIcon = bAlert? IM_RECOVERY_N_NORMAL: IM_RECOVERY_R_NORMAL;
	}
	else
	if (nState == COMP_STOP || nState == (COMP_NORMAL|COMP_STOP))
	{
		//
		// STOPPED:
		nImageIcon = bAlert? IM_RECOVERY_N_STOPPED: IM_RECOVERY_R_STOPPED;
	}
	else
	if (nState == COMP_START || nState == (COMP_NORMAL|COMP_START))
	{
		//
		// STARTED:
		nImageIcon = bAlert? IM_RECOVERY_N_STARTED: IM_RECOVERY_R_STARTED;
	}
	else
	if (nState&COMP_STARTMORE && !(nState&COMP_STOP) && !(nState&COMP_HALFSTART))
	{
		//
		// STARTED MORE:
		nImageIcon = bAlert? IM_RECOVERY_N_STARTEDMORE: IM_RECOVERY_R_STARTEDMORE;
	}
	else
	{
		//
		// HALF STARTED:
		nImageIcon = bAlert? IM_RECOVERY_N_HALFSTARTED: IM_RECOVERY_R_HALFSTARTED;
	}
	pTree->SetItemImage (m_treeCtrlData.GetTreeItem(), nImageIcon, nImageIcon);
}



//
// ARCHIVE Item data
// --------------
CaItemDataArchiver::CaItemDataArchiver():CaTreeComponentItemData()
{
	m_bDisplayInstance = FALSE;
	m_nOrder = ORDER_ARCHIVER;
	m_treeCtrlData.SetImage (IM_ARCHIVER_R_NORMAL, IM_ARCHIVER_R_NORMAL);
	if (GVvista())
	{
		m_strStart = _T("winstart /start /param=\"-dmfacp\"");
		m_strStop = _T("winstart /stop /param=\"-dmfacp\"");
	}
	else
	{
	m_strStart = _T("ingstart -dmfacp");
	m_strStop = _T("ingstop -dmfacp");
	}
}


CaPageInformation* CaItemDataArchiver::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [4] = 
	{
		IDS_TAB_STATUS,
		IDS_TAB_ARCHIVER_LOG,
		IDS_TAB_LOGEVENT,
		IDS_TAB_STATISTIC,
	};
	UINT nDlgID [4] = 
	{
		IDD_STATUS_SINGLEINSTANCE_COMPONENT,
		IDD_LOGEVENT_ACPRCP,
		IDD_LOGEVENT_GENERIC,
		IDD_MESSAGE_STATISTIC,
	};
	
	m_pPageInfo = new CaPageInformation (_T("ARCHIVER"), 4, nTabID, nDlgID);
	CaEventFilter& filter = m_pPageInfo->GetEventFilter();
	//
	// Category and Name are not set for this item.
	filter.SetFilterFlag (FILER_DATE_BEGIN|FILER_DATE_END|FILER_INSTANCE);
	return m_pPageInfo;
}

void CaItemDataArchiver::UpdateIcon(CTreeCtrl* pTree)
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
		nImageIcon = bAlert? IM_ARCHIVER_N_NORMAL: IM_ARCHIVER_R_NORMAL;
	}
	else
	if (nState == COMP_STOP || nState == (COMP_NORMAL|COMP_STOP))
	{
		//
		// STOPPED:
		nImageIcon = bAlert? IM_ARCHIVER_N_STOPPED: IM_ARCHIVER_R_STOPPED;
	}
	else
	if (nState == COMP_START || nState == (COMP_NORMAL|COMP_START))
	{
		//
		// STARTED:
		nImageIcon = bAlert? IM_ARCHIVER_N_STARTED: IM_ARCHIVER_R_STARTED;
	}
	else
	if (nState&COMP_STARTMORE && !(nState&COMP_STOP) && !(nState&COMP_HALFSTART))
	{
		//
		// STARTED MORE:
		nImageIcon = bAlert? IM_ARCHIVER_N_STARTEDMORE: IM_ARCHIVER_R_STARTEDMORE;
	}
	else
	{
		//
		// HALF STARTED:
		nImageIcon = bAlert? IM_ARCHIVER_N_HALFSTARTED: IM_ARCHIVER_R_HALFSTARTED;
	}
	pTree->SetItemImage (m_treeCtrlData.GetTreeItem(), nImageIcon, nImageIcon);
}


//
// OpenIngres DESKTOP DBMS Item data
// ---------------------------------
CaItemDataDbmsOidt::CaItemDataDbmsOidt():CaTreeComponentFolderItemData()
{
	m_nOrder = ORDER_DBMS;
	m_treeCtrlData.SetImage (IM_DBMS_R_NORMAL, IM_DBMS_R_NORMAL);
}


CaPageInformation* CaItemDataDbmsOidt::GetPageInformation ()
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

	m_pPageInfo = new CaPageInformation (_T("OIDTDBMS"), 3, nTabID, nDlgID);
	CaEventFilter& filter = m_pPageInfo->GetEventFilter();
	//
	// Category and Name are not set for this item.
	filter.SetFilterFlag (FILER_DATE_BEGIN|FILER_DATE_END|FILER_INSTANCE);
	return m_pPageInfo;
}

void CaItemDataDbmsOidt::UpdateIcon(CTreeCtrl* pTree)
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
		nImageIcon = bAlert? IM_DBMS_N_NORMAL: IM_DBMS_R_NORMAL;
	}
	else
	if (nState == COMP_STOP || nState == (COMP_NORMAL|COMP_STOP))
	{
		//
		// STOPPED:
		nImageIcon = bAlert? IM_DBMS_N_STOPPED: IM_DBMS_R_STOPPED;
	}
	else
	if (nState == COMP_START || nState == (COMP_NORMAL|COMP_START))
	{
		//
		// STARTED:
		nImageIcon = bAlert? IM_DBMS_N_STARTED: IM_DBMS_R_STARTED;
	}
	else
	if (nState&COMP_STARTMORE && !(nState&COMP_STOP) && !(nState&COMP_HALFSTART))
	{
		//
		// STARTED MORE:
		nImageIcon = bAlert? IM_DBMS_N_STARTEDMORE: IM_DBMS_R_STARTEDMORE;
	}
	else
	{
		//
		// HALF STARTED:
		nImageIcon = bAlert? IM_DBMS_N_HALFSTARTED: IM_DBMS_R_HALFSTARTED;
	}
	pTree->SetItemImage (m_treeCtrlData.GetTreeItem(), nImageIcon, nImageIcon);
}


//
// RMCMD Item data
// --------------
CaItemDataRmcmd::CaItemDataRmcmd():CaTreeComponentItemData()
{
	m_nOrder = ORDER_RMCMD;
	m_treeCtrlData.SetImage (IM_RMCMD_R_NORMAL, IM_RMCMD_R_NORMAL);
	if (GVvista())
	{
		m_strStart = _T("winstart /start /param=\"-rmcmd\"");
		m_strStop = _T("winstart /stop /param=\"-rmcmd\"");
	}
	else
	{
	m_strStart = _T("ingstart -rmcmd");
	m_strStop = _T("ingstop -rmcmd");
	}
}


CaPageInformation* CaItemDataRmcmd::GetPageInformation ()
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
	
	m_pPageInfo = new CaPageInformation (_T("RMCMD"), 3, nTabID, nDlgID);
	CaEventFilter& filter = m_pPageInfo->GetEventFilter();
	//
	// Category and Name are not set for this item.
	filter.SetFilterFlag (FILER_DATE_BEGIN|FILER_DATE_END|FILER_INSTANCE);
	return m_pPageInfo;
}

void CaItemDataRmcmd::UpdateIcon(CTreeCtrl* pTree)
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
		nImageIcon = bAlert? IM_RMCMD_N_NORMAL: IM_RMCMD_R_NORMAL;
	}
	else
	if (nState == COMP_STOP || nState == (COMP_NORMAL|COMP_STOP))
	{
		//
		// STOPPED:
		nImageIcon = bAlert? IM_RMCMD_N_STOPPED: IM_RMCMD_R_STOPPED;
	}
	else
	if (nState == COMP_START || nState == (COMP_NORMAL|COMP_START))
	{
		//
		// STARTED:
		nImageIcon = bAlert? IM_RMCMD_N_STARTED: IM_RMCMD_R_STARTED;
	}
	else
	if (nState&COMP_STARTMORE && !(nState&COMP_STOP) && !(nState&COMP_HALFSTART))
	{
		//
		// STARTED MORE:
		nImageIcon = bAlert? IM_RMCMD_N_STARTEDMORE: IM_RMCMD_R_STARTEDMORE;
	}
	else
	{
		//
		// HALF STARTED:
		nImageIcon = bAlert? IM_RMCMD_N_HALFSTARTED: IM_RMCMD_R_HALFSTARTED;
	}
	pTree->SetItemImage (m_treeCtrlData.GetTreeItem(), nImageIcon, nImageIcon);
}



//
// ORACLE Item data
// --------------
CaItemDataOracle::CaItemDataOracle():CaTreeComponentGatewayItemData()
{
	m_treeCtrlData.SetImage (IM_GATEWAY_R_NORMAL, IM_GATEWAY_R_NORMAL);
	m_strStart = _T("");
	m_strStop = _T("");
}

void CaItemDataOracle::SetStartStopArg(LPCTSTR lpstartstoparg)
{
	if (GVvista())
	{
	m_strStart.Format(_T("winstart /start /param=\"-%s\""), lpstartstoparg);
	m_strStop.Format (_T("winstart /stop /param=\"-%s\"") , lpstartstoparg);
	}
	else
	{
	m_strStart.Format(_T("ingstart -%s"), lpstartstoparg);
	m_strStop.Format (_T("ingstop -%s") , lpstartstoparg);
	}
}

CaPageInformation* CaItemDataOracle::GetPageInformation ()
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

	m_pPageInfo = new CaPageInformation (_T("GW_GENERIC"), 3, nTabID, nDlgID);
	CaEventFilter& filter = m_pPageInfo->GetEventFilter();
	//
	// Category and Name are not set for this item.
	filter.SetFilterFlag (FILER_DATE_BEGIN|FILER_DATE_END|FILER_INSTANCE);
	return m_pPageInfo;
}

//
// INFORMIX Item data
// --------------
CaItemDataInformix::CaItemDataInformix():CaTreeComponentGatewayItemData()
{
	m_treeCtrlData.SetImage (IM_GATEWAY_R_NORMAL, IM_GATEWAY_R_NORMAL);
	if (GVvista())
	{
		m_strStart = _T("winstart /start /param=\"-informix\"");
		m_strStop = _T("winstart /stop /param=\"-informix\"");
	}
	else
	{
	m_strStart = _T("ingstart -informix");
	m_strStop = _T("ingstop -informix");
	}
}



CaPageInformation* CaItemDataInformix::GetPageInformation ()
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

	m_pPageInfo = new CaPageInformation (_T("INFORMIX"), 3, nTabID, nDlgID);
	CaEventFilter& filter = m_pPageInfo->GetEventFilter();
	//
	// Category and Name are not set for this item.
	filter.SetFilterFlag (FILER_DATE_BEGIN|FILER_DATE_END|FILER_INSTANCE);
	return m_pPageInfo;
}

//
// SYBASE Item data
// --------------
CaItemDataSybase::CaItemDataSybase():CaTreeComponentGatewayItemData()
{
	m_treeCtrlData.SetImage (IM_GATEWAY_R_NORMAL, IM_GATEWAY_R_NORMAL);
	if (GVvista())
	{
		m_strStart = _T("winstart /start /param=\"-sybase\"");
		m_strStop  = _T("winstart /stop /param=\"-sybase\"");
	}
	else
	{
	m_strStart = _T("ingstart -sybase");
	m_strStop  = _T("ingstop -sybase");
	}
}



CaPageInformation* CaItemDataSybase::GetPageInformation ()
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

	m_pPageInfo = new CaPageInformation (_T("SYBASE"), 3, nTabID, nDlgID);
	CaEventFilter& filter = m_pPageInfo->GetEventFilter();
	//
	// Category and Name are not set for this item.
	filter.SetFilterFlag (FILER_DATE_BEGIN|FILER_DATE_END|FILER_INSTANCE);
	return m_pPageInfo;
}

//
// MSSQL Item data
// --------------

CaItemDataMsSql::CaItemDataMsSql():CaTreeComponentGatewayItemData()
{
	m_treeCtrlData.SetImage (IM_GATEWAY_R_NORMAL, IM_GATEWAY_R_NORMAL);
	if (GVvista())
	{
		m_strStart = _T("winstart /start /param=\"-mssql\"");
		m_strStop  = _T("winstart /stop /param=\"-mssql\"");
	}
	else
	{
	m_strStart = _T("ingstart -mssql");
	m_strStop  = _T("ingstop -mssql");
	}
}



CaPageInformation* CaItemDataMsSql::GetPageInformation ()
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

	m_pPageInfo = new CaPageInformation (_T("MSSQL"), 3, nTabID, nDlgID);
	CaEventFilter& filter = m_pPageInfo->GetEventFilter();
	//
	// Category and Name are not set for this item.
	filter.SetFilterFlag (FILER_DATE_BEGIN|FILER_DATE_END|FILER_INSTANCE);
	return m_pPageInfo;
}

//
// ODBC Item data
// --------------
CaItemDataOdbc::CaItemDataOdbc():CaTreeComponentGatewayItemData()
{
	m_treeCtrlData.SetImage (IM_GATEWAY_R_NORMAL, IM_GATEWAY_R_NORMAL);
	if (GVvista())
	{
		m_strStart = _T("winstart /start /param=\"-odbc\"");
		m_strStop  = _T("winstart /stop /param=\"-odbc\"");
	}
	else
	{
	m_strStart = _T("ingstart -odbc");
	m_strStop  = _T("ingstop -odbc");
	}
}


CaPageInformation* CaItemDataOdbc::GetPageInformation ()
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

	m_pPageInfo = new CaPageInformation (_T("ODBC"), 3, nTabID, nDlgID);
	CaEventFilter& filter = m_pPageInfo->GetEventFilter();
	//
	// Category and Name are not set for this item.
	filter.SetFilterFlag (FILER_DATE_BEGIN|FILER_DATE_END|FILER_INSTANCE);
	return m_pPageInfo;
}

