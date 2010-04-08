// StMetalB.cpp : implementation file
//
/* History:
**	19-Mar-2009 (drivi01)
**	   In effort to port to Visual Studio 2008,
**	   cleanup warnings by fixing precedence.
**
*/

#include "stdafx.h"
#include "stmetalb.h"
#include "cellctrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuStaticMetalBlue

CuStaticMetalBlue::CuStaticMetalBlue()
{
}

CuStaticMetalBlue::~CuStaticMetalBlue()
{
}


BEGIN_MESSAGE_MAP(CuStaticMetalBlue, CStatic)
	//{{AFX_MSG_MAP(CuStaticMetalBlue)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuStaticMetalBlue message handlers

void CuStaticMetalBlue::OnPaint() 
{
  CPaintDC dc(this); // device context for painting

  //TRACE0 ("CuStaticMetalBlue::OnPaint() ...\n");

  CRect rcInit;
  GetClientRect (rcInit);

  BOOL bUsesPalette = FALSE;
  CPalette* pOldPalette = NULL;
  CPalette metalBluePalette;

  CClientDC mainDc(AfxGetMainWnd());
  if ((mainDc.GetDeviceCaps(RASTERCAPS) & RC_PALETTE) != 0) {
    int palSize  = mainDc.GetDeviceCaps(SIZEPALETTE);
    if (palSize > 0) {
      ASSERT (palSize >= 256);
      BOOL bSuccess = CuCellCtrl::CreateMetalBluePalette(&metalBluePalette);
      ASSERT (bSuccess);
      if (bSuccess) {
        bUsesPalette = TRUE;
        pOldPalette = dc.SelectPalette (&metalBluePalette, FALSE);
        dc.RealizePalette();
      }
    }
  }

  CBrush brush;
  if (bUsesPalette)
    brush.CreateSolidBrush(PALETTEINDEX(NBDETAILCOLORS + (NBMETALBLUECOLORS - 1)));   // See palette organization in CellCtrl.cpp
  else
    brush.CreateSolidBrush(CuCellCtrl::GetHundredPctColor());

  //CBrush* pOldBrush = dc.SelectObject (&brush);
  dc.FillRect (rcInit, &brush);
  //dc.SelectObject (pOldBrush);

  CFont* pDialogFont = GetParent()->GetFont();
  ASSERT (pDialogFont);
  CFont* pOldFont = (CFont*)dc.SelectObject(pDialogFont);
  int oldBkMode = dc.SetBkMode(TRANSPARENT);
	dc.DrawText ("100",
               rcInit,
               DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
  dc.SetBkMode(oldBkMode);
  dc.SelectObject(pOldFont);

  if (bUsesPalette) {
    ASSERT (pOldPalette);
    dc.SelectPalette(pOldPalette, FALSE);
    //dc.RealizePalette();
    metalBluePalette.DeleteObject();
  }
}
