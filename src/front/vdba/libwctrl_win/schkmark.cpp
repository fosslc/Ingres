/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : 2schkmark.cpp : implementation file 
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : control check mark that looks like a check box
**
** History:
**
** 06-Mar-2001 (uk$so01)
**    Created
**/


#include "stdafx.h"
#include "schkmark.h"
#include "rcdepend.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuStaticCheckMark::CuStaticCheckMark()
{
	m_cxImage = 16;
	m_bCreateImage = FALSE;
}

CuStaticCheckMark::~CuStaticCheckMark()
{
}


BEGIN_MESSAGE_MAP(CuStaticCheckMark, CStatic)
	//{{AFX_MSG_MAP(CuStaticCheckMark)
	ON_WM_PAINT()
	ON_WM_DESTROY()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CuStaticCheckMark::OnPaint() 
{
	CPaintDC dc(this);
	if (m_ImageList.m_hImageList)
		m_ImageList.Draw(&dc, 0, CPoint(0, 0), ILD_TRANSPARENT);
}

void CuStaticCheckMark::OnDestroy() 
{
	CStatic::OnDestroy();
}

int CuStaticCheckMark::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CStatic::OnCreate(lpCreateStruct) == -1)
		return -1;
	if (!m_bCreateImage)
	{
		m_ImageList.Create (IDB_CHECKMARKINFO, m_cxImage, 0, RGB(255,0,255));
		m_bCreateImage = TRUE;
	}
	return 0;
}

void CuStaticCheckMark::PreSubclassWindow() 
{
	if (!m_bCreateImage)
	{
		m_ImageList.Create (IDB_CHECKMARKINFO, m_cxImage, 0, RGB(255,0,255));
		m_bCreateImage = TRUE;
	}
	CStatic::PreSubclassWindow();
}
