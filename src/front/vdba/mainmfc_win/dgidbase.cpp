/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : DgIDbase.cpp, Implementation file.                                    //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Page of Table control: Detail page of Database                        //              
//                                                                                     //
****************************************************************************************/

#include "stdafx.h"
#include "mainmfc.h"
#include "dgidbase.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CuDlgIpmDetailDatabase, CFormView)

CuDlgIpmDetailDatabase::CuDlgIpmDetailDatabase()
	: CFormView(CuDlgIpmDetailDatabase::IDD)
{
	//{{AFX_DATA_INIT(CuDlgIpmDetailDatabase)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CuDlgIpmDetailDatabase::~CuDlgIpmDetailDatabase()
{

}

void CuDlgIpmDetailDatabase::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmDetailDatabase)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgIpmDetailDatabase, CFormView)
	//{{AFX_MSG_MAP(CuDlgIpmDetailDatabase)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmDetailDatabase diagnostics

#ifdef _DEBUG
void CuDlgIpmDetailDatabase::AssertValid() const
{
	CFormView::AssertValid();
}

void CuDlgIpmDetailDatabase::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmDetailDatabase message handlers


void CuDlgIpmDetailDatabase::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();
}

void CuDlgIpmDetailDatabase::OnSize(UINT nType, int cx, int cy) 
{
	CFormView::OnSize(nType, cx, cy);
}


LONG CuDlgIpmDetailDatabase::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	ResetDisplay();
	return 0L;
}

void CuDlgIpmDetailDatabase::ResetDisplay()
{
}