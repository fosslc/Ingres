/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : calsctrl.h,  header file
**    Project  : 
**    Author   : UK Sotheavut 
**    Purpose  : Owner draw LISTCTRL to have a vertical and a horizontal line
**               to seperate each item in the CListCtrl.
**
** History:
**
** xx-Jan-1996 (uk$so01)
**    Created
** 08-Nov-2000 (uk$so01)
**    Create library ctrltool.lib for more resuable of this control
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 24-Jun-2010 (drivi01)
**    Remove hardcoded column length, use DBOBJECTLEN defined 
**    in constdef.h which uses DB_MAXNAME from dbms headers.
**/

#ifndef CALSCTRL_HEADER
#define CALSCTRL_HEADER
extern "C"
{
#include "constdef.h"
#include <compat.h>
#include <iicommon.h>
}

#define XBITMAP_FOCUS           0
#define XBITMAP_BITMAP          1
#define XBITMAP_SELECTFOCUS     2
#define XBITMAP_SELECT          3

//
// If you need, this structure can provide the facility of sorting the column headers.
// You can pass this structure in the third parameter of the member SortItem of ListCtrl.
typedef struct tagSORTPARAMS
{
	BOOL m_bAsc;       // Ascending or Descending
	int  m_nItem;      // Item to compare. You can use this member to determine
	                   // which member in the ListCtrl's item data to be compared.
} SORTPARAMS;

#define MAXLSCTRL_HEADERLENGTH DBOBJECTLEN
typedef struct tagLSCTRLHEADERPARAMS
{
	TCHAR m_tchszHeader [MAXLSCTRL_HEADERLENGTH]; // Column's Title
	int   m_cxWidth;                                // Column's width
	int   m_fmt;                                    // Column's allignment
	BOOL  m_bUseCheckMark;                          // Column can have a check box.
} LSCTRLHEADERPARAMS;

typedef struct tagLSCTRLHEADERPARAMS2
{
	UINT  m_nIDS;             // Column's Title
	int   m_cxWidth;          // Column's width
	int   m_fmt;              // Column's allignment
	BOOL  m_bUseCheckMark;    // Column can have a check box.
} LSCTRLHEADERPARAMS2;


class CaListCtrlColorItemData
{
public:
	CaListCtrlColorItemData()
	{
		m_bDrawLine = FALSE;
		m_lUserData = 0;
		m_bUseFgRGB = FALSE;
		m_bUseBgRGB = FALSE;
		m_FgRGB     = GetSysColor(COLOR_WINDOWTEXT);
		m_BgRGB     = GetSysColor(COLOR_WINDOW);
	}
	CaListCtrlColorItemData(BOOL bDrawLine, LPARAM lUserData = 0): m_bDrawLine(bDrawLine), m_lUserData(lUserData)
	{
		m_bUseFgRGB = FALSE;
		m_bUseBgRGB = FALSE;
		m_FgRGB     = GetSysColor(COLOR_WINDOWTEXT);
		m_BgRGB     = GetSysColor(COLOR_WINDOW);
	}

	virtual ~CaListCtrlColorItemData(){}
	void SetFgColor (BOOL bUseRGB, COLORREF rgbForeground = NULL);
	void SetBgColor (BOOL bUseRGB, COLORREF rgbBackground = NULL);
	BOOL IsUseFgRGB(){return m_bUseFgRGB;}
	BOOL IsUseBgRGB(){return m_bUseBgRGB;}
	COLORREF GetFgRGB() {return m_FgRGB;}
	COLORREF GetBgRGB() {return m_BgRGB;}

	BOOL    m_bDrawLine;
	LPARAM  m_lUserData;

protected:
	BOOL    m_bUseFgRGB;
	BOOL    m_bUseBgRGB;
	COLORREF m_FgRGB;
	COLORREF m_BgRGB;
};


class CuListCtrl : public CListCtrl
{
public:
	CuListCtrl();
	virtual ~CuListCtrl();
	void SetFont(CFont* pFont, BOOL bRedraw = TRUE);
	int InsertColumn (int nCol, const LV_COLUMN* pColumn, BOOL bCheckBox = FALSE);
	int InsertColumn (int nCol, LPCTSTR lpszColumnHeading, 
	                  int nFormat   = LVCFMT_LEFT, 
	                  int nWidth    = -1, 
	                  int nSubItem  = -1,
	                  BOOL bCheckBox= FALSE);
	BOOL DeleteColumn(int nCol);
	int  GetTotalColumn (){return m_iTotalColumn;}; 
	BOOL GetCellRect (CRect rect, CPoint point, CRect& rCell, int& iCellNumber);
	BOOL GetCellRect (CRect rect, int col, CRect& rCell);

	void SetLineSeperator   (BOOL bLine = TRUE, BOOL bVertLine = TRUE, BOOL bHorzLine = TRUE)
	{
		m_bLineSeperator = bLine;
		m_bLineVertical  = bVertLine;
		m_bLineHorizontal= bHorzLine;
	}
	void SetFullRowSelected (BOOL bFullRow = TRUE) {m_bFullRowSelected = FALSE;}
	BOOL GetFullRowSelected () {return m_bFullRowSelected;}
	void SetCheckImageList (CImageList* pImageList){m_pImageList = pImageList;}
	
	BOOL GetCheck   (int nItem, int nSubItem);
	void SetCheck   (int nItem, int nSubItem, BOOL bCheck = TRUE);
	void EnableCheck(int nItem, int nSubItem, BOOL bEnable= TRUE);

	//
	// Color Item ->
	// You should call ActivateColorItem() before using color item:
	void  ActivateColorItem(){m_bActivateColorItem = TRUE;}
	void  DrawUnderLine(int nItem, BOOL bLine = TRUE);
	BOOL  SetItem (LV_ITEM* pItem);
	BOOL  SetItem (int nItem, int nSubItem, UINT nMask, LPCTSTR lpszItem, int nImage, UINT nState, UINT nStateMask, LPARAM lParam );
	BOOL  SetItemData (int nItem, DWORD dwData);
	DWORD GetItemData (int nItem ) const;
	void  SetFgColor  (int iIndex, COLORREF rgbForeground);
	void  SetBgColor  (int iIndex, COLORREF rgbBackground);
	void  UnSetFgColor(int iIndex);
	void  UnSetBgColor(int iIndex);

	int FindItem(LV_FINDINFO* pFindInfo, int nStart = -1) const;
	// Color <-

	void SetAllowSelect (BOOL bAllow){m_bAllowSelect = bAllow;}
	virtual void OnCheckChange (int iItem, int iSubItem, BOOL bCheck);

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuListCtrl)
	public:
	virtual BOOL OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult);
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL
protected:
	CString m_strCheck;
	CString m_strUncheck;
	CString m_strCheckDisable;
	CString m_strUncheckDisable;
protected:
	int OFFSET_FIRST;
	int OFFSET_OTHER;

	BOOL        m_bFullRowSelected;
	BOOL        m_bLineSeperator;
	BOOL        m_bLineVertical;
	BOOL        m_bLineHorizontal;

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
	BOOL m_bAllowSelect;
	BOOL m_bActivateColorItem;

	virtual CString MakeShortString (CDC* pDC, LPCTSTR lpszLong, int nColumnLen, int nOffset);
	void RepaintSelectedItems ();
	void PredrawItem (LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual void DrawItem (LPDRAWITEMSTRUCT lpDrawItemStruct);
	
	//{{AFX_MSG(CuListCtrl)
	afx_msg void OnInsertitem(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPaint();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	
	afx_msg void OnSysColorChange();
	afx_msg void OnDeleteallitems (NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeleteitem     (NMHDR* pNMHDR, LRESULT* pResult);
	
	DECLARE_MESSAGE_MAP()
};

//
// If ncolMask = 0 then the default mask is set to LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH
BOOL InitializeHeader (CuListCtrl* pListCtrl, UINT ncolMask, LSCTRLHEADERPARAMS*  aHeader, int nHeaderSize);
BOOL InitializeHeader2(CuListCtrl* pListCtrl, UINT ncolMask, LSCTRLHEADERPARAMS2* aHeader, int nHeaderSize);

#endif
