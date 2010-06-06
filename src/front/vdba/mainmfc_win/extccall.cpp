/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : extccall.cpp, Implementation file
**
**  Project  : Ingres II/ VDBA.
**
**  Author   : UK Sotheavut
**
**  Purpose  : Interface for extern "C" caller
**
**  History after 04-May-1999:
**	18-Jan-2000 (schph01)
**	    bug #100034 copy the dbname between the struture LPINDEXPARAMS and 
**	    DMLCREATESTRUCT.
**	15-feb-2000 (somsa01)
**	    Removed extra comma from enum declaration.
**  21-Dec-2000 (noifr01)
**   (SIR 103548) use the new prototype of function GetGWlistLL() (pass FALSE
**   for getting the whole list of gateways)
**  26-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
** 02-May-2001 (uk$so01)
**    SIR #102678
**    Support the composite histogram of base table and secondary index.
** 30-May-2001 (uk$so01)
**    SIR #104736, Integrate IIA and Cleanup old code of populate.
** 18-Jun-2001 (uk$so01)
**    BUG #104799, Deadlock on modify structure when there exist an opened 
**    cursor on the table.
** 10-Dec-2001 (noifr01)
**   (sir 99596) removal of obsolete code and resources
** 14-Fev-2002 (uk$so01)
**    SIR #106648, Integrate SQL Assistant In-Process COM Server
** 04-Jun-2002 (hanje04)
**    Aninmation had been disabled for UNIX because of problems with
**    the animation thread to exiting. This is causing an error to be 
**    return from Task_BkRefresh every time a refresh is performed.
**    So we no longer perform an 'is animation running' check for MAINWIN.
**  26-Mar-2002 (noifr01)
**   (bug 107442) removed the "force refresh" dialog box in DOM windows: force
**    refresh now refreshes all data
** 18-Mar-2003 (schph01)
**    sir 107523 management of sequences
** 13-Jun-2003 (uk$so01)
**    SIR #106648, Take into account the STAR database for connection.
** 17-Jul-2003 (uk$so01)
**    SIR #106648, Vdba-Split, ensure that the ingres sessions 
**    have the descriptions.
** 20-Sep-2004 (uk$so01)
**    BUG 113080 / ISSUE 13691268, Fail to open the SQL Assistant because
**    the vnode contains extra info like: "(local) /INGRES (user:uk$so01)"
**    SQL Assistant needs seperate info: node=(local), svr=INGRES, user=uk$so01
** 20-Aug-2008 (whiro01)
**    Remove redundant <afx...> include which is already in "stdafx.h"
** 09-Mar-2009 (smeke01) b121764
**    Change parameters to CreateSQLWizard CreateSelectStmWizard, CreateSQLWizard,
**    and CreateSearchCondWizard to LPUCHAR so that we don't need to use the T2A macro.
**    This is because on non-Unicode builds, T2A uses the original pointer, resulting
**    in the above functions unintentionally altering the stored virtual node name, which
**    in turn can cause bug 121764.
** 20-May-2010 (drivi01)
**    Add a case statement for Ingres Vectorwise tables to load
**    a slightly modified tree icon.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "extccall.h"
#include "xdgidxop.h" // Index Option Dialog
#include "xdlgindx.h" // Create Index Dialog
#include "xtbpkey.h"  // Table Primary Key
#include "xdlgwait.h" // Task Interruptible Dialog
#include "xinstenl.h" // Enable/Disable Security Auditing.
#include "xdlgrule.h" // Create Rule Dialog Box.
#include "xdlgdbgr.h" // Grant Database Dialog Box.
#include "xdlgdrop.h" // Drop Object with Cascade|Restrict
#include "sqlquery.h"
#include "xdlgpref.h" // Preference Dialog
#include "chooseob.h" // Choose Objects Dialog box

#include "maindoc.h"  // CMainMfcDoc
#include "mainvi2.h"  // CMainMfcView2
#include "sqlasinf.h" // sql assistant interface
#include "libguids.h" // GUIDs
#include "sqlselec.h"

extern "C" 
{
#include "dba.h"
#include "dbaset.h"
#include "dbadlg1.h"
#include "dbaginfo.h"
#include "winutils.h"
#include "monitor.h"
#include "error.h"
#include "domdisp.h"
#include "tree.h"
#include "getdata.h"
#include "domdata.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
extern "C" void VDBA_TRACE0 (LPCTSTR lpszText) {TRACE0 (lpszText);}

void NotifyAnimationDlgTxt(HWND hwnd, LPCTSTR buf)
{
	::SendMessage(hwnd, WM_EXECUTE_TASK, (LPARAM)1, (LPARAM)(LPCTSTR)buf);
}


int Task_DbaAddObjectInteruptible(LPCTSTR lpszVNode, int iObjType, LPVOID lpParam, int nhSession)
{
	//
	// If "NOT USE ANIMATION" or "ANIMATION IS ALREADY RUNNING":
	if (!theApp.IsAnimateEnabled() || theApp.IsDlgWaitRunning())
		return DBAAddObjectLL((LPUCHAR)lpszVNode, iObjType, lpParam, nhSession);

	CaExecParamAddObject* pAddObject = NULL;
	try
	{
		pAddObject = new CaExecParamAddObject();
		pAddObject->SetExecParam (lpszVNode, iObjType, lpParam, nhSession);
		
		CxDlgWait dlgWait;
		dlgWait.SetExecParam (pAddObject);
		dlgWait.SetUseAnimateAvi(IDR_ANFETCHF);
		dlgWait.DoModal();
		return dlgWait.m_nReturnCode;
	}
	catch (...)
	{
		CString strMsg = _T("Internal Error While Creating Object ...\n");
		TRACE0 (strMsg);
	}
	return RES_ERR;
}


int Task_DbaAlterObjectInteruptible (LPVOID lpOriginParam, LPVOID lpNewParam, int iObjType, int nhSession)
{
	CaExecParamAlterObject* pAlterObject = NULL;
	try
	{
		pAlterObject = new CaExecParamAlterObject();
		pAlterObject->SetExecParam (lpOriginParam, lpNewParam, iObjType, nhSession);
	
		CxDlgWait dlgWait;
		dlgWait.SetExecParam (pAlterObject);
		dlgWait.DoModal();
		return dlgWait.m_nReturnCode;
	}
	catch (...)
	{
		CString strMsg = _T("Cannot execute task DBAAlterObject ...\n");
		TRACE0 (strMsg);
	}
	return RES_ERR;
}



int Task_DbaDropObjectInteruptible  (LPCTSTR lpszVNode, int iObjType, LPVOID lpParam, int nhSession)
{
	CaExecParamDropObject* pDropObject = NULL;
	try
	{
		pDropObject = new CaExecParamDropObject();
		pDropObject->SetExecParam (lpszVNode, iObjType, lpParam, nhSession);
	
		CxDlgWait dlgWait;
		dlgWait.SetExecParam (pDropObject);
		dlgWait.DoModal();
		return dlgWait.m_nReturnCode;
	}
	catch (...)
	{
		CString strMsg = _T("Cannot execute task DBADropObject ...\n");
		TRACE0 (strMsg);
	}
	return RES_ERR;
}


int Task_GetDetailInfoInterruptible (LPCTSTR lpszVNode, int iObjType, LPVOID lpParam, BOOL bRetainSessForLock, int* ilocsession)
{
	//
	// If "NOT USE ANIMATION" or "ANIMATION IS ALREADY RUNNING":
	if (!theApp.IsAnimateEnabled() || theApp.IsDlgWaitRunning())
		return GetDetailInfoLL ((LPUCHAR)lpszVNode,iObjType,lpParam, bRetainSessForLock,ilocsession);

	CaExecParamGetDetailInfo* pDetailInfo = NULL;
	try
	{
		pDetailInfo = new CaExecParamGetDetailInfo();
		pDetailInfo->SetExecParam ((void *)lpszVNode, iObjType, lpParam, bRetainSessForLock, ilocsession);
	
		CxDlgWait dlgWait(VDBA_MfcResourceString(IDS_I_FETCHING_INF));//"Fetching Information ..."
		dlgWait.SetExecParam (pDetailInfo);
		dlgWait.CenterDialog(CxDlgWait::OPEN_CENTER_AT_MOUSE);

		dlgWait.SetUseAnimateAvi(IDR_ANFETCHR);
//		dlgWait.SetSize2(.28, .48);
		dlgWait.DoModal();
		return dlgWait.m_nReturnCode;
	}
	catch (...)
	{
		CString strMsg = _T("Internal error while getting object information ...\n");
		TRACE0 (strMsg);
	}
	return RES_ERR;
}


//
// Call MonGetDetailInfo(int hnode, void* pbigstruct, int oldtype, int newtype);
int Task_MonGetDetailInfoInterruptible  (int nHNode, LPVOID lpBigStruct, int nOldType, int nNewType)
{
	//
	// If "NOT USE ANIMATION" or "ANIMATION IS ALREADY RUNNING":
	if (!theApp.IsAnimateEnabled() || theApp.IsDlgWaitRunning())
		return MonGetDetailInfoLL(nHNode,lpBigStruct,nOldType, nNewType);

	CaExecParamMonGetDetailInfo* pDetailInfo = NULL;
	try
	{
		pDetailInfo = new CaExecParamMonGetDetailInfo();
		pDetailInfo->SetExecParam (nHNode, lpBigStruct, nOldType, nNewType);

		CxDlgWait dlgWait(VDBA_MfcResourceString(IDS_I_FETCHING_INF));
		dlgWait.SetExecParam (pDetailInfo);
		dlgWait.SetUseAnimateAvi(IDR_ANFETCHR);
//		dlgWait.SetSize2(.5, 1.0);

		dlgWait.DoModal();
		return dlgWait.m_nReturnCode;
	}
	catch (...)
	{
		CString strMsg = _T("Cannot execute task MonGetDetailInfo ...\n");
		TRACE0 (strMsg);
	}
	return RES_ERR;
}

int Task_GetNodesList ()
{
	//
	// If "NOT USE ANIMATION" or "ANIMATION IS ALREADY RUNNING":
	if (!theApp.IsAnimateEnabled() || theApp.IsDlgWaitRunning())
		return NodeLLFillNodeListsLL();

	CaExecParamGetNodesList* pGetFirstObject = NULL;
	try
	{
		pGetFirstObject = new CaExecParamGetNodesList();
		CxDlgWait dlgWait(VDBA_MfcResourceString(IDS_I_GET_LIST_NODES));//"Getting the List of Nodes ..."
		dlgWait.m_bDeleteExecParam = FALSE;
		dlgWait.SetExecParam (pGetFirstObject);
		dlgWait.SetUseAnimateAvi(IDR_ANCLOCK);
//		dlgWait.CenterDialog(CxDlgWait::OPEN_CENTER_AT_MOUSE);
//		dlgWait.SetSize2(.28, .48);
		dlgWait.DoModal();

		delete pGetFirstObject;
		return dlgWait.m_nReturnCode;
	}
	catch (...)
	{
		CString strMsg = _T("Internal Error while getting the list of nodes\n");
		TRACE0 (strMsg);
	}
	return RES_ERR;
}

int Task_BkRefresh (time_t refreshtime)
{
	//
	// If "NOT USE ANIMATION" or "ANIMATION IS ALREADY RUNNING":
#ifndef MAINWIN
	if (!theApp.IsAnimateEnabled() || theApp.IsDlgWaitRunning())
		return myerror(ERR_REFRESH_WIN);
#endif

	CaExecParamBkRefresh* pBkRefresh = NULL;
	try
	{
		pBkRefresh = new CaExecParamBkRefresh();
		pBkRefresh->SetExecParam (refreshtime);
		CxDlgWait dlgWait(VDBA_MfcResourceString(IDS_I_BACKGROUND_REFRESH));//"Background Refresh in Progress..."
		dlgWait.m_bDeleteExecParam = FALSE;
		dlgWait.SetExecParam (pBkRefresh);
//		dlgWait.CenterDialog(CxDlgWait::OPEN_CENTER_AT_MOUSE);
		dlgWait.SetUseAnimateAvi(IDR_ANREFRESH);
		dlgWait.SetUseExtraInfo();
//		dlgWait.SetSize2(.28, .48);
		dlgWait.DoModal();

		delete pBkRefresh;
		return dlgWait.m_nReturnCode;
	}
	catch (...)
	{
		CString strMsg = _T("Internal Error during Background Refresh\n");
		TRACE0 (strMsg);
	}
	return RES_ERR;
}

LPUCHAR * Task_GetGWList(LPUCHAR host)
{
	//
	// If "NOT USE ANIMATION" or "ANIMATION IS ALREADY RUNNING":
	if (!theApp.IsAnimateEnabled() || theApp.IsDlgWaitRunning())
		return GetGWlistLL(host, FALSE);

	CaExecParamGetGWList* pGWListParms = NULL;
	try
	{
		LPUCHAR * presult;
		pGWListParms = new CaExecParamGetGWList();
		pGWListParms->SetExecParam ((TCHAR *)host);
		CxDlgWait dlgWait(VDBA_MfcResourceString(IDS_I_LST_SVR));//"Getting List of Servers ..."
		dlgWait.m_bDeleteExecParam = FALSE;
		dlgWait.SetExecParam (pGWListParms);
		dlgWait.SetUseAnimateAvi(IDR_ANFETCHR);
		dlgWait.SetUseExtraInfo();
		dlgWait.DoModal();
		presult = (LPUCHAR *) pGWListParms->m_presult;
		delete pGWListParms; // pointer in presult is still valid (static)
		return presult;
	}
	catch (...)
	{
		CString strMsg = _T("Internal Error during Background Refresh\n");
		TRACE0 (strMsg);
	}
	return (LPUCHAR *)NULL;


}
int Task_DBAGetFirstObjectInterruptible (
	LPUCHAR lpVirtNode,
	int     iobjecttype,
	int     level,
	LPUCHAR *parentstrings,
	BOOL    bWithSystem,
	LPUCHAR lpobjectname,
	LPUCHAR lpownername,
	LPUCHAR lpextradata)
{
	//
	// If "NOT USE ANIMATION" or "ANIMATION IS ALREADY RUNNING":
	if (!theApp.IsAnimateEnabled() || theApp.IsDlgWaitRunning())
	{
		BOOL bOK = DBAGetFirstObjectLL(
			lpVirtNode,
			iobjecttype,
			level,
			parentstrings,
			bWithSystem,
			lpobjectname,
			lpownername,
			lpextradata);
		return bOK;
	}

	CaExecParamDBAGetFirstObject* pGetFirstObject = NULL;
	try
	{
		pGetFirstObject = new CaExecParamDBAGetFirstObject();
		pGetFirstObject->SetExecParam (
			lpVirtNode,
			iobjecttype,
			level,
			parentstrings,
			bWithSystem,
			lpobjectname,
			lpownername,
			lpextradata);

		CxDlgWait dlgWait(VDBA_MfcResourceString(IDS_I_FETCHING_INF));
		dlgWait.m_bDeleteExecParam = FALSE;
		dlgWait.SetExecParam (pGetFirstObject);
		dlgWait.SetUseAnimateAvi(IDR_ANFETCHR);
		dlgWait.DoModal();

		delete pGetFirstObject;
		return dlgWait.m_nReturnCode;
	}
	catch (...)
	{
		CString strMsg = _T("Internal Error while getting data\n");
		TRACE0 (strMsg);
	}
	return RES_ERR;
}

int Task_DOMGetFirstObjectInteruptible (
	int nHNode, 
	int nOIType, 
	int nLevel, 
	LPUCHAR* lpParentString,
	BOOL bSystem,
	LPUCHAR lpfilterowner,
	LPUCHAR lpresultobjectname,
	LPUCHAR lpresultowner,
	LPUCHAR lpresultextrastring)
{
	CaExecParamDOMGetFirstObject* pGetFirstObject = NULL;
	try
	{
		pGetFirstObject = new CaExecParamDOMGetFirstObject();
		pGetFirstObject->SetExecParam (
			nHNode,
			nOIType,
			nLevel,
			(LPVOID)lpParentString,
			bSystem);

		CxDlgWait dlgWait;
		dlgWait.m_bDeleteExecParam = FALSE;
		dlgWait.SetExecParam (pGetFirstObject);
		dlgWait.DoModal();

		if (lpfilterowner)
			lstrcpy ((LPTSTR)lpfilterowner, pGetFirstObject->m_tchszFilterOwner);
		if (lpresultobjectname)
			lstrcpy ((LPTSTR)lpresultobjectname, pGetFirstObject->m_tchszResultObject);
		if (lpresultowner)
			lstrcpy ((LPTSTR)lpresultowner, pGetFirstObject->m_tchszResultOwner);
		if (lpresultextrastring)
			lstrcpy ((LPTSTR)lpresultextrastring, pGetFirstObject->m_tchszResultExtra);
		
		delete pGetFirstObject;
		return dlgWait.m_nReturnCode;
	}
	catch (...)
	{
		CString strMsg = _T("Cannot execute task Task_DOMGetFirstObjectInteruptible ...\n");
		TRACE0 (strMsg);
	}
	return RES_ERR;
}



extern "C" int VDBA_CreateIndex(HWND hwndParent, DMLCREATESTRUCT* pCr)
{
	CxDlgCreateIndex dlg;
	dlg.SetCreateParam(pCr);

	int answer = dlg.DoModal();
	return answer;
}

extern "C" void INDEXPARAMS2DMLCREATESTRUCT (LPVOID pIndexParams, DMLCREATESTRUCT* pCr)
{
	LPUCHAR pStatement = NULL;
	int iRes = BuildSQLCreIdx(&pStatement, (LPINDEXPARAMS)pIndexParams);
	if (iRes != RES_SUCCESS)
		return;
	pCr->lpszStatement  = NULL;
	pCr->lpszStatement2 = (LPCTSTR)pStatement;
	x_strcpy(pCr->tchszDatabase,(LPTSTR)((LPINDEXPARAMS)(pIndexParams))->DBName);
}

//
// Constraint Enhancement (Index Option):
// nCallFor = 0 : primary key constraint enforcement.
// nCallFor = 1 : unique key constraint enforcement.
// nCallFor = 2 : foreign key constraint enforcement.
int VDBA_IndexOption(
	HWND hwndParent, 
	LPCTSTR lpszDatabase, 
	int nCallFor, 
	INDEXOPTION*  pConstraintWithClause,
	LPTABLEPARAMS pTable)
{
	CxDlgIndexOption dlg;
	dlg.SetTableParams ((LPVOID)pTable);
	dlg.m_nCallFor = nCallFor;

	switch (nCallFor)
	{
	case 0:
		dlg.m_strTitle.LoadString(IDS_T_PRIMARY_KEY);      // = _T("Primary Key Index Enforcement");
		break;
	case 1:
		dlg.m_strTitle.LoadString(IDS_T_UNIQUE_KEY);       // = _T("Unique Key Index Enforcement");
		break;
	case 2:
		dlg.m_strTitle.LoadString(IDS_T_FOREIGN_KEY);      // = _T("Foreign Key Index Enforcement");
		break;
	default:
		dlg.m_strTitle.LoadString(IDS_T_INDEX_ENFORCEMENT);// = _T("Index Enforcement");
		break;
	}

	dlg.m_pParam = (LPVOID)pConstraintWithClause;
	dlg.m_strDatabase = lpszDatabase;
	if (!pTable->bCreate)
	{
		dlg.m_strTable = (LPCTSTR)StringWithoutOwner (pTable->objectname);
		dlg.m_strTableOwner = (LPCTSTR)pTable->szSchema;
		dlg.m_bFromCreateTable = FALSE;
	}
	else
		dlg.m_bFromCreateTable = TRUE;

	int answer = dlg.DoModal();
	if (answer == IDOK)
	{
		INDEXOPTION_IDX2STRUCT (&dlg, (LPVOID)pConstraintWithClause);
	}
	return answer;
}


int VDBA_TablePrimaryKey(
	HWND hwndParent, 
	LPCTSTR lpszDatabase, 
	PRIMARYKEYSTRUCT* pPrimaryKey,
	LPTABLEPARAMS pTable)
{
	CxDlgTablePrimaryKey dlg;
	if (!pTable->bCreate)
	{
		dlg.m_strTable = (LPCTSTR)StringWithoutOwner (pTable->objectname);
		dlg.m_strTableOwner = (LPCTSTR)pTable->szSchema;
	}
	dlg.SetTableParams ((LPVOID)pTable);
	dlg.SetPrimaryKeyParam (pPrimaryKey);
	dlg.m_strDatabase = lpszDatabase;
	return dlg.DoModal();
}


BOOL INDEXOPTION_COPY (INDEXOPTION* pDest, INDEXOPTION* pSource)
{
	lstrcpy (pDest->tchszIndexName,  pSource->tchszIndexName);
	lstrcpy (pDest->tchszStructure,  pSource->tchszStructure);
	lstrcpy (pDest->tchszFillfactor, pSource->tchszFillfactor);
	lstrcpy (pDest->tchszMinPage,    pSource->tchszMinPage);
	lstrcpy (pDest->tchszMaxPage,    pSource->tchszMaxPage);
	lstrcpy (pDest->tchszLeaffill,   pSource->tchszLeaffill);
	lstrcpy (pDest->tchszNonleaffill,pSource->tchszNonleaffill);
	lstrcpy (pDest->tchszAllocation, pSource->tchszAllocation);
	lstrcpy (pDest->tchszExtend,     pSource->tchszExtend);
	
	pDest->pListLocation   = StringList_Copy (pSource->pListLocation);
	pDest->bAlter          = pSource->bAlter;
	pDest->bDefineProperty = pSource->bDefineProperty;
	pDest->nGenerateMode   = pSource->nGenerateMode;
	return TRUE;
}


BOOL VDBA_EnableDisableSecurityLevel()
{
	CxDlgInstallLevelSecurityEnableLevel dlg;

	return (dlg.DoModal() == IDOK)? TRUE: FALSE;
}

int WINAPI __export DlgCreateRule (HWND hwndOwner, LPRULEPARAMS lpruleparams)
{
	CxDlgRule dlg;
	dlg.SetParam ((LPVOID)lpruleparams);

	return (dlg.DoModal() == IDOK)? 1: 0;
}

int WINAPI __export DlgGrantDatabase (HWND hwndOwner, LPGRANTPARAMS lpgrantparams)
{
	CxDlgGrantDatabase dlg;
	dlg.SetParam ((LPVOID)lpgrantparams);

	return (dlg.DoModal() == IDOK)? 1: 0;
}

BOOL VDBA_DropObjectCacadeRestrict(DROPOBJECTSTRUCT* pStruct, LPCTSTR lpszTitle)
{
	CxDlgDropObjectCascadeRestrict dlg;
	dlg.m_strTitle = lpszTitle;
	dlg.SetParam ((LPVOID)pStruct);

	return (dlg.DoModal() == IDOK)? TRUE: FALSE;
}

BOOL CanBeInSecondaryThead()
{
	return theApp.IsDlgWaitRunning();
}
 
int Notify_MT_Action(WPARAM wParam, LPARAM lParam)
{
	DBAThreadDisableCurrentSession();
	int ires = (int) ::SendMessage (AfxGetMainWnd()->m_hWnd, WM_EXECUTE_TASK, wParam, lParam);
	DBAThreadEnableCurrentSession();
	return ires;
}

LONG ManageBaseListAction(WPARAM wParam, LPARAM lParam, BOOL *bActionComplete)
{
	long lResult = 0L;
	DBAThreadEnableCurrentSession();
	switch ((int)wParam) {
		case ACTION_DOMUPDATEDISPLAYDATA:
			{
				DOMUPDATEDISPLAYDATA * pdata = (DOMUPDATEDISPLAYDATA *) lParam;
				lResult = (long) DOMUpdateDisplayData_MT (
									pdata->hnodestruct,
									pdata->iobjecttype,
									pdata->level,
									pdata->parentstrings,
                  pdata->bWithChildren,
                  pdata->iAction,
                  pdata->idItem,
                  pdata->hwndDOMDoc);
									/* PREVIOUS PARAMETERS BY FNN, SPECIFIC TO BK REFRESH:  FALSE,ACT_BKTASK,0L,0); */
			}
			break;
		case ACTION_UPDATENODEDISPLAY:
			UpdateNodeDisplay_MT();
			break;
		case ACTION_UPDATEDBEDISPLAY:
			{
				UPDATEDBEDISPLAYPARAMS * pdata = (UPDATEDBEDISPLAYPARAMS *) lParam;
				UpdateDBEDisplay_MT(pdata->hnodestruct,pdata->dbname);
			}
			break;
		case ACTIONREFRESHPROPSWINDOW:
			{
				UPDATEREFRESHPROPSPARAMS * pdata = (UPDATEREFRESHPROPSPARAMS *) lParam;
//UKS				lResult = (long) RefreshPropWindows_MT(pdata->bOnLoad,pdata->refreshtime);
			}
			break;
		case ACTION_MESSAGEBOX:
			{
				MESSAGEBOXPARAMS * pdata = (MESSAGEBOXPARAMS *) lParam;
				lResult = (long) ::MessageBox(pdata->hWnd,pdata->pTxt,pdata->pTitle,pdata->uType);
			}
			break;
		case ACTION_GETFOCUS:
			lResult = (long)GetFocus();
			break;
		case ACTION_UPDATEMONDISPLAY:
			{
				UPDMONDISPLAYPARAMS * pdata = (UPDMONDISPLAYPARAMS *) lParam;
			}
			break;
		case ACTION_SENDMESSAGE:
			{
				SENDMESSAGEPARAMS * pdata = (SENDMESSAGEPARAMS *) lParam;
				lResult = SendMessage(pdata->hWnd,
									pdata->Msg,
									pdata->wParam,
									pdata->lParam);
			}
			break;

    case ACTION_UPDATERIGHTPANE:
      {
        LPUPDATERIGHTPANEPARAMS lpData = (LPUPDATERIGHTPANEPARAMS)lParam;
        ASSERT (lpData);
        MT_MfcUpdateRightPane(lpData->hwndDoc,
                              lpData->lpDomData,
                              lpData->bCurSelStatic,
                              lpData->dwCurSel,
                              lpData->CurItemObjType,
                              lpData->lpRecord,
                              lpData->bHasProperties,
                              lpData->bUpdate,
                              lpData->initialSel,
                              lpData->bClear);
      }
      break;

		default:
			*bActionComplete = FALSE;
			return lResult;
	}
	*bActionComplete = TRUE;
	DBAThreadDisableCurrentSession();
	return lResult;
}

BOOL VDBA_DlgPreferences()
{
	CxDlgPreferences dlg;
	return (dlg.DoModal() == IDOK)? TRUE: FALSE;
}

//
// Expand branch
//
int Task_ManageExpandBranchInterruptible(HWND hwndMdi, LPDOMDATA lpDomData, DWORD dwItem, HWND hWndTaskDlg)
{
  //
  // If "NOT USE ANIMATION" or "ANIMATION IS ALREADY RUNNING":
  if (!theApp.IsAnimateEnabled() || theApp.IsDlgWaitRunning())
    return ManageExpandBranch_MT(hwndMdi, lpDomData, dwItem, hWndTaskDlg);

  CaExecParamExpandBranch* pExpandBranchParam = NULL;
  try
  {
    pExpandBranchParam= new CaExecParamExpandBranch();
    pExpandBranchParam->SetExecParam (hwndMdi, lpDomData, dwItem);

    CxDlgWait dlgWait(VDBA_MfcResourceString(IDS_I_EXPAND_BRANCH));//"Expanding Branch..."
    dlgWait.SetExecParam (pExpandBranchParam);
    dlgWait.SetUseAnimateAvi(IDR_ANFETCHR);
		dlgWait.SetUseExtraInfo();

    dlgWait.DoModal();
    return dlgWait.m_nReturnCode;
  }
  catch (...)
  {
    CString strMsg = _T("Cannot execute task Expand Branch...\n");
    TRACE0 (strMsg);
  }
  return RES_ERR;
}

//
// Expand all
//
int Task_ManageExpandAllInterruptible(HWND hwndMdi, LPDOMDATA lpDomData, HWND hWndTaskDlg)
{
  //
  // If "NOT USE ANIMATION" or "ANIMATION IS ALREADY RUNNING":
  if (!theApp.IsAnimateEnabled() || theApp.IsDlgWaitRunning())
    return ManageExpandAll_MT(hwndMdi, lpDomData, hWndTaskDlg);

  CaExecParamExpandAll* pExpandAllParam = NULL;
  try
  {
    pExpandAllParam= new CaExecParamExpandAll();
    pExpandAllParam->SetExecParam (hwndMdi, lpDomData);

    CxDlgWait dlgWait(VDBA_MfcResourceString(IDS_I_EXPAND_ALL_BRANCH));//"Expanding all Branches..."
    dlgWait.SetExecParam (pExpandAllParam);
    dlgWait.SetUseAnimateAvi(IDR_ANFETCHR);
		dlgWait.SetUseExtraInfo();

    dlgWait.DoModal();
    return dlgWait.m_nReturnCode;
  }
  catch (...)
  {
    CString strMsg = _T("Cannot execute task Expand all Branches...\n");
    TRACE0 (strMsg);
  }
  return RES_ERR;
}

//
// Drag-Drop
//
int Task_StandardDragDropEndInterruptible(HWND hwndMdi, LPDOMDATA lpDomData, BOOL bRightPane, LPTREERECORD lpRightPaneRecord, HWND hwndDomDest, LPDOMDATA lpDestDomData)
{
  //
  // If "NOT USE ANIMATION" or "ANIMATION IS ALREADY RUNNING":
  if (!theApp.IsAnimateEnabled() || theApp.IsDlgWaitRunning())
    return StandardDragDropEnd_MT(hwndMdi, lpDomData, bRightPane, lpRightPaneRecord, hwndDomDest, lpDestDomData);

  CaExecParamDragDropEnd* pDragDropEndParam = NULL;
  try
  {
    pDragDropEndParam= new CaExecParamDragDropEnd();
    pDragDropEndParam->SetExecParam (hwndMdi, lpDomData, bRightPane, lpRightPaneRecord, hwndDomDest, lpDestDomData);

    CxDlgWait dlgWait(VDBA_MfcResourceString(IDS_I_DUPLI_OBJ));//"Duplicating Object..."
    dlgWait.SetExecParam (pDragDropEndParam);
    dlgWait.SetUseAnimateAvi(IDR_ANFETCHR);
		//dlgWait.SetUseExtraInfo();

    dlgWait.DoModal();
    return dlgWait.m_nReturnCode;
  }
  catch (...)
  {
    CString strMsg = _T("Cannot execute task Drag-Drop...\n");
    TRACE0 (strMsg);
  }
  return RES_ERR;
}


//
// Force refresh of branch and sub branches
//
int Task_ManageForceRefreshCursubInterruptible(HWND hwndMdi, LPDOMDATA lpDomData, DWORD dwItem, HWND hWndTaskDlg)
{
  //
  // If "NOT USE ANIMATION" or "ANIMATION IS ALREADY RUNNING":
  if (!theApp.IsAnimateEnabled() || theApp.IsDlgWaitRunning())
    return ManageForceRefreshCursub_MT(hwndMdi, lpDomData, dwItem, hWndTaskDlg);

  CaExecParamForceRefreshCurSub* pForceRefreshCurSubParam = NULL;
  try
  {
    pForceRefreshCurSubParam= new CaExecParamForceRefreshCurSub();
    pForceRefreshCurSubParam->SetExecParam (hwndMdi, lpDomData, dwItem);

    CxDlgWait dlgWait(VDBA_MfcResourceString(IDS_I_REFRESH_CURR_BRANCH));//"Refreshing Current and Sub-Branch..."
    dlgWait.SetExecParam (pForceRefreshCurSubParam);
    dlgWait.SetUseAnimateAvi(IDR_ANFETCHR);
		dlgWait.SetUseExtraInfo();

    dlgWait.DoModal();
    return dlgWait.m_nReturnCode;
  }
  catch (...)
  {
    CString strMsg = _T("Cannot execute task Refresh Branch and Subbranches...\n");
    TRACE0 (strMsg);
  }
  return RES_ERR;
}

//
// Force refresh of all branches
//
int Task_ManageForceRefreshAllInterruptible(HWND hwndMdi, LPDOMDATA lpDomData, HWND hWndTaskDlg)
{
  //
  // If "NOT USE ANIMATION" or "ANIMATION IS ALREADY RUNNING":
  if (!theApp.IsAnimateEnabled() || theApp.IsDlgWaitRunning())
    return ManageForceRefreshAll_MT(hwndMdi, lpDomData, hWndTaskDlg);

  CaExecParamForceRefreshAll* pForceRefreshAllParam = NULL;
  try
  {
    pForceRefreshAllParam= new CaExecParamForceRefreshAll();
    pForceRefreshAllParam->SetExecParam (hwndMdi, lpDomData);

    CxDlgWait dlgWait(VDBA_MfcResourceString(IDS_I_REFRESH_ALL_BRANCH));//"Refreshing All Branches..."
    dlgWait.SetExecParam (pForceRefreshAllParam);
    dlgWait.SetUseAnimateAvi(IDR_ANFETCHR);
		dlgWait.SetUseExtraInfo();

    dlgWait.DoModal();
    return dlgWait.m_nReturnCode;
  }
  catch (...)
  {
    CString strMsg = _T("Cannot execute task Refresh All Branches...\n");
    TRACE0 (strMsg);
  }
  return RES_ERR;
}

enum
{
	IMGINGRES_DATABASE = 0,
	IMGINGRES_USER
};

//
// New implementation: Extract an icon from the image list.
// The zero base index of image list is calculated from the [in] parameter
// type.
HICON LoadIconInCache(int type)
{
  HICON hIcon1;

  // NOTE : uses OT_STATIC_XYZ EXCEPT for STAR ICONS

  switch (type) {
    //
    // Star management : icons for star objects
    //
    // databases:
    case OT_STAR_DB_DDB:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STAR_DB_DDB);
      break;
    case OT_STAR_DB_CDB:
    case OT_STATIC_R_CDB:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STAR_DB_CDB);
      break;

    // tables:
    case OT_STAR_TBL_NATIVE:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STAR_TBL_NATIVE);
      break;
    case OT_STAR_TBL_LINK:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STAR_TBL_LINK);
      break;
	case OT_VW_TABLE:
	  hIcon1 = IconCacheLoadIcon(IDI_TREE_VW_TABLE);
	  break;

    // views:
    case OT_STAR_VIEW_NATIVE:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STAR_VIEW_NATIVE);
      break;
    case OT_STAR_VIEW_LINK:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STAR_VIEW_LINK);
      break;
    case OT_STAR_PROCEDURE:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STAR_PROCEDURE);
      break;
    case OT_STAR_INDEX:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STAR_INDEX);
      break;

    case OT_STATIC_DATABASE:
    case OT_STATIC_R_USERSCHEMA:
    case OT_STATIC_R_DBGRANT:
    case OT_STATIC_R_DBGRANT_ACCESY:
    case OT_STATIC_R_DBGRANT_ACCESN:
    case OT_STATIC_R_DBGRANT_CREPRY:
    case OT_STATIC_R_DBGRANT_CREPRN:
    case OT_STATIC_R_DBGRANT_CRETBY:
    case OT_STATIC_R_DBGRANT_CRETBN:
    case OT_STATIC_R_DBGRANT_DBADMY:
    case OT_STATIC_R_DBGRANT_DBADMN:
    case OT_STATIC_R_DBGRANT_LKMODY:
    case OT_STATIC_R_DBGRANT_LKMODN:
    case OT_STATIC_R_DBGRANT_QRYIOY:
    case OT_STATIC_R_DBGRANT_QRYION:
    case OT_STATIC_R_DBGRANT_QRYRWY:
    case OT_STATIC_R_DBGRANT_QRYRWN:
    case OT_STATIC_R_DBGRANT_UPDSCY:
    case OT_STATIC_R_DBGRANT_UPDSCN:
    case OT_STATIC_R_DBGRANT_SELSCY:
    case OT_STATIC_R_DBGRANT_SELSCN:
    case OT_STATIC_R_DBGRANT_CNCTLY:
    case OT_STATIC_R_DBGRANT_CNCTLN:
    case OT_STATIC_R_DBGRANT_IDLTLY:
    case OT_STATIC_R_DBGRANT_IDLTLN:
    case OT_STATIC_R_DBGRANT_SESPRY:
    case OT_STATIC_R_DBGRANT_SESPRN:
    case OT_STATIC_R_DBGRANT_TBLSTY:
    case OT_STATIC_R_DBGRANT_TBLSTN:
    case OT_STATIC_R_DBGRANT_QRYCPY:
    case OT_STATIC_R_DBGRANT_QRYCPN:
    case OT_STATIC_R_DBGRANT_QRYPGY:
    case OT_STATIC_R_DBGRANT_QRYPGN:
    case OT_STATIC_R_DBGRANT_QRYCOY:
    case OT_STATIC_R_DBGRANT_QRYCON:
    case OT_STATIC_R_DBGRANT_CRESEQY:
    case OT_STATIC_R_DBGRANT_CRESEQN:
    case OT_STATIC_INSTALL                  :
    case OT_STATIC_INSTALL_GRANTEES:
    case OT_STATIC_INSTALL_ALARMS           :
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_DATABASE);
      break;
    case OT_STATIC_PROFILE:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_PROFILE);
      break;
    case OT_STATIC_USER:
    case OT_STATIC_GROUPUSER:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_USER);
      break;
    case OT_STATIC_R_USERGROUP:
    case OT_STATIC_GROUP:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_GROUP);
      break;
    case OT_STATIC_ROLE:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_ROLE);
      break;
    case OT_STATIC_LOCATION:
    case OT_STATIC_TABLELOCATION:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_LOCATION);
      break;
    case OT_STATIC_SYNONYMED:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_SYNONYMED);
      break;

    case OT_STATIC_TABLE:
    case OT_STATIC_SCHEMAUSER_TABLE:
    case OT_STATIC_REPLIC_REGTABLE:
    case OT_STATIC_R_REPLIC_CDDS_TABLE:
    case OT_STATIC_R_LOCATIONTABLE:
    case OT_STATIC_R_TABLEGRANT:
    case OT_STATIC_R_TABLEGRANT_SEL:
    case OT_STATIC_R_TABLEGRANT_INS:
    case OT_STATIC_R_TABLEGRANT_UPD:
    case OT_STATIC_R_TABLEGRANT_DEL:
    case OT_STATIC_R_TABLEGRANT_REF:
    case OT_STATIC_R_TABLEGRANT_CPI:
    case OT_STATIC_R_TABLEGRANT_CPF:
    case OT_STATIC_R_TABLEGRANT_ALL:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_TABLE);
      break;
    case OT_STATIC_VIEW:
    case OT_STATIC_SCHEMAUSER_VIEW:
    case OT_STATIC_R_TABLEVIEW:
    case OT_STATIC_R_VIEWGRANT:
    case OT_STATIC_R_VIEWGRANT_SEL:
    case OT_STATIC_R_VIEWGRANT_INS:
    case OT_STATIC_R_VIEWGRANT_UPD:
    case OT_STATIC_R_VIEWGRANT_DEL:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_VIEW);
      break;
    case OT_STATIC_PROCEDURE:
    case OT_STATIC_SCHEMAUSER_PROCEDURE:
    case OT_STATIC_R_PROCGRANT:
    case OT_STATIC_R_PROCGRANT_EXEC:
    case OT_STATIC_RULEPROC:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_PROCEDURE);
      break;
    case OT_STATIC_SCHEMA:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_SCHEMA);
      break;
    case OT_STATIC_SYNONYM:
    case OT_STATIC_R_TABLESYNONYM:
    case OT_STATIC_R_VIEWSYNONYM:
    case OT_STATIC_R_INDEXSYNONYM:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_SYNONYM);
      break;
    case OT_STATIC_DBGRANTEES:
    case OT_STATIC_DBGRANTEES_ACCESY:
    case OT_STATIC_DBGRANTEES_ACCESN:
    case OT_STATIC_DBGRANTEES_CREPRY:
    case OT_STATIC_DBGRANTEES_CREPRN:
    case OT_STATIC_DBGRANTEES_CRETBY:
    case OT_STATIC_DBGRANTEES_CRETBN:
    case OT_STATIC_DBGRANTEES_DBADMY:
    case OT_STATIC_DBGRANTEES_DBADMN:
    case OT_STATIC_DBGRANTEES_LKMODY:
    case OT_STATIC_DBGRANTEES_LKMODN:
    case OT_STATIC_DBGRANTEES_QRYIOY:
    case OT_STATIC_DBGRANTEES_QRYION:
    case OT_STATIC_DBGRANTEES_QRYRWY:
    case OT_STATIC_DBGRANTEES_QRYRWN:
    case OT_STATIC_DBGRANTEES_UPDSCY:
    case OT_STATIC_DBGRANTEES_UPDSCN:
    case OT_STATIC_DBGRANTEES_SELSCY:
    case OT_STATIC_DBGRANTEES_SELSCN:
    case OT_STATIC_DBGRANTEES_CNCTLY:
    case OT_STATIC_DBGRANTEES_CNCTLN:
    case OT_STATIC_DBGRANTEES_IDLTLY:
    case OT_STATIC_DBGRANTEES_IDLTLN:
    case OT_STATIC_DBGRANTEES_SESPRY:
    case OT_STATIC_DBGRANTEES_SESPRN:
    case OT_STATIC_DBGRANTEES_TBLSTY:
    case OT_STATIC_DBGRANTEES_TBLSTN:
    case OT_STATIC_DBGRANTEES_QRYCPY:
    case OT_STATIC_DBGRANTEES_QRYCPN:
    case OT_STATIC_DBGRANTEES_QRYPGY:
    case OT_STATIC_DBGRANTEES_QRYPGN:
    case OT_STATIC_DBGRANTEES_QRYCOY:
    case OT_STATIC_DBGRANTEES_QRYCON:
    case OT_STATIC_DBGRANTEES_CRSEQY:
    case OT_STATIC_DBGRANTEES_CRSEQN:
    case OT_STATIC_TABLEGRANT_SEL_USER:
    case OT_STATIC_TABLEGRANT_INS_USER:
    case OT_STATIC_TABLEGRANT_UPD_USER:
    case OT_STATIC_TABLEGRANT_DEL_USER:
    case OT_STATIC_TABLEGRANT_REF_USER:
    case OT_STATIC_TABLEGRANT_CPI_USER:
    case OT_STATIC_TABLEGRANT_CPF_USER:
    case OT_STATIC_TABLEGRANT_ALL_USER:
    // desktop
    case OT_STATIC_TABLEGRANT_INDEX_USER:
    // desktop
    case OT_STATIC_TABLEGRANT_ALTER_USER:
    case OT_STATIC_TABLEGRANTEES:
    case OT_STATIC_VIEWGRANTEES:
    case OT_STATIC_VIEWGRANT_SEL_USER:
    case OT_STATIC_VIEWGRANT_INS_USER:
    case OT_STATIC_VIEWGRANT_UPD_USER:
    case OT_STATIC_VIEWGRANT_DEL_USER:
    case OT_STATIC_DBEGRANT_RAISE_USER:
    case OT_STATIC_DBEGRANT_REGTR_USER:
    case OT_STATIC_ROLEGRANT_USER:
    case OT_STATIC_PROCGRANT_EXEC_USER:
    case OT_STATIC_SEQGRANT_NEXT_USER:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_DBGRANTEES);
      break;

    case OT_STATIC_DBEVENT:
    case OT_STATIC_R_DBEGRANT:
    case OT_STATIC_R_DBEGRANT_RAISE:
    case OT_STATIC_R_DBEGRANT_REGISTER:
    case OT_STATIC_ALARM_EVENT:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_DBEVENT);
      break;
    case OT_STATIC_DBALARM:
    case OT_STATIC_DBALARM_CONN_SUCCESS:
    case OT_STATIC_DBALARM_CONN_FAILURE:
    case OT_STATIC_DBALARM_DISCONN_SUCCESS:
    case OT_STATIC_DBALARM_DISCONN_FAILURE:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_DBALARM);
      break;

    case OT_STATIC_R_GRANT:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_R_GRANT);
      break;
    case OT_STATIC_R_ROLEGRANT:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_R_ROLEGRANT);
      break;


    // level 2, child of "table of database"
    case OT_STATIC_INTEGRITY:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_INTEGRITY);
      break;
    case OT_STATIC_RULE:
    case OT_STATIC_R_PROC_RULE:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_RULE);
      break;
    case OT_STATIC_INDEX:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_INDEX);
      break;
    case OT_STATIC_SECURITY:
    case OT_STATIC_INSTALL_SECURITY         :
    case OT_STATIC_SEC_SEL_SUCC:
    case OT_STATIC_SEC_SEL_FAIL:
    case OT_STATIC_SEC_DEL_SUCC:
    case OT_STATIC_SEC_DEL_FAIL:
    case OT_STATIC_SEC_INS_SUCC:
    case OT_STATIC_SEC_INS_FAIL:
    case OT_STATIC_SEC_UPD_SUCC:
    case OT_STATIC_SEC_UPD_FAIL:
    case OT_STATIC_R_SECURITY:
    case OT_STATIC_R_SEC_SEL_SUCC:
    case OT_STATIC_R_SEC_SEL_FAIL:
    case OT_STATIC_R_SEC_DEL_SUCC:
    case OT_STATIC_R_SEC_DEL_FAIL:
    case OT_STATIC_R_SEC_INS_SUCC:
    case OT_STATIC_R_SEC_INS_FAIL:
    case OT_STATIC_R_SEC_UPD_SUCC:
    case OT_STATIC_R_SEC_UPD_FAIL:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_SECURITY);
      break;

    // level 2, child of "view of database"
    case OT_STATIC_VIEWTABLE:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_VIEWTABLE);
      break;

    // replicator, all levels mixed
    case OT_STATIC_REPLICATOR:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_REPLICATOR);
      break;

    case OT_STATIC_REPLIC_CONNECTION:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_REPLIC_CONNECTION);
      break;
    case OT_STATIC_REPLIC_CDDS:
    case OT_STATIC_R_REPLIC_TABLE_CDDS:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_REPLIC_CDDS);
      break;
    case OT_STATIC_REPLIC_MAILUSER:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_REPLIC_MAILUSER);
      break;
    case OT_STATIC_REPLIC_CDDS_DETAIL:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_REPLIC_CDDS_DETAIL);
      break;

    // new style alarms (with 2 sub-branches alarmee and launched dbevent)
    case OT_STATIC_ALARMEE:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_ALARMEE);
      break;

    //
    // ICE
    //

    case OT_STATIC_ICE                      :
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_ICE);
      break;
    // Under "Security"
    case OT_STATIC_ICE_SECURITY             :
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_ICE_SECURITY);
      break;
    case OT_STATIC_ICE_ROLE               :
    case OT_STATIC_ICE_WEBUSER_ROLE         :
    case OT_STATIC_ICE_PROFILE_ROLE         :
    case OT_STATIC_ICE_BUNIT_SEC_ROLE       :
    case OT_STATIC_ICE_BUNIT_FACET_ROLE     :
    case OT_STATIC_ICE_BUNIT_PAGE_ROLE     :
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_ICE_ROLE);
      break;
    case OT_STATIC_ICE_DBUSER               :
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_ICE_DBUSER);
      break;
    case OT_STATIC_ICE_DBCONNECTION         :
    case OT_STATIC_ICE_WEBUSER_CONNECTION   :
    case OT_STATIC_ICE_PROFILE_CONNECTION   :
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_ICE_DBCONNECTION);
      break;
    case OT_STATIC_ICE_WEBUSER              :
    case OT_STATIC_ICE_BUNIT_SEC_USER       :
    case OT_STATIC_ICE_BUNIT_FACET_USER     :
    case OT_STATIC_ICE_BUNIT_PAGE_USER     :
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_ICE_WEBUSER);
      break;
    case OT_STATIC_ICE_PROFILE              :
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_ICE_PROFILE);
      break;
    case OT_STATIC_ICE_BUNIT                :
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_ICE_BUNIT);
      break;
    case OT_STATIC_ICE_BUNIT_FACET          :
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_ICE_BUNIT_FACET);
      break;
    case OT_STATIC_ICE_BUNIT_PAGE           :
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_ICE_BUNIT_PAGE);
      break;
    case OT_STATIC_ICE_BUNIT_LOCATION       :
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_ICE_BUNIT_LOCATION);
      break;
    // Under "Server"
    case OT_STATIC_ICE_SERVER               :
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_ICE_SERVER);
      break;
    case OT_STATIC_ICE_SERVER_APPLICATION   :
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_ICE_SERVER_APPLICATION);
      break;
    case OT_STATIC_ICE_SERVER_LOCATION      :
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_ICE_SERVER_LOCATION);
      break;
    case OT_STATIC_ICE_SERVER_VARIABLE      :
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_ICE_SERVER_VARIABLE);
      break;

    case OT_STATIC_SEQUENCE:
    case OT_STATIC_R_SEQGRANT_NEXT:
    case OT_STATIC_R_SEQGRANT:
      hIcon1 = IconCacheLoadIcon(IDI_TREE_STATIC_SEQUENCE);
      break;
    default:
      ASSERT(FALSE);
      hIcon1 = 0;
      break;
  }

  return hIcon1;
}

static void CALLBACK DisplayChooseIndexItem(void* pItemData, CuListCtrlCheckBox* pListCtrl, int nItem)
{
	CaObjectChildofTable* pObject = (CaObjectChildofTable*)pItemData;
	CString strItem;
	strItem.Format (_T("%s.%s"), (LPCTSTR)pObject->GetBaseTableOwner(), (LPCTSTR)pObject->GetBaseTable());
	pListCtrl->SetItemText (nItem, 0, pObject->GetName());
	pListCtrl->SetItemText (nItem, 1, pObject->GetOwner());
	pListCtrl->SetItemText (nItem, 2, strItem);
}


extern "C" int VDBA_ChooseIndex(HWND hwndParent, LPCTSTR lpszDatabase, LPCHECKEDOBJECTS* lpListObjects)
{
	const int nHeader = 3;
	CString strText;
	int hdl = -1, ires = -1, nNodeHandle = -1;
	int  i, nSize = 0;
	CTypedPtrList< CObList, CaObjectChildofTable* > listObject;

	try
	{
		nNodeHandle = GetCurMdiNodeHandle();
		CString strVNodeName = (LPCTSTR)GetVirtNodeName(nNodeHandle);
		strText.Format (_T("%s::%s"), (LPCTSTR)strVNodeName, lpszDatabase);
		ires = Getsession ((LPUCHAR)(LPCTSTR)strText, SESSION_TYPE_CACHENOREADLOCK, &hdl);
		INGRESII_llQueryIndex (listObject);

		LSCTRLHEADERPARAMS lsp2[nHeader] =
		{
			{"Index Name",    150,  LVCFMT_LEFT, FALSE},
			{"Index Owner",    80,  LVCFMT_LEFT, FALSE},
			{"Base Table",    200,  LVCFMT_LEFT, FALSE}
		};

		UINT nArrayIDS[nHeader] = {IDS_CHOOSEINDEX_NAME, IDS_CHOOSEINDEX_OWNER, IDS_CHOOSEINDEX_BASETABLE};
		for (i=0; i<nHeader; i++)
		{
			strText.LoadString(nArrayIDS[i]);
			lstrcpy (lsp2[i].m_tchszHeader, strText);
		}
		strText.LoadString (IDS_CHOOSEINDEX_TITLE);

		CxChooseObjects dlg;
		dlg.SetTitle (strText);
		dlg.InitializeHeader (lsp2, nHeader);
		dlg.SetDisplayFunction(DisplayChooseIndexItem);
		dlg.SetHelpID(HELPID_CHOOSEOBJECT_INDEX);

		CPtrArray& arrayObj = dlg.GetArrayObjects();
		CPtrArray& arraySelectedObj = dlg.GetArraySelectedObjects();

		//
		// List of indexes:
		POSITION pos = listObject.GetHeadPosition();
		while (pos != NULL)
		{
			CaObjectChildofTable* pObject = listObject.GetNext(pos);
			arrayObj.Add(pObject);
		}


		//
		// List of pre-selected indexes:
		LPCHECKEDOBJECTS lpPreselectedIndexes = *lpListObjects;
		if (lpPreselectedIndexes)
		{
			//
			// We know that index names are unique in a database !
			while (lpPreselectedIndexes)
			{
				pos = listObject.GetHeadPosition();
				while (pos != NULL)
				{
					CaObjectChildofTable* pObject = listObject.GetNext(pos);
					if (pObject->GetName().CompareNoCase((LPTSTR)lpPreselectedIndexes->dbname) == 0)
					{
						arraySelectedObj.Add(pObject);
					}
				}
				lpPreselectedIndexes = lpPreselectedIndexes->pnext;
			}
		}

		//
		// Display the Dialog:
		dlg.DoModal();

		//
		// Construct the new list of selected objects:
		lpPreselectedIndexes = *lpListObjects;
		lpPreselectedIndexes = FreeCheckedObjects (lpPreselectedIndexes);
		LPCHECKEDOBJECTS pObj = NULL;
		nSize = arraySelectedObj.GetSize();
		for (i=0; i<nSize; i++)
		{
			CaObjectChildofTable* pObject = (CaObjectChildofTable*)arraySelectedObj.GetAt(i);

			pObj = (CHECKEDOBJECTS*)ESL_AllocMem (sizeof (CHECKEDOBJECTS));
			if (!pObj)
			{
				lpPreselectedIndexes = FreeCheckedObjects (lpPreselectedIndexes);
				VDBA_OutOfMemoryMessage();
				throw (int)1;
			}
			else
			{
				lstrcpy ((LPTSTR)pObj->dbname, QuoteIfNeeded((LPCTSTR)pObject->GetName()));
				pObj->bchecked = TRUE;
				lpPreselectedIndexes = AddCheckedObject (lpPreselectedIndexes, pObj);
			}
		}
		*lpListObjects = lpPreselectedIndexes;
	}
	catch(CeSQLException e)
	{
		AfxMessageBox (e.m_strReason, MB_ICONEXCLAMATION|MB_OK);
	}
	catch(...)
	{

	}

	//
	// Cleanup:
	while (!listObject.IsEmpty())
		delete listObject.RemoveHead();

	if (hdl != -1)
		ReleaseSession(hdl, RELEASE_COMMIT);

	return 0;
}


//
// Global function and external "C" Interface called.
extern "C" UINT GetExistOpenCursor (FINDCURSOR* pFindCursor, REQUESTMDIINFO** pArrayInfo, int* iSize)
{
/*UKS
	try
	{
		CPtrArray arrInfo;
		UINT nExist = 0;
		CDocument* pDoc;
		POSITION pos, curTemplatePos = theApp.GetFirstDocTemplatePosition();
		while(curTemplatePos != NULL)
		{
			CDocTemplate* curTemplate = theApp.GetNextDocTemplate (curTemplatePos);
			pos = curTemplate->GetFirstDocPosition ();
			while (pos)
			{
				pDoc = curTemplate->GetNextDoc (pos);
				if(pDoc->IsKindOf (RUNTIME_CLASS (CMainMfcDoc)) || pDoc->IsKindOf (RUNTIME_CLASS (CdSqlQueryRichEditDoc)))
				{
					POSITION p = pDoc->GetFirstViewPosition();
					while (p != NULL)
					{
						CView* pView = pDoc->GetNextView(p);
						if (pView && pView->IsKindOf(RUNTIME_CLASS (CvSqlQueryLowerView)))
						{
							CvSqlQueryLowerView* pSqlQueryLowerView = (CvSqlQueryLowerView*)pView;
							CuDlgSqlQueryResult* pDlgResult = pSqlQueryLowerView->GetDlgSqlQueryResult();
							if (pDlgResult)
							{
								CTabCtrl& tab = pDlgResult->m_cTab1;
								int nCount = tab.GetItemCount();
								//
								// If the cursor exits and it is alive, it must
								// be the one of the last execution (at first tab of index 0):
								TCITEM item;
								memset (&item, 0, sizeof (item));
								item.mask = TCIF_PARAM;
								if (tab.GetItem (0, &item))
								{
									CWnd* pWnd = (CWnd*)item.lParam;
									if (pWnd && IsWindow (pWnd->m_hWnd))
									{
										REQUESTMDIINFO request;
										memset (&request, 0, sizeof(request));
										if ((BOOL)pWnd->SendMessage (WM_QUERY_OPEN_CURSOR, (LPARAM)&request, (LPARAM)pFindCursor))
										{
											nExist |= OPEN_CURSOR_SQLACT;
											REQUESTMDIINFO* pInfo = new REQUESTMDIINFO;
											memcpy (pInfo, &request, sizeof(request));
											arrInfo.Add(pInfo);
										}
									}
								}
							}
						}
						else
						if (pView && pView->IsKindOf(RUNTIME_CLASS (CMainMfcView2)))
						{
							REQUESTMDIINFO request;
							memset (&request, 0, sizeof(request));
							if ((BOOL)pView->SendMessage (WM_QUERY_OPEN_CURSOR, (LPARAM)&request, (LPARAM)pFindCursor))
							{
								nExist |= OPEN_CURSOR_DOM;
								REQUESTMDIINFO* pInfo = new REQUESTMDIINFO;
								memcpy (pInfo, &request, sizeof(request));
								arrInfo.Add(pInfo);
							}
						}
					}
				}
			}
		}

		if (arrInfo.GetSize() > 0)
		{
			int i, n = arrInfo.GetSize();
			REQUESTMDIINFO* pInfo =  (REQUESTMDIINFO*)malloc (n * sizeof (REQUESTMDIINFO));
			for (i=0; i<n; i++)
			{
				REQUESTMDIINFO* pObj = (REQUESTMDIINFO*)arrInfo.GetAt(i);
				memcpy (&pInfo[i], pObj, sizeof(REQUESTMDIINFO));
				delete pObj;
			}
			arrInfo.RemoveAll();
			*iSize = n;
			*pArrayInfo = pInfo;
		}
		else
		{
			*iSize = 0;
			*pArrayInfo = NULL;
		}
		return nExist;
	}
	catch (...)
	{

	}
*/
	return 0;
}

extern "C" BOOL IsReadLockActivated()
{
/*UKS
	CaSqlQuerySetting* pSetting = theApp.GetSqlActSetting();
	if (pSetting && pSetting->m_bReadlock)
		return TRUE;
*/
	return FALSE;
}

extern "C" UINT GetUserMessageQueryOpenCursor() {return WM_QUERY_OPEN_CURSOR;}



static BOOL GetInterfacePointer(IUnknown* pObj, REFIID riid, PVOID* ppv)
{
	BOOL bResult = FALSE;
	HRESULT hError;

	*ppv=NULL;

	if (NULL != pObj)
	{
		hError = pObj->QueryInterface(riid, ppv);
	}
	return FAILED(hError)? FALSE: TRUE;
}

CString SqlWizard(HWND hParent, SQLASSISTANTSTRUCT* piiparam)
{
	USES_CONVERSION;
	CString strMsg;
	IUnknown*   pAptSqlAssistant = NULL;
	ISqlAssistant* pSqlAssistant;
	HRESULT hError = NOERROR;
	CString strStatement = _T("");
	CWaitCursor doWaitCursor;

	try
	{
		hError = CoCreateInstance(
			CLSID_SQLxASSISTANCT,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IUnknown,
			(PVOID*)&pAptSqlAssistant);

		if (SUCCEEDED(hError))
		{
			BOOL bOk = GetInterfacePointer(pAptSqlAssistant, IID_ISqlAssistant, (LPVOID*)&pSqlAssistant);
			if (bOk)
			{
				BSTR bstrStatement = NULL;
				hError = pSqlAssistant->SqlAssistant (hParent, piiparam, &bstrStatement);
				if (SUCCEEDED(hError) && bstrStatement)
				{
					strStatement = W2T(bstrStatement);
					SysFreeString(bstrStatement);
					strStatement.TrimLeft();
					strStatement.TrimRight();
				}
				pSqlAssistant->Release();
			}
			else
			{
				//
				// Failed to query interface of SQL Assistant
				AfxMessageBox (IDS_MSG_FAIL_2_QUERYINTERFACE_SQLASSISTANT);
			}
			
			pAptSqlAssistant->Release();
			CoFreeUnusedLibraries();
		}
		else
		{
			//
			// Failed to create an SQL Assistant COM Object
			AfxMessageBox (IDS_MSG_FAIL_2_CREATE_SQLASSISTANT);
		}
	}
	catch(...)
	{

	}
	return strStatement;
}

extern "C" LPTSTR CreateSelectStmWizard(HWND hwndOwner, LPUCHAR lpVirtNodeName, LPUCHAR lpDatabaseName)
{
	USES_CONVERSION;
	SQLASSISTANTSTRUCT iiparam;
	memset(&iiparam, 0, sizeof(iiparam));

	UCHAR ucVirtNodeName[MAXOBJECTNAME];
	UCHAR ucBuffer[MAXOBJECTNAME];
    
	lstrcpy((char *)ucVirtNodeName, (char *)lpVirtNodeName);
	
	ucBuffer[0] = '\0';
	if (GetGWClassNameFromString(ucVirtNodeName, ucBuffer))
	{
		if (ucBuffer[0])
			wcscpy (iiparam.wczServerClass, T2W((LPTSTR)ucBuffer));
		RemoveGWNameFromString(ucVirtNodeName);
	}

	ucBuffer[0] = '\0';
	if(GetConnectUserFromString(ucVirtNodeName, ucBuffer))
	{
		if (ucBuffer[0])
			wcscpy (iiparam.wczUser, T2W((LPTSTR)ucBuffer));
		RemoveConnectUserFromString(ucVirtNodeName);
	}

	wcscpy (iiparam.wczNode, T2W((LPTSTR)ucVirtNodeName));
	wcscpy (iiparam.wczDatabase, T2W((LPTSTR)lpDatabaseName));
	wcscpy (iiparam.wczSessionDescription, L"Ingres Visual DBA invokes SQL Assistant");

	if (IsStarDatabase(GetCurMdiNodeHandle(), lpDatabaseName))
		iiparam.nDbFlag = DBFLAG_STARNATIVE;
	iiparam.nSessionStart = 500;
	iiparam.nActivation = 1; // Wizard for select statement
	CString strStatement = SqlWizard(hwndOwner, &iiparam);
	if (strStatement.IsEmpty())
		return NULL;
	LPTSTR lpszStatment = (LPTSTR)malloc (strStatement.GetLength() +1);
	if (!lpszStatment)
		return NULL;
	lstrcpy (lpszStatment, (LPCTSTR)strStatement);
	//
	// Caller must free this allocated string:
	return lpszStatment;
}

extern "C" LPTSTR CreateSQLWizard (HWND hwndOwner, LPUCHAR lpVirtNodeName, LPUCHAR lpDatabaseName)
{
	USES_CONVERSION;
	SQLASSISTANTSTRUCT iiparam;
	memset(&iiparam, 0, sizeof(iiparam));
	
	UCHAR ucVirtNodeName[MAXOBJECTNAME];
	UCHAR ucBuffer[MAXOBJECTNAME];
    
	lstrcpy((char *)ucVirtNodeName, (char *)lpVirtNodeName);
	
	ucBuffer[0] = '\0';
	if (GetGWClassNameFromString(ucVirtNodeName, ucBuffer))
	{
		if (ucBuffer[0])
			wcscpy (iiparam.wczServerClass, T2W((LPTSTR)ucBuffer));
		RemoveGWNameFromString(ucVirtNodeName);
	}

	ucBuffer[0] = '\0';
	if(GetConnectUserFromString(ucVirtNodeName, ucBuffer))
	{
		if (ucBuffer[0])
			wcscpy (iiparam.wczUser, T2W((LPTSTR)ucBuffer));
		RemoveConnectUserFromString(ucVirtNodeName);
	}

	wcscpy (iiparam.wczNode, T2W((LPTSTR)ucVirtNodeName));
	wcscpy (iiparam.wczDatabase, T2W((LPTSTR)lpDatabaseName));
	wcscpy (iiparam.wczSessionDescription, L"Ingres Visual DBA invokes SQL Assistant");
	if (IsStarDatabase(GetCurMdiNodeHandle(), lpDatabaseName))
		iiparam.nDbFlag = DBFLAG_STARNATIVE;
	iiparam.nSessionStart = 500;
	iiparam.nActivation = 0; // Wizard general

	CString strStatement = SqlWizard(hwndOwner, &iiparam);
	if (strStatement.IsEmpty())
		return NULL;
	LPTSTR lpszStatment = (LPTSTR)malloc (strStatement.GetLength() +1);
	if (!lpszStatment)
		return NULL;
	lstrcpy (lpszStatment, (LPCTSTR)strStatement);
	//
	// Caller must free this allocated string:
	return lpszStatment;
}

extern "C" LPTSTR CreateSearchCondWizard(HWND hwndOwner, LPUCHAR lpVirtNodeName, LPUCHAR lpDatabaseName, TOBJECTLIST* pTableList, TOBJECTLIST* pListCol)
{
	USES_CONVERSION;
	SQLASSISTANTSTRUCT iiparam;
	memset(&iiparam, 0, sizeof(iiparam));
	
	UCHAR ucVirtNodeName[MAXOBJECTNAME];
	UCHAR ucBuffer[MAXOBJECTNAME];
    
	lstrcpy((char *)ucVirtNodeName, (char *)lpVirtNodeName);
	
	ucBuffer[0] = '\0';
	if (GetGWClassNameFromString(ucVirtNodeName, ucBuffer))
	{
		if (ucBuffer[0])
			wcscpy (iiparam.wczServerClass, T2W((LPTSTR)ucBuffer));
		RemoveGWNameFromString(ucVirtNodeName);
	}

	ucBuffer[0] = '\0';
	if(GetConnectUserFromString(ucVirtNodeName, ucBuffer))
	{
		if (ucBuffer[0])
			wcscpy (iiparam.wczUser, T2W((LPTSTR)ucBuffer));
		RemoveConnectUserFromString(ucVirtNodeName);
	}

	wcscpy (iiparam.wczNode, T2W((LPTSTR)ucVirtNodeName));
	wcscpy (iiparam.wczDatabase, T2W((LPTSTR)lpDatabaseName));
	wcscpy (iiparam.wczSessionDescription, L"Ingres Visual DBA invokes SQL Assistant");
	if (IsStarDatabase(GetCurMdiNodeHandle(), lpDatabaseName))
		iiparam.nDbFlag = DBFLAG_STARNATIVE;
	iiparam.pListTables = pTableList;
	iiparam.pListColumns = pListCol;

	iiparam.nSessionStart = 500;
	iiparam.nActivation = 2; // Wizard for search condition

	CString strStatement = SqlWizard(hwndOwner, &iiparam);
	if (strStatement.IsEmpty())
		return NULL;
	LPTSTR lpszStatment = (LPTSTR)malloc (strStatement.GetLength() +1);
	if (!lpszStatment)
		return NULL;
	lstrcpy (lpszStatment, (LPCTSTR)strStatement);
	//
	// Caller must free this allocated string:
	return lpszStatment;
}

