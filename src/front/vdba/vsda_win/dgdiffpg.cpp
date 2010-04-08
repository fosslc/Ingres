/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dgdiffpg.cpp : implementation file
**    Project  : INGRESII/VDDA (vsda.ocx)
**    Author   : UK Sotheavut
**    Purpose  : Use the modeless dialog that has
**               - staic control (Header of the difference)
**               - edit multiline(The text of the difference)
**               This dialog is a child of CvDifferenceDetail and used 
**               for the pop-ud detail.
**
** History:
**
** 17-May-2004 (uk$so01)
**    SIR #109220, ISSUE 13407277: additional change to put the header
**    in the pop-up for an item's detail of difference.
*/

#include "stdafx.h"
#include "vsda.h"
#include "dgdiffpg.h"
#include ".\dgdiffpg.h"


// CuDlgDifferenceDetailPage dialog

IMPLEMENT_DYNAMIC(CuDlgDifferenceDetailPage, CDialog)
CuDlgDifferenceDetailPage::CuDlgDifferenceDetailPage(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgDifferenceDetailPage::IDD, pParent)
	, m_strEdit1(_T(""))
	, m_strStatic1(_T(""))
{
}

CuDlgDifferenceDetailPage::~CuDlgDifferenceDetailPage()
{
}

void CuDlgDifferenceDetailPage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_strEdit1);
	DDX_Text(pDX, IDC_STATIC1, m_strStatic1);
	DDX_Control(pDX, IDC_STATIC1, m_cStatic1);
	DDX_Control(pDX, IDC_EDIT1, m_cEdit1);
}


BEGIN_MESSAGE_MAP(CuDlgDifferenceDetailPage, CDialog)
	ON_WM_SIZE()
END_MESSAGE_MAP()


// CuDlgDifferenceDetailPage message handlers

void CuDlgDifferenceDetailPage::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);
	if (IsWindow(m_cEdit1.m_hWnd))
	{
		CRect r, r1;
		GetClientRect (r);
		m_cEdit1.GetWindowRect(r1);
		ScreenToClient (r1);
		r1.left = r.left;
		r1.right= r.right;
		r1.bottom = r.bottom;
		m_cEdit1.MoveWindow(r1);

		m_cStatic1.GetWindowRect(r1);
		ScreenToClient (r1);
		r1.left = r.left;
		r1.right= r.right;
		m_cStatic1.MoveWindow(r1);
	}
}

void CuDlgDifferenceDetailPage::PostNcDestroy()
{
	delete this;
	CDialog::PostNcDestroy();
}
