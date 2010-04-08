/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : vevsettl.cpp , Implementation File                                    //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II / Visual Manager                                            //
//    Author   : Sotheavut UK (uk$so01)                                                //
//                                                                                     //
//                                                                                     //
//    Purpose  : View of the left pane of Event Setting Frame                          //
****************************************************************************************/
/* History:
* --------
* uk$so01: (21-May-1999 created)
*
*
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "vevsettl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CvEventSettingLeft

IMPLEMENT_DYNCREATE(CvEventSettingLeft, CView)

CvEventSettingLeft::CvEventSettingLeft()
{
	m_pDlg = NULL;
}

CvEventSettingLeft::~CvEventSettingLeft()
{
}


BEGIN_MESSAGE_MAP(CvEventSettingLeft, CView)
	//{{AFX_MSG_MAP(CvEventSettingLeft)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CvEventSettingLeft drawing

void CvEventSettingLeft::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CvEventSettingLeft diagnostics

#ifdef _DEBUG
void CvEventSettingLeft::AssertValid() const
{
	CView::AssertValid();
}

void CvEventSettingLeft::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CvEventSettingLeft message handlers

int CvEventSettingLeft::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	try
	{
		CRect r;
		GetClientRect (r);
		m_pDlg = new CuDlgEventSettingLeft(this);
		m_pDlg->Create (IDD_EVENT_SETTING_LEFT, this);
		m_pDlg->MoveWindow (r);
		m_pDlg->ShowWindow (SW_SHOW);
	}
	catch (...)
	{
		AfxMessageBox (_T("System error (CvEventSettingLeft::OnCreate): \nFail to create pane."));
	}

	return 0;
}

void CvEventSettingLeft::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	if (m_pDlg && IsWindow (m_pDlg->m_hWnd))
	{
		CRect r;
		GetClientRect (r);
		m_pDlg->MoveWindow (r);
	}
}
