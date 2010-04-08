/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : mgrfrm.cpp, Implementation file                                       //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II / Visual Manager                                            //
//    Author   : Sotheavut UK (uk$so01)                                                //
//                                                                                     //
//                                                                                     //
//    Purpose  : Frame support to provide the splitter window                          //
****************************************************************************************/
/* History:
* --------
* uk$so01: (15-Dec-1998 created)
*
*
*/

#include "stdafx.h"
#include "ivm.h"
#include "mgrfrm.h"
#include "maintab.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CfManagerFrame::CfManagerFrame(CdMainDoc* pDoc)
{
	ASSERT (pDoc);
	m_pMainDoc = pDoc;
	m_bAllViewCreated = FALSE;
}

CfManagerFrame::~CfManagerFrame()
{
}


BEGIN_MESSAGE_MAP(CfManagerFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CfManagerFrame)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CfManagerFrame message handlers

BOOL CfManagerFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
	ASSERT (m_pMainDoc);
	if (!m_pMainDoc)
	{
		TRACE0 ("Main document is NULL while creating CfManagerFrame\n");
		return FALSE;
	}
	CCreateContext context1, context2;
	CRuntimeClass* pView1 = RUNTIME_CLASS(CvManagerViewLeft);
	CRuntimeClass* pView2 = RUNTIME_CLASS(CvManagerViewRight);
	context1.m_pNewViewClass = pView1;
	context1.m_pCurrentDoc   = m_pMainDoc;
	context2.m_pNewViewClass = pView2;
	context2.m_pCurrentDoc   = context1.m_pCurrentDoc;
	if (!m_Splitterwnd.CreateStatic (this, 1, 2)) 
	{
		TRACE0 ("Failed to create Splitter\n");
		return FALSE;
	}
	if (!m_Splitterwnd.CreateView (0, 0, pView1, CSize (306, 400), &context1)) 
	{
		TRACE0 ("Failed to create first pane\n");
		return FALSE;
	}
	if (!m_Splitterwnd.CreateView (0, 1, pView2, CSize (400, 400), &context2)) 
	{
		TRACE0 (" Failed to create second pane\n");
		return FALSE;
	}

	m_Splitterwnd.SetActivePane( 0,0); 
	CWnd* pWnd = m_Splitterwnd.GetPane (0,0);
	pWnd->Invalidate();
	m_Splitterwnd.RecalcLayout();
	m_bAllViewCreated = TRUE;

	return TRUE;
}

void CfManagerFrame::OnSize(UINT nType, int cx, int cy) 
{
	CFrameWnd::OnSize(nType, cx, cy);
}

CvManagerViewLeft* CfManagerFrame::GetLeftView()
{
	ASSERT (m_bAllViewCreated);
	if (!m_bAllViewCreated)
		return NULL;
	CvManagerViewLeft* pView =(CvManagerViewLeft*)m_Splitterwnd.GetPane (0, 0);
	ASSERT_VALID(pView);
	return pView;
}

CvManagerViewRight* CfManagerFrame::GetRightView()
{
	ASSERT (m_bAllViewCreated);
	if (!m_bAllViewCreated)
		return NULL;
	CvManagerViewRight* pView =(CvManagerViewRight*)m_Splitterwnd.GetPane (0, 1);
	ASSERT_VALID(pView);
	return pView;
}


UINT CfManagerFrame::GetHelpID()
{
	CvManagerViewRight* pViewRight = GetRightView();
	if (!pViewRight)
		return 0;

	CuMainTabCtrl* pTab = pViewRight->GetTabCtrl();
	return pTab? pTab->GetHelpID(): 0;
}
