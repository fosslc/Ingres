/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : runnode.cpp, implementation file.
**    Project  : INGRES II/ Replication Monitoring.
**    Author   : Schalk Philippe
**    Purpose  : Page of Table control. The Ice Active User Detail
**
** History:
**
** 11-May-1998 (schph01)
**    Created.
** 28-Sep-1999 (schph01)
**    bug #98656
**    Verify the Ingres version before display of the message.
**    (IDS_I_DONT_FORGET_REBUILD)
** 28-Jun-2001 (hanje04)
**    Added CString() around m_ReplicSvrData.RunNode to stop Solaris 
**    compiler errors after upgrade WS 6 update 1
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "ipmdoc.h"
#include "runnode.h"
#include "vnodedml.h"
#include "monrepli.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



CxDlgReplicationRunNode::CxDlgReplicationRunNode(CdIpmDoc* pDoc, CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgReplicationRunNode::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgReplicationRunNode)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_pIpmDoc = pDoc;
}


void CxDlgReplicationRunNode::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgReplicationRunNode)
	DDX_Control(pDX, IDOK, m_cOK);
	DDX_Control(pDX, IDC_LIST1, m_listCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgReplicationRunNode, CDialog)
	//{{AFX_MSG_MAP(CxDlgReplicationRunNode)
	ON_LBN_SELCHANGE(IDC_LIST1, OnSelchangeListVnode)
	ON_WM_DESTROY()
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgReplicationRunNode message handlers

BOOL CxDlgReplicationRunNode::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CString strVnodeName, cTempo;
	CString csLocHostName = LIBMON_getLocalHostName();
	int index = -1;

	CTypedPtrList< CObList, CaDBObject* > lNew;
	BOOL bOk = (VNODE_QueryNode (lNew) == NOERROR)? TRUE: FALSE;
	while (bOk && !lNew.IsEmpty())
	{
		strVnodeName = _T("");
		CaNode* pNode = (CaNode*)lNew.RemoveHead();
		if (pNode->IsLocalNode()) // Node is local
		{
			cTempo = csLocHostName;
			if (!cTempo.IsEmpty())
			{
				strVnodeName.Format (_T("%s %s"), (LPCTSTR)pNode->GetName(), (LPCTSTR)cTempo);
			}
		}
		else // not a local node
		{
			if (csLocHostName.CompareNoCase(pNode->GetName()) == 0 && (pNode->GetLogin() == _T("*")))
				continue;

			strVnodeName = pNode->GetName();
		}
		//
		// Add node to the list:
		if (!strVnodeName.IsEmpty()) // avoid adding an undefined string !!!
		{
			index = m_listCtrl.AddString(strVnodeName);
			if (pNode->IsLocalNode())
			{
				if (cTempo.Compare((LPCTSTR)m_ReplicSvrData.RunNode) == 0)
					m_listCtrl.SetCurSel(index);
			}
			else
			{
				if (strVnodeName.Compare((LPCTSTR)m_ReplicSvrData.RunNode) == 0)
					m_listCtrl.SetCurSel(index);
			}
		}

		delete pNode;
	}

	EnableButton();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgReplicationRunNode::OnOK() 
{
	CString strNewRunNode;
	BOOL bOK = FALSE;
	int nSel = m_listCtrl.GetCurSel();
	if ( nSel != LB_ERR )
	{
		m_listCtrl.GetText(nSel, strNewRunNode );
		if (!strNewRunNode.IsEmpty())
		{
			bOK = IPM_ReplicationRunNode(m_pIpmDoc, m_ReplicSvrData, strNewRunNode);
		}
	}

	if (!bOK)
	{
		//
		// Fail to run new node:

		return;
	}

	CDialog::OnOK();
}

void CxDlgReplicationRunNode::EnableButton()
{
	int ItemSel;
	BOOL bEnable = FALSE;

	ItemSel = m_listCtrl.GetCurSel();
	if ( ItemSel != LB_ERR )
		bEnable = TRUE;
	m_cOK.EnableWindow (bEnable);
}

void CxDlgReplicationRunNode::OnSelchangeListVnode() 
{
	EnableButton();
}

void CxDlgReplicationRunNode::OnDestroy() 
{
	CDialog::OnDestroy();
}


BOOL CxDlgReplicationRunNode::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	// OLD vdba: IDHELP_IDD_REPLIC_RUNNODE  8019
	return APP_HelpInfo(m_hWnd, 8019);
}
