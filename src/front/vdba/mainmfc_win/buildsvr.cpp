/*****************************************************************************
**
** Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**
**  Project : Visual DBA
**
**  Name:    buildsvr.cpp - implementation file
**
**  Description:
**	dialog box for change run node
**
** History:
**    03-aug-1998 (schph01)
**	Created. 
**    28-Jun-2001 (hanje04)
**	Added CString() around m_ReplicSvrData.RunNode to stop Solaris
**	compiler errors after upgrade WS 6 update 1
**
*****************************************************************************/


#include "stdafx.h"
#include "mainmfc.h"
#include "buildsvr.h"
#include "msghandl.h"
#include "starutil.h"

extern "C" {
#include "monitor.h"
#include "domdata.h"
#include "dbaginfo.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// interface to dom.c
extern "C" void MfcDlgBuildSvr(long CurNodeHandle, char *CurNode,char *CurDBName,char *CurUser)
{
	CuDlgReplicationBuildServer dlg;
	CString csLocal;
	csLocal.LoadString(IDS_I_LOCALNODE);

	dlg.m_CurNode	= CurNode;
	if(dlg.m_CurNode == csLocal)
		dlg.m_CurNode =  GetLocalHostName();
	dlg.m_CurDBName	= CurDBName;
	dlg.m_CurUser	= CurUser;
	dlg.m_CurNodeHandle = CurNodeHandle;
	dlg.DoModal();
}
/////////////////////////////////////////////////////////////////////////////
// CuDlgReplicationBuildServer dialog


CuDlgReplicationBuildServer::CuDlgReplicationBuildServer(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgReplicationBuildServer::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgReplicationBuildServer)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgReplicationBuildServer::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgReplicationBuildServer)
	DDX_Control(pDX, IDC_FORCE_BUILD, m_ForceBuild);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgReplicationBuildServer, CDialog)
	//{{AFX_MSG_MAP(CuDlgReplicationBuildServer)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgReplicationBuildServer message handlers

BOOL CuDlgReplicationBuildServer::OnInitDialog() 
{
	CDialog::OnInitDialog();
	NODEDATAMIN nodedata;
	int ires,index;
	CString csLocal,cTempo,cDisplay,csRunNode,csLocHostName;
	POSITION posServer,posDBName;

	VERIFY (m_CheckBuildServerList.SubclassDlgItem (IDC_BUILD_SERVER_LIST, this));

	m_CheckBuildServerList.ResetContent();
	csLocal.LoadString(IDS_I_LOCALNODE);
	csLocHostName = GetLocalHostName();
	FillReplicServerList();
	ires=GetFirstMonInfo(0,0,NULL,OT_NODE,(void * )&nodedata,NULL);
	while (ires==RES_SUCCESS) {
		cTempo.Empty();
		if (csLocal == CString(nodedata.NodeName)) {
			cTempo = csLocHostName;
			cDisplay = csLocal + " " + csLocHostName;
		}
		else {
			if (csLocHostName.CompareNoCase((LPCTSTR)nodedata.NodeName) == 0 &&
				*nodedata.LoginDta.Login == '*') {
				ires=GetNextMonInfo((void * )&nodedata);// This is the Installation Password,
				continue;								// see the next node name.
			}
			cTempo = nodedata.NodeName;
			cDisplay = nodedata.NodeName;
		}

		ASSERT( m_ListReplicDBName.GetCount() == m_ListReplicServer.GetCount());
	
		posDBName = m_ListReplicDBName.GetHeadPosition();
		posServer = m_ListReplicServer.GetHeadPosition();
		while ( posServer != NULL && posDBName != NULL ) {
			csRunNode = m_ListReplicServer.GetNext(posServer);
			CString& rLpCsDBName  = m_ListReplicServer.GetNext(posDBName);
			if (csRunNode.CompareNoCase(cTempo) == 0) {
				index = m_CheckBuildServerList.AddString(cDisplay);
				m_CheckBuildServerList.SetItemData(index, (DWORD)(LPCTSTR)rLpCsDBName);
				m_CheckBuildServerList.SetCheck(index,1);
				break;
			}
		}
		ires=GetNextMonInfo((void * )&nodedata);
	}
	PushHelpId (IDHELP_IDD_REPLICATION_BUILD_SERVER);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgReplicationBuildServer::OnOK() 
{
	int index,hnode;
	CString cNodeNameTemp,cNodeName,csLocal,cTitle;

	csLocal.LoadString(IDS_I_LOCALNODE);

	CDialog::OnOK();
	for (index = 0; index < m_CheckBuildServerList.GetCount(); index++) {
		if (m_CheckBuildServerList.GetCheck(index) == 1) {
			m_CheckBuildServerList.GetText( index, cNodeNameTemp );
			LPCTSTR LpCsDBName;
			LpCsDBName = (LPCTSTR)m_CheckBuildServerList.GetItemData(index);

			if ( !cNodeNameTemp.IsEmpty())	{
				if ( cNodeNameTemp.Find(csLocal) != -1)
					cNodeName = csLocal;
				else
					cNodeName = cNodeNameTemp;
				// 
				cNodeName += LPUSERPREFIXINNODENAME + m_CurUser + LPUSERSUFFIXINNODENAME;
				hnode = OpenNodeStruct  ((LPUCHAR)(LPCTSTR)cNodeName);
				if (hnode<0) {
					CString strMsg = VDBA_MfcResourceString (IDS_MAX_NB_CONNECT);//_T("Maximum number of connections has been reached"
					strMsg += VDBA_MfcResourceString (IDS_E_BUILD_SVR);			//	" - Cannot Build Server."  
					AfxMessageBox (strMsg);
				return;
				}
				// Temporary for activate a session
				UCHAR buf[MAXOBJECTNAME];
				DOMGetFirstObject (hnode, OT_DATABASE, 0, NULL, FALSE, NULL, buf, NULL, NULL);

				CString cCommandLine;
				if (m_ForceBuild.GetCheck() == 1)
					cCommandLine.Format("imagerep %s -u%s -f",(LPCTSTR)LpCsDBName,
															  (LPCTSTR)m_CurUser);
				else
					cCommandLine.Format("imagerep %s -u%s",(LPCTSTR)LpCsDBName,
														   (LPCTSTR)m_CurUser);
				cTitle.Format("Build Server Operation on %s",(LPCTSTR)cNodeName);
				execrmcmd1(	(char *)(LPCTSTR)cNodeName, 
							(char *)(LPCTSTR)cCommandLine,
							(char *)(LPCTSTR)cTitle,
							FALSE);
				CloseNodeStruct(hnode,FALSE);

			}
		}
	}
}

void CuDlgReplicationBuildServer::OnDestroy() 
{
	CDialog::OnDestroy();
	PopHelpId();
}

void CuDlgReplicationBuildServer::FillReplicServerList()
{
	int iret,NodeHandle;
	RESOURCEDATAMIN DBResourceDataMin;
	REPLICSERVERDATAMIN data;
	CWaitCursor cWaitCur;

	m_ListReplicServer.RemoveAll();
	m_ListReplicDBName.RemoveAll();

	LPUCHAR vnodeName = (LPUCHAR)GetVirtNodeName ( m_CurNodeHandle );
	NodeHandle  = OpenNodeStruct ( vnodeName );
	if (NodeHandle == -1)
		return;
	memset(&DBResourceDataMin,'\0',sizeof(RESOURCEDATAMIN));
	memset(&data,'\0',sizeof(REPLICSERVERDATAMIN));

	FillResStructFromDB(&DBResourceDataMin,(UCHAR *)(LPCTSTR)m_CurDBName);
	for ( iret = DBAGetFirstObject (vnodeName,
									OT_MON_REPLIC_SERVER,
									1,
									(LPUCHAR *)&DBResourceDataMin,
									TRUE,
									(LPUCHAR)&data,
									NULL,NULL);
									iret == RES_SUCCESS ;
									iret = DBAGetNextObject((LPUCHAR)&data,NULL,NULL) )
	{
		m_ListReplicServer.AddTail((LPCTSTR)data.RunNode);
		m_ListReplicDBName.AddTail((LPCTSTR)data.LocalDBName);
	}
	CloseNodeStruct(NodeHandle,FALSE);
	ASSERT( m_ListReplicDBName.GetCount() == m_ListReplicServer.GetCount());
}
