/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : vevsettb.cpp , Implementation File                                    //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II / Visual Manager                                            //
//    Author   : Sotheavut UK (uk$so01)                                                //
//                                                                                     //
//                                                                                     //
//    Purpose  : View of the bottom pane of Event Setting Frame                        //
****************************************************************************************/
/* History:
* --------
* uk$so01: (21-May-1999 created)
*
*
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "vevsettb.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CvEventSettingBottom

IMPLEMENT_DYNCREATE(CvEventSettingBottom, CView)

CvEventSettingBottom::CvEventSettingBottom()
{
	m_pDlg = NULL;
}

CvEventSettingBottom::~CvEventSettingBottom()
{
}


BEGIN_MESSAGE_MAP(CvEventSettingBottom, CView)
	//{{AFX_MSG_MAP(CvEventSettingBottom)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CvEventSettingBottom drawing

void CvEventSettingBottom::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CvEventSettingBottom diagnostics

#ifdef _DEBUG
void CvEventSettingBottom::AssertValid() const
{
	CView::AssertValid();
}

void CvEventSettingBottom::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CvEventSettingBottom message handlers

int CvEventSettingBottom::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	try
	{
		CRect r;
		GetClientRect (r);
		m_pDlg = new CuDlgEventSettingBottom(this);
		m_pDlg->Create (IDD_EVENT_SETTING_BOTTOM, this);
		m_pDlg->MoveWindow (r);
		m_pDlg->ShowWindow (SW_SHOW);
	}
	catch (...)
	{
		AfxMessageBox (_T("System error (CvEventSettingBottom::OnCreate): \nFail to create pane."));
	}
	
	return 0;
}

void CvEventSettingBottom::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	if (m_pDlg && IsWindow (m_pDlg->m_hWnd))
	{
		CRect r;
		GetClientRect (r);
		m_pDlg->MoveWindow (r);
	}
}
