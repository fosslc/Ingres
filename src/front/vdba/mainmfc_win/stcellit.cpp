/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : stcellit.cpp : implementation file
**    Project  : OpenIngres Visual DBA
**    Author   : Emmanuel Blattes
**
** History:
**
** 15-May-1998 (Emmanuel Blattes)
**    Created
** 04-Jul-2002 (uk$so01)
**    Fix BUG #108183, Graphic in Pages Tab of DOM/Table keeps flickering.
** 19-Mar-2009 (drivi01)
**    In effort to port to Visual Studio 2008,
**    cleanup warnings by fixing precedence.
*/

#include "stdafx.h"
#include "stcellit.h"
#include "cellctrl.h"
#include "resmfc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuStaticCellItem

CuStaticCellItem::CuStaticCellItem()
{
  m_ItemType = 0;
  m_pPalette = NULL;
}

CuStaticCellItem::~CuStaticCellItem()
{
}


BEGIN_MESSAGE_MAP(CuStaticCellItem, CStatic)
	//{{AFX_MSG_MAP(CuStaticCellItem)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuStaticCellItem message handlers

void CuStaticCellItem::OnPaint() 
{
  //TRACE0 ("CuStaticCellItem::OnPaint() ...\n");
  CPaintDC dc(this); // device context for painting

  CRect rcItem;
  GetClientRect (rcItem);

  // Pick type of detailed item to be drawn
  ASSERT (m_ItemType);
  BYTE by = m_ItemType;

  // Determine whether we need a palette, and use it if found so
  BOOL bUsesPalette = FALSE;
  CPalette* pOldPalette = NULL;
  CPalette cellItemPalette;
  CClientDC mainDc(AfxGetMainWnd());
  if ((mainDc.GetDeviceCaps(RASTERCAPS) & RC_PALETTE) != 0) {
    int palSize  = mainDc.GetDeviceCaps(SIZEPALETTE);
    if (palSize > 0) {
      ASSERT (palSize >= 256);
	  if (m_pPalette)
	  {
        pOldPalette = dc.SelectPalette (m_pPalette, TRUE);
        dc.RealizePalette();


        bUsesPalette = TRUE;
	  }
	  else
	  {
		BOOL bSuccess = CuCellCtrl::CreateDetailedPalette(&cellItemPalette);
		ASSERT (bSuccess);
		if (bSuccess) {
			pOldPalette = dc.SelectPalette (&cellItemPalette, FALSE);
			dc.RealizePalette();
			bUsesPalette = TRUE;
		}
	  }
    }
  }

  // Coloured rectangle according to byte
  COLORREF pageColor;
  if (bUsesPalette)
    pageColor = PALETTEINDEX(CuCellCtrl::GetDetailPaletteIndex(by));
  else
    pageColor = CuCellCtrl::GetPageColor(by);
  CBrush brush;
  brush.CreateSolidBrush(pageColor);
  dc.FillRect (rcItem, &brush);
  // Finished with palette
  if (bUsesPalette) {
    dc.SelectPalette(pOldPalette, FALSE);
    //dc.RealizePalette();
    if (!m_pPalette)
       cellItemPalette.DeleteObject();
  }
  // Image according to page type, drawn without the palette
  CImageList ImageList;
  ImageList.Create(IDB_PAGETYPE, PAGE_IMAGE_SIZE, 1, RGB (255, 255, 255));
	UINT uiFlags = ILD_TRANSPARENT;
	COLORREF clrImage = pageColor;  // m_colorTextBk;

  if (rcItem.left < rcItem.right-1) {
    // Use height of bitmap inside image list.
    // images must be centered in the imagelist bounding rectangle.
    IMAGEINFO imgInfo;
    int height = PAGE_IMAGE_SIZE;
    if (ImageList.GetImageInfo(0, &imgInfo))
	    height = imgInfo.rcImage.bottom - imgInfo.rcImage.top;
    int imageIndex = CuCellCtrl::GetImageIndex(by);
    ASSERT (imageIndex != -1);
    int drawWidth = PAGE_IMAGE_SIZE;
    int offsetH = (rcItem.Width() - PAGE_IMAGE_SIZE) / 2;
    if (offsetH < 0) {
      drawWidth = rcItem.Width();
      offsetH = 0;    // Mask if truncate image
    }
    ImageList_DrawEx (ImageList.m_hImageList, imageIndex, dc.m_hDC,
                      rcItem.left + offsetH, rcItem.top,
                      drawWidth, PAGE_IMAGE_SIZE,
                      CLR_NONE/*m_colorTextBk*/, CLR_NONE/*clrImage*/, uiFlags);
  }
}
