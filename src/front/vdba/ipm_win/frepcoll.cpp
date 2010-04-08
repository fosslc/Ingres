/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : frepcoll.cpp, Implementation file
**    Project  : INGRES II/ Replication Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Frame to provide splitter of the Replication Page for Collision
**
** History:
**
** xx-Sep-1997 (uk$so01)
**    Created
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "frepcoll.h"
#include "vrepcolf.h" // View: Left  Pane (Tree View)
#include "vrepcort.h" // View: Right Pane (View that contains the modeless Dialog)
#include "repcodoc.h" // Document for both views.

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



CfReplicationPageCollision::CfReplicationPageCollision()
{
	m_bAllViewCreated = FALSE;
}

CfReplicationPageCollision::~CfReplicationPageCollision()
{
}


BEGIN_MESSAGE_MAP(CfReplicationPageCollision, CFrameWnd)
	//{{AFX_MSG_MAP(CfReplicationPageCollision)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CfReplicationPageCollision message handlers

BOOL CfReplicationPageCollision::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
	CCreateContext context1, context2;
	CRuntimeClass* pView1 = RUNTIME_CLASS (CvReplicationPageCollisionViewLeft);
	CRuntimeClass* pView2 = RUNTIME_CLASS (CvReplicationPageCollisionViewRight);
	context1.m_pNewViewClass = pView1;
	context1.m_pCurrentDoc   = new CdReplicationPageCollisionDoc ;
	context2.m_pNewViewClass = pView2;
	context2.m_pCurrentDoc   = context1.m_pCurrentDoc;
	if (!m_Splitterwnd.CreateStatic (this, 1, 2)) 
	{
		TRACE0 ("Failed to create Splitter\n");
		return FALSE;
	}
	if (!m_Splitterwnd.CreateView (0, 0, pView1, CSize (220, 300), &context1)) 
	{
		TRACE0 ("CfReplicationPageCollision::Failed to create first pane\n");
		return FALSE;
	}
	if (!m_Splitterwnd.CreateView (0, 1, pView2, CSize (300, 300), &context2)) 
	{
		TRACE0 ("CfReplicationPageCollision::Failed to create second pane\n");
		return FALSE;
	}
	
	m_Splitterwnd.SetActivePane( 0,0); 
	CWnd* pWnd = m_Splitterwnd.GetPane (0,0);
	pWnd->Invalidate();
	m_Splitterwnd.RecalcLayout();
	m_bAllViewCreated = TRUE;
	return TRUE;
}

void CfReplicationPageCollision::OnSize(UINT nType, int cx, int cy) 
{
	CFrameWnd::OnSize(nType, cx, cy);
}
