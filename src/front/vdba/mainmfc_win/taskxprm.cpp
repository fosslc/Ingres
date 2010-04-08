/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : taskxprm.cpp, Implementation file
**
**  Project  : Ingres II/ VDBA.
**
**  Author   : UK Sotheavut
**
**    Purpose  : Allow interrupt task
**               (Parameters and execute task, ie DBAAddObject (...)
**
**  History :
**  03-Feb-2000 (uk$so01)
**   (bug #100324) Replace the named Mutex by the unnamed Mutex.
**  21-Dec-2000 (noifr01)
**   (SIR 103548) use the new prototype of function GetGWlistLL() (pass FALSE
**   for getting the whole list of gateways)
** 05-Jul-2001 (uk$so01)
**    SIR #105199. Integrate & Generate XML.
** 19-Feb-2002 (uk$so01)
**    SIR #106648, Integrate SQL Test ActiveX Control
**  26-Mar-2002 (noifr01)
**   (bug 107442) removed the "force refresh" dialog box in DOM windows: force
**    refresh now refreshes all data
** 27-May-2002 (uk$so01)
**    BUG #107880, Add the XML decoration encoding='UTF-8' or 'WINDOWS-1252'
**    ... depending on the Ingres II_CHARSETxx.
**      19-Jun-2002 (hanje04)
**          Define tchszReturn to be 0x0a 0x00 for MAINWIN builds to stop
**          unwanted ^M's in generated text files.
**          Cast pCol->GetName with (LPCTSTR) to give it correct type.
**	    Changed order of filename creation and deletion in 
**	    XML_GetTempFileName because funtion was returning the same 
**	    filenames to CaExecParamGenerateXMLfromSelect::Run on UNIX,
**	    it to go into an infinite loop further on. (for uk$so01)
*/

#include "stdafx.h"
#include "taskxprm.h"

extern "C"
{
#include "domdata.h"
#include "dbaginfo.h"
#include "msghandl.h"
#include "monitor.h"
}
#include "extccall.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_SERIAL (CaExecParam, CObject, 1)

void CaExecParam::Copy(const CaExecParam& c)
{
	m_bExecuteImmediately = c.m_bExecuteImmediately;
	m_bInterrupted = c.m_bInterrupted;
	m_nInterruptType = c.m_nInterruptType;
}

void CaExecParam::Serialize(CArchive& ar)
{
	try
	{
		if (ar.IsStoring())
		{
			ar << m_bExecuteImmediately;
			ar << m_bInterrupted;
			ar << m_nInterruptType;
		}
		else
		{
			ar >> m_bExecuteImmediately;
			ar >> m_bInterrupted;
			ar >> m_nInterruptType;
		}
	}
	catch (CMemoryException* e)
	{
		VDBA_OutOfMemoryMessage();
		e->Delete();
	}
	catch (CArchiveException* e)
	{
		VDBA_ArchiveExceptionMessage (e);
		e->Delete();
	}
	catch (...)
	{
		CString strMsg;
		strMsg.LoadString(IDS_E_UNKNOWN_ERROR);
		AfxMessageBox (strMsg);//"Unknown error occured"
	}
}

void CaExecParam::Init()
{
	m_hMutex = CreateMutex (NULL, FALSE, NULL);
	if (m_hMutex == NULL)
	{
		//_T("Cannot create Mutex");
		AfxMessageBox (VDBA_MfcResourceString(IDS_E_MUTEX));
	}
	m_bInterrupted = FALSE;
	m_bExecuteImmediately = FALSE;
	//
	// DlgWait is running !!
	theApp.SetDlgWaitState(TRUE);
}


CaExecParam::CaExecParam()
{
	m_nInterruptType = INTERRUPT_NOT_ALLOWED;
	Init();
}

CaExecParam::CaExecParam(UINT nInteruptType)
{
	m_nInterruptType = nInteruptType;
	Init();
}


BOOL CaExecParam::Lock()
{
	DWORD dwWaitResult;
	dwWaitResult = WaitForSingleObject (m_hMutex, INFINITE);
	switch (dwWaitResult)
	{
	case WAIT_OBJECT_0:
		return TRUE;
		break;
	default:
		break;
	}
	return FALSE;
}

BOOL CaExecParam::Unlock()
{
	ReleaseMutex (m_hMutex);
	return FALSE;
}



//
// DBAAddObject (...)
CaExecParamAddObject::CaExecParamAddObject(): CaExecParam(INTERRUPT_QUERY)
{
	m_strVNode = _T("");
	m_lParam   = NULL;
	m_iObjType =-1;
	m_nhSession=-1;
}

void CaExecParamAddObject::SetExecParam (LPCTSTR lpszVNode, int iObjType, LPVOID lpParam, int nhSession)
{
	m_strVNode = lpszVNode? lpszVNode: _T("");
	m_lParam   = lpParam;
	m_iObjType = iObjType;
	m_nhSession= nhSession;
}


int CaExecParamAddObject::Run(HWND hWndTaskDlg)
{
	if (m_strVNode.IsEmpty())
	{
		//
		// Ignore virtual node name:
		ASSERT (m_nhSession != -1);
		if (m_nhSession != -1)
			return DBAAddObjectLL (NULL, m_iObjType, m_lParam, m_nhSession);
	}
	else
	{
		//
		// Ignore session handle:
		return DBAAddObjectLL ((LPUCHAR)(LPCTSTR)m_strVNode, m_iObjType, m_lParam);
	}
	return RES_ERR;
}


//
// DBAAlterObject (...)
CaExecParamAlterObject::CaExecParamAlterObject(): CaExecParam(INTERRUPT_QUERY)
{
	m_loriginParam = NULL;
	m_lnewParam    = NULL;
	m_iObjType     = -1;
	m_nhSession    = -1;
}

void CaExecParamAlterObject::SetExecParam (LPVOID lporiginParam, LPVOID lpnewParam, int iObjType, int nhSession)
{
	m_loriginParam = lporiginParam;
	m_lnewParam    = lpnewParam;
	m_iObjType     = iObjType;
	m_nhSession    = nhSession;
}

int CaExecParamAlterObject::Run(HWND hWndTaskDlg)
{
	return DBAAlterObject (m_loriginParam, m_lnewParam, m_iObjType, m_nhSession);
}


//
// DBADropObject (...)
CaExecParamDropObject::CaExecParamDropObject(): CaExecParamAddObject()
{

}

int CaExecParamDropObject::Run(HWND hWndTaskDlg)
{
	if (m_strVNode.IsEmpty())
	{
		//
		// Ignore virtual node name:
		ASSERT (m_nhSession != -1);
		if (m_nhSession != -1)
			return DBADropObject (NULL, m_iObjType, m_lParam, m_nhSession);
	}
	else
	{
		//
		// Ignore session handle:
		return DBADropObject ((LPUCHAR)(LPCTSTR)m_strVNode, m_iObjType, m_lParam);
	}
	return RES_ERR;
}


//
// GetDetailInfo:
CaExecParamGetDetailInfo::CaExecParamGetDetailInfo(): CaExecParam(INTERRUPT_QUERY)
{
	m_lpNode   = NULL;
	m_lParam   = NULL;
	m_iObjType =-1;
	m_bRetainSessForLock = FALSE;
	m_pLocSession = NULL;
}

void CaExecParamGetDetailInfo::SetExecParam (void * lpNode, int iObjType, LPVOID lpParam, BOOL bRetainSessForLock, int* ilocsession)
{
	m_lpNode = (void *)lpNode;
	m_lParam   = lpParam;
	m_iObjType = iObjType;
	m_bRetainSessForLock = bRetainSessForLock;
	m_pLocSession = ilocsession;
}

int CaExecParamGetDetailInfo::Run(HWND hWndTaskDlg)
{
	return GetDetailInfoLL ((LPUCHAR)m_lpNode, m_iObjType, m_lParam, m_bRetainSessForLock, m_pLocSession);
}


//
// MonGetDetailInfo:
CaExecParamMonGetDetailInfo::CaExecParamMonGetDetailInfo(): CaExecParam(INTERRUPT_QUERY)
{
	m_nHNode = -1;
	m_lpBigStruct = NULL;
	m_nOldType = -1;
	m_nNewType = -1;
}

void CaExecParamMonGetDetailInfo::SetExecParam (int nHNode, LPVOID lpBigStruct, int nOldType, int nNewType)
{
	m_nHNode = nHNode;
	m_lpBigStruct = lpBigStruct;
	m_nOldType = nOldType;
	m_nNewType = nNewType;
}

int CaExecParamMonGetDetailInfo::Run(HWND hWndTaskDlg)
{
	return MonGetDetailInfoLL (m_nHNode, m_lpBigStruct, m_nOldType, m_nNewType);
}



//
// DOMGetFirstObject:

CaExecParamDOMGetFirstObject::CaExecParamDOMGetFirstObject(): CaExecParam(INTERRUPT_QUERY)
{
	m_nHNode = -1;
	m_nOIype = -1;
	m_nLevel = -1;
	m_lpParentString = NULL;
	m_bSystemObject = FALSE;
	m_tchszFilterOwner [0] = _T('\0');
	m_tchszResultObject[0] = _T('\0');
	m_tchszResultOwner [0] = _T('\0');
	m_tchszResultExtra [0] = _T('\0');
}


void CaExecParamDOMGetFirstObject::SetExecParam (int nHNode, int nOIType, int nLevel, LPVOID lpParentString, BOOL bSystem)
{
	m_nHNode = nHNode;
	m_nOIype = nOIType;
	m_nLevel = nLevel;
	m_lpParentString = lpParentString;
	m_bSystemObject  = bSystem;
}

int CaExecParamDOMGetFirstObject::Run(HWND hWndTaskDlg)
{
	int nRes = DOMGetFirstObject(
		m_nHNode,
		m_nOIype,
		m_nLevel,
		(LPUCHAR*)m_lpParentString,
		m_bSystemObject,
		(LPUCHAR)m_tchszFilterOwner,
		(LPUCHAR)m_tchszResultObject,
		(LPUCHAR)m_tchszResultOwner,
		(LPUCHAR)m_tchszResultExtra);
	return nRes;
}

//
// Get List of nodes
CaExecParamGetNodesList::CaExecParamGetNodesList():CaExecParam()
{
}

int CaExecParamGetNodesList::Run(HWND hWndTaskDlg)
{
	int nres=NodeLLFillNodeListsLL();
	return nres;
}


//
// Background refresh
CaExecParamBkRefresh::CaExecParamBkRefresh():CaExecParam(INTERRUPT_ALL_TYPE)
{
		// background refresh, when invoked "with popup window"
		// should show the window immediatly
		m_bExecuteImmediately = TRUE;
}
void CaExecParamBkRefresh::SetExecParam (time_t refreshtime)
{
	m_refreshtime = refreshtime;
}

int CaExecParamBkRefresh::Run(HWND hWndTaskDlg)
{
	int nres = DOMBkRefresh(m_refreshtime,TRUE,hWndTaskDlg);
	return nres;
}

//
// Get List of gateways on a node
CaExecParamGetGWList::CaExecParamGetGWList():CaExecParam(INTERRUPT_ALL_TYPE)
{
//		m_bExecuteImmediately = TRUE;
}
void CaExecParamGetGWList::SetExecParam (TCHAR * host)
{
	m_lphost = host;
}

int CaExecParamGetGWList::Run(HWND hWndTaskDlg)
{
	m_presult = (TCHAR **) GetGWlistLL((LPUCHAR) m_lphost, FALSE);
	return 0; // result is LPUCHAR, stored in memeber variable (pointer on a static array of (static) pointers)
}

//
// DBAGetFirstObject:

CaExecParamDBAGetFirstObject::CaExecParamDBAGetFirstObject(): CaExecParam(INTERRUPT_ALL_TYPE)
{
	m_lpNode    = (LPUCHAR) NULL;
	m_iobjtype  = -1;
	m_nLevel    = -1;
	m_parents   = NULL;
	m_bSystem   = FALSE;
	m_lpobjname = NULL;
	m_lpowner   = NULL;
	m_lpextra   = NULL;
}


void CaExecParamDBAGetFirstObject::SetExecParam (void * lpNode,int iobjtype,int nLevel, void * parents, BOOL bSystem, void * lpobjname,void * lpowner,void * lpextra)
{
	m_lpNode    = lpNode;
	m_iobjtype  = iobjtype;
	m_nLevel    = nLevel;
	m_parents   = parents;
	m_bSystem   = bSystem;
	m_lpobjname = lpobjname;
	m_lpowner   = lpowner;
	m_lpextra   = lpextra;
}

int CaExecParamDBAGetFirstObject::Run(HWND hWndTaskDlg)
{
	int nRes = DBAGetFirstObjectLL((LPUCHAR)m_lpNode,
		m_iobjtype,
		m_nLevel,
		(LPUCHAR *)m_parents,
		m_bSystem,
		(LPUCHAR)m_lpobjname,
		(LPUCHAR)m_lpowner,
		(LPUCHAR)m_lpextra);
	return nRes;
}

//
// Expand Branch
CaExecParamExpandBranch::CaExecParamExpandBranch():CaExecParam(INTERRUPT_ALL_TYPE)
{
  // box must display immediately
	m_bExecuteImmediately = TRUE;

  m_hwndMdi   = 0;
  m_lpDomData = NULL;
  m_dwItem    = 0L;
}

void CaExecParamExpandBranch::SetExecParam (HWND hwndMdi, LPDOMDATA lpDomData, DWORD dwItem)
{
  ASSERT (hwndMdi);
  ASSERT (lpDomData);
  m_hwndMdi   = hwndMdi;
  m_lpDomData = lpDomData;
  m_dwItem    = dwItem;
}

int CaExecParamExpandBranch::Run(HWND hWndTaskDlg)
{
  ASSERT (m_hwndMdi);
  ASSERT (m_lpDomData);
	int iret = ManageExpandBranch_MT(m_hwndMdi, m_lpDomData, m_dwItem, hWndTaskDlg);
	return iret;
}

//
// Expand All
CaExecParamExpandAll::CaExecParamExpandAll():CaExecParam(INTERRUPT_ALL_TYPE)
{
  // box must display immediately
	m_bExecuteImmediately = TRUE;

  m_hwndMdi   = 0;
  m_lpDomData = NULL;
}

void CaExecParamExpandAll::SetExecParam (HWND hwndMdi, LPDOMDATA lpDomData)
{
  ASSERT (hwndMdi);
  ASSERT (lpDomData);
  m_hwndMdi   = hwndMdi;
  m_lpDomData = lpDomData;
}

int CaExecParamExpandAll::Run(HWND hWndTaskDlg)
{
  ASSERT (m_hwndMdi);
  ASSERT (m_lpDomData);
	int iret = ManageExpandAll_MT(m_hwndMdi, m_lpDomData, hWndTaskDlg);
	return iret;
}

//
// Drag Drop End
CaExecParamDragDropEnd::CaExecParamDragDropEnd():CaExecParam(INTERRUPT_ALL_TYPE)
{
  // box must display immediately
	m_bExecuteImmediately = TRUE;

  m_hwndMdi           = 0;
  m_lpDomData         = NULL;
  m_bRightPane        = FALSE;
  m_lpRightPaneRecord = NULL;
  m_hwndDomDest       = 0;
  m_lpDestDomData     = NULL;
}

void CaExecParamDragDropEnd::SetExecParam (HWND hwndMdi, LPDOMDATA lpDomData, BOOL bRightPane, LPTREERECORD lpRightPaneRecord, HWND hwndDomDest, LPDOMDATA lpDestDomData)
{
  ASSERT (hwndMdi);
  ASSERT (lpDomData);

  m_hwndMdi           = hwndMdi;
  m_lpDomData         = lpDomData;
  m_bRightPane        = bRightPane;
  m_lpRightPaneRecord = lpRightPaneRecord;
  m_hwndDomDest       = hwndDomDest;
  m_lpDestDomData     = lpDestDomData;
}

int CaExecParamDragDropEnd::Run(HWND hWndTaskDlg)
{
  ASSERT (m_hwndMdi);
  ASSERT (m_lpDomData);
	int iret = StandardDragDropEnd_MT(m_hwndMdi, m_lpDomData, m_bRightPane, m_lpRightPaneRecord, m_hwndDomDest, m_lpDestDomData);
  return iret;
}

//
// Refresh branch and it's subbranches
CaExecParamForceRefreshCurSub::CaExecParamForceRefreshCurSub():CaExecParam(INTERRUPT_ALL_TYPE)
{
  // box must display immediately
	m_bExecuteImmediately = TRUE;

  m_hwndMdi   = 0;
  m_lpDomData = NULL;
  m_dwItem    = 0L;
}

void CaExecParamForceRefreshCurSub::SetExecParam (HWND hwndMdi, LPDOMDATA lpDomData, DWORD dwItem)
{
  ASSERT (hwndMdi);
  ASSERT (lpDomData);
  m_hwndMdi   = hwndMdi;
  m_lpDomData = lpDomData;
  m_dwItem    = dwItem;
}

int CaExecParamForceRefreshCurSub::Run(HWND hWndTaskDlg)
{
  ASSERT (m_hwndMdi);
  ASSERT (m_lpDomData);
	int iret = ManageForceRefreshCursub_MT(m_hwndMdi, m_lpDomData, m_dwItem, hWndTaskDlg);
	return iret;
}

//
// Refresh all branches
CaExecParamForceRefreshAll::CaExecParamForceRefreshAll():CaExecParam(INTERRUPT_ALL_TYPE)
{
  // box must display immediately
	m_bExecuteImmediately = TRUE;

  m_hwndMdi   = 0;
  m_lpDomData = NULL;
}

void CaExecParamForceRefreshAll::SetExecParam (HWND hwndMdi, LPDOMDATA lpDomData)
{
  ASSERT (hwndMdi);
  ASSERT (lpDomData);
  m_hwndMdi   = hwndMdi;
  m_lpDomData = lpDomData;
}

int CaExecParamForceRefreshAll::Run(HWND hWndTaskDlg)
{
  ASSERT (m_hwndMdi);
  ASSERT (m_lpDomData);
	int iret = ManageForceRefreshAll_MT(m_hwndMdi, m_lpDomData, hWndTaskDlg);
	return iret;
}

