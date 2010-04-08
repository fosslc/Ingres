/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : xcopydb.cpp, Implementation file
**
**  Project  : Ingres II/ VDBA.
**
**  Author   : Schalk Philippe
**
**  Purpose  : Dialog and Class used for manage the new version of utility
**             "copydb"(INGRES 2.6 only) on databases and tables and views.
**
**
**  25-Apr-2001 (schph01)
**      SIR 103299 Created
**  12-Jun-2002 (uk$so01)
**     BUG #107996, Explicitly cast the CString into (LPCTSTR) when used
**     as argument list in the .Format (strFormatString, ...) because
**     in some platforms, the compiler C++ does not automatically cast the CString
**     into (LPCTSTR).
**  17-Apr-2003 (schph01)
**     SIR 107523 Add Sequences in list option.
**  02-May-2003 (uk$so01)
**     BUG #110167, Disable the OK button if no statement is selected.
**  02-May-2003 (uk$so01)
**     BUG #110167, added missing carriage return at the end of file, because
**     the lack of this character was resulting in propagation problems
**     across codelines
**  23-Jan-2004 (schph01)
**     (sir 104378) detect version 3 of Ingres, for managing
**     new features provided in this version. replaced references
**     to 2.65 with refereces to 3  in #define definitions for
**     better readability in the future
**  02-feb-2004 (somsa01)
**     Updated CxDlgCopyDb::GenerateCopyDBSyntax() to "typecast" LPUCHAR
**     into CString before concatenation.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "xcopydb.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CxDlgCopyDb dialog
void CxDlgCopyDb::InitializeLstOptions()
{
	try
	{
		m_listOptions.Add(new CaCopyDBOptionList(IDS_WITH_TABLES    ,_T("-with_tables")  ));
		m_listOptions.Add(new CaCopyDBOptionList(IDS_WITH_MODIFY    ,_T("-with_modify")  ));
		m_listOptions.Add(new CaCopyDBOptionList(IDS_WITH_DATA      ,_T("-with_data")    ));
		m_listOptions.Add(new CaCopyDBOptionList(IDS_WITH_INDEX     ,_T("-with_index")   ));
		m_listOptions.Add(new CaCopyDBOptionList(IDS_WITH_CONSTR    ,_T("-with_constr")  ));
		m_listOptions.Add(new CaCopyDBOptionList(IDS_WITH_VIEWS     ,_T("-with_views")   ));
		m_listOptions.Add(new CaCopyDBOptionList(IDS_WITH_SYNONYMS  ,_T("-with_synonyms")));
		m_listOptions.Add(new CaCopyDBOptionList(IDS_WITH_EVENTS    ,_T("-with_events")  ));
		m_listOptions.Add(new CaCopyDBOptionList(IDS_WITH_PROC      ,_T("-with_proc")    ));
		m_listOptions.Add(new CaCopyDBOptionList(IDS_WITH_REG       ,_T("-with_reg")     ));
		m_listOptions.Add(new CaCopyDBOptionList(IDS_WITH_RULES     ,_T("-with_rules")   ));
		m_listOptions.Add(new CaCopyDBOptionList(IDS_WITH_ALARMS    ,_T("-with_alarms")  ));
		m_listOptions.Add(new CaCopyDBOptionList(IDS_WITH_COMMENTS  ,_T("-with_comments")));
		m_listOptions.Add(new CaCopyDBOptionList(IDS_WITH_ROLES     ,_T("-with_roles")   ));
		m_listOptions.Add(new CaCopyDBOptionList(IDS_WITH_PERMITS   ,_T("-with_permits") ));
		if (GetOIVers()>=OIVERS_30)
			m_listOptions.Add(new CaCopyDBOptionList(IDS_WITH_SEQUENCES ,_T("-with_sequences") ));
	}
	catch (CMemoryException* e)
	{
		VDBA_OutOfMemoryMessage ();
		e->Delete();
	}
}


CxDlgCopyDb::CxDlgCopyDb(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgCopyDb::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgCopyDb)
	//}}AFX_DATA_INIT
	memset(&m_lpTblParam,0,sizeof(AUDITDBTPARAMS));
}


void CxDlgCopyDb::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgCopyDb)
	DDX_Control(pDX, IDC_EDIT_OUTPUT_FILENAME, m_ctrlEditOutputName);
	DDX_Control(pDX, IDC_EDIT_INPUT_FILENAME, m_ctrlEditInputName);
	DDX_Control(pDX, IDC_EDIT_DIRECTORY_SOURCE, m_ctrlEditDirSource);
	DDX_Control(pDX, IDC_EDIT_DIRECTORY_NAME, m_ctrlEditDirName);
	DDX_Control(pDX, IDC_EDIT_DIRECTORY_DESTINATION, m_ctrlEditDirDest);
	DDX_Control(pDX, IDC_CHECK_TABLES, m_ctrlCheckTables);
	DDX_Control(pDX, IDC_EDIT_SELTBL, m_ctrlSelTbl);
	DDX_Control(pDX, IDC_STATIC_TBL, m_staticTbl);
	DDX_Control(pDX, IDC_BUTTON_SPECIFY_TABLES, m_ctrlSpecifyTable);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgCopyDb, CDialog)
	//{{AFX_MSG_MAP(CxDlgCopyDb)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_CHECK_TABLES, OnCheckTables)
	ON_BN_CLICKED(IDC_BUTTON_SPECIFY_TABLES, OnButtonSpecifyTables)
	//}}AFX_MSG_MAP
	ON_CLBN_CHKCHANGE (IDC_LIST_OPTIONS, OnCheckChangeOption)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgCopyDb message handlers

BOOL CxDlgCopyDb::OnInitDialog()
{
	CString csFormatTitle,csCaption;
	LPUCHAR parent [MAXPLEVEL];
	TCHAR   buffer [MAXOBJECTNAME];

	CDialog::OnInitDialog();

	VERIFY (m_lstCheckOptions.SubclassDlgItem (IDC_LIST_OPTIONS, this));
	InitializeLstOptions();
	FillListOptions();
	ASSERT (m_lstCheckOptions.GetCount() > 0);

	GetWindowText(csFormatTitle);
	csCaption.Format(csFormatTitle, (LPCTSTR)m_csNodeName, (LPCTSTR)m_csDBName);

	if (!m_csTableName.IsEmpty())
	{
		m_ctrlSelTbl.ShowWindow(SW_SHOW);
		m_staticTbl.ShowWindow(SW_SHOW);
		m_ctrlSelTbl.SetWindowText(m_csTableName);

		m_ctrlSpecifyTable.ShowWindow(SW_HIDE);
		m_ctrlCheckTables.ShowWindow(SW_HIDE);

		parent [0] = (LPUCHAR)(LPTSTR)(LPCTSTR)m_csDBName;
		parent [1] = NULL;
		GetExactDisplayString (m_csTableName, m_csTableOwner, m_iObjectType, parent, buffer);
		if(m_iObjectType == OT_TABLE)
			csCaption += _T(" , table ");
		else
			csCaption += _T(" , view ");
		csCaption += buffer;
	}
	else
	{
		m_ctrlSelTbl.ShowWindow(SW_HIDE);
		m_staticTbl.ShowWindow(SW_HIDE);
		m_ctrlSpecifyTable.ShowWindow(SW_SHOW);
		m_ctrlCheckTables.ShowWindow(SW_SHOW);
	}

	SetWindowText(csCaption);

	PushHelpId(IDD_COPYINOUT);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgCopyDb::OnDestroy()
{
	CDialog::OnDestroy();
	if (m_lpTblParam.lpTable)
	{
		FreeObjectList(m_lpTblParam.lpTable);
		m_lpTblParam.lpTable = NULL;
	}

	CaCopyDBOptionList* lpTemp;
	int iSize = m_listOptions.GetSize();
	for (int i = 0; i<iSize;i++)
	{
		lpTemp = m_listOptions.GetAt(i);
		delete(lpTemp);
	}
	m_listOptions.RemoveAll();

	PopHelpId ();
}

void CxDlgCopyDb::FillListOptions()
{
	CaCopyDBOptionList *lpTempDBlist;
	CString Tempo;
	int nIdx,iMax,index;

	m_lstCheckOptions.ResetContent();
	iMax = m_listOptions.GetSize();
	for (index = 0;  index < iMax; index++)
	{
		lpTempDBlist = m_listOptions.GetAt(index);
		Tempo.LoadString(lpTempDBlist->m_iStringID);
		nIdx = m_lstCheckOptions.AddString(Tempo);
		if (nIdx != LB_ERR)
		{
			m_lstCheckOptions.SetItemData(nIdx, (DWORD)(LPCTSTR)lpTempDBlist->m_strOption);
			m_lstCheckOptions.SetCheck(nIdx); // Default all options are checked
		}
	}
}

void CxDlgCopyDb::OnCheckTables()
{
	if (IsDlgButtonChecked(IDC_CHECK_TABLES) == BST_CHECKED)
		m_ctrlSpecifyTable.EnableWindow(TRUE);  // unGrayed "Tables" button
	else
		m_ctrlSpecifyTable.EnableWindow(FALSE); // Grayed "Tables" button
}

void CxDlgCopyDb::OnButtonSpecifyTables()
{
	int iret;
	m_lpTblParam.bRefuseTblWithDupName = TRUE;
	_tcscpy((LPTSTR)m_lpTblParam.DBName, (LPCTSTR)m_csDBName);
	iret = DlgAuditDBTable (m_hWnd , &m_lpTblParam);
}


void CxDlgCopyDb::GenerateCopyDBSyntax(CString& csCommandSyntax)
{
	CString csTempo;
	TCHAR szgateway[50];
	int nEditLen;

	BOOL bHasGWSuffix = GetGWClassNameFromString((LPUCHAR)(LPCTSTR)m_csNodeName, (LPUCHAR)szgateway);
	csCommandSyntax = _T("copydb ");
	csCommandSyntax += m_csDBName;

	if ( IsStarDatabase(GetCurMdiNodeHandle (),(LPUCHAR)(LPCTSTR)m_csDBName) )
		csCommandSyntax += _T("/star ");
	else if (bHasGWSuffix)
	{
		csCommandSyntax += _T("/");
		csCommandSyntax += szgateway;
		csCommandSyntax += _T(" ");
	}
	else
		csCommandSyntax += _T(" ");

	// store the user name in command line
	TCHAR tchUserName[MAXOBJECTNAME];
	DBAGetUserName ((LPUCHAR)(LPCTSTR)m_csNodeName , (LPUCHAR)tchUserName);

	csCommandSyntax += _T("-u");
	csCommandSyntax += tchUserName;
	csCommandSyntax += _T(" ");

	// store create printable data file option in command line
	if (IsDlgButtonChecked(IDC_CHECK_PRINT_DATA_FILES) == BST_CHECKED)
		csCommandSyntax += _T("-c ");

	// store the directory name in command line
	nEditLen = m_ctrlEditDirName.GetWindowTextLength();
	if ( nEditLen )
	{
		csCommandSyntax += _T("-d");
		m_ctrlEditDirName.GetWindowText(csTempo);
		csCommandSyntax += csTempo;
		csCommandSyntax += _T(" ");
	}

	// store the source directory in command line
	nEditLen = m_ctrlEditDirSource.GetWindowTextLength();
	if (nEditLen)
	{
		csCommandSyntax +=  _T("-source=");
		m_ctrlEditDirSource.GetWindowText(csTempo);
		csCommandSyntax += csTempo;
		csCommandSyntax += _T(" ");
	}

	// store the destination directory in command line
	nEditLen = m_ctrlEditDirDest.GetWindowTextLength();
	if ( nEditLen)
	{
		csCommandSyntax += _T("-dest=");
		m_ctrlEditDirDest.GetWindowText(csTempo);
		csCommandSyntax += csTempo;
		csCommandSyntax += _T(" ");
	}

	// store the statements options
	int iOptSel = m_lstCheckOptions.GetCheckCount();
	int i,iMax  = m_lstCheckOptions.GetCount();
	if (iOptSel == iMax)
		csCommandSyntax += _T("-all ");
	else
	{
		for (i=0; i<iMax; i++)
		{
			CString csFlag;
			csFlag = (LPCTSTR)m_lstCheckOptions.GetItemData(i);
			if ( m_lstCheckOptions.GetCheck(i) )
			{
				if (i>=1)
					csCommandSyntax += _T(" ");
				csCommandSyntax += csFlag;
			}
		}
		csCommandSyntax += _T(" ");
	}

	// store -order_ccm option in command line
	if (IsDlgButtonChecked(IDC_CHECK_ORDER_CCM) == BST_CHECKED)
		csCommandSyntax += _T("-order_ccm ");

	// store -add_drop option in command line
	// (generate the drop statement before writing the create statement)
	if (IsDlgButtonChecked(IDC_CHECK_ADD_DROP) == BST_CHECKED)
		csCommandSyntax += _T("-add_drop ");

	// store -infile option
	nEditLen = m_ctrlEditInputName.GetWindowTextLength();
	if ( nEditLen)
	{
		csCommandSyntax += _T("-infile=");
		m_ctrlEditInputName.GetWindowText(csTempo);
		csCommandSyntax += csTempo;
		csCommandSyntax += _T(" ");
	}

	// store -outfile option
	nEditLen = m_ctrlEditOutputName.GetWindowTextLength();
	if ( nEditLen)
	{
		csCommandSyntax += _T("-outfile=");
		m_ctrlEditOutputName.GetWindowText(csTempo);
		csCommandSyntax += csTempo;
		csCommandSyntax += _T(" ");
	}

	//store -no_loc option in command line
	if (IsDlgButtonChecked(IDC_CHECK_NO_LOC) == BST_CHECKED)
		csCommandSyntax += _T("-no_loc ");

	//store -relpath option in command line
	if (IsDlgButtonChecked(IDC_CHECK_RELPATH) == BST_CHECKED)
		csCommandSyntax += _T("-relpath ");

	//store -noint option in command line
	if (IsDlgButtonChecked(IDC_CHECK_NOINT) == BST_CHECKED)
		csCommandSyntax += _T("-noint ");

	if (m_csTableName.IsEmpty() && m_csTableOwner.IsEmpty())
	{
		//store the tables name selected 
		LPOBJECTLIST ls = m_lpTblParam.lpTable;
		while (ls)
		{
			csCommandSyntax += CString(StringWithoutOwner((LPUCHAR)ls->lpObject));
			ls = (LPOBJECTLIST)ls->lpNext;
			if (ls)
				csCommandSyntax += _T(" ");
		}
	}
	else
	{
		// The current selection is on Table name or a view.
		if ( IsTheSameOwner(GetCurMdiNodeHandle(),(LPTSTR)(LPCTSTR)m_csTableOwner) == FALSE )
		{
			csCommandSyntax.Empty();
			return;
		}
		csCommandSyntax += m_csTableName;
	}

	if (csCommandSyntax.GetLength() >= MAX_LINE_COMMAND)
	{
		CString csMsg = VDBA_MfcResourceString(IDS_E_MAX_CHAR_COMMAND); // "The resulting copydb command line is too long.\n Cannot execute the command."
		AfxMessageBox (csMsg);
		csCommandSyntax.Empty();
		return;
	}
}

void CxDlgCopyDb::ExecuteRemoteCommand(LPCTSTR csCommandLine)
{
	int hnode;
	CString csTempo,csTitle;

	hnode = OpenNodeStruct  ((LPUCHAR)(LPCTSTR)m_csNodeName);
	if (hnode<0)
	{
		CString strMsg = VDBA_MfcResourceString (IDS_MAX_NB_CONNECT);//_T("Maximum number of connections has been reached\n"
		AfxMessageBox (strMsg);
		return;
	}
	// Temporary for activate a session
	UCHAR buf[MAXOBJECTNAME];
	DOMGetFirstObject (hnode, OT_DATABASE, 0, NULL, FALSE, NULL, buf, NULL, NULL);
	csTitle.Format(IDS_T_RMCMD_COPY_INOUT, (LPCTSTR)m_csNodeName, (LPCTSTR)m_csDBName);
	execrmcmd1( (char *)(LPCTSTR)m_csNodeName,
	            (char *)(LPCTSTR)csCommandLine,
	            (char *)(LPCTSTR)csTitle,
	            TRUE);

	CloseNodeStruct(hnode,FALSE);
}

void CxDlgCopyDb::OnOK() 
{
	CString csCommandSyntax;

	GenerateCopyDBSyntax(csCommandSyntax);

	if (!csCommandSyntax.IsEmpty())
		ExecuteRemoteCommand(csCommandSyntax);
	else
		return;

	CDialog::OnOK();
}

void CxDlgCopyDb::OnCheckChangeOption()
{
	CWaitCursor doWaitCursor;
	CWnd* pWnd = GetDlgItem(IDOK);
	if (!pWnd)
		return;
	if (!IsWindow(pWnd->m_hWnd))
		return;
	int   i, nCount= m_lstCheckOptions.GetCount();
	BOOL bAtLeastOne = FALSE;
	for (i=0; i<nCount; i++)
	{
		if (m_lstCheckOptions.GetCheck(i))
		{
			bAtLeastOne = TRUE;
			break;
		}
	}
	pWnd->EnableWindow(bAtLeastOne);
}
