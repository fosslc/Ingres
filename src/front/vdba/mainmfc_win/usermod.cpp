/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : usermod.cpp, Implementation file
**
**  Project  : Ingres II/ VDBA.
**
**  Author   : Schalk Philippe
**
**  Purpose  : Dialog and Class used for manage the utility "usermod" on databases and
**             tables
**
**
**  11-Apr-2001 (schph01)
**      SIR 103292 Created
**  12-Jun-2002 (uk$so01)
**     BUG #107996, Explicitly cast the CString into (LPCTSTR) when used
**     as argument list in the .Format (strFormatString, ...) because
**     in some platforms, the compiler C++ does not automatically cast the CString
**     into (LPCTSTR).
**  02-feb-2004 (somsa01)
**     Updated to "typecast" LPUCHAR into CString before concatenation.
*/

#include "stdafx.h"
#include "rcdepend.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern "C" 
{
#include "dba.h"
#include "dbaset.h"
#include "dbadlg1.h"
#include "domdata.h"
#include "dbaginfo.h"
}
#include "usermod.h"

/////////////////////////////////////////////////////////////////////////////
// CxDlgUserMod dialog


CxDlgUserMod::CxDlgUserMod(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgUserMod::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgUserMod)
	//}}AFX_DATA_INIT
}


void CxDlgUserMod::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgUserMod)
	DDX_Control(pDX, IDC_EDIT_TBLNAME, m_ctrlTblName);
	DDX_Control(pDX, IDC_BUTTON_TABLES, m_ctrlButtonTables);
	DDX_Control(pDX, IDC_CHECK_SPECIF_TABLES, m_CheckSpecifTbl);
	DDX_Control(pDX, IDC_STATIC_TBLNAME, m_ctrlStaticTblName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgUserMod, CDialog)
	//{{AFX_MSG_MAP(CxDlgUserMod)
	ON_BN_CLICKED(IDC_BUTTON_TABLES, OnButtonTables)
	ON_BN_CLICKED(IDC_CHECK_SPECIF_TABLES, OnCheckSpecifTables)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgUserMod message handlers

BOOL CxDlgUserMod::OnInitDialog()
{
	CDialog::OnInitDialog();
	LPUCHAR parent [MAXPLEVEL];
	TCHAR   buffer [MAXOBJECTNAME];
	CString csFormatTitle,csCaption;

	//
	// Make up title:
	GetWindowText(csFormatTitle);
	parent [0] = (LPUCHAR)(LPTSTR)(LPCTSTR)m_csDBName;
	parent [1] = NULL;
	csCaption.Format(csFormatTitle, (LPTSTR)GetVirtNodeName(m_nNodeHandle), (LPCTSTR)m_csDBName);

	if (!m_csTableName.IsEmpty() && !m_csTableOwner.IsEmpty())
	{
		GetExactDisplayString (m_csTableName, m_csTableOwner, OT_TABLE, parent, buffer);
		csCaption += buffer;
		bChooseTableEnable = FALSE;
	}
	else
		bChooseTableEnable = TRUE;
	SetWindowText(csCaption);

	if ( bChooseTableEnable )
		m_ctrlButtonTables.EnableWindow(FALSE); // Grayed "Tables" button
	else
	{
		m_ctrlButtonTables.ShowWindow(SW_HIDE); // hide button "tables"
		m_CheckSpecifTbl.ShowWindow(SW_HIDE);   // hide checkBox "specify tables"

		m_ctrlStaticTblName.ShowWindow(SW_SHOW); //Show static "table"
		m_ctrlStaticTblName.EnableWindow(TRUE) ; //Ungrayed static "table"
		m_ctrlTblName.ShowWindow(SW_SHOW);       //Show edit controle with the table name.
		m_ctrlTblName.EnableWindow(FALSE);       //Grayed edit controle with the table name.
		m_ctrlTblName.SetWindowText(m_csTableName);
	}

	m_lpTblParam.lpTable = NULL;

	PushHelpId(IDD_USERMOD);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgUserMod::OnButtonTables()
{
	int iret;
	m_lpTblParam.bRefuseTblWithDupName = TRUE;
	_tcscpy((LPTSTR)m_lpTblParam.DBName, (LPCTSTR)m_csDBName);
	iret = DlgAuditDBTable (m_hWnd , &m_lpTblParam);
}

void CxDlgUserMod::OnCheckSpecifTables()
{
	if (IsDlgButtonChecked(IDC_CHECK_SPECIF_TABLES) == BST_CHECKED)
		m_ctrlButtonTables.EnableWindow(TRUE); // unGrayed "Tables" button
	else
		m_ctrlButtonTables.EnableWindow(FALSE); // Grayed "Tables" button
}

void CxDlgUserMod::OnOK() 
{
	CString csCommandLine;

	GenerateUsermodSyntax(csCommandLine);

	ExecuteRemoteCommand(csCommandLine);

	CDialog::OnOK();
}

void CxDlgUserMod::OnDestroy() 
{
	CDialog::OnDestroy();

	if (m_lpTblParam.lpTable)
	{
		FreeObjectList(m_lpTblParam.lpTable);
		m_lpTblParam.lpTable = NULL;
	}
    PopHelpId();
}

void CxDlgUserMod::ExecuteRemoteCommand(LPCTSTR csCommandLine)
{
	int hnode;
	CString csTempo,csNodeName,csTitle;

	LPUCHAR vnodeName = (LPUCHAR)GetVirtNodeName ( m_nNodeHandle);
	csNodeName = vnodeName;
	hnode = OpenNodeStruct  ((LPUCHAR)(LPCTSTR)csNodeName);
	if (hnode<0)
	{
		CString strMsg = VDBA_MfcResourceString (IDS_MAX_NB_CONNECT);//_T("Maximum number of connections has been reached"
		strMsg += CString(VDBA_MfcResourceString (IDS_E_USERMOD));            // " - Cannot launch 'usermod' command.");
		AfxMessageBox (strMsg);
		return;
	}
	// Temporary for activate a session
	UCHAR buf[MAXOBJECTNAME];
	DOMGetFirstObject (hnode, OT_DATABASE, 0, NULL, FALSE, NULL, buf, NULL, NULL);
	csTitle.Format(IDS_T_USERMOD, (LPCTSTR)csNodeName, (LPCTSTR)m_csDBName);
	execrmcmd1( (char *)(LPCTSTR)csNodeName,
	            (char *)(LPCTSTR)csCommandLine,
	            (char *)(LPCTSTR)csTitle,
	            TRUE);

	CloseNodeStruct(hnode,FALSE);
}

BOOL CxDlgUserMod::GenerateUsermodSyntax(CString& csCommandSyntax)
{
	TCHAR szusernamebuf[MAXOBJECTNAME];
	LPOBJECTLIST lpObjList = m_lpTblParam.lpTable;

	csCommandSyntax = _T("usermod ");
	csCommandSyntax += m_csDBName;
	if ( IsStarDatabase(m_nNodeHandle, (LPUCHAR)(LPCTSTR)m_csDBName) )
	{
		csCommandSyntax += _T("/star");
	}


	// -u flag
	DBAGetUserName ((LPUCHAR)(LPCTSTR)GetVirtNodeName(m_nNodeHandle),(LPUCHAR) szusernamebuf);
	csCommandSyntax += _T(" -u");
	csCommandSyntax += szusernamebuf;

	// tables list or current selected table
	if (bChooseTableEnable)
	{
		if (IsDlgButtonChecked(IDC_CHECK_SPECIF_TABLES) == BST_CHECKED)
		{
			while (lpObjList)
			{
				csCommandSyntax += _T(" ");
				csCommandSyntax += CString(StringWithoutOwner((LPUCHAR)lpObjList->lpObject));
				lpObjList = (LPOBJECTLIST)lpObjList->lpNext;
			}
		}
	}
	else
	{
		csCommandSyntax += _T(" ");
		csCommandSyntax += m_csTableName;
	}

	// -noint flag
	if (IsDlgButtonChecked(IDC_CHECK_NOINTERRUPT) == BST_CHECKED)
		csCommandSyntax += _T(" -noint");

	return TRUE;
}
