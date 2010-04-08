/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : edlsdbgr.cpp, Implementation File 
**    Project  : Ingres II/Vdba.
**    Author   : UK Sotheavut
**
**    Purpose  : Editable List control to provide to Change Database Privileges
**
** History:
** xx-Nov-1998 (uk$so01)
**    Created
** 28-Mar-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "edlsdbgr.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern "C"
{
#include "dbaset.h"
}

CuEditableListCtrlGrantDatabase::CuEditableListCtrlGrantDatabase()
{
}

CuEditableListCtrlGrantDatabase::~CuEditableListCtrlGrantDatabase()
{
}


BEGIN_MESSAGE_MAP(CuEditableListCtrlGrantDatabase, CuEditableListCtrl)
	//{{AFX_MSG_MAP(CuEditableListCtrlGrantDatabase)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_LAYOUTEDITNUMBERDLG_OK,  OnEditNumberDlgOK)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuEditableListCtrlGrantDatabase message handlers


void CuEditableListCtrlGrantDatabase::OnLButtonDblClk(UINT nFlags, CPoint point) 
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
	EditValue (index, iColumnNumber, rCell);
}


void CuEditableListCtrlGrantDatabase::OnRButtonDown(UINT nFlags, CPoint point) 
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
	EditValue (index, iColumnNumber, rCell);
}


LONG CuEditableListCtrlGrantDatabase::OnEditNumberDlgOK (UINT wParam, LONG lParam)
{
	int iItem, iSubItem;
	CString s = EDITNUMBER_GetText();
	EDITNUMBER_GetEditItem(iItem, iSubItem);
	if (iItem < 0)
	{
		SetFocus();
		return 0L;
	}
	try
	{
		CaDatabasePrivilegesItemData* pData = (CaDatabasePrivilegesItemData*)GetItemData(iItem);
		if (pData)
			pData->SetValue (s);
		DisplayPrivileges (iItem);
		//
		// Inform the parent that something changes:
		GetParent()->SendMessage (WM_LAYOUTEDITNUMBERDLG_OK, (WPARAM)(LPCTSTR)m_ComboDlgOriginalText, (LPARAM)(LPCTSTR)s);
	}
	catch (...)
	{
		//CString strMsg = _T("Cannot change edit text.");
		AfxMessageBox (VDBA_MfcResourceString(IDS_E_CHANGE_EDIT));
	}
	SetFocus();
	return 0L;
}

void CuEditableListCtrlGrantDatabase::EditValue (int iItem, int iSubItem, CRect rcCell)
{
	if (iSubItem < 1)
		return;
	CaDatabasePrivilegesItemData* pData = (CaDatabasePrivilegesItemData*)GetItemData(iItem);
	if (!pData)
		return;

	switch (iSubItem)
	{
	case C_GRANT:
	case C_LIMIT:
	case C_GRANTNOPRIV:
		break;
	case C_LIMIT_VALUE:
		if (!pData->HasValue())
			break;
		if (!pData->GetGrantPrivilege())
			break;
		SetEditNumber (iItem, iSubItem, rcCell);
		break;
	default:
		break;
	}
}


void CuEditableListCtrlGrantDatabase::OnCheckChange (int iItem, int iSubItem, BOOL bCheck)
{
	try
	{
		CaDatabasePrivilegesItemData* pData = (CaDatabasePrivilegesItemData*)GetItemData(iItem);
		ASSERT (pData);
		if (!pData)
			return;
		switch (iSubItem)
		{
		case C_GRANT:
			pData->SetPrivilege(bCheck);
			break;
		case C_LIMIT:
			pData->SetPrivilege(bCheck);
			if (bCheck)
			{
				CRect rect, rCell;
				GetItemRect (iItem, rect, LVIR_BOUNDS);
				if (!GetCellRect (rect, C_LIMIT_VALUE, rCell))
					return;
				rCell.top    -= 2;
				rCell.bottom += 2;
				HideProperty();
				EditValue (iItem, C_LIMIT_VALUE, rCell);
			}
			else
				pData->SetValue (_T(""));
			break;
		case C_GRANTNOPRIV:
			pData->SetNoPrivilege(bCheck);
			break;
		default:
			return;
		}
		DisplayPrivileges (iItem);
		//
		// Inform the parent that something changes:
		GetParent()->SendMessage (WM_LAYOUTCHECKBOX_CHECKCHANGE, 0, 0);
	}
	catch (...)
	{
		//CString strMsg = _T("Cannot change check box.");
		AfxMessageBox (VDBA_MfcResourceString(IDS_E_CHANGE_CHECK_BOX));
	}
}


void CuEditableListCtrlGrantDatabase::DisplayPrivileges(int nItem)
{
	CaDatabasePrivilegesItemData* pData = NULL;
	int i, nCount = (nItem == -1)? GetItemCount(): nItem +1;
	int nStart = (nItem == -1)? 0: nItem;

	for (i= nStart; i<nCount; i++)
	{
		pData = (CaDatabasePrivilegesItemData*)GetItemData (i);
		SetItemText (i, CuEditableListCtrlGrantDatabase::C_PRIV,  GetPrivilegeString2(pData->GetIDPrivilege()));
		if (pData->HasValue())
		{
			SetItemText (i, C_GRANT,  _T(""));
			SetCheck    (i, C_LIMIT, pData->GetGrantPrivilege());
			SetItemText (i, C_LIMIT_VALUE,  pData->GetValue());
		}
		else
		{
			SetCheck    (i, C_GRANT, pData->GetGrantPrivilege());
			SetItemText (i, C_LIMIT, _T(""));
		}
		SetItemText (i, C_NOPRIV,  GetPrivilegeString2(pData->GetIDNoPrivilege()));
		SetCheck    (i, C_GRANTNOPRIV,  pData->GetGrantNoPrivilege());
	}
}
