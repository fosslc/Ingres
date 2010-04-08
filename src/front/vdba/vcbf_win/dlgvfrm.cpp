/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : DlgVFrm.cpp, Implementation file.                                     //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring, Modifying for .                             //
//               OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Frame for CFormView, Scrollable Dialog box of Detail Page.            //              
//                                                                                     //
****************************************************************************************/

#include "stdafx.h"
#include "resource.h"
#include "dlgvfrm.h"
#include "dlgvdoc.h"


#include "dbasedlg.h"  // DBMS Database
#include "logfidlg.h"  // Primary LOG FILE Configure Form view
#include "logfsdlg.h"  // Secondary LOG FILE Configure Form view

#include "wmusrmsg.h"  // NAME Parameter Form View

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



CuDlgViewFrame::CuDlgViewFrame()
{
	m_nDlgID = 0;
}

CuDlgViewFrame::CuDlgViewFrame(UINT nDlgID)
{
	m_nDlgID = nDlgID;
}


CuDlgViewFrame::~CuDlgViewFrame()
{
}


BEGIN_MESSAGE_MAP(CuDlgViewFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CuDlgViewFrame)
	ON_WM_CREATE()
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_UPDATING, OnUpdateData)
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_VALIDATE, OnValidateData)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON1, OnButton1)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON2, OnButton2)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON3, OnButton3)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON4, OnButton4)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON5, OnButton5)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgViewFrame message handlers

int CuDlgViewFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	return 0;
}

BOOL CuDlgViewFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
	CCreateContext context1;
	context1.m_pCurrentDoc   = new CuDlgViewDoc;
	
	switch (m_nDlgID)
	{
	case IDD_DBMS_PAGE_DATABASE:
		context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgDbmsDatabase);
		break;
	case IDD_LOGFILE_PAGE_CONFIGURE:
		context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgLogfileConfigure);
    break;
	case IDD_LOGFILE_PAGE_SECONDARY:
		context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgLogfileSecondary);
		break;
	default:
		break;
	}
	return CFrameWnd::OnCreateClient(lpcs, &context1);
}

LONG CuDlgViewFrame::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	CView* pView = GetActiveView();
	if (pView)
		pView->SendMessage (WMUSRMSG_CBF_PAGE_UPDATING, wParam, lParam);
	return 0L;
}

LONG CuDlgViewFrame::OnValidateData (WPARAM wParam, LPARAM lParam)
{
	CView* pView = GetActiveView();
	if (pView)
		pView->SendMessage (WMUSRMSG_CBF_PAGE_VALIDATE, wParam, lParam);
	return 0L;
}

LONG CuDlgViewFrame::OnButton1 (WPARAM wParam, LPARAM lParam)
{
	CView* pView = GetActiveView();
	if (pView)
		pView->SendMessage (WM_CONFIGRIGHT_DLG_BUTTON1, wParam, lParam);
	return 0L;
}

LONG CuDlgViewFrame::OnButton2 (WPARAM wParam, LPARAM lParam)
{
	CView* pView = GetActiveView();
	if (pView)
		pView->SendMessage (WM_CONFIGRIGHT_DLG_BUTTON2, wParam, lParam);
	return 0L;
}

LONG CuDlgViewFrame::OnButton3 (WPARAM wParam, LPARAM lParam)
{
	CView* pView = GetActiveView();
	if (pView)
		pView->SendMessage (WM_CONFIGRIGHT_DLG_BUTTON3, wParam, lParam);
	return 0L;
}

LONG CuDlgViewFrame::OnButton4 (WPARAM wParam, LPARAM lParam)
{
	CView* pView = GetActiveView();
	if (pView)
		pView->SendMessage (WM_CONFIGRIGHT_DLG_BUTTON4, wParam, lParam);
	return 0L;
}

LONG CuDlgViewFrame::OnButton5 (WPARAM wParam, LPARAM lParam)
{
	CView* pView = GetActiveView();
	if (pView)
		pView->SendMessage (WM_CONFIGRIGHT_DLG_BUTTON5, wParam, lParam);
	return 0L;
}

void CuDlgViewFrame::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CView* pView = GetActiveView();
	int nShow = bShow? SW_SHOW: SW_HIDE;
	if (pView)
		pView->ShowWindow (nShow);
	CFrameWnd::OnShowWindow(bShow, nStatus);
}
