/******************************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**  Source   : xdlgdbgr.cpp, Implementation file
**
**
**    Project  : Ingres II/VDBA 
**    Author   : UK Sotheavut  
**                            
**                           
**    Purpose  : Create Rule Dialog Box    
**
**    01-Nov-2001 (hanje04)
**	Moved declaration of i outside for loop to stop NON-ANSI errors on 
**	Linux
**  25-Mar-2003 (noifr01)
**   (sir 107523) management of sequences
**  23-Jan-2004 (schph01)
**   (sir 104378) detect version 3 of Ingres, for managing
**    new features provided in this version. replaced references
**    to 2.65 with refereces to 3  in #define definitions for
**    better readability in the future
**  02-Jun-2010 (drivi01)
**    Removed BUFSIZE redefinition.  It's not needed here anymore.
**    the constant is defined in main.h now. and main.h is included
**    in dom.h.
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "xdlgdbgr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern "C"
{
#include "dbaset.h"
#include "getdata.h"
#include "dlgres.h"
#include "msghandl.h"
#include "dbadlg1.h"
#include "domdata.h"
#include "resource.h"
LPOBJECTLIST APublicUser ();
}

/////////////////////////////////////////////////////////////////////////////
// CxDlgGrantDatabase dialog


CxDlgGrantDatabase::CxDlgGrantDatabase(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgGrantDatabase::IDD, pParent)
{
	m_pParam = NULL;
	m_nNodeHandle = GetCurMdiNodeHandle();
	//{{AFX_DATA_INIT(CxDlgGrantDatabase)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CxDlgGrantDatabase::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgGrantDatabase)
	DDX_Control(pDX, IDOK, m_cButtonOK);
	DDX_Control(pDX, IDC_MFC_STATIC1, m_cStatic1);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgGrantDatabase, CDialog)
	//{{AFX_MSG_MAP(CxDlgGrantDatabase)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_MFC_RADIO1, OnRadioUser)
	ON_BN_CLICKED(IDC_MFC_RADIO2, OnRadioRole)
	ON_BN_CLICKED(IDC_MFC_RADIO3, OnRadioGroup)
	ON_BN_CLICKED(IDC_MFC_RADIO4, OnRadioPublic)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_LAYOUTEDITNUMBERDLG_OK,      OnPrivilegeChange)
	ON_MESSAGE (WM_LAYOUTCHECKBOX_CHECKCHANGE,  OnPrivilegeChange)
	ON_CLBN_CHKCHANGE (IDC_MFC_LIST1,           OnCheckChangeGrantee)
	ON_CLBN_CHKCHANGE (IDC_MFC_LIST2,           OnCheckChangeDatabase)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgGrantDatabase message handlers

BOOL CxDlgGrantDatabase::OnInitDialog() 
{
	int nIdx = -1;
	CDialog::OnInitDialog();
	ASSERT (m_pParam && m_nNodeHandle != -1);
	if (!m_pParam || m_nNodeHandle == -1)
		return FALSE;
	VERIFY (m_cCheckListBoxGrantee.SubclassDlgItem (IDC_MFC_LIST1, this));
	VERIFY (m_cCheckListBoxDatabase.SubclassDlgItem (IDC_MFC_LIST2, this));
	VERIFY (m_cListPrivilege.SubclassDlgItem (IDC_MFC_LIST3, this));
	m_ImageCheck.Create (IDB_CHECK, 16, 1, RGB (255, 0, 0));
	m_cListPrivilege.SetCheckImageList(&m_ImageCheck);
	
	LPGRANTPARAMS pGrant = (LPGRANTPARAMS)m_pParam;
	//
	// Initalize the Column Header of CListCtrl (CuListCtrl)
	//
	LV_COLUMN lvcolumn;
	LSCTRLHEADERPARAMS lsp[6] =
	{
		{_T(""),   100,  LVCFMT_LEFT,   FALSE},
		{_T(""),    45,  LVCFMT_CENTER, TRUE },
		{_T(""),    68,  LVCFMT_CENTER, TRUE },
		{_T(""),    36,  LVCFMT_LEFT,   FALSE},
		{_T(""),   114,  LVCFMT_LEFT,   FALSE},
		{_T(""),    45,  LVCFMT_CENTER, TRUE }
	};
	lstrcpy(lsp[0].m_tchszHeader,VDBA_MfcResourceString(IDS_TC_PRIVILEGE)); //_T("Privilege"),  
	lstrcpy(lsp[1].m_tchszHeader,VDBA_MfcResourceString(IDS_TC_GRANT));     //_T("Grant"),      
	lstrcpy(lsp[2].m_tchszHeader,VDBA_MfcResourceString(IDS_TC_GRANT_LIM)); //_T("Grant Limit"),
	lstrcpy(lsp[3].m_tchszHeader,VDBA_MfcResourceString(IDS_TC_LIMIT));     //_T("Limit"),      
	lstrcpy(lsp[4].m_tchszHeader,VDBA_MfcResourceString(IDS_TC_NO_PRIV));   //_T("No Privilege")
	lstrcpy(lsp[5].m_tchszHeader,VDBA_MfcResourceString(IDS_TC_GRANT));     //_T("Grant"),      

	memset (&lvcolumn, 0, sizeof (LV_COLUMN));
	lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
	int i=0;
	for (i=0; i<6; i++)
	{
		lvcolumn.fmt      = lsp[i].m_fmt;
		lvcolumn.pszText  = lsp[i].m_tchszHeader;
		lvcolumn.iSubItem = i;
		lvcolumn.cx       = lsp[i].m_cxWidth;
		m_cListPrivilege.InsertColumn (i, &lvcolumn, lsp[i].m_bUseCheckMark);
	}

	//
	// Make up title:
	CString strCap;
	if (pGrant->bInstallLevel) 
	{
		m_cStatic1.SetWindowText (_T("On:"));
		//_T("Grant Current Installation on %s")
		strCap.Format (VDBA_MfcResourceString(IDS_F_GRANT_INSTALL), (LPTSTR)GetVirtNodeName(m_nNodeHandle));
	}
	else 
	{
		CString strFormat;
		if (strFormat.LoadString (IDS_T_GRANT_DATABASE) > 0)
			strCap.Format (strFormat, (LPTSTR)GetVirtNodeName(m_nNodeHandle));
		else
			strCap = _T("Grant Database");
	}
	SetWindowText (strCap);

	InitializeFillPrivileges ();
	InitializeDatabase();

	switch (pGrant->GranteeType)
	{
	case OT_ROLE:
	case OT_GROUP:
		InitializeGrantee(pGrant->GranteeType);
		break;
	default:
		if (lstrcmpi ((LPCTSTR)pGrant->PreselectGrantee, (LPCTSTR)lppublicdispstring()) == 0)
			InitializeGrantee(-1);
		else
			InitializeGrantee(OT_USER);
		break;
	}
	//
	// Preselect Grantee:
	if (lstrlen ((LPCTSTR)pGrant->PreselectGrantee) > 0)
	{
		nIdx = m_cCheckListBoxGrantee.FindStringExact (-1, (LPCTSTR)pGrant->PreselectGrantee);
		if (nIdx != LB_ERR)
			m_cCheckListBoxGrantee.SetCheck (nIdx, TRUE);
	}
	//
	// Preselect Database:
	if (lstrlen ((LPCTSTR)pGrant->PreselectObject) > 0)
	{
		nIdx = m_cCheckListBoxDatabase.FindStringExact (-1, (LPCTSTR)pGrant->PreselectObject);
		if (nIdx != LB_ERR)
			m_cCheckListBoxDatabase.SetCheck (nIdx, TRUE);
	}
	//
	// Precheck the privileges
	//
	for (i = 0; i <= GRANT_MAX_PRIVILEGES; i++)
	{
		if (pGrant->PreselectPrivileges [i])
			CheckPrevileges (i);
	}

	m_cListPrivilege.DisplayPrivileges();
	EnableButtonOK();
	PushHelpId (pGrant->bInstallLevel? HELPID_IDD_GRANT_INSTALLATION: IDD_GRANT_DATABASE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgGrantDatabase::OnDestroy() 
{
	Cleanup();
	CDialog::OnDestroy();
	PopHelpId();
}


void CxDlgGrantDatabase::InitializeFillPrivileges ()
{
	int     i, idx;
	typedef struct tagDBPRIV
	{
		UINT nPrivilege;
		UINT nNoPrivilege;
		BOOL bValue; // TRUE if Privilege needs value.
	} DBPRIV;

	DBPRIV DBPrivTable [17] =
	{
		{GRANT_ACCESS,             GRANT_NO_ACCESS,            FALSE},
		{GRANT_CREATE_PROCEDURE,   GRANT_NO_CREATE_PROCEDURE,  FALSE},
		{GRANT_CREATE_TABLE,       GRANT_NO_CREATE_TABLE,      FALSE},
		{GRANT_DATABASE_ADMIN,     GRANT_NO_DATABASE_ADMIN,    FALSE},
		{GRANT_LOCKMOD,            GRANT_NO_LOCKMOD,           FALSE},
		{GRANT_SELECT_SYSCAT,      GRANT_NO_SELECT_SYSCAT,     FALSE},
		{GRANT_UPDATE_SYSCAT,      GRANT_NO_UPDATE_SYSCAT,     FALSE},
		{GRANT_TABLE_STATISTICS,   GRANT_NO_TABLE_STATISTICS,  FALSE},
		{GRANT_CREATE_SEQUENCE,    GRANT_NO_CREATE_SEQUENCE,   FALSE},
		{GRANT_CONNECT_TIME_LIMIT, GRANT_NO_CONNECT_TIME_LIMIT,TRUE},
		{GRANT_QUERY_IO_LIMIT,     GRANT_NO_QUERY_IO_LIMIT,    TRUE},
		{GRANT_QUERY_ROW_LIMIT,    GRANT_NO_QUERY_ROW_LIMIT,   TRUE},
		{GRANT_SESSION_PRIORITY,   GRANT_NO_SESSION_PRIORITY,  TRUE},
		{GRANT_IDLE_TIME_LIMIT,    GRANT_NO_IDLE_TIME_LIMIT,   TRUE},
		{GRANT_QUERY_CPU_LIMIT   , GRANT_NO_QUERY_CPU_LIMIT,   TRUE},
		{GRANT_QUERY_PAGE_LIMIT  , GRANT_NO_QUERY_PAGE_LIMIT,  TRUE},
		{GRANT_QUERY_COST_LIMIT  , GRANT_NO_QUERY_COST_LIMIT,  TRUE}

	};
	CaDatabasePrivilegesItemData* pDBPriv = NULL;

	for (i = 0; i<17; i++)
	{
		if (DBPrivTable[i].nPrivilege==GRANT_CREATE_SEQUENCE && GetOIVers()<OIVERS_30)
			continue;
		pDBPriv = new CaDatabasePrivilegesItemData (DBPrivTable [i].nPrivilege, DBPrivTable [i].nNoPrivilege, DBPrivTable [i].bValue);
		idx = m_cListPrivilege.InsertItem (m_cListPrivilege.GetItemCount(), _T(""));
		if (idx != -1)
			m_cListPrivilege.SetItemData (idx, (DWORD)pDBPriv);
	}
}


void CxDlgGrantDatabase::Cleanup()
{
	CaDatabasePrivilegesItemData* pData = NULL;
	int i, nCount = m_cListPrivilege.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		pData = (CaDatabasePrivilegesItemData*)m_cListPrivilege.GetItemData (i);
		if (pData)
			delete pData;
	}
}


BOOL CxDlgGrantDatabase::InitializeDatabase()
{
	int idx;
	LPGRANTPARAMS pGrant = (LPGRANTPARAMS)m_pParam;
	if (pGrant->bInstallLevel) 
	{
		idx = m_cCheckListBoxDatabase.AddString (VDBA_MfcResourceString(IDS_CURRENT_INSTALLATION));//_T("<current installation>")
		m_cCheckListBoxDatabase.EnableWindow (FALSE);
		if (idx != LB_ERR)
			m_cCheckListBoxDatabase.SetCheck (0, TRUE);
	}
	else
	{
		int   ires = RES_ERR;
		TCHAR buf       [MAXOBJECTNAME];
		TCHAR buffilter [MAXOBJECTNAME];
		CWaitCursor waitCursor;
		
		memset (buf, 0, sizeof (buf));
		memset (buffilter, 0, sizeof (buffilter));
		ires    = DOMGetFirstObject (m_nNodeHandle, OT_DATABASE, 0, NULL, TRUE, NULL, (LPUCHAR)buf, NULL, NULL);
		if (ires != RES_SUCCESS)
			return FALSE;
		while (ires == RES_SUCCESS)
		{
			m_cCheckListBoxDatabase.AddString (buf);
			ires  = DOMGetNextObject ((LPUCHAR)buf, (LPUCHAR)buffilter, NULL);
		}
	}
	return TRUE;
}

BOOL CxDlgGrantDatabase::InitializeGrantee(int nGranteeType)
{
	int   nGranntee = IDC_MFC_RADIO4, ires = RES_ERR;
	TCHAR buf       [MAXOBJECTNAME];
	TCHAR buffilter [MAXOBJECTNAME];
	CWaitCursor waitCursor;

	memset (buf, 0, sizeof (buf));
	memset (buffilter, 0, sizeof (buffilter));
	m_cCheckListBoxGrantee.ResetContent();
	if (nGranteeType == -1)
	{
		CheckRadioButton (IDC_MFC_RADIO1, IDC_MFC_RADIO4, IDC_MFC_RADIO4);
		return TRUE;
	}
	ires    = DOMGetFirstObject (m_nNodeHandle, nGranteeType, 0, NULL, GetSystemFlag(), NULL, (LPUCHAR)buf, NULL, NULL);
	if (ires != RES_SUCCESS)
		return FALSE;
	while (ires == RES_SUCCESS)
	{
		if (nGranteeType == OT_USER && lstrcmpi (buf, (LPTSTR)lppublicdispstring()) == 0)
		{
			ires  = DOMGetNextObject ((LPUCHAR)buf, (LPUCHAR)buffilter, NULL);
			continue;
		}
		m_cCheckListBoxGrantee.AddString (buf);
		ires  = DOMGetNextObject ((LPUCHAR)buf, (LPUCHAR)buffilter, NULL);
	}
	
	switch (nGranteeType)
	{
	case OT_USER:
		nGranntee =  IDC_MFC_RADIO1;
		break;
	case OT_ROLE:
		nGranntee =  IDC_MFC_RADIO2;
		break;
	case OT_GROUP:
		nGranntee =  IDC_MFC_RADIO3;
		break;
	default:
		break;
	}
	CheckRadioButton (IDC_MFC_RADIO1, IDC_MFC_RADIO4, nGranntee);
	return TRUE;
}

void CxDlgGrantDatabase::CheckPrevileges (int nPrivNoPriv)
{
	CaDatabasePrivilegesItemData* pData = NULL;
	int i, nCount = m_cListPrivilege.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		pData = (CaDatabasePrivilegesItemData*)m_cListPrivilege.GetItemData (i);
		if (pData->GetIDPrivilege() == nPrivNoPriv)
		{
			pData->SetPrivilege();
		}
		else
		if (pData->GetIDNoPrivilege() == nPrivNoPriv)
		{
			pData->SetNoPrivilege();
		}
	}
}

void CxDlgGrantDatabase::OnOK() 
{
	int ires, i, nCount;
	CString strItem,strItemQuoted;
	LPGRANTPARAMS pGrant = (LPGRANTPARAMS)m_pParam;
	//
	// Verify the input:
	// 1) Must specify the grantee(s)
	// 2) Must specify the database(s)
	// 3) Must specify the privilege(s)
	CString strMsg;
	if (!ValidateInputData(strMsg))
	{
		AfxMessageBox (strMsg);
		return;
	}

	//
	// Construct the list of databases:
	nCount = m_cCheckListBoxDatabase.GetCount();
	LPOBJECTLIST list = NULL;
	LPOBJECTLIST obj;
	for (i = 0; i < nCount; i++)
	{
		if (!m_cCheckListBoxDatabase.GetCheck (i))
			continue;
		m_cCheckListBoxDatabase.GetText (i, strItem);
		obj = AddListObject (list, strItem.GetLength() +1);
		if (obj)
		{
			lstrcpy ((LPTSTR)obj->lpObject, strItem);
			list = obj;
		}
		else
		{
			//
			// Cannot allocate memory 
			FreeObjectList (list);
			VDBA_OutOfMemoryMessage();
			list = NULL;
			return;
		}
	}
	pGrant->lpobject = list;
	//
	// Grantee type:
	int nGrantee = GetCheckedRadioButton(IDC_MFC_RADIO1, IDC_MFC_RADIO4);
	switch (nGrantee)
	{
	case IDC_MFC_RADIO1:
		pGrant->GranteeType = OT_USER;
		break;
	case IDC_MFC_RADIO2:
		pGrant->GranteeType = OT_ROLE;
		break;
	case IDC_MFC_RADIO3:
		pGrant->GranteeType = OT_GROUP;
		break;
	case IDC_MFC_RADIO4:
		pGrant->GranteeType = OT_PUBLIC;
		break;
	default:
		break;
	}
	//
	// Construct the list of Grantees:
	if (pGrant->GranteeType == OT_PUBLIC)
	{
		pGrant->lpgrantee = APublicUser();
		if (!pGrant->lpgrantee)
		{
			FreeObjectList (pGrant->lpobject);
			FreeObjectList (pGrant->lpgrantee);
			VDBA_OutOfMemoryMessage();
			return;
		}
	}
	else
	{
		list = NULL;
		nCount = m_cCheckListBoxGrantee.GetCount();
		for (i = 0; i < nCount; i++)
		{
			if (!m_cCheckListBoxGrantee.GetCheck (i))
				continue;
			m_cCheckListBoxGrantee.GetText (i, strItem);
			if (lstrcmpi (strItem, (LPTSTR)lppublicdispstring()) == 0)
				obj = AddListObject (list, lstrlen((LPTSTR)lppublicsysstring())+1);
			else
			{
				strItemQuoted = QuoteIfNeeded(strItem);
				obj = AddListObject (list, strItemQuoted.GetLength() +1);
			}
			if (obj)
			{
				if (lstrcmpi (strItem, (LPTSTR)lppublicdispstring()) == 0)
					lstrcpy ((LPTSTR)obj->lpObject, (LPTSTR)lppublicsysstring());
				else
					lstrcpy ((LPTSTR)obj->lpObject, strItemQuoted);
				list = obj;
			}
			else
			{
				//
				// Cannot allocate memory 
				FreeObjectList (pGrant->lpobject);
				FreeObjectList (list);
				VDBA_OutOfMemoryMessage();
				list = NULL;
				return;
			}
		}
		pGrant->lpgrantee = list;
	}

	//
	// List of Privileges:
	memset (pGrant->Privileges, 0, sizeof (pGrant->Privileges));
	CaDatabasePrivilegesItemData* pData = NULL;
	nCount = m_cListPrivilege.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		pData = (CaDatabasePrivilegesItemData*)m_cListPrivilege.GetItemData (i);
		if (!pData)
			continue;
		if (pData->GetPrivilegeType() == 1 && pData->GetGrantPrivilege())
		{
			pGrant->Privileges [pData->GetIDPrivilege()] = TRUE;
			if (pData->HasValue())
				pGrant->PrivValues [pData->GetIDPrivilege()] = atoi (pData->GetValue());
		}
		else
		if (pData->GetPrivilegeType() ==-1 && pData->GetGrantNoPrivilege())
			pGrant->Privileges [pData->GetIDNoPrivilege()] = TRUE;
	}

	ires = DBAAddObject ((LPUCHAR)GetVirtNodeName(m_nNodeHandle), OTLL_GRANT, (void *)pGrant);
	FreeObjectList (pGrant->lpobject);
	FreeObjectList (pGrant->lpgrantee);
	if (ires != RES_SUCCESS)
	{
		ErrorMessage ((UINT)IDS_E_GRANT_DATABASE_FAILED, ires);
		return;
	}
	CDialog::OnOK();
}

void CxDlgGrantDatabase::OnRadioUser() 
{
	InitializeGrantee(OT_USER);
}

void CxDlgGrantDatabase::OnRadioRole() 
{
	InitializeGrantee(OT_ROLE);
}

void CxDlgGrantDatabase::OnRadioGroup() 
{
	InitializeGrantee(OT_GROUP);
}

void CxDlgGrantDatabase::OnRadioPublic() 
{
	InitializeGrantee(-1);
}

BOOL CxDlgGrantDatabase::ValidateInputData(CString& strMsg)
{
	BOOL bOK = FALSE;
	//
	// Verify Grantees:
	int nGrantee = GetCheckedRadioButton(IDC_MFC_RADIO1, IDC_MFC_RADIO4);
	if (nGrantee == IDC_MFC_RADIO4)
		bOK = TRUE;
	else
		bOK = (m_cCheckListBoxGrantee.GetCheckCount() > 0)? TRUE: FALSE;
	if (!bOK)
	{
		strMsg.LoadString(IDS_A_PLEASE_CHOOSE_GRANTEE);// = _T("Please choose the grantee(s)");
		return FALSE;
	}
	//
	// Verify Databases:
	bOK = (m_cCheckListBoxDatabase.GetCheckCount() > 0)? TRUE: FALSE;
	if (!bOK)
	{
		strMsg.LoadString(IDS_A_PLEASE_CHOOSE_DB);// = _T("Please choose the database(s)");
		return FALSE;
	}
	//
	// Verify Privileges:
	CaDatabasePrivilegesItemData* pData = NULL;
	int i, nPriv = 0, nCount = m_cListPrivilege.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		pData = (CaDatabasePrivilegesItemData*)m_cListPrivilege.GetItemData (i);
		if (!pData)
			continue;
		if (pData->GetPrivilegeType() == 1 && pData->GetGrantPrivilege())
		{
			nPriv++;
			if (pData->HasValue() && pData->GetValue().IsEmpty())
			{
				CString strPriv = m_cListPrivilege.GetItemText (i, 0);
				//_T("Please specify the limit for the privilege <%s>")
				strMsg.Format (VDBA_MfcResourceString(IDS_F_LIMIT_PRIVILEGE), (LPCTSTR)strPriv);
				return FALSE;
			}
		}
		else
		if (pData->GetPrivilegeType() ==-1 && pData->GetGrantNoPrivilege())
			nPriv++;
	}
	if (nPriv == 0)
	{
		strMsg.LoadString(IDS_A_SPECIFY_PRIVILEGE);//_T("Please specify the privilege(s)");
		return FALSE;
	}
	return TRUE;
}

void CxDlgGrantDatabase::EnableButtonOK()
{
	CString strMsg;
	BOOL bEnable = ValidateInputData(strMsg);
	m_cButtonOK.EnableWindow (bEnable);
}

LONG CxDlgGrantDatabase::OnPrivilegeChange (UINT wParam, LONG lParam)
{
	EnableButtonOK();
	return 0;
}

void CxDlgGrantDatabase::OnCheckChangeGrantee()
{
	EnableButtonOK();
}

void CxDlgGrantDatabase::OnCheckChangeDatabase()
{
	EnableButtonOK();
}
