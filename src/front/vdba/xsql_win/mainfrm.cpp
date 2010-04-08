/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : mainfrm.cpp : implementation file
**    Project  : SQL/Test (stand alone)
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Main frame.
**
** History:
**	10-Dec-2001 (uk$so01)
**	    Created
**	19-Dec-2002 (uk$so01)
**	    SIR #109220, Enhance the library.
**	26-Feb-2003 (uk$so01)
**	    SIR #106648, conform to the change of DDS VDBA split
**	13-Jun-2003 (uk$so01)
**	    SIR #106648, Take into account the STAR database for connection.
**	07-Jul-2003 (uk$so01)
**	    SIR #106648, Take into account the STAR database for connection.
**	03-Oct-2003 (uk$so01)
**	    SIR #106648, Vdba-Split, Additional fix for GATEWAY Enhancement 
**	15-Oct-2003 (uk$so01)
**	    SIR #106648, (Integrate 2.65 features for EDBC)
**	17-Oct-2003 (uk$so01)
**	    SIR #106648, Additional change to change #464605 (role/password)
**	30-Jan-2004 (uk$so01)
**	    SIR #111701, Use Compiled HTML Help (.chm file)
**	02-feb-2004 (somsa01)
**	    Updated old CMemoryException's which are now errors.
**  16-Mar-2004 (uk$so01)
**     BUG #111964 / ISSUE 13296761 Vdbasql shows the wrong help file.
**     It shows the wdbamon help file instead of vdbasql help file.
** 26-Mar-2004 (uk$so01)
**    SIR #111701, The default help page is available now!. 
**    Activate the default page if no specific ID help is specified.
** 01-Sept-2004 (schph01)
**    SIR #106648 add management for the 'History of SQL Statements Errors'
**    dialog 
** 12-Oct-2004 (uk$so01)
**    BUG #113215 / ISSUE 13720075,
**    F1-key should always invoke the Help of SQLTest.
** 14-Aug-2008 (smeke01) b129684
**    Fix group and role initialisation and command-line handling. This 
**    was mostly moving the existing code to the right place. Also 
**    fixed a resource-id clash that prevented the group dropdown box 
**    from receiving messages.
*/


#include "stdafx.h"
#include <htmlhelp.h>
#include "xsql.h"
#include "mainfrm.h"
#include "xsqlview.h"
#include "xsqldoc.h"
#include "rctools.h"
#include "xsqldml.h"
#include "dmluser.h"
#include "dmldbase.h"
#include "usrexcep.h"
#include "compfile.h"
#include "sqlerr.h"
#if defined (_OPTION_GROUPxROLE)
#include "dmlgroup.h"
#include "dmlrole.h"
#include "imageidx.h"
#include "ingobdml.h"
#include "inputpwd.h"
#include "mainfrm.h"
extern CString INGRESII_llQueryRolePassword (LPCTSTR lpszRoleName, CaLLQueryInfo* pQueryInfo, CaSessionManager* pSessionManager);
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
BOOL APP_HelpInfo(HWND hWnd, UINT nHelpID);

/////////////////////////////////////////////////////////////////////////////
// CfMainFrame

IMPLEMENT_DYNCREATE(CfMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CfMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CfMainFrame)
	ON_WM_CREATE()
	ON_COMMAND(IDM_CLEAR, OnClear)
	ON_COMMAND(IDM_OPEN_QUERY, OnOpenQuery)
	ON_COMMAND(IDM_SAVE_QUERY, OnSaveQuery)
	ON_COMMAND(IDM_ASSISTANT, OnAssistant)
	ON_COMMAND(IDM_RUN, OnRun)
	ON_COMMAND(IDM_QEP, OnQep)
	ON_COMMAND(IDM_XML, OnXml)
	ON_COMMAND(IDM_TRACE, OnTrace)
	ON_COMMAND(IDM_SETTING, OnSetting)
	ON_UPDATE_COMMAND_UI(IDM_CLEAR, OnUpdateClear)
	ON_UPDATE_COMMAND_UI(IDM_OPEN_QUERY, OnUpdateOpenQuery)
	ON_UPDATE_COMMAND_UI(IDM_SAVE_QUERY, OnUpdateSaveQuery)
	ON_UPDATE_COMMAND_UI(IDM_ASSISTANT, OnUpdateAssistant)
	ON_UPDATE_COMMAND_UI(IDM_RUN, OnUpdateRun)
	ON_UPDATE_COMMAND_UI(IDM_QEP, OnUpdateQep)
	ON_UPDATE_COMMAND_UI(IDM_TRACE, OnUpdateTrace)
	ON_UPDATE_COMMAND_UI(IDM_XML, OnUpdateXml)
	ON_UPDATE_COMMAND_UI(IDM_SETTING, OnUpdateSetting)
	ON_COMMAND(IDM_REFRESH, OnRefresh)
	ON_COMMAND(IDM_OPEN_ENVIRONMENT, OnOpenEnvironment)
	ON_UPDATE_COMMAND_UI(IDM_OPEN_ENVIRONMENT, OnUpdateOpenEnvironment)
	ON_COMMAND(IDM_SAVE_ENVIRONMENT, OnSaveEnvironment)
	ON_UPDATE_COMMAND_UI(IDM_SAVE_ENVIRONMENT, OnUpdateSaveEnvironment)
	ON_WM_SIZE()
	ON_COMMAND(IDM_COMMIT, OnCommit)
	ON_UPDATE_COMMAND_UI(IDM_COMMIT, OnUpdateCommit)
	ON_WM_CLOSE()
	ON_COMMAND(IDM_PREFERENCE_SAVE, OnPreferenceSave)
	ON_COMMAND(IDM_NEW, OnNew)
	ON_UPDATE_COMMAND_UI(IDM_NEW, OnUpdateNew)
	ON_COMMAND(IDM_PREFERENCE, OnSetting)
	ON_COMMAND(ID_DEFAULT_HELP, OnDefaultHelp)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP


	//
	// Virtual Node Combo notification:
	ON_CBN_DROPDOWN (IDM_NODE, OnDropDownNode)
	ON_CBN_SELCHANGE(IDM_NODE, OnSelChangeNode)
	//
	// Server Class Combo notification:
	ON_CBN_DROPDOWN (IDM_SERVER, OnDropDownServer)
	ON_CBN_SELCHANGE(IDM_SERVER, OnSelChangeServer)
	//
	// User Combo notification:
	ON_CBN_DROPDOWN (IDM_USER, OnDropDownUser)
	ON_CBN_SELCHANGE(IDM_USER, OnSelChangeUser)
	//
	// Database Combo notification:
	ON_CBN_DROPDOWN (IDM_DATABASE, OnDropDownDatabase)
	ON_CBN_SELCHANGE(IDM_DATABASE, OnSelChangeDatabase)
#if defined (_OPTION_GROUPxROLE)
	//
	// Group Combo notification:
	ON_CBN_DROPDOWN (IDM_GROUP, OnDropDownGroup)
	ON_CBN_SELCHANGE(IDM_GROUP, OnSelChangeGroup)
	//
	// Role Combo notification:
	ON_CBN_DROPDOWN (IDM_ROLE, OnDropDownRole)
	ON_CBN_SELCHANGE(IDM_ROLE, OnSelChangeRole)
#endif

	ON_COMMAND(ID_SQL_ERROR, OnHistoricSqlError)
	ON_UPDATE_COMMAND_UI(ID_SQL_ERROR, OnUpdateHistoricSqlError)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
#if defined (_SHOW_INDICATOR_READLOCK_)
	ID_INDICATOR_READLOCK_0,
#endif
	ID_INDICATOR_AUTOCOMMIT_0
};
#if defined (_SHOW_INDICATOR_READLOCK_)
#define READLOCK_POS   4
#define AUTOCOMMIT_POS (READLOCK_POS +1)
#else
#define AUTOCOMMIT_POS 4
#endif

/////////////////////////////////////////////////////////////////////////////
// CfMainFrame construction/destruction

CfMainFrame::CfMainFrame()
{
	m_bPropertyChange = FALSE;
	m_bUpdateLoading = FALSE;
	m_nAlreadyRefresh = 0;
	m_strDefaultServer.LoadString(IDS_DEFAULT_SERVER);
	m_strDefaultUser.LoadString(IDS_DEFAULT_USER);
	m_strDefaultDatabase.LoadString(IDS_DEFAULT_DATABASE);

	m_pDefaultServer   = new CaDBObject (m_strDefaultServer, _T(""));
	m_pDefaultUser     = new CaDBObject (m_strDefaultUser, _T(""));
	m_pDefaultDatabase = new CaDBObject (m_strDefaultDatabase, _T(""));

#if defined (_OPTION_GROUPxROLE)
	m_strDefaultGroup.LoadString(IDS_DEFAULT_GROUP);
	m_strDefaultRole.LoadString(IDS_DEFAULT_ROLE);
	m_pDefaultGroup     = new CaDBObject (m_strDefaultGroup, _T(""));
	m_pDefaultRole      = new CaDBObject (m_strDefaultRole, _T(""));
#endif

	m_bSavePreferenceAsDefault = TRUE;
	m_uSetArg = 0;
	m_nIDHotImage = IDB_HOTMAINFRAME;
}

CfMainFrame::~CfMainFrame()
{
	if (m_pDefaultServer)
		delete m_pDefaultServer;
	if (m_pDefaultUser)
		delete m_pDefaultUser;
	if (m_pDefaultDatabase)
		delete m_pDefaultDatabase;
	while (!m_lNode.IsEmpty())
		delete m_lNode.RemoveHead();
	while (!m_lServer.IsEmpty())
		delete m_lServer.RemoveHead();
	while (!m_lUser.IsEmpty())
		delete m_lUser.RemoveHead();
	while (!m_lDatabase.IsEmpty())
		delete m_lDatabase.RemoveHead();

#if defined (_OPTION_GROUPxROLE)
	if (m_pDefaultGroup)
		delete m_pDefaultGroup;
	if (m_pDefaultRole)
		delete m_pDefaultRole;
	while (!m_lGroup.IsEmpty())
		delete m_lGroup.RemoveHead();
	while (!m_lRole.IsEmpty())
		delete m_lRole.RemoveHead();
#endif

}

void CfMainFrame::UpdateLoadedData(CdSqlQuery* pDoc)
{
	try
	{
		//
		// if (m_bUpdateLoading = TRUE) then the SetDefaultXxx() will
		// not modify the member of the document 'CdSqlQuery'.
		m_bUpdateLoading = TRUE; 

		m_nAlreadyRefresh = 0;
		m_pComboNode    = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_NODE);
		m_pComboServer  = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_SERVER);
		m_pComboUser    = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_USER);
		m_pComboDatabase= (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_DATABASE);
		m_pComboNode->ResetContent();
		m_pComboServer->ResetContent();
		m_pComboUser->ResetContent();
		m_pComboDatabase->ResetContent();

		int nIdx = CB_ERR;
		COMBOBOXEXITEM cbitem;
		memset (&cbitem, 0, sizeof(cbitem));
		cbitem.mask = CBEIF_IMAGE | CBEIF_TEXT | CBEIF_SELECTEDIMAGE | CBEIF_LPARAM ;
		POSITION pos = NULL;

		//
		// Update node combobox:
		pos = m_lNode.GetHeadPosition();
		while (pos != NULL)
		{
			CaNode* pNode = (CaNode*)m_lNode.GetNext(pos);
			if (pNode->IsLocalNode())
			{
				cbitem.iImage = 1;
				cbitem.iSelectedImage = 1;
			}
			else
			{
				cbitem.iImage = 0;
				cbitem.iSelectedImage = 0;
			}

			cbitem.pszText = (LPTSTR)(LPCTSTR)pNode->GetName();
			cbitem.lParam  = (LPARAM)pNode;
			cbitem.iItem   = m_pComboNode->GetCount();
			nIdx = m_pComboNode->InsertItem (&cbitem);
		}
		nIdx = m_pComboNode->FindStringExact(-1, pDoc->GetNode());
		if (nIdx != CB_ERR)
			m_pComboNode->SetCurSel(nIdx);
		else
			m_pComboNode->SetCurSel(0);

		//
		// Update server combobox:
		SetDefaultServer();
		pos = m_lServer.GetHeadPosition();
		while (pos != NULL)
		{
			CaNodeServer* pNodeServer = (CaNodeServer*)m_lServer.GetNext(pos);
			cbitem.pszText = (LPTSTR)(LPCTSTR)pNodeServer->GetName();
			cbitem.lParam  = (LPARAM)pNodeServer;
			cbitem.iItem   = m_pComboServer->GetCount();
			m_pComboServer->InsertItem (&cbitem);
		}
		nIdx = m_pComboServer->FindStringExact(-1, pDoc->GetServer());
		if (nIdx != CB_ERR)
			m_pComboServer->SetCurSel(nIdx);
		else
			m_pComboServer->SetCurSel(0);

		//
		// Update User combobox:
		SetDefaultUser();
		pos = m_lUser.GetHeadPosition();
		while (pos != NULL)
		{
			CaUser* pUser = (CaUser*)m_lUser.GetNext(pos);
			cbitem.pszText = (LPTSTR)(LPCTSTR)pUser->GetName();
			cbitem.lParam  = (LPARAM)pUser;
			cbitem.iItem   = m_pComboUser->GetCount();
			m_pComboUser->InsertItem (&cbitem);
		}
		nIdx = m_pComboUser->FindStringExact(-1, pDoc->GetUser());
		if (nIdx != CB_ERR)
			m_pComboUser->SetCurSel(nIdx);
		else
			m_pComboUser->SetCurSel(0);
		
		//
		// Update Database combobox:
		SetDefaultDatabase();
		pos = m_lDatabase.GetHeadPosition();
		while (pos != NULL)
		{
			CaDatabase* pDatabase = (CaDatabase*)m_lDatabase.GetNext(pos);
			cbitem.pszText = (LPTSTR)(LPCTSTR)pDatabase->GetName();
			cbitem.lParam  = (LPARAM)pDatabase;
			cbitem.iItem   = m_pComboDatabase->GetCount();
			m_pComboDatabase->InsertItem (&cbitem);
		}
		nIdx = m_pComboDatabase->FindStringExact(-1, pDoc->GetDatabase());
		if (nIdx != CB_ERR)
			m_pComboDatabase->SetCurSel(nIdx);
		else
			m_pComboDatabase->SetCurSel(0);

#if defined (_OPTION_GROUPxROLE)
		m_pComboGroup  = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_GROUP);
		m_pComboRole   = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_ROLE);
		m_pComboGroup->ResetContent();
		m_pComboRole->ResetContent();

		//
		// Update Group combobox:
		SetDefaultGroup();
		pos = m_lGroup.GetHeadPosition();
		while (pos != NULL)
		{
			CaGroup* pGroup = (CaGroup*)m_lGroup.GetNext(pos);
			cbitem.pszText = (LPTSTR)(LPCTSTR)pGroup->GetName();
			cbitem.lParam  = (LPARAM)pGroup;
			cbitem.iItem   = m_pComboGroup->GetCount();
			m_pComboGroup->InsertItem (&cbitem);
		}
		nIdx = m_pComboGroup->FindStringExact(-1, pDoc->GetGroup());
		if (nIdx != CB_ERR)
			m_pComboGroup->SetCurSel(nIdx);
		else
			m_pComboGroup->SetCurSel(0);

		//
		// Update Role combobox:
		SetDefaultRole();
		pos = m_lRole.GetHeadPosition();
		while (pos != NULL)
		{
			CaRole* pRole = (CaRole*)m_lRole.GetNext(pos);
			cbitem.pszText = (LPTSTR)(LPCTSTR)pRole->GetName();
			cbitem.lParam  = (LPARAM)pRole;
			cbitem.iItem   = m_pComboRole->GetCount();
			m_pComboRole->InsertItem (&cbitem);
		}
		nIdx = m_pComboRole->FindStringExact(-1, pDoc->GetRole());
		if (nIdx != CB_ERR)
			m_pComboRole->SetCurSel(nIdx);
		else
			m_pComboRole->SetCurSel(0);
#endif
	}
	catch(...)
	{

	}
	m_bUpdateLoading = FALSE;
}

CuSqlQueryControl* CfMainFrame::GetSqlqueryControl()
{
	CdSqlQuery* pDoc = (CdSqlQuery*)GetSqlDoc();
	ASSERT(pDoc);
	if (!pDoc)
		return FALSE;
	CuSqlQueryControl* pCtrl = pDoc->GetSqlQueryCtrl();
	ASSERT(pCtrl && IsWindow (pCtrl->m_hWnd));
	if (pCtrl && IsWindow (pCtrl->m_hWnd))
		return pCtrl;
	return NULL;
}

BOOL CfMainFrame::SetConnection(BOOL bDisplayMessage)
{
	CdSqlQuery* pDoc = (CdSqlQuery*)GetSqlDoc();
	ASSERT(pDoc);
	if (!pDoc)
		return FALSE;
	CString strNode = pDoc->GetNode();
	CString strServer = pDoc->GetServer();
	CString strUser = pDoc->GetUser();
	CString strDatabase = pDoc->GetDatabase();
	if (strDatabase.IsEmpty() || strDatabase.CompareNoCase(m_strDefaultDatabase) == 0)
	{
		if (bDisplayMessage)
			AfxMessageBox (IDS_PLEASE_SELECT_DATABASE);
		return FALSE;
	}
	if (strServer.CompareNoCase(m_strDefaultServer) == 0)
		strServer = _T("");
	if (strUser.CompareNoCase(m_strDefaultUser) == 0)
		strUser = _T("");

	CString strItem;
	CString strConnectionOption = _T("");
#if defined (_OPTION_GROUPxROLE)
	BOOL bRolePassword = FALSE;
	BOOL bOne = TRUE;
	CString strGroup = pDoc->GetGroup();
	CString strRole = pDoc->GetRole();
	if (strGroup.CompareNoCase(m_strDefaultGroup) == 0)
		strGroup = _T("");
	if (strRole.CompareNoCase(m_strDefaultRole) == 0)
		strRole = _T("");
	if (!strGroup.IsEmpty())
	{
		strItem.Format(_T("-G%s"), (LPCTSTR)strGroup);
		strConnectionOption += strItem;
		bOne = FALSE;
	}
	if (!strRole.IsEmpty())
	{
		if (!bOne)
			strConnectionOption += _T(",");
		bOne = FALSE;
		strItem.Format(_T("-R%s"), (LPCTSTR)strRole);
		if (!pDoc->RolePassword())
		{
			CString strPwd = pDoc->GetRolePassword();
			if (!strPwd.IsEmpty())
				strItem.Format(_T("-R%s/%s"), (LPCTSTR)strRole, (LPCTSTR)strPwd);
		}
		strConnectionOption += strItem;
	}
#endif

	CuSqlQueryControl* pCtrl = pDoc->GetSqlQueryCtrl();
	ASSERT(pCtrl && IsWindow (pCtrl->m_hWnd));
	if (pCtrl && IsWindow (pCtrl->m_hWnd))
	{
		if (strConnectionOption.IsEmpty())
			pCtrl->Initiate(strNode, strServer, strUser);
		else
			pCtrl->Initiate2(strNode, strServer, strUser, strConnectionOption);
		UINT nFlag = pDoc->GetDbFlag();
		if (nFlag > 0)
			pCtrl->SetDatabaseStar(strDatabase, nFlag);
		else
			pCtrl->SetDatabase(strDatabase);
		return TRUE;
	}
	return FALSE;
}

int CfMainFrame::SetDefaultDatabase(BOOL bCleanCombo)
{
	m_pComboDatabase = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_DATABASE);
	if (bCleanCombo)
	{
		while (!m_lDatabase.IsEmpty())
			delete m_lDatabase.RemoveHead();
		m_pComboDatabase->ResetContent();
	}
	//
	// Initially, put the item text <Please Select a Database> in the Combobox database:
	COMBOBOXEXITEM cbitem;
	memset (&cbitem, 0, sizeof(cbitem));
	cbitem.mask = CBEIF_IMAGE | CBEIF_TEXT | CBEIF_SELECTEDIMAGE | CBEIF_LPARAM ;
	cbitem.iImage = 0;
	cbitem.iSelectedImage = 0;
	cbitem.pszText = (LPTSTR)(LPCTSTR)m_pDefaultDatabase->GetName();
	cbitem.lParam  = (LPARAM)m_pDefaultDatabase;
	cbitem.iItem   = m_pComboDatabase->GetCount();
	int nSel = m_pComboDatabase->InsertItem (&cbitem);
	m_pComboDatabase->SetCurSel(nSel);
	CdSqlQuery* pDoc = (CdSqlQuery*)GetSqlDoc();
	ASSERT(pDoc);
	if (pDoc && !m_bUpdateLoading)
	{
		pDoc->SetDatabase(_T(""), 0);
	}
	return nSel;
}

int CfMainFrame::SetDefaultServer(BOOL bCleanCombo)
{
	m_pComboServer = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_SERVER);
	if (bCleanCombo)
	{
		while (!m_lServer.IsEmpty())
			delete m_lServer.RemoveHead();
		m_pComboServer->ResetContent();
	}
	
	COMBOBOXEXITEM cbitem;
	memset (&cbitem, 0, sizeof(cbitem));
	cbitem.mask = CBEIF_IMAGE | CBEIF_TEXT | CBEIF_SELECTEDIMAGE | CBEIF_LPARAM ;
	cbitem.iImage = 0;
	cbitem.iSelectedImage = 0;
	cbitem.pszText = (LPTSTR)(LPCTSTR)m_pDefaultServer->GetName();
	cbitem.lParam  = (LPARAM)m_pDefaultServer;
	cbitem.iItem   = m_pComboServer->GetCount();
	int nSel = m_pComboServer->InsertItem (&cbitem);
	m_pComboServer->SetCurSel(nSel);

	CdSqlQuery* pDoc = (CdSqlQuery*)GetSqlDoc();
	ASSERT(pDoc);
	if (pDoc && !m_bUpdateLoading)
	{
		pDoc->SetServer(m_pDefaultServer->GetName());
	}
	
	return nSel;
}


int CfMainFrame::SetDefaultUser(BOOL bCleanCombo)
{
	m_pComboUser = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_USER);
	if (bCleanCombo)
	{
		while (!m_lUser.IsEmpty())
			delete m_lUser.RemoveHead();
		m_pComboUser->ResetContent();
	}

	COMBOBOXEXITEM cbitem;
	memset (&cbitem, 0, sizeof(cbitem));
	cbitem.mask = CBEIF_IMAGE | CBEIF_TEXT | CBEIF_SELECTEDIMAGE | CBEIF_LPARAM ;
	cbitem.iImage = 0;
	cbitem.iSelectedImage = 0;
	cbitem.pszText = (LPTSTR)(LPCTSTR)m_pDefaultUser->GetName();
	cbitem.lParam  = (LPARAM)m_pDefaultUser;
	cbitem.iItem   = m_pComboUser->GetCount();
	int nSel = m_pComboUser->InsertItem (&cbitem);
	m_pComboUser->SetCurSel(nSel);

	CdSqlQuery* pDoc = (CdSqlQuery*)GetSqlDoc();
	ASSERT(pDoc);
	if (pDoc && !m_bUpdateLoading)
	{
		pDoc->SetUser(m_pDefaultUser->GetName());
	}

	return nSel;
}

#if defined (_OPTION_GROUPxROLE)
int CfMainFrame::SetDefaultGroup(BOOL bCleanCombo)
{
	m_pComboGroup = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_GROUP);
	if (bCleanCombo)
	{
		while (!m_lGroup.IsEmpty())
			delete m_lGroup.RemoveHead();
		m_pComboGroup->ResetContent();
	}

	COMBOBOXEXITEM cbitem;
	memset (&cbitem, 0, sizeof(cbitem));
	cbitem.mask = CBEIF_IMAGE | CBEIF_TEXT | CBEIF_SELECTEDIMAGE | CBEIF_LPARAM ;
	cbitem.iImage = 0;
	cbitem.iSelectedImage = 0;
	cbitem.pszText = (LPTSTR)(LPCTSTR)m_pDefaultGroup->GetName();
	cbitem.lParam  = (LPARAM)m_pDefaultGroup;
	cbitem.iItem   = m_pComboGroup->GetCount();
	int nSel = m_pComboGroup->InsertItem (&cbitem);
	m_pComboGroup->SetCurSel(nSel);

	CdSqlQuery* pDoc = (CdSqlQuery*)GetSqlDoc();
	ASSERT(pDoc);
	if (pDoc && !m_bUpdateLoading)
	{
		pDoc->SetGroup(m_pDefaultGroup->GetName());
	}

	return nSel;
}

int CfMainFrame::SetDefaultRole(BOOL bCleanCombo)
{
	m_pComboRole = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_ROLE);
	if (bCleanCombo)
	{
		while (!m_lRole.IsEmpty())
			delete m_lRole.RemoveHead();
		m_pComboRole->ResetContent();
	}

	COMBOBOXEXITEM cbitem;
	memset (&cbitem, 0, sizeof(cbitem));
	cbitem.mask = CBEIF_IMAGE | CBEIF_TEXT | CBEIF_SELECTEDIMAGE | CBEIF_LPARAM ;
	cbitem.iImage = 0;
	cbitem.iSelectedImage = 0;
	cbitem.pszText = (LPTSTR)(LPCTSTR)m_pDefaultRole->GetName();
	cbitem.lParam  = (LPARAM)m_pDefaultRole;
	cbitem.iItem   = m_pComboRole->GetCount();
	int nSel = m_pComboRole->InsertItem (&cbitem);
	m_pComboRole->SetCurSel(nSel);

	CdSqlQuery* pDoc = (CdSqlQuery*)GetSqlDoc();
	ASSERT(pDoc);
	if (pDoc && !m_bUpdateLoading)
	{
		pDoc->SetRole(m_pDefaultRole->GetName());
	}

	return nSel;
}

CString CfMainFrame::RequestRolePassword(LPCTSTR lpszRole)
{
	CxDlgInputPassword dlgPassword;
	dlgPassword.m_strRole = lpszRole;
	int nAnswer = dlgPassword.DoModal();
	if (nAnswer == IDOK)
		return dlgPassword.m_strPassword;
	return _T("");
}

BOOL CfMainFrame::CheckRolePassword()
{
	BOOL bCheckOK = TRUE;
	CdSqlQuery* pDoc = (CdSqlQuery*)GetSqlDoc();
	ASSERT(pDoc);
	if (!pDoc)
		return FALSE;
	CuSqlQueryControl* pCtrl = pDoc->GetSqlQueryCtrl();
	ASSERT(pCtrl);
	if (!pCtrl)
		return FALSE;
	ASSERT(IsWindow (pCtrl->m_hWnd));
	if (!IsWindow (pCtrl->m_hWnd))
		return FALSE;
	CString strNode = pDoc->GetNode();
	CString strServer = pDoc->GetServer();
	CString strUser = pDoc->GetUser();
	CString strDatabase = pDoc->GetDatabase();
	CString strGroup = pDoc->GetGroup();
	CString strRole = pDoc->GetRole();
	if (strServer.CompareNoCase(m_strDefaultServer) == 0)
		strServer = _T("");
	if (strUser.CompareNoCase(m_strDefaultUser) == 0)
		strUser = _T("");
	if (strGroup.CompareNoCase(m_strDefaultGroup) == 0)
		strGroup = _T("");
	if (strRole.CompareNoCase(m_strDefaultRole) == 0)
		strRole = _T("");

	//
	// Set the role password;
	if (!strRole.IsEmpty() && pDoc->RolePassword())
	{
		CString strRolePassword = RequestRolePassword(strRole);
		if (!strRolePassword.IsEmpty())
		{
			BOOL bOne = TRUE;
			CString strItem;
			CString strConnectionOption = _T("");
			if (!strGroup.IsEmpty())
			{
				strItem.Format(_T("-G%s"), (LPCTSTR)strGroup);
				strConnectionOption += strItem;
				bOne = FALSE;
			}
			if (!bOne)
				strConnectionOption += _T(",");
			strItem.Format(_T("-R%s/%s"), (LPCTSTR)strRole, (LPCTSTR)strRolePassword);
			strConnectionOption += strItem;
			pCtrl->Initiate2(strNode, strServer, strUser, strConnectionOption);
			BOOL bNeedPasswordAgain = FALSE;
			long lResult = pCtrl->ConnectIfNeeded(1);
			if (lResult != 0) // -30140 (error password or role not exists)
			{
				bNeedPasswordAgain = TRUE;
				strRolePassword = _T("");
				bCheckOK = FALSE;
			}
			pDoc->RolePassword(strRolePassword);
			pDoc->RolePassword(bNeedPasswordAgain);
		}
	}

	return bCheckOK;
}
#endif // _OPTION_GROUPxROLE

void CfMainFrame::SetIdicatorAutocomit(int nOnOff)
{
	HICON icon = NULL;
	CString strText;
	if (nOnOff == 0)
	{
		// Autocommit = OFF
		icon = (HICON)::LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_AUTOCOMIT_OFF), IMAGE_ICON, 16, 16, LR_SHARED);
		strText.LoadString (ID_INDICATOR_AUTOCOMMIT_0);
	}
	else
	{
		// Autocommit = ON
		icon = (HICON)::LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_AUTOCOMIT_ON), IMAGE_ICON, 16, 16, LR_SHARED);
		strText.LoadString (ID_INDICATOR_AUTOCOMMIT_1);
	}
	m_wndStatusBar.GetStatusBarCtrl().SetIcon(AUTOCOMMIT_POS, icon);
	m_wndStatusBar.SetPaneText(AUTOCOMMIT_POS, strText);
}


void CfMainFrame::SetIdicatorReadLock(int nOnOff)
{
#if defined (_SHOW_INDICATOR_READLOCK_)
	HICON icon = NULL;
	CString strText;
	if (nOnOff == 0)
	{
		// Autocommit = OFF
		icon = (HICON)::LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_READLOCK0), IMAGE_ICON, 16, 16, LR_SHARED);
		strText.LoadString (ID_INDICATOR_READLOCK_0);
	}
	else
	{
		// Autocommit = ON
		icon = (HICON)::LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_READLOCK1), IMAGE_ICON, 16, 16, LR_SHARED);
		strText.LoadString (ID_INDICATOR_READLOCK_1);
	}
	m_wndStatusBar.GetStatusBarCtrl().SetIcon(READLOCK_POS, icon);
	m_wndStatusBar.SetPaneText(READLOCK_POS, strText);
#endif
}


int CfMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	int i;
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	CdSqlQuery* pDoc = GetSqlDoc();
	ASSERT(pDoc);
	if (!pDoc)
		return -1;

	UINT nStyle = WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP;
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT | TBSTYLE_TRANSPARENT, nStyle) ||
	    !m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}
	CToolBarCtrl& tbctrl = m_wndToolBar.GetToolBarCtrl();

	//
	// The Raw Button is a Check Button:
	int nIndex = m_wndToolBar.CommandToIndex (IDM_TRACE);
	m_wndToolBar.SetButtonStyle (nIndex, TBBS_CHECKBOX);
	CuSqlQueryControl* pCtrl = pDoc->GetSqlQueryCtrl();
	ASSERT(pCtrl && IsWindow (pCtrl->m_hWnd));
	if (!(pCtrl || IsWindow (pCtrl->m_hWnd)))
		return -1;
	CMenu* pMenu = GetMenu();
	if (pCtrl->IsUsedTracePage())
	{
		tbctrl.CheckButton(IDM_TRACE, TRUE);
		if (pMenu)
			pMenu->CheckMenuItem(IDM_TRACE, MF_CHECKED|MF_BYCOMMAND);
	}
	else
	{
		tbctrl.CheckButton(IDM_TRACE, FALSE);
		if (pMenu)
			pMenu->CheckMenuItem(IDM_TRACE, MF_UNCHECKED|MF_BYCOMMAND);
	}

#if defined (_TOOTBAR_BTN_TEXT_)
	//
	// Set the Button Text:
	CString strTips;
	UINT nID = 0;
	int nButtonCount = tbctrl.GetButtonCount();
	for (i=0; i<nButtonCount; i++)
	{
		if (m_wndToolBar.GetButtonStyle(i) & TBBS_SEPARATOR)
			continue;
		nID = m_wndToolBar.GetItemID(i);
		if (strTips.LoadString(nID))
		{
			int nFound = strTips.Find (_T("\n"));
			if (nFound != -1)
				strTips = strTips.Mid (nFound+1);
			m_wndToolBar.SetButtonText(i, strTips);
		}
	}
	CRect temp;
	m_wndToolBar.GetItemRect(0,&temp);
	m_wndToolBar.SetSizes( CSize(temp.Width(),temp.Height()), CSize(16,16));
#endif

#if defined (_TOOTBAR_HOT_IMAGE_)
	//
	// Set the Hot Image:
	CImageList image;
	image.Create(m_nIDHotImage, 16, 0, RGB (128, 128, 128));
	tbctrl.SetHotImageList (&image);
	image.Detach();
#endif

	if (!m_wndStatusBar.Create(this) || !m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	//
	// Create the image list for the comboboxes:
	m_ImageListNode.Create(IDB_NODES, 16, 1, RGB(255, 0, 255));
	m_ImageListServer.Create(IDB_SERVER_OTHER, 16, 1, RGB(255, 0, 255));
	m_ImageListUser.Create(IDB_USER, 16, 1, RGB(255, 0, 255));
	m_ImageListDatabase.Create(IDB_DATABASE, 16, 1, RGB(255, 0, 255));
	UINT narrayImage[2] = {IDB_DATABASE_DISTRIBUTED, IDB_DATABASE_COORDINATOR};
	for (i = 0; i<2; i++)
	{
		CImageList im;
		if (im.Create(narrayImage[i], 16, 1, RGB(255, 0, 255)))
		{
			HICON hIconBlock = im.ExtractIcon(0);
			if (hIconBlock)
			{
				m_ImageListDatabase.Add (hIconBlock);
				DestroyIcon (hIconBlock);
			}
		}
	}
#if defined (_OPTION_GROUPxROLE)
	m_ImageListGroup.Create(16, 16, TRUE, 1, 1);
	m_ImageListRole.Create(16, 16, TRUE, 1, 1);
	CImageList imageList;
	HICON hIcon;
	imageList.Create (IDB_INGRESOBJECT3_16x16, 16, 0, RGB(255,0,255));
	hIcon = imageList.ExtractIcon (IM_GROUP);
	m_ImageListGroup.Add (hIcon);
	DestroyIcon (hIcon);
	hIcon = imageList.ExtractIcon (IM_ROLE);
	m_ImageListRole.Add (hIcon);
	DestroyIcon (hIcon);
#endif

	if (!m_wndDlgBar.Create(this, IDR_MAINFRAME, CBRS_ALIGN_ANY, AFX_IDW_DIALOGBAR))
	{
		TRACE0("Failed to create dialogbar\n");
		return -1; // fail to create
	}

	m_pComboNode    = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_NODE);
	m_pComboServer  = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_SERVER);
	m_pComboUser    = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_USER);
	m_pComboDatabase= (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_DATABASE);
#if defined (_OPTION_GROUPxROLE)
	m_pComboGroup = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_GROUP);
	m_pComboRole  = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_ROLE);
#endif
	
	m_pComboNode->SetImageList(&m_ImageListNode);
	m_pComboServer->SetImageList(&m_ImageListServer);
	m_pComboUser->SetImageList(&m_ImageListUser);
	m_pComboDatabase->SetImageList(&m_ImageListDatabase);
#if defined (_OPTION_GROUPxROLE)
	m_pComboGroup->SetImageList(&m_ImageListGroup);
	m_pComboRole->SetImageList(&m_ImageListRole);
	//Because these are optional, the .rc file has NOT WS_VISIBLE
	m_pComboGroup->ShowWindow(TRUE);
	m_pComboRole->ShowWindow(TRUE);
#endif

	if (!m_wndReBar.Create(this) ||
	    !m_wndReBar.AddBar(&m_wndToolBar) ||
	    !m_wndReBar.AddBar(&m_wndDlgBar))
	{
		TRACE0("Failed to create rebar\n");
		return -1;      // fail to create
	}

	UINT nExtraStyle = CBRS_FLYBY | CBRS_TOOLTIPS | CBRS_SIZE_DYNAMIC /*| CBRS_GRIPPER*/ | CBRS_BORDER_3D;
	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() | nExtraStyle);
	m_wndDlgBar.SetBarStyle(m_wndDlgBar.GetBarStyle() | nExtraStyle);

	SetDefaultServer();
	SetDefaultUser();
	SetDefaultDatabase();
#if defined (_OPTION_GROUPxROLE)
	SetDefaultGroup();
	SetDefaultRole();
#endif

	//
	// Handle the command lines:
	UINT nError = HandleCommandLine(pDoc);
	if (nError != 0)
	{
		CString strMsg = _T("Command line error");
		if (nError & MASK_SETNODE)
			strMsg.LoadString (IDS_CMDERROR_NODE);
		else
		if (nError & MASK_SETSERVER)
			strMsg.LoadString (IDS_CMDERROR_SERVER);
		else
		if (nError & MASK_SETUSER)
			strMsg.LoadString (IDS_CMDERROR_USER);
		else
		if (nError & MASK_SETDATABASE)
			strMsg.LoadString (IDS_CMDERROR_DATABASE);
		else
		if (nError & MASK_SETGROUP)
			strMsg.LoadString (IDS_CMDERROR_GROUP);
		else
		if (nError & MASK_SETROLE)
			strMsg.LoadString (IDS_CMDERROR_ROLE);
		else
		if (nError & MASK_UNKNOWN)
			strMsg.LoadString (IDS_INVALID_CMDLINE_XX);
		AfxMessageBox (strMsg);
		return -1;
	}
	CString strItem;
	BOOL bSetCommandLine = 
	    (m_uSetArg & MASK_SETNODE)||(m_uSetArg & MASK_SETSERVER)||
	    (m_uSetArg & MASK_SETUSER)||(m_uSetArg & MASK_SETGROUP)||(m_uSetArg & MASK_SETROLE);

	if (!bSetCommandLine)
	{
		OnDropDownNode();

		int nSel = m_pComboNode->GetCurSel();
		if (nSel != CB_ERR)
		{
			m_pComboNode->GetLBText(nSel, strItem);
			if (!strItem.IsEmpty())
				pDoc->SetNode(strItem);
		}
		m_pComboServer->GetLBText(0, strItem);
		pDoc->SetServer(strItem);
		m_pComboUser->GetLBText(0, strItem);
		pDoc->SetUser(strItem);
		nSel = m_pComboDatabase->GetCurSel();
		if (nSel != CB_ERR)
		{
			m_pComboDatabase->GetLBText(nSel, strItem);
			if (!strItem.IsEmpty() && strItem.CompareNoCase(m_strDefaultDatabase) != 0)
			{
				UINT nFlag = 0;
				CaDatabase* pDb = (CaDatabase*)m_pComboDatabase->GetItemDataPtr(nSel);
				if (pDb)
					nFlag = pDb->GetFlag();
				pDoc->SetDatabase(strItem, nFlag);
			}
		}
#if defined (_OPTION_GROUPxROLE)
		m_pComboGroup->GetLBText(0, strItem);
		pDoc->SetGroup(strItem);
		m_pComboRole->GetLBText(0, strItem);
		pDoc->SetRole(strItem);
#endif
	}

	SetIdicatorAutocomit(0);
	SetIdicatorReadLock (0);

	PermanentState(TRUE);

	return 0;
}

BOOL CfMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CfMainFrame diagnostics

#ifdef _DEBUG
void CfMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CfMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CfMainFrame message handlers


void CfMainFrame::OnClear() 
{
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (!pCtrl)
		return;
	pCtrl->Clear();
}

void CfMainFrame::OnOpenQuery() 
{
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (!pCtrl)
		return;
	pCtrl->Open();
}

void CfMainFrame::OnSaveQuery() 
{
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (!pCtrl)
		return;
	pCtrl->Save();
}

void CfMainFrame::OnAssistant() 
{
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (!pCtrl)
		return;
	if (!SetConnection())
		return;
	BOOL bOK = TRUE;
#if defined (_OPTION_GROUPxROLE)
	bOK = CheckRolePassword();
#endif
	if (!bOK)
		return;
	pCtrl->SqlAssistant();
}

void CfMainFrame::OnRun() 
{
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (!pCtrl)
		return;
	if (!SetConnection())
		return;
	BOOL bOK = TRUE;
#if defined (_OPTION_GROUPxROLE)
	bOK = CheckRolePassword();
#endif
	if (!bOK)
		return;
	pCtrl->Run();
}

void CfMainFrame::OnQep() 
{
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (!pCtrl)
		return;
	if (!SetConnection())
		return;
	BOOL bOK = TRUE;
#if defined (_OPTION_GROUPxROLE)
	bOK = CheckRolePassword();
#endif
	if (!bOK)
		return;
	pCtrl->Qep();
}

void CfMainFrame::OnXml() 
{
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (!pCtrl)
		return;
	if (!SetConnection())
		return;
	BOOL bOK = TRUE;
#if defined (_OPTION_GROUPxROLE)
	bOK = CheckRolePassword();
#endif
	if (!bOK)
		return;
	pCtrl->Xml();
}

void CfMainFrame::OnTrace() 
{
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (!pCtrl)
		return;
	pCtrl->EnableTrace();

	CToolBarCtrl& tbctrl = m_wndToolBar.GetToolBarCtrl();
	int nIndex = m_wndToolBar.CommandToIndex (IDM_TRACE);
	CMenu* pMenu = GetMenu();
	if (pCtrl->IsUsedTracePage())
	{
		tbctrl.CheckButton(IDM_TRACE, TRUE);
		if (pMenu)
			pMenu->CheckMenuItem(IDM_TRACE, MF_CHECKED|MF_BYCOMMAND);
	}
	else
	{
		tbctrl.CheckButton(IDM_TRACE, FALSE);
		if (pMenu)
			pMenu->CheckMenuItem(IDM_TRACE, MF_UNCHECKED|MF_BYCOMMAND);
	}
}

void CfMainFrame::OnSetting() 
{
	USES_CONVERSION;
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (!pCtrl)
		return;

	CString strSqlSettingCaption;
	strSqlSettingCaption.LoadString(IDS_APP_TITLE_XX);
	LPCOLESTR lpCaption = T2COLE((LPTSTR)(LPCTSTR)strSqlSettingCaption);

	IUnknown* pUk = pCtrl->GetControlUnknown();
	ISpecifyPropertyPagesPtr pSpecifyPropertyPages = pUk;
	CAUUID pages;
	HRESULT hr = pSpecifyPropertyPages->GetPages( &pages );
	if(FAILED(hr ))
		return;

	m_bPropertyChange = FALSE;
	hr = OleCreatePropertyFrame(
		m_hWnd,
		10,
		10,
		lpCaption,
		1,
		(IUnknown**) &pUk,
		pages.cElems,
		pages.pElems,
		0, 
		0L,
		NULL );

	CoTaskMemFree( (void*) pages.pElems );

	//
	// Save the properties to file IIGUIS.PRF:
	BOOL bSave2Default = FALSE;
	CMenu* pMenu = GetMenu();
	if (pMenu)
	{
		UINT nState = pMenu->GetMenuState(IDM_PREFERENCE_SAVE, MF_BYCOMMAND);
		if (nState == MF_CHECKED)
			bSave2Default = TRUE;
	}
	if (m_bPropertyChange && bSave2Default)
	{
		BOOL bOk = SaveStreamInit(pCtrl->GetControlUnknown(), _T("SqlQuery"));
		if (!bOk)
			AfxMessageBox (IDS_FAILED_2_SAVESTORAGE);
	}
}

void CfMainFrame::OnUpdateClear(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (pCtrl)
	{
		bEnable = pCtrl->IsClearEnable();
	}
	pCmdUI->Enable(bEnable);
}

void CfMainFrame::OnUpdateOpenQuery(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (pCtrl)
	{
		bEnable = pCtrl->IsOpenEnable();
	}
	pCmdUI->Enable(bEnable);
}

void CfMainFrame::OnUpdateSaveQuery(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (pCtrl)
	{
		bEnable = pCtrl->IsClearEnable();
	}
	pCmdUI->Enable(bEnable);
}

void CfMainFrame::OnUpdateAssistant(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (pCtrl)
	{
		if (SetConnection(FALSE))
			bEnable = TRUE;
	}
	pCmdUI->Enable(bEnable);
}

void CfMainFrame::OnUpdateRun(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (pCtrl && SetConnection(FALSE))
		bEnable = pCtrl->IsRunEnable();
	pCmdUI->Enable(bEnable);
}

void CfMainFrame::OnUpdateQep(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (pCtrl && SetConnection(FALSE))
		bEnable = pCtrl->IsQepEnable();
	pCmdUI->Enable(bEnable);
}

void CfMainFrame::OnUpdateTrace(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (pCtrl)
		bEnable = pCtrl->IsTraceEnable();

	CToolBarCtrl& tbctrl = m_wndToolBar.GetToolBarCtrl();
	int nIndex = m_wndToolBar.CommandToIndex (IDM_TRACE);
	CMenu* pMenu = GetMenu();
	if (pCtrl->IsUsedTracePage())
	{
		tbctrl.CheckButton(IDM_TRACE, TRUE);
		if (pMenu)
			pMenu->CheckMenuItem(IDM_TRACE, MF_CHECKED|MF_BYCOMMAND);
	}
	else
	{
		tbctrl.CheckButton(IDM_TRACE, FALSE);
		if (pMenu)
			pMenu->CheckMenuItem(IDM_TRACE, MF_UNCHECKED|MF_BYCOMMAND);
	}
	pCmdUI->Enable(bEnable);
	//
	// ALSO Enable/Disable the combo User, Group, Role:
	if ((m_uSetArg & MASK_SETNODE) ||
	    (m_uSetArg & MASK_SETDATABASE) ||
	    (m_uSetArg & MASK_SETSERVER) ||
	    (m_uSetArg & MASK_SETUSER) || 
	    (m_uSetArg & MASK_SETGROUP) || 
	    (m_uSetArg & MASK_SETROLE))
	{
		return; // command line arg set (they are always disabled)
	}

	bEnable = TRUE;
	if (pCtrl && pCtrl->GetConnectParamInfo() == INGRESVERS_NOTOI)
		bEnable = FALSE;
	m_pComboUser    = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_USER);
	m_pComboUser->EnableWindow(bEnable);
#if defined (_OPTION_GROUPxROLE)
	m_pComboGroup = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_GROUP);
	m_pComboRole  = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_ROLE);
	m_pComboGroup->EnableWindow(bEnable);
	m_pComboRole->EnableWindow(bEnable);
#endif
}

void CfMainFrame::OnUpdateXml(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (pCtrl && SetConnection(FALSE))
		bEnable = pCtrl->IsXmlEnable();
	pCmdUI->Enable(bEnable);
}

void CfMainFrame::OnUpdateSetting(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (pCtrl)
		bEnable = TRUE;
	pCmdUI->Enable(bEnable);
}


BOOL CfMainFrame::QueryNode(LPCTSTR lpszCmdNode)
{
	int nIdx = CB_ERR;
	int nLocal = -1;
	BOOL bOK = TRUE;
	try
	{
		CdSqlQuery* pDoc = (CdSqlQuery*)GetSqlDoc();
		CString strNode = pDoc->GetNode();
		m_pComboNode    = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_NODE);
#if defined(_MINI_CACHE)
		if ((m_nAlreadyRefresh & REFRESH_NODE) && !lpszCmdNode)
			return bOK; // Nodes have been queried !
#endif
		CWaitCursor doWaitCursor;
		CTypedPtrList< CObList, CaDBObject* > lNew;
		if (lpszCmdNode)
		{
			CaNode* pCmdNode = new CaNode (lpszCmdNode);
			lNew.AddTail(pCmdNode);
			if (pCmdNode->GetName().CompareNoCase(_T("(local)")) == 0)
				pCmdNode->LocalNode();
		}
		else
		{
			bOK = DML_QueryNode(pDoc, lNew);
		}

		if (bOK)
		{
			while (!m_lNode.IsEmpty())
				delete m_lNode.RemoveHead();
			m_pComboNode->ResetContent();

			COMBOBOXEXITEM cbitem;
			memset (&cbitem, 0, sizeof(cbitem));
			cbitem.mask = CBEIF_IMAGE | CBEIF_TEXT | CBEIF_SELECTEDIMAGE | CBEIF_LPARAM ;
			POSITION pos = lNew.GetHeadPosition();
			while (!lNew.IsEmpty())
			{
				CaNode* pNode = (CaNode*)lNew.RemoveHead();
				if (pNode->IsLocalNode())
				{
					cbitem.iImage = 1;
					cbitem.iSelectedImage = 1;
				}
				else
				{
					cbitem.iImage = 0;
					cbitem.iSelectedImage = 0;
				}

				cbitem.pszText = (LPTSTR)(LPCTSTR)pNode->GetName();
				cbitem.lParam  = (LPARAM)pNode;
				cbitem.iItem   = m_pComboNode->GetCount();
				nIdx = m_pComboNode->InsertItem (&cbitem);
				if (pNode->IsLocalNode())
					nLocal = nIdx;

				m_lNode.AddTail(pNode);
			}

			if (!strNode.IsEmpty())
			{
				nIdx = m_pComboNode->FindStringExact(-1, strNode);
				if (nIdx != CB_ERR)
					m_pComboNode->SetCurSel(nIdx);
				else
					m_pComboNode->SetCurSel(0);
			}
			else
			if (nLocal != -1)
			{
				m_pComboNode->SetCurSel (nLocal);
			}
			else
				m_pComboNode->SetCurSel (0);

			m_nAlreadyRefresh |= REFRESH_NODE;
		}
		return bOK;
	}
	catch (CeNodeException e)
	{
		SetForegroundWindow();
		BOOL bStarted = INGRESII_IsRunning();
		if (!bStarted)
			AfxMessageBox (IDS_MSG_INGRES_NOT_START);
		else
			AfxMessageBox (e.GetReason());
		bOK=FALSE;
	}
	catch(...)
	{
		bOK=FALSE;
	}
	return bOK;
}

BOOL CfMainFrame::QueryServer(LPCTSTR lpszCmdServer)
{
	BOOL bOK = TRUE;
	int nIdx = CB_ERR;
#if defined(_MINI_CACHE)
	if ((m_nAlreadyRefresh & REFRESH_SERVER) && !lpszCmdServer)
		return bOK; // Servers have been queried !
#endif
	try
	{
		CWaitCursor doWaitCursor;
		CdSqlQuery* pDoc = (CdSqlQuery*)GetSqlDoc();
		m_pComboNode    = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_NODE);
		m_pComboServer  = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_SERVER);
		nIdx = m_pComboNode->GetCurSel();
		if (nIdx == CB_ERR)
			return FALSE;

		CString strServer = pDoc->GetServer();
		CTypedPtrList< CObList, CaDBObject* > lNew;
		CaNode* pNode = (CaNode*)m_pComboNode->GetItemDataPtr(nIdx);

		if (lpszCmdServer)
		{
			CaNodeServer* pCmdServer = new CaNodeServer (pNode->GetName(), lpszCmdServer, pNode->IsLocalNode());
			lNew.AddTail(pCmdServer);
		}
		else
		{
			bOK = DML_QueryServer(pDoc, pNode, lNew);
		}
		if (bOK)
		{
			while (!m_lServer.IsEmpty())
				delete m_lServer.RemoveHead();
			m_pComboServer->ResetContent();
			COMBOBOXEXITEM cbitem;
			memset (&cbitem, 0, sizeof(cbitem));
			cbitem.mask = CBEIF_IMAGE | CBEIF_TEXT | CBEIF_SELECTEDIMAGE | CBEIF_LPARAM ;
			cbitem.iImage = 0;
			cbitem.iSelectedImage = 0;
			int nDefault = SetDefaultServer();
			POSITION pos = lNew.GetHeadPosition();
			while (!lNew.IsEmpty())
			{
				CaNodeServer* pNodeServer = (CaNodeServer*)lNew.RemoveHead();
				cbitem.pszText = (LPTSTR)(LPCTSTR)pNodeServer->GetName();
				cbitem.lParam  = (LPARAM)pNodeServer;
				cbitem.iItem   = m_pComboServer->GetCount();
				m_pComboServer->InsertItem (&cbitem);

				m_lServer.AddTail(pNodeServer);
			}

			if (!strServer.IsEmpty())
			{
				nIdx = m_pComboServer->FindStringExact(-1, strServer);
				if (nIdx != CB_ERR)
					m_pComboServer->SetCurSel(nIdx);
				else
					m_pComboServer->SetCurSel(nDefault);
			}
			else
			{
				m_pComboServer->SetCurSel(nDefault);
			}
			m_nAlreadyRefresh |= REFRESH_SERVER;
		}
		return bOK;
	}
	catch (CeNodeException e)
	{
		SetForegroundWindow();
		BOOL bStarted = INGRESII_IsRunning();
		if (!bStarted)
			AfxMessageBox (IDS_MSG_INGRES_NOT_START);
		else
			AfxMessageBox (e.GetReason());
		bOK=FALSE;
	}
	catch(...)
	{
		bOK=FALSE;
	}
	SetDefaultServer(TRUE);
	return bOK;
}

BOOL CfMainFrame::QueryUser()
{
	BOOL bOK = TRUE;
	int nIdx = CB_ERR;
	CdSqlQuery* pDoc = (CdSqlQuery*)GetSqlDoc();
#if defined(_MINI_CACHE)
	if (m_nAlreadyRefresh & REFRESH_USER)
		return bOK; // Users have been queried !
#endif
	CWaitCursor doWaitCursor;
	try
	{
		m_pComboNode    = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_NODE);
		m_pComboUser    = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_USER);
		nIdx = m_pComboNode->GetCurSel();
		if (nIdx == CB_ERR)
			return FALSE;

		CString strUser = pDoc->GetUser();
		CTypedPtrList< CObList, CaDBObject* > lNew;
		bOK = DML_QueryUser(pDoc, lNew);
		if (bOK)
		{
			while (!m_lUser.IsEmpty())
				delete m_lUser.RemoveHead();
			m_pComboUser->ResetContent();
			COMBOBOXEXITEM cbitem;
			memset (&cbitem, 0, sizeof(cbitem));
			cbitem.mask = CBEIF_IMAGE | CBEIF_TEXT | CBEIF_SELECTEDIMAGE | CBEIF_LPARAM ;
			cbitem.iImage = 0;
			cbitem.iSelectedImage = 0;
			int nDefault = SetDefaultUser();

			POSITION pos = lNew.GetHeadPosition();
			while (!lNew.IsEmpty())
			{
				CaUser* pUser = (CaUser*)lNew.RemoveHead();
				cbitem.pszText = (LPTSTR)(LPCTSTR)pUser->GetName();
				cbitem.lParam  = (LPARAM)pUser;
				cbitem.iItem   = m_pComboUser->GetCount();
				m_pComboUser->InsertItem (&cbitem);

				m_lUser.AddTail(pUser);
			}
			if (!strUser.IsEmpty())
			{
				nIdx = m_pComboUser->FindStringExact(-1, strUser);
				if (nIdx != CB_ERR)
					m_pComboUser->SetCurSel(nIdx);
				else
					m_pComboUser->SetCurSel(nDefault);
			}
			else
			{
				m_pComboUser->SetCurSel(nDefault);
			}
			m_nAlreadyRefresh |= REFRESH_USER;
		}
		return bOK;
	}
	catch (CeSqlException e)
	{
		bOK = FALSE;
		if (e.GetErrorCode() == -30140)
			AfxMessageBox (IDS_MSG_USER_UNAVAILABLE);
		else
			AfxMessageBox (e.GetReason());
	}
	catch(...)
	{
		bOK = FALSE;
	}
	SetDefaultUser(TRUE);
	return bOK;
}

#if defined (_OPTION_GROUPxROLE)
BOOL CfMainFrame::QueryGroup()
{
	BOOL bOK = TRUE;
	int nIdx = CB_ERR;
	CdSqlQuery* pDoc = (CdSqlQuery*)GetSqlDoc();
#if defined(_MINI_CACHE)
	if (m_nAlreadyRefresh & REFRESH_GROUP)
		return bOK; // Users have been queried !
#endif
	CWaitCursor doWaitCursor;
	try
	{
		m_pComboNode  = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_NODE);
		m_pComboGroup = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_GROUP);
		nIdx = m_pComboNode->GetCurSel();
		if (nIdx == CB_ERR)
			return FALSE;

		CString strGroup = pDoc->GetGroup();
		CTypedPtrList< CObList, CaDBObject* > lNew;
		bOK = DML_QueryGroup(pDoc, lNew);
		if (bOK)
		{
			while (!m_lGroup.IsEmpty())
				delete m_lGroup.RemoveHead();
			m_pComboGroup->ResetContent();
			COMBOBOXEXITEM cbitem;
			memset (&cbitem, 0, sizeof(cbitem));
			cbitem.mask = CBEIF_IMAGE | CBEIF_TEXT | CBEIF_SELECTEDIMAGE | CBEIF_LPARAM ;
			cbitem.iImage = 0;
			cbitem.iSelectedImage = 0;
			int nDefault = SetDefaultGroup();

			POSITION pos = lNew.GetHeadPosition();
			while (!lNew.IsEmpty())
			{
				CaGroup* pGroup = (CaGroup*)lNew.RemoveHead();
				cbitem.pszText = (LPTSTR)(LPCTSTR)pGroup->GetName();
				cbitem.lParam  = (LPARAM)pGroup;
				cbitem.iItem   = m_pComboGroup->GetCount();
				m_pComboGroup->InsertItem (&cbitem);

				m_lGroup.AddTail(pGroup);
			}
			if (!strGroup.IsEmpty())
			{
				nIdx = m_pComboGroup->FindStringExact(-1, strGroup);
				if (nIdx != CB_ERR)
					m_pComboGroup->SetCurSel(nIdx);
				else
					m_pComboGroup->SetCurSel(nDefault);
			}
			else
			{
				m_pComboGroup->SetCurSel(nDefault);
			}
			m_nAlreadyRefresh |= REFRESH_GROUP;
		}
		return bOK;
	}
	catch (CeSqlException e)
	{
		bOK = FALSE;
		if (e.GetErrorCode() == -30140)
			AfxMessageBox (IDS_MSG_GROUP_UNAVAILABLE);
		else
			AfxMessageBox (e.GetReason());
	}
	catch(...)
	{
		bOK = FALSE;
	}
	SetDefaultGroup(TRUE);
	return bOK;
}

BOOL CfMainFrame::QueryRole()
{
	BOOL bOK = TRUE;
	int nIdx = CB_ERR;
	CdSqlQuery* pDoc = (CdSqlQuery*)GetSqlDoc();
#if defined(_MINI_CACHE)
	if (m_nAlreadyRefresh & REFRESH_ROLE)
		return bOK; // Users have been queried !
#endif
	CWaitCursor doWaitCursor;
	try
	{
		m_pComboNode    = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_NODE);
		m_pComboRole    = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_ROLE);
		nIdx = m_pComboNode->GetCurSel();
		if (nIdx == CB_ERR)
			return FALSE;

		CString strRole = pDoc->GetRole();
		CTypedPtrList< CObList, CaDBObject* > lNew;
		bOK = DML_QueryRole(pDoc, lNew);
		if (bOK)
		{
			while (!m_lRole.IsEmpty())
				delete m_lRole.RemoveHead();
			m_pComboRole->ResetContent();
			COMBOBOXEXITEM cbitem;
			memset (&cbitem, 0, sizeof(cbitem));
			cbitem.mask = CBEIF_IMAGE | CBEIF_TEXT | CBEIF_SELECTEDIMAGE | CBEIF_LPARAM ;
			cbitem.iImage = 0;
			cbitem.iSelectedImage = 0;
			int nDefault = SetDefaultRole();

			POSITION pos = lNew.GetHeadPosition();
			while (!lNew.IsEmpty())
			{
				CaRole* pRole = (CaRole*)lNew.RemoveHead();
				cbitem.pszText = (LPTSTR)(LPCTSTR)pRole->GetName();
				cbitem.lParam  = (LPARAM)pRole;
				cbitem.iItem   = m_pComboRole->GetCount();
				m_pComboRole->InsertItem (&cbitem);

				m_lRole.AddTail(pRole);
			}
			if (!strRole.IsEmpty())
			{
				nIdx = m_pComboRole->FindStringExact(-1, strRole);
				if (nIdx != CB_ERR)
					m_pComboRole->SetCurSel(nIdx);
				else
					m_pComboRole->SetCurSel(nDefault);
			}
			else
			{
				m_pComboRole->SetCurSel(nDefault);
			}
			m_nAlreadyRefresh |= REFRESH_ROLE;
		}
		return bOK;
	}
	catch (CeSqlException e)
	{
		bOK = FALSE;
		if (e.GetErrorCode() == -30140)
			AfxMessageBox (IDS_MSG_ROLE_UNAVAILABLE);
		else
			AfxMessageBox (e.GetReason());
	}
	catch(...)
	{
		bOK = FALSE;
	}
	SetDefaultRole(TRUE);
	return bOK;
}

void CfMainFrame::OnDropDownGroup()
{
	QueryGroup();
}

void CfMainFrame::OnSelChangeGroup()
{
	CWaitCursor doWaitCursor;
	int nIdx = CB_ERR;
	int nLocal = -1;
	CdSqlQuery* pDoc = (CdSqlQuery*)GetSqlDoc();
	CString strGroup = pDoc->GetGroup();

	CString strNewGroup;
	m_pComboGroup    = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_GROUP);
	nIdx = m_pComboGroup->GetCurSel();
	if (nIdx == CB_ERR)
		return;
	m_pComboGroup->GetLBText (nIdx, strNewGroup);
	if (strNewGroup.CompareNoCase(strGroup) != 0)
	{
		pDoc->SetGroup(strNewGroup);
		CommitOldSession();
		CuSqlQueryControl* pCtrl = GetSqlqueryControl();
		if (pCtrl)
			pCtrl->SetConnectParamInfo (-1);
		pDoc->UpdateAllViews(NULL, 1);
	}
}

void CfMainFrame::OnDropDownRole()
{
	QueryRole();
}

void CfMainFrame::OnSelChangeRole()
{
	CWaitCursor doWaitCursor;
	int nIdx = CB_ERR;
	int nLocal = -1;
	CdSqlQuery* pDoc = (CdSqlQuery*)GetSqlDoc();
	CString strRole = pDoc->GetRole();
	try
	{
		CString strNewRole = _T("");
		m_pComboRole    = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_ROLE);
		nIdx = m_pComboRole->GetCurSel();
		if (nIdx == CB_ERR)
			return;
		m_pComboRole->GetLBText (nIdx, strNewRole);
		if (strNewRole.CompareNoCase(strRole) != 0)
		{
			if (strNewRole.CompareNoCase(m_strDefaultRole) != 0)
			{
				//
				// Check role password:
				CaRoleDetail detailRole;
				CaLLQueryInfo info(OBT_ROLE, pDoc->GetNode(), _T("iidbdb"));
				CString strServer = pDoc->GetServer();
				CString strUser = pDoc->GetUser();
				if (strServer.CompareNoCase(m_strDefaultServer) == 0)
					strServer = _T("");
				if (strUser.CompareNoCase(m_strDefaultUser) == 0)
					strUser = _T("");
				info.SetUser (strUser);
				info.SetServerClass (strServer);

				pDoc->SetRole(strNewRole);
				CString strRolePass = INGRESII_llQueryRolePassword (strNewRole, &info, &(theApp.GetSessionManager()));
				if (!strRolePass.IsEmpty())
					pDoc->RolePassword(TRUE);
				pDoc->RolePassword(_T(""));
				CommitOldSession();
				CuSqlQueryControl* pCtrl = GetSqlqueryControl();
				if (pCtrl)
					pCtrl->SetConnectParamInfo (-1);
				pDoc->UpdateAllViews(NULL, 1);
			}
		}
	}
	catch (CeSqlException e)
	{
		AfxMessageBox(e.GetReason());
	}
	catch (...)
	{
		TRACE0("System error: CfMainFrame::OnSelChangeRole");
	}
}


#endif // _OPTION_GROUPxROLE

BOOL CfMainFrame::QueryDatabase(LPCTSTR lpszCmdDatabase)
{
	BOOL bOK = TRUE;
	int nIdx = CB_ERR;
	CdSqlQuery* pDoc = (CdSqlQuery*)GetSqlDoc();
#if defined(_MINI_CACHE)
	if ((m_nAlreadyRefresh & REFRESH_DATABASE) && !lpszCmdDatabase)
		return bOK; // Databases have been queried !
#endif
	CWaitCursor doWaitCursor;
	try
	{
		m_pComboNode    = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_NODE);
		m_pComboDatabase= (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_DATABASE);
		nIdx = m_pComboNode->GetCurSel();
		if (nIdx == CB_ERR)
			return FALSE;

		//
		// Clean the combobox:
		while (!m_lDatabase.IsEmpty())
			delete m_lDatabase.RemoveHead();
		m_pComboDatabase->ResetContent();

		CString strDatabase = pDoc->GetDatabase();
		CTypedPtrList< CObList, CaDBObject* > lNew;
		int nSessionVersion = -1;
		if (lpszCmdDatabase)
		{
			CaDatabase* pCmdDatabase = new CaDatabase (lpszCmdDatabase, _T(""));
			lNew.AddTail(pCmdDatabase);
		}
		else
		{
			bOK = DML_QueryDatabase(pDoc, lNew, nSessionVersion);
		}
		if (bOK)
		{
			COMBOBOXEXITEM cbitem;
			memset (&cbitem, 0, sizeof(cbitem));
			cbitem.mask = CBEIF_IMAGE | CBEIF_TEXT | CBEIF_SELECTEDIMAGE | CBEIF_LPARAM ;
			cbitem.iImage = 0;
			cbitem.iSelectedImage = 0;

			POSITION pos = lNew.GetHeadPosition();
			while (!lNew.IsEmpty())
			{
				CaDatabase* pDatabase = (CaDatabase*)lNew.RemoveHead();
				cbitem.pszText = (LPTSTR)(LPCTSTR)pDatabase->GetName();
				cbitem.lParam  = (LPARAM)pDatabase;
				cbitem.iItem   = m_pComboDatabase->GetCount();
				m_pComboDatabase->InsertItem (&cbitem);

				m_lDatabase.AddTail(pDatabase);
			}
			if (!strDatabase.IsEmpty())
			{
				nIdx = m_pComboDatabase->FindStringExact(-1, strDatabase);
				if (nIdx != CB_ERR)
					m_pComboDatabase->SetCurSel(nIdx);
				else
					m_pComboDatabase->SetCurSel(0);
			}
			else
			{
				m_pComboDatabase->SetCurSel(0);
			}
			m_nAlreadyRefresh |= REFRESH_DATABASE;
			CuSqlQueryControl* pCtrl = GetSqlqueryControl();
			if (pCtrl)
				pCtrl->SetConnectParamInfo (nSessionVersion);
		}
		else
		{
			SetDefaultDatabase();
		}
		return bOK;
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason());
		bOK = FALSE;
	}
	catch(...)
	{
		bOK = FALSE;
	}

	SetDefaultDatabase();
	return bOK;
}


void CfMainFrame::OnDropDownNode()
{
	QueryNode();
}

void CfMainFrame::OnSelChangeNode()
{
	CWaitCursor doWaitCursor;
	int nIdx = CB_ERR;
	int nLocal = -1;
	CdSqlQuery* pDoc = (CdSqlQuery*)GetSqlDoc();
	CString strNode = pDoc->GetNode();

	CString strNewNode;
	m_pComboNode    = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_NODE);
	nIdx = m_pComboNode->GetCurSel();
	if (nIdx == CB_ERR)
		return;
	m_pComboNode->GetLBText (nIdx, strNewNode);
	if (strNewNode.CompareNoCase(strNode) != 0)
	{
		pDoc->SetNode(strNewNode);
		m_nAlreadyRefresh &=~REFRESH_SERVER;
		m_nAlreadyRefresh &=~REFRESH_USER;
		m_nAlreadyRefresh &=~REFRESH_DATABASE;
		SetDefaultServer(TRUE);
		SetDefaultUser(TRUE);
		SetDefaultDatabase(TRUE);
#if defined (_OPTION_GROUPxROLE)
		m_nAlreadyRefresh &=~REFRESH_GROUP;
		m_nAlreadyRefresh &=~REFRESH_ROLE;
		SetDefaultGroup(TRUE);
		SetDefaultRole(TRUE);
#endif
		CommitOldSession();
		CuSqlQueryControl* pCtrl = GetSqlqueryControl();
		if (pCtrl)
			pCtrl->SetConnectParamInfo (-1);

		pDoc->UpdateAllViews(NULL, 1);
	}
}

void CfMainFrame::OnDropDownServer()
{
	QueryServer();
}

void CfMainFrame::OnSelChangeServer()
{
	CWaitCursor doWaitCursor;
	int nIdx = CB_ERR;
	int nLocal = -1;
	CdSqlQuery* pDoc = (CdSqlQuery*)GetSqlDoc();
	CString strServer = pDoc->GetServer();

	CString strNewServer;
	m_pComboServer  = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_SERVER);
	nIdx = m_pComboServer->GetCurSel();
	if (nIdx == CB_ERR)
		return;
	m_pComboServer->GetLBText (nIdx, strNewServer);
	if (strNewServer.CompareNoCase(strServer) != 0)
	{
		pDoc->SetServer(strNewServer);
		m_nAlreadyRefresh &=~REFRESH_DATABASE;
		SetDefaultDatabase(TRUE);
		CommitOldSession();
		CuSqlQueryControl* pCtrl = GetSqlqueryControl();
		if (pCtrl)
			pCtrl->SetConnectParamInfo (-1);
		pDoc->UpdateAllViews(NULL, 1);
	}
}

void CfMainFrame::OnDropDownUser()
{
	QueryUser();
}



void CfMainFrame::OnSelChangeUser()
{
	CWaitCursor doWaitCursor;
	int nIdx = CB_ERR;
	int nLocal = -1;
	CdSqlQuery* pDoc = (CdSqlQuery*)GetSqlDoc();
	CString strUser = pDoc->GetUser();

	CString strNewUser;
	m_pComboUser    = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_USER);
	nIdx = m_pComboUser->GetCurSel();
	if (nIdx == CB_ERR)
		return;
	m_pComboUser->GetLBText (nIdx, strNewUser);
	if (strNewUser.CompareNoCase(strUser) != 0)
	{
		pDoc->SetUser(strNewUser);
		CommitOldSession();
		CuSqlQueryControl* pCtrl = GetSqlqueryControl();
		if (pCtrl)
			pCtrl->SetConnectParamInfo (-1);
		pDoc->UpdateAllViews(NULL, 1);
	}
}

void CfMainFrame::OnDropDownDatabase()
{
	QueryDatabase();
}



void CfMainFrame::OnSelChangeDatabase()
{
	CWaitCursor doWaitCursor;
	int nIdx = CB_ERR;
	int nLocal = -1;
	CdSqlQuery* pDoc = (CdSqlQuery*)GetSqlDoc();
	CString strDatabase = pDoc->GetDatabase();

	CString strNewDatabase;
	m_pComboDatabase= (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_DATABASE);
	nIdx = m_pComboDatabase->GetCurSel();
	if (nIdx == CB_ERR)
		return;
	m_pComboDatabase->GetLBText (nIdx, strNewDatabase);
	if (strNewDatabase.CompareNoCase(strDatabase) != 0)
	{
		UINT nFlag = 0;
		CaDatabase* pDb = (CaDatabase*)m_pComboDatabase->GetItemDataPtr(nIdx);
		if (pDb)
			nFlag = pDb->GetStar();
		pDoc->SetDatabase(strNewDatabase, nFlag);

		CommitOldSession();
	}
}

void CfMainFrame::OnRefresh() 
{
	//
	// This will cause all combobox to requery the data on dropdown:
	m_nAlreadyRefresh = 0;
}

void CfMainFrame::OnOpenEnvironment() 
{
#if defined (_NO_OPENSAVE)
	OnOpenQuery();
	return;
#endif

	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (!pCtrl)
		return;
	CString strFullName;
	CFileDialog dlg(
		TRUE,
		NULL,
		NULL,
		OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		_T("Ingres Visual SQL Files (*.vdbasql)|*.vdbasql||"));

	if (dlg.DoModal() != IDOK)
		return; 

	strFullName = dlg.GetPathName ();
	int nDot = strFullName.ReverseFind(_T('.'));
	if (nDot==-1)
	{
		AfxMessageBox (IDS_UNKNOWN_ENVIRONMENT_FILE);
		return;
	}
	else
	{
		CString strExt = strFullName.Mid(nDot+1);

		if (strExt.CompareNoCase (_T("vdbasql")) != 0)
		{
			AfxMessageBox (IDS_UNKNOWN_ENVIRONMENT_FILE);
			return;
		}
	}
	IStream* pStream = NULL;
	try
	{
		CdSqlQuery* pDoc = (CdSqlQuery*)GetActiveDocument();
		ASSERT(pDoc);
		if (!pDoc)
			throw;
		CaCompoundFile compoundFile(strFullName);
		//
		// Property Data:
		OpenStreamInit (pCtrl->GetControlUnknown(), theApp.m_tchszSqlProperty, compoundFile.GetRootStorage());
		//
		// Container Data:
		pStream = compoundFile.OpenStream(NULL, theApp.m_tchszContainerData, STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE);
		if (pStream)
		{
			COleStreamFile file (pStream);
			CArchive ar(&file, CArchive::load);

			//
			// 1) Load the document of the container:
			pDoc->Serialize(ar);
			m_lNode.Serialize(ar);
			m_lServer.Serialize(ar);
			m_lUser.Serialize(ar);
			m_lDatabase.Serialize(ar);
#if defined (_OPTION_GROUPxROLE)
			m_lGroup.Serialize(ar);
			m_lRole.Serialize(ar);
#endif
			UpdateLoadedData(pDoc);
		}

		//
		// Sql Data:
		pStream = compoundFile.OpenStream(NULL, theApp.m_tchszSqlData, STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE);
		if (pStream)
		{
			pCtrl->Loading((LPUNKNOWN)(pStream));
			pStream->Release();
		}
		return;
	}
	catch (CArchiveException* e)
	{
		e->ReportError();
		e->Delete();
	}
	catch (CFileException* e)
	{
		e->ReportError();
		e->Delete();
	}
	catch (CeCompoundFileException e)
	{
		AfxMessageBox (IDS_MSG_FAIL_2_SAVEENVIRONMENT);
	}
	catch (CMemoryException *e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch (...)
	{
		AfxMessageBox (IDS_MSG_FAIL_2_LOADENVIRONMENT);
	}
}

void CfMainFrame::OnUpdateOpenEnvironment(CCmdUI* pCmdUI) 
{
#if defined (_NO_OPENSAVE)
	OnUpdateOpenQuery(pCmdUI);
	return;
#endif

	BOOL bEnable = FALSE;
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (pCtrl)
		bEnable = TRUE;
	pCmdUI->Enable(bEnable);
}


void CfMainFrame::SaveWorkingFile(CString& strFullName)
{
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (!pCtrl)
		return;
	IStream* pStream = NULL;
	try
	{
		CdSqlQuery* pDoc = (CdSqlQuery*)GetActiveDocument();
		ASSERT(pDoc);
		if (!pDoc)
			throw;
		CaCompoundFile compoundFile(strFullName);

		//
		// Property:
		SaveStreamInit (pCtrl->GetControlUnknown(), theApp.m_tchszSqlProperty, compoundFile.GetRootStorage());
		//
		// SqlData:
		pStream = compoundFile.NewStream(NULL, theApp.m_tchszSqlData, STGM_DIRECT|STGM_CREATE|STGM_WRITE|STGM_SHARE_EXCLUSIVE);
		if (pStream)
		{
			COleStreamFile file (pStream);
			CArchive ar(&file, CArchive::store);
		
			IStream* pStreamSql = NULL;
			pCtrl->Storing((LPUNKNOWN*)(&pStreamSql));
			if (pStreamSql)
			{
				//
				// 2) Store the data of the SqlQuery control:
				ULONG lRead = 0;
				DWORD dwCount = 4090;
				BYTE buffer [4096];
				LARGE_INTEGER nDisplacement;
				nDisplacement.QuadPart = 0;
				pStreamSql->Seek(nDisplacement, STREAM_SEEK_SET, NULL);
				pStreamSql->Read((void*)buffer, dwCount, &lRead);
				while (lRead > 0)
				{
					ar.Write ((void*)buffer, lRead);
					pStreamSql->Read((void*)buffer, dwCount, &lRead);
				}

				pStreamSql->Release();
			}
		}

		//
		// Container Data:
		pStream = compoundFile.NewStream(NULL, theApp.m_tchszContainerData, STGM_DIRECT|STGM_CREATE|STGM_WRITE|STGM_SHARE_EXCLUSIVE);
		if (pStream)
		{
			COleStreamFile file (pStream);
			CArchive ar(&file, CArchive::store);

			pDoc->Serialize(ar);
			m_lNode.Serialize(ar);
			m_lServer.Serialize(ar);
			m_lUser.Serialize(ar);
			m_lDatabase.Serialize(ar);
#if defined (_OPTION_GROUPxROLE)
			m_lGroup.Serialize(ar);
			m_lRole.Serialize(ar);
#endif
		}
		return;
	}
	catch (CArchiveException* e)
	{
		e->ReportError();
		e->Delete();
	}
	catch (CFileException* e)
	{
		e->ReportError();
		e->Delete();
	}
	catch (CeCompoundFileException e)
	{
		AfxMessageBox (IDS_MSG_FAIL_2_SAVEENVIRONMENT);
	}
	catch (CMemoryException *e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch (...)
	{
		AfxMessageBox (IDS_MSG_FAIL_2_SAVEENVIRONMENT);
	}
}


void CfMainFrame::OnSaveEnvironment() 
{
#if defined (_NO_OPENSAVE)
	OnSaveQuery();
	return;
#endif

	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (!pCtrl)
		return;

	CString strFullName;
	CFileDialog dlg(
	    FALSE,
	    NULL,
	    NULL,
	    OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
	    _T("Ingres Visual SQL Files (*.vdbasql)|*.vdbasql||"));

	if (dlg.DoModal() != IDOK)
		return; 
	strFullName = dlg.GetPathName ();
	int nDot = strFullName.ReverseFind(_T('.'));
	if (nDot==-1)
	{
		strFullName += _T(".vdbasql");
	}
	else
	{
		CString strExt = strFullName.Mid(nDot+1);
		if (strExt.IsEmpty())
			strFullName += _T("vdbasql");
		else
		if (strExt.CompareNoCase (_T("vdbasql")) != 0)
			strFullName += _T(".vdbasql");
	}
	
	SaveWorkingFile(strFullName);
}


void CfMainFrame::OnUpdateSaveEnvironment(CCmdUI* pCmdUI) 
{
#if defined (_NO_OPENSAVE)
	OnUpdateSaveQuery(pCmdUI);
	return;
#endif
	
	BOOL bEnable = FALSE;
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (pCtrl)
		bEnable = TRUE;
	pCmdUI->Enable(bEnable);
}

void CfMainFrame::OnSize(UINT nType, int cx, int cy) 
{
	CFrameWnd::OnSize(nType, cx, cy);
}

void CfMainFrame::OnCommit() 
{
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (!pCtrl)
		return;
	if (!SetConnection())
		return;
	pCtrl->Commit(1);
}

void CfMainFrame::OnUpdateCommit(CCmdUI* pCmdUI) 
{
	BOOL nAutocommitOFF = FALSE;
	BOOL bEnable = FALSE;
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	BOOL nAutoCommit = pCtrl->GetSessionAutocommitState();
	BOOL nReadLock   = pCtrl->GetSessionReadLockState();
	if (pCtrl && nAutoCommit == nAutocommitOFF)
	{
		bEnable = pCtrl->IsCommitEnable();
	}
	pCmdUI->Enable(bEnable);
	SetIdicatorAutocomit(nAutoCommit? 1: 0);
	SetIdicatorReadLock (nReadLock? 1: 0);
}

void CfMainFrame::CommitOldSession()
{
	BOOL nAutocommitOFF= FALSE;
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	BOOL nAutoCommit = pCtrl->GetSessionAutocommitState();
	if (pCtrl && nAutoCommit == nAutocommitOFF)
	{
		if (!pCtrl->IsCommitEnable())
			return;
		int nAnsw = AfxMessageBox (IDS_MSG_COMMIT_INVITATION, MB_ICONQUESTION|MB_YESNO);
		if (nAnsw == IDYES)
			pCtrl->Commit(1);
		else
			pCtrl->Commit(0);
	}
}

void CfMainFrame::PermanentState(BOOL bLoad)
{
#if defined (_PERSISTANT_STATE_)
	try
	{
		CaPersistentStreamState file (_T("Vdbasql"), bLoad);
		if (!file.IsStateOK())
			return;
		if (bLoad)
		{
			CArchive ar(&file, CArchive::load);
			ar >> m_bSavePreferenceAsDefault;

			CMenu* pMenu = GetMenu();
			if (m_bSavePreferenceAsDefault)
				pMenu->CheckMenuItem(IDM_PREFERENCE_SAVE, MF_BYCOMMAND|MF_CHECKED);
			else
				pMenu->CheckMenuItem(IDM_PREFERENCE_SAVE, MF_BYCOMMAND|MF_UNCHECKED);
		}
		else
		{
			CArchive ar(&file, CArchive::store);
			ar << m_bSavePreferenceAsDefault;
		}
	}
	catch(CeCompoundFileException e)
	{
		CString strError, strErrorNo;
		strErrorNo.Format(_T("%d"), e.GetErrorCode());
		AfxFormatString1(strError, IDS_COMPOUNDFILE_ERROR, (LPCTSTR)strErrorNo);
		AfxMessageBox(strError);
	}
	catch(...)
	{
		TRACE0("Raise exception at CfMainFrame::PermanentState(BOOL bLoad)...\n");
	}
#endif
}

void CfMainFrame::OnClose() 
{
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (!pCtrl)
		return;
	CdSqlQuery* pDoc = (CdSqlQuery*)GetSqlDoc();
	if (!pDoc)
		return;

	CString strWorkingFile = pDoc->GetWorkingFile();
	if (!strWorkingFile.IsEmpty())
	{
		int nAnsver = AfxMessageBox (IDS_MSG_ASKFORSAVING_WORKINGFILE, MB_ICONQUESTION|MB_YESNO);
		if (nAnsver == IDYES)
		{
			SaveWorkingFile(strWorkingFile);
		}
	}

	PermanentState(FALSE);
	CFrameWnd::OnClose();
}

void CfMainFrame::OnPreferenceSave() 
{
	CMenu* pMenu = GetMenu();
	UINT nState = pMenu->GetMenuState(IDM_PREFERENCE_SAVE, MF_BYCOMMAND);
	if (nState == MF_CHECKED)
	{
		pMenu->CheckMenuItem(IDM_PREFERENCE_SAVE, MF_BYCOMMAND|MF_UNCHECKED);
		m_bSavePreferenceAsDefault = FALSE;
	}
	else
	{
		pMenu->CheckMenuItem(IDM_PREFERENCE_SAVE, MF_BYCOMMAND|MF_CHECKED);
		m_bSavePreferenceAsDefault = TRUE;
	}
}

UINT CfMainFrame::HandleCommandLine(CdSqlQuery* pDoc)
{
	UINT nMaskError = 0;
	m_uSetArg = 0;
	BOOL bEnableControl = FALSE;
	BOOL b2Connect = FALSE;
	CTypedPtrList <CObList, CaKeyValueArg*>& listKey = theApp.m_cmdLineArg.GetKeys();
	POSITION p, pos = listKey.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		CaKeyValueArg* pKey = listKey.GetNext (pos);
		if (pKey && pKey->IsMatched())
		{
			CString strValue;
			CString strKey = pKey->GetKey();
			if (strKey.CompareNoCase(_T("-node")) == 0)
			{
				b2Connect = TRUE;
				strValue = pKey->GetValue();
				if (!strValue.IsEmpty())
				{
					m_pComboNode    = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_NODE);
					if (m_pComboNode)
					{
						QueryNode(strValue);
						int nInx = m_pComboNode->FindStringExact(-1, strValue);
						if (nInx != CB_ERR)
						{
							pDoc->SetNode(strValue);
							m_pComboNode->SetCurSel(nInx);
							m_uSetArg |= MASK_SETNODE;
						}
						else
						{
							nMaskError |= MASK_SETNODE;
							break; 
						}
					}
				}
			}
			else
			if (strKey.CompareNoCase(_T("-server")) == 0)
			{
				b2Connect = TRUE;
				strValue = pKey->GetValue();
				if (!strValue.IsEmpty())
				{
					if (!QueryServer(strValue))
					{
						nMaskError |= MASK_SETSERVER;
						break; 
					}
					m_pComboServer  = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_SERVER);
					if (m_pComboServer)
					{
						int nInx = m_pComboServer->FindStringExact(-1, strValue);
						if (nInx != CB_ERR)
						{
							pDoc->SetServer(strValue);
							m_pComboServer->SetCurSel(nInx);
							m_uSetArg |= MASK_SETSERVER;
						}
						else
						{
							nMaskError |= MASK_SETSERVER;
							break; 
						}
					}
				}
			}
			else
			if (strKey.CompareNoCase(_T("-database")) == 0)
			{
				b2Connect = TRUE;
				strValue = pKey->GetValue();
				if (!strValue.IsEmpty())
				{
					if (!QueryDatabase(strValue))
					{
						nMaskError |= MASK_SETDATABASE;
						break; 
					}
					m_pComboDatabase= (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_DATABASE);
					if (m_pComboDatabase)
					{
						int nInx = m_pComboDatabase->FindStringExact(-1, strValue);
						if (nInx != CB_ERR)
						{
							m_pComboDatabase->SetCurSel(nInx);
							UINT nFlag = 0;
							CaDatabase* pDb = (CaDatabase*)m_pComboDatabase->GetItemDataPtr(nInx);
							if (pDb)
								nFlag = pDb->GetStar();
							pDoc->SetDatabase(strValue, nFlag);
							m_uSetArg |= MASK_SETDATABASE;
						}
						else
						{
							nMaskError |= MASK_SETDATABASE;
							break; 
						}
					}
				}
			}
			else
			if (strKey.CompareNoCase(_T("-u")) == 0)
			{
				b2Connect = TRUE;
				strValue = pKey->GetValue();
				if (!strValue.IsEmpty())
				{
					if (!QueryUser())
					{
						nMaskError |= MASK_SETUSER;
						break; 
					}
					m_pComboUser    = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_USER);
					if (m_pComboUser)
					{
						int nInx = m_pComboUser->FindStringExact(-1, strValue);
						if (nInx != CB_ERR)
						{
							pDoc->SetUser(strValue);
							m_pComboUser->SetCurSel(nInx);
							m_uSetArg |= MASK_SETUSER;
						}
						else
						{
							nMaskError |= MASK_SETUSER;
							break; 
						}
					}
				}
			}
#if defined (_OPTION_GROUPxROLE)
			else
			if (strKey.CompareNoCase(_T("-G")) == 0)
			{
				b2Connect = TRUE;
				strValue = pKey->GetValue();
				if (!strValue.IsEmpty())
				{
					if (!QueryGroup())
					{
						nMaskError |= MASK_SETGROUP;
						break; 
					}
					m_pComboGroup    = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_GROUP);
					if (m_pComboGroup)
					{
						int nInx = m_pComboGroup->FindStringExact(-1, strValue);
						if (nInx != CB_ERR)
						{
							pDoc->SetGroup(strValue);
							m_pComboGroup->SetCurSel(nInx);
							m_uSetArg |= MASK_SETGROUP;
						}
						else
						{
							nMaskError |= MASK_SETGROUP;
							break; 
						}
					}
				}
			}
			else
			if (strKey.CompareNoCase(_T("-R")) == 0)
			{
				b2Connect = TRUE;
				strValue = pKey->GetValue();
				if (!strValue.IsEmpty())
				{
					if (!QueryRole())
					{
						nMaskError |= MASK_SETROLE;
						break; 
					}
					m_pComboRole    = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_ROLE);
					if (m_pComboRole)
					{
						int nInx = m_pComboRole->FindStringExact(-1, strValue);
						if (nInx != CB_ERR)
						{
							pDoc->SetRole(strValue);
							m_pComboRole->SetCurSel(nInx);
							m_uSetArg |= MASK_SETROLE;
						}
						else
						{
							nMaskError |= MASK_SETROLE;
							break; 
						}
					}
				}
			}
#endif
			if (strKey.CompareNoCase(_T("-maxapp")) == 0)
			{
				strValue = pKey->GetValue();
				if (strValue.CompareNoCase(_T("TRUE")) == 0)
				{
					ShowWindow(SW_SHOWMAXIMIZED);
				}
			}
		}
	}

	if (b2Connect && nMaskError == 0)
	{
		m_pComboNode    = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_NODE);
		m_pComboServer  = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_SERVER);
		m_pComboUser    = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_USER);
		m_pComboDatabase= (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_DATABASE);
#if defined (_OPTION_GROUPxROLE)
		m_pComboGroup   = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_GROUP);
		m_pComboRole    = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_ROLE);
#endif
		if (m_pComboDatabase)
		{
			if (m_uSetArg & MASK_SETDATABASE)
				m_pComboDatabase->EnableWindow(FALSE);
			else
				m_pComboDatabase->EnableWindow(TRUE);
		}

		if ((m_uSetArg & MASK_SETNODE)    ||
		    (m_uSetArg & MASK_SETSERVER)  ||
		    (m_uSetArg & MASK_SETUSER)    ||
		    (m_uSetArg & MASK_SETGROUP)   ||
		    (m_uSetArg & MASK_SETROLE))
		{
			if (m_pComboNode && m_pComboServer && m_pComboUser && m_pComboDatabase)
			{
				m_pComboNode->EnableWindow(bEnableControl);
				m_pComboServer->EnableWindow(bEnableControl);
				m_pComboUser->EnableWindow(bEnableControl);
			}
#if defined (_OPTION_GROUPxROLE)
			if (m_pComboGroup && m_pComboRole)
			{
				m_pComboGroup->EnableWindow(bEnableControl);
				m_pComboRole->EnableWindow(bEnableControl);

			}
#endif
		}
		pDoc->UpdateAllViews(NULL, 2);
	}
	return nMaskError;
}
void CfMainFrame::OnNew() 
{
	OnClear();
}

void CfMainFrame::OnUpdateNew(CCmdUI* pCmdUI) 
{
	OnUpdateClear(pCmdUI);
}

void CfMainFrame::OnDefaultHelp() 
{
	APP_HelpInfo(m_hWnd, 0);
}

BOOL CfMainFrame::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	CuSqlQueryControl* pControl = GetSqlqueryControl();
	if (pControl)
		pControl->SendMessageToDescendants(WM_HELP);
	return CFrameWnd::OnHelpInfo(pHelpInfo);
}

BOOL APP_HelpInfo(HWND hWnd, UINT nHelpID)
{
	int  nPos = 0;
	CString strHelpFilePath;
	CString strTemp;
	strTemp = theApp.m_pszHelpFilePath;
#ifdef MAINWIN
	nPos = strTemp.ReverseFind( '/'); 
#else
	nPos = strTemp.ReverseFind( '\\'); 
#endif
	if (nPos != -1) 
	{
		strHelpFilePath = strTemp.Left( nPos +1 );
		strHelpFilePath += _T("vdbasql.chm");
	}
	TRACE1 ("APP_HelpInfo, Help Context ID = %d\n", nHelpID);
	if (nHelpID == 0)
		HtmlHelp(hWnd, strHelpFilePath, HH_DISPLAY_TOPIC, NULL);
	else
		HtmlHelp(hWnd, strHelpFilePath, HH_HELP_CONTEXT, nHelpID);

	return FALSE;
}

void CfMainFrame::OnHistoricSqlError()
{
	CdSqlQuery* pDoc = (CdSqlQuery*)GetSqlDoc();
	ASSERT(pDoc);
	if (!pDoc)
		return;

	CDlgSqlErrorHistory dlg;
	dlg.m_FilesErrorClass = pDoc->GetClassFilesErrors();
	if (!dlg.m_FilesErrorClass->GetStoreInFilesEnable())
		return; //Files names initializations failed, nothing to display
	dlg.DoModal();
}

void CfMainFrame::OnUpdateHistoricSqlError(CCmdUI *pCmdUI)
{
	BOOL bEnable = TRUE;
	pCmdUI->Enable(bEnable);
}
