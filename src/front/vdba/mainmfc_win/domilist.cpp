// domilist.cpp : implementation file
// NOTE: Bitmap assumed to have RGB(255,0,255) pixels to be drawn as transparent,
// can be enhanced by adding a new member with RGB to be drawn as transparent

#include "stdafx.h"
#include "domilist.h"

extern "C" {
  #include "resource.h"
};

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define IMAGE_WIDTH 16
#define IMAGE_HEIGHT 16

/////////////////////////////////////////////////////////////////////////////
// CuDomImageList
void CuDomImageList::AddIcon(WORD idIcon)
{
  ASSERT (idIcon);

  HICON hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(idIcon));
  ASSERT (hIcon);

  int iconIndex = -1;
  CDC * pscreenDc;
  CDC memDc;
  CBitmap bmp;
  CBitmap *pOldBmp;

  pscreenDc = AfxGetMainWnd()->GetDC();
  if (pscreenDc) {
    if (memDc.CreateCompatibleDC(pscreenDc)) {
      if (bmp.CreateCompatibleBitmap(pscreenDc, IMAGE_WIDTH, IMAGE_HEIGHT)) {
        pOldBmp = memDc.SelectObject(&bmp);
        if (pOldBmp) {
          COLORREF backgroundColor = RGB(255, 0, 255);
          CBrush brushForBackground(backgroundColor);
          CRect cr(0, 0, IMAGE_WIDTH, IMAGE_HEIGHT);
          memDc.FillRect(&cr, &brushForBackground);

          //memDc.DrawIcon(0, 0, hIcon);
          memDc.DrawIcon(0, 1, hIcon);
          memDc.SelectObject(pOldBmp);

          iconIndex = Add(&bmp, backgroundColor);
          ASSERT (iconIndex >= 0);

        }
        bmp.DeleteObject();   // defined in CGdiObject class
      }
      memDc.DeleteDC();
    }
    AfxGetMainWnd()->ReleaseDC(pscreenDc);
  }
}

