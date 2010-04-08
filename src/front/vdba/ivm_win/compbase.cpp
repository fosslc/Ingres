/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : compbase.cpp , Implementation File                                    //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II / Visual Manager                                            //
//    Author   : Sotheavut UK (uk$so01)                                                //
//                                                                                     //
//                                                                                     //
//    Purpose  : Manipulation Ingres Visual Manager Data                               //
****************************************************************************************/
/* History:
* --------
* uk$so01: (13-Jan-1999 created)
*
*
*/

#include "stdafx.h"
#include "compbase.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CaTreeCtrlData::CaTreeCtrlData() 
{
	m_nState = 0;
	m_nItemFlag = ITEM_NEW;
	m_hTreeItem = NULL;
	m_bExpanded = FALSE;
	m_bRead = TRUE;
	m_nActivity = 0;

	m_nImage = IM_FOLDER_CLOSED_R_NORMAL;
	m_nExpandedImage = IM_FOLDER_OPENED_R_NORMAL;

	m_nAlertCount = 0;
}


CaTreeCtrlData::CaTreeCtrlData(ItemState nFlag, HTREEITEM hTreeItem)
{
	m_nItemFlag = nFlag;
	m_hTreeItem = hTreeItem;
	m_bExpanded = FALSE;
	m_bRead = TRUE;
	m_nActivity = 0;

	m_nImage = IM_FOLDER_CLOSED_R_NORMAL;
	m_nExpandedImage = IM_FOLDER_OPENED_R_NORMAL;

	m_nState = 0;
	m_nAlertCount = 0;
}


void CaTreeCtrlData::AlertMarkUpdate (CTreeCtrl* pTree, BOOL bInstance)
{
//UX	ASSERT (m_hTreeItem);
	if (m_hTreeItem && pTree)
	{
		if (bInstance)
		{
			if (m_nAlertCount > 0)
				pTree->SetItemImage (m_hTreeItem, m_nExpandedImage, m_nExpandedImage);
			else
				pTree->SetItemImage (m_hTreeItem, m_nImage, m_nImage);
			return;
		}

		if (m_bExpanded)
			pTree->SetItemImage (m_hTreeItem, m_nExpandedImage, m_nExpandedImage);
		else
			pTree->SetItemImage (m_hTreeItem, m_nImage, m_nImage);
	}
}

