/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**
**  Source   : CellCtrl.cpp, implementation file
**
**  Project  : Ingres/Visual DBA.
**
**  Author   : Emmanuel Blattes - based on UK Sotheavut's work for
**		ownerdraw ListCtrl
**
**  Purpose  : Cellular control Originally designed to display pages usage
**		of an OpenIngres table, as a matrix with nice
**		colors/images/texts in it. Absolutely NO DATA/TEXT/ETC
**		stored in items, all is managed by OnDraw using
**		exclusively the byte array plus
**
**  History (after 01-01-2000):
**	20-jan-2000 (noifr01)
**	    (bug 100088) a typo error in 2 assignments in the
**	    DrawDeflatedItem() method (== used instead of =) resulted in
**	    the intensity and overflow information to keep a value of 0
**	    in the case where the "pages per cell" minimum value was chosen
**	    by the user, and was bigger than 1
**	08-feb-2000 (somas01)
**	    Added comment braces around comment following "#endif".
** 23-May-2000 (uk$so01)
**    bug 99242 (DBCS) modify the functions: MakeShortString()
** 04-Jul-2002 (uk$so01)
**    Fix BUG #108183, Graphic in Pages Tab of DOM/Table keeps flickering.
**  28-apr-2009 (smeke01) b121545
**   Enable display of ISAM index levels 1 to 9 and '?' for unrecognised  
**   pagetypes. 
** 19-Jun-2009 (drivi01)
**    In effort to port to Visual Studio 2008, clean up warnings by
**    fixing precedence in the statements.
*/

#include "stdafx.h"
#include "cellctrl.h"
#include "resmfc.h" // IDB_PAGETYPE

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// SPECIAL Emb : Not to slow paints in debug mode, we can get rid of traces
// by defining NOTRACEFORPAINT
#define NOTRACEFORPAINT
#ifdef NOTRACEFORPAINT
#define MYTRACE0(x)
#define MYTRACE1(x, y)
#else
#define MYTRACE0(x)     (TRACE0(x))
#define MYTRACE1(x, y)  (TRACE1(x, y))
#endif

/////////////////////////////////////////////////////////////////////////////
// Customized tooltip control

BOOL CuToolTipCtrl::AddWindowTool (CWnd* pWnd,  LPCTSTR pszText)
{
  TOOLINFO ti;
  ti.cbSize = sizeof (TOOLINFO);
  ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
  ti.hwnd = pWnd->GetParent()->GetSafeHwnd();
  ti.uId = (UINT) pWnd->GetSafeHwnd();
  ti.hinst = AfxGetInstanceHandle();
  ti.lpszText = (LPTSTR) pszText;

  return (BOOL) SendMessage(TTM_ADDTOOL, 0, (LPARAM)&ti);
}

BOOL CuToolTipCtrl::AddRectTool(CWnd*pWnd, LPCTSTR pszText, LPCRECT lpRect, UINT nIDTool)
{
  TOOLINFO ti;
  ti.cbSize = sizeof (TOOLINFO);
  ti.uFlags = TTF_SUBCLASS;
  ti.hwnd = pWnd->GetSafeHwnd();
  ti.uId = nIDTool;
  ti.hinst = AfxGetInstanceHandle();
  ti.lpszText = (LPTSTR) pszText;
  ::CopyRect (&ti.rect, lpRect);

  return (BOOL) SendMessage(TTM_ADDTOOL, 0, (LPARAM)&ti);
  
}

/////////////////////////////////////////////////////////////////////////////
// color families in detailed mode



/////////////////////////////////////////////////////////////////////////////
// New methods

CString CuCellCtrl::GetShortTooltipText()
{
  // Get cursor position, in screen coordinates
  CPoint pt;
  BOOL bOk = GetCursorPos(&pt);
  ASSERT (bOk);

  // Convert into control client coordinates
  ScreenToClient(&pt);

  // Get line
  CString csPt;
  int hitItem = HitTest(pt);
  if (hitItem == -1) {
    return _T("");  // No cell
    //csPt.Format("No Item x=%d, y=%d", pt.x, pt.y);
    //return csPt;
  }

  // Find column
  CRect hitItemRect;
	GetItemRect (hitItem, hitItemRect, LVIR_BOUNDS);
  CRect rCell;
  int iColumnNumber;
	if (!GetCellRect (hitItemRect, pt, rCell, iColumnNumber))
		return _T("");  // out of bounds

  // here, we are on line "hitItem", column "iColumnNumber"
  //csPt.Format("Hit on cell line %d col %d", hitItem, iColumnNumber);
  if (iColumnNumber == 0)
    return _T("");    // No comment for caption column

  // Calculate cell(s) number(s), for display
  int lineNumber = hitItem;
  int indexInByteArray = lineNumber * m_nbCellsByLine * m_deflateFactor
                        + m_deflateFactor * (iColumnNumber - 1);
  int startPageNumber = indexInByteArray * m_itemsPerByte;
  if (startPageNumber >= m_nbElements * m_itemsPerByte)
    return _T("");    // No comment if goes too far

  if (m_itemsPerByte == 1 && m_deflateFactor == 1) {
    csPt.Format("Detailed Page %d", indexInByteArray);
    return csPt;
  }

  int stopPageNumber = startPageNumber + m_deflateFactor * m_itemsPerByte - 1;
  if (stopPageNumber >= m_nbElements * m_itemsPerByte)
    stopPageNumber = m_nbElements * m_itemsPerByte - 1;
  csPt.Format("Pages %d to %d", startPageNumber, stopPageNumber);
  return csPt;
}

BOOL CuCellCtrl::CreateMultiUsePalette(CPalette* pMultiUsePalette)
{
  ASSERT (pMultiUsePalette);

  struct {
    LOGPALETTE lp;
    PALETTEENTRY ape[NBDETAILCOLORS + NBMETALBLUECOLORS - 1];
  } detPal;

  LOGPALETTE* pLP = (LOGPALETTE*) &detPal;
  pLP->palVersion = 0x300;
  pLP->palNumEntries = NBDETAILCOLORS + NBMETALBLUECOLORS;

  // Indexes 0 to NBDETAILCOLORS-1 included: enumerate pagecolors
  BYTE atypes[NBDETAILCOLORS] = {
    (BYTE) PAGE_HEADER       ,
    (BYTE) PAGE_MAP          ,
    (BYTE) PAGE_FREE         ,
    (BYTE) PAGE_ROOT         ,
    (BYTE) PAGE_INDEX        ,
	(BYTE) PAGE_INDEX_1      ,
	(BYTE) PAGE_INDEX_2      ,
	(BYTE) PAGE_INDEX_3      ,
	(BYTE) PAGE_INDEX_4      ,
	(BYTE) PAGE_INDEX_5      ,
	(BYTE) PAGE_INDEX_6      ,
	(BYTE) PAGE_INDEX_7      ,
	(BYTE) PAGE_INDEX_8      ,
	(BYTE) PAGE_INDEX_9      ,
	(BYTE) PAGE_SPRIG        ,
    (BYTE) PAGE_LEAF         ,
    (BYTE) PAGE_OVERFLOW_LEAF,
    (BYTE) PAGE_OVERFLOW_DATA,
    (BYTE) PAGE_ASSOCIATED   ,
    (BYTE) PAGE_DATA         ,
    (BYTE) PAGE_UNUSED       ,
    // hash specific section (ingres >= 2.5)
    (BYTE) PAGE_EMPTY_DATA_NO_OVF,
    (BYTE) PAGE_EMPTY_OVERFLOW,
    (BYTE) PAGE_DATA_WITH_OVF,
    (BYTE) PAGE_EMPTY_DATA_WITH_OVF,
	(BYTE) PAGE_ERROR_UNKNOWN
  };
  int cpt;
  for (cpt=0; cpt < NBDETAILCOLORS; cpt++) {

    // check compliance between indexes and byte values
    ASSERT (GetDetailPaletteIndex(atypes[cpt]) == cpt);

    COLORREF color = GetPageColor(atypes[cpt]);
    pLP->palPalEntry[cpt].peRed   = GetRValue(color);
    pLP->palPalEntry[cpt].peGreen = GetGValue(color);
    pLP->palPalEntry[cpt].peBlue  = GetBValue(color);
    pLP->palPalEntry[cpt].peFlags = 0;
  }

  // Indexes NBDETAILCOLORS to NBDETAILCOLORS + NBMETALBLUECOLORS - 1 included: from dark blue to white

  for (cpt=0; cpt < NBMETALBLUECOLORS; cpt++) {
    COLORREF color = GetPctUsedColor(cpt);  // cpt used as intensity
    pLP->palPalEntry[cpt + NBDETAILCOLORS].peRed   = GetRValue(color);
    pLP->palPalEntry[cpt + NBDETAILCOLORS].peGreen = GetGValue(color);
    pLP->palPalEntry[cpt + NBDETAILCOLORS].peBlue  = GetBValue(color);
    pLP->palPalEntry[cpt + NBDETAILCOLORS].peFlags = 0;
  }
  return pMultiUsePalette->CreatePalette(pLP);
}

BOOL CuCellCtrl::CreateMetalBluePalette(CPalette* pMetalBluePalette)
{
  return CreateMultiUsePalette(pMetalBluePalette);
}

BOOL CuCellCtrl::CreateDetailedPalette(CPalette* pDetailedPalette)
{
  return CreateMultiUsePalette(pDetailedPalette);
}

int CuCellCtrl::GetDetailPaletteIndex(BYTE by)
{
  switch (by) {
    case (BYTE) PAGE_HEADER       : return 0;
    case (BYTE) PAGE_MAP          : return 1;
    case (BYTE) PAGE_FREE         : return 2;
    case (BYTE) PAGE_ROOT         : return 3;
    case (BYTE) PAGE_INDEX        : return 4;
    case (BYTE) PAGE_INDEX_1      : return 5; 
    case (BYTE) PAGE_INDEX_2      : return 6; 
    case (BYTE) PAGE_INDEX_3      : return 7; 
    case (BYTE) PAGE_INDEX_4      : return 8; 
    case (BYTE) PAGE_INDEX_5      : return 9; 
    case (BYTE) PAGE_INDEX_6      : return 10; 
    case (BYTE) PAGE_INDEX_7      : return 11; 
    case (BYTE) PAGE_INDEX_8      : return 12; 
    case (BYTE) PAGE_INDEX_9      : return 13; 
    case (BYTE) PAGE_SPRIG        : return 14;
    case (BYTE) PAGE_LEAF         : return 15;
    case (BYTE) PAGE_OVERFLOW_LEAF: return 16;
    case (BYTE) PAGE_OVERFLOW_DATA: return 17;
    case (BYTE) PAGE_ASSOCIATED   : return 18;
    case (BYTE) PAGE_DATA         : return 19;
    case (BYTE) PAGE_UNUSED       : return 20;
    // hash specific section (ingres >= 2.5)
    case (BYTE) PAGE_EMPTY_DATA_NO_OVF    : return 21;
    case (BYTE) PAGE_EMPTY_OVERFLOW       : return 22;
    case (BYTE) PAGE_DATA_WITH_OVF        : return 23;
    case (BYTE) PAGE_EMPTY_DATA_WITH_OVF  : return 24;
    case (BYTE) PAGE_ERROR_UNKNOWN		  : return 25;
    default:
      ASSERT (FALSE);
      return 0;
  }
}

COLORREF CuCellCtrl::GetPctUsedColor(int intensity)
{
  ASSERT (intensity >= 0);
  ASSERT (intensity < NBMETALBLUECOLORS);

  /* values captured with emb's hands using color palette of paint application,
     chroma = 138, saturation = 136,
     luminance variation from 102 to 240 by steps of 2/3 to have exactly NBMETALBLUECOLORS (64) values
     including borders
  */
  static COLORREF aMetalBlue[NBMETALBLUECOLORS] = {
                            RGB(47  , 115 , 170) ,   // luminance 102
                            RGB(48  , 117 , 173) ,   // luminance 104
                            RGB(49  , 119 , 176) ,   // luminance 106
                            RGB(50  , 121 , 180) ,   // luminance 108
                            RGB(51  , 123 , 183) ,   // luminance 110
                            RGB(52  , 126 , 188) ,   // luminance 113
                            RGB(53  , 130 , 191) ,   // luminance 115
                            RGB(54  , 132 , 194) ,   // luminance 117
                            RGB(55  , 134 , 198) ,   // luminance 119
                            RGB(57  , 136 , 200) ,   // luminance 121
                            RGB(62  , 139 , 202) ,   // luminance 124
                            RGB(65  , 141 , 203) ,   // luminance 126
                            RGB(69  , 142 , 203) ,   // luminance 128
                            RGB(72  , 145 , 204) ,   // luminance 130
                            RGB(75  , 147 , 205) ,   // luminance 132
                            RGB(81  , 150 , 206) ,   // luminance 135
                            RGB(84  , 152 , 207) ,   // luminance 137
                            RGB(87  , 154 , 208) ,   // luminance 139
                            RGB(90  , 156 , 209) ,   // luminance 141
                            RGB(94  , 158 , 210) ,   // luminance 143
                            RGB(99  , 160 , 211) ,   // luminance 146
                            RGB(102 , 163 , 213) ,   // luminance 148
                            RGB(105 , 165 , 214) ,   // luminance 150
                            RGB(108 , 167 , 215) ,   // luminance 152
                            RGB(112 , 169 , 216) ,   // luminance 154
                            RGB(117 , 172 , 217) ,   // luminance 157
                            RGB(120 , 174 , 218) ,   // luminance 159
                            RGB(123 , 176 , 219) ,   // luminance 161
                            RGB(126 , 177 , 220) ,   // luminance 163
                            RGB(131 , 180 , 220) ,   // luminance 165
                            RGB(135 , 183 , 222) ,   // luminance 168
                            RGB(138 , 185 , 223) ,   // luminance 170
                            RGB(141 , 187 , 224) ,   // luminance 172
                            RGB(146 , 189 , 224) ,   // luminance 174
                            RGB(149 , 191 , 225) ,   // luminance 176
                            RGB(153 , 194 , 227) ,   // luminance 179
                            RGB(157 , 196 , 227) ,   // luminance 181
                            RGB(160 , 198 , 228) ,   // luminance 183
                            RGB(164 , 200 , 230) ,   // luminance 185
                            RGB(167 , 202 , 231) ,   // luminance 187
                            RGB(172 , 205 , 232) ,   // luminance 190
                            RGB(175 , 207 , 233) ,   // luminance 192
                            RGB(179 , 209 , 234) ,   // luminance 194
                            RGB(182 , 211 , 235) ,   // luminance 196
                            RGB(185 , 213 , 236) ,   // luminance 198
                            RGB(190 , 216 , 237) ,   // luminance 201
                            RGB(193 , 218 , 238) ,   // luminance 203
                            RGB(197 , 220 , 239) ,   // luminance 205
                            RGB(200 , 222 , 240) ,   // luminance 207
                            RGB(203 , 224 , 241) ,   // luminance 209
                            RGB(208 , 227 , 242) ,   // luminance 212
                            RGB(211 , 230 , 243) ,   // luminance 214
                            RGB(215 , 231 , 244) ,   // luminance 216
                            RGB(219 , 233 , 244) ,   // luminance 218
                            RGB(222 , 235 , 245) ,   // luminance 220
                            RGB(226 , 238 , 248) ,   // luminance 223
                            RGB(231 , 240 , 248) ,   // luminance 225
                            RGB(234 , 242 , 249) ,   // luminance 227
                            RGB(237 , 244 , 250) ,   // luminance 229
                            RGB(240 , 247 , 251) ,   // luminance 231
                            RGB(245 , 249 , 252) ,   // luminance 234
                            RGB(249 , 251 , 253) ,   // luminance 236
                            RGB(252 , 253 , 254) ,   // luminance 238
                            RGB(255 , 255 , 255) ,   // luminance 240
  };
  int index = NBMETALBLUECOLORS-1-intensity;
  ASSERT (index >= 0);
  return aMetalBlue[index];
}

COLORREF CuCellCtrl::GetOverflowColor(int overflow)
{
  ASSERT (overflow > 0);
  ASSERT (overflow < 4);

  COLORREF overflowColor = RGB(255, 0, 0);    // Bright red
  return overflowColor;
}

COLORREF CuCellCtrl::GetPageColor(BYTE by)
{
  switch (by) {
    case (BYTE) PAGE_HEADER       : return RGB(192, 192, 192);    // light gray
    case (BYTE) PAGE_MAP          : return RGB(192, 192, 192);    // light gray
    case (BYTE) PAGE_FREE         : return GetPctUsedColor(0);    // same as null intensity
    case (BYTE) PAGE_ROOT         : return RGB(254, 192, 65);     // pumpkin brown-orange
    case (BYTE) PAGE_INDEX        :
    case (BYTE) PAGE_INDEX_1      :
    case (BYTE) PAGE_INDEX_2      :
    case (BYTE) PAGE_INDEX_3      :
    case (BYTE) PAGE_INDEX_4      :
    case (BYTE) PAGE_INDEX_5      :
    case (BYTE) PAGE_INDEX_6      :
    case (BYTE) PAGE_INDEX_7      :
    case (BYTE) PAGE_INDEX_8      :
    case (BYTE) PAGE_INDEX_9      : return RGB(240, 240, 0);      // Yellow
    case (BYTE) PAGE_SPRIG        : return RGB(123, 188, 3);      // Tender green
    case (BYTE) PAGE_LEAF         : return RGB(0, 255, 0);        // Leaf green
    case (BYTE) PAGE_OVERFLOW_LEAF: return RGB(155, 0, 128);      // tired blood red
    case (BYTE) PAGE_OVERFLOW_DATA: return RGB(255, 0, 0);        // bloody red
    case (BYTE) PAGE_ASSOCIATED   : return GetPctUsedColor(45);   // same as medium plus intensity (lum. 135)
    case (BYTE) PAGE_DATA         : return GetPctUsedColor(30);   // same as medium intensity (lum. 167)
    case (BYTE) PAGE_UNUSED       : return GetPctUsedColor(0);    // same as null intensity
    // hash specific section (ingres >= 2.5)
    case (BYTE) PAGE_EMPTY_DATA_NO_OVF    : return GetPctUsedColor(15);   // same as medium light intensity (lum. 207)
    case (BYTE) PAGE_EMPTY_OVERFLOW       : return RGB (255, 0, 255);     // pink , same as vut's
    case (BYTE) PAGE_DATA_WITH_OVF        : return GetPctUsedColor(30);   // same as medium intensity (lum. 167)
    case (BYTE) PAGE_EMPTY_DATA_WITH_OVF  : return GetPctUsedColor(15);   // same as medium light intensity (lum. 207)
    case (BYTE) PAGE_ERROR_UNKNOWN        : return RGB(255, 255, 255);    // white

    default:
      ASSERT (FALSE);
      return RGB(0, 0, 0);        // black as error
  }
}

int CuCellCtrl::GetImageIndex(BYTE by)
{
  switch (by) {
    case (BYTE) PAGE_HEADER       : return 0;
    case (BYTE) PAGE_MAP          : return 1;
    case (BYTE) PAGE_FREE         : return 2;
    case (BYTE) PAGE_ROOT         : return 3;
    case (BYTE) PAGE_INDEX        : return 4;
    case (BYTE) PAGE_INDEX_1      : return 5;
    case (BYTE) PAGE_INDEX_2      : return 6;
    case (BYTE) PAGE_INDEX_3      : return 7;
    case (BYTE) PAGE_INDEX_4      : return 8;
    case (BYTE) PAGE_INDEX_5      : return 9;
    case (BYTE) PAGE_INDEX_6      : return 10;
    case (BYTE) PAGE_INDEX_7      : return 11;
    case (BYTE) PAGE_INDEX_8      : return 12;
    case (BYTE) PAGE_INDEX_9      : return 13;
    case (BYTE) PAGE_SPRIG        : return 14;
    case (BYTE) PAGE_LEAF         : return 15;
    case (BYTE) PAGE_OVERFLOW_LEAF: return 16;
    case (BYTE) PAGE_OVERFLOW_DATA: return 17;
    case (BYTE) PAGE_ASSOCIATED   : return 18;
    case (BYTE) PAGE_DATA         : return 19;
    case (BYTE) PAGE_UNUSED       : return 20;
    // hash specific section (ingres >= 2.5)
    case (BYTE) PAGE_EMPTY_DATA_NO_OVF    : return 21;
    case (BYTE) PAGE_EMPTY_OVERFLOW       : return 22;
    case (BYTE) PAGE_DATA_WITH_OVF        : return 23;
    case (BYTE) PAGE_EMPTY_DATA_WITH_OVF  : return 24;
    case (BYTE) PAGE_ERROR_UNKNOWN        : return 25;
    default:
      ASSERT (FALSE);
      return -1;
  }
}

void CuCellCtrl::InitializeCells(int nbCellsByLine)
{
  // initialize tooltip
  m_ctlTT.Create(this);
  // Masqued tooltip balloon: m_ctlTT.AddWindowTool(this, LPSTR_TEXTCALLBACK );

  // Initialize image list if necessary
  if (!m_pImageList) {
  	m_ImageList.Create(IDB_PAGETYPE, PAGE_IMAGE_SIZE, 1, RGB (255, 255, 255));
	  SetImageList (&m_ImageList, LVSIL_SMALL);
    m_pImageList = &m_ImageList;
  }

  ASSERT (nbCellsByLine > 0);
  m_nbCellsByLine = nbCellsByLine;

  // default widths -
  // header width will be recalculated according to maximum displayed number
  int headerWidth = 70;
  int cellWidth   = 12;    // or PAGE_IMAGE_SIZE;  - originally 20

  // Create columns, with one extra for title display
  LV_COLUMN lvcolumn;
  memset (&lvcolumn, 0, sizeof (LV_COLUMN));
  lvcolumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
  for (int cpt = 0; cpt <= m_nbCellsByLine; cpt++) {
    if (cpt == 0)
      lvcolumn.fmt = LVCFMT_RIGHT;
    else
      lvcolumn.fmt = LVCFMT_CENTER;
    if (cpt == 0)
      lvcolumn.pszText = _T("Page N°");
    else
      lvcolumn.pszText = _T("Col");
    lvcolumn.iSubItem = cpt;
    if (cpt == 0)
      lvcolumn.cx = headerWidth;
    else
      lvcolumn.cx = cellWidth;
    InsertColumn (cpt, &lvcolumn);
  }
}

void CuCellCtrl::LiberateDataForDisplay()
{
  if (m_pByteArray) {
    delete m_pByteArray;
    m_pByteArray = NULL;
  }
}

void CuCellCtrl::SetDataForDisplay(int itemsPerByte, long nbElements, CByteArray* pByteArray, int itemsPerCell)
{
  // Adjust dumb parameter if needed
  if (itemsPerCell == -1)
    itemsPerCell = itemsPerByte;

  ASSERT (itemsPerByte >= 0);
  ASSERT (nbElements >= 0);
  ASSERT (itemsPerCell >= 0);
  if (nbElements > 0)
    ASSERT (pByteArray);
  else
    ASSERT (!pByteArray);

  LiberateDataForDisplay();

  m_itemsPerByte    = itemsPerByte;
  m_nbElements      = nbElements;
  m_pByteArray      = pByteArray;

  // Create the relevant lines in the control, using no reduce Ratio
  DisplayAllLines(itemsPerCell);
}

void CuCellCtrl::DisplayAllLines(int nbItemsPerCell)
{
  DeleteAllItems();

  // Absolutely no lines if empty array
  if (!m_pByteArray)
    return;

  // Store the display ratio - will set m_nbItemsPerCell and m_deflateFactor
  SetDisplayCellRatio(nbItemsPerCell);

  // Calculate number of cells needed
  int nbCells = (m_nbElements + (m_deflateFactor - 1)) / m_deflateFactor;
  ASSERT (nbCells > 0);

  // calculate number of lines needed
  int nbLines = (nbCells + (m_nbCellsByLine - 1)) / m_nbCellsByLine;
  ASSERT (nbLines > 0);

  // Adjust width of first column, based on caption of the last line
  CString csMax;
  long lItemsPerLine = m_nbItemsPerCell * m_nbCellsByLine;
  long lVal = lItemsPerLine * (nbLines-1);
  ASSERT (lVal <= m_nbElements * m_itemsPerByte);
  if (lVal < 100)
    lVal = 100;   // Minimum 3 digits
  csMax.Format (_T("%ld  "), lVal);
  int len = GetStringWidth(csMax);
  len += 2;   // left margin of pseudo-image
  len += 2 * OFFSET_FIRST;  // left/right margin of first column
  SetColumnWidth(0, len);

  // insert as many lines as needed
  for (int cpt = 0; cpt < nbLines; cpt++) {
    int index = InsertItem(cpt, _T(""));
  }
  ASSERT (GetItemCount() == nbLines);

  // For speed: set member variable "number of items on one line"
  m_nbItemsPerLine = m_nbItemsPerCell * m_nbCellsByLine;
}


void CuCellCtrl::SetDisplayCellRatio(int nbItemsPerCell)
{
  ASSERT (nbItemsPerCell);
  ASSERT ( (nbItemsPerCell % m_itemsPerByte) == 0);

  if (nbItemsPerCell < m_itemsPerByte) {
    ASSERT (FALSE);
    m_nbItemsPerCell = m_itemsPerByte;
  }
  else
    m_nbItemsPerCell = nbItemsPerCell;

  m_deflateFactor = nbItemsPerCell / m_itemsPerByte;
}

void CuCellCtrl::DrawDetailedItem(CDC* pDC, CRect& rcItem, int index)
{
  BYTE by = m_pByteArray->GetAt(index);

  // Coloured rectangle according to byte
  COLORREF pageColor;
  if (m_bUsesPalette)
    pageColor = PALETTEINDEX(GetDetailPaletteIndex(by));
  else
    pageColor = GetPageColor(by);

  CRect rcInside(rcItem);
  COLORREF oldBkColor = pDC->GetBkColor();
  pDC->FillSolidRect(rcInside, pageColor);
  pDC->SetBkColor(oldBkColor);      // Since has been changed by FillSolidRect...

  // Image according to page type
	CImageList* pImageList;
	UINT uiFlags = ILD_TRANSPARENT;
	COLORREF clrImage = pageColor;  // m_colorTextBk;
  pImageList = CListCtrl::GetImageList(LVSIL_SMALL);
  if (pImageList) {
    if (rcItem.left < rcItem.right-1) {
      // Use height of bitmap inside image list.
      // images must be centered in the imagelist bounding rectangle.
      IMAGEINFO imgInfo;
      int height = PAGE_IMAGE_SIZE;
      if (pImageList->GetImageInfo(0, &imgInfo))
	      height = imgInfo.rcImage.bottom - imgInfo.rcImage.top;
      int imageIndex = GetImageIndex(by);
      ASSERT (imageIndex != -1);
      int drawWidth = PAGE_IMAGE_SIZE;
      int offsetH = (rcItem.Width() - PAGE_IMAGE_SIZE) / 2;
      if (offsetH < 0) {
        drawWidth = rcItem.Width();
        offsetH = 0;    // Mask if truncate image
      }
      if (m_bUsesPalette) {
        ASSERT (m_pOldPalette);
        pDC->SelectPalette (m_pOldPalette, FALSE);
        pDC->RealizePalette();
      }
      ImageList_DrawEx (pImageList->m_hImageList, imageIndex, pDC->m_hDC,
                        rcItem.left + offsetH, rcItem.top,
                        drawWidth, PAGE_IMAGE_SIZE,
                        CLR_NONE/*m_colorTextBk*/, CLR_NONE/*clrImage*/, uiFlags);
      if (m_bUsesPalette && m_pDetailedPalette) {
        m_pOldPalette = pDC->SelectPalette (m_pDetailedPalette, FALSE);
        pDC->RealizePalette();
      }
    }
  }

}

void CuCellCtrl::DrawDeflatedItem(CDC* pDC, CRect& rcItem, int startIndex, int stopIndex)
{
  long lIntensity = 0;
  long lOverflow  = 0;

  int lGroupFactor = stopIndex + 1 - startIndex;
  ASSERT (lGroupFactor > 0);
  if (lGroupFactor == 1) {
    // Only one cell : use overflow factor 'as is'
    BYTE by = m_pByteArray->GetAt(startIndex);
    lIntensity = GetFillFactor(by, m_itemsPerByte);
    lOverflow  = GetOverflowFactor(by, m_itemsPerByte);
  }
  else {
    // Levels for pseudo exponential calculation on 3 levels for all bytes
    long level1of3 = 0L;
    long level2of3 = 0L;
    level1of3 = 1;
    while (level1of3*level1of3*level1of3 < lGroupFactor*m_itemsPerByte)
      level1of3++;
    level2of3 = level1of3 * level1of3;

    // Levels for pseudo exponential calculation on 6 levels for one byte
    long level1of6 = 0L;
    long level3of6 = 0L;
    long level5of6 = 0L;
    level1of6 = 1;
    while ( (level1of6*level1of6*level1of6 * level1of6*level1of6*level1of6) < m_itemsPerByte)
      level1of6++;
    level3of6 = level1of6 * level1of6 * level1of6;
    level5of6 = level3of6 * level1of6 * level1of6;
  
    for (int index = startIndex; index <= stopIndex; index++) {
      BYTE by = m_pByteArray->GetAt(index);
      lIntensity += GetFillFactor(by, m_itemsPerByte);
      /*if (m_itemsPerByte == 1)
        lOverflow += GetOverflowFactor(by, m_itemsPerByte);
      else { */
        switch (GetOverflowFactor(by, m_itemsPerByte)) {
          case 0: break;
          case 1: lOverflow += level1of6; break;
          case 2: lOverflow += level3of6; break;
          case 3: lOverflow += level5of6; break;
          default:
            ASSERT (FALSE);
        }
      /*} */
    }

    // average value for intensity
    lIntensity /= lGroupFactor;

    // values by level for overflow
    if (lOverflow == 0)
      lOverflow = 0;
    else if (lOverflow < level1of3)
      lOverflow = 1;
    else if (lOverflow < level2of3)
      lOverflow = 2;
    else
      lOverflow = 3;
  }

  ASSERT (lIntensity >= 0);
  ASSERT (lIntensity < NBMETALBLUECOLORS);
  ASSERT (lOverflow >= 0);
  ASSERT (lOverflow < 4);

  //
  // 1) prepare our drawing objects
  //

  // "old" pens/brushes
  CPen*   pOldPen   = NULL;
  CBrush* pOldBrush = NULL;

  //
  // 1) Rectangle indicating free/use percentage (dark = used, clear = unused)
  //

  // Color for percentage used
  COLORREF pctUsedColor;
  if (m_bUsesPalette)
    pctUsedColor = PALETTEINDEX(lIntensity + NBDETAILCOLORS);   // offset due to detail colors at the beginning of the palette
  else
    pctUsedColor = GetPctUsedColor(lIntensity);

  CRect rcInside(rcItem);
  COLORREF oldBkColor = pDC->GetBkColor();
  pDC->FillSolidRect(rcInside, pctUsedColor);
  pDC->SetBkColor(oldBkColor);      // Since has been changed by FillSolidRect...

  //
  // 2) Indicator for overflow pages presence - value 0 to 3 inclusive
  //
  if (lOverflow > 0) {
    // Color for Overflow presence
    COLORREF overflowColor = GetOverflowColor(lOverflow);

    // Red character '*', the higher the overflow is, the more characters we display

    CRect rcInside(rcItem);
    COLORREF oldTxtColor = pDC->SetTextColor(overflowColor);
    int oldBkMode = pDC->SetBkMode(TRANSPARENT);

    CString csOverflow(_T('*'), lOverflow);

    rcInside.right -= 2;  // otherwize overflow sign would be too close to cell border

		pDC->DrawText (csOverflow,
                   rcInside,
                   DT_RIGHT | DT_SINGLELINE | DT_NOPREFIX | DT_TOP);

    pDC->SetBkMode(oldBkMode);
    pDC->SetTextColor(oldTxtColor);
  }

}

/////////////////////////////////////////////////////////////////////////////
// Old code, intensively revized


LPCTSTR CuCellCtrl::MakeShortString (CDC* pDC, LPCTSTR lpszLong, int nColumnLen, int nOffset)
{
	static const _TCHAR szThreeDots[] = _T("...");
	int nStringLen = lstrlen(lpszLong);
	
	if(nStringLen==0 || pDC->GetTextExtent(lpszLong,nStringLen).cx+nOffset <= nColumnLen)
		return lpszLong;
	static TCHAR szShort[MAX_PATH];
	
	lstrcpy (szShort,lpszLong);
	int nAddLen = pDC->GetTextExtent (szThreeDots, sizeof(szThreeDots)).cx;
	
	TCHAR* ptchszShort = _tcsdec(szShort, szShort + _tcslen(szShort));
	int nCx = pDC->GetTextExtent (szShort, _tcslen(szShort)).cx;
	BOOL bShort = (nCx + nOffset + nAddLen <= nColumnLen);
	while (!bShort && szShort[0] != _T('\0'))
	{
		ptchszShort = _tcsdec(szShort, ptchszShort);
		if (!ptchszShort)
			break;
		*ptchszShort= _T('\0');
		nCx = pDC->GetTextExtent (szShort, _tcslen(szShort)).cx;
		bShort = (nCx + nOffset + nAddLen <= nColumnLen);
	}

	lstrcat (szShort, szThreeDots);
	return szShort;
}


CuCellCtrl::CuCellCtrl()
{
  m_nbCellsByLine   = -1;
  m_itemsPerByte = 1;
  m_nbElements      = -1L;
  m_pByteArray      = NULL;
  m_nbItemsPerCell  = 1;
  m_nbItemsPerLine  = -1;
  m_deflateFactor   =  -1;

  // check whether we need a palette
  m_bUsesPalette = FALSE;
  CClientDC dc(AfxGetMainWnd());
  if ((dc.GetDeviceCaps(RASTERCAPS) & RC_PALETTE) != 0) {
    int palSize  = dc.GetDeviceCaps(SIZEPALETTE);
    if (palSize > 0) {
      ASSERT (palSize >= 256);
      m_bUsesPalette = TRUE;
    }
  }
  m_pOldPalette = NULL;

	OFFSET_FIRST = 2;
	OFFSET_OTHER = 6;

	m_pImageList = NULL;
	m_pCheckBox  = NULL;
	m_bFullRowSelected  = FALSE;    // not full row selected!
	m_bLineSeperator    = TRUE;
	m_colorText         = ::GetSysColor (COLOR_WINDOWTEXT);
	m_colorTextBk       = ::GetSysColor (COLOR_WINDOW);
	m_colorHighLight    = ::GetSysColor (COLOR_HIGHLIGHT);
	m_colorHighLightText= ::GetSysColor (COLOR_HIGHLIGHTTEXT);
  m_colorLine         = RGB (0, 0, 0);
	if (m_colorLine == m_colorTextBk)
		m_colorLine = ::GetSysColor (COLOR_GRAYTEXT); 
	m_iTotalColumn  = 0;
	m_xBitmapSpace  = 0;
	m_hListBitmap   = NULL;

}

CuCellCtrl::~CuCellCtrl()
{
	LiberateDataForDisplay();

	if (m_pCheckBox)
		delete m_pCheckBox;
}


BEGIN_MESSAGE_MAP(CuCellCtrl, CListCtrl)
	//{{AFX_MSG_MAP(CuCellCtrl)
	ON_NOTIFY_REFLECT(LVN_INSERTITEM, OnInsertitem)
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDBLCLK()
	//}}AFX_MSG_MAP
	ON_WM_SYSCOLORCHANGE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuCellCtrl message handlers

void CuCellCtrl::DrawItem (LPDRAWITEMSTRUCT lpDrawItemStruct)
{
  //TRACE0 ("CuCellCtrl::DrawItem() ...\n");
	
	static TCHAR tchszBuffer [MAX_PATH];  
	LPCTSTR  pszItem;
	CString  strItem;
	COLORREF colorTextOld;
	COLORREF colorBkOld;
	
	CDC* pDC = CDC::FromHandle (lpDrawItemStruct->hDC);
	if ((int)lpDrawItemStruct->itemID < 0)
		return;

  // Update palette need flag(s)
  // check whether we need a palette
  m_bUsesPalette = FALSE;
  CClientDC dc(AfxGetMainWnd());
  if ((dc.GetDeviceCaps(RASTERCAPS) & RC_PALETTE) != 0) {
    int palSize  = dc.GetDeviceCaps(SIZEPALETTE);
    if (palSize > 0) {
      ASSERT (palSize >= 256);
      m_bUsesPalette = TRUE;
    }
  }

  //CPalette* pOldPalette = NULL;
  if (m_bUsesPalette) {
    if (m_itemsPerByte == 1 && m_deflateFactor == 1 && m_pDetailedPalette)
      m_pOldPalette = pDC->SelectPalette (m_pDetailedPalette, FALSE);
    else
	if (m_pMetalBluePalette)
      m_pOldPalette = pDC->SelectPalette (m_pMetalBluePalette, FALSE);
    pDC->RealizePalette();
  }

	int  nItem     = lpDrawItemStruct->itemID;
  MYTRACE1 ("DrawItem %d...\n", nItem);
	BOOL bFocus    = GetFocus() == this;
	BOOL bSelected = (bFocus || (GetStyle() & LVS_SHOWSELALWAYS)) && (GetItemState(nItem, LVIS_SELECTED) & LVIS_SELECTED);
	
	CRect rcItem;
	CRect rcAllLabels;
	CRect rcLabel;
	GetItemRect (nItem, rcAllLabels, LVIR_BOUNDS);
	GetItemRect (nItem, rcLabel,     LVIR_LABEL);
	rcAllLabels.left = rcLabel.left;

	if (m_bFullRowSelected)
	{
		if (rcAllLabels.right < m_cxClient)
			rcAllLabels.right = m_cxClient;
	}
	CRect r = rcAllLabels;
	r.top++;
	
	if (bSelected)
	{
		colorTextOld = pDC->SetTextColor (m_colorHighLightText);
		colorBkOld   = pDC->SetBkColor   (m_colorHighLight);
		CBrush brush(m_colorHighLight);
		pDC->FillRect (r, &brush);
	}
	else
	{
		colorTextOld = pDC->SetTextColor (m_colorText);
		CBrush brush(m_colorTextBk);
		pDC->FillRect (r, &brush);
	}

	//
	// Get item data
	//CImageList* pImageList;
	UINT uiFlags = ILD_TRANSPARENT;
	COLORREF clrImage = m_colorTextBk;

	LV_ITEM  lvi;
	lvi.mask  = LVIF_IMAGE | LVIF_STATE|LVIF_PARAM;
	lvi.iItem = lpDrawItemStruct->itemID;
	lvi.iSubItem = 0;
	lvi.stateMask = 0xFFFF; // Get all state flags
	CListCtrl::GetItem(&lvi);
	//
	// set color and mask for the icon

	if (lvi.state & LVIS_CUT)
	{
		clrImage = m_colorTextBk;
		uiFlags |= ILD_BLEND50;
	}
	else if (bSelected)
	{
		clrImage = ::GetSysColor(COLOR_HIGHLIGHT);
		uiFlags |= ILD_BLEND50;
	}

	GetItemRect (nItem, rcItem, LVIR_LABEL);
	CRect rcIcon;
	CListCtrl::GetItemRect(nItem, rcIcon, LVIR_ICON);

  // Cumulate both widths
  rcItem.left = rcIcon.left;

	//
	// Draw the Main Item Label.
	BOOL   bDrawCheck = FALSE;
	int    iCheck     = 0; // (0 uncheck), (1 check), (2 uncheck grey), (3 check grey)
	CPoint checkPos;
	LV_COLUMN lvc; 
	memset (&lvc, 0, sizeof (lvc));
	lvc.mask = LVCF_FMT | LVCF_WIDTH;
	BOOL bOk = CListCtrl::GetColumn (0, &lvc);
  ASSERT (bOk);

	// Not anymore: strItem = GetItemText (nItem, 0);
  long lNum;
  lNum = lpDrawItemStruct->itemID * m_nbItemsPerLine;
  strItem.Format(_T("%ld  "), lNum);   // 2 trailing spaces because of right justify

	//pszItem = MakeShortString (pDC, strItem, rcItem.right - rcItem.left, 2*OFFSET_OTHER);
  pszItem = strItem;

	rcLabel = rcItem;
	rcLabel.left  += OFFSET_FIRST;
	rcLabel.right -= OFFSET_FIRST;
		
	UINT nJustify = DT_LEFT;
	checkPos.x = rcLabel.left; 
	checkPos.y = rcLabel.top;

  // Emb found that received fmt looses justifymask attribute for first column
  // Force to right justify
  lvc.fmt |= LVCFMT_RIGHT;
	switch(lvc.fmt & LVCFMT_JUSTIFYMASK)
	{
	case LVCFMT_RIGHT:
    if (strItem == pszItem)
      nJustify = DT_RIGHT;
    else
      nJustify = DT_LEFT;
		//nJustify = (strItem == pszItem)? DT_RIGHT: DT_LEFT;

		checkPos.x = rcLabel.right - PAGE_IMAGE_SIZE;
		checkPos.y = rcLabel.top;
		break;
	case LVCFMT_CENTER:
		nJustify = (strItem == pszItem)? DT_CENTER: DT_LEFT;
		checkPos.x = rcLabel.left + rcLabel.Width()/2 - 8; 
		checkPos.y = rcLabel.top;
		break;
	default:
		break;
	}

	if (!bDrawCheck)
		pDC->DrawText (pszItem, rcLabel, nJustify|DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER);
	else
	{
		m_pImageList->Draw(pDC, iCheck, checkPos, ILD_TRANSPARENT);
	}

#define BIGPENWIDTH 2
	CPen  penCell (PS_SOLID, 1, m_colorLine);
  CPen  bigPen  (PS_SOLID, BIGPENWIDTH, m_colorLine);
	CPen* pOldPen = pDC->SelectObject (&bigPen);
	if (m_bLineSeperator)
	{
		pDC->MoveTo (rcItem.right-1, rcItem.top);
		pDC->LineTo (rcItem.right-1, rcItem.bottom - (BIGPENWIDTH -1) );
	}
	pDC->SelectObject (&penCell);

  int lineNumber = lpDrawItemStruct->itemID;
  int nColumn, indexInByteArray;
  BOOL bNotFullLine = FALSE;
	for (nColumn=1, indexInByteArray = lineNumber * m_nbCellsByLine * m_deflateFactor;
       nColumn <= m_nbCellsByLine ;
       nColumn++, indexInByteArray += m_deflateFactor)
	{
    BOOL bOk = CListCtrl::GetColumn (nColumn, &lvc);
    ASSERT (bOk);
    BOOL bPartialOverflow = FALSE;

		bDrawCheck = FALSE;
		rcItem.left   = rcItem.right;
		rcItem.right += lvc.cx;

		// not anymore: strItem = GetItemText (nItem, nColumn);
    if (indexInByteArray >= m_nbElements) {
#ifdef _DEBUG
        CString csTrace;
        csTrace.Format("Cell %d of line %d: access byteArray at %d would overflow number of elements %d\n",
                        nColumn-1, lineNumber, indexInByteArray, m_nbElements);
        MYTRACE0(csTrace);
#endif /* _DEBUG */
      bNotFullLine = TRUE;
      break;
    }
    else {
      // partial overflow in array ?
      if (indexInByteArray  + m_deflateFactor - 1 >= m_nbElements) {
        bPartialOverflow = TRUE;
        #ifdef _DEBUG
          CString csTrace;
	        csTrace.Format("Cell %d of line %d: access byteArray from %d to %d inclusive truncated to %d\n",
                        nColumn-1, lineNumber,
                        indexInByteArray, indexInByteArray + m_deflateFactor - 1,
                        m_nbElements - 1);
          MYTRACE0(csTrace);
        #endif  // _DEBUG
      }
      else {
        #ifdef _DEBUG
          CString csTrace;
          csTrace.Format("Cell %d of line %d: access byteArray from %d to %d inclusive\n",
                          nColumn-1, lineNumber,
                          indexInByteArray, indexInByteArray + m_deflateFactor - 1);
          MYTRACE0(csTrace);
        #endif  // _DEBUG
      }
    }

    // Calculate top index in byteArray
    int topIndex; // inclusive
    if (bPartialOverflow)
      topIndex = m_nbElements -1;
    else
      topIndex = indexInByteArray + m_deflateFactor - 1;

    // display according to indexes
    if (m_itemsPerByte == 1 && topIndex == indexInByteArray)
      DrawDetailedItem(pDC, rcItem, indexInByteArray);
    else
      DrawDeflatedItem(pDC, rcItem, indexInByteArray, topIndex);

    // draw separator before next cell
		if (m_bLineSeperator)
		{
			pDC->MoveTo (rcItem.right-1, rcItem.top);
			pDC->LineTo (rcItem.right-1, rcItem.bottom);
		}
	}
	if (m_bLineSeperator)
	{
    int firstColWidth = GetColumnWidth(0);

    if (bNotFullLine) {
      rcAllLabels.right = rcItem.right - lvc.cx;
    }

		pDC->MoveTo (rcAllLabels.left - rcIcon.right - 1,  rcAllLabels.top-1);
		pDC->LineTo (rcAllLabels.right, rcAllLabels.top-1);
		pDC->MoveTo (rcAllLabels.left - rcIcon.right - 1,  rcAllLabels.bottom-1);
		pDC->LineTo (rcAllLabels.right, rcAllLabels.bottom-1);

	}

	pDC->SelectObject (pOldPen);
	
	CRect rcFocus = rcAllLabels;
	rcFocus.top++;
	rcFocus.bottom--;
	if (bFocus && (GetItemState(nItem, LVIS_FOCUSED) & LVIS_FOCUSED))
		pDC->DrawFocusRect (rcFocus);
	
	if (bSelected)
	{
		pDC->SetBkColor   (colorBkOld);
	}
	pDC->SetTextColor (colorTextOld);

  if (m_bUsesPalette) {
    ASSERT (m_pOldPalette);
    if (m_pOldPalette)
      pDC->SelectPalette(m_pOldPalette, FALSE);
  }
}

BOOL CuCellCtrl::OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult) 
{
	switch (message)
	{
	case WM_DRAWITEM:
		ASSERT(pLResult == NULL);    // No return value expected
		DrawItem ((LPDRAWITEMSTRUCT)lParam);
		break;
	default:
		return CListCtrl::OnChildNotify(message, wParam, lParam, pLResult);
	}
	return TRUE;
}

void CuCellCtrl::OnInsertitem(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	
	*pResult = 0;
}



int CuCellCtrl::InsertColumn (int nCol, const LV_COLUMN* pColumn, BOOL bCheckBox)
{
  // No checkboxes for our matrix cell control
  ASSERT (!bCheckBox);

	if (bCheckBox)
	{
		//
		// You must specify the check image list
		ASSERT (m_pImageList != NULL);
	}
	int i, index = CListCtrl::InsertColumn (nCol, pColumn);
	if (index > -1)
	{
		BOOL* pArray = new BOOL [m_iTotalColumn +1];
		if (nCol < m_iTotalColumn)
		{
			//
			// Shift the columns to the right.
			for (i=0; i<nCol; i++)
				pArray[i] = m_pCheckBox [i];
			pArray[nCol]  = TRUE;
			for (i=nCol+1; i<=m_iTotalColumn; i++)
				pArray[i] = m_pCheckBox [i-1];
		}
		else
		{
			for (i=0; i<m_iTotalColumn; i++)
				pArray[i] = m_pCheckBox [i];
			pArray[nCol]  = bCheckBox;
		}
		if (m_pCheckBox)
			delete [] m_pCheckBox;
		m_pCheckBox = pArray;
		m_iTotalColumn++;
	}
	return index;
}


BOOL CuCellCtrl::DeleteColumn(int nCol)
{
	BOOL ok = CListCtrl::DeleteColumn (nCol);
	if (ok)
	{
		if (nCol < (m_iTotalColumn-1))
		{
			// 
			// Shift columns to the left.
			for (int i=nCol; i<m_iTotalColumn-1; i++)
				m_pCheckBox[i] = m_pCheckBox[i+1];
		}
		m_iTotalColumn--;
	}
	return ok;
}


BOOL CuCellCtrl::PreCreateWindow(CREATESTRUCT& cs) 
{
	cs.style &= ~LVS_TYPEMASK;
	cs.style |= LVS_REPORT | LVS_OWNERDRAWFIXED | LVS_SHOWSELALWAYS;    
	m_bFullRowSelected = FALSE;
	return CListCtrl::PreCreateWindow(cs);
}

BOOL CuCellCtrl::GetCellRect (CRect rect, CPoint point, CRect& rCell, int& iCellNumber)
{
	int xStarted = rect.left, xEnded = rect.left;
	int iWidth = 0;
	rCell.CopyRect (rect); 
	
	for (int i=0; i<m_iTotalColumn; i++)
	{
		iWidth   = GetColumnWidth (i); 
		xStarted = xEnded+1;
		xEnded  += iWidth;
		if (xStarted <= point.x && point.x <= xEnded)
		{
			if (i==0)
			{
        // Emb replacement:
				rCell.left = xStarted-1;

			}
			else
				rCell.left = xStarted-1;
			rCell.right= xEnded;
			iCellNumber= i;
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CuCellCtrl::GetCellRect (CRect rect, int col, CRect& rCell)
{
	int xStarted = rect.left, xEnded = rect.left;
	int iWidth = 0;
	rCell.CopyRect (rect); 
	
	for (int i=0; i<m_iTotalColumn; i++)
	{
		iWidth   = GetColumnWidth (i); 
		xStarted = xEnded+1;
		xEnded  += iWidth;
		if (i == col)
		{
			if (i==0)
			{
        // Emb replacement:
				rCell.left = xStarted-1;

			}
			else
				rCell.left = xStarted-1;
			rCell.right= xEnded;
			return TRUE;
		}
	}
	return FALSE;
}


void CuCellCtrl::OnSize(UINT nType, int cx, int cy) 
{
	m_cxClient = cx;
	CListCtrl::OnSize(nType, cx, cy);
}

void CuCellCtrl::OnSysColorChange() 
{
	CListCtrl::OnSysColorChange();
	m_colorText         = ::GetSysColor (COLOR_WINDOWTEXT);
	m_colorTextBk       = ::GetSysColor (COLOR_WINDOW);
	m_colorHighLight    = ::GetSysColor (COLOR_HIGHLIGHT);
	m_colorHighLightText= ::GetSysColor (COLOR_HIGHLIGHTTEXT);
	if (m_colorTextBk == RGB (0, 0, 0))
		m_colorLine = RGB (192, 192, 192);
	else
		m_colorLine = RGB (0, 0, 0);
}

void CuCellCtrl::OnPaint() 
{
	if (m_bFullRowSelected || (GetStyle() & LVS_REPORT))
	{
		CRect rcAllLabels;
		GetItemRect (0, rcAllLabels, LVIR_BOUNDS);
		if (rcAllLabels.right <= m_cxClient)
		{
			CPaintDC dc(this);
			CRect rcClip;
			dc.GetClipBox (rcClip);
			rcClip.left = min (rcAllLabels.right-1, rcClip.left);
			rcClip.right= m_cxClient;
			InvalidateRect (rcClip, FALSE);
		}
	}
	CListCtrl::OnPaint();
}


void CuCellCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
{
  // Simply ignore clicks, not to draw selections
}

void CuCellCtrl::OnRButtonDown(UINT nFlags, CPoint point) 
{
  // Simply ignore clicks, not to draw selections
}

void CuCellCtrl::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
  // Simply ignore clicks, not to draw selections
}

void CuCellCtrl::OnRButtonDblClk(UINT nFlags, CPoint point) 
{
  // Simply ignore clicks, not to draw selections
}



