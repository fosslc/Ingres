/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : StatView.cpp, Implementation file.  (View Window)
**    Project  : CA-OpenIngres/Monitoring
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Drawing the statistic bar 
**
** History:
** xx-April-1997 (uk$so01)
**    Created
** 23-May-2000 (uk$so01)
**    bug 99242 (DBCS) modify the functions: 
**    DrawBars (CDC* pDC, CRect r, int nBar, CaBarData* pBar, double dDensity, int nWidth)
**    DrawBars3D (CDC* pDC, CRect r, int nBar, CaBarData* pBar, double dDensity, int nWidth, BOOL bUsesPalette)
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
** 08-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (vdba uses libwctrl.lib).
** 09-Mar-2004 (uk$so01)
**    SIR #110013 (Handle the DAS Server: remove the warning)
**/

#include "stdafx.h"
#include "statfrm.h"
#include "piedoc.h"
#include "statview.h"
#include "colorind.h" // UxCreateFont


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CvStatisticBarView, CScrollView)

static long AdjustGraphMax(double d)
{
	int i10=0;
	long l;
	
	// if size null (no disk), return immediately
	if (!d)
		return 1000000000L;
	
	while (d>=10) {
		i10++;
		d/=(double)10;
	}
	l=(long)( d+.0001);
	while (TRUE) {
		if (l>=8) {
			l=10;
			break;
		}
		if (l>=6) {
			l=8;
			break;
		}
		if (l>=4) {
			l=6;
			break;
		}
		if (l>=3) {
			l=4;
			break;
		}
		d*=10.;
		l=(long)( d+.0001);
		i10--;
		l= 4 * ( (l/4)+1);
		break;
	}
	while (i10) {
		i10--;
		l*=10;
	}
	return l;
}

static long AdjustGraphUnit(double d)
{
	return AdjustGraphMax(d)/8;
}




CvStatisticBarView::CvStatisticBarView()
{
	m_sizeTotal.cx  = 100;
	m_sizeTotal.cy  = 10;
	m_xBarSeperator = 20;
	m_xLeftMargin   = 40;
	m_yTopMargin    = 20;
	m_yBottomMargin = 20;
	m_nBarWidth     = 40;
	m_cColorFree    = NULL;
	m_nXHatchWidth  = 6;
	m_nXHatchHeight = 3;
}

CvStatisticBarView::~CvStatisticBarView()
{
}


BEGIN_MESSAGE_MAP(CvStatisticBarView, CScrollView)
	//{{AFX_MSG_MAP(CvStatisticBarView)
	ON_WM_LBUTTONDOWN()
	ON_WM_SIZE()
	ON_WM_LBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CvStatisticBarView drawing

void CvStatisticBarView::OnInitialUpdate()
{
	SetScrollSizes(MM_TEXT, m_sizeTotal);
}



void CvStatisticBarView::OnDraw(CDC* pDC)
{
	CSize txSize;
	CdStatisticPieChartDoc* pDoc = (CdStatisticPieChartDoc*)GetDocument();
	ASSERT (pDoc);
	if (!pDoc)
		return;
	//
	// Calculate the magins:
	CClientDC dc (this);
	CString strW = pDoc->m_PieChartCreationData.m_strBarWidth;
	txSize = (strW.IsEmpty())? dc.GetTextExtent (_T("EEEE"), 4): dc.GetTextExtent (strW, strW.GetLength());
	m_nBarWidth     = txSize.cx;
	m_yBottomMargin = txSize.cy+4;

	if (!pDoc->m_PieChartCreationData.m_bShowBarTitle)
		m_yBottomMargin = 4;
	
	if (!pDoc->m_PieChartCreationData.m_bDrawAxis)
	{
		m_yTopMargin  =  4;
		m_xLeftMargin = 10;
	}
	//
	// Draw the bars:
	if (pDoc && pDoc->m_PieChartCreationData.m_b3D)
		Draw3D(pDC);
	else
		Draw2D(pDC);
}

/////////////////////////////////////////////////////////////////////////////
// CvStatisticBarView diagnostics

#ifdef _DEBUG
void CvStatisticBarView::AssertValid() const
{
	CScrollView::AssertValid();
}

void CvStatisticBarView::Dump(CDumpContext& dc) const
{
	CScrollView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CvStatisticBarView message handlers

void CvStatisticBarView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CdStatisticPieChartDoc* pDoc = (CdStatisticPieChartDoc*)GetDocument();
	if (pDoc && !pDoc->m_PieChartCreationData.m_bUseLButtonDown)
		return;
	CPoint logPoint = point;
	CClientDC dc (this);
	OnPrepareDC (&dc);
	dc.DPtoLP (&logPoint);
	CRect rcSave;
	SetCapture ();
	
	CaBarData* pBarData;
	CaBarInfoData* pBar= pDoc->GetDiskInfo();
	POSITION pos = pBar->m_listUnit.GetHeadPosition();
	
	while (pos != NULL)
	{
		pBarData = (CaBarData*)pBar->m_listUnit.GetNext (pos);
		POSITION p = pBarData->m_listOccupant.GetHeadPosition();
		CaOccupantData* pOccupant = NULL;
		while (p != NULL)
		{
			pOccupant = (CaOccupantData*)pBarData->m_listOccupant.GetNext (p);
			//
			// Click on the Occupant.
			if (!pOccupant->m_rcOccupant.IsRectNull() && pOccupant->m_rcOccupant.PtInRect (logPoint))
			{
				CRect rx = *((LPCRECT)(pOccupant->m_rcOccupant));
				dc.LPtoDP (&rx);
		
				pBar->SetTrackerSelectObject (rx);
				pOccupant->m_bFocus = TRUE;
				pBar->m_Tracker.Draw (&dc);
				DrawSelectInfo (pBarData, pOccupant);
				return;
			}
		}
		//
		// Click on the Disk's Free region
		if (!pBarData->m_rcBarUnit.IsRectNull() && pBarData->m_rcBarUnit.PtInRect (logPoint))
		{
			CRect rx = *((LPCRECT)(pBarData->m_rcBarUnit));
			dc.LPtoDP (&rx);
			
			pBar->SetTrackerSelectObject (rx); //pDiskData->m_rcDiskUnit);
			pBarData->m_bFocus = TRUE;
			pBar->m_Tracker.Draw (&dc);
			DrawSelectInfo (pBarData);
			return;
		}
		//
		// Click on the Disk's caption (C:, D:, ...)
		if (pBarData->m_rcCaption.PtInRect (logPoint))
		{
			CRect rx = *((LPCRECT)(pBarData->m_rcCaption));
			dc.LPtoDP (&rx);
			
			pBar->SetTrackerSelectObject (rx); 
			pBarData->m_bFocus = TRUE;
			pBar->m_Tracker.Draw (&dc);
			DrawSelectInfo (pBarData, NULL, TRUE);
			return;
		}
	}
}

void CvStatisticBarView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	CClientDC dc (this);
	OnPrepareDC (&dc);

	CdStatisticPieChartDoc* pDoc = (CdStatisticPieChartDoc*)GetDocument();
	CaBarInfoData* pBar= pDoc->GetDiskInfo();
	
	if (lHint == 0)
	{
		CRect rcTrueRect;
		pBar->m_Tracker.GetTrueRect (rcTrueRect);
		dc.LPtoDP (&rcTrueRect);
		InvalidateRect (rcTrueRect);
	}
	else
	{
		CRect* rcTrueRect = (CRect*)lHint;
		dc.LPtoDP (rcTrueRect);
		InvalidateRect (rcTrueRect);
	}
}

void CvStatisticBarView::OnSize(UINT nType, int cx, int cy) 
{
	CScrollView::OnSize(nType, cx, cy);
	CdStatisticPieChartDoc* pDoc = (CdStatisticPieChartDoc*)GetDocument();
	CaBarInfoData*    pBar = pDoc->GetDiskInfo();
	int nCount = pBar->m_listUnit.GetCount();
	if (nCount == 0)
		return;
	CRect r;
	GetClientRect (r);
	POSITION pos = pBar->m_listUnit.GetHeadPosition();
	if ((m_xLeftMargin + nCount*(m_xBarSeperator + m_nBarWidth) + 10) > r.Width())
	{
		m_sizeTotal.cx = m_xLeftMargin + nCount*(m_xBarSeperator + m_nBarWidth) + 20;
		SetScrollSizes(MM_TEXT, m_sizeTotal);
	}
}

void CvStatisticBarView::DrawBars (CDC* pDC, CRect r, int nBar, CaBarData* pBar, double dDensity, int nWidth)
{
	ASSERT (nBar > 0);
	CdStatisticPieChartDoc* pDoc = (CdStatisticPieChartDoc*)GetDocument();

	int nPixels = (int)(dDensity * pBar->m_nCapacity);
	int x1, x2,  y1, y2 = r.bottom;
	int sy1 = 0, sy2 = y2;
	int nPreviousBkMode = 0;

	//
	// Draw the InitialBar 
	CBrush br(m_cColorFree);
	x1 = m_xLeftMargin + nBar * m_xBarSeperator + (nBar-1) * nWidth; 
	x2 = x1 + nWidth; 
	y1 = y2 - nPixels;
	pBar->m_rcBarUnit = CRect (x1, y1, x2, y2);
	if (y1 >= y2)
	{
		pBar->m_rcCaption.SetRectEmpty();
		POSITION p = pBar->m_listOccupant.GetHeadPosition();
		while (p != NULL)
		{
			CaOccupantData* pOccupant = (CaOccupantData*)pBar->m_listOccupant.GetNext (p);
			pOccupant->m_rcOccupant.SetRectEmpty();
			pBar->m_rcBarUnit.SetRectEmpty();
		}
		return;
	}
	pDC->FillRect (pBar->m_rcBarUnit, &br);
	//
	// Draw the name of the bar.
	COLORREF colorText = GetSysColor (COLOR_WINDOWTEXT);
	COLORREF colorTextOld = pDC->SetTextColor (colorText);
	CRect rcBarName (x1, y2+2, x2, r.bottom + m_yBottomMargin);
	nPreviousBkMode = pDC->SetBkMode (TRANSPARENT);
#if defined (MAINWIN)
	//
	// Draw only the first 4 characters:
	TCHAR buffd [128];
	_tcscpy (buffd, (LPCTSTR)pBar->m_strBarUnit);
	if (_tcslen (buffd) > 4)
	{
		int cC4 = 0;
		TCHAR* pch = buffd;
		while (*pch != _T('\0') && cC4 < 4)
		{
			cC4++;
			pch = _tcsinc(pch);
			if (cC4 == 4)
			{
				*pch = _T('\0');
				break;
			}
		}
	}
	pDC->DrawText  ((LPCTSTR)buffd, rcBarName, DT_CENTER|DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER);
#else
	pDC->DrawText  (pBar->m_strBarUnit, rcBarName, DT_CENTER|DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER);
#endif
	pDC->SetBkMode (nPreviousBkMode);
	pDC->SetTextColor (colorTextOld);
	rcBarName.bottom -= 2;
	pBar->m_rcCaption = rcBarName;

	//
	// Draw the stacks (the occupants).
	if (pBar->m_listOccupant.IsEmpty()) 
		return;
	POSITION p = pBar->m_listOccupant.GetHeadPosition();
	sy1 = 0;
	sy2 = y2;
	while (p != NULL)
	{
		CaOccupantData* pOccupant = (CaOccupantData*)pBar->m_listOccupant.GetNext (p);
		nPixels   = (int)(dDensity * (pOccupant->m_fPercentage*pBar->m_nCapacity / 100));
		sy1 = sy2 - nPixels;
		pOccupant->m_rcOccupant = CRect (x1, sy1, x2, sy2);
		sy2 = sy1;
		pBar->m_rcBarUnit.bottom = sy1-1;
		if (p == NULL && pBar->m_rcBarUnit.bottom == pBar->m_rcBarUnit.top)
		{
			pOccupant->m_rcOccupant.top = y1;
		}
		CBrush brush(pOccupant->m_colorRGB);
		pDC->FillRect (pOccupant->m_rcOccupant, &brush);
	}
}


void CvStatisticBarView::DrawBars3D (CDC* pDC, CRect r, int nBar, CaBarData* pBar, double dDensity, int nWidth, BOOL bUsesPalette)
{
	ASSERT (nBar > 0);
	CdStatisticPieChartDoc* pDoc = (CdStatisticPieChartDoc*)GetDocument();
	ASSERT (pDoc);
	if (!pDoc)
		return;

	int nPixels = (int)(dDensity * pBar->m_nCapacity);
	int x1, x2,  y1, y2 = r.bottom;
	int sy1 = 0, sy2 = y2;
	int nPreviousBkMode = 0;

	//
	// Draw the InitialBar with color 'm_cColorFree' 
	CBrush br(m_cColorFree);
	x1 = m_xLeftMargin + (nBar-1) * (m_xBarSeperator + nWidth); 
	x2 = x1 + nWidth; 
	y1 = y2 - nPixels;
	pBar->m_rcBarUnit = CRect (x1, y1, x2, y2);
	if (y1 >= y2)
	{
		pBar->m_rcCaption.SetRectEmpty();
		POSITION p = pBar->m_listOccupant.GetHeadPosition();
		while (p != NULL)
		{
			CaOccupantData* pOccupant = (CaOccupantData*)pBar->m_listOccupant.GetNext (p);
			pOccupant->m_rcOccupant.SetRectEmpty();
			pBar->m_rcBarUnit.SetRectEmpty();
		}
		return;
	}

	if (pDoc->m_PieChartCreationData.m_crFree)
	{
		pDC->FillRect (pBar->m_rcBarUnit, &br);
		Draw3DEffect  (pDC, pBar->m_rcBarUnit, m_cColorFree);
	}
	// 
	// Draw Borders:
	DrawBorder (pDC, pBar->m_rcBarUnit);

	//
	// Draw the name of the bar.
	COLORREF colorText = GetSysColor (COLOR_WINDOWTEXT);
	COLORREF colorTextOld = pDC->SetTextColor (colorText);
	CRect rcBarName (x1, y2+2, x2, r.bottom + m_yBottomMargin);
	nPreviousBkMode = pDC->SetBkMode (TRANSPARENT);
	if (pDoc->m_PieChartCreationData.m_bShowBarTitle)
	{
#if defined (MAINWIN)
		//
		// Draw only the first 4 characters:
		TCHAR buffd [128];
		_tcscpy (buffd, (LPCTSTR)pBar->m_strBarUnit);
		if (_tcslen (buffd) > 4)
		{
			int cC4 = 0;
			TCHAR* pch = buffd;
			while (*pch != _T('\0') && cC4 < 4)
			{
				cC4++;
				pch = _tcsinc(pch);
				if (cC4 == 4)
				{
					*pch = _T('\0');
					break;
				}
			}
		}
		pDC->DrawText  ((LPCTSTR)buffd, rcBarName, DT_CENTER|DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER);
#else
		pDC->DrawText  (pBar->m_strBarUnit, rcBarName, DT_CENTER|DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER);
#endif
		rcBarName.bottom -= 2;
		pBar->m_rcCaption = rcBarName;
	}
	pDC->SetBkMode (nPreviousBkMode);
	pDC->SetTextColor (colorTextOld);

	//
	// Draw the stacks (the occupants).
	if (pBar->m_listOccupant.IsEmpty()) 
		return;
	POSITION p = pBar->m_listOccupant.GetHeadPosition();
	sy1 = 0;
	sy2 = y2;
	while (p != NULL)
	{
		CaOccupantData* pOccupant = (CaOccupantData*)pBar->m_listOccupant.GetNext (p);
		nPixels   = (int)(dDensity * (pOccupant->m_fPercentage*pBar->m_nCapacity / 100));
		sy1 = sy2 - nPixels;
		pOccupant->m_rcOccupant = CRect (x1, sy1, x2, sy2);
		sy2 = sy1;
		pBar->m_rcBarUnit.bottom = sy1-1;
		if (p == NULL && pBar->m_rcBarUnit.bottom == pBar->m_rcBarUnit.top)
		{
			pOccupant->m_rcOccupant.top = y1;
		}

		// Coloured rectangle according to byte
		COLORREF occupantColor;
		if (bUsesPalette)
			occupantColor = PALETTERGB(GetRValue(pOccupant->m_colorRGB), GetGValue(pOccupant->m_colorRGB), GetBValue(pOccupant->m_colorRGB));
		else
			occupantColor = pOccupant->m_colorRGB;
		CBrush brush (occupantColor);
		pDC->FillRect (pOccupant->m_rcOccupant, &brush);
		//
		// Draw border:
		DrawBorder(pDC, pOccupant->m_rcOccupant);
		int nEffect = !(p || pDoc->m_PieChartCreationData.m_crFree)? EFFECT_ALL: EFFECT_RIGHT;
		Draw3DEffect  (pDC, pOccupant->m_rcOccupant, occupantColor, nEffect);
		Draw3DHatch (pDC, pOccupant->m_rcOccupant, pOccupant->m_bHatch, pOccupant->m_colorHatch);
	}
}

void CvStatisticBarView::DrawBorder(CDC* pDC, CRect r)
{
	int x1 = r.left;
	int y1 = r.top;
	int x2 = r.right;
	int y2 = r.bottom;
	if (!pDC)
		return;
	CPen  pen (PS_SOLID, 1, RGB (0, 0, 0));
	CPen* pOldPen = pDC->SelectObject (&pen);
	pDC->MoveTo (x1, y1);
	pDC->LineTo (x2, y1);
	pDC->LineTo (x2, y2);
	pDC->LineTo (x1, y2);
	pDC->LineTo (x1, y1);
	pDC->SelectObject (pOldPen);
}

void CvStatisticBarView::Draw3DEffect(CDC* pDC, CRect r, COLORREF crColor, UINT nEffect)
{
	int m_nVx = 6;
	int m_nVy = 5;
	CPoint p[4];

	if (!pDC)
		return;
	CPen   pen (PS_SOLID, 1, RGB (0, 0, 0));
	CPen*  pOldPen = pDC->SelectObject (&pen);
	CBrush br(crColor);
	CBrush*pOldBrush = pDC->SelectObject (&br); 
	if (nEffect & EFFECT_TOP)
	{
		p[0] = CPoint (r.left, r.top);
		p[1] = CPoint (r.left + m_nVx,	r.top - m_nVy);
		p[2] = CPoint (r.right+ m_nVx,	r.top - m_nVy);
		p[3] = CPoint (r.right, r.top);
		pDC->Polygon(p, 4);
	}
	if (nEffect & EFFECT_RIGHT)
	{
		p[0] = CPoint (r.right+ m_nVx, r.top  - m_nVy);
		p[1] = CPoint (r.right+ m_nVx, r.bottom - m_nVy);
		p[2] = CPoint (r.right, r.bottom);
		p[3] = CPoint (r.right, r.top);

		pDC->Polygon(p, 4);
	}
	pDC->SelectObject (pOldBrush); 
	pDC->SelectObject (pOldPen);
}


void CvStatisticBarView::DrawAxis (CDC* pDC, CRect rect, double dDensity)
{
	CdStatisticPieChartDoc* pDoc = (CdStatisticPieChartDoc*)GetDocument();
	CaBarInfoData*    pBar= pDoc->GetDiskInfo();
	int nPreviousBkMode = 0;

	double dMax = pBar->GetMaxCapacity();	// In Mega Bytes
	LONG nUnit = (LONG)AdjustGraphUnit(dMax);
	int nPixels = (int)(dDensity * dMax);
	if (rect.bottom - nPixels >= rect.bottom)
		return;
	dMax = (double)AdjustGraphMax(dMax);
	
	COLORREF colorText  = GetSysColor (COLOR_WINDOWTEXT);
	COLORREF colorTextOld;

	CPen pen (PS_SOLID, 1, colorText);
	CPen* pOldPen = pDC->SelectObject (&pen);
	//
	// Draw Y-Axis
	pDC->MoveTo (m_xLeftMargin, m_yTopMargin);
	pDC->LineTo (m_xLeftMargin, rect.bottom);
	//
	// Draw X-Axis
	int nXEnd = m_xLeftMargin +  pBar->GetBarUnitCount()*(m_xBarSeperator + m_nBarWidth) + 10;
	pDC->MoveTo (m_xLeftMargin,  rect.bottom);
	pDC->LineTo (nXEnd,          rect.bottom);
	//
	// Draw the Grid on the Y-Axis
	int nStart = rect.bottom;
	int nGrid  = 0;
	//
	// Draw the value on the Y-Axis.
	CFont font;
	if (UxCreateFont (font, _T("Courier New"), 14))
	{
		CString strValue;
		CRect   rcValue;
		BOOL    bOK    = FALSE;
		int     nFreq;
		int     nDraw  = 1;
		int     nValue = 0;
		CFont* pOldFond = pDC->SelectObject (&font);
		
		//
		// Draw the text Mb at the top of the Y-Axis
		CSize txSize= pDC->GetTextExtent (pBar->m_strUnit, pBar->m_strUnit.GetLength());
		CRect rcUnit (m_xLeftMargin - txSize.cx / 2, 0, m_xLeftMargin + txSize.cx / 2, m_yTopMargin);
		nPreviousBkMode = pDC->SetBkMode (TRANSPARENT);
		
		colorTextOld = pDC->SetTextColor (colorText);
		pDC->DrawText (pBar->m_strUnit, rcUnit, DT_RIGHT|DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER);
		pDC->SetBkMode (nPreviousBkMode);
		pDC->SetTextColor (colorTextOld);
		
		txSize= pDC->GetTextExtent ((LPCTSTR)"888", 3);
		while (nDraw <= 8) // 8 Grids Maximums.
		{
			if (txSize.cy <= dDensity*nUnit*nDraw)
			{
				bOK = TRUE;
				break;
			}
			nDraw *= 2;
		}
		nStart = rect.bottom;
		nFreq  = 1;
		int DrawCount = 0;
		while ((nStart - txSize.cy/2) > m_yTopMargin && bOK && DrawCount <= (8/nDraw))
		{
			if (nFreq == nDraw)
			{
				DrawCount++;
				nStart = (int)rect.bottom - (int)(DrawCount*nDraw*dDensity*nUnit);
				
				pDC->MoveTo (m_xLeftMargin-2, nStart);
				pDC->LineTo (m_xLeftMargin+2, nStart);
				
				COLORREF oldCr  = pDC->SetTextColor (RGB (0, 0, 255));
				rcValue.top     = nStart - txSize.cy/2;
				rcValue.bottom  = nStart + txSize.cy/2;
				rcValue.left    = 0;
				rcValue.right   = m_xLeftMargin-4;
				nFreq   = 1;
				nValue += nUnit*nDraw;
				strValue.Format (_T("%d"), nValue);
				nPreviousBkMode = pDC->SetBkMode (TRANSPARENT);
				pDC->DrawText ((LPCTSTR)strValue, rcValue, DT_CENTER|DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER);
				pDC->SetBkMode (nPreviousBkMode);
				pDC->SetTextColor (oldCr);
			}
			else
				nFreq *= 2;
		}
		pDC->SelectObject (pOldFond);
	}
	if (pOldPen) pDC->SelectObject (pOldPen);
}


void CvStatisticBarView::DrawSelectInfo (CaBarData* pBar, CaOccupantData* pOccupant, BOOL bCaption)
{
	CdStatisticPieChartDoc* pDoc    = (CdStatisticPieChartDoc*)GetDocument();
	CaBarInfoData*   pDiskInfo= pDoc->GetDiskInfo();
	CClientDC dc (this);
	CFont font;
	if (UxCreateFont (font, _T("Courier New"), 14))
	{
		CRect r;
		CString strMsg;
		double Capacity = 0.0;
		double pCent = 100.0;
		
		if (pOccupant)
		{
			//
			// Draw the percentage of the occupant.
			Capacity = (pOccupant->m_fPercentage * pBar->m_nCapacity) / 100;
			if ((int)Capacity > 0)
				strMsg.Format (_T("%s = (%0.2f%%, %0.2f%s)"), (LPCTSTR)pOccupant->m_strName, pOccupant->m_fPercentage, Capacity, (LPCTSTR)pDiskInfo->m_strUnit);
			else
			{
				if (!pDiskInfo->m_strSmallUnit.IsEmpty())
					strMsg.Format (_T("%s = (%0.2f%%, %0.4f%s)"), (LPCTSTR)pOccupant->m_strName, pOccupant->m_fPercentage, Capacity * 1024.0, (LPCTSTR)pDiskInfo->m_strSmallUnit);
				else
					strMsg.Format (_T("%s = (%0.2f%%, %0.2f%s)"), (LPCTSTR)pOccupant->m_strName, pOccupant->m_fPercentage, Capacity, (LPCTSTR)pDiskInfo->m_strUnit);
			}
		}
		else
		if (bCaption)
		{
			//
			// Draw the total capacity of the disk.
			if ((int)pBar->m_nCapacity > 0)
			{
				if (!pDiskInfo->m_strBarCaption.IsEmpty())
					strMsg.Format (_T("%s %s = %0.2f%s"), (LPCTSTR)pDiskInfo->m_strBarCaption, (LPCTSTR)pBar->m_strBarUnit, pBar->m_nCapacity, (LPCTSTR)pDiskInfo->m_strUnit);
				else
					strMsg.Format (_T("%s = %0.2f%s"), (LPCTSTR)pBar->m_strBarUnit, pBar->m_nCapacity, (LPCTSTR)pDiskInfo->m_strUnit);
			}
			else
			{
				if (!pDiskInfo->m_strSmallUnit.IsEmpty())
				{
					if (!pDiskInfo->m_strBarCaption.IsEmpty())
						strMsg.Format (_T("%s %s = %0.4f%s"), (LPCTSTR)pDiskInfo->m_strBarCaption, (LPCTSTR)pBar->m_strBarUnit, pBar->m_nCapacity * 1024.0, (LPCTSTR)pDiskInfo->m_strSmallUnit);
					else
						strMsg.Format (_T("%s = %0.4f%s"), (LPCTSTR)pBar->m_strBarUnit, pBar->m_nCapacity * 1024.0, (LPCTSTR)pDiskInfo->m_strSmallUnit);
				}
				else
				{
					if (!pDiskInfo->m_strBarCaption.IsEmpty())
						strMsg.Format (_T("%s %s = %0.2f%s"), (LPCTSTR)pDiskInfo->m_strBarCaption, (LPCTSTR)pBar->m_strBarUnit, pBar->m_nCapacity, (LPCTSTR)pDiskInfo->m_strUnit);
					else
						strMsg.Format (_T("%s = %0.2f%s"), (LPCTSTR)pBar->m_strBarUnit, pBar->m_nCapacity, (LPCTSTR)pDiskInfo->m_strUnit);
				}
			}
		}
		else
		{
			//
			// Draw the percentage of the disk free.
			POSITION pos = pBar->m_listOccupant.GetHeadPosition ();
			while (pos != NULL)
			{
				CaOccupantData* pOccupant = (CaOccupantData*)pBar->m_listOccupant.GetNext (pos);
				Capacity += pOccupant->m_fPercentage;
			}
			double dtotal = (pCent - Capacity)*pBar->m_nCapacity/100.0;   
			if ((int)dtotal > 0)
				strMsg.Format (_T("%s = (%0.2f%%, %0.2f%s)"), (LPCTSTR)m_strFree, pCent - Capacity, dtotal, (LPCTSTR)pDiskInfo->m_strUnit);
			else
			{
				if (!pDiskInfo->m_strSmallUnit.IsEmpty())
					strMsg.Format (_T("%s = (%0.2f%%, %0.4f%s)"), (LPCTSTR)m_strFree, pCent - Capacity, dtotal * 1024.0, (LPCTSTR)pDiskInfo->m_strSmallUnit);
				else
					strMsg.Format (_T("%s = (%0.2f%%, %0.2f%s)"), (LPCTSTR)m_strFree, pCent - Capacity, dtotal, (LPCTSTR)pDiskInfo->m_strUnit);
			}
		}
		GetClientRect (r);
		CFont* pOldFond = dc.SelectObject (&font);
		CSize txSize= dc.GetTextExtent ((LPCTSTR)strMsg, strMsg.GetLength());
		r.bottom = txSize.cy;
		m_nTextHeight = r.bottom;
		COLORREF oldTextBkColor = dc.SetBkColor (RGB(255, 255, 53));
		dc.DrawText (strMsg, r, DT_CENTER|DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER);
		dc.SetBkColor (oldTextBkColor);
		dc.SelectObject (pOldFond);
	}
}



void CvStatisticBarView::OnLButtonUp(UINT nFlags, CPoint point) 
{
	CdStatisticPieChartDoc* pDoc = (CdStatisticPieChartDoc*)GetDocument();
	if (pDoc && !pDoc->m_PieChartCreationData.m_bUseLButtonDown)
		return;
	CClientDC dc (this);
	OnPrepareDC  (&dc);

	CaBarData* pDiskData = NULL;
	CaBarInfoData* pBar= pDoc->GetDiskInfo();

	if (pBar->m_Tracker.m_nStyle != 0)
	{
		//
		// Erase the RectTracker
		CRect rcSave;
		pBar->m_Tracker.GetTrueRect (&rcSave);
		pBar->m_Tracker.m_nStyle = 0;
		InvalidateRect (rcSave);
		//
		// Erase the Disk info Popup.
		CRect r;
		GetClientRect (r);
		r.bottom = m_nTextHeight;
		InvalidateRect (r);
	}
	ReleaseCapture();
	CScrollView::OnLButtonUp(nFlags, point);
}


void CvStatisticBarView::Draw2D(CDC* pDC)
{
	CdStatisticPieChartDoc* pDoc = (CdStatisticPieChartDoc*)GetDocument();
	CaBarInfoData*    pBar= pDoc->GetDiskInfo();
	double iPixCount;
	double dMax = pBar->GetMaxCapacity(); // In Mega Bytes
	dMax = (double)AdjustGraphMax(dMax);
	CaBarData* pDiskData;
	CRect rec;
	GetClientRect (rec);
	m_sizeTotal.cx = rec.Width();
	if (pBar->GetBarUnitCount() == 0)
		return;
	//
	// Initialize the Color fo the Free.
	CString strFree;
	POSITION pos = pBar->m_listBar.GetHeadPosition();
//UKS	if (strFree.LoadString (IDS_DISKSPACE_FREE) == 0)
		strFree = _T("Free");
	while (pos != NULL)
	{
		CaBarPieItem* pLoc = (CaBarPieItem*)pBar->m_listBar.GetNext (pos);
		if (pLoc->m_strName.CompareNoCase (strFree) == 0)
		{
			m_cColorFree = pLoc->m_colorRGB; 
			m_strFree  = pLoc->m_strName;
			break;
		}
	}
	if (!m_cColorFree)
		m_cColorFree = RGB (0 ,192, 0);
	//
	// Calculate the pixel count (density) per unit (1 MB)
	rec.top  = rec.top  + m_yTopMargin;
	rec.bottom = rec.bottom - m_yBottomMargin;
	iPixCount = rec.Height() / dMax;
	DrawAxis (pDC, rec, iPixCount);

	int disk   = 0;
	pos = pBar->m_listUnit.GetHeadPosition();
	while (pos != NULL)
	{
		disk++;
		pDiskData = (CaBarData*)pBar->m_listUnit.GetNext (pos);
		DrawBars (pDC, rec, disk, pDiskData, iPixCount, m_nBarWidth);
	}
}

void CvStatisticBarView::Draw3D(CDC* pDC)
{
	CdStatisticPieChartDoc* pDoc = (CdStatisticPieChartDoc*)GetDocument();
	CaBarInfoData*  pBar= pDoc->GetDiskInfo();
	double iPixCount;
	double dMax = pBar->GetMaxCapacity();
	dMax = (double)AdjustGraphMax(dMax);
	CaBarData* pDiskData;
	CRect rec;
	GetClientRect (rec);
	m_sizeTotal.cx = rec.Width();
	if (pBar->GetBarUnitCount() == 0)
		return;
	CaPieChartCreationData& crData = pDoc->m_PieChartCreationData;

	//
	// Reinitialize the drawing area:
	int nCount = pBar->m_listUnit.GetCount();
	if (nCount == 0)
		return;

	// Determine whether we need a palette, and use it if found so
	BOOL bUsesPalette = FALSE;
	CPalette draw3dPalette;
	CPalette* pOldPalette = NULL;
	if (crData.m_pfnCreatePalette)
	{
		CClientDC mainDc(AfxGetMainWnd());
		if ((mainDc.GetDeviceCaps(RASTERCAPS) & RC_PALETTE) != 0) 
		{
			int palSize  = mainDc.GetDeviceCaps(SIZEPALETTE);
			if (palSize > 0) 
			{
				BOOL bSuccess = crData.m_pfnCreatePalette(&draw3dPalette);
				ASSERT (bSuccess);
				if (bSuccess) 
				{
					pOldPalette = pDC->SelectPalette (&draw3dPalette, FALSE);
					pDC->RealizePalette();
					bUsesPalette = TRUE;
				}
			}
		}
	}
	else
	if (crData.m_pPalette)
	{
		CPalette* pPalette = crData.m_pPalette;
		// Determine whether we need a palette, and use it if found so
		bUsesPalette = FALSE;
		CClientDC mainDc(AfxGetMainWnd());
		if ((mainDc.GetDeviceCaps(RASTERCAPS) & RC_PALETTE) != 0) 
		{
			int palSize  = mainDc.GetDeviceCaps(SIZEPALETTE);
			if (palSize > 0) 
			{
				ASSERT (palSize >= 256);
				pOldPalette = pDC->SelectPalette (pPalette, FALSE);
				pDC->RealizePalette();
				bUsesPalette = TRUE;
			}
		}
	}

	CRect r;
	GetClientRect (r);
	POSITION pos = pBar->m_listUnit.GetHeadPosition();
	if ((m_xLeftMargin + nCount*(m_xBarSeperator + m_nBarWidth) + 10) > r.Width())
	{
		m_sizeTotal.cx = m_xLeftMargin + nCount*(m_xBarSeperator + m_nBarWidth) - m_xBarSeperator + m_nXHatchWidth;
		SetScrollSizes(MM_TEXT, m_sizeTotal);
	}

	//
	// Initialize the Color fo the Free.
	CString strFree;
	pos = pBar->m_listBar.GetHeadPosition();
		strFree = _T("Free");
	while (pos != NULL)
	{
		CaBarPieItem* pLoc = (CaBarPieItem*)pBar->m_listBar.GetNext (pos);
		if (pLoc->m_strName.CompareNoCase (strFree) == 0)
		{
			m_cColorFree = pLoc->m_colorRGB; 
			m_strFree  = pLoc->m_strName;
			break;
		}
	}
	if (!m_cColorFree)
		m_cColorFree = pDoc->m_PieChartCreationData.m_crFree? pDoc->m_PieChartCreationData.m_crFree: RGB (0 ,192, 0);
	//
	// Calculate the pixel count (density) per unit (1 MB)
	rec.top    = rec.top    + m_yTopMargin;
	rec.bottom = rec.bottom - m_yBottomMargin;
	iPixCount  = rec.Height() / dMax;
	if (pDoc && pDoc->m_PieChartCreationData.m_bDrawAxis)
		DrawAxis (pDC, rec, iPixCount);
	
	int disk  = 0;
	pos = pBar->m_listUnit.GetHeadPosition();
	while (pos != NULL)
	{
		disk++;
		pDiskData = (CaBarData*)pBar->m_listUnit.GetNext (pos);
		DrawBars3D (pDC, rec, disk, pDiskData, iPixCount, m_nBarWidth, bUsesPalette);
	}

	// Finished with palette
	if (bUsesPalette) 
	{
		pDC->SelectPalette(pOldPalette, FALSE);
		//pDC->RealizePalette();
		draw3dPalette.DeleteObject();
	}

}


void CvStatisticBarView::Draw3DHatch (CDC* pDC, CRect r, BOOL bHatch, COLORREF crColorHatch)
{
	if (!bHatch)
		return;
	if (!pDC)
		return;
	if (r.Height() < 3)
		return;

	CRect rh = r;
	CBrush br(crColorHatch);
	int nY1, nY2 = r.Height();
	if (nY2 < m_nXHatchHeight)
		return;

	rh.left    = rh.right - m_nXHatchWidth - 4;
	rh.right   = rh.right - 2;
	nY1 = r.bottom;
	while ((nY1 - m_nXHatchHeight) >= r.top)
	{
		rh.bottom = nY1 -1;
		rh.top = rh.bottom - m_nXHatchHeight;
		nY1 = rh.top - m_nXHatchHeight;
		pDC->FillRect (rh, &br);
	}
}
