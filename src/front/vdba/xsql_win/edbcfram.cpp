/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : edbcfram.cpp : implementation file
**    Project  : SQL/Test (stand alone)
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Main frame.
**
** History:
**
** 09-Oct-2003 (uk$so01)
**    Created
**    SIR #106648, (EDBC Specific)
*/

#include "stdafx.h"
#include "xsql.h"
#include "edbcfram.h"
#include "xsqldoc.h"
#include "xsqlview.h"
#include "edbcdoc.h"
#include "dmldbase.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define NO_EDBC_USER_COMBOBOX

IMPLEMENT_DYNCREATE(CfMainFrameEdbc, CfMainFrame)

CfMainFrameEdbc::CfMainFrameEdbc():CfMainFrame()
{
	m_nIDHotImage = IDB_HOTMAINFRAME_EDBC;
}

CfMainFrameEdbc::~CfMainFrameEdbc()
{
}


BEGIN_MESSAGE_MAP(CfMainFrameEdbc, CfMainFrame)
	//{{AFX_MSG_MAP(CfMainFrameEdbc)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP

	ON_COMMAND(IDM_CLEAR, CfMainFrame::OnClear)
	ON_COMMAND(IDM_OPEN_QUERY, CfMainFrame::OnOpenQuery)
	ON_COMMAND(IDM_SAVE_QUERY, CfMainFrame::OnSaveQuery)
	ON_COMMAND(IDM_ASSISTANT, CfMainFrame::OnAssistant)
	ON_COMMAND(IDM_RUN, CfMainFrame::OnRun)
	ON_COMMAND(IDM_QEP, CfMainFrame::OnQep)
	ON_COMMAND(IDM_XML, CfMainFrame::OnXml)
	ON_COMMAND(IDM_TRACE, CfMainFrame::OnTrace)
	ON_COMMAND(IDM_SETTING, CfMainFrame::OnSetting)
	ON_UPDATE_COMMAND_UI(IDM_CLEAR, CfMainFrame::OnUpdateClear)
	ON_UPDATE_COMMAND_UI(IDM_OPEN_QUERY, CfMainFrame::OnUpdateOpenQuery)
	ON_UPDATE_COMMAND_UI(IDM_SAVE_QUERY, CfMainFrame::OnUpdateSaveQuery)
	ON_UPDATE_COMMAND_UI(IDM_ASSISTANT, CfMainFrame::OnUpdateAssistant)
	ON_UPDATE_COMMAND_UI(IDM_RUN, CfMainFrame::OnUpdateRun)
	ON_UPDATE_COMMAND_UI(IDM_QEP, CfMainFrame::OnUpdateQep)
	ON_UPDATE_COMMAND_UI(IDM_TRACE, CfMainFrame::OnUpdateTrace)
	ON_UPDATE_COMMAND_UI(IDM_XML, CfMainFrame::OnUpdateXml)
	ON_UPDATE_COMMAND_UI(IDM_SETTING, CfMainFrame::OnUpdateSetting)
	ON_COMMAND(IDM_REFRESH, CfMainFrame::OnRefresh)
	ON_COMMAND(IDM_COMMIT, CfMainFrame::OnCommit)
	ON_UPDATE_COMMAND_UI(IDM_COMMIT, CfMainFrame::OnUpdateCommit)
	ON_COMMAND(IDM_PREFERENCE_SAVE, CfMainFrame::OnPreferenceSave)
	ON_COMMAND(IDM_PREFERENCE, CfMainFrame::OnSetting)
	ON_COMMAND(IDM_NEW, CfMainFrame::OnNew)
	ON_UPDATE_COMMAND_UI(IDM_NEW, CfMainFrame::OnUpdateNew)

	//
	// Virtual Node Combo notification:
	ON_CBN_DROPDOWN (IDM_NODE, CfMainFrame::OnDropDownNode)
	ON_CBN_SELCHANGE(IDM_NODE, CfMainFrame::OnSelChangeNode)
	//
	// Server Class Combo notification:
	ON_CBN_DROPDOWN (IDM_SERVER, CfMainFrame::OnDropDownServer)
	ON_CBN_SELCHANGE(IDM_SERVER, CfMainFrame::OnSelChangeServer)
	//
	// User Combo notification:
#if !defined (NO_EDBC_USER_COMBOBOX)
	ON_CBN_DROPDOWN (IDM_USER, OnDropDownUser)
	ON_CBN_SELCHANGE(IDM_USER, OnSelChangeUser)
#endif
	//
	// Database Combo notification:
	ON_CBN_DROPDOWN (IDM_DATABASE, CfMainFrame::OnDropDownDatabase)
	ON_CBN_SELCHANGE(IDM_DATABASE, CfMainFrame::OnSelChangeDatabase)

END_MESSAGE_MAP()


BOOL CfMainFrameEdbc::SetConnection(BOOL bDisplayMessage)
{
	CdSqlQueryEdbc* pDoc = (CdSqlQueryEdbc*)GetSqlDoc();
	ASSERT(pDoc);
	if (!pDoc)
		return FALSE;
	CString strNode = pDoc->GetNode();
	CString strServer = pDoc->GetServer();
	CString strUser   = _T("");
	CString strDatabase = pDoc->GetDatabase();

	if (strDatabase.IsEmpty() || strDatabase.CompareNoCase(m_strDefaultDatabase) == 0)
	{
		if (bDisplayMessage)
			AfxMessageBox (IDS_PLEASE_SELECT_DATABASE);
		return FALSE;
	}
	if (strServer.CompareNoCase(m_strDefaultServer) == 0)
		strServer = _T("");
#if !defined (NO_EDBC_USER_COMBOBOX)
	strUser = pDoc->GetUser();
	if (strUser.CompareNoCase(m_strDefaultUser) == 0)
		strUser = _T("");
#endif

	CString strConnectionString = pDoc->GetConnectInfo();
	CuSqlQueryControl* pCtrl = pDoc->GetSqlQueryCtrl();
	ASSERT(pCtrl && IsWindow (pCtrl->m_hWnd));
	if (pCtrl && IsWindow (pCtrl->m_hWnd))
	{
		if (strConnectionString.IsEmpty())
			pCtrl->Initiate(strNode, strServer, strUser);
		else
		{
			pCtrl->Initiate(strConnectionString, strServer, strUser);
		}
		UINT nFlag = pDoc->GetDbFlag();
		if (nFlag > 0)
			pCtrl->SetDatabaseStar(strDatabase, nFlag);
		else
			pCtrl->SetDatabase(strDatabase);
		return TRUE;
	}
	return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// CfMainFrameEdbc message handlers

int CfMainFrameEdbc::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CfMainFrame::OnCreate(lpCreateStruct) == -1)
		return -1;
	CdSqlQueryEdbc* pDoc = (CdSqlQueryEdbc*)GetSqlDoc();
	ASSERT(pDoc);
	if (!pDoc)
		return -1;
	//
	// Hide the Combobox: user
#if defined (NO_EDBC_USER_COMBOBOX)
	m_pComboUser    = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_USER);
	if (m_pComboUser)
		m_pComboUser->ShowWindow (SW_HIDE);
#endif

	return 0;
}



void CfMainFrameEdbc::OnSize(UINT nType, int cx, int cy) 
{
	CFrameWnd::OnSize(nType, cx, cy);
}


void CfMainFrameEdbc::OnClose() 
{
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (!pCtrl)
		return;
	CdSqlQueryEdbc* pDoc = (CdSqlQueryEdbc*)GetSqlDoc();
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

UINT CfMainFrameEdbc::HandleCommandLine(CdSqlQuery* pDoc)
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
					if (!QueryServer())
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
					if (!QueryDatabase())
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
			if (strKey.CompareNoCase(_T("-connectinfo")) == 0)
			{
				b2Connect = TRUE;
				strValue = pKey->GetValue();
				if (!strValue.IsEmpty())
				{
					CdSqlQueryEdbc* pDocEdbc = (CdSqlQueryEdbc*)pDoc;
					pDocEdbc->SetConnectInfo(strValue);
				}
				else
				{
					nMaskError |= MASK_SETCONNECT;
					break; 
				}
			}
			else
			{
				nMaskError |= MASK_UNKNOWN;
				break;
			}
		}
	}

	if (b2Connect && nMaskError == 0)
	{
		m_pComboNode    = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_NODE);
		m_pComboServer  = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_SERVER);
		m_pComboDatabase= (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_DATABASE);
#if !defined (NO_EDBC_USER_COMBOBOX)
		m_pComboUser    = (CComboBoxEx*)m_wndDlgBar.GetDlgItem(IDM_USER);
#endif
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
			if (m_pComboNode && m_pComboServer && m_pComboDatabase)
			{
				m_pComboNode->EnableWindow(bEnableControl);
				m_pComboServer->EnableWindow(bEnableControl);
				if (m_pComboUser)
					m_pComboUser->EnableWindow(bEnableControl);
#if defined (_OPTION_GROUPxROLE)
				if (m_pComboGroup)
					m_pComboGroup->EnableWindow(bEnableControl);
				if (m_pComboRole)
					m_pComboRole->EnableWindow(bEnableControl);
#endif
			}
		}
		pDoc->UpdateAllViews(NULL, 2);
	}

	return nMaskError;
}
