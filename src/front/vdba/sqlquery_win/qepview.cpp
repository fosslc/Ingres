/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qepview.cpp, Implementation file
**    Project  : CA-OpenIngres Visual DBA.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : The view for drawing the QEP's Tree.
**
** History:
**
** xx-Oct-1997 (uk$so01)
**    Created
** 16-feb-2000 (somsa01)
**    In InitialDrawing(), typecasted use of pow(), as platforms
**    such as HP have two versions of it. Also, for some reason
**    on HP, we need to place the "ON_MESSAGE" declarations before
**    "ON_COMMAND" declarations to avoid compilation errors.
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
*/


#include "stdafx.h"
#include <math.h>
#include "rcdepend.h"
#include "qepframe.h"
#include "qepdoc.h"
#include "qepview.h"
#include "qepboxdg.h"
#include "qpageqin.h"
#include "qframe.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static BOOL SetIconAndText(CWnd* pWnd, UINT nIcon, CString strText)
{
	ASSERT (pWnd);
	ASSERT (nIcon);
	
	SetLastError(0);
	LONG lOld = SetWindowLong(pWnd->m_hWnd, GWL_USERDATA, (LONG)nIcon);
	DWORD dwErr = GetLastError();
	ASSERT (!dwErr);
	if (dwErr)
		return FALSE;
	
	CString csCaptionWithSpace = CString(TCHAR(' '), 5);
	csCaptionWithSpace += strText;
		pWnd->SetWindowText (csCaptionWithSpace);
	return TRUE;
}

static BOOL UxCreateFont (CFont& font, LPCTSTR lpFaceName, int size)
{
	LOGFONT lf;
	memset(&lf, 0, sizeof(lf));
	lf.lfHeight        = size; 
	lf.lfWidth         = 0;
	lf.lfEscapement    = 0;
	lf.lfOrientation   = 0;
	lf.lfWeight        = 400;
	lf.lfItalic        = 0;
	lf.lfUnderline     = 0;
	lf.lfStrikeOut     = 0;
	lf.lfCharSet       = 0;
	lf.lfOutPrecision  = 3;
	lf.lfClipPrecision = 2;
	lf.lfQuality       = 22;
	lstrcpy(lf.lfFaceName, lpFaceName);
	return font.CreateFontIndirect(&lf);
}

void CvQueryExecutionPlanView::CalculateDepth (CaSqlQueryExecutionPlanBinaryTree* pQepBinaryTree, int& nDepth, int level)
{
	if (pQepBinaryTree)
	{
		if (nDepth < level)
			nDepth = level;
		CalculateDepth (pQepBinaryTree->m_pQepNodeLeft,  nDepth, level + 1);
		CalculateDepth (pQepBinaryTree->m_pQepNodeRight, nDepth, level + 1);
	}
}

void CvQueryExecutionPlanView::DrawChildStart (CaSqlQueryExecutionPlanBinaryTree* pQepBinaryTree, CDC* pDC, BOOL bPreview)
{
	int  w;
	BOOL bStart = TRUE;
	CRect r;
	CaQepNodeInformation* pNodeInfo = pQepBinaryTree->m_pQepNodeInfo;
	ASSERT (pNodeInfo);
	if (!pNodeInfo)
		return;
	CWnd* pBox = pNodeInfo->GetQepBox(bPreview);
	if (!pBox)
		return;
	pBox->GetWindowRect (r);
	ScreenToClient (r);
	w = r.Width();
	CPoint pCenter(r.left  + w/2,  r.top);
	CPoint pLeft  (pCenter.x - 10, r.bottom);
	CPoint pRight (pCenter.x + 10, r.bottom);

	CPen   pen (PS_SOLID, 1, RGB (0,0,0));
	CPen*  old = pDC->SelectObject (&pen);
	CBrush br (RGB (0,0,0));

	bStart = (pQepBinaryTree->m_pQepNodeLeft || pQepBinaryTree->m_pQepNodeRight);

	CRect rx (pLeft.x, pLeft.y, pLeft.x+5, pLeft.y + 5);
	pDC->DPtoLP (rx);
	if (bStart && pQepBinaryTree->m_pQepNodeLeft)
		pDC->FillRect (rx, &br);
	rx = CRect (pRight.x, pRight.y, pRight.x+5, pRight.y + 5);
	pDC->DPtoLP (rx);
	if (bStart && pQepBinaryTree->m_pQepNodeRight)
		pDC->FillRect (rx, &br);
	
	if (!pNodeInfo->IsRoot())
	{
		rx = CRect (pCenter.x, pCenter.y, pCenter.x+5, pCenter.y - 5);
		pDC->DPtoLP (rx);
		pDC->FillRect (rx, &br);
	}

	pDC->SelectObject (old);
}

void CvQueryExecutionPlanView::DrawQepBoxes (CaSqlQueryExecutionPlanBinaryTree* pQepBinaryTree, CDC* pDC, BOOL bPreview)
{
	if (pQepBinaryTree)
	{
		BOOL bCreateOK = FALSE;
		CaQepNodeInformation* pNodeInfo = pQepBinaryTree->m_pQepNodeInfo;
		pNodeInfo->SetDisplayMode (bPreview);
		CWnd* pQepBox = pNodeInfo->GetQepBox(bPreview);
		BOOL b2Flash = FALSE;
		if (!pQepBox)
		{
			pNodeInfo->AllocateQepBox(bPreview);
			bCreateOK = pNodeInfo->CreateQepBox(this, bPreview);
			if (!bCreateOK)
			{
				CString strMsg = _T("Cannot create the QEP boxes.");
				throw CeSqlQueryException (strMsg);
			}
			// Emb for custom bitmap draw: replace Load/SetIcon stuff, and modify window text
			pQepBox = pNodeInfo->GetQepBox(bPreview);
			SetIconAndText(pQepBox, pNodeInfo->m_nIcon, pNodeInfo->m_strNodeType);

			b2Flash = TRUE;
		}
		if (pQepBox && !pQepBox->IsWindowVisible())
		{
			pQepBox->SendMessage (WM_SQL_QUERY_UPADATE_DATA, (WPARAM)0, (LPARAM)pNodeInfo);
			CRect rcQep = pNodeInfo->GetQepBoxRect (bPreview);
			pQepBox->SetWindowPos (NULL, rcQep.left, rcQep.top, 0, 0, SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOZORDER);
			if (b2Flash)
				pQepBox->FlashWindow (TRUE);

			// Emb: Added frame change to force redraw of caption so that image will gracefully appear
			pQepBox->SetWindowPos (NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
		}
		DrawChildStart (pQepBinaryTree, pDC, bPreview);
		DrawQepBoxes (pQepBinaryTree->m_pQepNodeLeft,  pDC, bPreview);
		DrawQepBoxes (pQepBinaryTree->m_pQepNodeRight, pDC, bPreview);
	}
}


/////////////////////////////////////////////////////////////////////////////
// CvQueryExecutionPlanView

IMPLEMENT_DYNCREATE(CvQueryExecutionPlanView, CScrollView)

BEGIN_MESSAGE_MAP(CvQueryExecutionPlanView, CScrollView)
	//{{AFX_MSG_MAP(CvQueryExecutionPlanView)
	ON_WM_CREATE()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_MESSAGE (WM_QEP_REDRAWLINKS,     OnRedrawLinks)
	ON_MESSAGE (WM_QEP_OPEN_POPUPINFO,  OnOpenPopupInfo)
	ON_MESSAGE (WM_QEP_CLOSE_POPUPINFO, OnClosePopupInfo)
	ON_COMMAND(ID_FILE_PRINT, CScrollView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT,  CScrollView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CScrollView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CvQueryExecutionPlanView construction/destruction

CvQueryExecutionPlanView::CvQueryExecutionPlanView()
{
	m_nSequence          = NULL;
	m_bPreview           = FALSE;
	m_bCalPosition       = FALSE;
	m_bCalPositionPreview= FALSE;
	m_pPopupInfoWnd      = NULL;
	m_pPopupInfo         = NULL;
	m_pPopupInfoLeaf     = NULL;
	m_pPopupInfoStar     = NULL;
	m_pPopupInfoStarLeaf = NULL;
	m_bDrawingFailed     = FALSE;
	m_bCalSizeBox        = FALSE;
	m_bCalSizeBoxPreview = FALSE;
}

CvQueryExecutionPlanView::~CvQueryExecutionPlanView()
{
}

BOOL CvQueryExecutionPlanView::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= WS_CLIPCHILDREN;
	return CScrollView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CvQueryExecutionPlanView drawing

void CvQueryExecutionPlanView::SetMode (BOOL bPreview)
{
	m_bPreview = bPreview;
	Invalidate();
}


void CvQueryExecutionPlanView::OnDraw(CDC* pDC)
{
	CdQueryExecutionPlanDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;
	if (m_nSequence == NULL)
		return;
	if (m_bDrawingFailed)
		return;
	try
	{
		CaSqlQueryExecutionPlanData* pData = pDoc->m_listQepData.GetAt (m_nSequence);
		BOOL bPreview = pData->GetDisplayMode();
		//
		// Draw the Qep Boxes.
		DrawQepBoxes (pData->m_pQepBinaryTree, pDC, bPreview);
		//
		// Draw the links. (At this point, all Boxes of Qep of the current tree
		//                  have been allocated and created).
		CPoint pStart;
		CPoint pEnd;
		CWnd* pParentBox = NULL;
		CWnd* pChildBox  = NULL;
		CaSqlQueryExecutionPlanBoxLink* pLink = NULL;
		POSITION pos = pData->m_listLink.GetHeadPosition();
		while (pos != NULL)
		{
			pLink = pData->m_listLink.GetNext (pos);
			pParentBox =  pLink->GetParentBox(bPreview);
			pChildBox  =  pLink->GetChildBox (bPreview);
			ASSERT (pParentBox);
			ASSERT (pChildBox);
			if (!(pParentBox && pChildBox))
				return;
			GetStartingPoint (pParentBox, pStart, pLink->m_nSon);
			GetEndingPoint   (pChildBox,  pEnd);
			pDC->DPtoLP (&pStart);
			pDC->MoveTo (pStart);
			pDC->DPtoLP (&pEnd);
			pDC->LineTo (pEnd);
		}
		if (m_pPopupInfoWnd)
		{
			ClientToScreen (m_rcPopupInfo);
			m_pPopupInfoWnd->SetWindowPos (&wndTop, m_rcPopupInfo.left, m_rcPopupInfo.top, 0, 0, SWP_NOSIZE|SWP_SHOWWINDOW);
			m_pPopupInfoWnd->SetWindowPos (NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
		}
	}
	catch (CeSqlQueryException e)
	{
		AfxMessageBox (e.GetReason());
		return;
	}
	catch (...)
	{
		AfxMessageBox (IDS_MSG_FAIL_2_DRAW_QEP_TREE);
		return;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CvQueryExecutionPlanView printing

BOOL CvQueryExecutionPlanView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CvQueryExecutionPlanView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CvQueryExecutionPlanView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CvQueryExecutionPlanView diagnostics

#ifdef _DEBUG
void CvQueryExecutionPlanView::AssertValid() const
{
	CScrollView::AssertValid();
}

void CvQueryExecutionPlanView::Dump(CDumpContext& dc) const
{
	CScrollView::Dump(dc);
}

CdQueryExecutionPlanDoc* CvQueryExecutionPlanView::GetDocument() // non-debug version is inline
{
	return (CdQueryExecutionPlanDoc*)m_pDocument;
}
#endif //_DEBUG
/////////////////////////////////////////////////////////////////////////////
// CvQueryExecutionPlanView message handlers


void CvQueryExecutionPlanView::InitialDrawing ()
{
	CSize sizeBox;
	CSize sizePad;
	BOOL  bPreview = FALSE;
	int   nDepth = 0;
	try
	{
		CdQueryExecutionPlanDoc* pDoc = (CdQueryExecutionPlanDoc*)GetDocument();
		CaSqlQueryExecutionPlanData* pTreeData = pDoc->m_listQepData.GetAt (m_nSequence);
		if (!pTreeData)
			return;
		bPreview = pTreeData->GetDisplayMode();
		CalcQepBoxSize (pTreeData);
		if (bPreview)
		{
			sizeBox = pTreeData->m_sizeQepBoxPreview;
			sizePad = pDoc->m_sizeQepBoxPaddingPreview;
		}
		else
		{
			sizeBox = pTreeData->m_sizeQepBox;
			sizePad = pDoc->m_sizeQepBoxPadding;
		}
		if (bPreview && !pTreeData->IsCalDrawAreaPreview())
		{
			CalculateDepth (pTreeData->m_pQepBinaryTree, nDepth);
			pTreeData->m_sizeDrawingAreaPreview.cx = (int)((double)pow ((double)2, nDepth) * (sizeBox.cx + sizePad.cx));
			pTreeData->m_sizeDrawingAreaPreview.cy = (nDepth +1) * (sizeBox.cy + sizePad.cy);
		}
		if (!bPreview && !pTreeData->IsCalDrawArea())
		{
			CalculateDepth (pTreeData->m_pQepBinaryTree, nDepth);
			pTreeData->m_sizeDrawingArea.cx = (int)((double)pow ((double)2, nDepth) * (sizeBox.cx + sizePad.cx));
			pTreeData->m_sizeDrawingArea.cy = (nDepth +1) * (sizeBox.cy + sizePad.cy);
		}
		
		if (!pDoc->m_bFromSerialize && !m_bCalPosition && !bPreview)
		{
			pTreeData->QEPBoxesInitPosition   (pTreeData->m_pQepBinaryTree, pDoc);
			pTreeData->QEPBoxesChangePosition (pTreeData->m_pQepBinaryTree, pDoc);
			m_bCalPosition = TRUE;
		}
		
		if (bPreview && !m_bCalPositionPreview)
		{
			//
			// Initialize the position of the QepBoxes:
			pTreeData->QEPBoxesInitPosition   (pTreeData->m_pQepBinaryTree, pDoc);
			pTreeData->QEPBoxesChangePosition (pTreeData->m_pQepBinaryTree, pDoc);
			m_bCalPositionPreview = TRUE;
		}
		int nMaxPos = 0;
		if (bPreview && !pTreeData->IsCalDrawAreaPreview())
		{
			pTreeData->GetMaxPosition (pTreeData->m_pQepBinaryTree, nMaxPos);
			pTreeData->m_sizeDrawingAreaPreview.cx = nMaxPos + sizePad.cx;
			pTreeData->SetCalDrawAreaPreview (TRUE);
		}
		if (!bPreview && !pTreeData->IsCalDrawArea())
		{
			pTreeData->GetMaxPosition (pTreeData->m_pQepBinaryTree, nMaxPos);
			pTreeData->m_sizeDrawingArea.cx = nMaxPos + sizePad.cx;
			SetScrollSizes(MM_TEXT, pTreeData->m_sizeDrawingArea);
			pTreeData->SetCalDrawArea (TRUE);
		}

		if (bPreview)
		{
			SetScrollSizes(MM_TEXT, pTreeData->m_sizeDrawingAreaPreview);
			SetScrollPos(SB_HORZ, pTreeData->m_xScrollPreview);
			SetScrollPos(SB_VERT, pTreeData->m_yScrollPreview);
		}
		else
		{
			SetScrollSizes(MM_TEXT, pTreeData->m_sizeDrawingArea);
			SetScrollPos(SB_HORZ, pTreeData->m_xScroll);
			SetScrollPos(SB_VERT, pTreeData->m_yScroll);
		}
	}
	catch (...)
	{
		AfxMessageBox (IDS_MSG_FAIL_2_INIT_DRAW_QEP_TREE);
		m_bDrawingFailed = TRUE;
	}
}



void CvQueryExecutionPlanView::OnInitialUpdate() 
{
	CScrollView::OnInitialUpdate();
	try
	{
		CfQueryExecutionPlanFrame* pParent = (CfQueryExecutionPlanFrame*)GetParent();
		ASSERT (pParent);
		if (pParent)
		{
			pParent->m_pQueryExecutionPlanView = this;
			CuDlgSqlQueryPageQepIndividual* pParent2 = (CuDlgSqlQueryPageQepIndividual*)pParent->GetParent();
			ASSERT (pParent2);
			if (pParent2)
				m_nSequence = pParent2->m_nQepSequence;
			else
				m_nSequence = NULL; 
		}
		if (m_nSequence == NULL)
			return;
		m_pPopupInfo     = new CuDlgQueryExecutionPlanBox (this, TRUE);
		m_pPopupInfo->SetPopupInfo();
		m_pPopupInfo->Create (IDD_QEPBOX_OVERLAPPED, this);
		m_pPopupInfo->FlashWindow (TRUE);
		
		m_pPopupInfoLeaf = new CuDlgQueryExecutionPlanBoxLeaf (this, TRUE);
		m_pPopupInfoLeaf->SetPopupInfo();
		m_pPopupInfoLeaf->Create (IDD_QEPBOX_LEAF_OVERLAPPED, this);
		m_pPopupInfoLeaf->FlashWindow (TRUE);
		
		m_pPopupInfoStar     = new CuDlgQueryExecutionPlanBoxStar (this, TRUE);
		m_pPopupInfoStar->SetPopupInfo();
		m_pPopupInfoStar->Create (IDD_QEPBOX_STAR_OVERLAPPED, this);
		m_pPopupInfoStar->FlashWindow (TRUE);
		
		m_pPopupInfoStarLeaf     = new CuDlgQueryExecutionPlanBoxStarLeaf (this, TRUE);
		m_pPopupInfoStarLeaf->SetPopupInfo();
		m_pPopupInfoStarLeaf->Create (IDD_QEPBOX_STAR_LEAF_OVERLAPPED, this);
		m_pPopupInfoStarLeaf->FlashWindow (TRUE);
		
		InitialDrawing();
	}
	catch (...)
	{
		AfxMessageBox (IDS_MSG_FAIL_2_INIT_DRAW_QEP_TREE);
		m_bDrawingFailed = TRUE;
	}
}


void CvQueryExecutionPlanView::GetStartingPoint (CWnd* pBox, CPoint& p, int nSon)
{
	int w;
	CRect r;
	pBox->GetWindowRect (r);
	ScreenToClient (r);
	w = r.Width();
	CPoint pLeft  (r.left  + w/2 - 10, r.bottom + 5);
	CPoint pRight (r.right - w/2 + 10, r.bottom + 5);
	if (nSon == 1)
	{
		p = pLeft;
		p.x += 2;
	}
	else
	if (nSon == 2)
	{
		p = pRight;
		p.x += 2;
	}
}


void CvQueryExecutionPlanView::GetEndingPoint (CWnd* pBox, CPoint& p)
{
	int w;
	CRect r;
	pBox->GetWindowRect (r);
	ScreenToClient (r);
	w = r.Width();
	p = CPoint (r.left  + w/2,  r.top - 5);
	p.x += 2;
}


LONG CvQueryExecutionPlanView::OnRedrawLinks (UINT wParam, LONG lParam)
{
	Invalidate();
	return 0;
}

int CvQueryExecutionPlanView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CScrollView::OnCreate(lpCreateStruct) == -1)
		return -1;
	CSize sizeTotal;
	sizeTotal.cx = 1280;
	sizeTotal.cy = 1024;
	SetScrollSizes(MM_TEXT, sizeTotal);
	return 0;
}

LONG CvQueryExecutionPlanView::OnOpenPopupInfo (UINT wParam, LONG lParam)
{
	m_pPopupInfoWnd = NULL;
	CdQueryExecutionPlanDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return 0L;
	if (m_nSequence == NULL)
		return 0L;
	CWnd* pPreviewBox = (CWnd*)lParam;
	CaSqlQueryExecutionPlanData* pData = pDoc->m_listQepData.GetAt (m_nSequence);
	CaQepNodeInformation* pNodeInfo = pData->FindNodeInfo (pPreviewBox);
	if (pNodeInfo)
	{
		HICON hIcon = AfxGetApp()->LoadIcon(pNodeInfo->m_nIcon);
		ASSERT (hIcon);
		pPreviewBox->GetWindowRect (m_rcPopupInfo);
		ScreenToClient (m_rcPopupInfo);
		if (pNodeInfo->GetQepNode() == QEP_NODE_LEAF)
		{
			if (pData->m_qepType == QEP_NORMAL)
			{
				SetIconAndText(m_pPopupInfoLeaf, pNodeInfo->m_nIcon, pNodeInfo->m_strNodeType);
				m_pPopupInfoLeaf->SendMessage (WM_SQL_QUERY_UPADATE_DATA, (WPARAM)0, (LPARAM)pNodeInfo);
				m_pPopupInfoWnd = m_pPopupInfoLeaf;
			}
			else
			{
				SetIconAndText(m_pPopupInfoStarLeaf, pNodeInfo->m_nIcon, pNodeInfo->m_strNodeType);
				m_pPopupInfoStarLeaf->SendMessage (WM_SQL_QUERY_UPADATE_DATA, (WPARAM)0, (LPARAM)(CaQepNodeInformationStar*)pNodeInfo);
				m_pPopupInfoWnd = m_pPopupInfoStarLeaf;
			}
		}
		else
		{
			if (pData->m_qepType == QEP_NORMAL)
			{
				SetIconAndText(m_pPopupInfo, pNodeInfo->m_nIcon, pNodeInfo->m_strNodeType);
				m_pPopupInfo->SendMessage (WM_SQL_QUERY_UPADATE_DATA, (WPARAM)0, (LPARAM)pNodeInfo);
				m_pPopupInfoWnd = m_pPopupInfo;
			}
			else
			{
				SetIconAndText(m_pPopupInfoStar, pNodeInfo->m_nIcon, pNodeInfo->m_strNodeType);
				m_pPopupInfoStar->SendMessage (WM_SQL_QUERY_UPADATE_DATA, (WPARAM)0, (LPARAM)(CaQepNodeInformationStar*)pNodeInfo);
				m_pPopupInfoWnd = m_pPopupInfoStar;
			}
		}
		Invalidate();
	}
	return 0L;
}


LONG CvQueryExecutionPlanView::OnClosePopupInfo(UINT wParam, LONG lParam)
{
	if (m_pPopupInfoWnd)
		m_pPopupInfoWnd->ShowWindow (SW_HIDE);
	m_pPopupInfoWnd = NULL;
	Invalidate();
	return 0L;
}



void CvQueryExecutionPlanView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	CScrollView::OnHScroll(nSBCode, nPos, pScrollBar);
	if (nSBCode == SB_ENDSCROLL)
	{
		CdQueryExecutionPlanDoc* pDoc = GetDocument();
		ASSERT_VALID(pDoc);
		if (!pDoc)
			return;
		if (m_nSequence == NULL)
			return;
		if (m_bDrawingFailed)
			return;
		try
		{
			CaSqlQueryExecutionPlanData* pData = pDoc->m_listQepData.GetAt (m_nSequence);
			BOOL bPreview = pData->GetDisplayMode();
			if (bPreview)
			{
				pData->m_xScrollPreview = GetScrollPos (SB_HORZ);
				pData->m_yScrollPreview = GetScrollPos (SB_VERT);
			}
			else
			{
				pData->m_xScroll = GetScrollPos (SB_HORZ);
				pData->m_yScroll = GetScrollPos (SB_VERT);
			}
		}
		catch (...)
		{
			CString strMsr = _T("CvQueryExecutionPlanView: Cannot initialize scroll position."); // IDS_E_SCROLL_POS
			AfxMessageBox (strMsr);
		}
	}
}

void CvQueryExecutionPlanView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	CScrollView::OnVScroll(nSBCode, nPos, pScrollBar);
	if (nSBCode == SB_ENDSCROLL)
	{
		CdQueryExecutionPlanDoc* pDoc = GetDocument();
		ASSERT_VALID(pDoc);
		if (!pDoc)
			return;
		if (m_nSequence == NULL)
			return;
		if (m_bDrawingFailed)
			return;
		try
		{
			CaSqlQueryExecutionPlanData* pData = pDoc->m_listQepData.GetAt (m_nSequence);
			BOOL bPreview = pData->GetDisplayMode();
			if (bPreview)
			{
				pData->m_xScrollPreview = GetScrollPos (SB_HORZ);
				pData->m_yScrollPreview = GetScrollPos (SB_VERT);
			}
			else
			{
				pData->m_xScroll = GetScrollPos (SB_HORZ);
				pData->m_yScroll = GetScrollPos (SB_VERT);
			}
		}
		catch (...)
		{
			AfxMessageBox (_T("CvQueryExecutionPlanView::OnHScroll: unknown error"));
		}
	}
}

//
// TODO: improve the performant by calculating the qep box size only one time.
void CvQueryExecutionPlanView::CalcQepBoxSize (CaSqlQueryExecutionPlanData* pTreeData)
{
	ASSERT (pTreeData);
	if (!pTreeData)
		return;
	BOOL bPreview = pTreeData->GetDisplayMode();
	if (bPreview && m_bCalSizeBoxPreview)
		return;
	if (!bPreview && m_bCalSizeBox)
		return;
	//
	// The test should be carried out to determine if it is a STAR or NORMAL.
	CWnd* pBox = NULL;
	CRect r;
	if (pTreeData->m_qepType == QEP_NORMAL)
	{
		if (bPreview)
		{
			pBox = (CuDlgQueryExecutionPlanBoxPreview*)new CuDlgQueryExecutionPlanBoxPreview ();
			((CuDlgQueryExecutionPlanBoxPreview*)pBox)->Create (IDD_QEPBOX_PREVIEW, this);
			pBox->GetWindowRect (r);
			pBox->DestroyWindow();
		}
		else
		{
			pBox = (CuDlgQueryExecutionPlanBox*)new CuDlgQueryExecutionPlanBox ();
			((CuDlgQueryExecutionPlanBox*)pBox)->Create (IDD_QEPBOX, this);
			pBox->GetWindowRect (r);
			pBox->DestroyWindow();
		}
	}
	else
	{
		if (bPreview)
		{
			pBox = (CuDlgQueryExecutionPlanBoxStarPreview*)new CuDlgQueryExecutionPlanBoxStarPreview ();
			((CuDlgQueryExecutionPlanBoxStar*)pBox)->Create (IDD_QEPBOX_STAR_PREVIEW, this);
			pBox->GetWindowRect (r);
			pBox->DestroyWindow();
		}
		else
		{
			pBox = (CuDlgQueryExecutionPlanBoxStar*)new CuDlgQueryExecutionPlanBoxStar ();
			((CuDlgQueryExecutionPlanBoxStar*)pBox)->Create (IDD_QEPBOX_STAR, this);
			pBox->GetWindowRect (r);
			pBox->DestroyWindow();
		}
	}
	if (bPreview)
	{
		pTreeData->m_sizeQepBoxPreview.cx = r.Width();
		pTreeData->m_sizeQepBoxPreview.cy = r.Height();
		m_bCalSizeBoxPreview = TRUE;
	}
	else
	{
		pTreeData->m_sizeQepBox.cx = r.Width();
		pTreeData->m_sizeQepBox.cy = r.Height();
		m_bCalSizeBox = TRUE;
	}
}
