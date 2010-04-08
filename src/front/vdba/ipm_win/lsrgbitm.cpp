/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : lsrgbitm.cpp, Implementation file 
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : List control that allows the items to have foreground and background
**               colors
**
** History:
**
** xx-Sep-1998 (uk$so01)
**    Created
*/


#include "stdafx.h"
#include "lsrgbitm.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuListCtrlColorItem::CuListCtrlColorItem():CuListCtrl()
{
	m_bLineSeperator    = FALSE;
}

CuListCtrlColorItem::~CuListCtrlColorItem()
{
}


BEGIN_MESSAGE_MAP(CuListCtrlColorItem, CuListCtrl)
	ON_WM_CONTEXTMENU()
	//{{AFX_MSG_MAP(CuListCtrlColorItem)
	ON_WM_RBUTTONUP()
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



void CuListCtrlColorItem::OnRButtonUp(UINT nFlags, CPoint point) 
{
	CuListCtrl::OnRButtonUp(nFlags, point);
}

void CuListCtrlColorItem::OnLButtonDown(UINT nFlags, CPoint point) 
{
	return;
}
