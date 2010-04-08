/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : frdtdiff.cpp : implementation file
**    Project  : INGRES II/VSDA Control (vsda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Frame for detail difference page
**
** History:
**
** 10-Dec-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
*/

#include "stdafx.h"
#include "vsda.h"
#include "frdtdiff.h"
#include "dcdtdiff.h"
#include "vwdtdiff.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CfDifferenceDetail, CFrameWnd)
CfDifferenceDetail::CfDifferenceDetail()
{
	m_pDoc = NULL;
	m_bAllViewCreated = FALSE;
}

CfDifferenceDetail::~CfDifferenceDetail()
{
}

CvDifferenceDetail* CfDifferenceDetail::GetLeftPane()
{
	return (CvDifferenceDetail*)m_WndSplitter.GetPane (0, 0);
}

CvDifferenceDetail* CfDifferenceDetail::GetRightPane ()
{
	return (CvDifferenceDetail*)m_WndSplitter.GetPane (0, 1);
}



BEGIN_MESSAGE_MAP(CfDifferenceDetail, CFrameWnd)
	//{{AFX_MSG_MAP(CfDifferenceDetail)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CfDifferenceDetail message handlers

BOOL CfDifferenceDetail::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
	BOOL bOK = TRUE;
	CCreateContext context1, context2;
	int ncxCur = 200, ncxMin = 10;

	if (!m_pDoc)
		m_pDoc  = new CdDifferenceDetail();
	CRuntimeClass* pViewRight= RUNTIME_CLASS(CvDifferenceDetail);
	CRuntimeClass* pViewLeft = RUNTIME_CLASS(CvDifferenceDetail);
	context1.m_pNewViewClass = pViewLeft;
	context1.m_pCurrentDoc   = m_pDoc;
	context2.m_pNewViewClass = pViewRight;
	context2.m_pCurrentDoc   = context1.m_pCurrentDoc;

	// Create a splitter of 1 rows and 2 columns.
	if (!m_WndSplitter.CreateStatic (this, 1, 2)) 
	{
		TRACE0 ("CfDifferenceDetail::OnCreateClient: Failed to create Splitter\n");
		return FALSE;
	}

	// associate the default view (CvSdaLeft) with pane 0
	if (!m_WndSplitter.CreateView (0, 0, context1.m_pNewViewClass, CSize (200, 300), &context1)) 
	{
		TRACE0 ("CfDifferenceDetail::OnCreateClient: Failed to create first pane\n");
		return FALSE;
	}

	// associate the CvSdaRight view with pane 1
	if (!m_WndSplitter.CreateView (0, 1, context2.m_pNewViewClass, CSize (200, 300), &context2)) 
	{
		TRACE0 ("CfDifferenceDetail::OnCreateClient: Failed to create second pane\n");
		return FALSE;
	}

	m_WndSplitter.SetColumnInfo(0, ncxCur, ncxMin);
	m_WndSplitter.RecalcLayout();
	
	m_bAllViewCreated = TRUE;
	return bOK;
}
