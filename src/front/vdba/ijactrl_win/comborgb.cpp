/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : comborgb.cpp , Implementation File
**    Project  : Ingres Journal Analyser
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Owner draw combo for the RGB Item

** History:
**
** 29-Mar-2000 (uk$so01)
**    Created
**
**/

#include "stdafx.h"
#include "comborgb.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuComboColor

CuComboColor::CuComboColor()
{
}

CuComboColor::~CuComboColor()
{
}

int CuComboColor::AddRgb (COLORREF rgb)
{
	int iIdx = AddString ("");
	if (iIdx != CB_ERR)
		SetItemData (iIdx, (DWORD)rgb);
	return iIdx;
}

COLORREF CuComboColor::GetRgb ()
{
	int iIdx = GetCurSel();
	if (iIdx != CB_ERR)
		return (COLORREF)GetItemData (iIdx);
	return RGB (0, 0, 0);
}

void CuComboColor::SelectItem (COLORREF rgb)
{
	int i, nCount = GetCount();
	for (i=0; i<nCount; i++)
	{
		if ((COLORREF)GetItemData (i) == rgb)
		{
			SetCurSel (i);
			break;
		}
	}
}


BEGIN_MESSAGE_MAP(CuComboColor, CComboBox)
	//{{AFX_MSG_MAP(CuComboColor)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuComboColor message handlers

void CuComboColor::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	CRect r = lpDrawItemStruct->rcItem;
	CBrush br (lpDrawItemStruct->itemData);

	FillRect (lpDrawItemStruct->hDC, (LPRECT)&r, (HBRUSH)br);

	HBRUSH hbr;
	if (lpDrawItemStruct->itemState & ODS_SELECTED)
	{
		//
		// Selecting item -- paint a black frame
		hbr = (HBRUSH)GetStockObject(BLACK_BRUSH);
	}
	else
	{
		//
		// De-selecting item -- remove frame 
		hbr = (HBRUSH)CreateSolidBrush(GetSysColor(COLOR_WINDOW));
	}
	FrameRect(lpDrawItemStruct->hDC, (LPRECT)&r, hbr);
	DeleteObject (hbr);

}

BOOL CuComboColor::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (!(cs.style & CBS_OWNERDRAWFIXED))
		cs.style |= CBS_OWNERDRAWFIXED;
	return CComboBox::PreCreateWindow(cs);
}

void CuComboColor::PreSubclassWindow() 
{
	CComboBox::PreSubclassWindow();
}
