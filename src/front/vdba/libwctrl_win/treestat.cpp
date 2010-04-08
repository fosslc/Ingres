/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : treestat.cpp : implementation file
**    Project  : libwctrl.lib 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Tree Control that supports the state image list.
**               the CCheckListBox.
**
** History:
**
** 17-Jul-2001 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
**/


#include "stdafx.h"
#include "treestat.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CuTreeStateIcon::CuTreeStateIcon()
{
}

CuTreeStateIcon::~CuTreeStateIcon()
{
}

BOOL CuTreeStateIcon::IsItemChecked(HTREEITEM hItem)
{
	UINT nSNCheck = INDEXTOSTATEIMAGEMASK(2); // Not Checked
	UINT nSCheck  = INDEXTOSTATEIMAGEMASK(1); // Checked
	BOOL bChecked = FALSE;
	if (!hItem)
		return bChecked;
	CImageList* pStateImageList = GetImageList(TVSIL_STATE);
	if (!pStateImageList)
		return bChecked;

	TVITEM tvitem;
	memset (&tvitem, 0, sizeof(tvitem));
	tvitem.mask      = TVIF_STATE;
	tvitem.hItem     = hItem;
	tvitem.stateMask = TVIS_STATEIMAGEMASK;
	if (!GetItem (&tvitem))
		return bChecked;
	if (tvitem.state & nSNCheck)
	{
		bChecked = FALSE;
	}
	else
	if (tvitem.state & nSCheck)
	{
		bChecked = TRUE;
	}
	return bChecked;
}


BEGIN_MESSAGE_MAP(CuTreeStateIcon, CTreeCtrl)
	//{{AFX_MSG_MAP(CuTreeStateIcon)
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuTreeStateIcon message handlers

void CuTreeStateIcon::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CTreeCtrl::OnLButtonDown(nFlags, point);
	CImageList* pStateImageList = GetImageList(TVSIL_STATE);
	if (pStateImageList)
	{
		UINT nHitFlag = 0;
		UINT nSNCheck = INDEXTOSTATEIMAGEMASK(2); // Not Checked
		UINT nSCheck  = INDEXTOSTATEIMAGEMASK(1); // Checked

		HTREEITEM hTreeItem = HitTest(point, &nHitFlag);
		if (hTreeItem && (nHitFlag & TVHT_ONITEMSTATEICON))
		{
			TVITEM tvitem;
			memset (&tvitem, 0, sizeof(tvitem));
			tvitem.mask      = TVIF_STATE;
			tvitem.hItem     = hTreeItem;
			tvitem.stateMask = TVIS_STATEIMAGEMASK;
			if (!GetItem (&tvitem))
				return;
			if (tvitem.state & nSNCheck)
			{
				SetItemState (hTreeItem, nSCheck, LVIS_STATEIMAGEMASK);
			}
			else
			if (tvitem.state & nSCheck)
			{
				SetItemState (hTreeItem, nSNCheck, LVIS_STATEIMAGEMASK);
			}
		}
	}
}
