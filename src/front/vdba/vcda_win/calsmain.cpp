/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   
*/

/*
**    Source   : calsmain.cpp : implementation file
**    Project  : INGRES II/ Visual Configuration Diff Control (vcda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : List of differences for the main parameters
**
** History:
**
** 02-Oct-2002 (uk$so01)
**    Created
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
** 22-Apr-2005 (komve01)
**    BUG #114371/Issue#13864404, Added a handler for WM_RBUTTONDOWN to prevent 
**    the parameters being edited and avoid the default behavior of the parent class.
*/

#include "stdafx.h"
#include "vcda.h"
#include "calsmain.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuEditableListCtrlMain::CuEditableListCtrlMain()
{
}

CuEditableListCtrlMain::~CuEditableListCtrlMain()
{
}


BEGIN_MESSAGE_MAP(CuEditableListCtrlMain, CuEditableListCtrl)
	//{{AFX_MSG_MAP(CuEditableListCtrlMain)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
	ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuEditableListCtrlMain message handlers
void CuEditableListCtrlMain::OnCheckChange (int iItem, int iSubItem, BOOL bCheck)
{
	CWnd* pParent = GetParent();
	if (pParent)
		pParent->SendMessage(WMUSRMSG_UPDATEDATA, (WPARAM)bCheck, (LPARAM)iItem);
}

void CuEditableListCtrlMain::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	/*
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
	*/
}
//Handler for the right button down of the main parameters list control
void CuEditableListCtrlMain::OnRButtonDown(UINT nFlags, CPoint point) 
{
	//Do nothing on a RButtonDown
	//1. We do not want to make them editable
	//2. And hence we will neither display an editbox or a spin control
}

