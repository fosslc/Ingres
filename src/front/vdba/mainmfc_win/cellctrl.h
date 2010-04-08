/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : cellctrl.h, header file
**    Project  : CA-OpenIngres/Visual DBA.
**    Author   : Emmanuel Blattes - based on UK Sotheavut's work for ownerdraw ListCtrl
**    Purpose  : Cellular control Originally designed to display pages usage of an
**               OpenIngres table, as a matrix with nice colors/images/texts in it
**               Absolutely NO DATA/TEXT/ETC stored in items, all is managed 
**               by OnDraw using exclusively the byte array plus 
**
** History:
**
** 18-Mar-1998 (Emmanuel Blattes)
**    created.
** 04-Jul-2002 (uk$so01)
**    BUG #108183, Graphic in Pages Tab of DOM/Table keeps flickering.
**  28-apr-2009 (smeke01) b121545
**   Increase NBDETAILCOLORS to 26 to accommodate index types 1 to 9 and 
**   PAGE_ERROR_UNKNOWN (displayed as a '?').
*/

#ifndef CELLCTRL_HEADER
#define CELLCTRL_HEADER

#include "cellintc.h"   // C interface

// Size, in pixels, of each cell image - originally 16
#define PAGE_IMAGE_SIZE   13

#define XBITMAP_FOCUS           0
#define XBITMAP_BITMAP          1
#define XBITMAP_SELECTFOCUS     2
#define XBITMAP_SELECT          3

#define NBMETALBLUECOLORS 64
#define NBDETAILCOLORS 26


/////////////////////////////////////////////////////////////////////////////
// Customized tooltip control

class CuToolTipCtrl: public CToolTipCtrl
{
public:
  BOOL AddWindowTool (CWnd* pWnd,  LPCTSTR pszText);
  BOOL AddRectTool(CWnd*pWnd, LPCTSTR pszText, LPCRECT lpRect, UINT nIDTool);
};

/////////////////////////////////////////////////////////////////////////////
// Cellular custom control

class CuCellCtrl : public CListCtrl
{
public:
	CuCellCtrl();
	virtual ~CuCellCtrl();

  void InitializeCells(int nbCellsByLine = 50);
  void SetDataForDisplay(int itemsPerByte, long nbElements, CByteArray* pByteArray, int itemsPerCell = -1);

  int  GetDisplayItemsPerCell() { ASSERT (m_nbItemsPerCell); return m_nbItemsPerCell; }
  void DisplayAllLines(int nbItemsPerCell);

// redefined on purpose to prevent from storing/retrieving any text in control
  virtual BOOL SetItemText ( int nItem, int nSubItem, LPTSTR lpszText) { ASSERT (FALSE); return FALSE; }
  virtual int GetItemText ( int nItem, int nSubItem, LPTSTR lpszText, int nLen) const { ASSERT(FALSE); lpszText[0] = 0; return -1; }
  virtual CString GetItemText (int nItem, int nSubItem) const { ASSERT(FALSE); return _T(""); }

  // contextual tooltip
  CString GetShortTooltipText();
  void SetMetalBluePalette(CPalette* p){m_pMetalBluePalette = p;}
  void SetDetailPalette(CPalette* p){m_pDetailedPalette = p;}

private:
  // Cells per line - set by InitializeCells exclusively
  int m_nbCellsByLine;

  // Infos received from low level
  int  m_itemsPerByte;      // number of items per byte - is also the minimum number of items per cell
  long m_nbElements;        // number of elements in the byte array
  CByteArray* m_pByteArray;

  // Variations for display
  int m_nbItemsPerCell;     // How many items per cell currently? Can be set

  // Accelerators for calculations
  int m_nbItemsPerLine;     // How many items per line? set only by CreateAllLines()
  int m_deflateFactor;      // Ratio between m_itemsPerByte and m_nbItemsPerCell

  CImageList m_ImageList;   // images for pages types, in detailed mode

  CuToolTipCtrl m_ctlTT;

  void LiberateDataForDisplay();
  void SetDisplayCellRatio(int nbItemsPerCell);

  void DrawDetailedItem(CDC* pDC, CRect& rcItem, int index);
  void DrawDeflatedItem(CDC* pDC, CRect& rcItem, int startIndex, int stopIndex);

  static COLORREF GetPctUsedColor(int intensity);
  COLORREF GetOverflowColor(int overflow);

  // For palette management
  BOOL m_bUsesPalette;
  CPalette* m_pMetalBluePalette;
  CPalette* m_pDetailedPalette;
  CPalette* m_pOldPalette;

  static BOOL CreateMultiUsePalette(CPalette* pMultiUsePalette);

public:
  static   BOOL     CreateMetalBluePalette(CPalette* pMetalBluePalette);
  static   COLORREF GetZeroPctColor() { return GetPctUsedColor(0); }
  static   COLORREF GetHundredPctColor() { return GetPctUsedColor(63); }
  static   int      GetDetailPaletteIndex(BYTE by);
  static   BOOL     CreateDetailedPalette(CPalette* pDetailedPalette);
  static   COLORREF GetPageColor(BYTE by);
  static   int      GetImageIndex(BYTE by);

private:
	int InsertColumn (int nCol, const LV_COLUMN* pColumn, BOOL bCheckBox = FALSE);
	BOOL DeleteColumn(int nCol);
	int  GetTotalColumn (){return m_iTotalColumn;}; 
	BOOL GetCellRect (CRect rect, CPoint point, CRect& rCell, int& iCellNumber);
	BOOL GetCellRect (CRect rect, int col, CRect& rCell);

	void SetLineSeperator   (BOOL bLine = TRUE) {m_bLineSeperator = bLine;}
	void SetFullRowSelected (BOOL bFullRow = TRUE) {m_bFullRowSelected = bFullRow;}
	BOOL GetFullRowSelected () {return m_bFullRowSelected;}
	// void SetCheckImageList (CImageList* pImageList){m_pImageList = pImageList;}

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuCellCtrl)
	public:
	virtual BOOL OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult);
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL
private:

protected:
	int OFFSET_FIRST;
	int OFFSET_OTHER;

	BOOL        m_bFullRowSelected;
	BOOL        m_bLineSeperator;
	COLORREF    m_colorText;
	COLORREF    m_colorTextBk;
	COLORREF    m_colorHighLight;
	COLORREF    m_colorHighLightText;
	COLORREF    m_colorLine;
	
	CImageList m_image;
	CImageList m_imageList;
	CSize   m_ListBitmapSize;
	HBITMAP m_hListBitmap;
	int     m_cxClient;
	int     m_iTotalColumn;
	int     m_xBitmapSpace;
	//
	// In order to draw a check box in the column area, we use the following
	// information:
	// 
	// 1) Array of boolean (m_pCheckBox) to indicate if we should draw the check box.
	// 2) If m_pCheckBox[i] is TRUE then 
	//      - Draw the check box with check mark if the string is '$yes$'.
	//      - Draw the check box with uncheck mark if the string is '$no$'.
	//      - Draw the column as the normal column (ignore the check box)
	//
	// 3) m_pCheckImageList is a zero base index bitmap (0: uncheck, 1: check)
	//        Use the member SetCheckImageList (CImageList* pImageList) to set the check
	//        image list.

	BOOL* m_pCheckBox;
	CImageList* m_pImageList;
	
	LPCTSTR MakeShortString (CDC* pDC, LPCTSTR lpszLong, int nColumnLen, int nOffset);

	virtual void DrawItem (LPDRAWITEMSTRUCT lpDrawItemStruct);

	//{{AFX_MSG(CuCellCtrl)
	afx_msg void OnInsertitem(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	//}}AFX_MSG
	
	afx_msg void OnSysColorChange();
	
	DECLARE_MESSAGE_MAP()
};

#endif  // CELLCTRL_HEADER

