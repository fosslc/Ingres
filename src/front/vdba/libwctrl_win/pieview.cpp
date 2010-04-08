/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : pieview.cpp, Implementation file.  (View Window)
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Drawing the Pie chart
**
** History:
**
** xx-Apr-1997 (uk$so01)
**    Created
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
*/


#include "stdafx.h"
#include <math.h>
#include "piedoc.h"
#include "pieview.h"
#include "rctools.h"
#include "colorind.h" // UxCreateFont

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static const double pi   = 3.14159265;
static const double step = 1.0;
/////////////////////////////////////////////////////////////////////////////
// CvStatisticPieView

IMPLEMENT_DYNCREATE(CvStatisticPieView, CView)

BOOL CvStatisticPieView::IsFirstQuarter   (double degStart, double degEnd)
{
	if (degStart >= 0.0 && degEnd <= 90.0)
		return TRUE;
	return FALSE;
}

BOOL CvStatisticPieView::IsSecondQuarter  (double degStart, double degEnd)
{
	if (degStart >= 90.0 && degEnd <= 180.0)
		return TRUE;
	return FALSE;
}

BOOL CvStatisticPieView::IsThirdQuarter   (double degStart, double degEnd)
{
	if (degStart >= 180.0 && degEnd <= 270.0)
		return TRUE;
	return FALSE;
}

BOOL CvStatisticPieView::IsFourthQuarter  (double degStart, double degEnd)
{
	if (degStart >= 270.0 && degEnd <= 360.0)
		return TRUE;
	return FALSE;
}

BOOL CvStatisticPieView::PtInFisrtQuarter (double degStart, double degEnd, CPoint point)
{
	double d1A = 0.0, d2A = 0.0;
	double px = (double)point.x;
	double py = (double)point.y;
	BOOL   B [2];
	CPoint p [2];
	p[0] = CPoint (fx (degStart), fy (degStart));
	p[1] = CPoint (fx (degEnd),   fy (degEnd));

	ASSERT (degStart < 90.0);
	//
	// Line segment D1 of vector direction 'degStart'
	if (fabs (degStart) <= 0.0001)
	{
		B[0] = (point.y < m_center.y);
	}
	else
	{
		double x = p[0].x;
		double y = p[0].y;

		d1A = ((y - (double)m_center.y) / (x - (double)m_center.x));
		B[0] = (point.y < m_center.y + (int)((double)point.x -(double)m_center.x) * d1A);
	}
	if (!B[0])
		return FALSE;
	//
	// Line segment D2 of vector direction 'degEndt'
	if (fabs (degEnd - 90.00) <= 0.0001)
	{
		B[1] = (point.x > m_center.x);
	}
	else
	{
		double x = p[1].x;
		double y = p[1].y;

		d2A = ((y - (double)m_center.y) / (x - (double)m_center.x));
		B[1] = (point.y > m_center.y + (int)((double)point.x -(double)m_center.x) * d2A);
	}
	return B[1];
}

BOOL CvStatisticPieView::PtInSecondQuarter(double degStart, double degEnd, CPoint point)
{
	double d1A = 0.0, d2A = 0.0;
	double px = (double)point.x;
	double py = (double)point.y;
	BOOL   B [2];
	CPoint p [2];
	p[0] = CPoint (fx (degStart), fy (degStart));
	p[1] = CPoint (fx (degEnd),   fy (degEnd));

	ASSERT (degStart >= 90.0 && degEnd <= 180.0);
	//
	// Line segment D1 of vector direction 'degStart'
	if (fabs (degStart- 90.0) <= 0.0001)
	{
		B[0] = (point.x < m_center.x);
	}
	else
	{
		double x = p[0].x;
		double y = p[0].y;

		d1A = ((y - (double)m_center.y) / (x - (double)m_center.x));
		B[0] = (point.y > m_center.y + (int)((double)point.x -(double)m_center.x) * d1A);
	}
	if (!B[0])
		return FALSE;
	//
	// Line segment D2 of vector direction 'degEndt'
	if (fabs (degEnd - 180.00) <= 0.0001)
	{
		B[1] = (point.y < m_center.y);
	}
	else
	{
		double x = p[1].x;
		double y = p[1].y;

		d2A = ((y - (double)m_center.y) / (x - (double)m_center.x));
		B[1] = (point.y < m_center.y + (int)((double)point.x -(double)m_center.x) * d2A);
	}
	return B[1];
}

BOOL CvStatisticPieView::PtInThirdQuarter (double degStart, double degEnd, CPoint point)
{
	double d1A = 0.0, d2A = 0.0;
	double px = (double)point.x;
	double py = (double)point.y;
	BOOL   B [2];
	CPoint p [2];
	p[0] = CPoint (fx (degStart), fy (degStart));
	p[1] = CPoint (fx (degEnd),   fy (degEnd));

	ASSERT (degStart >= 180.0 && degEnd <= 270.0);
	//
	// Line segment D1 of vector direction 'degStart'
	if (fabs (degStart- 180.0) <= 0.0001)
	{
		B[0] = (point.y > m_center.y);
	}
	else
	{
		double x = p[0].x;
		double y = p[0].y;

		d1A = ((y - (double)m_center.y) / (x - (double)m_center.x));
		B[0] = (point.y > m_center.y + (int)((double)point.x -(double)m_center.x) * d1A);
	}
	if (!B[0])
		return FALSE;
	//
	// Line segment D2 of vector direction 'degEndt'
	if (fabs (degEnd - 270.00) <= 0.0001)
	{
		B[1] = (point.x < m_center.x);
	}
	else
	{
		double x = p[1].x;
		double y = p[1].y;

		d2A = ((y - (double)m_center.y) / (x - (double)m_center.x));
		B[1] = (point.y < m_center.y + (int)((double)point.x -(double)m_center.x) * d2A);
	}
	return B[1];
}

BOOL CvStatisticPieView::PtInFourthQuarter(double degStart, double degEnd, CPoint point)
{
	double d1A = 0.0, d2A = 0.0;
	double px = (double)point.x;
	double py = (double)point.y;
	BOOL   B [2];
	CPoint p [2];
	p[0] = CPoint (fx (degStart), fy (degStart));
	p[1] = CPoint (fx (degEnd),   fy (degEnd));

	ASSERT (degStart >= 270.0 && degEnd <= 360.0);
	//
	// Line segment D1 of vector direction 'degStart'
	if (fabs (degStart- 270.0) <= 0.0001)
	{
		B[0] = (point.x > m_center.x);
	}
	else
	{
		double x = p[0].x;
		double y = p[0].y;

		d1A = ((y - (double)m_center.y) / (x - (double)m_center.x));
		B[0] = (point.y < m_center.y + (int)((double)point.x -(double)m_center.x) * d1A);
	}
	if (!B[0])
		return FALSE;
	//
	// Line segment D2 of vector direction 'degEndt'
	if (fabs (degEnd - 360.00) <= 0.0001)
	{
		B[1] = (point.y > m_center.y);
	}
	else
	{
		double x = p[1].x;
		double y = p[1].y;

		d2A = ((y - (double)m_center.y) / (x - (double)m_center.x));
		B[1] = (point.y > m_center.y + (int)((double)point.x -(double)m_center.x) * d2A);
	}
	return B[1];
}


BOOL CvStatisticPieView::IsInsideDisk (CPoint point)
{
	double X = point.x - m_center.x;
	double Y = point.y - m_center.y;

	if (pow (X/m_dRadialX, 2.0) + pow (Y/m_dRadialY, 2.0) <= 1.0)
		return TRUE;
	return FALSE;
}

BOOL CvStatisticPieView::IsInsideSector (double degStart, double degEnd, CPoint point)
{
	ASSERT (degStart < degEnd);
	if (IsFirstQuarter (degStart, degEnd))
		return PtInFisrtQuarter (degStart, degEnd, point);
	else
	if (IsSecondQuarter (degStart, degEnd))
		return PtInSecondQuarter(degStart, degEnd, point);
	else
	if (IsThirdQuarter (degStart, degEnd))
		return PtInThirdQuarter (degStart, degEnd, point);
	else
	if (IsFourthQuarter (degStart, degEnd))
		return PtInFourthQuarter(degStart, degEnd, point);
	else
	{
		if (degStart < 360.0 && degEnd > 360.0)
		{
			if (IsInsideSector (degStart, 360.0,   point))
				return TRUE;
			return IsInsideSector (0.001, degEnd-360.0, point);
		}
		else
		if (degStart >= 360.0 && degEnd > 360.0)
		{
			return IsInsideSector (degStart-360.0, degEnd-360.0, point);
		}
		else
		{
			if (degStart < 90.0)
			{
				if (IsInsideSector (degStart, 90.0,   point))
					return TRUE;
				return IsInsideSector (90.0,  degEnd, point);
			}
			if (degStart < 180.0)
			{
				if (IsInsideSector (degStart, 180.0,   point))
					return TRUE;
				return IsInsideSector (180.0,  degEnd, point);
			}
			if (degStart < 270.0)
			{
				if (IsInsideSector (degStart, 270.0,   point))
					return TRUE;
				return IsInsideSector (270.0,  degEnd, point);
			}
		}
		//
		// degStart > 270, this must be in the 4th Quarter
		// degEnd must be <= 360.
		// It should have been tested in the 4th Quarter !!!
		ASSERT (FALSE);
		return FALSE;
	}

	return FALSE;
}


void CvStatisticPieView::EllipseTracker (double degStart, double degEnd)
{
	int iOldRop2;
	double start = degStart;
	CRect  rcSquare;
	CClientDC dc (this);
	CPoint p [4];
	CPen   pen (PS_SOLID, 2, RGB (255, 0, 0));
	CPen*  oldPen = NULL;
	p[0] = CPoint (fx (degStart), fy (degStart));
	p[1] = CPoint (fx (degEnd),   fy (degEnd));
	iOldRop2 = dc.SetROP2 (R2_XORPEN);

	CdStatisticPieChartDoc* pDoc = (CdStatisticPieChartDoc*)GetDocument();
	CaPieInfoData*  pPie = pDoc->GetPieInfo();

	oldPen = dc.SelectObject (&pen);
	if (pPie->m_listPie.GetCount() > 1)
	{
		dc.MoveTo (m_center.x, m_center.y); dc.LineTo (p[0]);
		dc.MoveTo (m_center.x, m_center.y); dc.LineTo (p[1]);
	}
	//
	// Ellipse arc.
	while (start < degEnd)
	{
		if ((start < degEnd) && (start + step > degEnd))
			start = degEnd;
		else
			start += step;
		p[1] = CPoint (fx (start), fy (start));
		dc.MoveTo (p[0]);
		dc.LineTo (p[1]);
		p[0] = p[1];
	}
	if (oldPen) dc.SelectObject (oldPen);
	if (pPie->m_listPie.GetCount() > 1)
	{
		//
		// Draw the 3 or 4 handles (small square).
		CBrush br (RGB (0, 0, 0));
		p[0] = CPoint (fx (degStart), fy (degStart));
		p[1] = CPoint (fx (degEnd),   fy (degEnd));
		rcSquare = CRect (m_center.x-4, m_center.y-4, m_center.x+4, m_center.y+4);
		dc.Rectangle (rcSquare);
		rcSquare = CRect (p[0].x-4, p[0].y-4, p[0].x+4, p[0].y+4);
		dc.Rectangle (rcSquare);
		rcSquare = CRect (p[1].x-4, p[1].y-4, p[1].x+4, p[1].y+4);
		dc.Rectangle (rcSquare);
		if (degEnd - degStart >= 3)
		{
			p[2] = CPoint (fx (degStart + (degEnd - degStart)/2.0),   fy (degStart + (degEnd - degStart)/2.0));
			rcSquare = CRect (p[2].x-4, p[2].y-4, p[2].x+4, p[2].y+4);
			dc.Rectangle (rcSquare);
		}
	}
	else
	{
		//
		// Draw the 4 handles (small square).
		CBrush br (RGB (0, 0, 0));
		p[0] = CPoint (fx (0.0),   fy (0.0));
		p[1] = CPoint (fx (90.0),  fy (90.0));
		p[2] = CPoint (fx (180.0), fy (180.0));
		p[3] = CPoint (fx (270.0), fy (270.0));
		rcSquare = CRect (p[0].x-4, p[0].y-4, p[0].x+4, p[0].y+4);
		dc.Rectangle (rcSquare);
		rcSquare = CRect (p[1].x-4, p[1].y-4, p[1].x+4, p[1].y+4);
		dc.Rectangle (rcSquare);
		rcSquare = CRect (p[2].x-4, p[2].y-4, p[2].x+4, p[2].y+4);
		dc.Rectangle (rcSquare);
		rcSquare = CRect (p[3].x-4, p[3].y-4, p[3].x+4, p[3].y+4);
		dc.Rectangle (rcSquare);
	}
	dc.SetROP2 (iOldRop2);
}

CvStatisticPieView::CvStatisticPieView()
{
	m_xLeftMargin   = 10;
	m_xRightMargin  = 10;
	m_yTopMargin    = 10;
	m_yBottomMargin = 30;

	m_bDrawPieBorder= TRUE;
	m_size.cx = 500;
	m_size.cy = 500;
	m_bInitialize = FALSE;

	m_nXDiffRadius = 20;
	m_nYDiffRadius = 26;
}

CvStatisticPieView::~CvStatisticPieView()
{
}


BEGIN_MESSAGE_MAP(CvStatisticPieView, CView)
	//{{AFX_MSG_MAP(CvStatisticPieView)
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CvStatisticPieView drawing

void CvStatisticPieView::DrawPie (CDC* pDC, double degStart, double degEnd, COLORREF color, BOOL bSplit)
{
	BOOL bEntire = FALSE;
	double xc = 180.0, start = degStart;
	CPoint p [5];
	CBrush brColorPie (color);
	CBrush* pOldBrush = NULL;
	CPen    penColorPie(PS_SOLID, 1, color);
	CPen*   pOldPen   = NULL;
	CdStatisticPieChartDoc* pDoc = (CdStatisticPieChartDoc*)GetDocument();
	CaPieInfoData*     pPie = pDoc->GetPieInfo();
	bEntire = (pPie->m_listPie.GetCount() > 1)? FALSE: TRUE;

	if (degStart <= 360.0 && degEnd <= 360.0 && degStart <= degEnd)
	{
		//
		// Draw pie case 1:
		if (degStart >= 0.0 && degStart < 180.0)
		{
			if (degEnd > 180.0)
			{
				//
				// Draw at [degStart, 180] and [180, degEnd]
				DrawPie (pDC, degStart, 180.0,  color, TRUE);
				DrawPie (pDC, 180.001,  degEnd, color, TRUE);
				//
				// Draw the border
				if (m_bDrawPieBorder)
				{
					p[0] = CPoint (fx (degStart), fy (degStart));
					start  = degStart;
					while (start < degEnd)
					{
						if ((start < degEnd) && (start + step > degEnd))
							start = degEnd;
						else
							start += step;
						p[1] = CPoint (fx (start), fy (start));
						pDC->MoveTo (p[0]);
						pDC->LineTo (p[1]);
						p[0] = p[1];
					}
					p[0] = CPoint (m_center.x, m_center.y);
					p[1] = CPoint (fx (degStart), fy (degStart));
					p[2] = CPoint (fx (degEnd),   fy (degEnd));
					if (!bEntire && !bSplit)
					{
						pDC->MoveTo (p[0]);  pDC->LineTo (p[1]);
						pDC->MoveTo (p[0]);  pDC->LineTo (p[2]);
					}
				}
			}
			else
			{
				double startSave = start;
				pOldBrush = pDC->SelectObject (&brColorPie); 
				pOldPen   = pDC->SelectObject (&penColorPie); 
				p[0] = CPoint (m_center.x, m_center.y);
				p[1] = CPoint (fx (degStart), fy (degStart));
				while (start < degEnd)
				{
					if ((start < degEnd) && (start + step > degEnd))
						start = degEnd;
					else
						start += step;
					p[2] = CPoint (fx (start), fy (start));
					pDC->Polygon (p, 3);
					p[1] = p[2];
				}
				pDC->SelectObject (pOldBrush); 
				pDC->SelectObject (pOldPen); 
				
				if (m_bDrawPieBorder)
				{
					p[0] = CPoint (fx (startSave), fy (startSave));
					start  = startSave;
					while (start < degEnd)
					{
						if ((start < degEnd) && (start + step > degEnd))
							start = degEnd;
						else
							start += step;
						p[1] = CPoint (fx (start), fy (start));
						pDC->MoveTo (p[0]);
						pDC->LineTo (p[1]);
						p[0] = p[1];
					}
					p[0] = CPoint (m_center.x, m_center.y);
					p[1] = CPoint (fx (startSave), fy (startSave));
					p[2] = CPoint (fx (degEnd), fy (degEnd));
					if (!bSplit && !bEntire)
					{
						pDC->MoveTo (p[0]);  pDC->LineTo (p[1]);
						pDC->MoveTo (p[0]);  pDC->LineTo (p[2]);
					}
				}
			}
		}
		else // (degStart > 180.0)
		{
			double startSave = start;
			pOldBrush = pDC->SelectObject (&brColorPie); 
			pOldPen   = pDC->SelectObject (&penColorPie); 
			
			p[0] = CPoint (m_center.x, m_center.y);
			p[1] = CPoint (fx (degStart), fy (degStart));
			while (start < degEnd)
			{
				if ((start < degEnd) && (start + step > degEnd))
					start = degEnd;
				else
					start += step;
				p[2] = CPoint (fx (start), fy (start));
				pDC->Polygon (p, 3);        // Uper pie
			
				p[4] = CPoint (p[1].x, p[1].y + m_pieHeight);
				p[3] = CPoint (p[2].x, p[2].y + m_pieHeight);
				pDC->Polygon (&p[1], 4);    // Lower pie
				p[1] = p[2];
			}
			pDC->SelectObject (pOldBrush); 
			pDC->SelectObject (pOldPen); 
			if (m_bDrawPieBorder)
			{
				p[0] = CPoint (fx (startSave), fy (startSave));
				start  = startSave;
				while (start < degEnd)
				{
					if ((start < degEnd) && (start + step > degEnd))
						start = degEnd;
					else
						start += step;
					p[1] = CPoint (fx (start), fy (start));
					pDC->MoveTo (p[0]);                 // Border of the upper ellipse
					pDC->LineTo (p[1]);                 // Border of the upper ellipse
					pDC->MoveTo (p[0].x, p[0].y + m_pieHeight);  // Border of the lower ellipse
					pDC->LineTo (p[1].x, p[1].y + m_pieHeight);  // Border of the lower ellipse
					p[0] = p[1];
				}
				p[0] = CPoint (m_center.x, m_center.y);
				p[1] = CPoint (fx (startSave), fy (startSave));
				p[2] = CPoint (fx (degEnd), fy (degEnd));
				if (!bSplit && !bEntire)
				{
					pDC->MoveTo (p[0]);  pDC->LineTo (p[1]);
					pDC->MoveTo (p[0]);  pDC->LineTo (p[2]);
				}
				pDC->MoveTo (p[1].x, p[1].y);  pDC->LineTo (p[1].x, p[1].y + m_pieHeight); 
				pDC->MoveTo (p[2].x, p[2].y);  pDC->LineTo (p[2].x, p[2].y + m_pieHeight); 
			}
		}
	}
	else
	{
		//
		// Draw pie case 2:
		if (degStart < 360.0 && degEnd >= 360.0)
		{
			DrawPie (pDC, degStart, 360.0,  color, TRUE);
			DrawPie (pDC, 0.0, degEnd-360.0,  color, TRUE);
		}
		else
		if (degStart >= 360.0 && degEnd >= 360.0)
		{
			DrawPie (pDC, degStart - 360.0, degEnd -360.0,  color, TRUE);
		}
	}
}

static void DrawPaneTooSmall (CDC* pDC, CString& strMsg, CRect r)
{
	CSize txSize (40, 16);
	CFont font;
	if (UxCreateFont (font, _T("Times New Roman"), 20, FW_BOLD, TRUE))
	{
		CBrush br (GetSysColor (COLOR_WINDOW));
		COLORREF colorText    = GetSysColor (COLOR_WINDOWTEXT);
		COLORREF colorTextOld = pDC->SetTextColor (colorText);
		COLORREF oldTextBkColor = pDC->SetBkColor (RGB(255, 255, 53));
		CFont* pOldFond = pDC->SelectObject (&font);
		txSize = pDC->GetTextExtent (strMsg, strMsg.GetLength());

		pDC->FillRect (r, &br);
		pDC->DrawText (strMsg, r, DT_CENTER|DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER);
	}
}



void CvStatisticPieView::OnDraw(CDC* pDC)
{
	CRect r;
	CdStatisticPieChartDoc* pDoc = (CdStatisticPieChartDoc*)GetDocument();
	CaPieChartCreationData& crData = pDoc->m_PieChartCreationData;
	if (m_EllipseRect.Width() < 10 || m_EllipseRect.Height() < 10)
	{
		// the new string below will be put in the resource file in an independent
		// change,  because of a concurrent, independent resource change
		CString strMsg = _T("Please increase pane size");
		if (crData.m_uIDS_IncreasePaneSize > 0)
			strMsg.LoadString (crData.m_uIDS_IncreasePaneSize);
		GetClientRect (r);
		DrawPaneTooSmall (pDC, strMsg, r);
		return; // The figure is too small
	}

	CaLegendData* pData = NULL;
	CaPieInfoData* pPie = pDoc->GetPieInfo();
	CFont font;
	BOOL bCreateFont = UxCreateFont (font, _T("Courier New"), 16);
	if (!pPie->m_strTitle.IsEmpty())
	{
		if (bCreateFont)
		{
			int iPrevMode;
			//
			// Draw the Title of the Pie Chart.
			COLORREF colorText    = GetSysColor (COLOR_WINDOWTEXT);
			COLORREF colorTextOld = pDC->SetTextColor (colorText);

			GetClientRect (r);
			CFont* pOldFond = pDC->SelectObject (&font);
			CSize txSize= pDC->GetTextExtent ((LPCTSTR)pPie->m_strTitle, pPie->m_strTitle.GetLength());
			r.top = r.bottom - txSize.cy;
			iPrevMode = pDC->SetBkMode (TRANSPARENT);
			pDC->DrawText (pPie->m_strTitle, r, DT_CENTER|DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER);
			pDC->SetBkMode (iPrevMode);
			pDC->SelectObject (pOldFond);
			pDC->SetTextColor (colorTextOld);
		} 
	}
	//
	// For the orientation:
	if (pPie->m_bDrawOrientation)
	{
		GetClientRect (r);
		int ty = fy (90.00) - 18;
		if (ty < r.top)
			ty = r.top;
		m_imageDirection.Draw (pDC, 0, CPoint (r.Width()/2 - 15, ty), ILD_TRANSPARENT);
		
		if (bCreateFont)
		{
			int iPrevMode;
			TCHAR tchszBlock0[] = _T("0");
			//
			// Draw the Title of the Pie Chart.
			COLORREF colorText    = GetSysColor (COLOR_WINDOWTEXT);
			COLORREF colorTextOld = pDC->SetTextColor (colorText);

			r.left = r.Width()/2 + 2;
			r.top  = ty;
			CFont* pOldFond = pDC->SelectObject (&font);
			CSize txSize= pDC->GetTextExtent (tchszBlock0, 7);
			r.right = r.left + txSize.cx;
			r.bottom= r.top  + txSize.cy;
			iPrevMode = pDC->SetBkMode (TRANSPARENT);
			pDC->DrawText (tchszBlock0, r, DT_LEFT|DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER);
			pDC->SetBkMode (iPrevMode);
			pDC->SelectObject (pOldFond);
			pDC->SetTextColor (colorTextOld);
		}
	}

	if (pPie->m_bError)
	{
		//
		// An error has occure during the data processing phase ...
		return;
	}
	else
	if (pPie->m_listLegend.GetCount() == 0)
	{
		//
		// No Databases in this Location
		CFont font;
		CString strMsg;
		if (crData.m_uIDS_LegendCount > 0)
			AfxFormatString1 (strMsg, crData.m_uIDS_LegendCount, pPie->m_strMainItem);
		else
			strMsg.Format (_T("No Databases in Location %s"), (LPCTSTR)pPie->m_strMainItem);
		if (pDoc && crData.m_bDBLocation)
			DrawMessage (pDC, strMsg);
		return;
	}
	else
	if (pPie->m_listPie.GetCount() == 0)
	{
		//
		// All databases in this location are empty.
		CFont font;
		CString strMsg;
		if (crData.m_uIDS_PieIsEmpty > 0)
			AfxFormatString1 (strMsg, crData.m_uIDS_PieIsEmpty, pPie->m_strMainItem);
		else
			strMsg.Format (_T("Location %s is empty."), (LPCTSTR)pPie->m_strMainItem);

		if (pDoc && crData.m_bDBLocation)
			DrawMessage (pDC, strMsg);
		return;
	}

	POSITION pos = pPie->m_listPie.GetHeadPosition();
	while (pos != NULL)
	{
		pData = (CaLegendData*)pPie->m_listPie.GetNext (pos);
		DrawPie (pDC, pData->m_dDegStart, pData->m_dDegEnd, pData->m_colorRGB);
		DrawUperBorder (pDC, pData->m_dDegStart, pData->m_dDegEnd);
	}

	if (pPie->m_bDrawOrientation)
		DrawLogFileExtraInfo (pDC, pPie);
}

/////////////////////////////////////////////////////////////////////////////
// CvStatisticPieView diagnostics

#ifdef _DEBUG
void CvStatisticPieView::AssertValid() const
{
	CView::AssertValid();
}

void CvStatisticPieView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CvStatisticPieView message handlers

void CvStatisticPieView::OnInitialUpdate() 
{
	CdStatisticPieChartDoc* pDoc = (CdStatisticPieChartDoc*)GetDocument();
	CaPieInfoData*     pPie = pDoc->GetPieInfo();
	ASSERT (pPie);
	if (!pPie->m_strTitle.IsEmpty() && !pPie->m_bDrawOrientation)
		m_yBottomMargin = 50;
	if (!pDoc->m_PieChartCreationData.m_bUseLButtonDown && !pPie->m_bDrawOrientation)
		m_yTopMargin = 1;

	m_EllipseRect = CRect (0, 0, m_size.cx, m_size.cy);
	m_EllipseRect.left   += m_xLeftMargin;
	m_EllipseRect.right  -= m_xRightMargin;
	m_EllipseRect.top    += m_yTopMargin;
	m_EllipseRect.bottom -= m_yBottomMargin;
	
	m_center   = CPoint (m_EllipseRect.Width()/2 + m_xLeftMargin, m_EllipseRect.Height()/2 + m_yTopMargin);
	m_dRadialX = m_EllipseRect.Width() /2;
	m_dRadialY = 0.5* m_dRadialX; 
	if (!m_bInitialize)
	{
		m_imageDirection.Create(IDB_ORIENTATION, 16, 1, RGB(255, 0, 255));
		m_bInitialize = TRUE;
	}
}

void CvStatisticPieView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	CdStatisticPieChartDoc* pDoc = (CdStatisticPieChartDoc*)GetDocument();
	CaPieInfoData*  pPie = pDoc->GetPieInfo();

	if (pPie && pPie->m_bDrawOrientation)
	{
		m_size.cx=cx;
		m_size.cy=cy;
		LogFileSetMargins();
	}

	m_EllipseRect = CRect (0, 0, m_size.cx, m_size.cy);
	m_EllipseRect.left   += m_xLeftMargin;
	m_EllipseRect.right  -= m_xRightMargin;
	m_EllipseRect.top    += m_yTopMargin;
	m_EllipseRect.bottom -= m_yBottomMargin;
	
	m_center   = CPoint (m_EllipseRect.Width()/2 + m_xLeftMargin, m_EllipseRect.Height()/2 + m_yTopMargin);
	m_dRadialX = m_EllipseRect.Width() /2;
	m_dRadialY = 0.5* m_dRadialX; 
}

void CvStatisticPieView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CdStatisticPieChartDoc* pDoc = (CdStatisticPieChartDoc*)GetDocument();
	ASSERT (pDoc);
	if (pDoc && !pDoc->m_PieChartCreationData.m_bUseLButtonDown)
	{
		CView::OnLButtonDown(nFlags, point);
		return;
	}

	BOOL bIsideDisk = FALSE;
	CClientDC dc(this);
	CPoint p = point;
	OnPrepareDC (&dc);
	dc.DPtoLP (&p);
	SetCapture();
	
	CaLegendData* pData     = NULL;
	CaPieInfoData*     pPie = pDoc->GetPieInfo();
	POSITION pos = pPie->m_listPie.GetHeadPosition();
	
	if (IsInsideDisk (p))
		bIsideDisk = TRUE;
	while (bIsideDisk && pos != NULL)
	{
		pData = (CaLegendData*)pPie->m_listPie.GetNext (pos);
		if (IsInsideSector (pData->m_dDegStart, pData->m_dDegEnd, p))
		{
			pData->m_bFocus = TRUE;
			EllipseTracker (pData->m_dDegStart, pData->m_dDegEnd);
			DrawSelectInfo (pPie, pData);
			break;
		}
	}
	CView::OnLButtonDown(nFlags, point);
}


int CvStatisticPieView::fx (double deg)
{
	double xc = 180.0;
	return (int)(m_center.x + m_dRadialX * cos (deg * pi / xc));
}

int CvStatisticPieView::fx (double deg, CPoint center)
{
	double xc = 180.0;
	return (int)(center.x + m_dRadialX * cos (deg * pi / xc));
}

int CvStatisticPieView::fy (double deg)
{
	double xc = 180.0;
	return (int)(m_center.y - m_dRadialY * sin (deg * pi / xc));
}

int CvStatisticPieView::fy (double deg, CPoint center)
{
	double xc = 180.0;
	return (int)(center.y - m_dRadialY * sin (deg * pi / xc));
}

void CvStatisticPieView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo) 
{
	CView::OnPrepareDC(pDC, pInfo);
	pDC->SetMapMode (MM_ISOTROPIC);

	CRect r;
	GetClientRect (r);
	m_size.cx = r.Width();
	m_size.cy = r.Height();
	pDC->SetWindowExt (m_size);

	pDC->SetViewportExt (r.Width(), r.Height());

	m_EllipseRect = CRect (0, 0, m_size.cx, m_size.cy);
	m_EllipseRect.left   += m_xLeftMargin;
	m_EllipseRect.right  -= m_xRightMargin;
	m_EllipseRect.top    += m_yTopMargin;
	m_EllipseRect.bottom -= m_yBottomMargin;
	
	m_center   = CPoint (m_EllipseRect.Width()/2 + m_xLeftMargin, m_EllipseRect.Height()/2 + m_yTopMargin);
	m_dRadialX = m_EllipseRect.Width() /2;
	while ((1.0 / 3.0)* m_dRadialX >  m_EllipseRect.Height()/2)
	{
		m_EllipseRect.left   += 1;
		m_EllipseRect.right  -= 1;
		m_center   = CPoint (m_EllipseRect.Width()/2 + m_EllipseRect.left, m_EllipseRect.Height()/2 + m_yTopMargin);
		m_dRadialX = m_EllipseRect.Width() /2;
	}
	m_dRadialY = (1.0 / 3.0)* m_dRadialX; 
	m_pieHeight= (int)(m_dRadialY * 0.2);

}

void CvStatisticPieView::OnLButtonUp(UINT nFlags, CPoint point) 
{
	CdStatisticPieChartDoc* pDoc = (CdStatisticPieChartDoc*)GetDocument();
	ASSERT (pDoc);
	if (pDoc && !pDoc->m_PieChartCreationData.m_bUseLButtonDown)
	{
		CView::OnLButtonUp(nFlags, point);
		return;
	}
	CClientDC dc(this);
	CPoint p = point;
	OnPrepareDC (&dc);
	dc.DPtoLP (&p);
	ReleaseCapture();

	CaLegendData* pData     = NULL;
	CaPieInfoData*     pPie = pDoc->GetPieInfo();
	POSITION pos = pPie->m_listPie.GetHeadPosition();

	while (pos != NULL)
	{
		pData = (CaLegendData*)pPie->m_listPie.GetNext (pos);
		if (pData->m_bFocus)
		{
			pData->m_bFocus = FALSE;
			EllipseTracker (pData->m_dDegStart, pData->m_dDegEnd);
			break;
		}
	}
	//
	// Erase the Disk info Popup.
	CRect r;
	GetClientRect (r);
	r.bottom = m_nTextHeight;
	InvalidateRect (r);
	CView::OnLButtonUp(nFlags, point);
}


void CvStatisticPieView::DrawSelectInfo (CaPieInfoData* pPie, CaLegendData* pData)
{
	CClientDC dc (this);
	OnPrepareDC (&dc);
	CFont font;
	if (UxCreateFont (font, _T("Courier New"), 14))
	{
		CRect r;
		CString strMsg;
		double Capacity = 0.0;
		double pCent    = 100.0;
		
		//
		// Draw the percentage of the Sector.
		Capacity = (pData->m_dPercentage * pPie->m_dCapacity) / 100.0;
		if ((int)pPie->m_dCapacity > 0 || pPie->m_strSmallUnit.IsEmpty())
		{
			if (pPie->m_bDrawOrientation) // It's for the Log File
			{
				strMsg.Format (
					pPie->m_strFormat,
					(LPCTSTR)pData->m_strName, 
					pData->m_dPercentage, 
					(int)Capacity, 
					(LPCTSTR)pPie->m_strUnit);
			}
			else
			{
				strMsg.Format (
					pPie->m_strFormat,
					(LPCTSTR)pData->m_strName, 
					pData->m_dPercentage, 
					Capacity, 
					(LPCTSTR)pPie->m_strUnit);
			}
		}
		else
		{
			if (!pPie->m_strSmallUnit.IsEmpty())
			{
				strMsg.Format (
					_T("%s = (%0.2f%%, %0.4f%s)"), 
					(LPCTSTR)pData->m_strName, 
					pData->m_dPercentage, 
					Capacity*1024.0, 
					(LPCTSTR)pPie->m_strSmallUnit);
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

void CvStatisticPieView::DrawMessage (CDC* pDC, const CString& strMessage)
{
	CDC dc;
	CFont font;
	if (UxCreateFont (font, _T("Times New Roman"), 20, FW_BOLD, TRUE))
	{
		int   iPrevMode, res, h;
		CRect r = m_EllipseRect;
		CFont*   pOldFond   = pDC->SelectObject (&font);
		COLORREF oldColor   = pDC->SetTextColor (RGB (0, 0, 255));
		CBrush   br1 (RGB (255, 255, 0));
		CBrush   br2 (RGB (0, 255, 255));
		CBrush*  oldBrush;
		iPrevMode = pDC->SetBkMode (TRANSPARENT);
 
		dc.CreateCompatibleDC (pDC);
		dc.SelectObject (&font);
		CSize size= dc.GetTextExtent (strMessage);

		res = size.cx / m_EllipseRect.Width();
		h   = (2 + res)*size.cy;
		r.top    = m_center.y - (h/2);
		r.bottom = m_center.y + (h/2);

		h = size.cy;
		CRect rs = m_EllipseRect;
		rs.top    = m_center.y - (h/2);
		rs.bottom = m_center.y + (h/2);
		dc.DrawText (strMessage, rs, DT_CENTER|DT_NOPREFIX|DT_VCENTER|DT_WORDBREAK|DT_CALCRECT);
		
		r = m_EllipseRect;
		r.top    = m_center.y - (rs.Height()/2);
		r.bottom = m_center.y + (rs.Height()/2);
		if (rs.Height() < m_EllipseRect.Height())
		{
			pDC->FrameRect (CRect (r.left-1, r.top-1, r.right+1, r.bottom+1), &br1);
			oldBrush = pDC->SelectObject (&br2);
			pDC->Rectangle (CRect (r.left-2, r.top-2, r.right+2, r.bottom+2));
			pDC->DrawText (strMessage, r, DT_CENTER|DT_NOPREFIX|DT_VCENTER|DT_WORDBREAK);
		}

		pDC->SetBkMode (iPrevMode);
		pDC->SelectObject (pOldFond);
		pDC->SetTextColor (oldColor);
		pDC->SelectObject (oldBrush);
	} 
	return;
}


void CvStatisticPieView::DrawUperBorder (CDC* pDC, double dDegStart, double dDegEnd)
{
	CPoint p [5];
	if (!m_bDrawPieBorder)
		return;
	p[0] = CPoint (m_center.x, m_center.y);
	p[1] = CPoint (fx (dDegStart), fy (dDegStart));
	p[2] = CPoint (fx (dDegEnd),   fy (dDegEnd));
	pDC->MoveTo (p[0]);  pDC->LineTo (p[1]);
	pDC->MoveTo (p[0]);  pDC->LineTo (p[2]);
}

//
//
// Log file info managements:
int CvStatisticPieView::Fx (double deg)
{
	double xc = 180.0;
	return (int)(m_center.x + (m_dRadialX +m_nXDiffRadius) * cos (deg * pi / xc));
}

int CvStatisticPieView::Fx (double deg, CPoint center)
{
	double xc = 180.0;
	return (int)(center.x + (m_dRadialX +m_nXDiffRadius) * cos (deg * pi / xc));
}

int CvStatisticPieView::Fy (double deg)
{
	double xc = 180.0;
	return (int)(m_center.y - (m_dRadialY+m_nYDiffRadius) * sin (deg * pi / xc));
}

int CvStatisticPieView::Fy (double deg, CPoint center)
{
	double xc = 180.0;
	return (int)(center.y - (m_dRadialY+m_nYDiffRadius) * sin (deg * pi / xc));
}

//
// R1 is the same as R2:
static BOOL RectEqual (CRect r1, CRect r2)
{
	return r1 == r2;
}



//
// if R1 intersects with R2 is not empty:
static BOOL RectIntersection (CRect r1, CRect r2)
{
	r1.NormalizeRect();
	r2.NormalizeRect();
	return r1.IntersectRect (r1, r2);
}

//
// Note: All rectagles are the same size, so it is imposible
// for R1 is inside R2 or R2 is Inside R1.
static BOOL IntersectRectangle (CRect r, CArray <CRect, CRect>& rectArray)
{
	int i, nCount = rectArray.GetSize();
	for (i=0; i<nCount; i++)
	{
		CRect rx = rectArray.GetAt (i);
		if (RectEqual (rx, r))
			return TRUE;
		if (RectIntersection (rx, r))
			return TRUE;
	}

	return FALSE;
}

static BOOL DrawLogfileExtraLegends (
	CDC* pDC, 
	LPCTSTR lpszLegent, 
	CPoint center, 
	CPoint start,
	CArray <CRect, CRect>& rectArray,
	double dDeg)
{
	CRect r;
	CSize txSize (40, 16);
	CFont font;
	BOOL bCreateFont = UxCreateFont (font, _T("Courier New"), 14);
	if (!bCreateFont)
		return TRUE;

	CBrush br (GetSysColor (COLOR_WINDOW)/*RGB(255, 255, 255)*/);
	COLORREF colorText    = GetSysColor (COLOR_WINDOWTEXT);
	COLORREF colorTextOld = pDC->SetTextColor (colorText);
	COLORREF oldTextBkColor = pDC->SetBkColor (GetSysColor (COLOR_WINDOW) /*RGB(255, 255, 53)*/);
	CFont* pOldFond = pDC->SelectObject (&font);
	txSize = pDC->GetTextExtent (_T("HHHHHHH"), 7);

	CString strText = lpszLegent;
	int nReturn = strText.Find (_T('\n'));
	UINT nSingleLine = DT_SINGLELINE;
	if (nReturn != -1)
	{
		txSize.cy *= 2;
		nSingleLine = 0;
	}
	//
	// Start is in the 1st quarter:
	if (start.x >= center.x && start.y <= center.y && dDeg >= 0.0 && dDeg <= 90.0)
	{
		if (dDeg <= 2.5)
			r = CRect (start.x, start.y - (txSize.cy/2), start.x + txSize.cx, start.y + (txSize.cy/2));
		else
		if (dDeg >= 87.5)
			r = CRect (start.x - (txSize.cx/2), start.y - txSize.cy, start.x + (txSize.cx/2), start.y);
		else
			r = CRect (start.x, start.y - txSize.cy, start.x + txSize.cx, start.y);
		if (IntersectRectangle (r, rectArray))
			return FALSE;
		pDC->FillRect (r, &br);
		pDC->DrawText (lpszLegent, r, DT_CENTER|nSingleLine|DT_NOPREFIX|DT_VCENTER);
		rectArray.Add (r);
	}
	else
	//
	// Start is in the 2nd quarter:
	if (start.x <= center.x && start.y <= center.y && dDeg >= 90.0 && dDeg <= 180.0)
	{
		if (dDeg >= 177.5)
			r = CRect (start.x - txSize.cx, start.y - (txSize.cy/2), start.x, start.y + (txSize.cy/2));
		else
		if (dDeg <= 92.5)
			r = CRect (start.x - (txSize.cx/2), start.y - txSize.cy, start.x + (txSize.cx/2), start.y);
		else
			r = CRect (start.x - txSize.cx, start.y - txSize.cy, start.x, start.y);
		if (IntersectRectangle (r, rectArray))
			return FALSE;
		pDC->FillRect (r, &br);
		pDC->DrawText (lpszLegent, r, DT_CENTER|nSingleLine|DT_NOPREFIX|DT_VCENTER);
		rectArray.Add (r);
	}
	else
	//
	// Start is in the 3th quarter:
	if (start.x <= center.x && start.y >= center.y && dDeg >= 180.0 && dDeg <= 270.0)
	{
		if (dDeg <= 182.5)
			r = CRect (start.x - txSize.cx, start.y - (txSize.cy/2), start.x, start.y + (txSize.cy/2));
		else
		if (dDeg >= 267.5)
			r = CRect (start.x - (txSize.cx/2), start.y, start.x + (txSize.cx/2), start.y + txSize.cy);
		else
			r = CRect (start.x - txSize.cx, start.y, start.x, start.y + txSize.cy);
		if (IntersectRectangle (r, rectArray))
			return FALSE;
		pDC->FillRect (r, &br);
		pDC->DrawText (lpszLegent, r, DT_CENTER|nSingleLine|DT_NOPREFIX|DT_VCENTER);
		rectArray.Add (r);
	}
	else
	//
	// Start is in the 4th quarter:
	if (start.x >= center.x && start.y >= center.y && dDeg >= 270.0 && dDeg <= 360.0)
	{
		if (dDeg >= 357.5)
			r = CRect (start.x, start.y - (txSize.cy/2), start.x + txSize.cx, start.y + (txSize.cy/2));
		else
		if (dDeg <= 272.5)
			r = CRect (start.x - (txSize.cx/2), start.y, start.x + (txSize.cx/2), start.y + txSize.cy);
		else
			r = CRect (start.x, start.y, start.x + txSize.cx, start.y + txSize.cy);
		if (IntersectRectangle (r, rectArray))
			return FALSE;
		pDC->FillRect (r, &br);
		pDC->DrawText (lpszLegent, r, DT_CENTER|nSingleLine|DT_NOPREFIX|DT_VCENTER);
		rectArray.Add (r);
	}

	pDC->SelectObject (pOldFond);
	pDC->SetTextColor (colorTextOld);
	pDC->SetBkColor (oldTextBkColor);
	return TRUE;
}


typedef struct tagLABELINFO
{
	TCHAR  tchszLabel[32];
	double dDegPos;

} LABELINFO;


inline static int compare (const void* e1, const void* e2)
{
	LABELINFO* o1 = (LABELINFO*)e1;
	LABELINFO* o2 = (LABELINFO*)e2;
	if (!(o1 && o2))
		return 0;
	return (o1->dDegPos > o2->dDegPos)? 1: (o1->dDegPos < o2->dDegPos)? -1: 0;
}


void CvStatisticPieView::DrawLogFileExtraInfo (CDC* pDC, CaPieInfoData* pPie)
{
	CRect r;
	CPoint p [5];
	double degCenter = 0.0;
	CArray <CRect, CRect> rectArray;
	CString strItem;

	CBrush br (RGB (255, 0, 255));
	CRect rFrame;
	GetClientRect (rFrame);
	int nbY = 2;
	if (!pPie->m_strTitle.IsEmpty())
	{
		nbY += pDC->GetTextExtent (pPie->m_strTitle, pPie->m_strTitle.GetLength()).cy;
	}

#if defined (CODING_STAGE)
	r = CRect(2, 2, rFrame.right - 2, rFrame.bottom - nbY);
	pDC->FrameRect (r, &br);

	for (int i = 0; i <360; i+= 2)
	{
		p [0] = CPoint (Fx (i), Fy(i));
		p [1] = CPoint (Fx (i+2), Fy(i+2));
		pDC->MoveTo (p[0]);
		pDC->LineTo (p[1]);
	}
#endif

	CPen   pen (PS_SOLID, 1, RGB (255, 0, 255));
	CPen   pen2(PS_SOLID, 1, RGB (0, 0, 0));
	CPen*  oldPen = NULL;
	oldPen = pDC->SelectObject (&pen);

	BOOL bOK = FALSE;
	LABELINFO lab [4];

	//
	// Calculate the position of Prev CP:
	double dDegPCP = 90.0;
	if (pPie->m_nBlockPrevCP >= 0)
	{
		dDegPCP+= (double)pPie->m_nBlockPrevCP*100.0*3.6/(double)pPie->m_nBlockTotal;
		while (dDegPCP >= 360.0)
			dDegPCP = dDegPCP - 360.0;
	}
	strItem.Format (_T("Prev CP\n(%d)"), pPie->m_nBlockPrevCP);
	lab[0].dDegPos = dDegPCP;
	lstrcpy (lab[0].tchszLabel, strItem);

	//
	// Calculate the position of BOF:
	double dDegBOF = 90.0;
	if (pPie->m_nBlockBOF >= 0)
	{
		dDegBOF+= (double)pPie->m_nBlockBOF*100.0*3.6/(double)pPie->m_nBlockTotal;
		while (dDegBOF >= 360.0)
			dDegBOF = dDegBOF - 360.0;
	}
	strItem.Format (_T("BOF\n(%d)"), pPie->m_nBlockBOF);
	lab[1].dDegPos = dDegBOF;
	lstrcpy (lab[1].tchszLabel, strItem);
	
	//
	// Calculate the position of EOF:
	double dDegEOF = 90.0;
	if (pPie->m_nBlockEOF >= 0)
	{
		dDegEOF+= (double)pPie->m_nBlockEOF*100.0*3.6/(double)pPie->m_nBlockTotal;
		while (dDegEOF >= 360.0)
			dDegEOF = dDegEOF - 360.0;
	}
	strItem.Format (_T("EOF\n(%d)"), pPie->m_nBlockEOF);
	lab[2].dDegPos = dDegEOF;
	lstrcpy (lab[2].tchszLabel, strItem);

	// 
	// Calculate the position of Next CP:
	double dDegNCP = 90.0;
	if (pPie->m_nBlockNextCP >= 0)
	{
		dDegNCP+= (double)pPie->m_nBlockNextCP*100.0*3.6/(double)pPie->m_nBlockTotal;
		while (dDegNCP >= 360.0)
			dDegNCP = dDegNCP - 360.0;
	}
	strItem.Format (_T("Next CP\n(%d)"), pPie->m_nBlockNextCP);
	lab[3].dDegPos = dDegNCP;
	lstrcpy (lab[3].tchszLabel, strItem);

	//
	// Find the logest arc of the sectors:
	qsort ((void*)lab, (size_t)4, (size_t)sizeof(LABELINFO), compare);
	int    i, k, nLongest = 0;
	double dLongest = 0.0;
	double dValue = 0.0;
	for (i=0; i<4; i++)
	{
		k = (i == 3)? 0: i+1;
		dValue = lab[k].dDegPos - lab[i].dDegPos;
		if (dValue < 0)
			dValue = 360.0 + dValue;
		if (dValue > dLongest)
		{
			nLongest = k;
			dLongest = dValue;
		}
	}

	//
	// Draw the legends:
	int nDrawCount = 0;
	BOOL bDraw [4] = {FALSE, FALSE, FALSE, FALSE};
	p[0] = CPoint (m_center.x, m_center.y);
	while (nDrawCount < 4)
	{
		double dDegPos  = lab[nLongest].dDegPos;
		double dDegNext = lab[nLongest].dDegPos;
		bOK = FALSE;
		while (bOK == FALSE)
		{
			while (dDegNext >= 360.0)
				dDegNext = dDegNext - 360.0;

			p[1] = CPoint (fx (dDegPos), fy (dDegPos));
			p[2] = CPoint (Fx (dDegNext), Fy (dDegNext));

			bOK = DrawLogfileExtraLegends (pDC, lab[nLongest].tchszLabel, m_center, p[2], rectArray, dDegNext);
			if (bOK)
			{
				pDC->SelectObject (&pen2);
				pDC->MoveTo (p[0]);
				pDC->LineTo (p[1]);
				pDC->SelectObject (&pen);
				pDC->MoveTo (p[1]);
				pDC->LineTo (p[2]);

				if (dDegPos > 180.0 && dDegPos < 360.0)
				{
					pDC->SelectObject (&pen2);
					pDC->MoveTo (p[1]);
					pDC->LineTo (p[1].x, p[1].y + m_pieHeight);
				}
			}
			else
			{
				dDegNext += 2.0;
			}
		}
		bDraw [nLongest] = TRUE;
		nLongest++;
		if (nLongest == 4)
			nLongest = 0;

		nDrawCount++;
	}

	if (oldPen)
		pDC->SelectObject (oldPen);
}

void CvStatisticPieView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	CdStatisticPieChartDoc* pDoc = (CdStatisticPieChartDoc*)GetDocument();
	CaPieInfoData*     pPie = pDoc->GetPieInfo();

	if (pPie && pPie->m_bDrawOrientation)
	{
		LogFileSetMargins();
	}
	Invalidate();
}

//
// This function is called for the specific used
// in the Log File Diagram.
void CvStatisticPieView::LogFileSetMargins()
{
	CSize cs (GetSystemMetrics (SM_CXSCREEN), GetSystemMetrics (SM_CYSCREEN));

	// default
	m_xLeftMargin   = 70;
	m_xRightMargin  = 70;
	m_nYDiffRadius =  (m_size.cy * 28 ) /200 ;
	m_yTopMargin    = 26+m_nYDiffRadius;
	m_yBottomMargin = 40+m_nYDiffRadius;

	//
	// 1280 x 1024
	if (cs.cx == 1280 && cs.cy == 1024)
	{
		m_xLeftMargin   = 70;
		m_xRightMargin  = 70;
		m_nYDiffRadius =  (m_size.cy * 28 ) /200 ;
		m_yTopMargin    = 26+m_nYDiffRadius;
		m_yBottomMargin = 40+m_nYDiffRadius;
	}
	else
	//
	// 1152 x 864
	if (cs.cx == 1152 && cs.cy == 864)
	{
		m_xLeftMargin   = 70;
		m_xRightMargin  = 70;
		m_nYDiffRadius =  (m_size.cy * 28 ) /200 ;
		m_yTopMargin    = 26+m_nYDiffRadius;
		m_yBottomMargin = 40+m_nYDiffRadius;
	}
	else
	//
	// 1024 x 768
	if (cs.cx == 1024 && cs.cy == 768)
	{
		m_xLeftMargin   = 70;
		m_xRightMargin  = 70;
		m_nYDiffRadius =  (m_size.cy * 28 ) /200 ;
		m_yTopMargin    = 26+m_nYDiffRadius;
		m_yBottomMargin = 40+m_nYDiffRadius;
	}
	else
	//
	// 800 x 600
	if (cs.cx == 800 && cs.cy == 600)
	{
		m_xLeftMargin   = 68;
		m_xRightMargin  = 68;
		m_yTopMargin    = 24+m_nYDiffRadius;
		m_yBottomMargin = 40+m_nYDiffRadius;
	}
	else
	//
	// 640 x 480
	if (cs.cx == 640 && cs.cy == 480)
	{
		m_xLeftMargin   = 68;
		m_xRightMargin  = 68;
		m_nYDiffRadius =  (m_size.cy *30 ) /200 ;
		m_yTopMargin    = 26+m_nYDiffRadius;
		m_yBottomMargin = 40+m_nYDiffRadius;
	}
	if (m_nYDiffRadius == 0)
		m_nYDiffRadius = 1;
}

