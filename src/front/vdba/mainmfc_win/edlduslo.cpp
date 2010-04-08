/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : edlduslo.cpp: implementation of the 
**               CuEditableListCtrlDuplicateDbSelectLocation class.
**    Project  : Ingres II/Vdba.
**    Author   : Schalk Philippe
** 
**    Purpose  : Editable List control to provide the new location.
**               Used in Dialog box for generating remote command 
**               relocatedb (duplicate database)
**
** History:
** xx-Oct-1998 (Schalk Philippe)
**    Created
** 28-Mar-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
*/

#include "stdafx.h"

#include "rcdepend.h"
#include "edlduslo.h"
#include "dgerrh.h"

extern "C" {
#include "dba.h"
#include "dbaset.h"
#include "main.h"
#include "domdata.h"
#include "getdata.h"
}

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CuEditableListCtrlDuplicateDbSelectLocation::CuEditableListCtrlDuplicateDbSelectLocation()
{

}

CuEditableListCtrlDuplicateDbSelectLocation::~CuEditableListCtrlDuplicateDbSelectLocation()
{

}

BEGIN_MESSAGE_MAP(CuEditableListCtrlDuplicateDbSelectLocation, CuEditableListCtrl)
	//{{AFX_MSG_MAP(CuEditableListCtrlDuplicateDbSelectLocation)
	ON_WM_DESTROY()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
	//ON_NOTIFY_REFLECT(LVN_DELETEITEM, OnDeleteitem)
	// compilation failed on unix (MAINWIN).Not used in this class.
	//ON_MESSAGE (WM_LAYOUTEDITDLG_OK,  OnEditDlgOK)
	ON_MESSAGE (WM_LAYOUTCOMBODLG_OK, OnComboDlgOK)
END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////
// CuEditableListCtrlDuplicateDbSelectLocation message handlers

void CuEditableListCtrlDuplicateDbSelectLocation::OnDestroy() 
{
	CuEditableListCtrl::OnDestroy();
}

void CuEditableListCtrlDuplicateDbSelectLocation::OnLButtonDblClk(UINT nFlags, CPoint point) 
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

void CuEditableListCtrlDuplicateDbSelectLocation::OnRButtonDblClk(UINT nFlags, CPoint point) 
{
	CuEditableListCtrl::OnRButtonDblClk(nFlags, point);
}

void CuEditableListCtrlDuplicateDbSelectLocation::OnRButtonDown(UINT nFlags, CPoint point) 
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

void CuEditableListCtrlDuplicateDbSelectLocation::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	CuEditableListCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}

LONG CuEditableListCtrlDuplicateDbSelectLocation::OnComboDlgOK (UINT wParam, LONG lParam)
{
	int iItem, iSubItem;
	CString s = COMBO_GetText();
	COMBO_GetEditItem(iItem, iSubItem);
	if (iItem < 0)
	{
		SetFocus();
		return 0L;
	}
	try
	{
		SetItemText (iItem, NEW_LOC,s);
		GetParent()->SendMessage (WM_LAYOUTCOMBODLG_OK, (WPARAM)(LPCTSTR)m_ComboDlgOriginalText, (LPARAM)(LPCTSTR)s);
	}
	catch (...)
	{
		//CString strMsg = _T("Cannot change combo text.");
		AfxMessageBox (VDBA_MfcResourceString(IDS_E_CHANGE_COMBO));
	}

	SetFocus();
	return 0L;
}

void CuEditableListCtrlDuplicateDbSelectLocation::EditValue (int iItem, int iSubItem, CRect rcCell)
{
	if (iSubItem < 1)
		return;

	CString strItem = GetItemText (iItem, iSubItem);
	strItem.TrimLeft();
	strItem.TrimRight();
	CComboBox* pCombo = (CComboBox*)COMBO_GetComboBox();
	int iIndex = -1;

	switch (iSubItem)
	{
	case NEW_LOC:
		InitSortComboBox();
		SetComboBox (iItem, iSubItem, rcCell, (LPCTSTR)strItem);
		break;
	default:
		break;
	}
}

void CuEditableListCtrlDuplicateDbSelectLocation::InitSortComboBox()
{
	int TypeLocation,i,iIndex,err;
	char szObject[MAXOBJECTNAME];
	char szFilter[MAXOBJECTNAME];
	BOOL bOnlyOneLocName;

	CComboBox* pCombo = COMBO_GetComboBox();
	pCombo->ResetContent();
	err = DOMGetFirstObject(GetCurMdiNodeHandle(),
							OT_LOCATION,
							0,
							NULL,
							TRUE,
							NULL,
							(LPUCHAR)szObject,
							NULL,
							NULL);
	while (err == RES_SUCCESS)
	{
		BOOL bOK;
		for (i = 0, TypeLocation = 0,bOnlyOneLocName = TRUE;i <= LOCATIONDUMP;i++,TypeLocation=i)
		{
			if (DOMLocationUsageAccepted(GetCurMdiNodeHandle(),(LPUCHAR)szObject,TypeLocation,&bOK) == RES_SUCCESS && bOK)
			{
				switch (TypeLocation)
				{
					case LOCATIONDATABASE:
					case LOCATIONWORK:
					{
						if ( bOnlyOneLocName )
						{
							iIndex = pCombo->AddString (szObject);
							bOnlyOneLocName = FALSE;
						}
					}
						break;
				}
			}
		}
		err = DOMGetNextObject ((LPUCHAR)szObject,(LPUCHAR)szFilter, NULL);
	}
	if (err != RES_SUCCESS && err != RES_ENDOFDATA)
	{
		//"Error while getting Location list."
		CString csMsg = VDBA_MfcResourceString (IDS_E_DUPLICATEDB_FILL_COMBO_LOCATION);
		MessageWithHistoryButton(m_hWnd,csMsg);
	}

}
