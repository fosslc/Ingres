// ComboBar.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "combobar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuComboToolBar

CuComboToolBar::CuComboToolBar()
{
}

CuComboToolBar::~CuComboToolBar()
{
}


BEGIN_MESSAGE_MAP(CuComboToolBar, CComboBox)
	//{{AFX_MSG_MAP(CuComboToolBar)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuComboToolBar message handlers

int CuComboToolBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CComboBox::OnCreate(lpCreateStruct) == -1)
		return -1;
	
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	if (hFont == NULL)
		hFont = (HFONT)GetStockObject(ANSI_VAR_FONT);
  	SendMessage(WM_SETFONT, (WPARAM)hFont);
	return 0;
}
