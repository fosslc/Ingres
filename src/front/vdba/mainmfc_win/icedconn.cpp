/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**  Name: IceDConn.cpp : implementation file
**
**  Description:
**
**  History:
**  02-Mar-2000 (schph01)
**    Bug #100688 replace csDisplay by nodedata.NodeName because csDisplay
**    is change on the fly when it is the local vnode"(local)".
**  26-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
**    24-Jun-2002 (hanje04)
**      Cast all CStrings being passed to other functions or methods using %s
**      with (LPCTSTR) and made calls more readable on UNIX.
**  22-Sep-2002 (schph01)
**    Bug 108645 call the new function GetLocalVnodeName ( )
**    (instead GCHostname()) to retrieve the "gcn.local_vnode" parameter in
**    config.dat file.
**  23-Sep-2003 (schph01)
**    Bug 110951 The database name field can be edited by the user to add the
**    Server class or the Gateway.
**  24-May-2005 (komve01)
**    Bug 114553/Issue#14110400 The database connection name field disappears
**    when a different VNode is selected. Hence added UpdateData(TRUE) to
**    retain the values.
**  15-Jul-2005 (komve01)
**    Bug 114553/Issue#14110400
**    UpdateData(TRUE) was updating all the values and also the node name this
**    was not allowing a change of node name. Made it work only for that control
**
*****************************************************************************/

#include "stdafx.h"
#include "mainmfc.h"
#include "icedconn.h"
#include "dgerrh.h"
#include "extccall.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// interface to dom.c
extern "C" BOOL MfcDlgCreateIceDbConnection( LPDBCONNECTIONDATA lpCreateDbConnect ,int nHnode)
{
	CxDlgIceDbConnection dlg;
	dlg.m_pStructDbConnect = lpCreateDbConnect;
	dlg.m_csCurrentVnodeName = GetVirtNodeName (nHnode);
	dlg.m_nHnode = nHnode;
	int iret = dlg.DoModal();
	if (iret == IDOK)
		return TRUE;
	else
		return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CxDlgIceDbConnection dialog


CxDlgIceDbConnection::CxDlgIceDbConnection(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgIceDbConnection::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgIceDbConnection)
	m_csDatabase = _T("");
	m_csUser = _T("");
	m_csComment = _T("");
	m_csConnectName = _T("");
	m_csNode = _T("");
	//}}AFX_DATA_INIT
}


void CxDlgIceDbConnection::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgIceDbConnection)
	DDX_Control(pDX, IDOK, m_ctrlOK);
	DDX_Control(pDX, IDC_ICE_NAME, m_ctrledName);
	DDX_Control(pDX, IDC_ICE_DBC_COMMENT, m_ctrledComment);
	DDX_Control(pDX, IDC_ICE_DB_USER, m_ctrlcbDbUser);
	DDX_Control(pDX, IDC_NODE, m_ctrlcbNode);
	DDX_Control(pDX, IDC_DATABASE, m_ctrlcbDatabase);
	DDX_CBString(pDX, IDC_DATABASE, m_csDatabase);
	DDX_CBString(pDX, IDC_ICE_DB_USER, m_csUser);
	DDX_Text(pDX, IDC_ICE_DBC_COMMENT, m_csComment);
	DDX_Text(pDX, IDC_ICE_NAME, m_csConnectName);
	DDX_CBString(pDX, IDC_NODE, m_csNode);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgIceDbConnection, CDialog)
	//{{AFX_MSG_MAP(CxDlgIceDbConnection)
	ON_EN_CHANGE(IDC_ICE_NAME, OnChangeIceName)
	ON_CBN_SELCHANGE(IDC_NODE, OnSelchangeNode)
	ON_CBN_SELCHANGE(IDC_ICE_DB_USER, OnSelchangeIceDbUser)
	ON_CBN_SELCHANGE(IDC_DATABASE, OnSelchangeDatabase)
	ON_WM_DESTROY()
	ON_CBN_KILLFOCUS(IDC_DATABASE, OnKillfocusDatabase)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgIceDbConnection message handlers

BOOL CxDlgIceDbConnection::OnInitDialog() 
{
	CString csTitle,csTempo;
	CDialog::OnInitDialog();
	m_ctrledName.LimitText(sizeof(m_pStructDbConnect->ConnectionName)-1);
	m_ctrledComment.LimitText(sizeof(m_pStructDbConnect->Comment)-1);
	m_ctrlcbDatabase.LimitText(sizeof(m_pStructDbConnect->DBName)-1);

	m_csStaticInfo.LoadString(IDS_ICE_STATIC_DBCONNECTION_INFO);

	if (m_pStructDbConnect->bAlter)
	{
		CString csDataBaseName;
		int index;
		csDataBaseName=m_pStructDbConnect->DBName;
		index = csDataBaseName.Find("::");
		if (index != -1)
		{
			m_csDbName = csDataBaseName.Mid(index+2);
			m_csVnodeName = csDataBaseName.Left(index);
		}
		else
		{
			m_csVnodeName.Empty();
			m_csDbName = csDataBaseName;
		}
		m_csDatabase    = m_csDbName;
		m_csNode        = m_csVnodeName;
		m_csUser        = m_pStructDbConnect->DBUsr.UserAlias;
		m_csConnectName = m_pStructDbConnect->ConnectionName;
		m_csComment     = m_pStructDbConnect->Comment;
		m_ctrledName.EnableWindow(FALSE);
		//"Alter Ice Database Connection %s on %s"
		csTitle.Format(IDS_ICE_ALTER_DATABASE_CONNECT_TITLE,
				(LPCTSTR)m_pStructDbConnect->ConnectionName,
				(LPCTSTR)m_csCurrentVnodeName);
		PushHelpId (HELPID_ALTER_IDD_ICE_DATABASE_CONNECT);
	}
	else
	{
		GetWindowText(csTempo);
		csTitle.Format(csTempo,m_csCurrentVnodeName);
		m_csVnodeName.Empty();
		m_csDbName.Empty();
		PushHelpId (HELPID_IDD_ICE_DATABASE_CONNECT);
	}
	SetWindowText(csTitle);
	FillVnodeName();
	if (m_pStructDbConnect->bAlter)
		FillDatabasesOfVnode(FALSE);
	else
		FillDatabasesOfVnode (TRUE);
	FillDatabasesUsers ();

	UpdateData(FALSE);
	EnableDisableOK();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgIceDbConnection::OnOK() 
{
	CString csMsg;
	DBCONNECTIONDATA StructDbConnectNew;

	memset(&StructDbConnectNew,0,sizeof(DBCONNECTIONDATA));

	if ( FillstructureFromCtrl(&StructDbConnectNew) == FALSE)
		return;

	if (!m_pStructDbConnect->bAlter)
	{
		if (ICEAddobject((LPUCHAR)(LPCTSTR)m_csCurrentVnodeName ,OT_ICE_DBCONNECTION,&StructDbConnectNew ) == RES_ERR)
		{
			csMsg.LoadString(IDS_E_ICE_ADD_DATABASE_CONNECTION_FAILED);
			MessageWithHistoryButton(m_hWnd,csMsg);
			return;
		}
	}
	else
	{
		if ( ICEAlterobject((LPUCHAR)(LPCTSTR)m_csCurrentVnodeName,OT_ICE_DBCONNECTION, m_pStructDbConnect, &StructDbConnectNew ) == RES_ERR )
		{
			csMsg.LoadString(IDS_E_ICE_ALTER_DATABASE_CONNECTION_FAILED);
			MessageWithHistoryButton(m_hWnd,csMsg);
			return;
		}
	}
	CDialog::OnOK();
}
BOOL CxDlgIceDbConnection::FillstructureFromCtrl(LPDBCONNECTIONDATA pbConnectNew)
{
	int index;
	UpdateData(TRUE);
	
	if ( m_csNode.IsEmpty() || m_csDatabase.IsEmpty() ||
		 m_csConnectName.IsEmpty() || m_csUser.IsEmpty() )
		return FALSE;

	lstrcpy((char *)pbConnectNew->ConnectionName,m_csConnectName);
	lstrcpy((char *)pbConnectNew->DBUsr.UserAlias,m_csUser);
	lstrcpy((char *)pbConnectNew->Comment,m_csComment);

	index = m_csNode.Find(m_csStaticInfo);
	if (index == -1)
	{
		lstrcpy((char *)pbConnectNew->DBName,(LPTSTR)(LPCTSTR)m_csNode);
		lstrcat((char *)pbConnectNew->DBName,_T("::"));
		lstrcat((char *)pbConnectNew->DBName,(LPTSTR)(LPCTSTR)m_csDatabase);
	}
	else
		lstrcpy((char *)pbConnectNew->DBName,(LPTSTR)(LPCTSTR)m_csDatabase);

	return TRUE;
}
void CxDlgIceDbConnection::FillVnodeName ()
{
	CString csDisplay;
	NODEDATAMIN nodedata;
	int ires,nIndex;
	char BaseHostName[MAXOBJECTNAME];

	m_ctrlcbNode.ResetContent();

	ires=GetFirstMonInfo(0,0,NULL,OT_NODE,(void * )&nodedata,NULL);
	while (ires==RES_SUCCESS)
	{
		{ // only Ingres Nodes
			if (nodedata.bIsLocal == TRUE)
			{
				memset (BaseHostName,'\0',MAXOBJECTNAME);
				GetLocalVnodeName (BaseHostName,MAXOBJECTNAME);
				if (BaseHostName[0] != '\0')
					csDisplay = BaseHostName;
			}
			else
				csDisplay = nodedata.NodeName;
			
			if (m_csCurrentVnodeName.Compare((LPTSTR)nodedata.NodeName)==0)
			{
				csDisplay = csDisplay + m_csStaticInfo;
			}
			nIndex = m_ctrlcbNode.AddString (csDisplay);
			if (m_csVnodeName.IsEmpty())
			{
				if (nIndex!=CB_ERR && m_csCurrentVnodeName.Compare((char *)nodedata.NodeName)==0)
					m_ctrlcbNode.SetCurSel(nIndex);
			}
			else
			{
				if (nIndex!=CB_ERR && m_csVnodeName.Compare((char *)nodedata.NodeName)==0)
					m_ctrlcbNode.SetCurSel(nIndex);
			}
		}
		ires=GetNextMonInfo((void * )&nodedata);
	}
}

void CxDlgIceDbConnection::FillDatabasesOfVnode ( BOOL bInitWithFirstItem )
{
	int  hdl, ires,nIndex;
	UCHAR buf [MAXOBJECTNAME];
	UCHAR extradata[EXTRADATASIZE];
	CString csCurVnode;
	CString csMsg;

	nIndex = m_ctrlcbNode.GetCurSel();

	if (nIndex == CB_ERR)
		return;
	m_ctrlcbNode.GetLBText( nIndex, csCurVnode );
	nIndex = csCurVnode.Find(_T(" (ICE Server Node)"));
	if (nIndex != -1)
		csCurVnode = csCurVnode.Left(nIndex);

	m_ctrlcbDatabase.ResetContent();
	hdl  = OpenNodeStruct ((LPUCHAR)(LPCTSTR)csCurVnode);
	if (hdl<0)
	{
		CString csCurrentMsg;
		csMsg.LoadString(IDS_MAX_NB_CONNECT);
		csCurrentMsg.LoadString(IDS_E_ICE_FILL_COMBO_DATABASE);
		csMsg = csMsg + csCurrentMsg;
		MessageWithHistoryButton(m_hWnd,csMsg);
		return;
	}

	ires = DOMGetFirstObject (hdl, OT_DATABASE, 0, NULL, FALSE, NULL, buf, NULL, extradata);
	while (ires == RES_SUCCESS)
	{
		if ( getint(extradata+STEPSMALLOBJ) == DBTYPE_REGULAR)
			m_ctrlcbDatabase.AddString ((char *)buf);
		ires  = DOMGetNextObject (buf, NULL, extradata);
	}

	if (ires != RES_SUCCESS && ires != RES_ENDOFDATA)
	{
		csMsg.LoadString(IDS_E_ICE_FILL_COMBO_DATABASE);
		MessageWithHistoryButton(m_hWnd,csMsg);
	}

	CloseNodeStruct (hdl, FALSE);
	if(m_pStructDbConnect->bAlter && !bInitWithFirstItem)
	{
		nIndex = m_ctrlcbDatabase.FindString(-1,m_csDbName);
		if (nIndex != CB_ERR)
			m_ctrlcbDatabase.SetCurSel(nIndex);
		else
		{
				int nNbIndex = m_csDatabase.Find(_T('/'));
				if (nNbIndex == -1)
					m_csDatabase.Empty();
				m_ctrlcbDatabase.SetCurSel(-1);
		}
	}
	else
	{
		CString csTempo;
		m_ctrlcbDatabase.GetLBText(0,csTempo);
		if (!csTempo.IsEmpty())
			m_csDatabase = csTempo;
	}
}

void CxDlgIceDbConnection::FillDatabasesUsers ()
{
	int ires;
	UCHAR buf [MAXOBJECTNAME];
	UCHAR bufOwner[MAXOBJECTNAME];
	UCHAR extradata[EXTRADATASIZE];
	CString csMsg;

	m_ctrlcbDbUser.ResetContent();

	ires = DOMGetFirstObject (	m_nHnode,
								OT_ICE_DBUSER, 0, NULL,
								FALSE, NULL, buf, bufOwner, extradata);
	while (ires == RES_SUCCESS)
	{
		m_ctrlcbDbUser.AddString ((char *)buf);
		ires  = DOMGetNextObject (buf, NULL, extradata);
	}

	if (ires != RES_SUCCESS && ires != RES_ENDOFDATA)
	{
		csMsg.LoadString(IDS_E_ICE_FILL_COMBO_DB_USER);
		MessageWithHistoryButton(m_hWnd,csMsg);
	}

	if (!m_pStructDbConnect->bAlter)
		m_ctrlcbDbUser.SetCurSel(0);
}

void CxDlgIceDbConnection::EnableDisableOK ()
{
	BOOL bValidDbName;
	CString csTemp;

	int nIndex = m_ctrlcbDatabase.GetCurSel();
	if (nIndex!= CB_ERR)
		m_ctrlcbDatabase.GetLBText(nIndex,csTemp);
	else
		m_ctrlcbDatabase.GetWindowText(csTemp);

	if (csTemp.IsEmpty())
		bValidDbName = FALSE;
	else
		bValidDbName = TRUE;

	if (m_ctrledName.LineLength()    == 0 ||
		m_ctrlcbDbUser.GetCurSel()   == CB_ERR ||
		m_ctrlcbNode.GetCurSel()     == CB_ERR ||
		!bValidDbName )
		m_ctrlOK.EnableWindow(FALSE);
	else
		m_ctrlOK.EnableWindow(TRUE);

}

void CxDlgIceDbConnection::OnChangeIceName() 
{
	m_ctrledName.UpdateData(TRUE);//Collect the values before we go anywhere
	EnableDisableOK ();
}

void CxDlgIceDbConnection::OnSelchangeNode() 
{
	FillDatabasesOfVnode(TRUE);
	FillDatabasesUsers ();
	UpdateData(FALSE);
	EnableDisableOK();
}

void CxDlgIceDbConnection::OnSelchangeIceDbUser() 
{
	EnableDisableOK();
}

void CxDlgIceDbConnection::OnSelchangeDatabase() 
{
	EnableDisableOK();
}

void CxDlgIceDbConnection::OnDestroy() 
{
	CDialog::OnDestroy();
	PopHelpId();
}

void CxDlgIceDbConnection::OnKillfocusDatabase() 
{
	CString csTemp;
	int nInd1, nInd2;

	m_ctrlcbDatabase.GetWindowText(csTemp);
	if (csTemp.IsEmpty())
		return;
	nInd1 = csTemp.ReverseFind(_T('/'));
	if ( nInd1 == -1)
	{
		nInd2 = m_ctrlcbDatabase.FindString(-1,csTemp);
		if (nInd2 == -1)
		{
			AfxMessageBox(IDS_E_ICE_DATABASE_NOT_FOUND);
			m_ctrlcbDatabase.SetFocus();
		}
	}
}
