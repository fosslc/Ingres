/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : urpectrl.cpp , Implementation File                                    //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II / VDBA                                                      //
//    Author   : Sotheavut UK (uk$so01)                                                //
//                                                                                     //
//                                                                                     //
//    Purpose  : Dialog box for Create/Alter User (List control of privileges)         //
****************************************************************************************/
/* History:
* --------
* uk$so01: (26-Oct-1999 created)
*          Rewrite the dialog code, in C++/MFC.
* drivi01: (10-Mar-2010)
*	   When User permissions are checked, check default permissions automatically.
*	   The user permissions are renamed to "Active by Request".
*	   The default permissions are renamed to "Active by Default".
*
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "urpectrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CuEditableListCtrlURPPrivileges::CuEditableListCtrlURPPrivileges()
{
	m_bReadOnly = FALSE;
}

CuEditableListCtrlURPPrivileges::~CuEditableListCtrlURPPrivileges()
{
}


BEGIN_MESSAGE_MAP(CuEditableListCtrlURPPrivileges, CuListCtrl)
	//{{AFX_MSG_MAP(CuEditableListCtrlURPPrivileges)
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuEditableListCtrlURPPrivileges message handlers


void CuEditableListCtrlURPPrivileges::OnLButtonDown(UINT nFlags, CPoint point) 
{
	if (m_bReadOnly)
		return;
	CuListCtrl::OnLButtonDown(nFlags, point);
}

void CuEditableListCtrlURPPrivileges::DisplayItem(int nItem)
{
	ASSERT (nItem != -1);
	CaURPPrivilegesItemData* pItem = (CaURPPrivilegesItemData*)GetItemData(nItem);
	ASSERT (pItem);
	if (!pItem)
		return;

	SetItemText (nItem, 0, pItem->m_strPrivilege);
	SetCheck    (nItem, 1, pItem->m_bUser);
	SetCheck    (nItem, 2, pItem->m_bDefault);
}


void CuEditableListCtrlURPPrivileges::DisplayPrivileges(int nItem)
{
	if (nItem != -1)
		DisplayItem (nItem);
	else
	{
		int i, nCount = GetItemCount();
		for (i = 0; i<nCount; i++)
			DisplayItem (i);
	}
}


void CuEditableListCtrlURPPrivileges::OnCheckChange (int iItem, int iSubItem, BOOL bCheck)
{
	try
	{
		CaURPPrivilegesItemData* pData = (CaURPPrivilegesItemData*)GetItemData(iItem);
		ASSERT (pData);
		if (!pData)
			return;
		switch (iSubItem)
		{
		case 1: // User
			if (!bCheck)
				pData->m_bDefault = FALSE;
			pData->m_bUser = bCheck;
			pData->m_bDefault = bCheck;
			break;
		case 2: // Default
			if (!pData->m_bUser)
			{
				pData->m_bDefault = FALSE;
				break;
			}
			pData->m_bDefault = bCheck;
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
		AfxMessageBox (_T("System error: CuEditableListCtrlURPPrivileges::OnCheckChange"));
	}
}
