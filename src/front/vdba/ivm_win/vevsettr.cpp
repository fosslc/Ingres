/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : vevsettr.cpp , Implementation File                                    //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II / Visual Manager                                            //
//    Author   : Sotheavut UK (uk$so01)                                                //
//                                                                                     //
//                                                                                     //
//    Purpose  : View of the right pane of Event Setting Frame                         //
****************************************************************************************/
/* History:
* --------
* uk$so01: (21-May-1999 created)
*
*
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "vevsettr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CvEventSettingRight

IMPLEMENT_DYNCREATE(CvEventSettingRight, CView)

CvEventSettingRight::CvEventSettingRight()
{
	m_pDlg = NULL;
}

CvEventSettingRight::~CvEventSettingRight()
{
}


BEGIN_MESSAGE_MAP(CvEventSettingRight, CView)
	//{{AFX_MSG_MAP(CvEventSettingRight)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CvEventSettingRight drawing

void CvEventSettingRight::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CvEventSettingRight diagnostics

#ifdef _DEBUG
void CvEventSettingRight::AssertValid() const
{
	CView::AssertValid();
}

void CvEventSettingRight::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CvEventSettingRight message handlers

int CvEventSettingRight::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	try
	{
		CRect r;
		GetClientRect (r);
		m_pDlg = new CuDlgEventSettingRight(this);
		m_pDlg->Create (IDD_EVENT_SETTING_RIGHT, this);
		m_pDlg->MoveWindow (r);
		m_pDlg->ShowWindow (SW_SHOW);
	}
	catch (...)
	{
		AfxMessageBox (_T("System error (CvEventSettingRight::OnCreate): \nFail to create pane."));
	}
	return 0;
}

void CvEventSettingRight::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	if (m_pDlg && IsWindow (m_pDlg->m_hWnd))
	{
		CRect r;
		GetClientRect (r);
		m_pDlg->MoveWindow (r);
	}
}
