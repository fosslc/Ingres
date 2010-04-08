/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Project  : Visual DBA
**
**  Source   : duplicdb.cpp: implementation file.
**
**  Created by: Schalk Philippe
**
**  Purpose  : Popup Dialog Box for Duplicate database.
**             Generating remote command (copy a database with "relocatedb").
**
**  history after 01-May-2002:
**
**  14-May-2002 (uk$so01)
**   (bug 107773) don't refuse any more international characters (such as
**   accented characters) for the database name
**  12-Jun-2002 (uk$so01)
**     BUG #107996, Explicitly cast the CString into (LPCTSTR) when used
**     as argument list in the .Format (strFormatString, ...) because
**     in some platforms, the compiler C++ does not automatically cast the CString
**     into (LPCTSTR).
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "duplicdb.h"
extern "C"
{
#include "dba.h"
#include "dbaset.h"
#include "main.h"
#include "domdata.h"
#include "getdata.h"
}
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CxDlgDuplicateDb dialog


CxDlgDuplicateDb::CxDlgDuplicateDb(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgDuplicateDb::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgDuplicateDb)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CxDlgDuplicateDb::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgDuplicateDb)
	DDX_Control(pDX, IDOK, m_ctrlOK);
	DDX_Control(pDX, IDC_ASSIGN_LOC, m_ctrlAssignLoc);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_NEW_DB_NAME, m_ctrlDbName);
}


BEGIN_MESSAGE_MAP(CxDlgDuplicateDb, CDialog)
	//{{AFX_MSG_MAP(CxDlgDuplicateDb)
	ON_EN_CHANGE(IDC_NEW_DB_NAME, OnChangeNewDbName)
	ON_BN_CLICKED(IDC_ASSIGN_LOC, OnAssignLoc)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgDuplicateDb message handlers

void CxDlgDuplicateDb::OnOK() 
{
	ExecuteRemoteCommand();
	
	CDialog::OnOK();
}

void CxDlgDuplicateDb::OnChangeNewDbName() 
{
	EnableDisableOK();
}

void CxDlgDuplicateDb::OnAssignLoc() 
{
	if (m_ctrlAssignLoc.GetCheck() == 1)
		m_cListCtrl.ShowWindow(TRUE);
	else
		m_cListCtrl.ShowWindow(FALSE);
}

BOOL CxDlgDuplicateDb::OnInitDialog() 
{
	CString csTitle;
	CString csFormatString;
	CDialog::OnInitDialog();
	// Generate title
	GetWindowText(csFormatString);
	csTitle.Format(csFormatString, (LPCTSTR)m_csCurrentDatabaseName);
	SetWindowText(csTitle);

	m_ctrlDbName.SetLimitText(MAXOBJECTNAMENOSCHEMA-1);

	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST_LOCATION, this));
	LONG style = GetWindowLong (m_cListCtrl.m_hWnd, GWL_STYLE);
	style |= LVS_SHOWSELALWAYS;
	SetWindowLong (m_cListCtrl.m_hWnd, GWL_STYLE, style);
	m_ImageList.Create(1, 20, TRUE, 1, 0);
	m_cListCtrl.SetImageList (&m_ImageList, LVSIL_SMALL);
	m_cListCtrl.SetFullRowSelected (TRUE);

	LV_COLUMN lvcolumn;
	LSCTRLHEADERPARAMS lsp[2] =
	{
		{_T(""), 120, LVCFMT_LEFT, FALSE},
		{_T(""),     120, LVCFMT_LEFT, FALSE}
	};
	lstrcpy(lsp[0].m_tchszHeader,VDBA_MfcResourceString(IDS_TC_INIT_LOC)); //_T("Initial Location")
	lstrcpy(lsp[1].m_tchszHeader,VDBA_MfcResourceString(IDS_TC_NEW_LOC));  //_T("New Location")

	memset (&lvcolumn, 0, sizeof (LV_COLUMN));
	lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
	for (int i=0; i<2; i++)
	{
		lvcolumn.fmt      = lsp[i].m_fmt;
		lvcolumn.pszText  = lsp[i].m_tchszHeader;
		lvcolumn.iSubItem = i;
		lvcolumn.cx       = lsp[i].m_cxWidth;
		m_cListCtrl.InsertColumn (i, &lvcolumn); 
	}
	
	FillLocation();
	EnableDisableOK();
	PushHelpId (IDD_DUPLICATE_DATABASE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
void CxDlgDuplicateDb::FillLocation()
{
	DATABASEPARAMS DbParams;
	int dummySesHndl,nItem,nIdx;

	memset (&DbParams, 0, sizeof (DbParams));
	lstrcpy ((LPTSTR)DbParams.objectname, (LPCTSTR)m_csCurrentDatabaseName);
	DbParams.DbType = m_DbType;

	int iResult = GetDetailInfo ((LPUCHAR)GetVirtNodeName(m_nNodeHandle),
								 OT_DATABASE,
								 &DbParams,
								 FALSE,
								 &dummySesHndl);
	if (iResult != RES_SUCCESS) {
		ASSERT (FALSE);
		return;
	}

	LPOBJECTLIST lplist = DbParams.lpDBExtensions;
	while (lplist)
	{
		LPDB_EXTENSIONS lpObjTemp = (LPDB_EXTENSIONS)lplist->lpObject;
		nItem = m_cListCtrl.GetItemCount();
		nIdx = m_cListCtrl.InsertItem( nItem, (LPTSTR)lpObjTemp->lname );
		if (nIdx!=-1)
			m_cListCtrl.SetItemText( nIdx,1, (LPTSTR)lpObjTemp->lname );
		lplist = (LPOBJECTLIST) lplist->lpNext;
	}

	FreeAttachedPointers (&DbParams, OT_DATABASE);
}

void CxDlgDuplicateDb::EnableDisableOK()
{
	if (m_ctrlDbName.LineLength())
		m_ctrlOK.EnableWindow(TRUE);
	else
		m_ctrlOK.EnableWindow(FALSE);
}


void CxDlgDuplicateDb::OnDestroy() 
{
	CDialog::OnDestroy();
	PopHelpId();
}

void CxDlgDuplicateDb::ExecuteRemoteCommand()
{
	CString csCommandLine,csTempo,csNodeName,csTitle;
	int hnode;
	csNodeName = GetVirtNodeName (m_nNodeHandle);

	m_ctrlDbName.GetWindowText(csTempo);
	if (csTempo.IsEmpty())
		return;
	hnode = OpenNodeStruct  ((LPUCHAR)(LPCTSTR)csNodeName);
	if (hnode<0)
	{
		CString strMsg =  VDBA_MfcResourceString (IDS_MAX_NB_CONNECT); //_T("Maximum number of connections has been reached"
		strMsg += VDBA_MfcResourceString (IDS_E_ALTER_DB);			   //" - Cannot alter database.");
		AfxMessageBox (strMsg);
		return;
	}
	// Temporary for activate a session
	UCHAR buf[MAXOBJECTNAME];
	DOMGetFirstObject (hnode, OT_DATABASE, 0, NULL, FALSE, NULL, buf, NULL, NULL);
	if(m_ctrlAssignLoc.GetCheck() == 0) //Button state is unchecked.
	{
		csTitle.Format(IDS_TF_DUPLICATE_DB, (LPCTSTR)m_csCurrentDatabaseName);//"Duplicate database %s"
		csCommandLine.Format(_T("relocatedb %s -new_database=%s"), (LPCTSTR)m_csCurrentDatabaseName, (LPCTSTR)csTempo);
		execrmcmd1((char *)(LPCTSTR)csNodeName,
					(char *)(LPCTSTR)csCommandLine,
					(char *)(LPCTSTR)csTitle,
					TRUE);
	}
	else
	{
		int nCount,i;
		CString csInitLoc;
		CString csNewLoc;
		nCount = m_cListCtrl.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			if (!csInitLoc.IsEmpty())
				csInitLoc+=_T(",");
			if (!csNewLoc.IsEmpty())
				csNewLoc+=_T(",");
			csInitLoc += m_cListCtrl.GetItemText (i, 0);
			csNewLoc  += m_cListCtrl.GetItemText (i, 1);
		}
		if (csInitLoc.IsEmpty() || csNewLoc.IsEmpty())
		{
			CloseNodeStruct(hnode,FALSE);
			return;
		}
		csTitle.Format(IDS_TF_DUPLICATE_DB, (LPCTSTR)m_csCurrentDatabaseName);//"Duplicate database %s"
		csCommandLine.Format(
			_T("relocatedb %s -new_database=%s -location=%s -new_location=%s"), 
			(LPCTSTR)m_csCurrentDatabaseName, 
			(LPCTSTR)csTempo ,
			(LPCTSTR)csInitLoc,
			(LPCTSTR)csNewLoc);
		execrmcmd1(	(char *)(LPCTSTR)csNodeName,
					(char *)(LPCTSTR)csCommandLine,
					(char *)(LPCTSTR)csTitle,
					TRUE);
	}

	CloseNodeStruct(hnode,FALSE);
}
