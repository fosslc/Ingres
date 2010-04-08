// LogIndic.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "logindic.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLogFileIndicator

CLogFileIndicator::CLogFileIndicator()
{
  m_dblBofPercent = 0;
  m_dblEofPercent = 0;
  m_dblRsvPercent = 0;
  m_bNotAvailable = FALSE;

}

CLogFileIndicator::~CLogFileIndicator()
{
}


BEGIN_MESSAGE_MAP(CLogFileIndicator, CStatic)
	//{{AFX_MSG_MAP(CLogFileIndicator)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CLogFileIndicator::SetBofEofPercent(double dblBofPercent, double dblEofPercent, double dblReservedPercent, BOOL bNotAvailable)
{
  m_dblBofPercent = dblBofPercent;
  m_dblEofPercent = dblEofPercent;
  m_dblRsvPercent = dblReservedPercent;
  m_bNotAvailable = bNotAvailable;
  if (IsWindow(m_hWnd))
    InvalidateRect(NULL, TRUE); // True erases
}

/////////////////////////////////////////////////////////////////////////////
// CLogFileIndicator message handlers

void CLogFileIndicator::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

  CRect cliRect;
  GetClientRect(&cliRect);

  if (m_bNotAvailable) {
    CString csNA = VDBA_MfcResourceString (IDS_NOT_AVAILABLE);//_T("n/a")
    CFont* pDialogFont = GetParent()->GetFont();
    ASSERT (pDialogFont);
    CFont* pOldFont = (CFont*)dc.SelectObject(pDialogFont);
    int oldBkMode = dc.SetBkMode(TRANSPARENT);
	  dc.DrawText (csNA,
                 cliRect,
                 DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    dc.SetBkMode(oldBkMode);
    dc.SelectObject(pOldFont);
  }
  else {
    int width = cliRect.Width();
    int height = cliRect.Height();
    COLORREF clr = RGB(0, 0, 192);

    // Remainder in green
    DrawPortion(dc, width, height, 0, 100, RGB(0, 192, 0));

    // "In Use" portion
    DrawPortion(dc, width, height, m_dblBofPercent, m_dblEofPercent, clr);

    // "Reserved" portion in yellow
    clr = RGB(255, 255, 0);
    if (m_dblRsvPercent) {

      double dblRsvBofPercent = (m_dblBofPercent - m_dblRsvPercent);
      if (dblRsvBofPercent >= 100.0)
        dblRsvBofPercent -= 100.0;
      if (dblRsvBofPercent < 0)
        dblRsvBofPercent += 100.0;
      ASSERT (dblRsvBofPercent >= 0.0);
      ASSERT (dblRsvBofPercent < 100.0);

      double dblRsvEofPercent = m_dblBofPercent;
      DrawPortion(dc, width, height, dblRsvBofPercent, dblRsvEofPercent, clr);
    }
  }

	// Do not call CStatic::OnPaint() for painting messages
}


BOOL CLogFileIndicator::OnEraseBkgnd(CDC* pDC) 
{
  COLORREF colorBk = ::GetSysColor (COLOR_BTNFACE);   // COLOR_APPWORKSPACE
  CBrush bkBrush;
  bkBrush.CreateSolidBrush(colorBk);
  CRect rect;
  GetClientRect(&rect);
  pDC->FillRect(&rect, &bkBrush);

  return TRUE;    // I have processed!
}


void CLogFileIndicator::DrawPortion(CPaintDC &dc, int width, int height, double dblBegPercent, double dblEndPercent, COLORREF clr)
{
  if (dblBegPercent != dblEndPercent) {
    if (dblBegPercent < dblEndPercent) {
      // not wrapped: 1 portion to draw
      int lPos = (int) ( dblBegPercent * width / 100 );
      int rPos = (int) ( dblEndPercent * width / 100 );
      ASSERT (rPos >= lPos);
      if (rPos == lPos)
        rPos++;
      dc.FillSolidRect(lPos, 0, rPos-lPos, height, clr);
    }
    else {
      // Wrapped : 2 portions to draw
      int lPos = (int) ( dblEndPercent * width / 100 );
      int rPos = (int) ( dblBegPercent * width / 100 );
      if (lPos == 0 && rPos == width)
        lPos++;
      dc.FillSolidRect(0, 0, lPos, height, clr);
      dc.FillSolidRect(rPos, 0, width-rPos, height, clr);
    }
  }
}
