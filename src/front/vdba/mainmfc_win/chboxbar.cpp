// ChBoxBar.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "chboxbar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuCheckBoxBar

CuCheckBoxBar::CuCheckBoxBar()
{
}

CuCheckBoxBar::~CuCheckBoxBar()
{
}


BEGIN_MESSAGE_MAP(CuCheckBoxBar, CButton)
    //{{AFX_MSG_MAP(CuCheckBoxBar)
    ON_WM_CREATE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuCheckBoxBar message handlers

BOOL CuCheckBoxBar::PreCreateWindow(CREATESTRUCT& cs) 
{
    if (!(cs.style & BS_AUTOCHECKBOX))
        cs.style |= BS_AUTOCHECKBOX;
    return CButton::PreCreateWindow(cs);
}

int CuCheckBoxBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
    if (CButton::OnCreate(lpCreateStruct) == -1)
        return -1;
    
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    if (hFont == NULL)
        hFont = (HFONT)GetStockObject(ANSI_VAR_FONT);
      SendMessage(WM_SETFONT, (WPARAM)hFont);
    return 0;
}
