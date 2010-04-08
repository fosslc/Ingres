/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : cachefrm.cpp, Implementation file                                     //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : Frame to provide splitter of the DBMS Cache Page                      //
//               See the CONCEPT.DOC for more detail                                   //
****************************************************************************************/
#include "stdafx.h"
#include "vcbf.h"
#include "cachefrm.h"
#include "cachevi1.h" // View: Left  Pane
#include "cachevi2.h" // View: Right Pane
#include "cachedoc.h" // Document for the view.

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CfDbmsCacheFrame


CfDbmsCacheFrame::CfDbmsCacheFrame()
{
	m_bAllViewCreated = FALSE;
}

CfDbmsCacheFrame::~CfDbmsCacheFrame()
{
}


BEGIN_MESSAGE_MAP(CfDbmsCacheFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CfDbmsCacheFrame)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CfDbmsCacheFrame message handlers

BOOL CfDbmsCacheFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
	CCreateContext context1, context2;
	CRuntimeClass* pView1 = RUNTIME_CLASS (CvDbmsCacheViewLeft);
	CRuntimeClass* pView2 = RUNTIME_CLASS (CvDbmsCacheViewRight);
	context1.m_pNewViewClass = pView1;
	context1.m_pCurrentDoc   = new CdCacheDoc;
	context2.m_pNewViewClass = pView2;
	context2.m_pCurrentDoc   = context1.m_pCurrentDoc;
	if (!m_Splitterwnd.CreateStatic (this, 1, 2)) 
	{
		TRACE0 ("Failed to create Splitter\n");
		return FALSE;
	}
	if (!m_Splitterwnd.CreateView (0, 0, pView1, CSize (220, 300), &context1)) 
	{
		TRACE0 ("Failed to create first pane\n");
		return FALSE;
	}
	if (!m_Splitterwnd.CreateView (0, 1, pView2, CSize (300, 300), &context2)) 
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

void CfDbmsCacheFrame::OnSize(UINT nType, int cx, int cy) 
{
	CFrameWnd::OnSize(nType, cx, cy);
}
