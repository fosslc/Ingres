/*
**  Copyright (C) 2005-2006 Ingres Corporation.
*/

/*
**    Source   : colorind.cpp, Implementation file 
**    Project  : OpenIngres Visual DBA
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Static Control that paints itself with a given color
**
** History:
**
** xx-Feb-1998 (uk$so01)
**    Created
** 03-Sep-2001 (uk$so01)
**    Transform code to be an ActiveX Control
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
** 01-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (vcbf uses libwctrl.lib).
** 08-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (vdba uses libwctrl.lib).
** 09-Mar-2004 (uk$so01)
**    SIR #110013 (Handle the DAS Server)
*/

#include "stdafx.h"
#include "colorind.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define NBMETALBLUECOLORS 64
#define NBDETAILCOLORS 16   // was 12, added Hash specific entries
typedef enum {
	PAGE_HEADER = 1,          // Free Header page
	PAGE_MAP,                 // Free Map    page
	PAGE_FREE,                // Free page, has been used and subsequently made free
	PAGE_ROOT,                // Root page
	PAGE_INDEX,               // Index page
	PAGE_SPRIG,               // Sprig page
	PAGE_LEAF,                // Leaf page
	PAGE_OVERFLOW_LEAF,       // Overflow leaf page
	PAGE_OVERFLOW_DATA,       // Overflow data page
	PAGE_ASSOCIATED,          // Associated data page
	PAGE_DATA,                // Data page
	PAGE_UNUSED,              // Unused page, has never been written to
	// hash specific (for Ingres >= 2.5)
	PAGE_EMPTY_DATA_NO_OVF,   // Empty data, with no overflow page
	PAGE_DATA_WITH_OVF,       // Non-Empty data, with overflow pages
	PAGE_EMPTY_DATA_WITH_OVF, // Empty data, wih overflow pages
	PAGE_EMPTY_OVERFLOW,      // Empty overflow page
	// end of hash specific section (for ingres >2.5)
	PAGE_ERROR_UNKNOWN        // UNKNOWN TYPE!
} pageType;

BOOL CreateMultiUsePalette(CPalette* pMultiUsePalette);
static int GetDetailPaletteIndex(BYTE by) {return by-1;}
static COLORREF GetPctUsedColor(int intensity);

static COLORREF GetPageColor(BYTE by)
{
	switch (by) 
	{
	case (BYTE) PAGE_HEADER       : return RGB(192, 192, 192);    // light gray
	case (BYTE) PAGE_MAP          : return RGB(192, 192, 192);    // light gray
	case (BYTE) PAGE_FREE         : return GetPctUsedColor(0);    // same as null intensity
	case (BYTE) PAGE_ROOT         : return RGB(254, 192, 65);     // pumpkin brown-orange
	case (BYTE) PAGE_INDEX        : return RGB(240, 240, 0);      // Yellow
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

	default:
		ASSERT (FALSE);
		return RGB(0, 0, 0);        // black as error
	}
}

static COLORREF GetPctUsedColor(int intensity)
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

BOOL UxCreateFont (CFont& font, LPCTSTR lpFaceName, int size, int weight, int bItalic)
{
	LOGFONT lf;
	memset(&lf, 0, sizeof(lf));
	lf.lfHeight        = size; 
	lf.lfWidth         = 0;
	lf.lfEscapement    = 0;
	lf.lfOrientation   = 0;
	lf.lfWeight        = weight;
	lf.lfItalic        = bItalic;
	lf.lfUnderline     = 0;
	lf.lfStrikeOut     = 0;
	lf.lfCharSet       = 0;
	lf.lfOutPrecision  = OUT_CHARACTER_PRECIS;
	lf.lfClipPrecision = 2;
	lf.lfQuality       = PROOF_QUALITY;
	lstrcpy(lf.lfFaceName, lpFaceName);
	return font.CreateFontIndirect(&lf);
}


/////////////////////////////////////////////////////////////////////////////
// CuStaticColorWnd

CuStaticColorWnd::CuStaticColorWnd()
{
	m_crColorPaint = NULL;

	m_bHatch = FALSE;
	m_crColorHatch = NULL;
	m_nXHatchWidth  = 4;
	m_nXHatchHeight = 2;
	m_Palette = NULL;
	m_bPaletteAware = TRUE;
}

CuStaticColorWnd::~CuStaticColorWnd()
{
}


BEGIN_MESSAGE_MAP(CuStaticColorWnd, CStatic)
	//{{AFX_MSG_MAP(CuStaticColorWnd)
	ON_WM_PAINT()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuStaticColorWnd message handlers

void CuStaticColorWnd::OnPaint() 
{
	CPaintDC dc(this);
	// Determine whether we need a palette, and use it if found so
	CPalette* pOldPalette = NULL;
	CPalette staticColorPalette;
	BOOL bUsesPalette = FALSE;
	if (m_bPaletteAware)
	{
		CClientDC mainDc(GetParent());
		if ((mainDc.GetDeviceCaps(RASTERCAPS) & RC_PALETTE) != 0) 
		{
			int palSize  = mainDc.GetDeviceCaps(SIZEPALETTE);
			if (palSize > 0) 
			{
				if (!m_Palette)
				{
					ASSERT (palSize >= 256);
					BOOL bSuccess = CreateMultiUsePalette(&staticColorPalette);
					ASSERT (bSuccess);
					if (bSuccess) {
						pOldPalette = dc.SelectPalette (&staticColorPalette, FALSE);
						dc.RealizePalette();
						bUsesPalette = TRUE;
					}
				}
				else
				{
					pOldPalette = dc.SelectPalette (m_Palette, FALSE);
					dc.RealizePalette();
					bUsesPalette = TRUE;
				}
			}
		}
	}

	// Coloured rectangle according to byte
	COLORREF crPaint;
	if (m_crColorPaint)
		if (bUsesPalette)
			crPaint = PALETTERGB(GetRValue(m_crColorPaint), GetGValue(m_crColorPaint), GetBValue(m_crColorPaint));
		else
			crPaint = m_crColorPaint;
	else
		crPaint =  GetSysColor (COLOR_BTNFACE);
	CBrush rb (crPaint);
	CBrush* pOld = dc.SelectObject (&rb);
	CRect rcInit;
	GetClientRect (rcInit);
	dc.FillRect (rcInit, &rb);
	dc.SelectObject (pOld);
	
	//
	// Draw Hatch
	if (m_bHatch && m_crColorHatch)
	{
		COLORREF crHatch;
		if (bUsesPalette)
			crHatch = PALETTERGB(GetRValue(m_crColorHatch), GetGValue(m_crColorHatch), GetBValue(m_crColorHatch));
		else
			crHatch = m_crColorHatch;
		CRect rh = rcInit;
		CBrush br(crHatch);
		//
		// Draw two rectangles:

		// upper:
		rh.left    = rh.right - m_nXHatchWidth - 4;
		rh.right   = rh.right - 2;
		rh.top     = rh.top    + 2;
		rh.bottom  = rh.top + m_nXHatchHeight;
		dc.FillRect (rh, &br);

		// lower:
		rh = rcInit;
		rh.left    = rh.right - m_nXHatchWidth - 4;
		rh.right   = rh.right - 2;
		rh.bottom  = rh.bottom - 1;
		rh.top     = rh.bottom - m_nXHatchHeight;
		dc.FillRect (rh, &br);
	}

	CString strText;
	GetWindowText (strText);
	if (!strText.IsEmpty())
	{
		CFont font;
		if (UxCreateFont (font, _T("MS Sans Serif"), 16))
		{
			//
			// Draw the Title of the Pie Chart.
			COLORREF colorText    = GetSysColor (COLOR_WINDOWTEXT);
			COLORREF colorTextOld = dc.SetTextColor (colorText);

			CFont* pOldFond = dc.SelectObject (&font);
			int nBkOldMode = dc.SetBkMode(TRANSPARENT);
			dc.DrawText(strText,-1, rcInit, DT_SINGLELINE|DT_LEFT);
			dc.SetBkMode(nBkOldMode);
			dc.SelectObject (pOldFond);
			dc.SetTextColor (colorTextOld);
		}
	}

	// Finished with palette
	if (bUsesPalette) {
		dc.SelectPalette(pOldPalette, FALSE);
		//dc.RealizePalette();
		staticColorPalette.DeleteObject();
	}
}

int CuStaticColorWnd::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CStatic::OnCreate(lpCreateStruct) == -1)
		return -1;
	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	if (hFont == NULL)
		hFont = (HFONT)GetStockObject(ANSI_VAR_FONT);
	SendMessage(WM_SETFONT, (WPARAM)hFont);

	return 0;
}

BOOL CreateMultiUsePalette(CPalette* pMultiUsePalette)
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
	};
	int cpt;
	for (cpt=0; cpt < NBDETAILCOLORS; cpt++) {

		// check compliance between indexes and byte values
//		ASSERT (GetDetailPaletteIndex(atypes[cpt]) == cpt);

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
