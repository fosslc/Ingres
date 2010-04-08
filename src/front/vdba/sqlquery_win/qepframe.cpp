/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qepframe.cpp, Implementation file 
**    Project  : CA-OpenIngres Visual DBA. 
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : The Frame for drawing the QEP's Tree.
**
** History:
**
** xx-Oct97 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "qepframe.h"
#include "qepdoc.h"
#include "qepview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



BEGIN_MESSAGE_MAP(CfQueryExecutionPlanFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CfQueryExecutionPlanFrame)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


CfQueryExecutionPlanFrame::CfQueryExecutionPlanFrame()
{
	m_pQueryExecutionPlanView = NULL;
	m_pDoc = NULL;
}

CfQueryExecutionPlanFrame::CfQueryExecutionPlanFrame(CdQueryExecutionPlanDoc* pDoc)
{
	m_pQueryExecutionPlanView = NULL;
	m_pDoc = pDoc;
}

CfQueryExecutionPlanFrame::~CfQueryExecutionPlanFrame()
{
}

BOOL CfQueryExecutionPlanFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CFrameWnd::PreCreateWindow(cs);
}

void CfQueryExecutionPlanFrame::SetMode (BOOL bPreview, POSITION posSequence)
{
	ASSERT (m_pDoc && posSequence);
	if (!m_pDoc || !posSequence)
		return;
	CaSqlQueryExecutionPlanData* pData = m_pDoc->m_listQepData.GetAt (posSequence);
	ASSERT (pData);
	if (!pData)
		return;
	pData->SetDisplayMode (bPreview);
	if (m_pQueryExecutionPlanView) 
	{
		m_pQueryExecutionPlanView->InitialDrawing ();
		m_pQueryExecutionPlanView->Invalidate();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CfQueryExecutionPlanFrame diagnostics

#ifdef _DEBUG
void CfQueryExecutionPlanFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CfQueryExecutionPlanFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG


int CfQueryExecutionPlanFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	return 0;
}

BOOL CfQueryExecutionPlanFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
	CCreateContext context1;
	ASSERT (m_pDoc);
	if (!m_pDoc)
	{
		AfxMessageBox (IDS_MSG_FAIL_2_CREATEBIN_TREE);
		return FALSE;
	}

	CRuntimeClass* pView1 = RUNTIME_CLASS (CvQueryExecutionPlanView);
	context1.m_pNewViewClass = pView1;
	context1.m_pCurrentDoc   = m_pDoc;
	return CFrameWnd::OnCreateClient(lpcs, &context1);
}
