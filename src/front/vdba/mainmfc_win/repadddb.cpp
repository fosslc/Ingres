/*****************************************************************************
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
**
**  Project : Visual DBA
**
**  Source : repadddb.cpp
**
**  subdialog of the CDDS dialog box,  for "add special database for
**   horizontal partitionning"
**
**  Author :  Philippe Schalk
**
**  History after 22-May-2000:
**
**  05-Jun-2000 (schph01)
**    bug 101728 change the calculation to obtain the vnode name.
**
**  06-Jun-2000 (noifr01)
**    bug 99242 (DBCS) cleanup for DBCS compliance
**
**  05-May-2003 (schph01)
**    bug 110176 Add PushHelpId() and PopHelpId() functions to manage
**    the context sensitive help.
******************************************************************************/


#include "stdafx.h"
#include "mainmfc.h"
extern "C"
{
#include "dbaset.h"
#include "dbaginfo.h"
#include "domdata.h"
#include "dll.h"
#include "dlgres.h"
extern CTLDATA typeTypesTarget; // array define in replserv.c
}
#include "repadddb.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// interface to cddsv11.c
extern "C" BOOL MfcAddDatabaseCdds( LPREPCDDS lpcdds , int nHandle)
{
	CxDlgReplicConnectionDB dlg;
	dlg.m_nNodeHandle = nHandle;
	dlg.m_pStructVariableInfo = lpcdds;

	int iret = dlg.DoModal();
	if (iret == IDOK)
		return TRUE;
	else
		return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CxDlgReplicConnectionDB dialog


CxDlgReplicConnectionDB::CxDlgReplicConnectionDB(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgReplicConnectionDB::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgReplicConnectionDB)
	m_editsvrNumber = 0;
	//}}AFX_DATA_INIT
}


void CxDlgReplicConnectionDB::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgReplicConnectionDB)
	DDX_Control(pDX, IDC_ADD_SERVER_NUMBER, m_ctrlEditSvrNum);
	DDX_Control(pDX, IDC_ADD_TARGET_TYPE, m_ctrlTargetType);
	DDX_Control(pDX, IDC_SPIN_SVR_NUMBER, m_ctrlSpin_SvrNumber);
	DDX_Control(pDX, IDC_COMBO_DB, m_comboDatabase);
	DDX_Text(pDX, IDC_ADD_SERVER_NUMBER, m_editsvrNumber);
	DDV_MinMaxUInt(pDX, m_editsvrNumber, 0, 999);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgReplicConnectionDB, CDialog)
	//{{AFX_MSG_MAP(CxDlgReplicConnectionDB)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgReplicConnectionDB message handlers

BOOL CxDlgReplicConnectionDB::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_ctrlEditSvrNum.SetLimitText(3);
	m_ctrlSpin_SvrNumber.SetRange( 0, 999 );
	PushHelpId (IDD_REPL_CDDS_NEW);

	if (!FillComboBoxDB())
	{
		AfxMessageBox(VDBA_MfcResourceString(IDS_E_DB_COMBO_FILL));
		return TRUE;
	}
	if (!FillComboBoxSvrType())
	{
		AfxMessageBox(VDBA_MfcResourceString(IDS_E_SVR_COMBO_FILL));
		return TRUE;
	}
	m_ctrlTargetType.EnableWindow(FALSE);
	m_ctrlEditSvrNum.EnableWindow(FALSE);
	m_ctrlSpin_SvrNumber.EnableWindow(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgReplicConnectionDB::OnOK() 
{
	int iDx,iDx2,iDbNo,iTypeRepl;
	CString csDbName,csTempo,csVnode,csName;
	LPDD_CONNECTION lpconn;
	LPOBJECTLIST  lpOConnect,lpOL = m_pStructVariableInfo->connections;
	UpdateData(TRUE);

	iDx = m_ctrlTargetType.GetCurSel();
	if (iDx==LB_ERR)
		return;
	iTypeRepl = m_ctrlTargetType.GetItemData(iDx);

	iDx = m_comboDatabase.GetCurSel();
	if (iDx==LB_ERR)
		return;
	iDbNo = m_comboDatabase.GetItemData(iDx);

	while ( lpOL )
	{
		lpconn = (LPDD_CONNECTION) lpOL->lpObject;
		if ((UINT)lpconn->CDDSNo == m_pStructVariableInfo->cdds &&
			lpconn->dbNo == iDbNo)
		{
			lpconn->bElementCanDelete = FALSE;
			break;
		}
		lpconn = NULL;
		lpOL = (LPOBJECTLIST)lpOL->lpNext;
	}

	if (!lpconn) // Element not in the current list.
	{
		lpOConnect = (LPOBJECTLIST) AddListObjectTail(&(m_pStructVariableInfo->connections),
												sizeof(DD_CONNECTION));
		if (lpOConnect)
			lpconn = (LPDD_CONNECTION) lpOConnect->lpObject;
		else
		{
			VDBA_OutOfMemoryMessage();
			return;
		}
		memset((char*) lpconn,0,sizeof(DD_CONNECTION));
		lpconn->bElementCanDelete = TRUE;
		m_comboDatabase.GetLBText(iDx,csDbName);
		iDx  = csDbName.Find(_T(' ')); 
		iDx2 = csDbName.Find(_T(':'));
		if (iDx2 != -1 && iDx != -1)
		{
			// to obtain the vnode name and the database name from the string
			// to format like this: "111 VNODE_NAME::DATABASE_NAME"
			csVnode = csDbName.Mid(iDx+1,iDx2-(iDx+1));
			csName  = csDbName.Mid(iDx2+2);
		}
		else
		{
			csVnode.Empty();
			csName.Empty();
		}
		lstrcpy( (LPSTR)lpconn->szVNode ,csVnode );
		lstrcpy( (LPSTR)lpconn->szDBName,csName );
	}
	if (lpconn)
	{
		lpconn->nTypeRepl              = iTypeRepl;
		lpconn->dbNo                   = iDbNo;
		lpconn->CDDSNo                 = m_pStructVariableInfo->cdds;
		lpconn->nServerNo              = m_editsvrNumber;
		lpconn->bIsInCDDS              = TRUE;
		lpconn->bConnectionWithoutPath = TRUE;
		ASSERT(lstrlen((LPSTR)lpconn->szVNode ));
		ASSERT(lstrlen((LPSTR)lpconn->szDBName));
	}
	CDialog::OnOK();
}

BOOL CxDlgReplicConnectionDB::FillComboBoxDB()
{
	LPOBJECTLIST  lpOL;
	LPDD_CONNECTION lpconn;
	LPDD_CONNECTION lpconn1 ;

	m_comboDatabase.ResetContent();

	for ( lpOL = (LPOBJECTLIST)m_pStructVariableInfo->connections; lpOL ; lpOL = (LPOBJECTLIST)lpOL->lpNext)
	{
		LPOBJECTLIST  lpOL1;
		CString Tempo;
		int index1,index2;
		BOOL bAlreadyInList=FALSE;
		lpconn  =(LPDD_CONNECTION)lpOL ->lpObject;
		for ( lpOL1 = (LPOBJECTLIST)m_pStructVariableInfo->connections; lpOL1!=lpOL ; lpOL1 = (LPOBJECTLIST)lpOL1->lpNext)
		{
			lpconn1 =(LPDD_CONNECTION)lpOL1->lpObject;
			if (!x_stricmp((LPSTR)lpconn->szVNode, (LPSTR)lpconn1->szVNode) &&
				!x_stricmp((LPSTR)lpconn->szDBName,(LPSTR)lpconn1->szDBName))
			{
				bAlreadyInList=TRUE;
				break;
			}
		}
		if (bAlreadyInList)
			continue;
		Tempo.Format("%d %s::%s",lpconn->dbNo,lpconn->szVNode,lpconn->szDBName);
		index1 = m_comboDatabase.AddString(Tempo);
		if (index1 == CB_ERR || index1 == CB_ERRSPACE)
			return FALSE;
		index2 = m_comboDatabase.SetItemData(index1,lpconn->dbNo);
		if (index2 == CB_ERR)
			return FALSE;
	}
	m_comboDatabase.SetCurSel(0);
	return TRUE;
}

BOOL CxDlgReplicConnectionDB::FillComboBoxSvrType()
{
	int iret,iIndex;
	int i = 0;
	LPCTLDATA lpData;

	m_ctrlTargetType.ResetContent();
	lpData = &typeTypesTarget;
	while (lpData[i].nId != -1)
	{
		iIndex = m_ctrlTargetType.AddString(VDBA_MfcResourceString(lpData[i].nStringId));
		if (iIndex == CB_ERR || iIndex == CB_ERRSPACE)
			return FALSE;
		iret = m_ctrlTargetType.SetItemData(iIndex ,lpData[i].nId);
		if (iret == CB_ERR)
			return FALSE;
		i++;
	}
	m_ctrlTargetType.SetCurSel(0);
	return TRUE;
}


void CxDlgReplicConnectionDB::OnDestroy() 
{
	CDialog::OnDestroy();
	PopHelpId();
}
