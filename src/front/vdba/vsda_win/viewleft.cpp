/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : viewleft.cpp : implementation file
**    Project  : INGRESII / Visual Schema Diff Control (vsda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : TreeView of the left pane
**
** History:
**
** 26-Aug-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
** 22-Apr-2003 (schph01)
**    SIR 107523 Add Sequence Object
** 06-Jan-2004 (schph01)
**    SIR #111462, Put string into resource files
** 18-Aug-2004 (uk$so01)
**    BUG #112841 / ISSUE #13623161, Allow user to cancel the Comparison
**    operation.
** 29-Sep-2004 (uk$so01)
**    BUG #113119, Add readlock mode in the session management.
** 13-Oct-2004 (uk$so01)
**    BUG #113226 / ISSUE 13720088: The DBMS connections should be disconnected
**    after the comparison has been done.
** 21-Oct-2004 (uk$so01)
**    BUG #113280 / ISSUE 13742473 (VDDA should minimize the number of DBMS connections)
**    Add _CHECK_NUMSESSION_IN_CACHE for testing the number of opened sessions.
** 05-Jul-2005 (komve01)
**    BUG #114789 / ISSUE 14239791
**    Split the case of OBT_DATABASE in VSD_RunVdba, to get the database names
**    for each database that is compared.
*/

#include "stdafx.h"
#include "vsda.h"
#include "vsdadoc.h"
#include "viewleft.h"
#include "vsdafrm.h"
#include "rctools.h"
#include "vsddml.h"
#include "loadsave.h"
#include "taskanim.h"
#include "usrexcep.h"
#include "ingobdml.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define IM_WIDTH  32
#define IM_HEIGHT 16
//#define _CHECK_NUMSESSION_IN_CACHE


static int CALLBACK fnCompareTreeItem(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	CtrfItemData* pComponent1 = (CtrfItemData*)lParam1;
	CtrfItemData* pComponent2 = (CtrfItemData*)lParam2;
	if (pComponent1 == NULL || pComponent2 == NULL)
		return 0;
	return (pComponent1->GetTreeItemType() > pComponent2->GetTreeItemType())? 1: -1;
}


static void SortTreeItem(CTreeCtrl& cTree, HTREEITEM hItem)
{
	TVSORTCB sortcb;
	while (hItem)
	{
		if (cTree.ItemHasChildren(hItem))
		{
			sortcb.hParent = hItem;
			sortcb.lpfnCompare = fnCompareTreeItem;
			sortcb.lParam = NULL;
			cTree.SortChildrenCB (&sortcb);
			HTREEITEM hChild = cTree.GetChildItem(hItem);
			SortTreeItem(cTree, hChild);
		}

		hItem = cTree.GetNextSiblingItem(hItem);
	}
}


static void UnDiscadItem(CtrfItemData* pItem)
{
	CaVsdItemData* pVsdItemData = (CaVsdItemData*)pItem->GetUserData();
	pVsdItemData->SetDiscard(FALSE);

	CTypedPtrList< CObList, CtrfItemData* >& lo = pItem->GetListObject();
	POSITION pos = lo.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfItemData* pIt = lo.GetNext(pos);
		UnDiscadItem (pIt);
	}
}

static BOOL AllowAccess2Vdba(CtrfItemData* pItem)
{
	if (pItem->IsFolder())
		return FALSE;
	int nObjectType = pItem->GetDBObject()->GetObjectID();
	switch (nObjectType)
	{
	case OBT_DATABASE: 
	case OBT_TABLE:
	case OBT_VIEW:
	case OBT_PROCEDURE:
	case OBT_SEQUENCE:
	case OBT_DBEVENT:
	case OBT_INDEX: 
	case OBT_RULE:
	case OBT_USER:
	case OBT_GROUP:
	case OBT_ROLE:
	case OBT_PROFILE:
	case OBT_LOCATION:
		if (pItem->GetUserData() != NULL)
		{
			CaVsdItemData* pVsdItemData = (CaVsdItemData*)pItem->GetUserData();
			TCHAR ch = pVsdItemData->GetDifference();
			if (ch == _T('+') || ch == _T('-'))
				return FALSE;
			else
				return TRUE;
		}
		else
			return FALSE;
		break;
	default:
		return FALSE;
		break;
	}
	return FALSE;
}

static CString GetCmdType(int nObjectType)
{
	CString strType = _T("");
	switch (nObjectType)
	{
	case OBT_USER:
		strType = _T("USER");
		break;
	case OBT_GROUP:
		strType = _T("GROUP");
		break;
	case OBT_ROLE:
		strType = _T("ROLE");
		break;
	case OBT_PROFILE:
		strType = _T("PROFILE");
		break;
	case OBT_LOCATION:
		strType = _T("LOCATION");
		break;
	case OBT_DATABASE:
		strType = _T("DATABASE");
		break;
	case OBT_TABLE:
		strType = _T("TABLE");
		break;
	case OBT_VIEW:
		strType = _T("VIEW");
		break;
	case OBT_PROCEDURE:
		strType = _T("PROCEDURE");
		break;
	case OBT_SEQUENCE:
		strType = _T("SEQUENCE");
		break;
	case OBT_DBEVENT:
		strType = _T("DBEVENT");
		break;
	case OBT_INDEX: 
		strType = _T("INDEX");
		break;
	case OBT_RULE:
		strType = _T("RULE");
		break;
	default:
		break;
	}

	return strType;
}


static UINT StartThreadProc(LPVOID pParam)
{
	LPTSTR lpszCmd = (LPTSTR)pParam;

	BOOL bOk = FALSE;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	if (!lpszCmd)
		return 0;
	// Execute the command with a call to the CreateProcess API call.
	memset(&si,0,sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.wShowWindow = SW_SHOW;
	bOk = CreateProcess(NULL, lpszCmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	if (!bOk)
		return 0;
	CloseHandle(pi.hThread);
	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);

	delete lpszCmd;
	return 1;
}

static void VSD_RunVdba(CtrfItemData* pItem)
{
	#if defined (MAINWIN)
	CString strAppName = _T("vdba");
	#else
	CString strAppName = _T("vdba.exe");
	#endif
	if (pItem->IsFolder())
		return;
	CString strCmd = _T("");
	CaVsdDisplayInfo* pDisplayInfo = (CaVsdDisplayInfo*)pItem->GetDisplayInfo();
	ASSERT(pDisplayInfo);
	if (!pDisplayInfo)
		return;
	CaVsdRefreshTreeInfo* pPopulateParamp = pDisplayInfo->GetRefreshTreeInfo();
	ASSERT(pPopulateParamp);
	if (!pPopulateParamp)
		return;
	TCHAR* pEnv;
	pEnv = _tgetenv(_T("II_SYSTEM"));
	if (!pEnv)
	{
		//_T("II_SYSTEM is not defined");
		AfxMessageBox (IDS_MSG_II_SYSTEM_NOT_DEFINED);
		return;
	}
	CString strFullPath = (LPCTSTR)pEnv;
	strFullPath += consttchszIngVdba;
	strFullPath += strAppName;
	strFullPath += _T(" /c NONODESWINDOW NOSAVEONEXIT NOSPLASHSCREEN");

	CaVsdItemData* pVsdItemData = (CaVsdItemData*)pItem->GetUserData();
	int nObjectType = pItem->GetDBObject()->GetObjectID();
	CString strType = GetCmdType(nObjectType);
	switch (nObjectType)
	{
	case OBT_USER:
	case OBT_GROUP:
	case OBT_ROLE:
	case OBT_PROFILE:
	case OBT_LOCATION:
		strCmd.Format (
		    _T("%s DOM NOMENU %s %s %s, DOM NOMENU %s %s %s"), 
		    (LPCTSTR)strFullPath,
		    (LPCTSTR)pPopulateParamp->GetNode1(),
		    (LPCTSTR)strType,
		    (LPCTSTR)(pItem->GetDBObject())->GetName(),
		    (LPCTSTR)pPopulateParamp->GetNode2(),
		    (LPCTSTR)strType,
		    (LPCTSTR)(pItem->GetDBObject())->GetName());
		break;
	case OBT_DATABASE:
		{
			CString strDatabase1 = pPopulateParamp->GetDatabase1();
			CString strDatabase2 = pPopulateParamp->GetDatabase2();

			strCmd.Format (
				_T("%s DOM NOMENU %s %s %s, DOM NOMENU %s %s %s"), 
				(LPCTSTR)strFullPath,
				(LPCTSTR)pPopulateParamp->GetNode1(),
				(LPCTSTR)strType,
				(LPCTSTR)strDatabase1,
				(LPCTSTR)pPopulateParamp->GetNode2(),
				(LPCTSTR)strType,
				(LPCTSTR)strDatabase2);
		}
		break;
	case OBT_TABLE:
	case OBT_VIEW:
	case OBT_PROCEDURE:
	case OBT_SEQUENCE:
	case OBT_DBEVENT:
		{
			CString strDatabase1 = pPopulateParamp->GetDatabase1();
			CString strDatabase2 = pPopulateParamp->GetDatabase2();
			if (pPopulateParamp->IsInstallation())
			{
				CaLLQueryInfo* pInfo = pItem->GetQueryInfo(NULL);
				strDatabase1 = pInfo->GetDatabase();
				strDatabase2 = pInfo->GetDatabase();
			}
			//
			// Actually we do not provide the owner of object.
			// The owner of the second object can be fetched from 'pVsdItemData':
			// pVsdItemData->GetOwner2();
			strCmd.Format (
				_T("%s DOM NOMENU %s %s %s/%s, DOM NOMENU %s %s %s/%s"), 
				(LPCTSTR)strFullPath,
				(LPCTSTR)pPopulateParamp->GetNode1(),
				(LPCTSTR)strType,
				(LPCTSTR)strDatabase1,
				(LPCTSTR)(pItem->GetDBObject())->GetName(),
				(LPCTSTR)pPopulateParamp->GetNode2(),
				(LPCTSTR)strType,
				(LPCTSTR)strDatabase2,
				(LPCTSTR)(pItem->GetDBObject())->GetName());
		}
		break;
	case OBT_INDEX:
	case OBT_RULE:
		{
			CString strDatabase1 = pPopulateParamp->GetDatabase1();
			CString strDatabase2 = pPopulateParamp->GetDatabase2();
			CaLLQueryInfo* pInfo = pItem->GetQueryInfo(NULL);
			if (pPopulateParamp->IsInstallation())
			{
				strDatabase1 = pInfo->GetDatabase();
				strDatabase2 = pInfo->GetDatabase();
			}
			CString strTable1 = pInfo->GetItem2();
			CString strTable2 = pInfo->GetItem2();
			//
			// Actually we do not provide the owner of object.
			// The owner of the second object can be fetched from 'pVsdItemData':
			strCmd.Format (
				_T("%s DOM NOMENU %s %s %s/%s/%s, DOM NOMENU %s %s %s/%s/%s"), 
				(LPCTSTR)strFullPath,
				(LPCTSTR)pPopulateParamp->GetNode1(),
				(LPCTSTR)strType,
				(LPCTSTR)strDatabase1,
				(LPCTSTR)strTable1,
				(LPCTSTR)(pItem->GetDBObject())->GetName(),
				(LPCTSTR)pPopulateParamp->GetNode2(),
				(LPCTSTR)strType,
				(LPCTSTR)strDatabase2,
				(LPCTSTR)strTable2,
				(LPCTSTR)(pItem->GetDBObject())->GetName());
		}
		break;
	default:
		return;
		break;
	}
	if (strCmd.IsEmpty())
		return;

	LPTSTR lpszCmd = new TCHAR [strCmd.GetLength() + 1];
	_tcscpy (lpszCmd, strCmd);
	CWinThread* pTread = AfxBeginThread(StartThreadProc, (LPVOID)lpszCmd);
}



IMPLEMENT_DYNCREATE(CvSdaLeft, CTreeView)
CvSdaLeft::CvSdaLeft()
{
	m_bInstallation = TRUE;
}

CvSdaLeft::~CvSdaLeft()
{
}


BEGIN_MESSAGE_MAP(CvSdaLeft, CTreeView)
	//{{AFX_MSG_MAP(CvSdaLeft)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDED, OnItemexpanded)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
	ON_WM_RBUTTONDOWN()
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, OnItemexpanding)
	ON_COMMAND(ID_POPUP_DISCARD, OnPopupDiscard)
	ON_COMMAND(ID_POPUP_UNDISCARD, OnPopupUndiscard)
	ON_NOTIFY_REFLECT(TVN_SELCHANGING, OnSelchanging)
	ON_COMMAND(ID_POPUP_ACCESSVDBA, OnPopupAccessVdba)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CvSdaLeft drawing

void CvSdaLeft::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CvSdaLeft diagnostics

#ifdef _DEBUG
void CvSdaLeft::AssertValid() const
{
	CTreeView::AssertValid();
}

void CvSdaLeft::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CvSdaLeft message handlers

BOOL CvSdaLeft::PreCreateWindow(CREATESTRUCT& cs) 
{
	cs.style |= TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS|TVS_SHOWSELALWAYS;
	return CTreeView::PreCreateWindow(cs);
}

void CvSdaLeft::OnInitialUpdate() 
{
	CTreeView::OnInitialUpdate();
	
	m_tree.SetSessionManager(&theApp.m_sessionManager);
	CTreeCtrl& cTree = GetTreeCtrl();
	m_tree.GetTreeCtrlData().SetTreeCtrl(&cTree);

	m_ImageList.Create (IM_WIDTH, IM_HEIGHT, TRUE, 18, 8);
	cTree.SetImageList (&m_ImageList, TVSIL_NORMAL);

	int i =0;
	CImageList imageList;
	HICON hIcon;
	//
	// FOLDERS:
	imageList.Create (IDB_VSD_FOLDER, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <16; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// EMPTY:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_EMPTY, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <1; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// DISCARD:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_DISCARD, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <1; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// INSTALLATION:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_INSTALLATION, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// GROUP:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_GROUP, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// USER:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_USER, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// ROLE:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_ROLE, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// PROFILE:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_PROFILE, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// LOCATION:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_LOCATION, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// DATABASE:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_DATABASE, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// DATABASE DISTRIBUTED:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_DATABASE_DISTRIBUTED, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// DATABASE COORDINATOR:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_DATABASE_COORDINATOR, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// TABLE:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_TABLE, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// TABLE STAR NATIVE:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_TABLE_STAR_NATIVE, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// TABLE STAR REGISTER AS LINK:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_TABLE_STAR_LINK, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}

	//
	// VIEW:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_VIEW, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// VIEW STAR NATIVE:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_VIEW_STAR_NATIVE, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// VIEW STAR REGISTER AS LINK:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_VIEW_STAR_LINK, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// PROCEDURE:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_PROCEDURE, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// PROCEDURE STAR:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_PROCEDURE_STAR, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// SEQUENCE:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_SEQUENCE, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// DBEVENT:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_DBEVENT, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// SYNONYM:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_SYNONYM, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// INDEX:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_INDEX, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// INDEX Register As link:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_INDEX_STAR_LINK, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// RULE:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_RULE, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// INTEGRITY:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_INTEGRITY, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// ALARM OF INSTALLATION:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_ALARM_INSTALLATION, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// ALARM OF DATABASE:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_ALARM_DATABASE, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// ALARM OF TABLE:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_ALARM_TABLE, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// GRANTEE OF INSTALLATION:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_GRANTEE_INSTALLATION, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// GRANTEE OF DATABASE:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_GRANTEE_DATABASE, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// GRANTEE OF TABLE:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_GRANTEE_TABLE, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// GRANTEE OF VIEW:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_GRANTEE_VIEW, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// GRANTEE OF PROCEDURE:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_GRANTEE_PROCEDURE, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// GRANTEE OF SEQUENCE:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_GRANTEE_SEQUENCE, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	//
	// GRANTEE OF DBEVENT:
	imageList.DeleteImageList();
	imageList.Create (IDB_VSD_GRANTEE_DBEVENT, IM_WIDTH, 0, RGB(255,0,255));
	for (i = 0; i <8; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	imageList.DeleteImageList();
}

BOOL CvSdaLeft::IsValidParameters (UINT& uiStar)
{
	CdSdaDoc* pDoc = (CdSdaDoc*)GetDocument();
	ASSERT(pDoc);
	if (!pDoc)
		return FALSE;

	m_bInstallation = FALSE;
	//
	// Check the valid parameters:
	if (pDoc->GetSchema1Type() == COMPARE_DBMS)
	{
		if (pDoc->GetSchema2Type() == COMPARE_DBMS)
		{
			if (pDoc->GetDatabase1().IsEmpty() && pDoc->GetDatabase2().IsEmpty())
			{
				//
				// Installation
				m_bInstallation = TRUE;
			}
			else
			if (pDoc->GetDatabase1().IsEmpty() || pDoc->GetDatabase2().IsEmpty())
			{
				//
				// Database Schema(s) cannot be compared with an installation.
				// If you select "<installation>" for a group of objects,
				// it must be selected also for the other group.
				AfxMessageBox(IDS_MSG_COMPARE_WRONGTYPE);
				return FALSE;
			}
		}
		else
		{
			if (pDoc->GetDatabase1().IsEmpty())
			{
				//
				// The file must have saved the installion
				if (!VSD_Installation(pDoc->GetFile2()))
				{
					//
					// The specified file does not contain "<Installation>" information.
					AfxMessageBox(IDS_MSG_COMPARE_WRONGTYPE_INSTALLATIONxFILE);
					return FALSE;
				}
				m_bInstallation = TRUE;
			}
			else
			{
				//
				// The file must not have saved the installion
				if (VSD_Installation(pDoc->GetFile2()))
				{
					//
					// The specified file contains "<Installation>" information and cannot be compared with database schema. 
					AfxMessageBox(IDS_MSG_COMPARE_WRONGTYPE_SCHEMAxFILE);
					return FALSE;
				}
			}
		}
	}
	else
	{
		if (pDoc->GetSchema2Type() == COMPARE_DBMS)
		{
			if (pDoc->GetDatabase2().IsEmpty())
			{
				//
				// The file must have saved installation
				if (!VSD_Installation(pDoc->GetFile1()))
				{
					//
					// The specified file does not contain "<Installation>" information.
					AfxMessageBox(IDS_MSG_COMPARE_WRONGTYPE_INSTALLATIONxFILE);
					return FALSE;
				}
				m_bInstallation = TRUE;
			}
			else
			{
				//
				// The file must not have saved the installion
				if (VSD_Installation(pDoc->GetFile1()))
				{
					//
					// The specified file contains "<Installation>" information and cannot be compared with database schema. 
					AfxMessageBox(IDS_MSG_COMPARE_WRONGTYPE_SCHEMAxFILE);
					return FALSE;
				}
			}
		}
		else
		{
			//
			// Both files must either have saved installation or
			// must not have saved installation:
			BOOL bType1 = VSD_Installation(pDoc->GetFile1());
			BOOL bType2 = VSD_Installation(pDoc->GetFile2());

			if (bType1 != bType2)
			{
				//
				// The specified files are not compatible.
				AfxMessageBox(IDS_MSG_COMPARE_WRONGTYPE_FILExFILE);
				return FALSE;
			}
			if (bType1 || bType2)
				m_bInstallation = TRUE;
		}
	}

	CString strUser1 = pDoc->GetUser1();
	CString strUser2 = pDoc->GetUser2();
	if (pDoc->GetSchema1Type() == COMPARE_FILE)
	{
		CaVsdaLoadSchema loadschema;
		loadschema.SetFile(pDoc->GetFile1());
		if (!loadschema.LoadSchema())
			return FALSE;
		strUser1 = loadschema.m_strSchema;
		loadschema.Close();
	}
	if (pDoc->GetSchema2Type() == COMPARE_FILE)
	{
		CaVsdaLoadSchema loadschema;
		loadschema.SetFile(pDoc->GetFile2());
		if (!loadschema.LoadSchema())
			return FALSE;
		strUser2 = loadschema.m_strSchema;
		loadschema.Close();
	}

	if ((strUser1.IsEmpty() && !strUser2.IsEmpty()) || (!strUser1.IsEmpty() && strUser2.IsEmpty()))
	{
		//
		// You should compare a specific schama with a specific schame or \n<all schemas> with <all schemas>.
		AfxMessageBox(IDS_MSG_COMPARE_WRONGTYPE_SCHEMA);
		return FALSE;
	}

	//
	// Check to see if both databases are star:
	uiStar = 0;
	if (!m_bInstallation)
	{
		CmtSessionManager* pSmgr = &(theApp.m_sessionManager);
		CaLLQueryInfo qry1;
		CaLLQueryInfo qry2;
		qry1.SetObjectType(OBT_DATABASE);
		qry2.SetObjectType(OBT_DATABASE);
		qry1.SetNode(pDoc->GetNode1());
		qry1.SetDatabase(_T("iidbdb"));
		qry2.SetNode(pDoc->GetNode2());
		qry2.SetDatabase(_T("iidbdb"));
		qry1.SetLockMode(LM_NOLOCK);
		qry2.SetLockMode(LM_NOLOCK);

		CaDatabaseDetail to1, to2;
		to1.SetItem(pDoc->GetDatabase1(), pDoc->GetUser1());
		to2.SetItem(pDoc->GetDatabase2(), pDoc->GetUser2()); 
		CaDBObject* pDt1 = &to1;
		CaDBObject* pDt2 = &to2;

		BOOL bOk   = INGRESII_llQueryDetailObject (&qry1, &to1, pSmgr);
		BOOL bOk2  = INGRESII_llQueryDetailObject (&qry2, &to2, pSmgr);

		if (bOk && bOk2 && pDt1 && pDt2)
		{
			UINT nFlag1 = to1.GetService();
			UINT nFlag2 = to2.GetService();
			if (to1.GetStar() != to2.GetStar())
			{
				// 
				// Database cannot be compared with an Star Database type
				AfxMessageBox(IDS_MSG_COMPARE_WRONGTYPE_DATABASE);
				return FALSE;
			}
			else
			{
				if (nFlag1 & DBFLAG_STARNATIVE)
					uiStar = nFlag1;
				return TRUE;
			}
		}
	}

	return TRUE;
}

BOOL CvSdaLeft::DoCompare (short nMode)
{
	CString strMsg;
	try
	{
		UINT iStar = 0;
		if (!IsValidParameters(iStar))
			return FALSE;
		CdSdaDoc* pDoc = (CdSdaDoc*)GetDocument();
		BOOL bIgnoreOwner = TRUE;
		switch (nMode)
		{
		case 0:
			bIgnoreOwner = FALSE;
			break;
		case 1:
			bIgnoreOwner = TRUE;
			break;
		default:
			ASSERT(FALSE);
			break;
		}

		CTreeCtrl& cTree = GetTreeCtrl();
		HTREEITEM hRoot =  TVI_ROOT;
		CaLLQueryInfo* pQueryInfo = m_tree.GetQueryInfo();
		ASSERT(pQueryInfo);
		if (pQueryInfo)
		{
			CfSdaFrame* pFrame = (CfSdaFrame*)GetParentFrame();
			ASSERT (pFrame);
			if (pFrame)
			{
				CWnd* pWndRightPane = pFrame->GetRightPane();
				if (pWndRightPane)
				{
					//
					// Clear the right pane:
					pWndRightPane->SendMessage (WMUSRMSG_UPDATEDATA, (WPARAM)0, (LPARAM)NULL);
				}
			}

			cTree.DeleteAllItems();
			m_tree.Cleanup();
			m_tree.Initialize(m_bInstallation);

			CaVsdDisplayInfo* pDisplayInfo = (CaVsdDisplayInfo*)m_tree.GetDisplayInfo();
			ASSERT(pDisplayInfo);
			if (!pDisplayInfo)
				return FALSE;
			CaVsdRefreshTreeInfo* pPopulateParamp = pDisplayInfo->GetRefreshTreeInfo();
			ASSERT(pPopulateParamp);
			if (!pPopulateParamp)
				return FALSE;
			pPopulateParamp->Initialize();
			pPopulateParamp->SetFlag(iStar);

			pPopulateParamp->SetInstallation(m_bInstallation);
			pPopulateParamp->SetIgnoreOwner(bIgnoreOwner);
#if defined (_DISPLAY_OWNER_OBJECT)
			CString strFormat;
			strFormat.LoadString(IDS_OWNERxITEM);
			if (pDoc->GetUser1().IsEmpty() && pDoc->GetUser2().IsEmpty())
				pDisplayInfo->SetOwnerPrefixedFormat(strFormat);
			else
				pDisplayInfo->SetOwnerPrefixedFormat(_T(""));
#endif
			ASSERT(pDoc->GetSchema1Type() != COMPARE_UNDEFINED && pDoc->GetSchema2Type() != COMPARE_UNDEFINED);
			if (pDoc->GetSchema1Type() != COMPARE_UNDEFINED && pDoc->GetSchema2Type() != COMPARE_UNDEFINED)
			{
				if (pDoc->GetSchema1Type() == COMPARE_DBMS && pDoc->GetSchema2Type() == COMPARE_DBMS) 
				{
					pPopulateParamp->SetNode (pDoc->GetNode1(), pDoc->GetNode2());
					pPopulateParamp->SetDatabase (pDoc->GetDatabase1(), pDoc->GetDatabase2());
					pPopulateParamp->SetSchema (pDoc->GetUser1(), pDoc->GetUser2());
					pPopulateParamp->SetLoadSchema (_T(""), _T(""));

					if (pDisplayInfo)
					{
						if (m_bInstallation)
							pDisplayInfo->SetCompareTitles(pDoc->GetNode1(), pDoc->GetNode2());
						else
						{
							CString strT1, strT2;
							if (!pDoc->GetUser1().IsEmpty())
								strT1.Format(
									_T("%s/%s (user: %s)"), 
									(LPCTSTR)pDoc->GetNode1(), 
									(LPCTSTR)pDoc->GetDatabase1(), 
									(LPCTSTR)pDoc->GetUser1());
							else
								strT1.Format(_T("%s/%s"), (LPCTSTR)pDoc->GetNode1(), (LPCTSTR)pDoc->GetDatabase1());

							if (!pDoc->GetUser2().IsEmpty())
								strT2.Format(
									_T("%s/%s (user: %s)"), 
									(LPCTSTR)pDoc->GetNode2(), 
									(LPCTSTR)pDoc->GetDatabase2(), 
									(LPCTSTR)pDoc->GetUser2());
							else
								strT2.Format(_T("%s/%s"), (LPCTSTR)pDoc->GetNode2(), (LPCTSTR)pDoc->GetDatabase2());
							pDisplayInfo->SetCompareTitles(strT1, strT2);
						}
					}
				}
				else
				if (pDoc->GetSchema1Type() == COMPARE_FILE && pDoc->GetSchema2Type() == COMPARE_FILE)
				{
					pPopulateParamp->SetLoadSchema (pDoc->GetFile1(), pDoc->GetFile2());
					if (pDisplayInfo)
						pDisplayInfo->SetCompareTitles(pDoc->GetFile1(), pDoc->GetFile2());
				}
				else
				if (pDoc->GetSchema1Type() == COMPARE_DBMS && pDoc->GetSchema2Type() == COMPARE_FILE)
				{
					pPopulateParamp->SetNode (pDoc->GetNode1(), _T(""));
					pPopulateParamp->SetDatabase (pDoc->GetDatabase1(), _T(""));
					pPopulateParamp->SetSchema (pDoc->GetUser1(), _T(""));
					pPopulateParamp->SetLoadSchema (_T(""), pDoc->GetFile2());
					if (pDisplayInfo)
					{
						if (m_bInstallation)
							pDisplayInfo->SetCompareTitles(pDoc->GetNode1(), pDoc->GetFile2());
						else
						{
							CString strT1;
							if (!pDoc->GetUser1().IsEmpty())
								strT1.Format(
									_T("%s/%s (user: %s)"), 
									(LPCTSTR)pDoc->GetNode1(), 
									(LPCTSTR)pDoc->GetDatabase1(), 
									(LPCTSTR)pDoc->GetUser1());
							else
								strT1.Format(_T("%s/%s"), (LPCTSTR)pDoc->GetNode1(), (LPCTSTR)pDoc->GetDatabase1());
							pDisplayInfo->SetCompareTitles(strT1, pDoc->GetFile2());
						}
					}
				}
				else // (pDoc->GetSchema1Type() == COMPARE_FILE && pDoc->GetSchema2Type() == COMPARE_DBMS)
				{
					pPopulateParamp->SetNode (_T(""), pDoc->GetNode2());
					pPopulateParamp->SetDatabase (_T(""), pDoc->GetDatabase2());
					pPopulateParamp->SetSchema (_T(""), pDoc->GetUser2());
					pPopulateParamp->SetLoadSchema (pDoc->GetFile1(), _T(""));
					if (pDisplayInfo)
					{
						if (m_bInstallation)
							pDisplayInfo->SetCompareTitles(pDoc->GetFile1(), pDoc->GetNode2());
						else
						{
							CString strT2;
							if (!pDoc->GetUser2().IsEmpty())
								strT2.Format(
									_T("%s/%s (user: %s)"), 
									(LPCTSTR)pDoc->GetNode2(), 
									(LPCTSTR)pDoc->GetDatabase2(), 
									(LPCTSTR)pDoc->GetUser2());
							else
								strT2.Format(_T("%s/%s"), (LPCTSTR)pDoc->GetNode2(), (LPCTSTR)pDoc->GetDatabase2());
							pDisplayInfo->SetCompareTitles(pDoc->GetFile1(), strT2);
						}
					}
				}

				pPopulateParamp->Interrupt(FALSE);
				CaExecParamComparing* p = new CaExecParamComparing(pPopulateParamp, &m_tree, m_bInstallation);
				CString strMsgAnimateTitle = _T("Comparing...");
				if (m_bInstallation)
					strMsgAnimateTitle.LoadString(IDS_TITLE_ANIMATE_INSTALLATION);
				else
					strMsgAnimateTitle.LoadString(IDS_TITLE_ANIMATE_DATABASE_SCHEMA);
#if defined (_ANIMATION_)
				CxDlgWait dlg (strMsgAnimateTitle);
				dlg.SetUseAnimateAvi(AVI_EXAMINE);
				dlg.SetExecParam (p);
				dlg.SetDeleteParam(FALSE);
				dlg.SetShowProgressBar(FALSE);
				dlg.SetUseExtraInfo();
				dlg.SetHideCancelBottonAlways(FALSE);
				dlg.DoModal();
#else
				p->Run();
#endif
				CString strError = p->GetFailMessage();
				BOOL bPopulated = p->IsPopulated();
				delete p;
				if (!strError.IsEmpty())
				{
					AfxMessageBox (strError);
				}
				else
				if (bPopulated)
				{
					if (m_bInstallation)
						m_tree.Display(&cTree);
					else
					{
						CaVsdDatabase* pDatabase = m_tree.FindFirstDatabase();
						if (pDatabase)
							pDatabase->Display(&cTree);
					}
				}
				else
				if (pPopulateParamp->IsInterrupted())
				{
					AfxMessageBox (IDS_COMPARISON_ABORTED, MB_ICONEXCLAMATION|MB_OK);
				}
				SortTreeItem(cTree, cTree.GetRootItem());
				cTree.SetFocus();
				Invalidate();
			}
		}

		GetParentFrame()->ShowWindow(SW_SHOW);
#if defined (_CHECK_NUMSESSION_IN_CACHE)
		CTypedPtrArray< CObArray, CaSession* >& ll = theApp.m_sessionManager.GetListSessions();
		INT_PTR nCount  = ll.GetSize();
		CString ss=_T("");
		for (INT_PTR i = 0; i <  nCount; i++)
		{
			CaSession* pSession = ll.GetAt(i);
			if (pSession && pSession->IsConnected())
			{
				CString s;
				s.Format (_T("[%s::%s Released=%d], "),pSession->GetNode(), pSession->GetDatabase(), pSession->IsReleased());
				ss += s;
			}
		}
		AfxMessageBox(ss);
#endif
		theApp.m_sessionManager.Cleanup(); // Disconnect all DBMS Sessions
		return TRUE;
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason());
	}
	catch (CFileException* e)
	{
		e->ReportError();
		e->Delete();
	}
	catch (CArchiveException* e)
	{
		e->ReportError();
		e->Delete();
	}
	catch (CeCompoundFileException e)
	{
		strMsg.Format(IDS_FILE_ERROR, e.GetErrorCode());
		AfxMessageBox (strMsg);
	}
	catch (...)
	{
		strMsg = _T("Unknown error occured");
		AfxMessageBox (strMsg);
	}
	return FALSE;
}

void CvSdaLeft::Reset ()
{
	CTreeCtrl& cTree = GetTreeCtrl();
	HTREEITEM hRoot =  TVI_ROOT;
	CaLLQueryInfo* pQueryInfo = m_tree.GetQueryInfo();
	if (cTree.DeleteAllItems())
	{
		m_tree.Reset();
	}
}

static void ManageItem (CString& strItem, CArray<TreeItemType, TreeItemType>& arrayItem)
{
	strItem.MakeLower();
	if (strItem.Find(_T("database")) != -1)
		arrayItem.Add(O_FOLDER_DATABASE);
	else
	if (strItem.Find(_T("location")) != -1)
		arrayItem.Add(O_FOLDER_LOCATION);
	else
	if (strItem.Find(_T("user")) != -1)
		arrayItem.Add(O_FOLDER_USER);
	else
	if (strItem.Find(_T("group")) != -1)
		arrayItem.Add(O_FOLDER_GROUP);
	else
	if (strItem.Find(_T("role")) != -1)
		arrayItem.Add(O_FOLDER_ROLE);
	else
	if (strItem.Find(_T("profile")) != -1)
		arrayItem.Add(O_FOLDER_PROFILE);
	else
	if (strItem.Find(_T("grant")) != -1)
		arrayItem.Add(O_FOLDER_GRANTEE);
	else
	if (strItem.Find(_T("alarm")) != -1)
		arrayItem.Add(O_FOLDER_ALARM);
	else
	if (strItem.Find(_T("table")) != -1)
		arrayItem.Add(O_FOLDER_TABLE);
	else
	if (strItem.Find(_T("view")) != -1)
		arrayItem.Add(O_FOLDER_VIEW);
	else
	if (strItem.Find(_T("procedure")) != -1)
		arrayItem.Add(O_FOLDER_PROCEDURE);
	else
	if (strItem.Find(_T("dbevent")) != -1)
		arrayItem.Add(O_FOLDER_DBEVENT);
	else
	if (strItem.Find(_T("synonym")) != -1)
		arrayItem.Add(O_FOLDER_SYNONYM);
	else
	if (strItem.Find(_T("sequence")) != -1)
		arrayItem.Add(O_FOLDER_SEQUENCE);
}

void CvSdaLeft::UpdateDisplayFilter(LPCTSTR lpszFilter)
{
	CArray<TreeItemType, TreeItemType> arrayItem;
	CString strFilter = lpszFilter;
	CString strItem = strFilter;
	int nPos = strFilter.Find(_T(','));

	while (nPos != -1)
	{
		strItem = strFilter.Left(nPos);
		strFilter = strFilter.Mid(nPos + 1);
		nPos = strFilter.Find(_T(','));

		ManageItem (strItem, arrayItem);
		strItem = strFilter;
	}
	ManageItem (strItem, arrayItem);
	CTreeCtrl& cTree = GetTreeCtrl();
	if (cTree.GetRootItem() == NULL)
		return;
	m_tree.UnDisplayItems(arrayItem);
	UpdateDisplayTree();
	SortTreeItem(cTree, cTree.GetRootItem());

	HTREEITEM hSel = cTree.GetSelectedItem();
	if (hSel)
	{
		CtrfItemData* pItem = (CtrfItemData*)cTree.GetItemData (hSel);
		CfSdaFrame* pFrame = (CfSdaFrame*)GetParentFrame();
		ASSERT (pFrame);
		if (pFrame)
		{
			CWnd* pWndRightPane = pFrame->GetRightPane();
			if (pWndRightPane)
				pWndRightPane->SendMessage (WMUSRMSG_UPDATEDATA, (WPARAM)0, (LPARAM)pItem);
		}
	}
}



void CvSdaLeft::OnItemexpanded(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	*pResult = 0;

	HTREEITEM hExpand = pNMTreeView->itemNew.hItem;
	CTreeCtrl& cTree = GetTreeCtrl();
	try
	{
		CtrfItemData* pItem = (CtrfItemData*)cTree.GetItemData (hExpand);
		if (!pItem)
			return;
		if (pNMTreeView->action & TVE_COLLAPSE)
		{
			//
			// Collapseing:
			pItem->Collapse (&cTree, hExpand);
		}
		else
		if (pNMTreeView->action & TVE_EXPAND)
		{
			//
			// Expanding:
			CWaitCursor doWaitCursor;
			pItem->Expand (&cTree, hExpand);
		}
	}
	catch(CeSqlException e)
	{
		AfxMessageBox(e.GetReason());
	}
	catch(...)
	{
		TRACE0 ("System error: CvViewLeft::OnItemexpanded\n");
		*pResult = 1;
	}
}

void CvSdaLeft::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	*pResult = 0;
	CWaitCursor doWaitCursor ;

	try 
	{
		CfSdaFrame* pFrame = (CfSdaFrame*)GetParentFrame();
		ASSERT (pFrame);
		if (!pFrame)
			return;
		if (!pFrame->IsAllViewCreated())
			return;
		CTreeCtrl& treeCtrl = GetTreeCtrl();

		if (pNMTreeView->itemNew.hItem != NULL)
		{
			CtrfItemData* pItem = (CtrfItemData*)treeCtrl.GetItemData (pNMTreeView->itemNew.hItem);
			CWnd* pWndRightPane = pFrame->GetRightPane();
			if (pWndRightPane)
				pWndRightPane->SendMessage (WMUSRMSG_UPDATEDATA, (WPARAM)0, (LPARAM)pItem);
		}
	}
	catch(CMemoryException* e)
	{
		e->ReportError();
		e->Delete();
	}
	catch(CResourceException* e)
	{
		e->ReportError();
		e->Delete();
	}
	catch(...)
	{
	}
}


void CvSdaLeft::OnRButtonDown(UINT nFlags, CPoint point) 
{
	CTreeView::OnRButtonDown(nFlags, point);

	CMenu menu;
	VERIFY(menu.LoadMenu(IDR_POPUP_DISCARD));

	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);
	CWnd* pWndPopupOwner = this;
	//
	// Menu handlers are coded in this source:
	// So do not use the following statement:
	// while (pWndPopupOwner->GetStyle() & WS_CHILD)
	//     pWndPopupOwner = pWndPopupOwner->GetParent();
	//
	CPoint p = point;
	ClientToScreen(&p);
	BOOL bStartEnable = FALSE;
	BOOL bStartClientEnable = FALSE;
	BOOL bStopEnable = FALSE;
	CtrfItemData* pItem = NULL;
	CTreeCtrl& cTree = GetTreeCtrl();
	UINT nFlag = TVHT_ONITEM;
	HTREEITEM hHit = cTree.HitTest (point, &nFlag);
	if (hHit)
		cTree.SelectItem (hHit);
	HTREEITEM hSel = cTree.GetSelectedItem();
	if (hSel)
	{
		pItem = (CtrfItemData*)cTree.GetItemData (hSel);
		if (pItem && pItem->GetTreeItemType() != O_BASE_ITEM)
		{
			//
			// Enable/Disable the menu <Discard Branch> / <Un-Discard Branch>:
			CaVsdItemData* pVsdItemData = (CaVsdItemData*)pItem->GetUserData();
			if (pVsdItemData->IsDiscarded())
				menu.EnableMenuItem (ID_POPUP_DISCARD, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
			else
				menu.EnableMenuItem (ID_POPUP_DISCARD, MF_BYCOMMAND|MF_ENABLED);

			BOOL bHasDiscard = FALSE;
			VSD_HasDiscadItem(pItem, bHasDiscard, FALSE);
			if (bHasDiscard)
				menu.EnableMenuItem (ID_POPUP_UNDISCARD, MF_BYCOMMAND|MF_ENABLED);
			else
				menu.EnableMenuItem (ID_POPUP_UNDISCARD, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);

			//
			// Enable/Disable the menu <Access in VDBA Windows>:
			BOOL bAllowAccesVdba = AllowAccess2Vdba(pItem);
			if (bAllowAccesVdba)
				menu.EnableMenuItem (ID_POPUP_ACCESSVDBA, MF_BYCOMMAND|MF_ENABLED);
			else
				menu.EnableMenuItem (ID_POPUP_ACCESSVDBA, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);

			pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, p.x, p.y, pWndPopupOwner);
		}
	}
}

void CvSdaLeft::OnItemexpanding(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	HTREEITEM hExpand = pNMTreeView->itemNew.hItem;
	*pResult = 0;
	CTreeCtrl& cTree = GetTreeCtrl();
	try
	{
		CtrfItemData* pItem = (CtrfItemData*)cTree.GetItemData (hExpand);
		if (!pItem)
			return;
		if (pNMTreeView->action & TVE_EXPAND)
		{
			//
			// Expanding:
			CaVsdItemData* pVsdItemData = (CaVsdItemData*)pItem->GetUserData();
			ASSERT(pVsdItemData);
			if (pVsdItemData && pVsdItemData->IsDiscarded())
			{
				*pResult = 1;
				return;
			}
		}
	}
	catch(CeSqlException e)
	{
		AfxMessageBox(e.GetReason());
	}
	catch(...)
	{
		TRACE0 ("System error: CvViewLeft::OnItemexpanding\n");
		*pResult = 1;
	}
}

void CvSdaLeft::UpdateDisplayTree()
{
	CTreeCtrl& cTree = GetTreeCtrl();
	if (m_bInstallation)
	{
		m_tree.Display(&cTree);
	}
	else
	{
		CaVsdDatabase* pDatabase = m_tree.FindFirstDatabase();
		ASSERT(pDatabase);
		if (pDatabase)
			pDatabase->Display(&cTree);
	}
}

void CvSdaLeft::OnPopupDiscard() 
{
	CTreeCtrl& cTree = GetTreeCtrl();
	HTREEITEM hSel = cTree.GetSelectedItem();
	if (hSel)
	{
		CtrfItemData* pItem = (CtrfItemData*)cTree.GetItemData (hSel);
		CaVsdItemData* pVsdItemData = (CaVsdItemData*)pItem->GetUserData();
		pVsdItemData->SetDiscard(TRUE);
		cTree.Expand(hSel, TVE_COLLAPSE);
		pItem->Collapse (&cTree, hSel);
		UpdateDisplayTree();
		CfSdaFrame* pFrame = (CfSdaFrame*)GetParentFrame();
		ASSERT (pFrame);
		if (pFrame)
		{
			CWnd* pWndRightPane = pFrame->GetRightPane();
			if (pWndRightPane)
				pWndRightPane->SendMessage (WMUSRMSG_UPDATEDATA, (WPARAM)0, (LPARAM)pItem);
		}
	}
}

void CvSdaLeft::OnPopupUndiscard() 
{
	CTreeCtrl& cTree = GetTreeCtrl();
	HTREEITEM hSel = cTree.GetSelectedItem();
	if (hSel)
	{
		CtrfItemData* pItem = (CtrfItemData*)cTree.GetItemData (hSel);
		UnDiscadItem(pItem);
		UpdateDisplayTree();
		CfSdaFrame* pFrame = (CfSdaFrame*)GetParentFrame();
		ASSERT (pFrame);
		if (pFrame)
		{
			CWnd* pWndRightPane = pFrame->GetRightPane();
			if (pWndRightPane)
				pWndRightPane->SendMessage (WMUSRMSG_UPDATEDATA, (WPARAM)0, (LPARAM)pItem);
		}
	}
}


BOOL CvSdaLeft::IsEnableDiscard() 
{
	BOOL bEnable = FALSE;
	CTreeCtrl& cTree = GetTreeCtrl();
	HTREEITEM hSel = cTree.GetSelectedItem();
	if (hSel)
	{
		CtrfItemData* pItem = (CtrfItemData*)cTree.GetItemData (hSel);
		if (pItem)
		{
			CaVsdItemData* pVsdItemData = (CaVsdItemData*)pItem->GetUserData();
			bEnable = pVsdItemData->IsDiscarded()? FALSE: TRUE;
		}
	}
	return bEnable;
}

void CvSdaLeft::DiscardItem() 
{
	OnPopupDiscard();
}

BOOL CvSdaLeft::IsEnableUndiscard() 
{
	BOOL bEnable = FALSE;
	CTreeCtrl& cTree = GetTreeCtrl();
	HTREEITEM hSel = cTree.GetSelectedItem();
	if (hSel)
	{
		CtrfItemData* pItem = (CtrfItemData*)cTree.GetItemData (hSel);
		if (pItem)
		{
			CaVsdItemData* pVsdItemData = (CaVsdItemData*)pItem->GetUserData();
			BOOL bHasDiscard = FALSE;
			VSD_HasDiscadItem(pItem, bHasDiscard, FALSE);
			bEnable = bHasDiscard? TRUE: FALSE;
		}
	}
	return bEnable;
}

void CvSdaLeft::UndiscardItem() 
{
	OnPopupUndiscard();
}

BOOL CvSdaLeft::IsEnableAccessVdba()
{
	BOOL bEnable = FALSE;
	CTreeCtrl& cTree = GetTreeCtrl();
	HTREEITEM hSel = cTree.GetSelectedItem();
	if (hSel)
	{
		CtrfItemData* pItem = (CtrfItemData*)cTree.GetItemData (hSel);
		if (pItem)
			bEnable = AllowAccess2Vdba(pItem);
	}
	return bEnable;
}

void CvSdaLeft::AccessVdba()
{
	OnPopupAccessVdba();
}

void CvSdaLeft::OnSelchanging(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMTREEVIEW* pNMTreeView = (NMTREEVIEW*)pNMHDR;
	try 
	{
		CfSdaFrame* pFrame = (CfSdaFrame*)GetParentFrame();
		ASSERT (pFrame);
		if (!pFrame)
			return;
		if (!pFrame->IsAllViewCreated())
			return;
		CTreeCtrl& treeCtrl = GetTreeCtrl();

		if (pNMTreeView->itemNew.hItem != NULL)
		{
			CtrfItemData* pItem = (CtrfItemData*)treeCtrl.GetItemData (pNMTreeView->itemNew.hItem);
			if (pItem && pItem->GetTreeItemType() == O_BASE_ITEM)
			{
				*pResult = 1;
				return;
			}
		}
	}
	catch(CMemoryException* e)
	{
		e->ReportError();
		e->Delete();
	}
	catch(CResourceException* e)
	{
		e->ReportError();
		e->Delete();
	}
	catch(...)
	{
	}
	*pResult = 0;
}

void CvSdaLeft::OnPopupAccessVdba() 
{
	CTreeCtrl& cTree = GetTreeCtrl();
	HTREEITEM hSel = cTree.GetSelectedItem();
	if (hSel)
	{
		CtrfItemData* pItem = (CtrfItemData*)cTree.GetItemData (hSel);
		if (pItem)
		{
			VSD_RunVdba(pItem);
		}
	}
}
