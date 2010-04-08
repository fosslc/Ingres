/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : qsplittr.cpp, Implementation file                                     //
//                                                                                     //
//                                                                                     //
//    Project  :                                                                       //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : Splitter that can prevent user from tracking.                         //
//                                                                                     //
****************************************************************************************/

#include "stdafx.h"
#include "qsplittr.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuQueryMainSplitterWnd

IMPLEMENT_DYNCREATE(CuQueryMainSplitterWnd, CSplitterWnd)

CuQueryMainSplitterWnd::CuQueryMainSplitterWnd()
{
	m_bCanTrack = TRUE;
}

CuQueryMainSplitterWnd::~CuQueryMainSplitterWnd()
{
}


BOOL CuQueryMainSplitterWnd::ReplaceView(int row, int col, CRuntimeClass* pViewClass, SIZE size)
{
	CCreateContext context;
	BOOL bSetActive;

	if ((GetPane(row,col)->IsKindOf(pViewClass))==TRUE)
		return FALSE;
	
	// Get pointer to CDocument object so that it can be used in the creation 
	// process of the new view
	CDocument * pDoc= ((CView *)GetPane(row,col))->GetDocument();
	CView * pActiveView=GetParentFrame()->GetActiveView();
	if (pActiveView == NULL || pActiveView == GetPane(row, col))
		bSetActive = TRUE;
	else
		bSetActive = FALSE;
	
	//
	// set flag so that document will not be deleted when view is destroyed
	pDoc->m_bAutoDelete = FALSE;
	//
	// Delete existing view 
	((CView *) GetPane(row,col))->DestroyWindow();
	//
	// set flag back to default 
	pDoc->m_bAutoDelete = TRUE;
	
	//
	// Create new view
	context.m_pNewViewClass   = pViewClass;
	context.m_pCurrentDoc     = pDoc;
	context.m_pNewDocTemplate = NULL;
	context.m_pLastView       = NULL;
	context.m_pCurrentFrame   = NULL;
	CreateView (row, col, pViewClass, size, &context);

	CView* pNewView = (CView *)GetPane(row, col);
	
	if (bSetActive == TRUE)
		GetParentFrame()->SetActiveView (pNewView);
	
	RecalcLayout(); 
	GetPane(row, col)->SendMessage(WM_PAINT);
	return TRUE;
}

BEGIN_MESSAGE_MAP(CuQueryMainSplitterWnd, CSplitterWnd)
	//{{AFX_MSG_MAP(CuQueryMainSplitterWnd)
	ON_WM_LBUTTONDOWN()
	ON_WM_SETCURSOR()
	ON_WM_MOUSEMOVE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuQueryMainSplitterWnd message handlers


// The next three handlers will prevent the user from moving the 
// scrollbars.
void CuQueryMainSplitterWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_bCanTrack)
		CSplitterWnd::OnLButtonDown(nFlags, point);
	else
	{
		//
		// prevent the user from dragging the splitter bar
		return;
	}
}

BOOL CuQueryMainSplitterWnd::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (m_bCanTrack)
		return CSplitterWnd::OnSetCursor(pWnd, nHitTest, message);
	else
	{
		//
		// Don't allow the cursor to change over splitbar
		return FALSE;
	}
}

void CuQueryMainSplitterWnd::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_bCanTrack)
		CSplitterWnd::OnMouseMove(nFlags, point);
	else
	{
		//
		// Don't allow the cursor to change over splitbar
		CWnd::OnMouseMove(nFlags, point);
	}
}
