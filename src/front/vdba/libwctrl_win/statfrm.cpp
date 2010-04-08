/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : statfrm.cpp, Implementation file.  (Frame Window)
**    Project  : CA-OpenIngres/Monitoring
**    Author   : Uk Sotheavut (uk$so01)
**    Purpose  : Drawing the statistic bar
**
** History:
**
** xx-Apr-1997 (uk$so01)
**    Created
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 08-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (vdba uses libwctrl.lib).
*/

#include "stdafx.h"
#include "usplittr.h"
#include "statfrm.h"
#include "piedoc.h"
#include "statview.h"
#include "pieview.h"
#include "vlegend.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Legeng Control (CListBox)
// ************************************************************************************************
CuStatisticBarLegendList::CuStatisticBarLegendList()
{
	m_nXHatchWidth  = 4;
	m_nXHatchHeight = 6;
	m_bHatch = FALSE;
	m_nRectColorWidth = 20;
	m_colorText  = ::GetSysColor (COLOR_WINDOWTEXT);
	m_colorTextBk = ::GetSysColor (COLOR_WINDOW);
}


CuStatisticBarLegendList::~CuStatisticBarLegendList()
{
}

int CuStatisticBarLegendList::AddItem (LPCTSTR lpszItem, COLORREF color)
{
	int index = AddString (lpszItem);
	if (index != LB_ERR)
		SetItemData (index, (DWORD)color);
	return index;
}

int CuStatisticBarLegendList::AddItem2(LPCTSTR lpszItem, DWORD dwItemData)
{
	m_bHatch = TRUE;
	int index = AddString (lpszItem);
	if (index != LB_ERR)
		SetItemData (index, dwItemData);
	return index;
}

BEGIN_MESSAGE_MAP(CuStatisticBarLegendList, CListBox)
	//{{AFX_MSG_MAP(CuStatisticBarLegendList)
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
	ON_WM_SYSCOLORCHANGE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuStatisticBarLegendList message handlers

BOOL CuStatisticBarLegendList::PreCreateWindow(CREATESTRUCT& cs) 
{
	if ((cs.style & LBS_OWNERDRAWFIXED) == 0)
		cs.style |= LBS_OWNERDRAWFIXED;
	if ((cs.style & LBS_MULTICOLUMN) == 0)
		cs.style |= LBS_MULTICOLUMN;
	if ((cs.style & LBS_HASSTRINGS) == 0)
		cs.style |= LBS_HASSTRINGS;
	if ((cs.style & LBS_NOINTEGRALHEIGHT) == 0)
		cs.style |= LBS_NOINTEGRALHEIGHT;
	if ((cs.style & LBS_NOINTEGRALHEIGHT) == 0)
		cs.style |= LBS_NOINTEGRALHEIGHT;
	return CListBox::PreCreateWindow(cs);
}

void CuStatisticBarLegendList::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct) 
{
	lpMeasureItemStruct->itemHeight = 20;
}

#define X_LEFTMARGIN 2
#define X_SEPERATOR  4

void CuStatisticBarLegendList::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	COLORREF cr = NULL;
	CaBarPieItem* pData = NULL;

	int nPreviousBkMode = 0;
	if ((int)lpDrawItemStruct->itemID < 0)
		return;
	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	if (m_bHatch)
	{
		pData = (CaBarPieItem*)lpDrawItemStruct->itemData;
		ASSERT (pData);
		if (!pData)
			return;
		cr = pData->m_colorRGB;
	}
	else
		cr = (COLORREF)lpDrawItemStruct->itemData; // RGB in item data

	CBrush brush(m_colorTextBk);
	pDC->FillRect (&lpDrawItemStruct->rcItem, &brush);
	
	CRect r = lpDrawItemStruct->rcItem;
	r.top    = r.top + 1;
	r.bottom = r.bottom - 1;
	r.left   = r.left + X_LEFTMARGIN;
	r.right  = r.left + m_nRectColorWidth;
	
	CBrush brush2(cr);
	pDC->FillRect (r, &brush2);
	if (m_bHatch && pData && pData->m_bHatch)
	{
		CRect rh = r;
		CBrush br(pData->m_colorHatch);
		//
		// Draw two rectangles:
	
		// upper:
		rh.left    = rh.right - m_nXHatchWidth - 4;
		rh.right   = rh.right - 2;
		rh.top     = rh.top    + 2;
		rh.bottom  = rh.top + m_nXHatchHeight;
		pDC->FillRect (rh, &br);
	
		// lower:
		rh = r;
		rh.left    = rh.right - m_nXHatchWidth - 4;
		rh.right   = rh.right - 2;
		rh.bottom  = rh.bottom - 1;
		rh.top     = rh.bottom - m_nXHatchHeight; 
		pDC->FillRect (rh, &br);
	}

	CRect rcText = lpDrawItemStruct->rcItem;
	CString strItem;
	GetText (lpDrawItemStruct->itemID, strItem);
	rcText.left = r.right + X_SEPERATOR;
	nPreviousBkMode = pDC->SetBkMode (TRANSPARENT);
	pDC->DrawText (strItem, rcText, DT_LEFT|DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER);
	pDC->SetBkMode (nPreviousBkMode);
}


void CuStatisticBarLegendList::OnLButtonDown(UINT nFlags, CPoint point) 
{
	//
	// Just swallow the message WM_LBUTTONDOWN
	return;
}

void CuStatisticBarLegendList::OnSysColorChange() 
{
	CListBox::OnSysColorChange();
	m_colorText = ::GetSysColor (COLOR_WINDOWTEXT);
	m_colorTextBk = ::GetSysColor (COLOR_WINDOW);
}



//
// Frame Window (CfStatisticFrame derived fromCFrameWnd)
// ************************************************************************************************
CfStatisticFrame::CfStatisticFrame(int nType)
{
	m_nType = nType;
	m_pPieChartDoc = NULL;
	m_pSplitter = NULL;
	m_pViewLegend = NULL;
	m_bAllCreated = FALSE;
}

CfStatisticFrame::~CfStatisticFrame()
{
	if (m_pSplitter)
		delete m_pSplitter;
}

BOOL CfStatisticFrame::Create(CWnd* pParent, CRect r, CaPieChartCreationData* pCreationData)
{
	m_pPieChartDoc = new CdStatisticPieChartDoc();
	((CdStatisticPieChartDoc*)m_pPieChartDoc)->m_PieChartCreationData = *pCreationData;
	if (pCreationData && pCreationData->m_bUseLegend)
	{
		m_pSplitter = (CSplitterWnd*)new CuUntrackableSplitterWnd();
		CuUntrackableSplitterWnd* pSplitter = (CuUntrackableSplitterWnd*)m_pSplitter;
	}

	if (CFrameWnd::Create (NULL, NULL, WS_CHILD, r, pParent) == -1)
		return FALSE;
	InitialUpdateFrame(NULL, TRUE);
	ShowWindow(SW_SHOW);
	return TRUE;
}


BEGIN_MESSAGE_MAP(CfStatisticFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CfStatisticFrame)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CfStatisticFrame message handlers

int CfStatisticFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	return 0;
}

BOOL CfStatisticFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
	CCreateContext context1;
	CaBarInfoData* pBar = NULL;
	CaPieInfoData* pPie  = NULL;
	CdStatisticPieChartDoc* pDoc = (CdStatisticPieChartDoc*)m_pPieChartDoc;
	ASSERT(pDoc);
	if (!pDoc)
		return FALSE;
	CaPieChartCreationData& crData = pDoc->m_PieChartCreationData;

	switch (m_nType)
	{
	case nTypeBar:
		pBar = new CaBarInfoData;
		pDoc->SetDiskInfo (pBar);
		context1.m_pNewViewClass = RUNTIME_CLASS (CvStatisticBarView);
		context1.m_pCurrentDoc   = m_pPieChartDoc;
		break;
	case nTypePie:
		pPie = new CaPieInfoData;
		pDoc->SetPieInfo (pPie);
		context1.m_pNewViewClass = RUNTIME_CLASS (CvStatisticPieView);
		context1.m_pCurrentDoc   = m_pPieChartDoc;
		break;
	default:
		ASSERT (FALSE);
		return FALSE;
		break;
	}

	if (crData.m_bUseLegend)
	{
		//
		// Create the legend view:
		ASSERT(m_pSplitter);
		CuUntrackableSplitterWnd* pSplitter = (CuUntrackableSplitterWnd*)m_pSplitter;
		if (!pSplitter)
			return FALSE;
		CCreateContext context2;
		CRuntimeClass* pv = RUNTIME_CLASS(CvLegend);
		m_pViewLegend = (CView*)pv;
		context2.m_pNewViewClass = pv;
		context2.m_pCurrentDoc   = m_pPieChartDoc;

		if (crData.m_bLegendRight)
		{
			// Create a splitter of 1 row and 2 columns.
			if (!pSplitter->CreateStatic (this, 1, 2)) 
			{
				TRACE0 ("CfStatisticFrame::OnCreateClient: Failed to create Splitter\n");
				return FALSE;
			}
			//
			// Left pane (the drawing view)
			if (!pSplitter->CreateView (0, 0, context1.m_pNewViewClass, CSize (200, 300), &context1)) 
			{
				TRACE0 ("CfIpmFrame::OnCreateClient: Failed to create first pane\n");
				return FALSE;
			}
			//
			// Right pane (the legend view)
			if (!pSplitter->CreateView (0, 1, context2.m_pNewViewClass, CSize (100, 300), &context2)) 
			{
				TRACE0 ("CfIpmFrame::OnCreateClient: Failed to create second pane\n");
				return FALSE;
			}
			m_bAllCreated = TRUE;
			//
			// Set the pane size:
			ResizePanes();
		}
		else
		{
			// Create a splitter of 2 rows and 1 column.
			if (!pSplitter->CreateStatic (this, 2, 1)) 
			{
				TRACE0 ("CfStatisticFrame::OnCreateClient: Failed to create Splitter\n");
				return FALSE;
			}
			//
			// Upper pane (the drawing view)
			if (!pSplitter->CreateView (0, 0, context1.m_pNewViewClass, CSize (200, 400), &context1)) 
			{
				TRACE0 ("CfIpmFrame::OnCreateClient: Failed to create first pane\n");
				return FALSE;
			}
			//
			// Lower pane (the legend view)
			if (!pSplitter->CreateView (1, 0, context2.m_pNewViewClass, CSize (100, 200), &context2)) 
			{
				TRACE0 ("CfIpmFrame::OnCreateClient: Failed to create second pane\n");
				return FALSE;
			}
			m_bAllCreated = TRUE;
			//
			// Set the pane size:
			ResizePanes();
		}
		RecalcLayout();
		return TRUE;
	}

	BOOL bOk = CFrameWnd::OnCreateClient(lpcs, &context1);
	m_bAllCreated = TRUE;
	return m_bAllCreated;
}

void CfStatisticFrame::ResizePanes()
{
	if (!m_bAllCreated)
		return;
	CdStatisticPieChartDoc* pDoc = (CdStatisticPieChartDoc*)m_pPieChartDoc;
	ASSERT(pDoc);
	if (!pDoc)
		return;
	CaPieChartCreationData& crData = pDoc->m_PieChartCreationData;
	if (!crData.m_bUseLegend)
		return;
	CuUntrackableSplitterWnd* pSplitter = (CuUntrackableSplitterWnd*)m_pSplitter;
	if (!pSplitter)
		return;

	if (pSplitter->UserDragged())
		return;
	if (crData.m_bLegendRight)
	{
		CvLegend* pViewLegend = (CvLegend*)pSplitter->GetPane(0, 1);
		CuStatisticBarLegendList* pListBox = pViewLegend->GetListBoxLegend();
		CRect rcItem, rcClient;
		GetClientRect(rcClient);
		pListBox->GetItemRect(0, rcItem);
		int nLegendWidth = (rcItem.Width()+4) * crData.m_nColumns;
		CSize sizePaneUpper (rcClient.Width() - nLegendWidth, rcClient.Height());

		pSplitter->SetColumnInfo(0, sizePaneUpper.cx, 40);
		pSplitter->RecalcLayout();
	}
	else
	{
		CvLegend* pViewLegend = (CvLegend*)pSplitter->GetPane(1, 0);
		CuStatisticBarLegendList* pListBox = pViewLegend->GetListBoxLegend();
		CRect rcItem, rcClient;
		GetClientRect(rcClient);
		pListBox->GetItemRect(0, rcItem);
		int nLegendHeight = (rcItem.Height()+4) * crData.m_nLines + rcItem.Height()/2;
		CSize sizePaneUpper (rcClient.Width(), rcClient.Height() - nLegendHeight - 10);

		pSplitter->SetRowInfo(0, sizePaneUpper.cy, 40);
		pSplitter->RecalcLayout();
	}
}

void CfStatisticFrame::OnSize(UINT nType, int cx, int cy) 
{
	CFrameWnd::OnSize(nType, cx, cy);
	ResizePanes();
}

void CfStatisticFrame::DrawLegend (CuStatisticBarLegendList* pList, BOOL bShowPercentage)
{
	CdStatisticPieChartDoc* pPieChartDoc = (CdStatisticPieChartDoc*)m_pPieChartDoc;
	CaBarInfoData* pBarInfo = NULL;
	CaPieInfoData*  pPieInfo = NULL;
	CString strItem;
	POSITION pos = NULL;
	ASSERT (pPieChartDoc);
	if (!pPieChartDoc)
		return;
	CaPieChartCreationData& crData = pPieChartDoc->m_PieChartCreationData;
	if (!pList && crData.m_bUseLegend && m_pViewLegend)
	{
		CvLegend* pViewLegend = NULL;
		if (crData.m_bLegendRight)
			pViewLegend = (CvLegend*)m_pSplitter->GetPane (0, 1);
		else
			pViewLegend = (CvLegend*)m_pSplitter->GetPane (1, 0);

		pList = pViewLegend->GetListBoxLegend();
	}

	if (!pList)
		return;
	pList->ResetContent();
	switch (m_nType)
	{
	case nTypeBar:
		pBarInfo = pPieChartDoc->GetDiskInfo();
		ASSERT(pBarInfo);
		pos = pBarInfo->m_listBar.GetHeadPosition();
		while (pos != NULL)
		{
			CaBarPieItem* pData = (CaBarPieItem*)pBarInfo->m_listBar.GetNext (pos);
			pList->AddItem2 (pData->m_strName, (DWORD)pData);
		}
		break;
	case nTypePie:
		pPieInfo = pPieChartDoc->GetPieInfo();
		ASSERT (pPieInfo);
		pos = pPieInfo->m_listLegend.GetHeadPosition();
		while (pos != NULL)
		{
			CaLegendData* pData = (CaLegendData*)pPieInfo->m_listLegend.GetNext (pos);
			if ((crData.m_bShowPercentage && !crData.m_bShowValue) || bShowPercentage)
			{
				strItem.Format (_T("%s (%0.2f%%)"), (LPCTSTR)pData->m_strName, pData->m_dPercentage);
				pList->AddItem (strItem, pData->m_colorRGB);
			}
			else
			if (!crData.m_bShowPercentage && crData.m_bShowValue)
			{
				if (crData.m_bValueInteger)
					strItem.Format (_T("%s (%d%s)"), (LPCTSTR)pData->m_strName, (int)pData->m_dValue,(LPCTSTR)pPieInfo->m_strUnit);
				else
					strItem.Format (_T("%s (%0.2f%s)"), (LPCTSTR)pData->m_strName, pData->m_dValue, (LPCTSTR)pPieInfo->m_strUnit);
				pList->AddItem (strItem, pData->m_colorRGB);
			}
			else
			if (crData.m_bShowPercentage && crData.m_bShowValue)
			{
				if (crData.m_bValueInteger)
					strItem.Format (_T("%s (%0.2f%%, %d%s)"), (LPCTSTR)pData->m_strName, pData->m_dPercentage, (int)pData->m_dValue, (LPCTSTR)pPieInfo->m_strUnit);
				else
					strItem.Format (_T("%s (%0.2f%%, %0.2f%s)"), (LPCTSTR)pData->m_strName, pData->m_dPercentage, pData->m_dValue, (LPCTSTR)pPieInfo->m_strUnit);
				pList->AddItem (strItem, pData->m_colorRGB);
			}
			else
				pList->AddItem (pData->m_strName, pData->m_colorRGB);
		}
		break;
	default:
		break;
	}

	int nMaxWidth = 0;
	CSize size;
	CDC* pDC = pList->GetDC();
	int i, nCount = pList->GetCount();
	for (i=0; i<nCount; i++)
	{
		pList->GetText (i, strItem);
		size = pDC->GetTextExtent (strItem);
		if (nMaxWidth < size.cx)
			nMaxWidth = size.cx;
	}
	pList->SetColumnWidth(nMaxWidth + 20);
	ReleaseDC(pDC);
}

void CfStatisticFrame::UpdateLegend()
{
	CdStatisticPieChartDoc* pPieChartDoc = (CdStatisticPieChartDoc*)m_pPieChartDoc;
	ASSERT (pPieChartDoc);
	if (!pPieChartDoc)
		return;
	CaPieChartCreationData& crData = pPieChartDoc->m_PieChartCreationData;
	DrawLegend (NULL, crData.m_bShowPercentage);
}

void CfStatisticFrame::UpdatePieChart()
{
	CdStatisticPieChartDoc* pPieChartDoc = (CdStatisticPieChartDoc*)m_pPieChartDoc;
	ASSERT (pPieChartDoc);
	if (!pPieChartDoc)
		return;
	pPieChartDoc->UpdateAllViews(NULL);
}

void CfStatisticFrame::SetPieInformation(CaPieInfoData* pData)
{
	CdStatisticPieChartDoc* pDoc = (CdStatisticPieChartDoc*)m_pPieChartDoc;
	pDoc->SetPieInfo(pData);
}

void CfStatisticFrame::SetBarInformation(CaBarInfoData* pData)
{
	CdStatisticPieChartDoc* pDoc = (CdStatisticPieChartDoc*)m_pPieChartDoc;
	pDoc->SetDiskInfo(pData);
}

CaPieInfoData*  CfStatisticFrame::GetPieInformation()
{
	ASSERT(m_nType == nTypePie); // Constructor must be called with 'nTypePie'
	ASSERT(m_pPieChartDoc);      // Must Create the frame first
	if (!m_pPieChartDoc)
		return NULL;
	return ((CdStatisticPieChartDoc*)m_pPieChartDoc)->GetPieInfo();
}

CaBarInfoData* CfStatisticFrame::GetBarInformation()
{
	ASSERT(m_nType == nTypeBar); // Constructor must be called with 'nTypePie'
	ASSERT(m_pPieChartDoc);      // Must Create the frame first
	if (!m_pPieChartDoc)
		return NULL;
	return ((CdStatisticPieChartDoc*)m_pPieChartDoc)->GetDiskInfo();

}

BOOL CfStatisticFrame::IsLegendUsed()
{
	CdStatisticPieChartDoc* pDoc = (CdStatisticPieChartDoc*)m_pPieChartDoc;
	if (!pDoc)
		return FALSE;
	return pDoc->m_PieChartCreationData.m_bUseLegend;
}

CWnd* CfStatisticFrame::GetCurrentView()
{
	CWnd* pWnd = NULL;
	CDocument* pDoc = GetDoc();
	if (pDoc)
	{
		POSITION pos = pDoc->GetFirstViewPosition();
		pWnd = (CView*)pDoc->GetNextView( pos );
	}
	return pWnd;
}

