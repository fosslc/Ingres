/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qsplittr.cpp, Implementation file
**    Project  : VDBA
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Splitter that can prevent user from tracking.
**
** History:
**
** xx-Mar-1996 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 09-Apr-2003 (uk$so01)
**    SIR #109718, Enhance the library. Add UserDragged(), it will return TRUE
**    if user has dragged the splitter bar. The m_bUserDrags will be set to TRUE
**    on the mouse move if the mouse's left button is down.
*/

#include "stdafx.h"
#include "usplittr.h"
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CuUntrackableSplitterWnd, CSplitterWnd)

CuUntrackableSplitterWnd::CuUntrackableSplitterWnd()
{
	m_bCanTrack = TRUE;
	m_bUserDrags= FALSE;
}

CuUntrackableSplitterWnd::~CuUntrackableSplitterWnd()
{
}


BOOL CuUntrackableSplitterWnd::ReplaceView(int row, int col, CRuntimeClass* pViewClass, SIZE size)
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

BEGIN_MESSAGE_MAP(CuUntrackableSplitterWnd, CSplitterWnd)
	//{{AFX_MSG_MAP(CuUntrackableSplitterWnd)
	ON_WM_LBUTTONDOWN()
	ON_WM_SETCURSOR()
	ON_WM_MOUSEMOVE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuUntrackableSplitterWnd message handlers


// The next three handlers will prevent the user from moving the 
// scrollbars.
void CuUntrackableSplitterWnd::OnLButtonDown(UINT nFlags, CPoint point)
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

BOOL CuUntrackableSplitterWnd::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
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

void CuUntrackableSplitterWnd::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_bCanTrack)
	{
		CSplitterWnd::OnMouseMove(nFlags, point);
		if ((nFlags & MK_LBUTTON) != 0)
			m_bUserDrags = TRUE;
	}
	else
	{
		//
		// Don't allow the cursor to change over splitbar
		CWnd::OnMouseMove(nFlags, point);
	}
}

