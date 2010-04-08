/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : ppagedit.cpp : implementation file
**  Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
**  Author   : UK Sotheavut (uk$so01)
**  Purpose  : CuPPageEdit is the property page of the sheet CuPpropertySheet.
**             It has an edit control.
**
** History :
**
** 06-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**    created
**/



#include "stdafx.h"
#include "ppgedit.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CuPPageEdit, CPropertyPage)

CuPPageEdit::CuPPageEdit() : CPropertyPage(CuPPageEdit::IDD)
{
	//{{AFX_DATA_INIT(CuPPageEdit)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CuPPageEdit::~CuPPageEdit()
{
}

void CuPPageEdit::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuPPageEdit)
	DDX_Control(pDX, IDC_RCT_EDIT1, m_cEdit1);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuPPageEdit, CPropertyPage)
	//{{AFX_MSG_MAP(CuPPageEdit)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



void CuPPageEdit::OnSize(UINT nType, int cx, int cy) 
{
	CPropertyPage::OnSize(nType, cx, cy);
	if (!IsWindow (m_cEdit1.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_cEdit1.MoveWindow (r);
}

void CuPPageEdit::SetText (LPCTSTR lpszText)
{
	m_cEdit1.SetWindowText (lpszText);
}
