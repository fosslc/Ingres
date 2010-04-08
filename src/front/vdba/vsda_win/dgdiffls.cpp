/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dgdiffls.cpp : implementation file
**    Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Page of right pane
**
** History:
**
** 03-Sep-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
** 22-Apr-2003 (schph01)
**    SIR 107523 Add Sequence Object
** 06-Jan-2004 (schph01)
**    SIR #111462, Put string into resource files
** 07-May-2004 (schph01)
**    SIR #112281, set the m_pListCtrl variable before launch the Detailed
**    Differences properties windows.
** 17-May-2004 (uk$so01)
**    SIR #109220, ISSUE 13407277: additional change to put the header
**    in the pop-up for an item's detail of difference.
** 17-Nov-2004 (uk$so01)
**    SIR #113475 (Add new functionality to allow user to save the results 
**    of a comparison into a CSV file.
 ** 22-Nov-2004 (schph01)
 **    BUG #113511 replace CompareNoCase function by CollateNoCase
 **    according to the LC_COLLATE category setting of the locale code
 **    page currently in  use.
** 27-Jan-2004 (komve01)
**    BUG #114336.Issue#13858794.Update the count of Objects with Differences
**	  when we change the node selection. Also ignore the differences when the
**    user does notwant to display it.
** 21-Apr-2004 (komve01)
**    BUG #114336.Issue#13858794.Propagation from main lost the last new line
**    character and closing the function with a brace }.
*/

#include "stdafx.h"
#include "vsda.h"
#include "dgdiffls.h"
#include "calsctrl.h"
#include "vsddml.h"
#include "rctools.h"
#include "sortlist.h"
#include "colorind.h" // UxCreateFont
#include "fminifrm.h"
#include "vsdafrm.h"
#include "ingobdml.h"
#define _DEPT_EDIT_CONTROL_BACKGROUND

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DIFFHEADER_COUNT 5
enum 
{
	DIFF_POS_OBJECT_NAME = 0,
	DIFF_POS_OBJECT_TYPE,
	DIFF_POS_DIFF,
	DIFF_POS_SCHEMA1,
	DIFF_POS_SCHEMA2
};
enum 
{
	DIFF_POS_OBJECT_DIFF = (DIFF_POS_OBJECT_TYPE+1),
	DIFF_POS_SUBBRANCH_DIFF
};
//
// Zero-base index of image list IDB_INGRESOBJECT2_16x16
// in rctools.rc
enum
{
	IMX_DATABASE,               // IDB_INGRESOBJECT2_16x16
	IMX_TABLE,
	IMX_VIEW,
	IMX_PROCEDURE,
	IMX_DBEVENT,
	IMX_SYNONYM,
	IMX_INDEX,
	IMX_RULE,
	IMX_INTEGRITY,
	IMX_COLUMN,
	IMX_SEQUENCE,
	IMX_INSTALLATION,           // IDB_INGRESOBJECT3_16X16
	IMX_USER,
	IMX_GROUP,
	IMX_ROLE,
	IMX_PROFILE,
	IMX_LOCATION,
	IMX_FOLDER,                 // IDB_INGRESOBJECT16x16
	IMX_GRANTEE_INSTALLATION,   // IDB_INGRESOBJECT16x16GRANTEE
	IMX_GRANTEE_DATABASE,
	IMX_GRANTEE_TABLE,
	IMX_GRANTEE_VIEW,
	IMX_GRANTEE_PROCEDURE,
	IMX_GRANTEE_DBEVENT,
	IMX_GRANTEE_SEQUENCE,
	IMX_ALARM_INSTALLATION,     // IDB_INGRESOBJECT16x16ALARM
	IMX_ALARM_DATABASE,
	IMX_ALARM_TABLE,
	IMX_TABLE_NOT_STAR,         // IDB_TABLE
	IMX_TABLE_STAR_LINK,
	IMX_TABLE_STAR_NATIVE,
	IMX_VIEW_NOT_STAR,          // IDB_VIEW
	IMX_VIEW_STAR_LINK,
	IMX_VIEW_STAR_NATIVE,
	IMX_PROCEDURE_NOT_STAR,     // IDB_PROCEDURE
	IMX_PROCEDURE_STAR_LINK,
	IMX_PROCEDURE_STAR_NATIVE,
	IMX_INDEX_NOT_STAR,         // IDB_INDEX
	IMX_INDEX_STAR_LINK,
	IMX_INDEX_STAR_NATIVE
};

class CaItem4Sort: public CObject
{
public:
	CaItem4Sort(){}
	~CaItem4Sort(){}

	DWORD m_dwData;
	CStringArray m_arrayItem;
};


static int CALLBACK CompareSubItem (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	int nC;
	CtrfItemData* lpItem1 = (CtrfItemData*)lParam1;
	CtrfItemData* lpItem2 = (CtrfItemData*)lParam2;
	if (lpItem1 == NULL || lpItem2 == NULL)
		return 0;
	CaVsdItemData* pItem1 = (CaVsdItemData*)lpItem1->GetUserData();
	CaVsdItemData* pItem2 = (CaVsdItemData*)lpItem2->GetUserData();

	if (pItem1 && pItem2)
	{
		SORTPARAMS*   pSr = (SORTPARAMS*)lParamSort;
		BOOL bAsc = pSr? pSr->m_bAsc: TRUE;
		nC = bAsc? 1 : -1;
		switch (pSr->m_nItem)
		{
		case 0: // Object Name:
			{
				CString s1 = GetFullObjectName(lpItem1, TRUE);
				CString s2 = GetFullObjectName(lpItem2, TRUE);
				TRACE2("COM: %s <-> %s\n", s1, s2);
				return nC * s1.CollateNoCase (s2);
			}
			break;
		case 1: // Object Type (by icon)
			{
				CString strKey1, strKey2;
				int nObjectID1 = lpItem1->GetDBObject()->GetObjectID();
				int nObjectID2 = lpItem2->GetDBObject()->GetObjectID();
				strKey1.Format (_T("%02d"), nObjectID1);
				strKey2.Format (_T("%02d"), nObjectID2);

				if (nObjectID1 == OBT_GRANTEE)
				{
					CaLLQueryInfo* pQueryInfo = lpItem1->GetQueryInfo(NULL);
					if (pQueryInfo)
					{
						switch (pQueryInfo->GetSubObjectType())
						{
						case OBT_INSTALLATION:
						case OBT_DATABASE:
						case OBT_TABLE:
						case OBT_VIEW:
						case OBT_PROCEDURE:
						case OBT_SEQUENCE:
						case OBT_DBEVENT:
						case OBT_INDEX:
							strKey1.Format (_T("%02d%02d"), nObjectID1, pQueryInfo->GetSubObjectType());
							break;
						default:
							break;
						}
					}
				}

				if (nObjectID2 == OBT_GRANTEE)
				{
					CaLLQueryInfo* pQueryInfo = lpItem2->GetQueryInfo(NULL);
					if (pQueryInfo)
					{
						switch (pQueryInfo->GetSubObjectType())
						{
						case OBT_INSTALLATION:
						case OBT_DATABASE:
						case OBT_TABLE:
						case OBT_VIEW:
						case OBT_PROCEDURE:
						case OBT_SEQUENCE:
						case OBT_DBEVENT:
						case OBT_INDEX:
							strKey2.Format (_T("%02d%02d"), nObjectID2, pQueryInfo->GetSubObjectType());
							break;
						default:
							break;
						}
					}
				}

				return nC * strKey1.CollateNoCase (strKey2);
			}
			break;
		case 2:
		case 3:
		case 4:
		default:
			ASSERT(FALSE); // must use CompareSubItem2
			break;
		}
	}

	return 0;
}


static int CALLBACK CompareSubItem2 (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	CaItem4Sort* lpItem1 = (CaItem4Sort*)lParam1;
	CaItem4Sort* lpItem2 = (CaItem4Sort*)lParam2;
	if (lpItem1 == NULL || lpItem2 == NULL)
		return 0;
	SORTPARAMS*   pSr = (SORTPARAMS*)lParamSort;
	BOOL bAsc = pSr? pSr->m_bAsc: TRUE;
	int nC = bAsc? 1 : -1;
	switch (pSr->m_nItem)
	{
	case 0: // Object Name:
	case 1: // Object Type (by icon)
		break;
	default:
		if (pSr->m_nItem > 0 && pSr->m_nItem < min (lpItem1->m_arrayItem.GetSize(), lpItem2->m_arrayItem.GetSize()))
		{
			CString s1 = lpItem1->m_arrayItem[pSr->m_nItem];
			CString s2 = lpItem2->m_arrayItem[pSr->m_nItem];
			return nC * s1.CollateNoCase (s2);
		}
	}

	return 0;
}

void QueryImageID(CtrfItemData* pItem, int& nImage, CString& strObjectType)
{
	int nSubType;
	CaLLQueryInfo* pQueryInfo = NULL;
	int nObjectType = pItem->GetDBObject()->GetObjectID();

	switch (nObjectType)
	{
	case OBT_INSTALLATION: 
		nImage = IMX_INSTALLATION;
		strObjectType = _T("Installation");
		break;
	case OBT_DATABASE: 
		nImage = IMX_DATABASE;
		strObjectType = _T("Database");
		break;
	case OBT_TABLE:
		{
		CaVsdTable* vtable = (CaVsdTable*) pItem;
		CaTable& table = vtable->GetObject();

		nSubType = table.GetFlag();
		if(nSubType == OBJTYPE_STARNATIVE)
			nImage = IMX_TABLE_STAR_NATIVE;
		else if(nSubType == OBJTYPE_STARLINK)
			nImage = IMX_TABLE_STAR_LINK;
		else
			nImage = IMX_TABLE;
		strObjectType = _T("Table");
		}
		break;
	case OBT_VIEW:
		{
		CaVsdView* vView = (CaVsdView*) pItem;
		CaView& view = vView->GetObject();
		nSubType = view.GetFlag();
		if(nSubType == OBJTYPE_STARNATIVE)
			nImage = IMX_VIEW_STAR_NATIVE;
		else if(nSubType == OBJTYPE_STARLINK)
			nImage = IMX_VIEW_STAR_LINK;
		else
			nImage = IMX_VIEW;
		strObjectType = _T("View");
		}
		break;
	case OBT_PROCEDURE:
		{
		CaVsdProcedure* vProcedure = (CaVsdProcedure*) pItem;
		CaProcedure& Procedure = vProcedure->GetObject();
		nSubType = Procedure.GetFlag();
		if(nSubType == OBJTYPE_STARLINK)
			nImage = IMX_PROCEDURE_STAR_LINK;
		else
			nImage = IMX_PROCEDURE;
		strObjectType = _T("Procedure");
		}
		break;
	case OBT_SEQUENCE:
		{
		nImage = IMX_SEQUENCE;
		strObjectType = _T("Sequence");
		}
		break;
	case OBT_DBEVENT:
		nImage = IMX_DBEVENT;
		strObjectType = _T("Database Event");
		break;
	case OBT_SYNONYM:
		nImage = IMX_SYNONYM;
		strObjectType = _T("Synonym");
		break;
	case OBT_GRANTEE:
		pQueryInfo = pItem->GetQueryInfo(NULL);
		ASSERT(pQueryInfo);
		if (pQueryInfo)
		{
			switch (pQueryInfo->GetSubObjectType())
			{
			case OBT_INSTALLATION:
				nImage = IMX_GRANTEE_INSTALLATION;
				strObjectType = _T("Installation Grantee");
				break;
			case OBT_DATABASE:
				nImage = IMX_GRANTEE_DATABASE;
				strObjectType = _T("Database Grantee");
				break;
			case OBT_TABLE:
				nImage = IMX_GRANTEE_TABLE;
				strObjectType = _T("Table Grantee");
				break;
			case OBT_VIEW:
				nImage = IMX_GRANTEE_VIEW;
				strObjectType = _T("View Grantee");
				break;
			case OBT_PROCEDURE:
				nImage = IMX_GRANTEE_PROCEDURE;
				strObjectType = _T("Procedure Grantee");
				break;
			case OBT_SEQUENCE:
				nImage = IMX_GRANTEE_SEQUENCE;
				strObjectType = _T("Sequence Grantee");
				break;
			case OBT_DBEVENT:
				nImage = IMX_GRANTEE_DBEVENT;
				strObjectType = _T("Database Event Grantee");
				break;
			case OBT_INDEX:
				ASSERT(FALSE);
				nImage = IMX_USER;
				strObjectType = _T("Index Grantee");
				break;
			default:
				ASSERT(FALSE);
				nImage = IMX_USER;
				strObjectType = _T("Grantee");
				break;
			}
		}
		break;
	case OBT_INDEX:
		{
		CaVsdIndex* vIndex = (CaVsdIndex*) pItem;
		CaIndex& Index = vIndex->GetObject();
		nSubType = Index.GetFlag();
		if(nSubType == OBJTYPE_STARLINK)
			nImage = IMX_INDEX_STAR_LINK;
		else if(nSubType == OBJTYPE_STARLINK)
			nImage = IMX_INDEX_STAR_NATIVE;
		else
			nImage = IMX_INDEX;
		strObjectType = _T("Index");
		}
		break;
	case OBT_INTEGRITY:
		nImage = IMX_INTEGRITY;
		strObjectType = _T("Integrity");
		break;
	case OBT_RULE:
		nImage = IMX_RULE;
		strObjectType = _T("Rule");
		break;
	case OBT_ALARM:
		pQueryInfo = pItem->GetQueryInfo(NULL);
		ASSERT(pQueryInfo);
		if (pQueryInfo)
		{
			switch (pQueryInfo->GetSubObjectType())
			{
			case OBT_INSTALLATION:
				nImage = IMX_ALARM_INSTALLATION;
				strObjectType = _T("Installation Security Alarm");
				break;
			case OBT_DATABASE:
				nImage = IMX_ALARM_DATABASE;
				strObjectType = _T("Database Security Alarm");
				break;
			case OBT_TABLE:
				nImage = IMX_ALARM_TABLE;
				strObjectType = _T("Table Security Alarm");
				break;
			default:
				ASSERT(FALSE);
				nImage = -1;
				strObjectType = _T("Security Alarm");
				break;
			}
		}
		break;
	case OBT_USER:
		nImage = IMX_USER;
		strObjectType = _T("User");
		break;
	case OBT_GROUP:
		nImage = IMX_GROUP;
		strObjectType = _T("Group");
		break;
	case OBT_ROLE:
		nImage = IMX_ROLE;
		strObjectType = _T("Role");
		break;
	case OBT_PROFILE:
		nImage = IMX_PROFILE;
		strObjectType = _T("Profile");
		break;
	case OBT_LOCATION: 
		nImage = IMX_LOCATION;
		strObjectType = _T("Location");
		break;
	default:
		if (pItem->IsFolder())
		{
			strObjectType = pItem->GetDisplayedItem();
			nImage = IMX_FOLDER;
		}
		else
		{
			nImage = -1;
			strObjectType = _T("???");
		}
		break;
	}
}

BOOL QueryOwner(int nObjectType)
{
	switch (nObjectType)
	{
	case OBT_DATABASE:
	case OBT_TABLE:
	case OBT_VIEW:
	case OBT_INDEX:
	case OBT_INTEGRITY:
	case OBT_PROCEDURE:
	case OBT_SEQUENCE:
	case OBT_RULE:
	case OBT_SYNONYM:
		return TRUE;
	default:
		return FALSE;
	}
}

//
// If you call with 'bCountContent' = FALSE
// The differences related to the 'pItem' will not counted. 
// Only the sub-folders of 'pItem' will be taken into account.
static void CountDifferencesInSubFolderOf(CtrfItemData* pItem, int& nCount, BOOL bCountContent)
{
	if (bCountContent)
	{
		CaVsdItemData* pVsdItemData = (CaVsdItemData*)pItem->GetUserData();
		if (pVsdItemData->GetDifference() == _T('+') || pVsdItemData->GetDifference() == _T('-'))
		{
			nCount++;
		}
		else
		if (!pItem->IsFolder())
		{
			CTypedPtrList< CObList, CaVsdDifference* >& listDiff = pVsdItemData->GetListDifference();
			nCount += listDiff.GetCount();
		}
	}

	CTypedPtrList< CObList, CtrfItemData* >& lo = pItem->GetListObject();
	POSITION pos = lo.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfItemData* pIt = lo.GetNext(pos);
		CaVsdItemData* pVsdItemData = (CaVsdItemData*)pIt->GetUserData();
		CountDifferencesInSubFolderOf(pIt, nCount, TRUE);
	}
}

//
// Only count from the selected item 'pItem':
static void CountObjectsThatHaveDifferences(CtrfItemData* pItem, int& nCount, int nDepth)
{
	CaVsdItemData* pVsdItemData = (CaVsdItemData*)pItem->GetUserData();
	if (pVsdItemData->GetDifference() == _T('+') || pVsdItemData->GetDifference() == _T('-'))
	{
		nCount++;
	}
	else
	{
		if (!pItem->IsFolder())
		{
			CTypedPtrList< CObList, CaVsdDifference* >& listDiff = pVsdItemData->GetListDifference();
			if (!listDiff.IsEmpty())
				nCount += 1;
		}

		if (nDepth > 0)
		{
			CTypedPtrList< CObList, CtrfItemData* >& lo = pItem->GetListObject();
			POSITION pos = lo.GetHeadPosition();
			while (pos != NULL)
			{
				CtrfItemData* pIt = lo.GetNext(pos);
				CaVsdItemData* pVsdItemData = (CaVsdItemData*)pIt->GetUserData();
				if(!IgnoreItem(pIt)) // If the item is ignored/not displayed don't go
				{				     // any further down the line.
					if (pIt->IsFolder())
						CountObjectsThatHaveDifferences(pIt, nCount, nDepth);
					else
						CountObjectsThatHaveDifferences(pIt, nCount, nDepth - 1);
				}
			}
		}
	}
}


CString GetFullObjectName(CtrfItemData* pItem, BOOL bKey4Sort)
{
	CString strItem;
	CtrfItemData* pBackParent = pItem->GetBackParent();
	CaVsdItemData* pVsdItemData = (CaVsdItemData*)pItem->GetUserData();
	CaLLQueryInfo* pInfo = pItem->GetQueryInfo(NULL);
	CaVsdDisplayInfo* pDisplayInfo = (CaVsdDisplayInfo*)pItem->GetDisplayInfo();
	CaVsdRefreshTreeInfo* pRefresh = pDisplayInfo->GetRefreshTreeInfo();
	BOOL bPrefixDatabase = (pRefresh->GetDatabase1().IsEmpty() && pRefresh->GetDatabase2().IsEmpty());
	int nObjectType = pItem->GetDBObject()->GetObjectID();
	CString strDisplayName = pItem->GetDisplayedItem(bKey4Sort? 1: 0);

	switch (nObjectType)
	{
	case OBT_TABLE:
	case OBT_VIEW:
	case OBT_PROCEDURE:
	case OBT_SEQUENCE:
	case OBT_DBEVENT:
	case OBT_SYNONYM:
		if (bPrefixDatabase) 
			strItem.Format(_T("%s / %s"), (LPCTSTR)pInfo->GetDatabase(), (LPCTSTR)strDisplayName);
		else
			strItem = strDisplayName;
		break;
	case OBT_GRANTEE:
		switch (pInfo->GetSubObjectType())
		{
		case OBT_INSTALLATION:
			strItem = strDisplayName;
			break;
		case OBT_DATABASE:
			if (bPrefixDatabase) 
				strItem.Format(_T("%s / %s"), (LPCTSTR)pInfo->GetDatabase(), (LPCTSTR)strDisplayName);
			else
				strItem = strDisplayName;
			break;
		case OBT_TABLE:
		case OBT_VIEW:
		case OBT_PROCEDURE:
		case OBT_SEQUENCE:
		case OBT_DBEVENT:
		case OBT_SYNONYM:
			ASSERT(pBackParent);
			while (pBackParent && pBackParent->IsFolder())
				pBackParent = pBackParent->GetBackParent();
			ASSERT(pBackParent);
			if (pBackParent)
			{
				if (bPrefixDatabase) 
					strItem.Format(_T("%s / %s / %s"), (LPCTSTR)pInfo->GetDatabase(), (LPCTSTR)pBackParent->GetDisplayedItem(), (LPCTSTR)strDisplayName);
				else
					strItem.Format(_T("%s / %s"), (LPCTSTR)pBackParent->GetDisplayedItem(), (LPCTSTR)strDisplayName);
			}
			break;
		default:
			strItem = strDisplayName;
			break;
		}
		break;
	case OBT_ALARM:
		switch (pInfo->GetSubObjectType())
		{
		case OBT_INSTALLATION:
			strItem = strDisplayName;
			break;
		case OBT_DATABASE:
			if (bPrefixDatabase) 
				strItem.Format(_T("%s / %s"), (LPCTSTR)pInfo->GetDatabase(), (LPCTSTR)strDisplayName);
			else
				strItem = strDisplayName;
			break;
		case OBT_TABLE:
			ASSERT(pBackParent);
			while (pBackParent && pBackParent->IsFolder())
				pBackParent = pBackParent->GetBackParent();
			ASSERT(pBackParent);
			if (pBackParent)
			{
				if (bPrefixDatabase) 
					strItem.Format(_T("%s / %s / %s"), (LPCTSTR)pInfo->GetDatabase(), (LPCTSTR)pBackParent->GetDisplayedItem(), (LPCTSTR)strDisplayName);
				else
					strItem.Format(_T("%s / %s"), (LPCTSTR)pBackParent->GetDisplayedItem(), (LPCTSTR)strDisplayName);
			}
			break;
		default:
			strItem = strDisplayName;
			break;
		}
		break;
	case OBT_INDEX:
	case OBT_INTEGRITY:
	case OBT_RULE:
		strDisplayName = pVsdItemData->GetName();
		if (bPrefixDatabase) 
			strItem.Format(_T("%s / %s"), (LPCTSTR)pInfo->GetDatabase(), (LPCTSTR)strDisplayName);
		else
			strItem = strDisplayName;
		break;
	default:
		strItem = strDisplayName;
		break;
	}
	if (bKey4Sort)
	{
		switch (pInfo->GetSubObjectType())
		{
		case OBT_TABLE:
		case OBT_VIEW:
		case OBT_PROCEDURE:
		case OBT_SEQUENCE:
		case OBT_DBEVENT:
		case OBT_SYNONYM:
			strDisplayName.Format(_T("%02d%2d %s"), nObjectType, pInfo->GetSubObjectType(), (LPCTSTR)strItem);
			break;
		default:
			strDisplayName.Format(_T("%02d00 %s"), nObjectType, (LPCTSTR)strItem);
			break;
		}
		return strDisplayName;
	}
	else
		return strItem;
}


/////////////////////////////////////////////////////////////////////////////
// CuDlgPageDifferentList dialog


CuDlgPageDifferentList::CuDlgPageDifferentList(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgPageDifferentList::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgPageDifferentList)
	m_bActionGroupByObject = FALSE;
	m_strDepth = _T("1");
	//}}AFX_DATA_INIT
	m_InitGroupBy = FALSE; // By default (the list is not a grouped by objects)
	m_pCurrentTreeItem = NULL;
	m_bDisplaySummarySubBranches = TRUE;

	m_rgbBkColor = GetSysColor(COLOR_WINDOW);
	m_rgbFgColor = GetSysColor(COLOR_WINDOWTEXT);
	m_pEditBkBrush = new CBrush(m_rgbBkColor);

	m_sortparam.m_nItem = 1;
	m_sortparam.m_bAsc  = TRUE;

	m_bCreateDiffFont = FALSE;
	m_pPropFrame = NULL;
	m_pDetailDifference = new CuDlgDifferenceDetail();
}


void CuDlgPageDifferentList::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgPageDifferentList)
	DDX_Control(pDX, IDC_CHECK1, m_cCheckGroup);
	DDX_Control(pDX, IDC_STATIC_DEPT2, m_cStaticDepth2);
	DDX_Control(pDX, IDC_STATIC_DEPT1, m_cStaticDepth1);
	DDX_Control(pDX, IDC_STATIC_NODIFFERENCE, m_cStaticNoDifference);
	DDX_Control(pDX, IDC_STATIC_DIFF_COUNT, m_cStaticDifferenceCount);
	DDX_Control(pDX, IDC_EDIT1, m_cEditDepth);
	DDX_Control(pDX, IDC_SPIN1, m_cSpinDepth);
	DDX_Check(pDX, IDC_CHECK1, m_bActionGroupByObject);
	DDX_Text(pDX, IDC_EDIT1, m_strDepth);
	//}}AFX_DATA_MAP
//	DDX_Control(pDX, IDC_LIST1, m_cListCtrl);
}


BEGIN_MESSAGE_MAP(CuDlgPageDifferentList, CDialog)
	//{{AFX_MSG_MAP(CuDlgPageDifferentList)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_CHECK1, OnCheckGroupByObject)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN1, OnDeltaposSpinDepth)
	ON_EN_CHANGE(IDC_EDIT1, OnChangeEditDepth)
	ON_WM_CTLCOLOR()
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST1, OnColumnclickList1)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST1, OnDblclkList1)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, OnItemchangedList1)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_UPDATEDATA, OnUpdateData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgPageDifferentList message handlers

void CuDlgPageDifferentList::PostNcDestroy() 
{
	if (m_pEditBkBrush)
		delete m_pEditBkBrush;
	if (m_pDetailDifference)
		delete m_pDetailDifference;

	delete this;
	CDialog::PostNcDestroy();
}

void CuDlgPageDifferentList::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cListCtrl.m_hWnd))
		return;
	int h = 0;
	CRect rDlg, r, r2;
	GetClientRect (rDlg);

	m_cStaticSelectedItem.GetWindowRect(r);
	ScreenToClient (r);
	r.top   = rDlg.top;
	r.left  = rDlg.left;
	r.right = rDlg.right;
	m_cStaticSelectedItem.MoveWindow(r);

	m_cStaticDifferenceCount.GetWindowRect(r);
	ScreenToClient (r);
	h = r.Height();
	r = rDlg;
	r.top = rDlg.bottom - h;
	m_cStaticDifferenceCount.MoveWindow(r);

	m_cListCtrl.GetWindowRect(r);
	ScreenToClient (r);
	rDlg.top = r.top;
	rDlg.bottom -= (h +2);
	m_cListCtrl.MoveWindow(rDlg);

	m_cStaticSelectedItem.GetWindowRect (r);
	ScreenToClient (r);
	int nY1 = r.bottom;
	m_cStaticDifferenceCount.GetWindowRect(r);
	ScreenToClient (r);
	int nY2 = r.bottom;
	m_cListCtrl.GetWindowRect(r);
	m_cStaticNoDifference.GetWindowRect(r2);
	ScreenToClient (r);
	ScreenToClient (r2);
	
	r2.left  = r.left;
	r2.right = r.right;
	r2.top   = nY1;
	r2.bottom= nY2;
	m_cStaticNoDifference.MoveWindow(r2);
}

void CuDlgPageDifferentList::ShowNoDifference()
{
	int i;
	const int ctrlCount = 7;
	CWnd* pArrayWnd[ctrlCount] =
	{
		&m_cListCtrl, 
		&m_cStaticDepth1, 
		&m_cStaticDepth2, 
		&m_cEditDepth, 
		&m_cSpinDepth, 
		&m_cCheckGroup,
		&m_cStaticDifferenceCount
	};
	if (m_cListCtrl.GetItemCount() == 0)
	{
		for (i=0; i<ctrlCount; i++)
			pArrayWnd[i]->ShowWindow (SW_HIDE);
		m_cStaticNoDifference.ShowWindow (SW_SHOW);

		if (m_pPropFrame && m_pPropFrame->IsWindowVisible())
		{
			m_pPropFrame->RemoveAllPages();
			m_pPropFrame->HandleData(0);
		}
	}
	else
	{
		for (i=0; i<ctrlCount; i++)
			pArrayWnd[i]->ShowWindow (SW_SHOW);
		m_cStaticNoDifference.ShowWindow (SW_HIDE);
	}
}

void CuDlgPageDifferentList::InitColumns(CtrfItemData* pItem)
{
	LSCTRLHEADERPARAMS2 lsp[DIFFHEADER_COUNT] =
	{
		{IDS_TAB_OBJECT,       75,  LVCFMT_LEFT, FALSE},
		{IDS_TAB_TYPE,         75,  LVCFMT_LEFT, FALSE},
		{IDS_TAB_DIFF,        150,  LVCFMT_LEFT, FALSE},
		{IDS_TAB_SCHEMA1,     150,  LVCFMT_LEFT, FALSE},
		{IDS_TAB_SCHEMA2,     150,  LVCFMT_LEFT, FALSE}
	};

	LSCTRLHEADERPARAMS2 lspgroup[4] =
	{
		{IDS_TAB_OBJECT,       75,  LVCFMT_LEFT,  FALSE},
		{IDS_TAB_TYPE,         75,  LVCFMT_LEFT,  FALSE},
		{IDS_TAB_GROUP_DIFF1, 150,  LVCFMT_RIGHT, FALSE},
		{IDS_TAB_GROUP_DIFF2, 200,  LVCFMT_RIGHT, FALSE},
	};

	while (m_cListCtrl.DeleteColumn(0));
	if (m_bActionGroupByObject)
	{
		InitializeHeader2(&m_cListCtrl, LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH, lspgroup, 4);
	}
	else
	{
		InitializeHeader2(&m_cListCtrl, LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH, lsp, DIFFHEADER_COUNT);
	}
	m_InitGroupBy = m_bActionGroupByObject;
}



BOOL CuDlgPageDifferentList::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_cListCtrl.SetFullRowSelected(TRUE);
	m_cListCtrl.SetLineSeperator(FALSE, FALSE, FALSE);
	m_cListCtrl.SetAllowSelect(TRUE);
	VERIFY (m_cStaticSelectedItem.SubclassDlgItem (IDC_STATIC_SELECTED_ITEM, this));
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST1, this));

	m_cStaticSelectedItem.SetImage (-1); // No Image for Now
	m_cStaticSelectedItem.SetImageLeftMagin(1);
	
	InitColumns(NULL);
	m_cSpinDepth.SetRange (1, 3);
	m_ImageList.Create (16, 16, TRUE, 18, 8);
	int i =0;
	CImageList imageList;
	HICON hIcon;
	imageList.Create (IDB_INGRESOBJECT2_16x16, 16, 0, RGB(255,0,255));
	for (i = 0; i <10; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}

	hIcon = imageList.ExtractIcon (11); // Sequence
	m_ImageList.Add (hIcon);
	DestroyIcon (hIcon);

	imageList.DeleteImageList();
	imageList.Create (IDB_INGRESOBJECT3_16x16, 16, 0, RGB(255,0,255));
	for (i = 0; i <6; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	imageList.DeleteImageList();
	imageList.Create (IDB_INGRESOBJECT16x16, 16, 0, RGB(255,0,255));
	for (i = 0; i <1; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	imageList.DeleteImageList();
	imageList.Create (IDB_INGRESOBJECT16x16GRANTEE, 16, 0, RGB(255,0,255));
	for (i = 0; i <7; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	imageList.DeleteImageList();
	imageList.Create (IDB_INGRESOBJECT16x16ALARM, 16, 0, RGB(255,0,255));
	for (i = 0; i <3; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	imageList.DeleteImageList();
	imageList.Create (IDB_TABLE, 16, 0, RGB(255,0,255));
	for (i = 0; i <3; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}
	imageList.DeleteImageList();
	imageList.Create (IDB_VIEW, 16, 0, RGB(255,0,255));
	for (i = 0; i <3; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}

	imageList.DeleteImageList();
	imageList.Create (IDB_PROCEDURE, 16, 0, RGB(255,0,255));
	for (i = 0; i <3; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}

	imageList.DeleteImageList();
	imageList.Create (IDB_INDEX, 16, 0, RGB(255,0,255));
	for (i = 0; i <3; i++)
	{
		hIcon = imageList.ExtractIcon (i);
		m_ImageList.Add (hIcon);
		DestroyIcon (hIcon);
	}

	imageList.DeleteImageList();
	m_cListCtrl.SetImageList (&m_ImageList, LVSIL_SMALL);
	ShowNoDifference();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

static int GetItemDepth(CtrfItemData* pItem)
{
	TreeItemType nType = pItem->GetTreeItemType();
	switch (nType)
	{
	case O_FOLDER_DATABASE: return 1;
	case O_FOLDER_PROFILE: return 1;
	case O_FOLDER_USER: return 1;
	case O_FOLDER_GROUP: return 1;
	case O_FOLDER_ROLE: return 1;
	case O_FOLDER_LOCATION: return 1;

	case O_FOLDER_TABLE: return 2;
	case O_FOLDER_VIEW: return 2;
	case O_FOLDER_PROCEDURE: return 2;
	case O_FOLDER_SEQUENCE: return 2;
	case O_FOLDER_DBEVENT: return 2;
	case O_FOLDER_SYNONYM: return 2;

	case O_FOLDER_INDEX: return 3;
	case O_FOLDER_RULE: return 3;
	case O_FOLDER_INTEGRITY: return 3;

	case O_DATABASE: return 1;
	case O_PROFILE: return 1;
	case O_USER: return 1;
	case O_GROUP: return 1;
	case O_ROLE: return 1;
	case O_LOCATION: return 1;

	case O_TABLE: return 2;
	case O_VIEW: return 2;
	case O_PROCEDURE: return 2;
	case O_SEQUENCE: return 2;
	case O_DBEVENT: return 2;
	case O_SYNONYM: return 2;

	case O_INDEX: return 3;
	case O_RULE: return 3;
	case O_INTEGRITY: return 3;
	default:
		return 0;
	}
}

BOOL IgnoreItem(CtrfItemData* pItem)
{
	if (!pItem->DisplayItem())
		return TRUE;
	CaVsdItemData* pVsdItemData = (CaVsdItemData*)pItem->GetUserData();
	ASSERT(pVsdItemData);
	if (pVsdItemData && pVsdItemData->IsDiscarded())
		return TRUE;
	return FALSE;
}


long CuDlgPageDifferentList::OnUpdateData(WPARAM wParam, LPARAM lParam)
{
	CString strCount;
	int nDepth = 1, nObjThatHasDiffs = 0;
	UpdateData(TRUE);
	m_cListCtrl.DeleteAllItems();
	CtrfItemData* pItem = (CtrfItemData*)lParam;
	if (pItem && pItem->GetTreeItemType() != O_BASE_ITEM)
	{
		m_pCurrentTreeItem = pItem;
		if (m_InitGroupBy != m_bActionGroupByObject)
			InitColumns(pItem);
		if (!m_bActionGroupByObject)
		{
			CaVsdDisplayInfo* pDisplayInfo = (CaVsdDisplayInfo*)pItem->GetDisplayInfo();
			ASSERT(pDisplayInfo);
			if (pDisplayInfo)
			{
				LVCOLUMN vc;
				memset (&vc, 0, sizeof (vc));
				vc.mask = LVCF_TEXT;
				vc.pszText = (LPTSTR)(LPCTSTR)pDisplayInfo->GetCompareTitle1();
				m_cListCtrl.SetColumn(DIFF_POS_SCHEMA1, &vc);
				vc.pszText = (LPTSTR)(LPCTSTR)pDisplayInfo->GetCompareTitle2();
				m_cListCtrl.SetColumn(DIFF_POS_SCHEMA2, &vc);
			}
		}

		if (wParam)
		{
			NMUPDOWN* pNMUpDown = (NMUPDOWN*)wParam;
			nDepth = pNMUpDown->iPos + pNMUpDown->iDelta;
		}
		else
		{
			nDepth = _ttoi(m_strDepth);
		}

		int nImage = -1;
		CString strObjectType;
		QueryImageID(pItem, nImage, strObjectType);
		HICON hIcon = m_ImageList.ExtractIcon(nImage);
		m_cStaticSelectedItem.SetWindowText(pItem->GetDisplayedItem());
		m_cStaticSelectedItem.ResetBitmap (hIcon);
		UINT uFlags = SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER;
		m_cStaticSelectedItem.SetWindowPos(NULL, 0, 0, 0, 0, uFlags);
		DestroyIcon(hIcon);

		Display(pItem, (nDepth==0)? 1: nDepth);
		CountObjectsThatHaveDifferences(pItem, nObjThatHasDiffs, (nDepth==0)? 1: nDepth);

		SortDifferenceItems(m_sortparam.m_nItem);
		TRACE2("CuDlgPageDifferentList::OnUpdateData (depth = %d, group by object = %d)\n", nDepth, m_bActionGroupByObject);

		strCount.Format(IDS_MSG_OBJ_DIFF, nObjThatHasDiffs, m_cListCtrl.GetItemCount());
		m_cStaticDifferenceCount.SetWindowText(strCount);
	}
	if (m_pPropFrame)
	{
		CString strMsg;
		strMsg.LoadString(IDS_NO_SELECTED_ITEM4DETAIL); // _T("No Selected Detail Line.")
		m_pPropFrame->SetNoPageMessage(strMsg);
		m_pPropFrame->RemoveAllPages();
		m_pPropFrame->HandleData(0);
	}
	ShowNoDifference();
	return 0;
}

void CuDlgPageDifferentList::Display (CtrfItemData* pItem, int nDepth)
{
	if (nDepth < 0)
		return;
	if (IgnoreItem(pItem)) // Not displayed or Discarded Item
		return;

	if (!pItem->IsFolder())
	{
		CaVsdItemData* pVsdItemData = (CaVsdItemData*)pItem->GetUserData();
		//
		// Display the differences related to the object:
		DisplayItem(pVsdItemData, pItem, (nDepth == 0));
		//
		// Display the differences related to the object's sub-branches:
		if (nDepth > 0)
		{
			CTypedPtrList< CObList, CtrfItemData* >& lo = pItem->GetListObject();
			POSITION pos = lo.GetHeadPosition();
			while (pos != NULL)
			{
				CtrfItemData* pIt = lo.GetNext(pos);
				Display (pIt, pIt->IsFolder()? nDepth: nDepth - 1);
			}
		}
	}
	else
	{
		CTypedPtrList< CObList, CtrfItemData* >& lo = pItem->GetListObject();
		POSITION pos = lo.GetHeadPosition();
		while (pos != NULL)
		{
			CtrfItemData* pIt = lo.GetNext(pos);
			CaVsdItemData* pVsdItemData = (CaVsdItemData*)pIt->GetUserData();
			if (pVsdItemData->GetDifference() == _T('+') || pVsdItemData->GetDifference() == _T('-'))
			{
				DisplayItem(pVsdItemData, pIt, (nDepth == 0));
			}
			else
			{
				if (nDepth > 0)
					Display (pIt, nDepth-1);
				else
					DisplayItem(pVsdItemData, pIt, TRUE);
			}
		}
	}
}

void CuDlgPageDifferentList::DisplayItem (CaVsdItemData* pVsdItemData, CtrfItemData* pItem, BOOL bShowSummary)
{
	if(pVsdItemData->IsDiscarded())
	{
		//Even if the parent is not discarded and we are discarded
		//do not display us
		return;
	}
	int nObjectType = pItem->GetDBObject()->GetObjectID();
	CString strObjectType = _T("");
	int nImage = -1;
	QueryImageID(pItem, nImage, strObjectType);
	CString strName = GetFullObjectName(pItem);

	if (pVsdItemData->GetDifference() == _T('+') || pVsdItemData->GetDifference() == _T('-'))
	{
		CString csTemp;
		csTemp.LoadString(IDS_AVAILABLE);
		BOOL bQueryOwner = QueryOwner(nObjectType);
		CString strAvailable = bQueryOwner? pVsdItemData->GetOwner(): csTemp;

		int nIdx = m_cListCtrl.InsertItem (m_cListCtrl.GetItemCount(), strName, nImage);
		if (nIdx != -1)
		{
			m_cListCtrl.SetItemText(nIdx, DIFF_POS_OBJECT_TYPE, strObjectType);
			m_cListCtrl.SetItemData(nIdx, (DWORD)pItem);
			if (m_bActionGroupByObject) // Group by Objects
			{
				m_cListCtrl.SetItemText(nIdx, DIFF_POS_OBJECT_DIFF, _T("1"));
				m_cListCtrl.SetItemText(nIdx, DIFF_POS_SUBBRANCH_DIFF, _T("0"));
			}
			else // Detail list
			{
				CString csMissingObj,csNotAvailable;
				csNotAvailable.LoadString(IDS_NOT_AVAILABLE);
				csMissingObj.LoadString(IDS_MISSING_OBJ);
				if (pVsdItemData->GetDifference() == _T('+'))
				{
					m_cListCtrl.SetItemText(nIdx, DIFF_POS_DIFF,   csMissingObj);
					m_cListCtrl.SetItemText(nIdx, DIFF_POS_SCHEMA1, strAvailable);
					m_cListCtrl.SetItemText(nIdx, DIFF_POS_SCHEMA2, csNotAvailable);
				}
				else
				if (pVsdItemData->GetDifference() == _T('-'))
				{
					m_cListCtrl.SetItemText(nIdx, DIFF_POS_DIFF, csMissingObj);
					m_cListCtrl.SetItemText(nIdx, DIFF_POS_SCHEMA1, csNotAvailable);
					m_cListCtrl.SetItemText(nIdx, DIFF_POS_SCHEMA2, strAvailable);
				}
			}
		}
	}
	else
	if (pVsdItemData->GetDifference() == _T('#') || pVsdItemData->GetDifference() == _T('*'))
	{
		CTypedPtrList< CObList, CaVsdDifference* >& listDiff = pVsdItemData->GetListDifference();
		if (m_bActionGroupByObject) // Group by Objects
		{
			int nCount = 0;
			CountDifferencesInSubFolderOf(pItem, nCount, FALSE);
			if ((listDiff.GetCount() + nCount) > 0)
			{
				int nIdx = m_cListCtrl.InsertItem (m_cListCtrl.GetItemCount(),strName , nImage);
				if (nIdx != -1)
				{
					m_cListCtrl.SetItemData(nIdx, (DWORD)pItem);
					m_cListCtrl.SetItemText(nIdx, DIFF_POS_OBJECT_TYPE, strObjectType);

					CString strCount;
					strCount.Format(_T("%d"), listDiff.GetCount());
					m_cListCtrl.SetItemText(nIdx, DIFF_POS_OBJECT_DIFF, strCount);
					strCount.Format(_T("%d"), nCount);
					m_cListCtrl.SetItemText(nIdx, DIFF_POS_SUBBRANCH_DIFF, strCount);
				}
			}
		}
		else // Detail list
		{
			int nIdx = -1;
			POSITION pos = listDiff.GetHeadPosition();
			while (pos != NULL)
			{
				CaVsdDifference* pDiff = listDiff.GetNext(pos);
				nIdx = m_cListCtrl.InsertItem (m_cListCtrl.GetItemCount(), strName, nImage);
				if (nIdx != -1)
				{
					m_cListCtrl.SetItemData(nIdx, (DWORD)pItem);
					m_cListCtrl.SetItemText(nIdx, DIFF_POS_OBJECT_TYPE, strObjectType);
					m_cListCtrl.SetItemText(nIdx, DIFF_POS_DIFF,        pDiff->GetType());
					m_cListCtrl.SetItemText(nIdx, DIFF_POS_SCHEMA1,     pDiff->GetInfoSchema1());
					m_cListCtrl.SetItemText(nIdx, DIFF_POS_SCHEMA2,     pDiff->GetInfoSchema2());
				}
			}
			//
			// Display the summary of sub-branches:
			if (m_bDisplaySummarySubBranches && bShowSummary)
			{
				int nCount = 0;
				CountDifferencesInSubFolderOf(pItem, nCount, FALSE);
				if (nCount > 0)
				{
					nIdx = m_cListCtrl.InsertItem (m_cListCtrl.GetItemCount(), strName, nImage);
					if (nIdx != -1)
					{
						CString strMsg;
						strMsg.LoadString(IDS_DIFFERENCES_IN_SUBBRANCHES); // _T("<Difference(s) in sub-branches>")

						m_cListCtrl.SetItemData(nIdx, (DWORD)pItem);
						m_cListCtrl.SetItemText(nIdx, DIFF_POS_OBJECT_TYPE, strObjectType);
						m_cListCtrl.SetItemText(nIdx, DIFF_POS_DIFF,        strMsg);
					}
				}
			}
		}
	}
}


void CuDlgPageDifferentList::OnCheckGroupByObject() 
{
	this->SendMessage(WMUSRMSG_UPDATEDATA, 0, (LPARAM)m_pCurrentTreeItem);
}

void CuDlgPageDifferentList::OnDeltaposSpinDepth(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMUPDOWN* pNMUpDown = (NMUPDOWN*)pNMHDR;
	int nDepth = pNMUpDown->iPos + pNMUpDown->iDelta;
	if (nDepth > 0 && nDepth <4)
		this->SendMessage(WMUSRMSG_UPDATEDATA, (WPARAM)pNMUpDown, (LPARAM)m_pCurrentTreeItem);
	*pResult = 0;
}


void CuDlgPageDifferentList::OnChangeEditDepth() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	// TODO: Add your control notification handler code here
}

static const BOOL bUseFont = TRUE;
HBRUSH CuDlgPageDifferentList::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = NULL;

	switch (nCtlColor) 
	{
	case CTLCOLOR_STATIC:
		switch (pWnd->GetDlgCtrlID())
		{
		case IDC_EDIT1:
#if defined (_DEPT_EDIT_CONTROL_BACKGROUND)
			pDC->SetBkColor(m_rgbBkColor);
			pDC->SetTextColor(m_rgbFgColor);
			hbr = (HBRUSH)(m_pEditBkBrush->GetSafeHandle());
#else
			hbr=CDialog::OnCtlColor(pDC, pWnd, nCtlColor); 
#endif
			break;
		case IDC_STATIC_NODIFFERENCE:
#if defined (_DEPT_EDIT_CONTROL_BACKGROUND)
			pDC->SetBkColor(m_rgbBkColor);
			if (bUseFont)
			{
				if (!m_bCreateDiffFont)
				{
					if (UxCreateFont (m_fontNoDifference, _T("Arial"), 20))
						m_bCreateDiffFont = TRUE;
				}
				if (m_bCreateDiffFont)
					pDC->SelectObject (&m_fontNoDifference);
			}
			pDC->SetTextColor(RGB(0, 0, 255));
			hbr = (HBRUSH)(m_pEditBkBrush->GetSafeHandle());
#else
			hbr=CDialog::OnCtlColor(pDC, pWnd, nCtlColor); 
#endif
			break;
		default:
			hbr=CDialog::OnCtlColor(pDC, pWnd, nCtlColor); 
			break;
		}
		break;
	default:
		hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	}
	return hbr;
}

void CuDlgPageDifferentList::OnColumnclickList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMLISTVIEW* pNMListView = (NMLISTVIEW*)pNMHDR;
	*pResult = 0;
	if (pNMListView->iSubItem == -1)
		return;
	CWaitCursor doWaitCursor;
	if (m_sortparam.m_nItem == pNMListView->iSubItem)
		m_sortparam.m_bAsc = !m_sortparam.m_bAsc;
	else
		m_sortparam.m_bAsc = TRUE;
	m_sortparam.m_nItem = pNMListView->iSubItem;

	SortDifferenceItems(m_sortparam.m_nItem);
}

void CuDlgPageDifferentList::ConstructList4Sorting(CObArray& T)
{
	CString strSubItem;
	int i, k, nCount = m_cListCtrl.GetItemCount();
	int nHeader = m_cListCtrl.GetHeaderCtrl()->GetItemCount();
	for (i=0; i<nCount; i++)
	{
		CaItem4Sort* pItem = new CaItem4Sort();
		T.Add(pItem);

		DWORD dwData = m_cListCtrl.GetItemData(i);
		pItem->m_dwData = dwData;
		for (k=0; k<nHeader; k++)
		{
			strSubItem = m_cListCtrl.GetItemText(i, k);
			pItem->m_arrayItem.Add(strSubItem);
		}
	}
}

void CuDlgPageDifferentList::SortDifferenceItems(int nHeader)
{
	try
	{
		if (nHeader < 2)
			m_cListCtrl.SortItems(CompareSubItem, (LPARAM)&m_sortparam);
		else
		{
			int nImage;
			LVITEM iv;
			CString strType;
			CObArray T;
			memset (&iv, 0, sizeof(iv));
			iv.mask = LVIF_IMAGE;
			ConstructList4Sorting(T);
			SORT_DichotomySort(T, CompareSubItem2, (LPARAM)&m_sortparam, NULL);
			int i, k, nCount = T.GetSize();
			for (i=0; i<nCount; i++)
			{
				CaItem4Sort* pItem = (CaItem4Sort*)T[i];
				QueryImageID((CtrfItemData*)pItem->m_dwData, nImage, strType);

				iv.iItem  = i;
				iv.iImage = nImage;
				m_cListCtrl.SetItem(&iv);
				m_cListCtrl.SetItemData(i, (DWORD)pItem->m_dwData);

				int nHeader = pItem->m_arrayItem.GetSize();
				for (k=0; k<nHeader; k++)
				{
					m_cListCtrl.SetItemText(i, k, pItem->m_arrayItem[k]);
				}

				delete pItem;
			}
		}
	}
	catch (...)
	{

	}
}


void CuDlgPageDifferentList::OnDblclkList1(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	// The property sheet attached to your project
	// via this function is not hooked up to any message
	// handler.  In order to actually use the property sheet,
	// you will need to associate this function with a control
	// in your project such as a menu item or tool bar button.
	//
	// If mini frame does not already exist, create a new one.
	// Otherwise, unhide it

	if (m_pPropFrame == NULL)
	{
		m_pPropFrame = new CfMiniFrame();
		CRect rect(0, 0, 0, 0);
		CString strTitle;
		AfxFormatString1(strTitle, IDS_DIFFPROPERTY_CAPTION, (LPCTSTR)INGRESII_QueryInstallationID());
		//VERIFY(strTitle.LoadString(IDS_DIFFPROPERTY_CAPTION));
		CString strMsg;
		strMsg.LoadString(IDS_NODETAIL_DIFFERENCExTHISITEM); // _T("N/A")
		m_pPropFrame->SetNoPageMessage(strMsg);
		UINT nStyle = WS_THICKFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU;
		if (!m_pPropFrame->Create(NULL, strTitle, nStyle, rect, this))
		{
			delete m_pPropFrame;
			m_pPropFrame = NULL;
			return;
		}
		m_pPropFrame->CenterWindow();
	}
	// Before unhiding the modeless property sheet, update its
	// settings appropriately.  For example, if you are reflecting
	// the state of the currently selected item, pick up that
	// information from the active view and change the property
	// sheet settings now.
	NMLISTVIEW* pNMListView = (NMLISTVIEW*)pNMHDR;
	if (pNMListView->iItem >= 0 && (m_cListCtrl.GetItemState (pNMListView->iItem, LVIS_SELECTED)&LVIS_SELECTED))
	{
		PopupDetail(pNMListView->iItem);
	}
	
	if (m_pPropFrame != NULL && !m_pPropFrame->IsWindowVisible())
		m_pPropFrame->ShowWindow(SW_SHOW);
}

void CuDlgPageDifferentList::OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMLISTVIEW* pNMListView = (NMLISTVIEW*)pNMHDR;
	if (pNMListView->iItem >= 0 && pNMListView->uNewState > 0 && m_pPropFrame && m_pPropFrame->IsWindowVisible())
	{
		PopupDetail(pNMListView->iItem);
		SetFocus();
	}
	
	*pResult = 0;
}

void CuDlgPageDifferentList::PopupDetail(int nSelectedDiffItem)
{
	BOOL bDoPopup = FALSE;
	if (nSelectedDiffItem >= 0)
	{
		m_pDetailDifference->SetListCtrl(&m_cListCtrl);
		DWORD dwItemData = m_cListCtrl.GetItemData (nSelectedDiffItem);
		CtrfItemData* pItem = (CtrfItemData*)dwItemData;
		CaVsdItemData* pVsdItemData = (CaVsdItemData*)pItem->GetUserData();
		if (pItem)
		{
			CString strMsg;
			strMsg.LoadString(IDS_NODETAIL_DIFFERENCExTHISITEM); // _T("N/A")
			m_pPropFrame->SetNoPageMessage(strMsg);
			if (!m_bActionGroupByObject)
			{
				CString csTemp;
				csTemp.LoadString(IDS_DIFFERENCES_IN_SUBBRANCHES);
				CString strType = m_cListCtrl.GetItemText (nSelectedDiffItem, 2);
				if (strType.CompareNoCase (csTemp) == 0)
					bDoPopup = FALSE;
				else
					bDoPopup = TRUE;
			}
			else
				bDoPopup = FALSE;
		}
		if (m_pPropFrame && m_pPropFrame->GetUserPages().GetCount() == 0 && bDoPopup)
		{
			CTypedPtrList < CObList, CPropertyPage* > listPages;
			listPages.AddTail(m_pDetailDifference);
			m_pPropFrame->NewPages (listPages, FALSE);
		}
		if (m_pPropFrame && bDoPopup)
		{
			m_pPropFrame->HandleData((LPARAM)pItem);
			return;
		}
	}

	if (m_pPropFrame && !bDoPopup)
	{
		m_pPropFrame->RemoveAllPages();
		m_pPropFrame->HandleData((LPARAM)0);
	}
}
