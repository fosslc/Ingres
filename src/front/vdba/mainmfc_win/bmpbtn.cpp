/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : bmpbtn.cpp, Implementation file 
**    Project  : CA-OpenIngres/Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Owner draw button does not response to the key SPACE, RETURN
**               when the button has focus.
** 
** Histoty:
** xx-Feb-1997 (uk$so01)
**    Created.
** 26-Fev-2002 (uk$so01)
**    SIR #106648, Integrate ipm.ocx.
*/

#include "stdafx.h"
#include "bmpbtn.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuBitmapButton::CuBitmapButton()
{
}

CuBitmapButton::~CuBitmapButton()
{
}


BEGIN_MESSAGE_MAP(CuBitmapButton, CButton)
	//{{AFX_MSG_MAP(CuBitmapButton)
	ON_WM_GETDLGCODE()
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuBitmapButton message handlers

UINT CuBitmapButton::OnGetDlgCode() 
{
	return DLGC_WANTALLKEYS;
}

void CuBitmapButton::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if (nChar == VK_SPACE || nChar == VK_RETURN)
		return;
	CButton::OnKeyDown(nChar, nRepCnt, nFlags);
}

CSize GetToolBarButtonSize(CWnd* pWnd, UINT nToolBarID)
{
	CSize buttonSize (0, 0);
	CToolBar tb;
	DWORD dwStyle = WS_CHILD | CBRS_TOP;
	if (tb.Create(pWnd, dwStyle, -1))
	{
		if (!tb.LoadToolBar(nToolBarID))
		{
			ASSERT(FALSE);
		}
	}
	else
	{
		ASSERT(FALSE);
	}

	DWORD dwSize = tb.GetToolBarCtrl().GetButtonSize();
	buttonSize.cx = LOWORD(dwSize);
	buttonSize.cy = HIWORD(dwSize);
	tb.DestroyWindow();
	return buttonSize;
}
