/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : view.cpp : implementation of the CvChildView class
**    Project  : Ingres II / IEA
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : View
**
** History:
**
** 16-Oct-2001 (uk$so01)
**    Created
**
**/

#include "stdafx.h"
#include "iea.h"
#include "view.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CvChildView::CvChildView()
{
}

CvChildView::~CvChildView()
{
}


BEGIN_MESSAGE_MAP(CvChildView,CWnd )
	//{{AFX_MSG_MAP(CvChildView)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CvChildView::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (!CWnd::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(
		CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
		::LoadCursor(NULL, IDC_ARROW), 
		HBRUSH(COLOR_WINDOW+1), 
		NULL);

	return TRUE;
}

void CvChildView::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	// TODO: Add your message handler code here
	
	// Do not call CWnd::OnPaint() for painting messages
}

