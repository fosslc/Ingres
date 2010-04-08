/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : editlsgn.cpp, Implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Special Owner draw List Control(Editable)
**
** History:
**
** xx-Sep-1997 (uk$so01)
**    Created
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "editlsgn.h"
#include "repitem.h"
#include "wmusrmsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuEditableListCtrlStartupSetting::CuEditableListCtrlStartupSetting()
	:CuEditableListCtrl()
{
}

CuEditableListCtrlStartupSetting::~CuEditableListCtrlStartupSetting()
{
}



BEGIN_MESSAGE_MAP(CuEditableListCtrlStartupSetting, CuEditableListCtrl)
	//{{AFX_MSG_MAP(CuEditableListCtrlStartupSetting)
	ON_WM_RBUTTONDOWN()
	ON_WM_DESTROY()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDBLCLK()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_LAYOUTEDITDLG_CANCEL,        OnEditDlgCancel)
	ON_MESSAGE (WM_LAYOUTEDITDLG_OK,            OnEditDlgOK)
	ON_MESSAGE (WM_LAYOUTEDITNUMBERDLG_CANCEL,  OnEditNumberDlgCancel)
	ON_MESSAGE (WM_LAYOUTEDITNUMBERDLG_OK,      OnEditNumberDlgOK)
	ON_MESSAGE (WM_LAYOUTEDITSPINDLG_CANCEL,    OnEditSpinDlgCancel)
	ON_MESSAGE (WM_LAYOUTEDITSPINDLG_OK,        OnEditSpinDlgOK)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuEditableListCtrlStartupSetting message handlers
	
LONG CuEditableListCtrlStartupSetting::OnEditDlgOK (UINT wParam, LONG lParam)
{
	int iItem, iSubItem;
	CString s = EDIT_GetText();
	EDIT_GetEditItem(iItem, iSubItem);
	if (iItem < 0)
		return 0L;

	SetItemText (iItem, iSubItem, s);
	try
	{
		CaReplicationItem* pData = (CaReplicationItem*)GetItemData(iItem);
		if (!pData)
			return 0L;
		if (pData)
		{
			pData->SetFlagContent (s);
			pData->SetValueModifyByUser(TRUE);
			GetParent()->SendMessage (WM_LAYOUTEDITDLG_OK, (WPARAM)0, (LPARAM)0);
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage ();
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("Other error occured ...\n");
	}
	return 0L;
}

LONG CuEditableListCtrlStartupSetting::OnEditDlgCancel (UINT wParam, LONG lParam)
{
	return 0L;
}

LONG CuEditableListCtrlStartupSetting::OnEditNumberDlgOK (UINT wParam, LONG lParam)
{
	int iItem, iSubItem;
	CString s = EDITNUMBER_GetText();

	EDITNUMBER_GetEditItem(iItem, iSubItem);
	if (iItem < 0)
		return 0L;
	SetItemText (iItem, iSubItem, s);
	try
	{
		CaReplicationItem* pData = (CaReplicationItem*)GetItemData(iItem);
		if (pData)
		{
			pData->SetFlagContent(s);
			pData->SetValueModifyByUser(TRUE);
			GetParent()->SendMessage (WM_LAYOUTEDITDLG_OK, (WPARAM)0, (LPARAM)0);
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage ();
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("Other error occured ...\n");
	}
	return 0L;
}

LONG CuEditableListCtrlStartupSetting::OnEditNumberDlgCancel (UINT wParam, LONG lParam)
{
	return 0L;
}



LONG CuEditableListCtrlStartupSetting::OnEditSpinDlgOK (UINT wParam, LONG lParam)
{
	int iItem, iSubItem;
	int iMin, iMax;
	CString s = EDITSPIN_GetText();
	if (s.IsEmpty())
	{
		EDITSPIN_GetRange (iMin, iMax);
		s.Format (_T("%d"), iMin);
	}
	EDITSPIN_GetEditItem(iItem, iSubItem);
	if (iItem < 0)
		return 0L;
	SetItemText (iItem, iSubItem, s);
	try
	{
		CaReplicationItem* pData = (CaReplicationItem*)GetItemData(iItem);
		if (pData)
		{
			pData->SetFlagContent (s);
			pData->SetValueModifyByUser(TRUE);
			GetParent()->SendMessage (WM_LAYOUTEDITDLG_OK, (WPARAM)0, (LPARAM)0);
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage ();
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("Other error occured ...\n");
	}
	return 0L;
}

LONG CuEditableListCtrlStartupSetting::OnEditSpinDlgCancel (UINT wParam, LONG lParam)
{
	return 0L;
}


void CuEditableListCtrlStartupSetting::OnCheckChange (int iItem, int iSubItem, BOOL bCheck)
{
	CString strOnOff = bCheck? _T("TRUE"): _T("FALSE");
	CaReplicationItem* pData = (CaReplicationItem*)GetItemData(iItem);
	try
	{
		if (pData)
		{
			pData->SetFlagContent (strOnOff);
			pData->SetValueModifyByUser(TRUE);
			GetParent()->SendMessage (WM_LAYOUTEDITDLG_OK, (WPARAM)0, (LPARAM)0);
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage ();
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("Other error occured ...\n");
	}
}


void CuEditableListCtrlStartupSetting::OnRButtonDown(UINT nFlags, CPoint point) 
{
	int   index, iColumnNumber = 1;
	int   iItem   = -1, iSubItem = -1;
	int   iNumMin = 0;
	int   iNumMax = 400;
	CRect rect, rCell;
	UINT  flags;
	
	index = HitTest (point, &flags);
	if (index < 0)
		return;
	GetItemRect (index, rect, LVIR_BOUNDS);
	if (!GetCellRect (rect, point, rCell, iColumnNumber))
		return;
	
	if (rect.PtInRect (point))
		CuListCtrl::OnRButtonDown(nFlags, point);
	rCell.top    -= 2;
	rCell.bottom += 2;
	HideProperty();
	if (iColumnNumber != 1)
		return;
	EditValue (index, iColumnNumber, rCell);
}




void CuEditableListCtrlStartupSetting::OnDestroy() 
{
	CuEditableListCtrl::OnDestroy();
}



void CuEditableListCtrlStartupSetting::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	int   index, iColumnNumber;
	int   iItem   = -1, iSubItem = -1;
	int   iNumMin = 0;
	int   iNumMax = 400;
	CRect rect, rCell;
	UINT  flags;
	
	index = HitTest (point, &flags);
	if (index < 0)
		return;
	GetItemRect (index, rect, LVIR_BOUNDS);
	if (!GetCellRect (rect, point, rCell, iColumnNumber))
		return;
	
	rCell.top    -= 2;
	rCell.bottom += 2;
	HideProperty();
	if (iColumnNumber != 1)
		return;
	EditValue (index, iColumnNumber, rCell);
}



void CuEditableListCtrlStartupSetting::OnRButtonDblClk(UINT nFlags, CPoint point) 
{
	//
	// Just swallow the message
	// CuListCtrl::OnRButtonDblClk(nFlags, point);
}



void CuEditableListCtrlStartupSetting::EditValue (int iItem, int iSubItem, CRect rcCell)
{
	CaReplicationItem* pItem = (CaReplicationItem*)GetItemData(iItem);
	if (!pItem)
		return;
	EDITNUMBER_SetSpecialParse (FALSE);
	if (iSubItem != 1)
		return;
	if (pItem->IsReadOnlyFlag())
		return;
	switch (pItem->GetType())
	{
	case CaReplicationItem::REP_STRING:
		SetEditText (iItem, iSubItem, rcCell);
		break;
	case CaReplicationItem::REP_NUMBER:
		SetEditNumber (iItem, iSubItem, rcCell);
		break;
	default:
		break;
	}
}



