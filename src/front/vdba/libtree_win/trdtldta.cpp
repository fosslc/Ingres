/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : trdtldta.cpp: implementation for the CaDisplayInfo class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : INFO for displaying the Objects in a Tree Hierachy.
**
** History:
**
** 17-Jan-2000 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/

#include "stdafx.h"
#include "trctldta.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CaDisplayInfo::CaDisplayInfo()
{
	m_strOWNERxITEM = _T("");

	m_nFolder              = IM_FOLDER_CLOSED;
	m_nFolderExpand        = IM_FOLDER_OPENED;
	m_nEmpty               = IM_FOLDER_EMPTY;
	m_nEmptyExpand         = IM_FOLDER_EMPTY;
	m_nNode                = IM_FOLDER_NODE_NORMAL;
	m_nNodeExpand          = IM_FOLDER_NODE_NORMAL;
	m_nNodeLocal           = IM_FOLDER_NODE_LOCAL;
	m_nNodeLocalExpand     = IM_FOLDER_NODE_LOCAL;
	m_nNodeServer          = IM_FOLDER_NODE_SERVER;
	m_nNodeServerExpand    = IM_FOLDER_NODE_SERVER;

	m_nNodeLogin           = IM_FOLDER_EMPTY;
	m_nNodeLoginExpand     = IM_FOLDER_EMPTY;
	m_nNodeConnection      = IM_FOLDER_EMPTY;
	m_nNodeConnectionExpand= IM_FOLDER_EMPTY;
	m_nNodeAttribute       = IM_FOLDER_EMPTY;
	m_nNodeAttributeExpand = IM_FOLDER_EMPTY;

	m_nUser            = -1;
	m_nUserExpand      = -1;
	m_nGroup           = -1;
	m_nGroupExpand     = -1;
	m_nProfile         = -1;
	m_nProfileExpand   = -1;
	m_nRole            = -1;
	m_nRoleExpand      = -1;
	m_nLocation        = -1;
	m_nLocationExpand  = -1;
	m_nDatabase        = IM_FOLDER_DATABASE;
	m_nDatabaseExpand  = IM_FOLDER_DATABASE;
	m_nTable           = IM_FOLDER_TABLE;
	m_nTableExpand     = IM_FOLDER_TABLE;
	m_nView            = IM_FOLDER_TABLE;
	m_nViewExpand      = IM_FOLDER_TABLE;
	m_nProcedure       = IM_FOLDER_EMPTY;
	m_nProcedureExpand = IM_FOLDER_EMPTY;
	m_nView            = IM_FOLDER_EMPTY;
	m_nViewExpand      = IM_FOLDER_EMPTY;
	m_nSynonym         = IM_FOLDER_EMPTY;
	m_nSynonymExpand   = IM_FOLDER_EMPTY;

	m_nColumn          = IM_FOLDER_EMPTY;
	m_nColumnExpand    = IM_FOLDER_EMPTY;
	m_nIndex           = IM_FOLDER_EMPTY;
	m_nIndexExpand     = IM_FOLDER_EMPTY;
	m_nRule            = IM_FOLDER_EMPTY;
	m_nRuleExpand      = IM_FOLDER_EMPTY;
	m_nIntegrity       = IM_FOLDER_EMPTY;
	m_nIntegrityExpand = IM_FOLDER_EMPTY;

	for (int i=0; i<NUM_OBJECTFOLDER; i++)
	{
		m_bArrayDispayInFolder[i] = TRUE;
	}

	m_nNodeChildrenFolderFlag  = 0;
	m_nServerChildrenFolderFlag= 0;
	m_nDBChildrenFolderFlag    = 0;
	m_nTableChildrenFolderFlag = 0;
	m_nViewChildrenFolderFlag  = 0;
	m_nProcedureChildrenFolderFlag = 0;
	m_nDBEventChildrenFolderFlag   = 0;
}


void CaDisplayInfo::NodeChildrenFolderAddFlag(UINT nFlag){m_nNodeChildrenFolderFlag |= nFlag;}
void CaDisplayInfo::NodeChildrenFolderRemoveFlag(UINT nFlag){m_nNodeChildrenFolderFlag &= ~nFlag;}

void CaDisplayInfo::ServerChildrenFolderAddFlag(UINT nFlag){m_nServerChildrenFolderFlag |= nFlag;}
void CaDisplayInfo::ServerChildrenFolderRemoveFlag(UINT nFlag){m_nServerChildrenFolderFlag &= ~nFlag;}

void CaDisplayInfo::DBChildrenFolderAddFlag(UINT nFlag){m_nDBChildrenFolderFlag |= nFlag;}
void CaDisplayInfo::DBChildrenFolderRemoveFlag(UINT nFlag){m_nDBChildrenFolderFlag &= ~nFlag;}

void CaDisplayInfo::TableChildrenFolderAddFlag(UINT nFlag){m_nTableChildrenFolderFlag |= nFlag;}
void CaDisplayInfo::TableChildrenFolderRemoveFlag(UINT nFlag){m_nTableChildrenFolderFlag &= ~nFlag;}

void CaDisplayInfo::ViewChildrenFolderAddFlag(UINT nFlag){m_nViewChildrenFolderFlag |= nFlag;}
void CaDisplayInfo::ViewChildrenFolderRemoveFlag(UINT nFlag){m_nViewChildrenFolderFlag &= ~nFlag;}

void CaDisplayInfo::ProcedureChildrenFolderAddFlag(UINT nFlag){m_nProcedureChildrenFolderFlag |= nFlag;}
void CaDisplayInfo::ProcedureChildrenFolderRemoveFlag(UINT nFlag){m_nProcedureChildrenFolderFlag &= ~nFlag;}

void CaDisplayInfo::DBEventChildrenFolderAddFlag(UINT nFlag){m_nDBEventChildrenFolderFlag |= nFlag;}
void CaDisplayInfo::DBEventChildrenFolderRemoveFlag(UINT nFlag){m_nDBEventChildrenFolderFlag &= ~nFlag;}

// Object: CtrfItemData (Base object of Tree Item Data)
// ****************************************************
IMPLEMENT_SERIAL (CtrfItemData, CObject, 1)
CtrfItemData::CtrfItemData(TreeItemType itemType): m_pBackParent(NULL)
{
	m_pDBObject = NULL;
	m_strDisplayString = _T("");
	m_strErrorString = _T("<Data Unavailable>");
	m_bError = FALSE;
	m_bDisplayThisItem = TRUE;
	m_treeItemType = itemType;
}

CtrfItemData::CtrfItemData(LPCTSTR lpszDisplayString, TreeItemType itemType): 
    m_pBackParent(NULL), m_strDisplayString(lpszDisplayString), m_strErrorString(_T("<Data Unavailable>"))
{
	m_pDBObject = NULL;
	m_bError = FALSE;
	m_bDisplayThisItem = TRUE;
	m_treeItemType = itemType;
}

CtrfItemData::~CtrfItemData()
{
	CtrfItemData* pObj = NULL;
	POSITION p, pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		pObj = m_listObject.GetNext(pos);
		m_listObject.RemoveAt(p);
		delete pObj;
	}
	if (m_pDBObject)
		delete m_pDBObject;
	m_pDBObject = NULL;
}

CaDBObject* CtrfItemData::GetDBObject()
{
	if (!m_pDBObject)
		m_pDBObject = new CaDBObject(m_strDisplayString, _T(""));

	return m_pDBObject;
}

void CtrfItemData::Display (CTreeCtrl* pTree, HTREEITEM hParent)
{
	CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
	BOOL bDisplayFolder = TRUE;
	HTREEITEM hItem = m_treeCtrlData.GetTreeItem();
	CtrfItemData* pData = NULL;
	POSITION p, pos = m_listObject.GetHeadPosition();
	//
	// pDisplayInfo must be accessible, it must be in the root parent of 
	// the three hierarchy !
	ASSERT(pDisplayInfo);
	if (!pDisplayInfo)
		return;
	if (!DisplayItem())
	{
		if (hItem)
		{
			//
			// Delete tree's item.
			pTree->DeleteItem(hItem);
			Reset();
		}
		return;
	}

	if (hItem)
	{
		SetImageID(pDisplayInfo);
		int nImage = m_treeCtrlData.GetImage();
		int nImageSel = m_treeCtrlData.GetImageExpanded();
		pTree->SetItemImage(hItem, nImage, nImageSel);
	}
	else
	{
		//
		// Create this tree item:
		if (IsFolder())
		{
			bDisplayFolder = IsDisplayFolder (pDisplayInfo);
			if (bDisplayFolder )
			{
				int nImage;
				CString strDisplay = GetDisplayedItem();

				SetImageID(pDisplayInfo);
				nImage = m_treeCtrlData.GetImage();
				hItem = CtrfItemData::TreeAddItem (strDisplay, pTree, hParent, TVI_LAST, nImage, (DWORD)this);
				m_treeCtrlData.SetTreeItem (hItem);
				m_treeCtrlData.SetState(CaTreeCtrlData::ITEM_EXIST);
			}
			else
			{
				m_treeCtrlData.SetTreeItem (NULL);
				hItem = hParent;
			}
		}
		else
		{
			int nImage;
			CString strDisplay = GetDisplayedItem();

			SetImageID(pDisplayInfo);
			nImage = m_treeCtrlData.GetImage();
			hItem = CtrfItemData::TreeAddItem (strDisplay, pTree, hParent, TVI_LAST, nImage, (DWORD)this);
			m_treeCtrlData.SetTreeItem (hItem);
			m_treeCtrlData.SetState(CaTreeCtrlData::ITEM_EXIST);
		}
	}

	if (IsFolder())
	{
		//
		// Display the list of Objects:
		if (!m_bError)
		{
			//
			// Delete the dummy item (avoid showing <No Objects/Data unavailable> while
			// displaying the list of objects:
			CtrfItemData* pEmptyNode = GetEmptyNode();
			if (pEmptyNode)
			{
				CaTreeCtrlData& emptyDta = pEmptyNode->GetTreeCtrlData();
				if (emptyDta.GetTreeItem())
					pTree->DeleteItem (emptyDta.GetTreeItem());
				emptyDta.SetTreeItem(NULL);
			}

			pos = m_listObject.GetHeadPosition();
			while (pos != NULL)
			{
				p = pos;
				pData = m_listObject.GetNext (pos);
				if (pData->GetTreeCtrlData().GetState() != CaTreeCtrlData::ITEM_DELETE)
				{
					pData->Display (pTree, hItem);
				}
				else
				{
					//
					// Delete all items if their states are <ITEM_DELETE>
					HTREEITEM hDeleteItem = pData->GetTreeCtrlData().GetTreeItem();
					if (hDeleteItem)
						pTree->DeleteItem (hDeleteItem);

					pData->GetTreeCtrlData().SetTreeItem(NULL);
					m_listObject.RemoveAt (p);
					delete pData;
				}
			}

			//
			// Display the dummy item <No Objects>:
			pos = m_listObject.GetHeadPosition();
			if (pos == NULL && bDisplayFolder)
			{
				CtrfItemData* pEmptyNode = GetEmptyNode();
				if (pEmptyNode)
					CtrfItemData::DisplayEmptyOrErrorNode (pEmptyNode, pTree, hItem);
			}
		}
		else
		if (bDisplayFolder)
		{
			//
			// Display the dummy <Data Unavailable>:
			CtrfItemData* pEmptyNode = GetEmptyNode();
			if (pEmptyNode)
			{
				CtrfItemData::DisplayEmptyOrErrorNode (pEmptyNode, pTree, hItem);
				CaTreeCtrlData& emptyDta = pEmptyNode->GetTreeCtrlData();
				HTREEITEM hDummy = emptyDta.GetTreeItem();
				CString strError = pEmptyNode->GetErrorString();
				if (hDummy)
					pTree->SetItemText (hDummy, strError);
			}
		}
	}
	else
	{
		//
		// Display the list of Folders that this object contains:
		POSITION pos = m_listObject.GetHeadPosition();
		while (pos != NULL)
		{
			pData = m_listObject.GetNext (pos);
			pData->Display (pTree, hItem);
		}

		//
		// Display the dummy item <No Objects> if we don't display
		// any folders:
		if (!pTree->ItemHasChildren(hItem))
		{
			CtrfItemData* pEmptyNode = GetEmptyNode();
			if (pEmptyNode)
				CtrfItemData::DisplayEmptyOrErrorNode (pEmptyNode, pTree, hItem);
		}
	}
	if (m_treeCtrlData.IsExpanded())
		pTree->Expand(hItem, TVE_EXPAND);
}


HTREEITEM CtrfItemData::TreeAddItem (LPCTSTR lpszItem, CTreeCtrl* pTree, HTREEITEM hParent, HTREEITEM hInsertAfter, int nImage, DWORD dwData)
{
	TVINSERTSTRUCT tvins;
	memset (&tvins, 0, sizeof(tvins));
	tvins.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	//
	// Set the text of the item. 
	tvins.item.pszText = (LPTSTR)lpszItem; 
	tvins.item.cchTextMax = lstrlen(lpszItem); 
	
	tvins.item.iImage = nImage;
	tvins.item.iSelectedImage = nImage; 
	tvins.hInsertAfter = hInsertAfter;
	tvins.hParent = hParent;
	
	HTREEITEM hItem = pTree->InsertItem(&tvins);
	if (hItem)
		pTree->SetItemData (hItem, dwData);
	return hItem; 
}

void CtrfItemData::DisplayEmptyOrErrorNode (CtrfItemData* pEmptyNode, CTreeCtrl* pTree, HTREEITEM hParent)
{
	CaTreeCtrlData& treeCtrlData = pEmptyNode->GetTreeCtrlData();
	HTREEITEM hItem = treeCtrlData.GetTreeItem();
	//
	// Create this tree item:
	if (!hItem)
	{
		int nImage;
		CString strDisplay = pEmptyNode->GetDisplayedItem();

		nImage = treeCtrlData.GetImage();
		hItem = CtrfItemData::TreeAddItem (strDisplay, pTree, hParent, TVI_LAST, nImage, (DWORD)pEmptyNode);
		treeCtrlData.SetTreeItem (hItem);
		treeCtrlData.SetState(CaTreeCtrlData::ITEM_EXIST);
	}
}


void CtrfItemData::Reset()
{
	CaTreeCtrlData& ctrData = GetTreeCtrlData();
	ctrData.SetTreeItem(NULL);
	CtrfItemData* pEmptyItem = GetEmptyNode();
	if (pEmptyItem)
		pEmptyItem->Reset();

	POSITION pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
		ptrfObj->Reset();
	}
}

static BOOL FindItemType(TreeItemType itemType, CArray<TreeItemType, TreeItemType>& arrayItem)
{
	int i, nSize = arrayItem.GetSize();
	for (i=0; i<nSize; i++)
	{
		if (arrayItem[i] == itemType)
			return TRUE;
	}
	return FALSE;
}

void CtrfItemData::UnDisplayItems(CArray<TreeItemType, TreeItemType>& arrayItem)
{
	if (FindItemType(GetTreeItemType(), arrayItem))
		DisplayItem(FALSE);
	else
		DisplayItem(TRUE);

	POSITION pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		BOOL bUnDisplay = FALSE;
		CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
		TreeItemType itemType = ptrfObj->GetTreeItemType();
		if (FindItemType(itemType, arrayItem))
			ptrfObj->DisplayItem(FALSE);
		else
			ptrfObj->DisplayItem(TRUE);
		ptrfObj->UnDisplayItems (arrayItem);
	}
}
