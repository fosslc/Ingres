/*
**  Copyright (C) 2005-2008 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vnitem.cpp, Implementation file
**
**    Project  : Ingres Visual DBA
**    Authors  : UK Sotheavut / Emmanuel Blattes
**
**    Purpose  : Manipulate the Virtual Node Tree ItemData to respond to the
**               methods like: Add, Alter, Drop,
**
**
**  History after 01-01-2000:
**   21-Jan-2000 (noifr01)
***   (SIR 100103) adapted the name of the Monitor (MDI) document, in the
**    case where VDBA was launched "in the context" , with the (exact)
**    parameters combination which is used when invoked from INGNET
**   26-Sep-2000 (uk$so01)
**    SIR #102711: Callable in context command line improvement (for Manage-IT)
**    Select the input database if specified.
**   28-Sep-2000 (noifr01)
**   (SIR 102711) customized the caption of DBEvent Trace windows when invoked
**   in the context (change for the ManageIT project)
**   27-Nov-2000 (schph01)
**    (SIR 102945) Grayed menu "Add Installation Password..." when the selection
**    on tree is on another branch that "Nodes" and "(Local)".
**   21-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
**   30-Mar-2001 (noifr01)
**    (sir 104378) differentiation of II 2.6.
**   31-May-2001 (zhayu01) SIR 104824 for EDBC
**    Use the title name to set the SQL Test window title for vnodeless 
**	  connections.
**   28-nov-2001 (somsa01)
**    When throw'ing a string, throw a local variable which is set to the
**    string. This will prevent a SIGABRT on HP.
** 18-Feb-2002 (uk$so01)
**    SIR #106648, Integrate SQL Test ActiveX Control
** 26-Feb-2002 (uk$so01)
**    SIR #106648, Integrate IPM ActiveX Control
** 15-Mar-2002 (schph01)
**    BUG 107331 initialize the variable member m_IngresVersion with
**    SetIngresVersion() method.
** 21-Mar-2002 (uk$so01)
**    SIR #106648, Integrate (ipm and sqlquery).ocx.
** 13-Jun-2003 (uk$so01)
**    SIR #106648, Take into account the STAR database for connection.
**   24-Jun-2003 (schph01)
**   (bug 110439) in CuTreeOtherAttr::Alter() function, fill the vnode name
**   before launch the dialog box.
** 14-Nov-2003 (uk$so01)
**    BUG #110983, If GCA Protocol Level = 50 then use MIB Objects 
**    to query the server classes.
** 02-feb-2004 (somsa01)
**    Updated to "typecast" UCHAR into CString before concatenation.
** 06-Feb-2004 (schph01)
**    SIR 111752 The "DBEvent Trace" icon is always available.
**    Add function INGRESII_IsDBEventsEnabled() to verify the  DBEVENTS flag
**    in iidbcapabilities tables, for managing DBEvent on Gateway.
** 01-Mar-2004 (schph01)
**    SIR 111752 in INGRESII_IsDBEventsEnabled() function, change the
**    initialization of the structures GWInfo and TblInfo.
** 03-Nov-2004 (uk$so01)
**    BUG #113263 / ISSUE #13755156
**    When a Vnode is incorrectly defined, Vdba displays an additional erroneous 
**    message if user tries to open the Performance Monitor Window on that Vnode.
** 16-Oct-2006 (wridu01)
**    Bug #116838 Change default protocol from wintcp to tcp_ip for IPV6 project.
** 23-Sep-2008 (lunbr01) Sir 119985
**    Remove #ifdef MAINWIN around default network protocol since it is no
**    longer needed after Durwin's change for bug 116838.
** 10-Mar-2010 (drivi01) SIR 123397
**    Update the code to retrieve a detailed error message when "Test" 
**    of the connection or newly created node fails.
*/

/* IMPORTANT NOTE: MENU IDs USED <--> METHODS TO WRITE
    IDM_VNODEBAR01: // Connect
    IDM_VNODEBAR02: // SqlTest
    IDM_VNODEBAR03: // Monitor
    IDM_VNODEBAR04: // Dbevent
    IDM_VNODEBAR05: // Disconnect
    IDM_VNODEBAR06: // CloseWin
    IDM_VNODEBAR07: // ActivateWin
    IDM_VNODEBAR08: // Add
    IDM_VNODEBAR09: // Alter
    IDM_VNODEBAR10: // Drop
    IDM_VNODEBARSC: // Scratchpad
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "mainfrm.h"
#include "vnitem.h"

#include "ipmaxdoc.h"   // monitor (CdIpm)
#include "dbedoc.h"     // dbevent

#include "dgvnode.h"    // Virtual Node Dialog, Convenient
#include "dgvnlogi.h"   // Virtual Node Dialog, Advanced Login
#include "dgvndata.h"   // Virtual Node Dialog, Advanced Connection Data

#include "disconn.h"    // Disconnect dialog box
#include "qsqldoc.h"    // CdSqlQuery (Document for the SQL Test)
#include "discon2.h"    // Disconnect Server dialog box

#include "starutil.h"   // GetLocalHostName()
#include "xdgvnpwd.h"   // Installation Password
#include "xdgvnatr.h"   // Virtual Node Attribute

#include "cmdline.h"
#include "dgerrh.h"     // MessageWithHistoryButton

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern "C"
{
  #include "main.h"
//  #include "getvnode.h"
  #include "dba.h"
  #include "domdata.h"
  #include "dbadlg1.h"
  #include "dbaginfo.h"
  #include "dbaset.h"     // Oivers
  #include "domdloca.h"
  #include "domdata.h"
  #include "msghandl.h"
  #include "er.h" //for ER_MAX_LEN define
// from main.c
BOOL MainOnSpecialCommand(HWND hwnd, int id, DWORD dwData);

#include "resource.h"   // IDM_NonMfc version
#include "monitor.h"    // low-lever interface structures
};


//
// Public Utility functions
//
BOOL INGRESII_IsIngresGateway(LPCTSTR lpszServerClass)
{
	const int nSize = 16;
	TCHAR tchNotIngresGwTab[nSize][16] = 
	{
		_T("ALB"),
		_T("DB2"),
		_T("DCOM"),
		_T("IDMS"),
		_T("IMS"),
		_T("INFORMIX"),
		_T("MSSQL"),
		_T("ODBC"),
		_T("ORACLE"),
		_T("RDB"),
		_T("RMS"),
		_T("SYBASE"),
		_T("VANT"),
		_T("DB2UDB"),
		_T("VSAM"),
		_T("ADABAS")
	};
	int i = 0;
	for (i = 0; i < nSize; i++)
	{
		if (_tcsicmp (tchNotIngresGwTab[i], lpszServerClass) == 0)
			return FALSE;
	}
	return TRUE;
}

extern "C" BOOL INGRESII_IsDBEventsEnabled (LPNODESVRCLASSDATAMIN lpInfo)
{
	CWaitCursor doWaitCursor;
	GATEWAYCAPABILITIESINFO GWInfo;
	TABLEPARAMS TblInfo;
	char szNodeName[200];
	int iret,nSession;

	if ( GetOIVers() != OIVERS_NOTOI ) //not a Gateway
		return TRUE;

	memset (&GWInfo,0,sizeof(GATEWAYCAPABILITIESINFO));
	memset (&TblInfo,0,sizeof(TABLEPARAMS));
	lstrcpy(szNodeName, BuildFullNodeName(lpInfo));
	lstrcat(szNodeName,"::iidbdb");

	iret = Getsession ((unsigned char*)szNodeName, SESSION_TYPE_CACHENOREADLOCK, &nSession);
	if (iret!=RES_SUCCESS)
	{
		MessageWithHistoryButton(GetFocus(),VDBA_MfcResourceString (IDS_E_GET_SESSION));//Cannot get a Session
		return FALSE;
	}

	if ( GetCapabilitiesInfo(&TblInfo, &GWInfo ) == RES_SUCCESS )
		ReleaseSession(nSession, RELEASE_COMMIT);
	else
		ReleaseSession(nSession, RELEASE_ROLLBACK);

	if (GWInfo.bGWOracleWithDBevents)
		return TRUE;

	return FALSE;
}

CString BuildFullNodeName(LPNODESVRCLASSDATAMIN pServerDataMin)
{
  CString name(pServerDataMin->NodeName);
  name += LPCLASSPREFIXINNODENAME;
  name += CString(pServerDataMin->ServerClass);
  name += LPCLASSSUFFIXINNODENAME;
  return name;
}

CString BuildFullNodeName(LPNODEUSERDATAMIN pUserDataMin)
{
  CString name(pUserDataMin->NodeName);

  if (pUserDataMin->ServerClass[0] != '\0') {
    name += LPCLASSPREFIXINNODENAME;
    name += CString(pUserDataMin->ServerClass);
    name += LPCLASSSUFFIXINNODENAME;
  }

  name += LPUSERPREFIXINNODENAME;
  name += CString(pUserDataMin->User);
  name += LPUSERSUFFIXINNODENAME;
  return name;
}

CdSqlQuery* CreateSqlTest(CString& strNode, CString& strServer, CString& strUser, CString& strDatabase)
{
	CWaitCursor doWaitCursor;
	BOOL bConnected = FALSE;
	int  hNode = -1;
	int  oldOIVers = GetOIVers();
	CString csSqlActTemplateString = GetSqlActTemplateString();
	CString strFullNode = strNode;
	if (!strServer.IsEmpty())
	{
		CString strSvr;
		strSvr.Format (_T(" (/%s)"), (LPCTSTR)strServer);
		strFullNode += strSvr;
	}
	if (!strUser.IsEmpty())
	{
		CString strUsr;
		strUsr.Format (_T(" (user:%s)"), (LPCTSTR)strUser);
		strFullNode += strUsr;
	}

	try
	{
		bConnected = CheckConnection((LPUCHAR)(LPCTSTR)strFullNode, FALSE,TRUE);
		if (!bConnected)
			throw (int)1;
		hNode = OpenNodeStruct((LPUCHAR)(LPCTSTR)strFullNode);
		if (hNode == -1)
			throw (int)2;
	}
	catch (int nErr)
	{
		// nErr = 1: Check connection failed. The MessageBox is already displayed
		//           So nothing to do here!

		if (nErr == 2)
		{
			CString strMsg, strMsg2; 
			strMsg.LoadString (IDS_MAX_NB_CONNECT); // _T("Maximum number of connections has been reached"
			strMsg2.LoadString(IDS_E_CREATE_SQL);   // " - Cannot create SQL Test Window."
			strMsg += strMsg2; 
			AfxMessageBox(strMsg);
		}
		SetOIVers(oldOIVers);
		return NULL;
	}

	POSITION pos = theApp.GetFirstDocTemplatePosition();
	while (pos != NULL) 
	{
		CDocTemplate* pTemplate = theApp.GetNextDocTemplate(pos);
		CString docName;
		pTemplate->GetDocString(docName, CDocTemplate::docName);
		if (docName == (LPCTSTR)csSqlActTemplateString)   // SqlAct
		{
			CDocument* pNewDoc = pTemplate->CreateNewDocument();
			ASSERT(pNewDoc);
			if (pNewDoc) 
			{
				UINT nDbFlag = 0;
				CdSqlQuery* pSqlQueryDoc = (CdSqlQuery *)pNewDoc;
				if ( IsStarDatabase(hNode, (LPUCHAR)(LPCTSTR)strDatabase))
					nDbFlag = DBFLAG_STARNATIVE;
				pSqlQueryDoc->SetNodeHandle(hNode);
				pSqlQueryDoc->SetNode(strNode);
				pSqlQueryDoc->SetServer(strServer);
				pSqlQueryDoc->SetUser(strUser);
				pSqlQueryDoc->SetDatabase(strDatabase, nDbFlag);
				pSqlQueryDoc->SetIngresVersion(GetOIVers());

				//
				// Associate to a new frame
				CFrameWnd* pFrame = pTemplate->CreateNewFrame(pNewDoc, NULL);
				ASSERT (pFrame);
				if (pFrame) 
				{
					bSaveRecommended = TRUE;

					CString capt; // Document Title
					capt.LoadString(IDS_CAPT_SQLTEST);
					CString buf;
					CMainFrame* pMainFrame = (CMainFrame *)AfxGetMainWnd();
					int seqnum = pMainFrame->GetNextSqlactSeqNum();
					buf.Format(_T("%s - %s - %d"), (LPCTSTR)strFullNode, (LPCTSTR)capt, seqnum);
					if (IsInvokedInContextWithOneWinExactly())
					{
						TCHAR buf1[200];
						if (! VDBA_InvokedInContextOneDocExactlyGetDocName(buf1,sizeof(buf1)))
							_tcscpy(buf1, _T(""));
#ifdef EDBC
						if (buf1[0] == _T('@') && _tcsstr(buf1, _T(",")))
						{
							TCHAR edbctitle[200];
							GetContextDrivenTitleName(edbctitle);
							pNewDoc->SetTitle(edbctitle);
						}
						else
							pNewDoc->SetTitle(buf1);
#else
						pNewDoc->SetTitle(buf1);
#endif
					}
					else
						pNewDoc->SetTitle(buf);

					// moved there because otherwise wasn't displayed
					// when invoked in the context from EDBCNET (app shown
					// later only)
					pTemplate->InitialUpdateFrame(pFrame, pNewDoc);
					pSqlQueryDoc->SetSeqNum(seqnum);
					return pSqlQueryDoc;
				}
			}
		}
	}

	CloseNodeStruct(hNode, FALSE);
	SetOIVers(oldOIVers);
	return NULL;
}


CdIpm* CreateMonitor(CString& strNode, CString& strServer, CString& strUser)
{
	CWaitCursor doWaitCursor;
	int oldOIVers = GetOIVers();
	int hNode = -1;

	LPTSTR lpszNode = (LPTSTR)(LPCTSTR)strNode;

	try
	{
		BOOL bConnected = CheckConnection((LPUCHAR)lpszNode, FALSE,TRUE);
		if (!bConnected)
			throw (int)1;
		hNode = OpenNodeStruct((LPUCHAR)lpszNode);
		if (hNode == -1)
			throw (int)2;
	}
	catch (int nErr)
	{
		// nErr = 1: Check connection failed. The MessageBox is already displayed
		//           So nothing to do here!

		if (nErr == 2)
		{
			CString strMsg, strMsg2; 
			strMsg.LoadString (IDS_MAX_NB_CONNECT); // _T("Maximum number of connections has been reached"
			strMsg2.LoadString(IDS_E_PERF_MONITOR);   // "  - Cannot create Performance Monitor Window."
			strMsg += strMsg2; 
			AfxMessageBox(strMsg);
		}
		SetOIVers(oldOIVers);
		return NULL;
	}

	// Added April 3, 97: must be open-ingres 2.0 node
	if (GetOIVers() < OIVERS_20) 
	{
		CString rString;
		AfxFormatString1(rString, IDS_ERR_MONITOR_NOTOIV2, (LPCTSTR)lpszNode);
		AfxMessageBox(rString, MB_OK | MB_ICONEXCLAMATION);
		CloseNodeStruct(hNode, FALSE);
		SetOIVers(oldOIVers);
		return NULL;   // ERROR!
	}

	// Added April 1, 97: test existence of imadb database
	int   resultType;
	UCHAR resultObjectName[MAXOBJECTNAME];
	UCHAR resultOwnerName[MAXOBJECTNAME];
	UCHAR resultExtraData[MAXOBJECTNAME];
	char* parentStrings[3];
	parentStrings[0] = parentStrings[1] = parentStrings[2] = NULL;
	int res = DOMGetObjectLimitedInfo(
		hNode,
		(LPUCHAR)"imadb",                 // object name,
		(LPUCHAR)"",                      // object owner
		OT_DATABASE,                      // iobjecttype
		0,                                // level
		(unsigned char **)parentStrings,  // parenthood
		TRUE,                             // bwithsystem
		&resultType,
		resultObjectName,
		resultOwnerName,
		resultExtraData);

	if (res != RES_SUCCESS)
	{
		CString rString;
		AfxFormatString1(rString, IDS_ERR_MONITOR_NOIMADB, lpszNode);
		AfxMessageBox(rString, MB_OK | MB_ICONEXCLAMATION);
		CloseNodeStruct(hNode, FALSE);
		SetOIVers(oldOIVers);
		return NULL;   // ERROR!
	}

	int OIVers = GetOIVers();   // new as of check connection
	CString csMonitorTemplateString = GetMonitorTemplateString();
	POSITION pos = theApp.GetFirstDocTemplatePosition();
	while (pos) 
	{
		CDocTemplate* pTemplate = theApp.GetNextDocTemplate(pos);
		CString docName;
		pTemplate->GetDocString(docName, CDocTemplate::docName);
		if (docName == (LPCTSTR)csMonitorTemplateString)
		{
			// prepare detached document
			CDocument* pNewDoc = pTemplate->CreateNewDocument();
			ASSERT(pNewDoc);
			if (pNewDoc)
			{
				// initialize document member variables
				CdIpm* pIpmDoc  = (CdIpm *)pNewDoc;
				pIpmDoc->SetNodeHandle(hNode);
				pIpmDoc->SetNode(strNode);
				pIpmDoc->SetServer(strServer);
				pIpmDoc->SetUser(strUser);
				pIpmDoc->SetIngresVersion(OIVers);
				SetOIVers(OIVers);

				// set document title
				CString capt;
				capt.LoadString(IDS_CAPT_IPM);
				CString buf;
				CMainFrame* pMainFrame = (CMainFrame *)AfxGetMainWnd();
				int seqnum = pMainFrame->GetNextIpmSeqNum();
				buf.Format(_T("%s - %s - %d"), lpszNode, (LPCTSTR)capt, seqnum);
				if (IsInvokedInContextWithOneWinExactly()) 
				{
					TCHAR buf1[200];
					if (! VDBA_InvokedInContextOneDocExactlyGetDocName(buf1,sizeof(buf1)))
						_tcscpy(buf1, _T(""));
					pNewDoc->SetTitle(buf1);
				}
				else
					pNewDoc->SetTitle(buf);
				pIpmDoc->SetSeqNum(seqnum);

				// Associate to a new frame
				CFrameWnd* pFrame = pTemplate->CreateNewFrame(pNewDoc, NULL);
				ASSERT (pFrame);
				if (pFrame) 
				{
					pTemplate->InitialUpdateFrame(pFrame, pNewDoc);
					bSaveRecommended = TRUE;
					return pIpmDoc;
				}
			}
		}
	}

	// Here, we have failed!
	CloseNodeStruct(hNode, FALSE);
	SetOIVers(oldOIVers);
	return NULL;
}

//
// prepare keywords - unicenter driven menu options taken in account
//
CString GetMonitorTemplateString()
{
  CString csKeyword = _T("Ipm");
  if (IsUnicenterDriven()) {
    CuWindowDesc* pDescriptor = GetAutomatedWindowDescriptor();
    ASSERT (pDescriptor);
    ASSERT (pDescriptor->GetType() == TYPEDOC_MONITOR);
    if (pDescriptor->HasNoMenu())
      csKeyword = csKeyword + _T("NoMenu");
    else if (pDescriptor->HasObjectActionsOnlyMenu())
      csKeyword = csKeyword + _T("ObjMenu");
  }
  return csKeyword;
}

// prepare keyword - unicenter driven menu options taken in account
CString GetSqlActTemplateString()
{
  CString csKeyword = _T("SqlAct");
  if (IsUnicenterDriven()) {
    CuWindowDesc* pDescriptor = GetAutomatedWindowDescriptor();
    ASSERT (pDescriptor);
    ASSERT (pDescriptor->GetType() == TYPEDOC_SQLACT);
    if (pDescriptor->HasNoMenu())
      csKeyword = csKeyword + _T("NoMenu");
    else if (pDescriptor->HasObjectActionsOnlyMenu())
      csKeyword = csKeyword + _T("ObjMenu");
  }
  return csKeyword;
}

// prepare keyword - unicenter driven menu options taken in account
CString GetDbEventTemplateString()
{
  CString csKeyword = _T("DbEvent");
  if (IsUnicenterDriven()) {
    CuWindowDesc* pDescriptor = GetAutomatedWindowDescriptor();
    ASSERT (pDescriptor);
    ASSERT (pDescriptor->GetType() == TYPEDOC_DBEVENT);
    if (pDescriptor->HasNoMenu())
      csKeyword = csKeyword + _T("NoMenu");
    else if (pDescriptor->HasObjectActionsOnlyMenu())
      csKeyword = csKeyword + _T("ObjMenu");
  }
  return csKeyword;
}

//
// Static utility functions
//

// Add a server - called from CuTreeServer and CuTreeServerStatic classes
static BOOL GlobTreeServerAdd(int hNode)
{
    TCHAR tchszProtocolDef[16];
    TCHAR tchszListenAddrDef[16];
    lstrcpy (tchszProtocolDef, _T("tcp_ip"));
    lstrcpy (tchszListenAddrDef, _T("II"));

    CxDlgVirtualNode dlg;
    dlg.m_bLoginPrivate = TRUE;
    dlg.m_bConnectionPrivate = FALSE;
    dlg.m_strProtocol = tchszProtocolDef;
    dlg.m_strListenAddress = tchszListenAddrDef;

    PushVDBADlg();
    int res = dlg.DoModal();
    PopVDBADlg();
    if (res == IDOK)
        return TRUE;
    return FALSE;
}

static BOOL IsServerClassIngres(LPNODESVRCLASSDATAMIN pServerDataMin)
{
  CString csIngres("Ingres");
  if (csIngres.CompareNoCase((LPCTSTR)pServerDataMin->ServerClass) == 0)
    return TRUE;
  else
    return FALSE;
}

// Add an installation password called from CuTreeServer and CuTreeServerStatic classes
static BOOL GlobTreeAddInstallationPassword(int hNode)
{
  if (InstallPasswordExists()) {
    //CString csTxt = _T("Installation password already defined on the node");
    AfxMessageBox(VDBA_MfcResourceString(IDS_E_INSTALL_PASSWORD), MB_OK | MB_ICONEXCLAMATION);
    return FALSE;
  }

  CxDlgVirtualNodeInstallationPassword dlg;
  PushVDBADlg();
  int res = dlg.DoModal();
  PopVDBADlg();
  if (res == IDOK)
      return TRUE;
  return FALSE;
}

//
// Classes Serialization declarations
//

IMPLEMENT_SERIAL (CuTreeServerStatic, CTreeItemNodes, 1)
IMPLEMENT_SERIAL (CuTreeServer, CTreeItemNodes, 1)
IMPLEMENT_SERIAL (CuTreeOpenwinStatic, CTreeItemNodes, 1)
IMPLEMENT_SERIAL (CuTreeOpenwin, CTreeItemNodes, 1)
IMPLEMENT_SERIAL (CuTreeAdvancedStatic, CTreeItemNodes, 2)
IMPLEMENT_SERIAL (CuTreeLoginStatic, CTreeItemNodes, 1)
IMPLEMENT_SERIAL (CuTreeLogin, CTreeItemNodes, 1)
IMPLEMENT_SERIAL (CuTreeConnectionStatic, CTreeItemNodes, 1)
IMPLEMENT_SERIAL (CuTreeConnection, CTreeItemNodes, 1)
IMPLEMENT_SERIAL (CuTreeSvrClassStatic, CTreeItemNodes, 1)
IMPLEMENT_SERIAL (CuTreeSvrClass, CTreeItemNodes, 1)
IMPLEMENT_SERIAL (CuTreeSvrOpenwinStatic, CTreeItemNodes, 1)
IMPLEMENT_SERIAL (CuTreeSvrOpenwin, CTreeItemNodes, 1)
IMPLEMENT_SERIAL (CuTreeOtherAttrStatic, CTreeItemNodes, 1)
IMPLEMENT_SERIAL (CuTreeOtherAttr, CTreeItemNodes, 1)

//
// Classes Implementation
//

CuTreeServerStatic::CuTreeServerStatic (CTreeGlobalData* pTreeGlobalData)
  :CTreeItemNodes (pTreeGlobalData, 0, IDS_NODE, SUBBRANCH_KIND_OBJ, IDB_NODE, NULL, OT_NODE, IDS_NODE_NONE)
{
  SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
}

CTreeItem* CuTreeServerStatic::AllocateChildItem (CTreeItemData* pItemData)
{
    CuTreeServer* pItem = new CuTreeServer (GetPTreeGlobData(), pItemData);
    return pItem;
}

CuTreeServerStatic::CuTreeServerStatic()
{
}

BOOL CuTreeServerStatic::IsEnabled(UINT idMenu)
{
  switch (idMenu) {
    case IDM_VNODEBAR08:    // Add
      return TRUE;
      break;

    case ID_SERVERS_INSTALLPSW:
      // fnn advice: if branch not expanded yet, don't call InstallPasswordExists(): problem if ingres not started
      if (!IsAlreadyExpanded())
        return TRUE;      
      if (InstallPasswordExists())
        return FALSE;
      else
        return TRUE;
      break;


    default:
      return CTreeItemNodes::IsEnabled(idMenu);
  }
  return FALSE;   // security if return forgotten in switch
}

BOOL CuTreeServerStatic::Add()
{
  int hNode = GetPTreeGlobData()->GetHNode();
  return GlobTreeServerAdd(hNode);
}

BOOL CuTreeServerStatic::InstallPassword()
{
  int hNode = GetPTreeGlobData()->GetHNode();
  return GlobTreeAddInstallationPassword(hNode);
}

CuTreeServer::CuTreeServer (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemNodes (pTreeGlobalData, OT_NODE, SUBBRANCH_KIND_STATIC, FALSE, pItemData)
{
  SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
  InitializeImageId(IDB_NODE);

  if (pItemData) {
    LPNODEDATAMIN pNodeDataMin = (LPNODEDATAMIN)pItemData->GetDataPtr();
    if (pNodeDataMin->bInstallPassword)
      SetSubBK(SUBBRANCH_KIND_NONE);
  }
}

BOOL CuTreeServer::CreateStaticSubBranches (HTREEITEM hParentItem)
{
    CTreeGlobalData* pGlobalData = GetPTreeGlobData();

    CuTreeOpenwinStatic*  pStat1 = new CuTreeOpenwinStatic (pGlobalData, GetPTreeItemData());
    CuTreeSvrClassStatic* pStat3 = new CuTreeSvrClassStatic (pGlobalData, GetPTreeItemData());
    CuTreeUserStatic*     pStat4 = new CuTreeUserStatic (pGlobalData, GetPTreeItemData());

    pStat3->CreateTreeLine (hParentItem);
    pStat4->CreateTreeLine (hParentItem);
    pStat1->CreateTreeLine (hParentItem);

    // No advance parametres if local node
    LPNODEDATAMIN pNodeDataMin = (LPNODEDATAMIN)GetPTreeItemData()->GetDataPtr();
    if (!pNodeDataMin->bIsLocal) {
      CuTreeAdvancedStatic* pStat2 = new CuTreeAdvancedStatic (pGlobalData, GetPTreeItemData());
      pStat2->CreateTreeLine (hParentItem);
    }

    return TRUE;
}

CuTreeServer::CuTreeServer()
{

}

BOOL CuTreeServer::IsEnabled(UINT idMenu)
{
  LPNODEDATAMIN pNodeDataMin;

  switch (idMenu) {
    case IDM_VNODEBAR01:    // Connect
    case IDM_VNODEBAR02:    // SqlTest
    case IDM_VNODEBARSC:    // Scratchpad
      if (IsNoItem() || IsErrorItem())
        return FALSE;
      pNodeDataMin = (LPNODEDATAMIN)GetPTreeItemData()->GetDataPtr();

      // Not acceptable for Installation Password definition
      if (pNodeDataMin->bInstallPassword)
        return FALSE;

      // Needs not to be local host name node (for installation password purpose only)
      if (GetLocalHostName().CompareNoCase((LPCTSTR)pNodeDataMin->NodeName) == 0)
        return FALSE;
      return TRUE;
      break;

    case IDM_VNODEBAR03:    // Monitor Ipm
      if (IsNoItem() || IsErrorItem())
        return FALSE;
      pNodeDataMin = (LPNODEDATAMIN)GetPTreeItemData()->GetDataPtr();

      // Not acceptable for Installation Password definition
      if (pNodeDataMin->bInstallPassword)
        return FALSE;

      // needs to be OpenIngres 2.0 node
      // Needs not to be local host name node (for installation password purpose only)
      if (GetLocalHostName().CompareNoCase((LPCTSTR)pNodeDataMin->NodeName) == 0)
        return FALSE;

      // Note : version 2.0 or not tested after call to CheckConnection()
      return TRUE;

    case IDM_VNODEBAR04:    // Dbevent
      if (IsNoItem() || IsErrorItem())
        return FALSE;
      pNodeDataMin = (LPNODEDATAMIN)GetPTreeItemData()->GetDataPtr();

      // Not acceptable for Installation Password definition
      if (pNodeDataMin->bInstallPassword)
        return FALSE;

      // Needs not to be local host name node (for installation password purpose only)
      if (GetLocalHostName().CompareNoCase((LPCTSTR)pNodeDataMin->NodeName) == 0)
        return FALSE;
      return TRUE;

    case IDM_VNODEBAR05:    // Disconnect
      if (IsNoItem() || IsErrorItem())
        return FALSE;
      pNodeDataMin = (LPNODEDATAMIN)GetPTreeItemData()->GetDataPtr();

      // Not acceptable for Installation Password definition
      if (pNodeDataMin->bInstallPassword)
        return FALSE;

      if (NodeConnections(pNodeDataMin->NodeName) == 0)
        return FALSE;   // Not connected!
      return TRUE;
      break;

    case IDM_VNODEBAR10:    // Drop
      if (IsNoItem() || IsErrorItem())
        return FALSE;

      // don't drop if local
      pNodeDataMin = (LPNODEDATAMIN)GetPTreeItemData()->GetDataPtr();
      if (pNodeDataMin->bIsLocal)
          return FALSE;

      // don't drop if at least one window on the node, or if used by replicator monitoring
      if (AtLeastOneWindowOnNode() || IsNodeUsed4ExtReplMon(pNodeDataMin->NodeName))
        return FALSE;
      return TRUE;
      break;

    case IDM_VNODEBAR08:    // Add
      if (IsErrorItem())
        return FALSE;
      return TRUE;
      break;

    case IDM_VNODEBAR09:    // Alter
      if (IsNoItem() || IsErrorItem())
        return FALSE;

      // don't alter if local
      pNodeDataMin = (LPNODEDATAMIN)GetPTreeItemData()->GetDataPtr();
      if (pNodeDataMin->bIsLocal)
          return FALSE;

      // Note : modified 29/3/97: don't gray menuitem, but prevent
      // with MessageBox using ad hoc message in the following cases:
      // 1) at least one window on the node
      // 2) node not simplified

      // can alter
      return TRUE;
      break;

    case ID_SERVERS_TESTNODE:
      if (IsNoItem() || IsErrorItem())
        return FALSE;
      pNodeDataMin = (LPNODEDATAMIN)GetPTreeItemData()->GetDataPtr();

      // Not acceptable for Installation Password definition
      if (pNodeDataMin->bInstallPassword)
        return FALSE;

      // don't test if local
      if (pNodeDataMin->bIsLocal)
          return FALSE;
      // Don't test if local host name node (for installation password purpose only)
      if (GetLocalHostName().CompareNoCase((LPCTSTR)pNodeDataMin->NodeName) == 0)
        return FALSE;
      // can test
      return TRUE;
      break;

    case ID_SERVERS_INSTALLPSW:
      if (IsNoItem() || IsErrorItem())
        return FALSE;

      pNodeDataMin = (LPNODEDATAMIN)GetPTreeItemData()->GetDataPtr();

      // install password only on local node
      if (!pNodeDataMin->bIsLocal)
          return FALSE;

      if (InstallPasswordExists())
        return FALSE;
      else
        return TRUE;
      break;

    default:
      return CTreeItemNodes::IsEnabled(idMenu);
      break;
  }

  ASSERT (FALSE);
  return FALSE;   // security if return forgotten in switch
}

BOOL CuTreeServer::TestNode()
{
  LPNODEDATAMIN pNodeDataMin = (LPNODEDATAMIN)GetPTreeItemData()->GetDataPtr();
  CWaitCursor hourglass;
  BOOL bNodeUp=FALSE;
  STATUS status = NodeCheckConnection((LPUCHAR)pNodeDataMin->NodeName);
  CString csTxt;
  if (status == OK)
  {
    csTxt.Format(IDS_E_TEST_NODE_OK, pNodeDataMin->NodeName);//_T("Node test on %s is successful")
	AfxMessageBox(csTxt, MB_OK | MB_ICONEXCLAMATION);
	bNodeUp=TRUE;
  }
  else
  {
    char buff[ ER_MAX_LEN ];
    char* pBuff = buff;
    csTxt.Format(IDS_E_TEST_NODE_FAILED, pNodeDataMin->NodeName);//_T("Node test on %s has failed")
    INGRESII_ERLookup(status, (char **)&pBuff, sizeof(buff));
    ErrorMessage2 (csTxt.GetBuffer() , buff);
  }
  return bNodeUp;
}

BOOL CuTreeServer::Connect()
{
  // Create a dom type document

  int oldOIVers = GetOIVers();

  CString nodeName = GetDisplayName();
  char szNodeName[200];
  x_strcpy(szNodeName, (char *)(LPCSTR)nodeName);
  BOOL bOk = MainOnSpecialCommand(AfxGetMainWnd()->m_hWnd,
                                  IDM_CONNECT,
                                  (DWORD)szNodeName);
  if (bOk)
    UpdateOpenedWindowsList();    // update low-level
  else {
    SetOIVers(oldOIVers);
  }
  return bOk;
}

BOOL CuTreeServer::SqlTest(LPCTSTR lpszDatabase)
{
	CString strNode = GetDisplayName();
	CString strServer = _T("");
	CString strUser = _T("");
	CString strDatabase = _T("");

	CdSqlQuery* pSqlDoc = CreateSqlTest(strNode, strServer, strUser, strDatabase);
	if (pSqlDoc)
	{
		UpdateOpenedWindowsList();    // Update low-level: AFTER SetTitle()!
		return TRUE;
	}
	return FALSE;
}

BOOL CuTreeServer::Monitor()
{
	CString strNode = GetDisplayName();
	CString strServer = _T("");
	CString strUser = _T("");
	CdIpm* pIpmDoc = CreateMonitor(strNode, strServer, strUser);
	if (pIpmDoc)
	{
		UpdateOpenedWindowsList();    // Update low-level: AFTER SetTitle()!
		return TRUE;
	}
	return FALSE;
}

BOOL CuTreeServer::Dbevent(LPCTSTR lpszDatabase)
{
  int oldOIVers = GetOIVers();

  CString nodeName = GetDisplayName();
  char szNodeName[200];
  x_strcpy(szNodeName, (char *)(LPCSTR)nodeName);
  if (!CheckConnection((LPUCHAR)szNodeName,FALSE,TRUE)) {
    SetOIVers(oldOIVers);
    return FALSE;   // ERROR!
  }

  int hNode = OpenNodeStruct((LPUCHAR)szNodeName);
  if (hNode == -1) {
    SetOIVers(oldOIVers);
    CString strMsg =  VDBA_MfcResourceString (IDS_MAX_NB_CONNECT);  // _T("Maximum number of connections has been reached"
    strMsg += VDBA_MfcResourceString (IDS_E_DBEVENT_TRACE);         // " - Cannot create DBEvent Trace Window."
    MessageBox(GetFocus(),strMsg, NULL, MB_OK | MB_ICONEXCLAMATION);
    return FALSE;   // ERROR!
  }

  LPNODESVRCLASSDATAMIN pServerDataMin = (LPNODESVRCLASSDATAMIN)GetPTreeItemData()->GetDataPtr();

  if (!INGRESII_IsDBEventsEnabled (pServerDataMin )) {
    BfxMessageBox(IDS_ERR_DBEVENT_NOTOI, MB_OK | MB_ICONEXCLAMATION);
    CloseNodeStruct(hNode, FALSE);
    SetOIVers(oldOIVers);
    return FALSE;   // ERROR!
  }

  int OIVers = GetOIVers();   // new as of check connection

	CWinApp* pWinApp = AfxGetApp();
  ASSERT (pWinApp);
  CString csDbEventTemplateString = GetDbEventTemplateString();
  POSITION pos = pWinApp->GetFirstDocTemplatePosition();
  while (pos) {
    CDocTemplate *pTemplate = pWinApp->GetNextDocTemplate(pos);
    CString docName;
    pTemplate->GetDocString(docName, CDocTemplate::docName);
    if (docName == (LPCTSTR)csDbEventTemplateString) {   // DbEvent
      // prepare detached document
      CDocument *pNewDoc = pTemplate->CreateNewDocument();
      ASSERT(pNewDoc);
      if (pNewDoc) {
        // initialize document member variables
        CDbeventDoc *pDbeventDoc = (CDbeventDoc *)pNewDoc;
        pDbeventDoc->m_hNode  = hNode;
        pDbeventDoc->m_OIVers = OIVers;
        pDbeventDoc->SetInputDatabase(lpszDatabase);
        SetOIVers(OIVers);

        // set document title
        CString capt;
        capt.LoadString(IDS_CAPT_DBEVENT);
        CString buf;
        CMainFrame* pMainFrame = (CMainFrame *)AfxGetMainWnd();
        int seqnum = pMainFrame->GetNextDbeventSeqNum();
        buf.Format("%s - %s - %d", szNodeName, (LPCTSTR)capt, seqnum);
		if (IsInvokedInContextWithOneWinExactly()) {
			char buf1[200];
			if (! VDBA_InvokedInContextOneDocExactlyGetDocName(buf1,sizeof(buf1)))
				x_strcpy(buf1,"");
			pNewDoc->SetTitle(buf1);
		}
		else
			pNewDoc->SetTitle(buf);
        pDbeventDoc->SetSeqNum(seqnum);

        // Associate to a new frame
        CFrameWnd *pFrame = pTemplate->CreateNewFrame(pNewDoc, NULL);
        ASSERT (pFrame);
        if (pFrame) {
          pTemplate->InitialUpdateFrame(pFrame, pNewDoc);
          UpdateOpenedWindowsList();    // update low-level
          bSaveRecommended = TRUE;
          return TRUE;
        }
      }
    }
  }

  // Here, we have failed!
  CloseNodeStruct(hNode, FALSE);
  SetOIVers(oldOIVers);
  return FALSE;
}

BOOL CuTreeServer::Scratchpad()
{
  // Create a scratchpad type document

  int oldOIVers = GetOIVers();

  CString nodeName = GetDisplayName();
  char szNodeName[200];
  x_strcpy(szNodeName, (char *)(LPCSTR)nodeName);
  BOOL bOk = MainOnSpecialCommand(AfxGetMainWnd()->m_hWnd,
                                  IDM_SCRATCHPAD,
                                  (DWORD)szNodeName);
  if (bOk)
    UpdateOpenedWindowsList();    // update low-level
  else {
    SetOIVers(oldOIVers);
  }
  return bOk;
}

CuTreeOpenwinStatic::CuTreeOpenwinStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemNodes (pTreeGlobalData, OT_NODE, IDS_NODE_OPENWIN, SUBBRANCH_KIND_OBJ, IDB_NODE_OPENWIN, pItemData, OT_NODE_OPENWIN, IDS_NODE_OPENWIN_NONE)
{
  SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
}

CTreeItem* CuTreeOpenwinStatic::AllocateChildItem (CTreeItemData* pItemData)
{
    CuTreeOpenwin* pItem = new CuTreeOpenwin (GetPTreeGlobData(), pItemData);
    return pItem;
}

CuTreeOpenwinStatic::CuTreeOpenwinStatic()
{


}

CuTreeOpenwin::CuTreeOpenwin (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemNodes (pTreeGlobalData, OT_NODE_OPENWIN, SUBBRANCH_KIND_NONE, FALSE, pItemData)
{
  SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
  InitializeImageId(IDB_NODE_OPENWIN);
}

CuTreeOpenwin::CuTreeOpenwin()
{

}


CuTreeAdvancedStatic::CuTreeAdvancedStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemNodes (pTreeGlobalData, OT_NODE, IDS_NODE_ADVANCED, SUBBRANCH_KIND_STATIC, IDB_NODE_ADVANCED, pItemData)
{
  SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
}

BOOL CuTreeAdvancedStatic::CreateStaticSubBranches (HTREEITEM hParentItem)
{
    CTreeGlobalData* pGlobalData = GetPTreeGlobData();

    CuTreeLoginStatic*      pStat1 = new CuTreeLoginStatic      (pGlobalData, GetPTreeItemData());
    CuTreeConnectionStatic* pStat2 = new CuTreeConnectionStatic (pGlobalData, GetPTreeItemData());
    CuTreeOtherAttrStatic*  pStat3 = new CuTreeOtherAttrStatic  (pGlobalData, GetPTreeItemData());

    pStat1->CreateTreeLine (hParentItem);
    pStat2->CreateTreeLine (hParentItem);
    pStat3->CreateTreeLine (hParentItem);
    return TRUE;
}

CuTreeAdvancedStatic::CuTreeAdvancedStatic()
{


}


CuTreeLoginStatic::CuTreeLoginStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemNodes (pTreeGlobalData, OT_NODE, IDS_NODE_LOGINPSW, SUBBRANCH_KIND_OBJ, IDB_NODE_LOGINPSW, pItemData, OT_NODE_LOGINPSW, IDS_NODE_LOGINPSW_NONE)
{
  SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
}

CTreeItem* CuTreeLoginStatic::AllocateChildItem (CTreeItemData* pItemData)
{
    CuTreeLogin* pItem = new CuTreeLogin (GetPTreeGlobData(), pItemData);
    return pItem;
}


CuTreeLoginStatic::CuTreeLoginStatic()
{


}

BOOL CuTreeLoginStatic::IsEnabled(UINT idMenu)
{
    LPNODEDATAMIN pNodeDataMin;

    switch (idMenu) 
    {
    case IDM_VNODEBAR08:    // Add
        if (IsErrorItem())
            return FALSE;
        // don't add if node is local
        pNodeDataMin = (LPNODEDATAMIN)GetPTreeItemData()->GetDataPtr();
        if (pNodeDataMin->bIsLocal)
            return FALSE;
        return TRUE;
    default:
        return FALSE;
    }
    return FALSE;
}


BOOL CuTreeLoginStatic::AtLeastOneWindowOnNode()
{

  CTreeGlobalData *pTreeGD = GetPTreeGlobData();
  CTreeCtrl *pTree = pTreeGD->GetPTree();
  HTREEITEM hCurItem = pTree->GetSelectedItem();
  CTreeItem *pTreeItem = (CTreeItem *)pTree->GetItemData(hCurItem);
  ASSERT (pTreeItem == this);

  HTREEITEM hParentItem = pTree->GetParentItem(hCurItem);
  hParentItem = pTree->GetParentItem(hParentItem);
  CuTreeServer *pTreeServerItem = (CuTreeServer *)pTree->GetItemData(hParentItem);
  return pTreeServerItem->AtLeastOneWindowOnNode();
}

BOOL CuTreeLoginStatic::Add()
{
    LPNODEDATAMIN pNodeDataMin = (LPNODEDATAMIN)GetPTreeItemData()->GetDataPtr();

    // Note : Added 22/04/97: prevent with MessageBox using ad hoc message
    // if at least one window opened on the node
    if (AtLeastOneWindowOnNode()) {
      CString rString;
      AfxFormatString1(rString, IDS_ERR_ADDNODELOGIN_OPENNODES, (LPCTSTR)pNodeDataMin->NodeName);
      BfxMessageBox(rString, MB_OK | MB_ICONEXCLAMATION);
      return FALSE;
    }

    //
    // The can be only two Logins in a single node: if one is Private then
    // another one must be Global.
    int ires, nLoginCount = 0;
    NODEDATAMIN node;
    NODELOGINDATAMIN data;
    memset (&node, 0, sizeof (node));
    memset (&data, 0, sizeof (data));
    lstrcpy ((LPTSTR)node.NodeName, (LPTSTR)pNodeDataMin->NodeName);
    ires = GetFirstMonInfo (0, OT_NODE, &node, OT_NODE_LOGINPSW, (void *)&data, NULL);
    while (ires == RES_SUCCESS) 
    {
        nLoginCount++;
        ires = GetNextMonInfo((void *)&data);
    }
    if (nLoginCount > 1)
    {
        BfxMessageBox (VDBA_MfcResourceString(IDS_E_TWO_LOGINS));//"You can only have two Logins per node."
        return FALSE;
    }
    if (nLoginCount == 1) {
        char buf[100];
        int ires;
        if (data.bPrivate) {
           //"You already have a private Login for node %s. "
           //"This one will be a GLOBAL Login."
           wsprintf(buf,VDBA_MfcResourceString(IDS_E_PRIVATE_LOG),pNodeDataMin->NodeName);
           ires=BfxMessageBox (buf,MB_OKCANCEL);
        }
        else {
           //"You already have a global Login for node %s. "
           //"This one will be a PRIVATE Login."
           wsprintf(buf,VDBA_MfcResourceString(IDS_E_GLOBAL_LOG),pNodeDataMin->NodeName);
           ires=BfxMessageBox (buf,MB_OKCANCEL);
        }
        if (ires==IDCANCEL)
           return FALSE;
    }

    CxDlgVirtualNodeLogin dlg;
    if (nLoginCount == 1)
    {
        dlg.m_bEnableCheck  = FALSE;  // Disable the private check box.
        dlg.m_bPrivate      = !data.bPrivate; 
    }
    else
        dlg.m_bPrivate = TRUE;
    dlg.m_strVNodeName = (LPTSTR)pNodeDataMin->NodeName;
    PushVDBADlg();
    int res = dlg.DoModal();
    PopVDBADlg();
    if (res == IDOK)
        return TRUE;
    return FALSE;
}

void CuTreeLoginStatic::MinFillpStructReq(void *pStructReq)
{
  ASSERT (pStructReq);
  ASSERT(GetPTreeItemData());

  LPNODEDATAMIN pNodeDataMin = (LPNODEDATAMIN)GetPTreeItemData()->GetDataPtr();
  LPNODELOGINDATAMIN pNodeLoginDataMin = (LPNODELOGINDATAMIN)pStructReq;

  x_strcpy((char *)pNodeLoginDataMin->NodeName, (const char *)pNodeDataMin->NodeName);
}


CuTreeLogin::CuTreeLogin (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemNodes (pTreeGlobalData, OT_NODE_LOGINPSW, SUBBRANCH_KIND_NONE, FALSE, pItemData)
{
  SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
  InitializeImageId(IDB_NODE_LOGINPSW);
}


CuTreeLogin::CuTreeLogin()
{


}

BOOL CuTreeLogin::IsEnabled(UINT idMenu)
{
    switch (idMenu) 
    {
    case IDM_VNODEBAR08:    // Add
        if (IsErrorItem())
            return FALSE;
        // don't add if node is local - Complicated stuff!
        {
          CTreeGlobalData *pTreeGD = GetPTreeGlobData();
          CTreeCtrl *pTree = pTreeGD->GetPTree();
          HTREEITEM hCurItem = pTree->GetSelectedItem();
          CTreeItem *pTreeItem = (CTreeItem *)pTree->GetItemData(hCurItem);
          ASSERT (pTreeItem == this);
          
          HTREEITEM hParentItem = pTree->GetParentItem(hCurItem);
          hParentItem = pTree->GetParentItem(hParentItem);
          CuTreeServer *pTreeServerItem = (CuTreeServer *)pTree->GetItemData(hParentItem);
          LPNODEDATAMIN pNodeDataMin;
          pNodeDataMin = (LPNODEDATAMIN)pTreeServerItem->GetPTreeItemData()->GetDataPtr();
          if (pNodeDataMin->bIsLocal)
             return FALSE;
        }
        return TRUE;

    case IDM_VNODEBAR09:    // Alter
    case IDM_VNODEBAR10:    // Drop
        if (IsErrorItem() || IsNoItem())
            return FALSE;
        else
            return TRUE;
    default:
        return FALSE;
    }
    return FALSE;
}



BOOL CuTreeLogin::AtLeastOneWindowOnNode()
{

  CTreeGlobalData *pTreeGD = GetPTreeGlobData();
  CTreeCtrl *pTree = pTreeGD->GetPTree();
  HTREEITEM hCurItem = pTree->GetSelectedItem();
  CTreeItem *pTreeItem = (CTreeItem *)pTree->GetItemData(hCurItem);
  ASSERT (pTreeItem == this);

  HTREEITEM hParentItem = pTree->GetParentItem(hCurItem);
  hParentItem = pTree->GetParentItem(hParentItem);
  hParentItem = pTree->GetParentItem(hParentItem);
  CuTreeServer *pTreeServerItem = (CuTreeServer *)pTree->GetItemData(hParentItem);
  return pTreeServerItem->AtLeastOneWindowOnNode();
}

BOOL CuTreeLogin::Add()
{
    LPNODELOGINDATAMIN pNodeLoginDataMin = (LPNODELOGINDATAMIN)GetPTreeItemData()->GetDataPtr();

    // Note : Added 22/04/97: prevent with MessageBox using ad hoc message
    // if at least one window opened on the node
    if (AtLeastOneWindowOnNode()) {
      CString rString;
      AfxFormatString1(rString, IDS_ERR_ADDNODELOGIN_OPENNODES, (LPCTSTR)pNodeLoginDataMin->NodeName);
      BfxMessageBox(rString, MB_OK | MB_ICONEXCLAMATION);
      return FALSE;
    }


    //
    // The can be only two Logins in a single node: if one is Private then
    // another one must be Global.
    int ires, nLoginCount = 0;
    NODEDATAMIN node;
    NODELOGINDATAMIN data;
    memset (&node, 0, sizeof (node));
    memset (&data, 0, sizeof (data));
    lstrcpy ((LPTSTR)node.NodeName, (LPTSTR)pNodeLoginDataMin->NodeName);
    ires = GetFirstMonInfo (0, OT_NODE, &node, OT_NODE_LOGINPSW, (void *)&data, NULL);
    while (ires == RES_SUCCESS) 
    {
        nLoginCount++;
        ires = GetNextMonInfo((void *)&data);
    }
    if (nLoginCount > 1)
    {
        BfxMessageBox (VDBA_MfcResourceString(IDS_E_TWO_LOGINS));//"You can only have two Logins per node."
        return FALSE;
    }
    if (nLoginCount == 1) {
        char buf[100];
        int ires;
        if (data.bPrivate) {
           //"You already have a private Login for node %s. "
           //"This one will be a GLOBAL Login."
           wsprintf(buf,VDBA_MfcResourceString(IDS_E_PRIVATE_LOG),pNodeLoginDataMin->NodeName);
           ires=BfxMessageBox (buf,MB_OKCANCEL);
        }
        else {
           //"You already have a global Login for node %s. "
           //"This one will be a PRIVATE Login."
           wsprintf(buf,VDBA_MfcResourceString(IDS_E_GLOBAL_LOG),pNodeLoginDataMin->NodeName);
           ires=BfxMessageBox (buf,MB_OKCANCEL);
        }
        if (ires==IDCANCEL)
           return FALSE;
    }

    CxDlgVirtualNodeLogin dlg;
    if (nLoginCount == 1)
    {
        dlg.m_bEnableCheck  = FALSE;  // Disable the private check box.
        dlg.m_bPrivate      = !data.bPrivate; 
    }
    else
        dlg.m_bPrivate = TRUE;
    dlg.m_strVNodeName = (LPTSTR)pNodeLoginDataMin->NodeName;
    PushVDBADlg();
    int res = dlg.DoModal();
    PopVDBADlg();
    if (res == IDOK)
        return TRUE;
    return FALSE;
}

BOOL CuTreeLogin::Alter()
{
    LPNODELOGINDATAMIN pNodeLoginDataMin = (LPNODELOGINDATAMIN)GetPTreeItemData()->GetDataPtr();

    // Note : Added 22/04/97: prevent with MessageBox using ad hoc message
    // if at least one window opened on the node
    if (AtLeastOneWindowOnNode()) {
      CString rString;
      AfxFormatString1(rString, IDS_ERR_ALTERNODELOGIN_OPENNODES, (LPCTSTR)pNodeLoginDataMin->NodeName);
      BfxMessageBox(rString, MB_OK | MB_ICONEXCLAMATION);
      return FALSE;
    }

    int ires, nLoginCount = 0;
    NODEDATAMIN node;
    NODELOGINDATAMIN data;
    memset (&node, 0, sizeof (node));
    memset (&data, 0, sizeof (data));
    lstrcpy ((LPTSTR)node.NodeName, (LPTSTR)pNodeLoginDataMin->NodeName);
    ires = GetFirstMonInfo (0, OT_NODE, &node, OT_NODE_LOGINPSW, (void *)&data, NULL);
    while (ires == RES_SUCCESS) 
    {
        nLoginCount++;
        ires = GetNextMonInfo((void *)&data);
    }

    CxDlgVirtualNodeLogin dlg;
    dlg.SetAlter();
    dlg.m_bEnableCheck = (nLoginCount > 1)? FALSE: TRUE;
    dlg.m_bPrivate     = (BOOL)pNodeLoginDataMin->bPrivate;
    dlg.m_strVNodeName = (LPTSTR)pNodeLoginDataMin->NodeName;
    dlg.m_strUserName  = (LPTSTR)pNodeLoginDataMin->Login;
    dlg.m_strPassword  = (LPTSTR)pNodeLoginDataMin->Passwd;
    PushVDBADlg();
    int res = dlg.DoModal();
    PopVDBADlg();
    if (res == IDOK)
        return TRUE;
    return FALSE;
}

BOOL CuTreeLogin::Drop()
{
    int result;
    LPNODELOGINDATAMIN pNodeLoginDataMin = (LPNODELOGINDATAMIN)GetPTreeItemData()->GetDataPtr();

    // Note : Added 22/04/97: prevent with MessageBox using ad hoc message
    // if at least one window opened on the node
    if (AtLeastOneWindowOnNode()) {
      CString rString;
      AfxFormatString1(rString, IDS_ERR_DROPNODELOGIN_OPENNODES, (LPCTSTR)pNodeLoginDataMin->NodeName);
      BfxMessageBox(rString, MB_OK | MB_ICONEXCLAMATION);
      return FALSE;
    }

    CString strMsg;
    CString strItem = "??";
    HTREEITEM hSelectedItem;
    CTreeGlobalData* pTreeGD = GetPTreeGlobData();
    CTreeCtrl*       pTree   = pTreeGD->GetPTree();
    hSelectedItem =  pTree->GetSelectedItem();
    if (hSelectedItem)
        strItem   =  pTree->GetItemText (hSelectedItem);
    AfxFormatString2 (strMsg, IDS_DROP_NODE_LOGIN, (LPCTSTR)strItem, (LPCTSTR)pNodeLoginDataMin->NodeName);
    if (BfxMessageBox (strMsg, MB_ICONQUESTION|MB_YESNO) != IDYES)
        return FALSE;
    if (NodeLLInit () == RES_SUCCESS)
    {
        result = LLDropNodeLoginData (pNodeLoginDataMin);
        UpdateMonInfo(0, 0, NULL, OT_NODE);
        NodeLLTerminate();
    }
    else
        result = RES_ERR;
    if (result == RES_SUCCESS)
        return TRUE;
    CString strMessage;
    AfxFormatString1 (strMessage, IDS_DROP_NODE_LOGIN_FAILED, (LPCTSTR)pNodeLoginDataMin->NodeName);
    BfxMessageBox (strMessage, MB_OK|MB_ICONEXCLAMATION);
    return FALSE;
}

UINT CuTreeLogin::GetCustomImageId()
{
  LPNODELOGINDATAMIN pLoginDataMin;

  pLoginDataMin = (LPNODELOGINDATAMIN)GetPTreeItemData()->GetDataPtr();
  if (pLoginDataMin->bPrivate)
    return IDB_TM_CONNECT_DATA_USER;
  else
    return IDB_TM_CONNECT_DATA_GROUP;
}


CuTreeConnectionStatic::CuTreeConnectionStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemNodes (pTreeGlobalData, OT_NODE, IDS_NODE_CONNECTIONDATA, SUBBRANCH_KIND_OBJ, IDB_NODE_CONNECTIONDATA, pItemData, OT_NODE_CONNECTIONDATA, IDS_NODE_CONNECTIONDATA_NONE)
{
  SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
}


CTreeItem* CuTreeConnectionStatic::AllocateChildItem (CTreeItemData* pItemData)
{
    CuTreeConnection* pItem = new CuTreeConnection (GetPTreeGlobData(), pItemData);
    return pItem;
}


CuTreeConnectionStatic::CuTreeConnectionStatic()
{

}

BOOL CuTreeConnectionStatic::IsEnabled(UINT idMenu)
{
    LPNODEDATAMIN pNodeDataMin;

    switch (idMenu) 
    {
    case IDM_VNODEBAR08:    // Add
        if (IsErrorItem())
            return FALSE;
        // don't add if node is local
        pNodeDataMin = (LPNODEDATAMIN)GetPTreeItemData()->GetDataPtr();
        if (pNodeDataMin->bIsLocal)
            return FALSE;
        return TRUE;
    default:
        return FALSE;
    }
    return FALSE;
}

BOOL CuTreeConnectionStatic::AtLeastOneWindowOnNode()
{

  CTreeGlobalData *pTreeGD = GetPTreeGlobData();
  CTreeCtrl *pTree = pTreeGD->GetPTree();
  HTREEITEM hCurItem = pTree->GetSelectedItem();
  CTreeItem *pTreeItem = (CTreeItem *)pTree->GetItemData(hCurItem);
  ASSERT (pTreeItem == this);

  HTREEITEM hParentItem = pTree->GetParentItem(hCurItem);
  hParentItem = pTree->GetParentItem(hParentItem);
  CuTreeServer *pTreeServerItem = (CuTreeServer *)pTree->GetItemData(hParentItem);
  return pTreeServerItem->AtLeastOneWindowOnNode();
}

BOOL CuTreeConnectionStatic::Add()
{
    LPNODEDATAMIN pNodeDataMin = (LPNODEDATAMIN)GetPTreeItemData()->GetDataPtr();

    // Note : Added 22/04/97: prevent with MessageBox using ad hoc message
    // if at least one window opened on the node
    if (AtLeastOneWindowOnNode()) {
      CString rString;
      AfxFormatString1(rString, IDS_ERR_ADDNODECONNECTION_OPENNODES, (LPCTSTR)pNodeDataMin->NodeName);
      BfxMessageBox(rString, MB_OK | MB_ICONEXCLAMATION);
      return FALSE;
    }

    CxDlgVirtualNodeData dlg;
    dlg.m_bPrivate     = TRUE;
    dlg.m_strVNodeName = (LPTSTR)pNodeDataMin->NodeName;
    PushVDBADlg();
    int res = dlg.DoModal();
    PopVDBADlg();
    if (res == IDOK)
        return TRUE;
    return FALSE;
}

void CuTreeConnectionStatic::MinFillpStructReq(void *pStructReq)
{
  ASSERT (pStructReq);
  ASSERT(GetPTreeItemData());

  LPNODEDATAMIN pNodeDataMin = (LPNODEDATAMIN)GetPTreeItemData()->GetDataPtr();
  LPNODECONNECTDATAMIN pNodeConnectDataMin = (LPNODECONNECTDATAMIN)pStructReq;

  x_strcpy((char *)pNodeConnectDataMin->NodeName, (const char *)pNodeDataMin->NodeName);
}


CuTreeConnection::CuTreeConnection (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemNodes (pTreeGlobalData, OT_NODE_CONNECTIONDATA, SUBBRANCH_KIND_NONE, FALSE, pItemData)
{
  SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
  InitializeImageId(IDB_NODE_CONNECTIONDATA);
}

CuTreeConnection::CuTreeConnection()
{
}

UINT CuTreeConnection::GetCustomImageId()
{
  LPNODECONNECTDATAMIN pConnectionDataMin;

  pConnectionDataMin = (LPNODECONNECTDATAMIN)GetPTreeItemData()->GetDataPtr();
  // Emb 14/05: different images from those for CuTreeLogin
  if (pConnectionDataMin->bPrivate)
    return IDB_TM_CONNECTION_DATA_USER;
  else
    return IDB_TM_CONNECTION_DATA_GROUP;
}

BOOL CuTreeConnection::IsEnabled(UINT idMenu)
{
    switch (idMenu) 
    {
    case IDM_VNODEBAR08:    // Add
        if (IsErrorItem())
            return FALSE;
        // don't add if node is local - Complicated stuff!
        {
          CTreeGlobalData *pTreeGD = GetPTreeGlobData();
          CTreeCtrl *pTree = pTreeGD->GetPTree();
          HTREEITEM hCurItem = pTree->GetSelectedItem();
          CTreeItem *pTreeItem = (CTreeItem *)pTree->GetItemData(hCurItem);
          ASSERT (pTreeItem == this);
          
          HTREEITEM hParentItem = pTree->GetParentItem(hCurItem);
          hParentItem = pTree->GetParentItem(hParentItem);
          CuTreeServer *pTreeServerItem = (CuTreeServer *)pTree->GetItemData(hParentItem);
          LPNODEDATAMIN pNodeDataMin;
          pNodeDataMin = (LPNODEDATAMIN)pTreeServerItem->GetPTreeItemData()->GetDataPtr();
          if (pNodeDataMin->bIsLocal)
             return FALSE;
        }
        return TRUE;
    case IDM_VNODEBAR09:    // Alter
    case IDM_VNODEBAR10:    // Drop
        if (IsErrorItem() || IsNoItem())
            return FALSE;
        else
            return TRUE;
    default:
        return FALSE;
    }
    return FALSE;
}



BOOL CuTreeConnection::AtLeastOneWindowOnNode()
{

  CTreeGlobalData *pTreeGD = GetPTreeGlobData();
  CTreeCtrl *pTree = pTreeGD->GetPTree();
  HTREEITEM hCurItem = pTree->GetSelectedItem();
  CTreeItem *pTreeItem = (CTreeItem *)pTree->GetItemData(hCurItem);
  ASSERT (pTreeItem == this);

  HTREEITEM hParentItem = pTree->GetParentItem(hCurItem);
  hParentItem = pTree->GetParentItem(hParentItem);
  hParentItem = pTree->GetParentItem(hParentItem);
  CuTreeServer *pTreeServerItem = (CuTreeServer *)pTree->GetItemData(hParentItem);
  return pTreeServerItem->AtLeastOneWindowOnNode();
}

BOOL CuTreeConnection::Add()
{
    LPNODECONNECTDATAMIN pNodeConnectDataMin = (LPNODECONNECTDATAMIN)GetPTreeItemData()->GetDataPtr();

    // Note : Added 22/04/97: prevent with MessageBox using ad hoc message
    // if at least one window opened on the node
    if (AtLeastOneWindowOnNode()) {
      CString rString;
      AfxFormatString1(rString, IDS_ERR_ADDNODECONNECTION_OPENNODES, (LPCTSTR)pNodeConnectDataMin->NodeName);
      BfxMessageBox(rString, MB_OK | MB_ICONEXCLAMATION);
      return FALSE;
    }

    CxDlgVirtualNodeData dlg;
    dlg.m_bPrivate     = TRUE;
    dlg.m_strVNodeName = (LPTSTR)pNodeConnectDataMin->NodeName;

    PushVDBADlg();
    int res = dlg.DoModal();
    PopVDBADlg();
    if (res == IDOK)
        return TRUE;
    return FALSE;
}

BOOL CuTreeConnection::Alter()
{
    LPNODECONNECTDATAMIN pNodeConnectDataMin = (LPNODECONNECTDATAMIN)GetPTreeItemData()->GetDataPtr();

    // Note : Added 22/04/97: prevent with MessageBox using ad hoc message
    // if at least one window opened on the node
    if (AtLeastOneWindowOnNode()) {
      CString rString;
      AfxFormatString1(rString, IDS_ERR_ALTERNODECONNECTION_OPENNODES, (LPCTSTR)pNodeConnectDataMin->NodeName);
      BfxMessageBox(rString, MB_OK | MB_ICONEXCLAMATION);
      return FALSE;
    }

    CxDlgVirtualNodeData dlg;
    dlg.SetAlter();

    dlg.m_bPrivate          = (BOOL)pNodeConnectDataMin->bPrivate;
    dlg.m_strVNodeName      = (LPTSTR)pNodeConnectDataMin->NodeName;
    dlg.m_strRemoteAddress  = (LPTSTR)pNodeConnectDataMin->Address;
    dlg.m_strProtocol       = (LPTSTR)pNodeConnectDataMin->Protocol;
    dlg.m_strListenAddress  = (LPTSTR)pNodeConnectDataMin->Listen;
    PushVDBADlg();
    int res = dlg.DoModal();
    PopVDBADlg();
    if (res == IDOK)
        return TRUE;
    return FALSE;
}

BOOL CuTreeConnection::Drop()
{
    int result;
    LPNODECONNECTDATAMIN pNodeConnectDataMin = (LPNODECONNECTDATAMIN)GetPTreeItemData()->GetDataPtr();

    // Note : Added 22/04/97: prevent with MessageBox using ad hoc message
    // if at least one window opened on the node
    if (AtLeastOneWindowOnNode()) {
      CString rString;
      AfxFormatString1(rString, IDS_ERR_DROPNODECONNECTION_OPENNODES, (LPCTSTR)pNodeConnectDataMin->NodeName);
      BfxMessageBox(rString, MB_OK | MB_ICONEXCLAMATION);
      return FALSE;
    }

    CString strMsg;
    CString strItem = "??";
    HTREEITEM hSelectedItem;
    CTreeGlobalData* pTreeGD = GetPTreeGlobData();
    CTreeCtrl*       pTree   = pTreeGD->GetPTree();
    hSelectedItem =  pTree->GetSelectedItem();
    if (hSelectedItem)
        strItem   =  pTree->GetItemText (hSelectedItem);
    AfxFormatString2 (strMsg, IDS_DROP_NODE_DATA, (LPCTSTR)strItem, (LPCTSTR)pNodeConnectDataMin->NodeName);
    if (BfxMessageBox (strMsg, MB_ICONQUESTION|MB_YESNO) != IDYES)
        return FALSE;
    if (NodeLLInit () == RES_SUCCESS)
    {
        result = LLDropNodeConnectData (pNodeConnectDataMin);
        UpdateMonInfo(0, 0, NULL, OT_NODE);
        NodeLLTerminate();
    }
    else
        result = RES_ERR;
    if (result == RES_SUCCESS)
        return TRUE;
    CString strMessage;
    AfxFormatString1 (strMessage, IDS_DROP_NODE_DATA_FAILED, (LPCTSTR)pNodeConnectDataMin->NodeName);
    BfxMessageBox (strMessage, MB_OK|MB_ICONEXCLAMATION);
    return FALSE;
}


BOOL CuTreeServer::Add()
{
  int hNode = GetPTreeGlobData()->GetHNode();
  return GlobTreeServerAdd(hNode);
}

BOOL CuTreeServer::InstallPassword()
{
  int hNode = GetPTreeGlobData()->GetHNode();
  return GlobTreeAddInstallationPassword(hNode);
}

BOOL CuTreeServer::Alter()
{
  LPNODEDATAMIN pNodeDataMin = (LPNODEDATAMIN)GetPTreeItemData()->GetDataPtr();

  if (pNodeDataMin->bInstallPassword) {
	if (pNodeDataMin->bTooMuchInfoInInstallPassword) {
	//"The underlying node had useless login, connection, or attributes data.\n"
	//"These data will be removed during the ""Alter Installation Password"" operation.\n"
	//"Confirmation ?"
		if (BfxMessageBox (VDBA_MfcResourceString(IDS_I_ALTER_INSTALL_PASSWORD), MB_ICONQUESTION|MB_YESNO) != IDYES)
			return FALSE;
	}
    CxDlgVirtualNodeInstallationPassword dlg;
    dlg.m_strNodeName = (LPTSTR)pNodeDataMin->NodeName;
    dlg.m_strUserName = (LPTSTR)pNodeDataMin->LoginDta.Login;
    dlg.SetAlter();
    PushVDBADlg();
    int res = dlg.DoModal();
    PopVDBADlg();
    if (res == IDOK)
      return TRUE;
    return FALSE;
  }
  else {
    // Note : modified 29/3/97: don't gray menuitem, but prevent
    // with MessageBox using ad hoc message in the following cases:
    // 1) at least one window opened on the node
    // 2) node not simplified

    if (AtLeastOneWindowOnNode()) {
      CString rString;
      AfxFormatString1(rString, IDS_ERR_ALTERNODE_OPENNODES, (LPCTSTR)pNodeDataMin->NodeName);
      BfxMessageBox(rString, MB_OK | MB_ICONEXCLAMATION);
      return FALSE;
    }
    if (!pNodeDataMin->isSimplifiedModeOK) {
      CString rString;
      AfxFormatString1(rString, IDS_ERR_ALTERNODE_COMPLEX, (LPCTSTR)pNodeDataMin->NodeName);
      BfxMessageBox(rString, MB_OK | MB_ICONEXCLAMATION);
      return FALSE;
    }

    CxDlgVirtualNode dlg;
    dlg.SetAlter();

    dlg.m_bConnectionPrivate= (BOOL)pNodeDataMin->ConnectDta.bPrivate;
    dlg.m_bLoginPrivate     = (BOOL)pNodeDataMin->LoginDta.bPrivate;
    dlg.m_strVNodeName      = (LPTSTR)pNodeDataMin->NodeName;
    dlg.m_strUserName       = (LPTSTR)pNodeDataMin->LoginDta.Login;
    dlg.m_strRemoteAddress  = (LPTSTR)pNodeDataMin->ConnectDta.Address;
    dlg.m_strProtocol       = (LPTSTR)pNodeDataMin->ConnectDta.Protocol;
    dlg.m_strListenAddress  = (LPTSTR)pNodeDataMin->ConnectDta.Listen;
    PushVDBADlg();
    int res = dlg.DoModal();
    PopVDBADlg();
    if (res == IDOK)
        return TRUE;
    return FALSE;
  }
}

BOOL CuTreeServer::Drop()
{
  int result;
  LPNODEDATAMIN pNodeDataMin = (LPNODEDATAMIN)GetPTreeItemData()->GetDataPtr();

  if (pNodeDataMin->bInstallPassword) {
    // Special handling for installation password
    CString strMsg;
    strMsg.LoadString(IDS_DROP_INSTALLPASSWORD);
    if (BfxMessageBox (strMsg, MB_ICONQUESTION|MB_YESNO) != IDYES)
        return FALSE;
	if (pNodeDataMin->bTooMuchInfoInInstallPassword) {
	//"The underlying node had useless login, connection, or attributes data.\n"
	//"These data will be removed during the ""drop installation password"" operation.\n"
	//"Confirmation ?"
		if (BfxMessageBox (VDBA_MfcResourceString(IDS_I_REMOVE_INSTALL_PASSWORD), MB_ICONQUESTION|MB_YESNO) != IDYES)
			return FALSE;
	}
    if (NodeLLInit() == RES_SUCCESS)
    {
        result = LLDropFullNode (pNodeDataMin);
        UpdateMonInfo(0, 0, NULL, OT_NODE);
        NodeLLTerminate();
    }
    else
        result = RES_ERR;
  }
  else {
    CString strMsg;
    AfxFormatString1 (strMsg, IDS_DROP_NODE, (LPCTSTR)pNodeDataMin->NodeName);
    if (BfxMessageBox (strMsg, MB_ICONQUESTION|MB_YESNO) != IDYES)
        return FALSE;

    if (NodeLLInit() == RES_SUCCESS)
    {
        result = LLDropFullNode (pNodeDataMin);
        UpdateMonInfo(0, 0, NULL, OT_NODE);
        NodeLLTerminate();
    }
    else
        result = RES_ERR;
  }
  if (result == RES_SUCCESS)
     return TRUE;
  CString strMessage;
  AfxFormatString1 (strMessage, IDS_DROP_NODE_FAILED, (LPCTSTR)pNodeDataMin->NodeName);
  BfxMessageBox (strMessage, MB_OK|MB_ICONEXCLAMATION);
  return FALSE;
}


UINT CuTreeServer::GetCustomImageId()
{
  LPNODEDATAMIN pNodeDataMin = (LPNODEDATAMIN)GetPTreeItemData()->GetDataPtr();

  // installation password pseudo server?
  if (pNodeDataMin->bInstallPassword)
    return IDB_NODE_INSTALLPASSWORD;

  // connected ?
  int count = NodeConnections(pNodeDataMin->NodeName);
  BOOL bConnected = ( count!=0 ? TRUE : FALSE );

  // local ?
  BOOL bLocal = pNodeDataMin->bIsLocal;

  // simplified mode ?
  BOOL bSimplified = pNodeDataMin->isSimplifiedModeOK;

  // if simplified mode : private or global ?
  BOOL bPrivate = TRUE;
  if (bSimplified) {
    ASSERT (pNodeDataMin->inbConnectData == 1);
    ASSERT (pNodeDataMin->inbLoginData == 1);
  //  ASSERT (pNodeDataMin->LoginDta.bPrivate == pNodeDataMin->ConnectDta.bPrivate); // OBSOLETE
    bPrivate = pNodeDataMin->ConnectDta.bPrivate;
  }

  //
  // Ten different states to manage !
  //
  if (bLocal)
    if (bConnected)
      return IDB_NODE_CONNECTED_LOCAL;
    else
      return IDB_NODE_NOTCONNECTED_LOCAL;
  else
    if (bSimplified)
      if (bPrivate)
        if (bConnected)
          return IDB_NODE_CONNECTED_PRIVATE;
        else
          return IDB_NODE_NOTCONNECTED_PRIVATE;
      else
        if (bConnected)
          return IDB_NODE_CONNECTED_GLOBAL;
        else
          return IDB_NODE_NOTCONNECTED_GLOBAL;
    else
      if (bConnected)
        return IDB_NODE_CONNECTED_COMPLEX;
      else
        return IDB_NODE_NOTCONNECTED_COMPLEX;

  // forgotten cases will assert
  ASSERT (FALSE);

  /* old code , obsolete:
  if (hNode == -1)
    return IDB_NODE_NOTCONNECTED;
  else
    return IDB_NODE_CONNECTED;
  */
}

BOOL CuTreeServer::AtLeastOneWindowOnNode()
{
  int   hNode             = 0;         // no node
  int   iobjecttypeParent = GetType();
  void *pstructParent     = GetPTreeItemData()->GetDataPtr();
  int   iobjecttypeReq    = OT_NODE_OPENWIN;
  int   reqSize           = GetMonInfoStructSize(iobjecttypeReq);
  LPSFILTER psFilter      = NULL;   // no filter

  ASSERT (iobjecttypeParent == OT_NODE);
  ASSERT (pstructParent);

  // allocate one requested structure
  void *pStructReq          = new char[reqSize];

  // call GetFirstMonInfo
  int res = GetFirstMonInfo(hNode,
                            iobjecttypeParent,
                            pstructParent,
                            iobjecttypeReq,
                            pStructReq,
                            psFilter);

  // immediately free memory
  delete pStructReq;

  if (res == RES_ENDOFDATA)
    return FALSE;
  else
    return TRUE;    // includes error cases
}

void CuTreeServer::UpdateOpenedWindowsList()
{
  // Update list of opened windows on a node
  void *pstructParent = GetPTreeItemData()->GetDataPtr();
  UpdateMonInfo(0,                // hnodestruct,
                OT_NODE,          // parent type
                pstructParent,    // parent structure
                OT_NODE_OPENWIN); // requested type

  // Also, update children servers and users openwins:

  // get hItem and check against current selection
  CTreeGlobalData *pTreeGD = GetPTreeGlobData();
  CTreeCtrl *pTree = pTreeGD->GetPTree();
  HTREEITEM hItem = pTree->GetSelectedItem();
  CTreeItem *pTreeItem = (CTreeItem *)pTree->GetItemData(hItem);
  ASSERT (pTreeItem == this);
  if (pTreeItem != this)
    return ;
  if (!pTreeItem->IsAlreadyExpanded())  // Not expanded yet---> servers branch does not exist
    return;

  // children servers openwins, first by order
  HTREEITEM hServers = pTree->GetChildItem(hItem);
  pTreeItem = (CTreeItem *)pTree->GetItemData(hServers);
  if (pTreeItem->IsAlreadyExpanded()) {
    HTREEITEM hServer;
    for (hServer = pTree->GetChildItem(hServers);
         hServer;
         hServer = pTree->GetNextSiblingItem(hServer)) {
      pTreeItem = (CTreeItem *)pTree->GetItemData(hServer);
      if (pTreeItem->IsAlreadyExpanded()) {
        HTREEITEM hUsers = pTree->GetChildItem(hServer);          // static "Users"
        HTREEITEM hOpenWins = pTree->GetNextSiblingItem(hUsers);  // static "Openwins"
        pTreeItem = (CTreeItem *)pTree->GetItemData(hOpenWins);
        ASSERT (pTreeItem);
        ASSERT (pTreeItem->IsStatic() && pTreeItem->GetChildType()==OT_NODE_OPENWIN);
        // if (pTreeItem->IsAlreadyExpanded() {
        void *pstructParent = pTreeItem->GetPTreeItemData()->GetDataPtr();
        UpdateMonInfo(0,                    // hnodestruct,
                      OT_NODE_SERVERCLASS,  // parent type
                      pstructParent,        // parent structure
                      OT_NODE_OPENWIN);     // requested type

        // Sub branches "Users" of Server Class
        pTreeItem = (CTreeItem *)pTree->GetItemData(hUsers);
        if (pTreeItem->IsAlreadyExpanded()) {
          HTREEITEM hUser;
          for (hUser = pTree->GetChildItem(hUsers);
               hUser;
               hUser = pTree->GetNextSiblingItem(hUser)) {
            pTreeItem = (CTreeItem *)pTree->GetItemData(hUser);
            if (pTreeItem->IsAlreadyExpanded()) {
              HTREEITEM hOpenWins = pTree->GetChildItem(hUser);
              pTreeItem = (CTreeItem *)pTree->GetItemData(hOpenWins);
              ASSERT (pTreeItem);
              ASSERT (pTreeItem->IsStatic() && pTreeItem->GetChildType()==OT_NODE_OPENWIN);
              // if (pTreeItem->IsAlreadyExpanded() {
              void *pstructParent = pTreeItem->GetPTreeItemData()->GetDataPtr();
              UpdateMonInfo(0,                    // hnodestruct,
                            OT_USER,              // parent type
                            pstructParent,        // parent structure
                            OT_NODE_OPENWIN);     // requested type
            }
          }
        }
      }
    }
  }

  // children users openwins, second by order
  HTREEITEM hUsers = pTree->GetNextSiblingItem(hServers);
  pTreeItem = (CTreeItem *)pTree->GetItemData(hUsers);
  if (pTreeItem->IsAlreadyExpanded()) {
    HTREEITEM hUser;
    for (hUser = pTree->GetChildItem(hUsers);
         hUser;
         hUser = pTree->GetNextSiblingItem(hUser)) {
      pTreeItem = (CTreeItem *)pTree->GetItemData(hUser);
      if (pTreeItem->IsAlreadyExpanded()) {
        HTREEITEM hOpenWins = pTree->GetChildItem(hUser);
        pTreeItem = (CTreeItem *)pTree->GetItemData(hOpenWins);
        ASSERT (pTreeItem);
        ASSERT (pTreeItem->IsStatic() && pTreeItem->GetChildType()==OT_NODE_OPENWIN);
        // if (pTreeItem->IsAlreadyExpanded() {
        void *pstructParent = pTreeItem->GetPTreeItemData()->GetDataPtr();
        UpdateMonInfo(0,                    // hnodestruct,
                      OT_USER,  // parent type
                      pstructParent,        // parent structure
                      OT_NODE_OPENWIN);     // requested type
      }
    }
  }
}

BOOL CuTreeServer::Disconnect()
{
  CDlgDisconnect dlg;

  dlg.m_pNodeDataMin = (LPNODEDATAMIN)GetPTreeItemData()->GetDataPtr();
  PushHelpId(IDD_NODE_DISCONNECT);
  PushVDBADlg();
  int zap = dlg.DoModal();
  PopVDBADlg();
  PopHelpId();
  if (zap == IDOK) {
    CleanServersAndUsersBranches();
    UpdateOpenedWindowsList();    // update low-level
    return TRUE;
  }
  else
    return FALSE;
}

// Collapse and clean sub-branches "Servers" and "Users"
BOOL CuTreeServer::CleanServersAndUsersBranches()
{
  CTreeGlobalData *pTreeGD = GetPTreeGlobData();
  CTreeCtrl *pTree = pTreeGD->GetPTree();

  HTREEITEM hItem = pTree->GetSelectedItem();
  ASSERT(hItem);
  if (!hItem)
    return FALSE;
  CTreeItem *pItem = (CTreeItem *)pTree->GetItemData(hItem);
  ASSERT (pItem);
  if (!pItem)
    return FALSE;
  ASSERT (pItem == this);
  if (pItem != this)
    return FALSE;

  // not expanded yet? Fine!
  if (!IsAlreadyExpanded())
    return TRUE;

  // This code takes advantage of the tree organization!
  HTREEITEM hChildItem = pTree->GetChildItem(hItem);
  CTreeItem *pTreeItem = (CTreeItem *)pTree->GetItemData(hChildItem);
  ASSERT (pTreeItem->IsStatic());
  ASSERT (pTreeItem->GetChildType() == OT_NODE_SERVERCLASS);
  pTreeItem->MakeBranchNotExpandable(hChildItem);
  pTreeItem->MakeBranchExpandable(hChildItem);
  pTree->Expand(hChildItem, TVE_COLLAPSE);

  hChildItem = pTree->GetNextSiblingItem(hChildItem);
  pTreeItem = (CTreeItem *)pTree->GetItemData(hChildItem);
  ASSERT (pTreeItem->IsStatic());
  ASSERT (pTreeItem->GetChildType() == OT_USER);
  pTreeItem->MakeBranchNotExpandable(hChildItem);
  pTreeItem->MakeBranchExpandable(hChildItem);
  pTree->Expand(hChildItem, TVE_COLLAPSE);

  return TRUE;
}

BOOL CuTreeServer::RefreshRelatedBranches(HTREEITEM hItem, RefreshCause because)
{
  BOOL bSuccess = TRUE;

  CTreeGlobalData *pTreeGD = GetPTreeGlobData();
  CTreeCtrl *pTree = pTreeGD->GetPTree();
  
  // Verification : chech hItem data against "this"
  CTreeItem *pTreeCheckItem = (CTreeItem *)pTree->GetItemData(hItem);
  ASSERT (pTreeCheckItem == this);
  if (pTreeCheckItem != this)
    return FALSE;
  
  if (because == REFRESH_ALTER) {

    // not expanded yet? Fine!
    if (!IsAlreadyExpanded())
      return TRUE;

    // This code takes advantage of the tree organization!
    HTREEITEM hChildItem = pTree->GetChildItem(hItem);
    hChildItem = pTree->GetNextSiblingItem(hChildItem);

    hChildItem = pTree->GetChildItem(hChildItem);
    CTreeItem *pTreeItem = (CTreeItem *)pTree->GetItemData(hChildItem);
    ASSERT (pTreeItem->IsStatic());
    ASSERT (pTreeItem->GetChildType() == OT_NODE_LOGINPSW);
    bSuccess = pTreeItem->RefreshDataSubBranches(hChildItem);
    if (!bSuccess)
      return bSuccess;

    hChildItem = pTree->GetNextSiblingItem(hChildItem);
    pTreeItem = (CTreeItem *)pTree->GetItemData(hChildItem);
    ASSERT (pTreeItem->IsStatic());
    ASSERT (pTreeItem->GetChildType() == OT_NODE_CONNECTIONDATA);
    bSuccess = pTreeItem->RefreshDataSubBranches(hChildItem);
    if (!bSuccess)
      return bSuccess;

    return TRUE;
  }

  if (because == REFRESH_DISCONNECT) {
    CTreeItem *pTreeItem = (CTreeItem *)pTree->GetItemData(hItem);
    ASSERT (pTreeItem);
    if (pTreeItem->IsAlreadyExpanded()) {
      BOOL bSuccess2 = TRUE;

      // First by order: list of servers
      HTREEITEM hServers = pTree->GetChildItem(hItem);
      CTreeItem *pTreeItem = (CTreeItem *)pTree->GetItemData(hServers);
      if (pTreeItem->IsAlreadyExpanded())
        bSuccess2 = bSuccess2 && pTreeItem->RefreshDataSubBranches(hServers);

      // Second by order: list of users
      HTREEITEM hUsers = pTree->GetNextSiblingItem(hServers);
      pTreeItem = (CTreeItem *)pTree->GetItemData(hUsers);
      if (pTreeItem->IsAlreadyExpanded())
        bSuccess2 = bSuccess2 && pTreeItem->RefreshDataSubBranches(hUsers);

      return bSuccess2;
    }
  }
  return TRUE;
}

BOOL CuTreeOpenwin::IsEnabled(UINT idMenu)
{
  switch (idMenu) {
    case IDM_VNODEBAR06:    // CloseWin
    case IDM_VNODEBAR07:    // ActivateWin
      if (IsNoItem() || IsErrorItem())
        return FALSE;
      return TRUE;
    default:
      return CTreeItemNodes::IsEnabled(idMenu);
  }
  return FALSE;   // security if return forgotten in switch
}

BOOL CuTreeOpenwin::ActivateWin()
{
  LPWINDOWDATAMIN pWindowDataMin = (LPWINDOWDATAMIN)GetPTreeItemData()->GetDataPtr();

  ASSERT (pWindowDataMin);
  ASSERT (pWindowDataMin->hwnd);
  HWND    hwnd = pWindowDataMin->hwnd;

  CWnd *pWnd;
  pWnd = pWnd->FromHandlePermanent(hwnd);
  ASSERT (pWnd);
  if (pWnd) {
    CMDIChildWnd *pMdiChild = (CMDIChildWnd *)pWnd;
    pMdiChild->MDIActivate();
    if (pMdiChild->IsIconic())
      pMdiChild->MDIRestore();
    return TRUE;
  }
  else
    return FALSE;
}

BOOL CuTreeOpenwin::CloseWin()
{
  LPWINDOWDATAMIN pWindowDataMin = (LPWINDOWDATAMIN)GetPTreeItemData()->GetDataPtr();

  ASSERT (pWindowDataMin);
  ASSERT (pWindowDataMin->hwnd);
  HWND    hwnd = pWindowDataMin->hwnd;

  BOOL bSuccess = TRUE;

  CWnd *pWnd;
  pWnd = pWnd->FromHandlePermanent(hwnd);
  ASSERT (pWnd);
  if (pWnd) {
    CMDIChildWnd *pMdiChild = (CMDIChildWnd *)pWnd;
    ForgetDelayedUpdates();
    pMdiChild->MDIDestroy();
    AcceptDelayedUpdates();
    UpdateOpenedWindowsList();  // update low level
  }
  else
    bSuccess = FALSE;
  return bSuccess;
}

void CuTreeOpenwin::UpdateOpenedWindowsList()
{
  // escalate parent branch in tree to get "good" parent structure
  CTreeGlobalData *pTreeGD = GetPTreeGlobData();
  CTreeCtrl *pTree = pTreeGD->GetPTree();

  HTREEITEM hItem = pTree->GetSelectedItem();
  CTreeItem *pTreeItem = (CTreeItem *)pTree->GetItemData(hItem);
  ASSERT (pTreeItem == this);
  if (pTreeItem != this)
    return;
  hItem = pTree->GetParentItem(hItem);
  CuTreeOpenwinStatic *pTOS = (CuTreeOpenwinStatic *)pTree->GetItemData(hItem);
  ASSERT (pTOS);
  if (pTOS) {
    void *pstructParent = pTOS->GetPTreeItemData()->GetDataPtr();
    UpdateMonInfo(0,                // hnodestruct,
                  OT_NODE,          // parent type
                  pstructParent,    // parent structure
                  OT_NODE_OPENWIN); // requested type
  }
}

CuTreeSvrClassStatic::CuTreeSvrClassStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemNodes (pTreeGlobalData, OT_NODE, IDS_SVRCLASS, SUBBRANCH_KIND_OBJ, IDB_SVRCLASS, pItemData, OT_NODE_SERVERCLASS, IDS_SVRCLASS_NONE)
{
  SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
}

CTreeItem* CuTreeSvrClassStatic::AllocateChildItem (CTreeItemData* pItemData)
{
    CuTreeSvrClass* pItem = new CuTreeSvrClass (GetPTreeGlobData(), pItemData);
    return pItem;
}

CuTreeSvrClassStatic::CuTreeSvrClassStatic()
{
}

BOOL CuTreeSvrClassStatic::IsEnabled(UINT idMenu)
{
  return CTreeItemNodes::IsEnabled(idMenu);
}

void CuTreeSvrClassStatic::UpdateParentBranchesAfterExpand(HTREEITEM hItem)
{
  CTreeGlobalData *pTreeGD = GetPTreeGlobData();
  CTreeCtrl *pTree = pTreeGD->GetPTree();

  // Verification : chech hItem data against "this"
  CTreeItem *pTreeCheckItem = (CTreeItem *)pTree->GetItemData(hItem);
  ASSERT (pTreeCheckItem == this);
  if (pTreeCheckItem != this)
    return;

  // This code takes advantage of the tree organization!
  hItem = pTree->GetParentItem(hItem);
  CTreeItem *pTreeItem = (CTreeItem *)pTree->GetItemData(hItem);
  ASSERT (pTreeItem->IsKindOf(RUNTIME_CLASS(CuTreeServer)));
  pTreeItem->ChangeImageId(pTreeItem->GetCustomImageId(), hItem);
}

CuTreeSvrClass::CuTreeSvrClass (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemNodes (pTreeGlobalData, OT_NODE_SERVERCLASS, SUBBRANCH_KIND_STATIC, FALSE, pItemData)
{
  SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
  InitializeImageId(IDB_SVRCLASS);
}

BOOL CuTreeSvrClass::CreateStaticSubBranches (HTREEITEM hParentItem)
{
  CTreeGlobalData* pGlobalData = GetPTreeGlobData();

  CuTreeSvrUserStatic* pStat2 = new CuTreeSvrUserStatic (pGlobalData, GetPTreeItemData());
  pStat2->CreateTreeLine (hParentItem);

  CuTreeSvrOpenwinStatic* pStat1  = new CuTreeSvrOpenwinStatic (pGlobalData, GetPTreeItemData());
  pStat1->CreateTreeLine (hParentItem);

  return TRUE;
}

CuTreeSvrClass::CuTreeSvrClass()
{

}

BOOL CuTreeSvrClass::IsEnabled(UINT idMenu)
{
  LPNODESVRCLASSDATAMIN pServerDataMin;
  CString csFullNode;

  switch (idMenu) {
    case IDM_VNODEBAR01:    // Connect
    case IDM_VNODEBAR02:    // SqlTest
    case IDM_VNODEBARSC:    // Scratchpad
      if (IsNoItem() || IsErrorItem())
        return FALSE;
      if (idMenu == IDM_VNODEBAR01 || idMenu == IDM_VNODEBARSC)
      {
          pServerDataMin = (LPNODESVRCLASSDATAMIN)GetPTreeItemData()->GetDataPtr();
          CString strServerClass = (LPCTSTR)pServerDataMin->ServerClass;
          BOOL bIngres = INGRESII_IsIngresGateway (strServerClass);
          if (!bIngres)
              return TRUE;
          if (strServerClass.CompareNoCase(_T("INGRES")) == 0)
              return TRUE;
          return FALSE;
      }
      return TRUE;
      break;

    case IDM_VNODEBAR03:    // Monitor Ipm
      {
        if (IsNoItem() || IsErrorItem())
          return FALSE;
        // needs to be OpenIngres 2.0 node - not desktop, not gateway
        pServerDataMin = (LPNODESVRCLASSDATAMIN)GetPTreeItemData()->GetDataPtr();
        if (!IsServerClassIngres(pServerDataMin))
          return FALSE;
        return TRUE;
      }
      break;
    case IDM_VNODEBAR04:    // Dbevent
        if (IsNoItem() || IsErrorItem())
          return FALSE;
        // DBEvent always available
        return TRUE;
      break;
    case IDM_VNODEBAR05:    // Disconnect
      if (IsNoItem() || IsErrorItem())
        return FALSE;
      pServerDataMin = (LPNODESVRCLASSDATAMIN)GetPTreeItemData()->GetDataPtr();
      csFullNode = BuildFullNodeName(pServerDataMin);
      //if (GetVirtNodeHandle((LPUCHAR)(LPCTSTR)csFullNode) == -1)
      if (NodeServerConnections((LPUCHAR)(LPCTSTR)csFullNode) == 0)
        return FALSE;   // Not connected!
      return TRUE;
      break;

    case IDM_VNODEBAR08:    // Add
    case IDM_VNODEBAR09:    // Alter
    case IDM_VNODEBAR10:    // Drop
        return FALSE;
      break;

    case ID_SERVERS_TESTNODE:
        return FALSE;
      break;

    default:
      return CTreeItemNodes::IsEnabled(idMenu);
      break;
  }

  ASSERT (FALSE);
  return FALSE;   // security if return forgotten in switch
}

BOOL CuTreeSvrClass::Connect()
{
  // Create a dom type document

  int oldOIVers = GetOIVers();

  LPNODESVRCLASSDATAMIN pServerDataMin = (LPNODESVRCLASSDATAMIN)GetPTreeItemData()->GetDataPtr();
  
  char szNodeName[200];
  lstrcpy(szNodeName, BuildFullNodeName(pServerDataMin));
  BOOL bOk = MainOnSpecialCommand(AfxGetMainWnd()->m_hWnd,
                                  IDM_CONNECT,
                                  (DWORD)szNodeName);
  if (bOk)
    UpdateOpenedWindowsList();    // update low-level
  else {
    SetOIVers(oldOIVers);
  }
  return bOk;
}

BOOL CuTreeSvrClass::SqlTest(LPCTSTR lpszDatabase)
{
	LPNODESVRCLASSDATAMIN pServerDataMin = (LPNODESVRCLASSDATAMIN)GetPTreeItemData()->GetDataPtr();
	CString strNode = pServerDataMin->NodeName;
	CString strServer = pServerDataMin->ServerClass;
	CString strUser = _T("");
	CString strDatabase = lpszDatabase;

	CdSqlQuery* pSqlDoc = CreateSqlTest(strNode, strServer, strUser, strDatabase);
	if (pSqlDoc)
	{
		UpdateOpenedWindowsList();    // Update low-level: AFTER SetTitle()!
		return TRUE;
	}
	return FALSE;
}

BOOL CuTreeSvrClass::Monitor()
{
	LPNODESVRCLASSDATAMIN pServerDataMin = (LPNODESVRCLASSDATAMIN)GetPTreeItemData()->GetDataPtr();
	CString strNode = pServerDataMin->NodeName;
	CString strServer = pServerDataMin->ServerClass;
	CString strUser = _T("");
	CdIpm* pIpmDoc = CreateMonitor(strNode, strServer, strUser);
	if (pIpmDoc)
	{
		UpdateOpenedWindowsList();    // Update low-level: AFTER SetTitle()!
		return TRUE;
	}
	return FALSE;
}

BOOL CuTreeSvrClass::Dbevent(LPCTSTR lpszDatabase)
{
  int oldOIVers = GetOIVers();

  LPNODESVRCLASSDATAMIN pServerDataMin = (LPNODESVRCLASSDATAMIN)GetPTreeItemData()->GetDataPtr();

  char szNodeName[200];
  lstrcpy(szNodeName, BuildFullNodeName(pServerDataMin));
  if (!CheckConnection((LPUCHAR)szNodeName,FALSE,TRUE)) {
    SetOIVers(oldOIVers);
    return FALSE;   // ERROR!
  }

  int hNode = OpenNodeStruct((LPUCHAR)szNodeName);
  if (hNode == -1) {
    SetOIVers(oldOIVers);
    CString strMsg =  VDBA_MfcResourceString (IDS_MAX_NB_CONNECT);  // _T("Maximum number of connections has been reached"
    strMsg += VDBA_MfcResourceString (IDS_E_DBEVENT_TRACE);         // " - Cannot create DBEvent Trace Window."
    MessageBox(GetFocus(),strMsg, NULL, MB_OK | MB_ICONEXCLAMATION);
    return FALSE;   // ERROR!
  }

  if (!INGRESII_IsDBEventsEnabled (pServerDataMin ))
  {
    BfxMessageBox(IDS_ERR_DBEVENT_NOTOI, MB_OK | MB_ICONEXCLAMATION);
    CloseNodeStruct(hNode, FALSE);
    SetOIVers(oldOIVers);
    return FALSE;   // ERROR!
  }

  int OIVers = GetOIVers();   // new as of check connection

	CWinApp* pWinApp = AfxGetApp();
  ASSERT (pWinApp);
  CString csDbEventTemplateString = GetDbEventTemplateString();
  POSITION pos = pWinApp->GetFirstDocTemplatePosition();
  while (pos) {
    CDocTemplate *pTemplate = pWinApp->GetNextDocTemplate(pos);
    CString docName;
    pTemplate->GetDocString(docName, CDocTemplate::docName);
    if (docName == (LPCTSTR)csDbEventTemplateString) {   // DbEvent
      // prepare detached document
      CDocument *pNewDoc = pTemplate->CreateNewDocument();
      ASSERT(pNewDoc);
      if (pNewDoc) {
        // initialize document member variables
        CDbeventDoc *pDbeventDoc = (CDbeventDoc *)pNewDoc;
        pDbeventDoc->m_hNode  = hNode;
        pDbeventDoc->m_OIVers = OIVers;
        pDbeventDoc->SetInputDatabase(lpszDatabase);
        SetOIVers(OIVers);

        // set document title
        CString capt;
        capt.LoadString(IDS_CAPT_DBEVENT);
        CString buf;
        CMainFrame* pMainFrame = (CMainFrame *)AfxGetMainWnd();
        int seqnum = pMainFrame->GetNextDbeventSeqNum();
        buf.Format("%s - %s - %d", szNodeName, (LPCTSTR)capt, seqnum);
        pNewDoc->SetTitle(buf);
        pDbeventDoc->SetSeqNum(seqnum);

        // Associate to a new frame
        CFrameWnd *pFrame = pTemplate->CreateNewFrame(pNewDoc, NULL);
        ASSERT (pFrame);
        if (pFrame) {
          pTemplate->InitialUpdateFrame(pFrame, pNewDoc);
          UpdateOpenedWindowsList();    // update low-level
          bSaveRecommended = TRUE;
          return TRUE;
        }
      }
    }
  }

  // Here, we have failed!
  CloseNodeStruct(hNode, FALSE);
  SetOIVers(oldOIVers);
  return FALSE;
}

BOOL CuTreeSvrClass::Scratchpad()
{
  // Create a scratchpad type document

  int oldOIVers = GetOIVers();

  LPNODESVRCLASSDATAMIN pServerDataMin = (LPNODESVRCLASSDATAMIN)GetPTreeItemData()->GetDataPtr();

  char szNodeName[200];
  lstrcpy(szNodeName, BuildFullNodeName(pServerDataMin));
  BOOL bOk = MainOnSpecialCommand(AfxGetMainWnd()->m_hWnd,
                                  IDM_SCRATCHPAD,
                                  (DWORD)szNodeName);
  if (bOk)
    UpdateOpenedWindowsList();    // update low-level
  else {
    SetOIVers(oldOIVers);
  }
  return bOk;
}

UINT CuTreeSvrClass::GetCustomImageId()
{
  LPNODESVRCLASSDATAMIN pServerDataMin = (LPNODESVRCLASSDATAMIN)GetPTreeItemData()->GetDataPtr();

  // connected ?
  /*
  int hNode = GetVirtNodeHandle((LPUCHAR)(LPCTSTR)BuildFullNodeName(pServerDataMin));
  BOOL bConnected = ( hNode!=-1 ? TRUE : FALSE );
  */
  BOOL bConnected;
  if (NodeServerConnections((LPUCHAR)(LPCTSTR)BuildFullNodeName(pServerDataMin)) > 0)
    bConnected = TRUE;
  else
    bConnected = FALSE;
  if (bConnected)
    return IDB_SVRCLASS_CONNECTED;
  else
    return IDB_SVRCLASS_NOTCONNECTED;
}

BOOL CuTreeSvrClass::AtLeastOneWindowOnNode()
{
  int   hNode             = 0;         // no node
  int   iobjecttypeParent = GetType();
  void *pstructParent     = GetPTreeItemData()->GetDataPtr();
  int   iobjecttypeReq    = OT_NODE_OPENWIN;
  int   reqSize           = GetMonInfoStructSize(iobjecttypeReq);
  LPSFILTER psFilter      = NULL;   // no filter

  ASSERT (iobjecttypeParent == OT_NODE_SERVERCLASS);
  ASSERT (pstructParent);

  // allocate one requested structure
  void *pStructReq          = new char[reqSize];

  // call GetFirstMonInfo
  int res = GetFirstMonInfo(hNode,
                            iobjecttypeParent,
                            pstructParent,
                            iobjecttypeReq,
                            pStructReq,
                            psFilter);

  // immediately free memory
  delete pStructReq;

  if (res == RES_ENDOFDATA)
    return FALSE;
  else
    return TRUE;    // includes error cases
}

void CuTreeSvrClass::UpdateOpenedWindowsList()
{
  // Update list of opened windows on a node
  void *pstructParent = GetPTreeItemData()->GetDataPtr();
  UpdateMonInfo(0,                    // hnodestruct,
                OT_NODE_SERVERCLASS,  // parent type
                pstructParent,        // parent structure
                OT_NODE_OPENWIN);     // requested type

  // Sub branches "Users" of Server Class
  CTreeGlobalData *pTreeGD = GetPTreeGlobData();
  CTreeCtrl *pTree = pTreeGD->GetPTree();

  HTREEITEM hItem = pTree->GetSelectedItem();
  CTreeItem *pTreeItem = (CTreeItem *)pTree->GetItemData(hItem);
  ASSERT (pTreeItem == this);
  if (pTreeItem != this)
    return;
  if (pTreeItem->IsAlreadyExpanded()) {
    HTREEITEM hUsers = pTree->GetChildItem(hItem);          // static "Users"
    pTreeItem = (CTreeItem *)pTree->GetItemData(hUsers);
    if (pTreeItem->IsAlreadyExpanded()) {
      HTREEITEM hUser;
      for (hUser = pTree->GetChildItem(hUsers);
           hUser;
           hUser = pTree->GetNextSiblingItem(hUser)) {
        pTreeItem = (CTreeItem *)pTree->GetItemData(hUser);
        if (pTreeItem->IsAlreadyExpanded()) {
          HTREEITEM hOpenWins = pTree->GetChildItem(hUser);
          pTreeItem = (CTreeItem *)pTree->GetItemData(hOpenWins);
          ASSERT (pTreeItem);
          ASSERT (pTreeItem->IsStatic() && pTreeItem->GetChildType()==OT_NODE_OPENWIN);
          // if (pTreeItem->IsAlreadyExpanded() {
          void *pstructParent = pTreeItem->GetPTreeItemData()->GetDataPtr();
          UpdateMonInfo(0,                    // hnodestruct,
                        OT_USER,              // parent type
                        pstructParent,        // parent structure
                        OT_NODE_OPENWIN);     // requested type
        }
      }
    }
  }

  // Also, escalate to CuTreeServer Parent to refresh it's windows list
  // escalate parent branch in tree to get "good" parent structure

  hItem = pTree->GetParentItem(hItem);  // svrclassstatic
  hItem = pTree->GetParentItem(hItem);  // node
  CuTreeServer* pTSV = (CuTreeServer*)pTree->GetItemData(hItem);
  ASSERT (pTSV);
  ASSERT (pTSV->IsKindOf(RUNTIME_CLASS(CuTreeServer)));
  if (pTSV) {
    void *pstructParent2 = pTSV->GetPTreeItemData()->GetDataPtr();
    UpdateMonInfo(0,                // hnodestruct,
                OT_NODE,          // parent type
                pstructParent2,   // parent structure
                OT_NODE_OPENWIN); // requested type
  }

}

BOOL CuTreeSvrClass::Disconnect()
{
  CDlgDisconnServer dlg;

  dlg.m_pServerDataMin = (LPNODESVRCLASSDATAMIN)GetPTreeItemData()->GetDataPtr();
  PushHelpId(HELPID_IDD_DISCONNECT_SVRCLASS);
  PushVDBADlg();
  int zap = dlg.DoModal();
  PopVDBADlg();
  PopHelpId();
  if (zap == IDOK) {
    CleanUsersBranch();
    UpdateOpenedWindowsList();    // update low-level
    return TRUE;
  }
  else
    return FALSE;
}
// Collapse and clean sub-branch "Users"
BOOL CuTreeSvrClass::CleanUsersBranch()
{
  CTreeGlobalData *pTreeGD = GetPTreeGlobData();
  CTreeCtrl *pTree = pTreeGD->GetPTree();

  HTREEITEM hItem = pTree->GetSelectedItem();
  ASSERT(hItem);
  if (!hItem)
    return FALSE;
  CTreeItem *pItem = (CTreeItem *)pTree->GetItemData(hItem);
  ASSERT (pItem);
  if (!pItem)
    return FALSE;
  ASSERT (pItem == this);
  if (pItem != this)
    return FALSE;

  // not expanded yet? Fine!
  if (!IsAlreadyExpanded())
    return TRUE;

  // This code takes advantage of the tree organization!
  HTREEITEM hChildItem = pTree->GetChildItem(hItem);
  CTreeItem *pTreeItem = (CTreeItem *)pTree->GetItemData(hChildItem);
  ASSERT (pTreeItem->IsStatic());
  ASSERT (pTreeItem->GetChildType() == OT_USER);
  pTreeItem->MakeBranchNotExpandable(hChildItem);
  pTreeItem->MakeBranchExpandable(hChildItem);
  pTree->Expand(hChildItem, TVE_COLLAPSE);

  return TRUE;
}

CuTreeSvrOpenwinStatic::CuTreeSvrOpenwinStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemNodes (pTreeGlobalData, OT_NODE_SERVERCLASS, IDS_NODE_OPENWIN, SUBBRANCH_KIND_OBJ, IDB_NODE_OPENWIN, pItemData, OT_NODE_OPENWIN, IDS_NODE_OPENWIN_NONE)
{
  SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
}

CTreeItem* CuTreeSvrOpenwinStatic::AllocateChildItem (CTreeItemData* pItemData)
{
    CuTreeSvrOpenwin* pItem = new CuTreeSvrOpenwin (GetPTreeGlobData(), pItemData);
    return pItem;
}

CuTreeSvrOpenwinStatic::CuTreeSvrOpenwinStatic()
{


}

CuTreeSvrOpenwin::CuTreeSvrOpenwin (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemNodes (pTreeGlobalData, OT_NODE_OPENWIN, SUBBRANCH_KIND_NONE, FALSE, pItemData)
{
  SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
  InitializeImageId(IDB_NODE_OPENWIN);
}

CuTreeSvrOpenwin::CuTreeSvrOpenwin()
{

}

BOOL CuTreeSvrOpenwin::IsEnabled(UINT idMenu)
{
  switch (idMenu) {
    case IDM_VNODEBAR06:    // CloseWin
    case IDM_VNODEBAR07:    // ActivateWin
      if (IsNoItem() || IsErrorItem())
        return FALSE;
      return TRUE;
    default:
      return CTreeItemNodes::IsEnabled(idMenu);
  }
  return FALSE;   // security if return forgotten in switch
}

BOOL CuTreeSvrOpenwin::ActivateWin()
{
  LPWINDOWDATAMIN pWindowDataMin = (LPWINDOWDATAMIN)GetPTreeItemData()->GetDataPtr();

  ASSERT (pWindowDataMin);
  ASSERT (pWindowDataMin->hwnd);
  HWND    hwnd = pWindowDataMin->hwnd;

  CWnd *pWnd;
  pWnd = pWnd->FromHandlePermanent(hwnd);
  ASSERT (pWnd);
  if (pWnd) {
    CMDIChildWnd *pMdiChild = (CMDIChildWnd *)pWnd;
    pMdiChild->MDIActivate();
    if (pMdiChild->IsIconic())
      pMdiChild->MDIRestore();
    return TRUE;
  }
  else
    return FALSE;
}

BOOL CuTreeSvrOpenwin::CloseWin()
{
  LPWINDOWDATAMIN pWindowDataMin = (LPWINDOWDATAMIN)GetPTreeItemData()->GetDataPtr();

  ASSERT (pWindowDataMin);
  ASSERT (pWindowDataMin->hwnd);
  HWND    hwnd = pWindowDataMin->hwnd;

  BOOL bSuccess = TRUE;

  CWnd *pWnd;
  pWnd = pWnd->FromHandlePermanent(hwnd);
  ASSERT (pWnd);
  if (pWnd) {
    CMDIChildWnd *pMdiChild = (CMDIChildWnd *)pWnd;
    ForgetDelayedUpdates();
    pMdiChild->MDIDestroy();
    AcceptDelayedUpdates();
    UpdateOpenedWindowsList();  // update low level
  }
  else
    bSuccess = FALSE;
  return bSuccess;
}

void CuTreeSvrOpenwin::UpdateOpenedWindowsList()
{
  // escalate parent branch in tree to get "good" parent structure
  CTreeGlobalData *pTreeGD = GetPTreeGlobData();
  CTreeCtrl *pTree = pTreeGD->GetPTree();

  HTREEITEM hItem = pTree->GetSelectedItem();
  CTreeItem *pTreeItem = (CTreeItem *)pTree->GetItemData(hItem);
  ASSERT (pTreeItem == this);
  if (pTreeItem != this)
    return;
  hItem = pTree->GetParentItem(hItem);  // static openwin
  CuTreeSvrOpenwinStatic *pTOS = (CuTreeSvrOpenwinStatic *)pTree->GetItemData(hItem);
  ASSERT (pTOS);
  ASSERT (pTOS->IsKindOf(RUNTIME_CLASS(CuTreeSvrOpenwinStatic)));
  if (pTOS) {
    void *pstructParent = pTOS->GetPTreeItemData()->GetDataPtr();
    UpdateMonInfo(0,                    // hnodestruct,
                  OT_NODE_SERVERCLASS,  // parent type
                  pstructParent,        // parent structure
                  OT_NODE_OPENWIN);     // requested type
  }

  // Also, escalate to CuTreeServer Parent to refresh it's windows list
  hItem = pTree->GetParentItem(hItem);  // server
  hItem = pTree->GetParentItem(hItem);  // server static
  hItem = pTree->GetParentItem(hItem);  // node
  CuTreeServer* pTSV = (CuTreeServer*)pTree->GetItemData(hItem);
  ASSERT (pTSV);
  ASSERT (pTSV->IsKindOf(RUNTIME_CLASS(CuTreeServer)));
  if (pTSV) {
    void *pstructParent2 = pTSV->GetPTreeItemData()->GetDataPtr();
    UpdateMonInfo(0,                // hnodestruct,
                OT_NODE,          // parent type
                pstructParent2,   // parent structure
                OT_NODE_OPENWIN); // requested type
  }
}


BOOL CuTreeSvrClass::RefreshRelatedBranches(HTREEITEM hItem, RefreshCause because)
{
  BOOL bSuccess = TRUE;

  CTreeGlobalData *pTreeGD = GetPTreeGlobData();
  CTreeCtrl *pTree = pTreeGD->GetPTree();

  // Verification : chech hItem data against "this"
  CTreeItem *pTreeCheckItem = (CTreeItem *)pTree->GetItemData(hItem);
  ASSERT (pTreeCheckItem == this);
  if (pTreeCheckItem != this)
    return FALSE;

  // This code takes advantage of the tree organization!
  hItem = pTree->GetParentItem(hItem);
  hItem = pTree->GetParentItem(hItem);
  CTreeItem *pTreeItem = (CTreeItem *)pTree->GetItemData(hItem);
  ASSERT (pTreeItem->IsKindOf(RUNTIME_CLASS(CuTreeServer)));
  pTreeItem->ChangeImageId(pTreeItem->GetCustomImageId(), hItem);

  return TRUE;
}


CuTreeOtherAttrStatic::CuTreeOtherAttrStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemNodes (pTreeGlobalData, OT_NODE, IDS_NODE_ATTRIBUTE, SUBBRANCH_KIND_OBJ, IDB_NODE_ATTRIBUTE, pItemData, OT_NODE_ATTRIBUTE, IDS_NODE_ATTRIBUTE_NONE)
{
  SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
}


CTreeItem* CuTreeOtherAttrStatic::AllocateChildItem (CTreeItemData* pItemData)
{
    CuTreeOtherAttr* pItem = new CuTreeOtherAttr (GetPTreeGlobData(), pItemData);
    return pItem;
}


CuTreeOtherAttrStatic::CuTreeOtherAttrStatic()
{

}

BOOL CuTreeOtherAttrStatic::IsEnabled(UINT idMenu)
{
    LPNODEDATAMIN pNodeDataMin;

    switch (idMenu) 
    {
    case IDM_VNODEBAR08:    // Add
        if (IsErrorItem())
            return FALSE;
        // don't add if node is local
        pNodeDataMin = (LPNODEDATAMIN)GetPTreeItemData()->GetDataPtr();
        if (pNodeDataMin->bIsLocal)
            return FALSE;
        return TRUE;
    default:
        return FALSE;
    }
    return FALSE;
}

BOOL CuTreeOtherAttrStatic::AtLeastOneWindowOnNode()
{

  CTreeGlobalData *pTreeGD = GetPTreeGlobData();
  CTreeCtrl *pTree = pTreeGD->GetPTree();
  HTREEITEM hCurItem = pTree->GetSelectedItem();
  CTreeItem *pTreeItem = (CTreeItem *)pTree->GetItemData(hCurItem);
  ASSERT (pTreeItem == this);

  HTREEITEM hParentItem = pTree->GetParentItem(hCurItem);
  hParentItem = pTree->GetParentItem(hParentItem);
  CuTreeServer *pTreeServerItem = (CuTreeServer *)pTree->GetItemData(hParentItem);
  return pTreeServerItem->AtLeastOneWindowOnNode();
}

BOOL CuTreeOtherAttrStatic::Add()
{
  LPNODEDATAMIN pNodeDataMin = (LPNODEDATAMIN)GetPTreeItemData()->GetDataPtr();

  if (AtLeastOneWindowOnNode()) {
    CString rString;
    AfxFormatString1(rString, IDS_ERR_ADDATTRIBUTE_OPENNODES, (LPCTSTR)pNodeDataMin->NodeName);
    BfxMessageBox(rString, MB_OK | MB_ICONEXCLAMATION);
    return FALSE;
  }

  CxDlgVirtualNodeAttribute dlg;
  dlg.m_bPrivate     = TRUE;
  dlg.m_strVNodeName = (LPTSTR)pNodeDataMin->NodeName;
  PushVDBADlg();
  int res = dlg.DoModal();
  PopVDBADlg();
  if (res == IDOK)
      return TRUE;

  return FALSE;
}

void CuTreeOtherAttrStatic::MinFillpStructReq(void *pStructReq)
{
  ASSERT (pStructReq);
  ASSERT(GetPTreeItemData());

  LPNODEDATAMIN pNodeDataMin = (LPNODEDATAMIN)GetPTreeItemData()->GetDataPtr();
  LPNODEATTRIBUTEDATAMIN pNodeAttributeDataMin = (LPNODEATTRIBUTEDATAMIN)pStructReq;

  x_strcpy((char *)pNodeAttributeDataMin->NodeName, (const char *)pNodeDataMin->NodeName);
}


CuTreeOtherAttr::CuTreeOtherAttr (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemNodes (pTreeGlobalData, OT_NODE_ATTRIBUTE, SUBBRANCH_KIND_NONE, FALSE, pItemData)
{
  SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
  InitializeImageId(IDB_NODE_ATTRIBUTE);     // the default
}

CuTreeOtherAttr::CuTreeOtherAttr()
{
}

UINT CuTreeOtherAttr::GetCustomImageId()
{
  LPNODEATTRIBUTEDATAMIN pNodeAttributeDataMin;

  pNodeAttributeDataMin = (LPNODEATTRIBUTEDATAMIN)GetPTreeItemData()->GetDataPtr();
  if (pNodeAttributeDataMin->bPrivate)
    return IDB_TM_ATTRIBUTE_USER;
  else
    return IDB_TM_ATTRIBUTE_GROUP;
}

BOOL CuTreeOtherAttr::IsEnabled(UINT idMenu)
{
    switch (idMenu) 
    {
    case IDM_VNODEBAR08:    // Add
        if (IsErrorItem())
            return FALSE;
        // don't add if node is local - Complicated stuff!
        {
          CTreeGlobalData *pTreeGD = GetPTreeGlobData();
          CTreeCtrl *pTree = pTreeGD->GetPTree();
          HTREEITEM hCurItem = pTree->GetSelectedItem();
          CTreeItem *pTreeItem = (CTreeItem *)pTree->GetItemData(hCurItem);
          ASSERT (pTreeItem == this);
          
          HTREEITEM hParentItem = pTree->GetParentItem(hCurItem);
          hParentItem = pTree->GetParentItem(hParentItem);
          CuTreeServer *pTreeServerItem = (CuTreeServer *)pTree->GetItemData(hParentItem);
          LPNODEDATAMIN pNodeDataMin;
          pNodeDataMin = (LPNODEDATAMIN)pTreeServerItem->GetPTreeItemData()->GetDataPtr();
          if (pNodeDataMin->bIsLocal)
             return FALSE;
        }
        return TRUE;
    case IDM_VNODEBAR09:    // Alter
    case IDM_VNODEBAR10:    // Drop
        if (IsErrorItem() || IsNoItem())
            return FALSE;
        else
            return TRUE;
    default:
        return FALSE;
    }
    return FALSE;
}



BOOL CuTreeOtherAttr::AtLeastOneWindowOnNode()
{

  CTreeGlobalData *pTreeGD = GetPTreeGlobData();
  CTreeCtrl *pTree = pTreeGD->GetPTree();
  HTREEITEM hCurItem = pTree->GetSelectedItem();
  CTreeItem *pTreeItem = (CTreeItem *)pTree->GetItemData(hCurItem);
  ASSERT (pTreeItem == this);

  HTREEITEM hParentItem = pTree->GetParentItem(hCurItem);
  hParentItem = pTree->GetParentItem(hParentItem);
  hParentItem = pTree->GetParentItem(hParentItem);
  CuTreeServer *pTreeServerItem = (CuTreeServer *)pTree->GetItemData(hParentItem);
  return pTreeServerItem->AtLeastOneWindowOnNode();
}

BOOL CuTreeOtherAttr::Add()
{
  LPNODEATTRIBUTEDATAMIN pNodeAttributeDataMin = (LPNODEATTRIBUTEDATAMIN)GetPTreeItemData()->GetDataPtr();

  // Note : Added 22/04/97: prevent with MessageBox using ad hoc message
  // if at least one window opened on the node
  if (AtLeastOneWindowOnNode()) {
    CString rString;
    AfxFormatString1(rString, IDS_ERR_ADDATTRIBUTE_OPENNODES, (LPCTSTR)pNodeAttributeDataMin->NodeName);
    BfxMessageBox(rString, MB_OK | MB_ICONEXCLAMATION);
    return FALSE;
  }

  CxDlgVirtualNodeAttribute dlg;
  dlg.m_bPrivate     = TRUE;
  dlg.m_strVNodeName = (LPTSTR)pNodeAttributeDataMin->NodeName;

  PushVDBADlg();
  int res = dlg.DoModal();
  PopVDBADlg();
  if (res == IDOK)
      return TRUE;
  return FALSE;
}

BOOL CuTreeOtherAttr::Alter()
{
  LPNODEATTRIBUTEDATAMIN pNodeAttributeDataMin = (LPNODEATTRIBUTEDATAMIN)GetPTreeItemData()->GetDataPtr();

  // Note : Added 22/04/97: prevent with MessageBox using ad hoc message
  // if at least one window opened on the node
  if (AtLeastOneWindowOnNode()) {
    CString rString;
    AfxFormatString1(rString, IDS_ERR_ALTERATTRIBUTE_OPENNODES, (LPCTSTR)pNodeAttributeDataMin->NodeName);
    BfxMessageBox(rString, MB_OK | MB_ICONEXCLAMATION);
    return FALSE;
  }

  CxDlgVirtualNodeAttribute dlg;
  dlg.SetAlter();
  dlg.m_strAttributeName = (LPCTSTR)pNodeAttributeDataMin->AttributeName;
  dlg.m_strAttributeValue= (LPCTSTR)pNodeAttributeDataMin->AttributeValue;
  dlg.m_strVNodeName     = (LPCTSTR)pNodeAttributeDataMin->NodeName;
  dlg.m_bPrivate = (BOOL)pNodeAttributeDataMin->bPrivate;
  PushVDBADlg();
  int res = dlg.DoModal();
  PopVDBADlg();
  if (res == IDOK)
      return TRUE;

  return FALSE;
}

BOOL CuTreeOtherAttr::Drop()
{
  int result;
  LPNODEATTRIBUTEDATAMIN pNodeAttributeDataMin = (LPNODEATTRIBUTEDATAMIN)GetPTreeItemData()->GetDataPtr();

  // Note : Added 22/04/97: prevent with MessageBox using ad hoc message
  // if at least one window opened on the node
  if (AtLeastOneWindowOnNode()) {
    CString rString;
    AfxFormatString1(rString, IDS_ERR_DROPATTRIBUTE_OPENNODES, (LPCTSTR)pNodeAttributeDataMin->NodeName);
    BfxMessageBox(rString, MB_OK | MB_ICONEXCLAMATION);
    return FALSE;
  }

  CString strMsg;
  CString strItem = "??";
  HTREEITEM hSelectedItem;
  CTreeGlobalData* pTreeGD = GetPTreeGlobData();
  CTreeCtrl*       pTree   = pTreeGD->GetPTree();
  hSelectedItem =  pTree->GetSelectedItem();
  if (hSelectedItem)
      strItem   =  pTree->GetItemText (hSelectedItem);
  AfxFormatString2 (strMsg, IDS_DROP_NODE_ATTRIBUTE, (LPCTSTR)strItem, (LPCTSTR)pNodeAttributeDataMin->NodeName);
  if (BfxMessageBox (strMsg, MB_ICONQUESTION|MB_YESNO) != IDYES)
      return FALSE;

  if (NodeLLInit () == RES_SUCCESS)
  {
      result = LLDropNodeAttributeData (pNodeAttributeDataMin);
      UpdateMonInfo(0, 0, NULL, OT_NODE);
      NodeLLTerminate();
  }
  else
      result = RES_ERR;
  if (result == RES_SUCCESS)
      return TRUE;
  CString strMessage;
  AfxFormatString1 (strMessage, IDS_DROP_NODE_ATTRIBUTE_FAILED, (LPCTSTR)pNodeAttributeDataMin->NodeName);
  BfxMessageBox (strMessage, MB_OK|MB_ICONEXCLAMATION);
  return FALSE;
}


