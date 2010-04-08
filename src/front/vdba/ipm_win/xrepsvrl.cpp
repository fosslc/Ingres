/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xrepsvrl.cpp, Implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : schph01
**    Purpose  : Send DBevents to Replication server. This Box allows to specify
**               Server(s) where the dbevents are to send to.
**
** History:
**
** xx-Dec-1997 (schph01)
**    Created
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "xrepsvrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CxDlgReplicationServerList::CxDlgReplicationServerList(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgReplicationServerList::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgReplicationServerList)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_pIpmDoc = NULL;
}


void CxDlgReplicationServerList::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgReplicationServerList)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgReplicationServerList, CDialog)
	//{{AFX_MSG_MAP(CxDlgReplicationServerList)
	ON_WM_DESTROY()
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgReplicationServerList message handlers

void CxDlgReplicationServerList::OnOK() 
{
	CDialog::OnOK();

	try
	{
		CPtrList listInfoStruct;
		int index, nCount = m_CheckListSvr.GetCount();
		for (index = 0; index < nCount; index++) 
		{
			if (m_CheckListSvr.GetCheck(index))
			{
				REPLICSERVERDATAMIN* pData = (REPLICSERVERDATAMIN*)m_CheckListSvr.GetItemDataPtr(index);
				listInfoStruct.AddTail(pData);
			}
		}
		IPM_SendEventToServers(m_pIpmDoc,m_strEventFlag, m_strServerFlag, listInfoStruct);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage ();
		e->Delete();
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
}

BOOL CxDlgReplicationServerList::OnInitDialog() 
{
	CDialog::OnInitDialog();

	VERIFY (m_CheckListSvr.SubclassDlgItem (IDC_SERVER_LIST, this));
	m_CheckListSvr.ResetContent();

	CPtrList listInfoStruct;
	REPLICSERVERDATAMIN data;
	IPMUPDATEPARAMS ups;
	memset (&ups, 0, sizeof(ups));
	memset (&data, 0, sizeof(data));
	ups.nType = 1;
	ups.pStruct = m_pDBResourceDataMin;
	ups.pSFilter= NULL;

	CaIpmQueryInfo queryInfo (m_pIpmDoc, OT_MON_REPLIC_SERVER, &ups, &data, sizeof(data));
	BOOL bOK = IPM_QueryInfo (&queryInfo, listInfoStruct);
	if (bOK)
	{
		int index = -1;
		TCHAR tchszServer[256];
		POSITION pos = listInfoStruct.GetHeadPosition();
		while (pos != NULL)
		{
			REPLICSERVERDATAMIN* pData = (REPLICSERVERDATAMIN*)listInfoStruct.GetNext(pos);
			IPM_GetInfoName(m_pIpmDoc, OT_MON_REPLIC_SERVER, pData, tchszServer, sizeof(tchszServer));
			index = m_CheckListSvr.AddString(tchszServer);
			if (index != LB_ERR)
			{
				m_CheckListSvr.SetItemDataPtr(index, pData);
			}
		}
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgReplicationServerList::OnDestroy() 
{
	int index;
	LPREPLICSERVERDATAMIN dta;
	for (index = 0; index < m_CheckListSvr.GetCount(); index++) {
		dta = (LPREPLICSERVERDATAMIN)m_CheckListSvr.GetItemDataPtr(index);
		delete dta;
	}
	CDialog::OnDestroy();
}


BOOL CxDlgReplicationServerList::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	// OLD vdba: DHELP_IDD_REPLICATION_SERVER_LIST   8020
	return APP_HelpInfo(m_hWnd, 8020);
}
