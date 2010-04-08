/*
**  Copyright (C) 2005-2006 Ingres Corporation.
*/

/*
**    Source   : calscbox.cpp : implementation file
**    Project  : Owner Drawn window control 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Make the CListCtrl to have a check box brhaviour like 
**               the CCheckListBox.
**
** History:
**
** 27-Apr-2001 (uk$so01)
**    Created for SIR #102678
**    Support the composite histogram of base table and secondary index.
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
**/

#include "stdafx.h"
#include "calscbox.h"
#include "wmusrmsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuListCtrlCheckBox::CuListCtrlCheckBox()
{
	m_bStateIcons = TRUE;
}

CuListCtrlCheckBox::~CuListCtrlCheckBox()
{
}


BEGIN_MESSAGE_MAP(CuListCtrlCheckBox, CuListCtrl)
	//{{AFX_MSG_MAP(CuListCtrlCheckBox)
	ON_WM_LBUTTONDOWN()
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuListCtrlCheckBox message handlers

void CuListCtrlCheckBox::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CuListCtrl::OnLButtonDown(nFlags, point);

	UINT uFlags = 0;
	int nHitItem = HitTest(point, &uFlags);

	// we need additional checking in owner-draw mode
	// because we only get LVHT_ONITEM
	BOOL bHit = FALSE;
	if (uFlags == LVHT_ONITEM && (GetStyle() & LVS_OWNERDRAWFIXED))
	{
		CRect rect;
		GetItemRect(nHitItem, rect, LVIR_ICON);
		//
		// Check if hit was on a state icon
		if (m_bStateIcons && point.x < rect.left)
			bHit = TRUE;
	}
	else if (uFlags & LVHT_ONITEMSTATEICON)
		bHit = TRUE;

	if (bHit)
	{
		CheckItem(nHitItem);
	}
}

void CuListCtrlCheckBox::CheckItem(int nNewCheckedItem, BOOL bCheck)
{
	UINT nState   = GetItemState (nNewCheckedItem, LVIS_STATEIMAGEMASK);

	if (m_bStateIcons && (nState & LVIS_STATEIMAGEMASK))
		SetItemState(nNewCheckedItem, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);
}



void CuListCtrlCheckBox::CheckItem(int nNewCheckedItem)
{
	UINT nState   = GetItemState (nNewCheckedItem, LVIS_STATEIMAGEMASK);
	UINT nSNCheck = INDEXTOSTATEIMAGEMASK(1); // Not Checked
	UINT nSCheck  = INDEXTOSTATEIMAGEMASK(2); // Checked

	//
	// You must set the state image list and insert item
	// with the state image mask:
	ASSERT (nState & LVIS_STATEIMAGEMASK);

	if (m_bStateIcons && (nState & nSCheck))
	{
		SetItemState (nNewCheckedItem, INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);
	}
	else
	if (m_bStateIcons && (nState & nSNCheck))
	{
		SetItemState(nNewCheckedItem, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);
	}
	GetParent()->SendMessage (WMUSRMSG_LISTCTRL_CHECKCHANGE, (WPARAM)m_hWnd, (LPARAM)nNewCheckedItem);
}

BOOL CuListCtrlCheckBox::GetItemChecked(int nItem)
{
	UINT nState   = GetItemState (nItem, LVIS_STATEIMAGEMASK);
	UINT nSNCheck = INDEXTOSTATEIMAGEMASK(1); // Not Checked
	UINT nSCheck  = INDEXTOSTATEIMAGEMASK(2); // Checked

	if (m_bStateIcons && nState == nSCheck)
	{
		return TRUE;
	}
	else
	if (m_bStateIcons && nState == nSNCheck)
	{
		return FALSE;
	}
	return FALSE;
}


void CuListCtrlCheckBox::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	int nSelectedItem = GetNextItem (-1, LVNI_FOCUSED|LVNI_SELECTED);
	if (nChar == VK_SPACE && nSelectedItem != -1 && m_bStateIcons)
	{
		UINT nState = GetItemState (nSelectedItem, LVIS_STATEIMAGEMASK);
		if (nState & LVIS_STATEIMAGEMASK)
			CheckItem(nSelectedItem);
	}
	CuListCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}
