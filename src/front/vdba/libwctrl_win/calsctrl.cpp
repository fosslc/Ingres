/*
**  Copyright (C) 2005-2006 Ingres Corporation.
*/

/*
**    Source   : calsctrl.cpp, implementation file
**    Project  : CA-OpenIngres/Monitoring
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Owner draw LISTCTRL to have a vertical and a horizontal line 
**               to seperate each item in the CListCtrl.
**
** History:
**
** xx-Jan-1996 (uk$so01)
**    Created.
** 23-May-2000 (uk$so01)
**    bug 99242 (DBCS) modify the functions: MakeShortString()
** 08-Nov-2000 (uk$so01)
**    Create library ctrltool.lib for more resuable of this control
**    Add functionality for color item management.
** 26-Jul-2001 (hanje04)
**    Added check condition for nColumnLen=0 in MakeShortString
**    to exit and return szThreeDots to avoid loop condition and eventual
**    SIGILL around _tcsdec on UNIX.
** 01-nov-2001 (somsa01)
**    Cleaned up 64-bit compiler warnings.
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 31-Jul-2002 (hanch04)
**    Mainwin needs to have the rows printed 1 pixel higher.
**    Could be that the font is not the same?
** 28-Feb-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
**    And integrate modif on 01-nov-2001 (somsa01) Cleaned up 64-bit compiler warnings.
**    and 26-Jul-2001 (hanje04)
** 07-Mar-2003 (uk$so01)
**    SIR #109773, Add property frame for displaying the description 
**    of ingres messages.
** 01-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (vcbf uses libwctrl.lib).
** 08-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (vdba uses libwctrl.lib).
** 02-feb-2004 (somsa01)
**    Corrected ifdef.
** 11-feb-2004 (noifr01)
**    fixed a paint problem that appeared when building with recent version(s)
**    of the compiler
**/

#include "stdafx.h"
#include "calsctrl.h"
#include "wmusrmsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
/*
#define OFFSET_FIRST    2
#define OFFSET_OTHER    6
*/

BOOL InitializeHeader (CuListCtrl* pListCtrl, UINT ncolMask, LSCTRLHEADERPARAMS*  aHeader, int nHeaderSize)
{
	LVCOLUMN lvcolumn;
	memset (&lvcolumn, 0, sizeof (LVCOLUMN));
	lvcolumn.mask = (ncolMask > 0)? ncolMask: LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
	for (int i=0; i<nHeaderSize; i++)
	{
		lvcolumn.fmt      = aHeader[i].m_fmt;
		lvcolumn.pszText  = (LPTSTR)aHeader[i].m_tchszHeader;
		lvcolumn.iSubItem = i;
		lvcolumn.cx       = aHeader[i].m_cxWidth;
		pListCtrl->InsertColumn (i, &lvcolumn, aHeader[i].m_bUseCheckMark);
	}

	return TRUE;
}

BOOL InitializeHeader2(CuListCtrl* pListCtrl, UINT ncolMask, LSCTRLHEADERPARAMS2* aHeader, int nHeaderSize)
{
	CString strHeader;
	LVCOLUMN lvcolumn;
	memset (&lvcolumn, 0, sizeof (LVCOLUMN));
	lvcolumn.mask = (ncolMask > 0)? ncolMask: LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
	for (int i=0; i<nHeaderSize; i++)
	{
		strHeader.LoadString (aHeader[i].m_nIDS);
		lvcolumn.fmt      = aHeader[i].m_fmt;
		lvcolumn.pszText  = (LPTSTR)(LPCTSTR)strHeader;
		lvcolumn.iSubItem = i;
		lvcolumn.cx       = aHeader[i].m_cxWidth;
		pListCtrl->InsertColumn (i, &lvcolumn, aHeader[i].m_bUseCheckMark);
	}
	return TRUE;
}



void CaListCtrlColorItemData::SetFgColor (BOOL bUseRGB, COLORREF rgbForeground)
{
	m_bUseFgRGB = bUseRGB;
	m_FgRGB     = rgbForeground;
}

void CaListCtrlColorItemData::SetBgColor (BOOL bUseRGB, COLORREF rgbBackground)
{
	m_bUseBgRGB = bUseRGB;
	m_BgRGB     = rgbBackground;
}


CString CuListCtrl::MakeShortString (CDC* pDC, LPCTSTR lpszLong, int nColumnLen, int nOffset)
{
	static const _TCHAR szThreeDots[] = _T("...");
	int nStringLen = (int)_tcslen(lpszLong);
	if (nColumnLen <= 0)
		return szThreeDots;
	if(nStringLen==0 || pDC->GetTextExtent(lpszLong,nStringLen).cx+nOffset <= nColumnLen)
		return lpszLong;

	CString strShort = lpszLong;
	TCHAR* pszShort = (LPTSTR)(LPCTSTR)strShort;
	int nAddLen = pDC->GetTextExtent (szThreeDots, sizeof(szThreeDots)).cx;
	
	TCHAR* ptchszShort = _tcsdec(pszShort, pszShort + (int)_tcslen(pszShort));
	int nCx = pDC->GetTextExtent (pszShort, (int)_tcslen(pszShort)).cx;
	BOOL bShort = (nCx + nOffset + nAddLen <= nColumnLen);

	while (!bShort && pszShort && pszShort[0] != _T('\0'))
	{
		ptchszShort = _tcsdec(pszShort, ptchszShort);
		if (!ptchszShort)
			break;
		*ptchszShort= _T('\0');
		nCx = pDC->GetTextExtent (pszShort, (int)_tcslen(pszShort)).cx;
		bShort = (nCx + nOffset + nAddLen <= nColumnLen);
	}

	CString strResult = pszShort;
	strResult += szThreeDots;
	return strResult;
}


void CuListCtrl::RepaintSelectedItems ()
{
	CRect rcItem;
	CRect rcLabel;
	
	int nItem  = GetNextItem (-1,LVNI_FOCUSED);
	if (nItem !=-1)
	{
		GetItemRect (nItem, rcItem,  LVIR_BOUNDS);
		GetItemRect (nItem, rcLabel, LVIR_LABEL);
		rcItem.left = rcLabel.left;
		InvalidateRect (rcItem,FALSE);
	}
	
	if(!(GetStyle() & LVS_SHOWSELALWAYS))
	{
		for (nItem = GetNextItem (-1, LVNI_SELECTED); nItem !=-1; nItem = GetNextItem (nItem, LVNI_SELECTED))
		{
			GetItemRect (nItem, rcItem,  LVIR_BOUNDS);
			GetItemRect (nItem, rcLabel, LVIR_LABEL);
			rcItem.left = rcLabel.left;
			InvalidateRect (rcItem, FALSE);
		}
	}
	UpdateWindow();
}



CuListCtrl::CuListCtrl()
{
	TCHAR tchszNo  []  = {(TCHAR)0x18, (TCHAR)0xAE, (TCHAR)0xAB, _T('n'), _T('o'), (TCHAR)0xBB, (TCHAR)0xAE, (TCHAR)0x00};
	TCHAR tchszYes []  = {(TCHAR)0x18, (TCHAR)0xAE, (TCHAR)0xAB, _T('y'), _T('e'), _T('s'), (TCHAR)0xBB, (TCHAR)0xAE, (TCHAR)0x00};
	TCHAR tchszNoD []  = {(TCHAR)0x18, (TCHAR)0xAE, (TCHAR)0xAB, _T('n'), _T('o'), _T('d'), (TCHAR)0xBB, (TCHAR)0xAE, (TCHAR)0x00};
	TCHAR tchszYesD[]  = {(TCHAR)0x18, (TCHAR)0xAE, (TCHAR)0xAB, _T('y'), _T('e'), _T('s'), _T('d'), (TCHAR)0xBB, (TCHAR)0xAE, (TCHAR)0x00};

	OFFSET_FIRST = 2;
	OFFSET_OTHER = 6;

	m_pImageList = NULL;
	m_pCheckBox  = NULL;
	m_bFullRowSelected  = FALSE;
	m_bLineSeperator    = TRUE;
	m_colorText         = ::GetSysColor (COLOR_WINDOWTEXT);
	m_colorTextBk       = ::GetSysColor (COLOR_WINDOW);
	m_colorHighLight    = ::GetSysColor (COLOR_HIGHLIGHT);
	m_colorHighLightText= ::GetSysColor (COLOR_HIGHLIGHTTEXT);
	m_colorLine         = RGB (192, 192, 192);
	if (m_colorLine == m_colorTextBk)
		m_colorLine = ::GetSysColor (COLOR_GRAYTEXT); 
	m_iTotalColumn  = 0;
	m_xBitmapSpace  = 0;
	m_hListBitmap   = NULL;

	m_strCheck          = tchszYes;
	m_strUncheck        = tchszNo;
	m_strCheckDisable   = tchszYesD;
	m_strUncheckDisable = tchszNoD;

	m_bAllowSelect = TRUE;
	m_bActivateColorItem = FALSE;

	//
	// Only if m_bLineSeperator = TRUE.
	m_bLineVertical = TRUE;
	m_bLineHorizontal = TRUE;
}

CuListCtrl::~CuListCtrl()
{
	if (m_pCheckBox)
		delete [] m_pCheckBox;
}

void CuListCtrl::SetFont(CFont* pFont, BOOL bRedraw)
{
	CListCtrl::SetFont(pFont, bRedraw);
	CImageList* pImageList = GetImageList(LVSIL_SMALL);
	//
	// BUG: if the control does not have the imagelist, then the GetItemRect won't give
	//      the heigh according to the font's size.
	if (pImageList)
	{
		SetImageList (pImageList, LVSIL_SMALL);
	}
}


BOOL CuListCtrl::GetCheck (int nItem, int nSubItem)
{
	ASSERT (m_pCheckBox && m_pCheckBox[nSubItem]);
	CString strText = GetItemText (nItem, nSubItem);
	return (strText == m_strCheck || strText == m_strCheckDisable)? TRUE: FALSE;
}


void CuListCtrl::SetCheck (int nItem, int nSubItem, BOOL bCheck)
{
	ASSERT (m_pCheckBox && m_pCheckBox[nSubItem]);
	if (bCheck)
		SetItemText (nItem, nSubItem, m_strCheck);
	else
		SetItemText (nItem, nSubItem, m_strUncheck);
}

void CuListCtrl::EnableCheck(int nItem, int nSubItem, BOOL bEnable)
{
	ASSERT (m_pCheckBox && m_pCheckBox[nSubItem]);
	CString strText = GetItemText (nItem, nSubItem);
	if (strText == m_strCheck)
		SetItemText (nItem, nSubItem, (bEnable)? m_strCheck: m_strCheckDisable);
	else
	if (strText == m_strUncheck)
		SetItemText (nItem, nSubItem, (bEnable)? m_strUncheck: m_strUncheckDisable);
}


BEGIN_MESSAGE_MAP(CuListCtrl, CListCtrl)
	//{{AFX_MSG_MAP(CuListCtrl)
	ON_NOTIFY_REFLECT(LVN_INSERTITEM, OnInsertitem)
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_KEYUP()
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
	ON_WM_SYSCOLORCHANGE()
	ON_NOTIFY_REFLECT (LVN_INSERTITEM,     OnInsertitem)
	ON_NOTIFY_REFLECT (LVN_DELETEALLITEMS, OnDeleteallitems)
	ON_NOTIFY_REFLECT (LVN_DELETEITEM,     OnDeleteitem)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuListCtrl message handlers


void CuListCtrl::DrawItem (LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CString  pszItem;
	CString  strItem;
	COLORREF colorTextOld;
	COLORREF colorBkOld;
	
	CDC* pDC = CDC::FromHandle (lpDrawItemStruct->hDC);
	if ((int)lpDrawItemStruct->itemID < 0)
		return;
	int  nItem     = lpDrawItemStruct->itemID;
	BOOL bFocus    = GetFocus() == this;
	BOOL bSelected = (bFocus || (GetStyle() & LVS_SHOWSELALWAYS)) && (GetItemState(nItem, LVIS_SELECTED) & LVIS_SELECTED);
	
	CRect rcItem;
	CRect rcAllLabels;
	CRect rcLabel;
	GetItemRect (nItem, rcAllLabels, LVIR_BOUNDS);
	GetItemRect (nItem, rcLabel,     LVIR_LABEL);
	rcAllLabels.left = rcLabel.left;
	GetItemRect (nItem, rcItem, LVIR_LABEL);

	if (m_bFullRowSelected)
	{
		if (rcAllLabels.right < m_cxClient)
			rcAllLabels.right = m_cxClient;
	}
	CRect r = rcAllLabels;
	if (m_bLineSeperator)
		r.top++;
	
	CBrush brushSelect(m_colorHighLight);
	CBrush brushNormal(m_colorTextBk);

	if (bSelected)
	{
		colorTextOld = pDC->SetTextColor (m_colorHighLightText);
		colorBkOld   = pDC->SetBkColor   (m_colorHighLight);
		pDC->FillRect (r, &brushSelect);
	}
	else
	{
		colorTextOld = pDC->SetTextColor (m_colorText);
		pDC->FillRect (r, &brushNormal);
	}

	//
	// Get item data
	CImageList* pImageList;
	UINT uiFlags = ILD_TRANSPARENT;
	COLORREF clrImage = m_colorTextBk;

	LV_ITEM  lvi;
	lvi.mask  = LVIF_IMAGE | LVIF_STATE|LVIF_PARAM;
	lvi.iItem = lpDrawItemStruct->itemID;
	lvi.iSubItem = 0;
	lvi.stateMask = 0xFFFF; // Get all state flags
	CListCtrl::GetItem(&lvi);
	CaListCtrlColorItemData* pItemData = NULL;

	if (bSelected)
	{
		colorTextOld = pDC->SetTextColor (m_colorHighLightText);
		colorBkOld   = pDC->SetBkColor   (m_colorHighLight);
		pDC->FillRect (r, &brushSelect);
	}
	else
	{
		if (m_bActivateColorItem)
			pItemData = (CaListCtrlColorItemData*)lvi.lParam;

		if (pItemData && pItemData->IsUseFgRGB())
			colorTextOld = pDC->SetTextColor (pItemData->GetFgRGB());
		else
			colorTextOld = pDC->SetTextColor (m_colorText);

		if (pItemData && pItemData->IsUseBgRGB())
		{
			colorBkOld   = pDC->SetBkColor   (pItemData->GetBgRGB());
			brushNormal.DeleteObject();
			brushNormal.CreateSolidBrush(pItemData->GetBgRGB());
			//CBrush brushBk(pData->GetBgRGB());
			pDC->FillRect (r, &brushNormal);
		}
		else
			pDC->FillRect (r, &brushNormal);
	}


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

	//
	// Draw state icon
	UINT nStateImageMask = lvi.state & LVIS_STATEIMAGEMASK;
	if (nStateImageMask)
	{
		CRect rcItemState = CRect(lpDrawItemStruct->rcItem);
		int nImage = (nStateImageMask>>12) - 1;
		pImageList = CListCtrl::GetImageList(LVSIL_STATE);
		if (pImageList)
		{
			pImageList->Draw(pDC, nImage, CPoint(rcItemState.left, rcItemState.top), ILD_TRANSPARENT);
		}
	}

	//
	// Draw normal and overlay icon
	int heightImg = 16;
	int widthImg  = 16;
	CRect rcIcon;
	CListCtrl::GetItemRect(nItem, rcIcon, LVIR_ICON);
	pDC->FillRect (rcIcon, &brushNormal);

	pImageList = CListCtrl::GetImageList(LVSIL_SMALL);
	if (pImageList)
	{
		UINT nOvlImageMask=lvi.state & LVIS_OVERLAYMASK;
		if (rcItem.left<rcItem.right-1)
		{
			// Use height of bitmap inside image list.
			// images must be centered in the imagelist bounding rectangle.
			IMAGEINFO imgInfo;
			if (pImageList->GetImageInfo(0, &imgInfo))
			{
				heightImg = imgInfo.rcImage.bottom - imgInfo.rcImage.top;
				widthImg  = imgInfo.rcImage.right  - imgInfo.rcImage.left;
			}
			int y1 = rcIcon.top + (rcIcon.Height() - heightImg)/2;
			ImageList_DrawEx (
				pImageList->m_hImageList, 
				lvi.iImage, 
				pDC->m_hDC,
				rcIcon.left, 
				y1, 
				widthImg, 
				heightImg, 
				m_colorTextBk, 
				clrImage, 
				uiFlags | nOvlImageMask);
		}
	}

	strItem = GetItemText (nItem, 0);
	pszItem = MakeShortString (pDC, strItem, rcItem.right - rcItem.left, 2*OFFSET_OTHER);
	//
	// Draw the Main Item Label.
	BOOL   bDrawCheck = FALSE;
	int    iCheck     = 0; // (0 uncheck), (1 check), (2 uncheck grey), (3 check grey)
	CPoint checkPos;
	LV_COLUMN lvc; 
	memset (&lvc, 0, sizeof (lvc));
	lvc.mask = LVCF_FMT | LVCF_WIDTH;
	if (CListCtrl::GetColumn (0, &lvc))
	{
		if (m_pCheckBox && m_pCheckBox[0])
		{
			bDrawCheck = TRUE;
			if (strItem.CompareNoCase (m_strUncheck) == 0)
				iCheck = 0;
			else
			if (strItem.CompareNoCase (m_strCheck) == 0)
				iCheck = 1;
			else
			if (strItem.CompareNoCase (m_strCheckDisable) == 0)
				iCheck = 3;
			else
			if (strItem.CompareNoCase (m_strUncheckDisable) == 0)
				iCheck = 2;
			else
				bDrawCheck = FALSE;
		}
	}
	rcLabel = rcItem;
	if (bSelected)
		pDC->FillRect (rcLabel, &brushSelect);
	else
		pDC->FillRect (rcLabel, &brushNormal);
	rcLabel.left  += OFFSET_FIRST;
	rcLabel.right -= OFFSET_FIRST;
		
	UINT nJustify = DT_LEFT;
	checkPos.x = rcLabel.left; 
	checkPos.y = rcLabel.top;

	UINT nDt = lvc.fmt & LVCFMT_JUSTIFYMASK;
	switch(nDt)
	{
	case LVCFMT_RIGHT:
		if (!bDrawCheck)
			nJustify = (strItem == pszItem)? DT_RIGHT: DT_LEFT;
		else
			nJustify = LVCFMT_RIGHT;
		checkPos.x = rcLabel.right - 16; 
		checkPos.y = rcLabel.top;
		break;
	case LVCFMT_CENTER:
		if (!bDrawCheck)
			nJustify = (strItem == pszItem)? DT_CENTER: DT_LEFT;
		else
			nJustify = DT_CENTER;
		checkPos.x = rcLabel.left + rcLabel.Width()/2 - 8; 
		checkPos.y = rcLabel.top;
		break;
	default:
		break;
	}

	UINT nMaskImg = ILD_TRANSPARENT;
	if (!bDrawCheck)
	{
#if defined(MAINWIN)
		CRect rcmwLabel = rcLabel;
		rcmwLabel.top    -= 1; 
		pDC->DrawText (pszItem, rcmwLabel, nJustify|DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER);
#else
		pDC->DrawText (pszItem, rcLabel, nJustify|DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER);
#endif
	}
	else
	{
		m_pImageList->Draw(pDC, iCheck, checkPos, nMaskImg);
	}

	CPen  penGray (PS_SOLID, 1, m_colorLine);
	CPen* pOldPen = pDC->SelectObject (&penGray);
	//
	// Draw Vertical line:
	if (m_bLineSeperator && m_bLineVertical)
	{
		pDC->MoveTo (rcItem.right-2, rcItem.top);
		pDC->LineTo (rcItem.right-2, rcItem.bottom);
	}

	for (int nColumn=1; CListCtrl::GetColumn (nColumn, &lvc); nColumn++)
	{
		bDrawCheck = FALSE;
		rcItem.left   = rcItem.right;
		rcItem.right += lvc.cx;
		strItem = GetItemText (nItem, nColumn);
		pszItem = MakeShortString (pDC, strItem, rcItem.right - rcItem.left, 2*OFFSET_OTHER);
		if (m_pCheckBox && m_pCheckBox[nColumn])
		{
			bDrawCheck = TRUE;
			if (strItem.CompareNoCase (m_strUncheck) == 0)
				iCheck = 0;
			else
			if (strItem.CompareNoCase (m_strCheck) == 0)
				iCheck = 1;
			else
			if (strItem.CompareNoCase (m_strCheckDisable) == 0)
				iCheck = 3;
			else
			if (strItem.CompareNoCase (m_strUncheckDisable) == 0)
				iCheck = 2;
			else
				bDrawCheck = FALSE;
		}
		
		nJustify = DT_LEFT;
		checkPos.x = rcItem.left; 
		checkPos.y = rcItem.top;
		switch(lvc.fmt & LVCFMT_JUSTIFYMASK)
		{
		case LVCFMT_RIGHT:
			if (!bDrawCheck)
				nJustify = (strItem == pszItem)? DT_RIGHT: DT_LEFT;
			else
				nJustify = LVCFMT_RIGHT;
			checkPos.x = rcItem.right - 16; 
			checkPos.y = rcItem.top;
			break;
		case LVCFMT_CENTER:
			if (!bDrawCheck)
				nJustify = (strItem == pszItem)? DT_CENTER: DT_LEFT;
			else
				nJustify = LVCFMT_CENTER;
			checkPos.x = rcItem.left + rcItem.Width()/2 - 8; 
			checkPos.y = rcItem.top;
			break;
		default:
			break;
		}

		if (bDrawCheck)
		{
			m_pImageList->Draw(pDC, iCheck, checkPos, nMaskImg);
		}
		else
		if (strItem.GetLength() > 0) 
		{
			rcLabel = rcItem;
			if (bSelected)
				pDC->FillRect (rcLabel, &brushSelect);
			else
				pDC->FillRect (rcLabel, &brushNormal);
			rcLabel.left  += OFFSET_FIRST;
			rcLabel.right -= OFFSET_FIRST;
#if defined(MAINWIN)
			CRect rcmwLabel = rcLabel;
			rcmwLabel.top    -= 1; 
			pDC->DrawText (pszItem, rcmwLabel, nJustify|DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER);
#else
			pDC->DrawText (pszItem, rcLabel, nJustify|DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER);
#endif
		}
		if (m_bLineSeperator && m_bLineVertical)
		{
			pDC->MoveTo (rcItem.right-1, rcItem.top);
			pDC->LineTo (rcItem.right-1, rcItem.bottom);
		}
	}
	if (m_bLineSeperator && m_bLineHorizontal)
	{
		pDC->MoveTo (rcAllLabels.left - rcIcon.right,  rcAllLabels.top);
		pDC->LineTo (rcAllLabels.right, rcAllLabels.top);
		pDC->MoveTo (rcAllLabels.left - rcIcon.right,  rcAllLabels.bottom);
		pDC->LineTo (rcAllLabels.right, rcAllLabels.bottom);
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
}


void CuListCtrl::PredrawItem (LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CDC* pDC = CDC::FromHandle (lpDrawItemStruct->hDC);
	DRAWITEMSTRUCT drawItem;
	memcpy (&drawItem, lpDrawItemStruct, sizeof (DRAWITEMSTRUCT));
	
	LV_ITEM lvitem;
	memset (&lvitem, 0, sizeof (LV_ITEM));
	lvitem.iItem = lpDrawItemStruct->itemID;
	CRect rx;
	GetClientRect (rx);
	rx.left = lpDrawItemStruct->rcItem.right;
	InvalidateRect (rx);
	
	
	UINT state = GetItemState (lpDrawItemStruct->itemID, LVIS_SELECTED|LVIS_FOCUSED);
	if (drawItem.itemID >= 0)
	{ 
		//int dy;
		//int whichBitmap;
	
	}
	if (((drawItem.itemID) >= 0) && 
		((drawItem.itemAction & (ODA_DRAWENTIRE|ODA_SELECT|ODA_FOCUS)) != 0))
	{
		DrawItem (&drawItem);
	}
}

BOOL CuListCtrl::OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult) 
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


int CuListCtrl::InsertColumn (int nCol, const LV_COLUMN* pColumn, BOOL bCheckBox)
{
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

int CuListCtrl::InsertColumn (int nCol, LPCTSTR lpszColumnHeading, int nFormat, int nWidth, int nSubItem, BOOL bCheckBox)
{
	if (bCheckBox)
	{
		//
		// You must specify the check image list
		ASSERT (m_pImageList != NULL);
	}
	int i, index = CListCtrl::InsertColumn (nCol, lpszColumnHeading, nFormat, nWidth, nSubItem);
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

BOOL CuListCtrl::DeleteColumn(int nCol)
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


BOOL CuListCtrl::PreCreateWindow(CREATESTRUCT& cs) 
{
	cs.style &= ~LVS_TYPEMASK;
	cs.style |= LVS_REPORT | LVS_OWNERDRAWFIXED | LVS_SHOWSELALWAYS;    
	m_bFullRowSelected = TRUE;
	return CListCtrl::PreCreateWindow(cs);
}

BOOL CuListCtrl::GetCellRect (CRect rect, CPoint point, CRect& rCell, int& iCellNumber)
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
				CImageList* pImageList = CListCtrl::GetImageList(LVSIL_SMALL);
				if (pImageList)
				{
					CRect rcIcon;
					CListCtrl::GetItemRect(i, rcIcon, LVIR_ICON);
					rCell.left = xStarted-1 + rcIcon.Width();
				}
				else
				{
					rCell.left = xStarted-1;
				}
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

BOOL CuListCtrl::GetCellRect (CRect rect, int col, CRect& rCell)
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
				CImageList* pImageList = CListCtrl::GetImageList(LVSIL_SMALL);
				if (pImageList)
				{
					CRect rcIcon;
					CListCtrl::GetItemRect(i, rcIcon, LVIR_ICON);
					rCell.left = xStarted-1 + rcIcon.Width();
				}
				else
				{
					rCell.left = xStarted-1;
				}
			}
			else
				rCell.left = xStarted-1;
			rCell.right= xEnded;
			return TRUE;
		}
	}
	return FALSE;
}


void CuListCtrl::OnSize(UINT nType, int cx, int cy) 
{
	m_cxClient = cx;
	CListCtrl::OnSize(nType, cx, cy);
}

void CuListCtrl::OnSysColorChange() 
{
	CListCtrl::OnSysColorChange();
	m_colorText         = ::GetSysColor (COLOR_WINDOWTEXT);
	m_colorTextBk       = ::GetSysColor (COLOR_WINDOW);
	m_colorHighLight    = ::GetSysColor (COLOR_HIGHLIGHT);
	m_colorHighLightText= ::GetSysColor (COLOR_HIGHLIGHTTEXT);
	if (m_colorTextBk == RGB (192, 192, 192))
		m_colorLine = RGB (0, 0, 0);
	else
		m_colorLine = RGB (192, 192, 192);
}

void CuListCtrl::OnPaint() 
{
	CListCtrl::OnPaint();
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
}

void CuListCtrl::OnSetFocus(CWnd* pOldWnd) 
{
	CListCtrl::OnSetFocus(pOldWnd);
	if (pOldWnd != NULL && pOldWnd->GetParent() == this)
		return;
	if (m_bFullRowSelected || (GetStyle() & LVS_REPORT))
		RepaintSelectedItems();
}

void CuListCtrl::OnKillFocus(CWnd* pNewWnd) 
{
	CListCtrl::OnKillFocus(pNewWnd);
	if (pNewWnd != NULL && pNewWnd->GetParent() == this)
		return;
	if (m_bFullRowSelected || (GetStyle() & LVS_REPORT))
		RepaintSelectedItems();
}



void CuListCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
{
	//
	// Test if the Mouse Pointer is in the check box column area.
	int   index, iColumnNumber;
	CRect rect, rCell;
	UINT  flags, nJustify = DT_LEFT;
	
	index = HitTest (point, &flags);
	if (index < 0)
		return;
	GetItemRect (index, rect, LVIR_BOUNDS);
	if (!GetCellRect (rect, point, rCell, iColumnNumber))
		return;
	if (m_bAllowSelect)
		CListCtrl::OnLButtonDown(nFlags, point);
	else
		return;

	if (m_pCheckBox && m_pCheckBox[iColumnNumber])
	{
		CRect rx = rCell;
		LV_COLUMN lvc; 
		memset (&lvc, 0, sizeof (lvc));
		lvc.mask = LVCF_FMT | LVCF_WIDTH;
		CListCtrl::GetColumn (iColumnNumber, &lvc);	
		switch(lvc.fmt & LVCFMT_JUSTIFYMASK)
		{
		case LVCFMT_RIGHT:
			rx.left  = rx.right - 11;
			rx.top   = rx.top  + rCell.Height()/2 - 6;
			rx.bottom= rx.top  + 11;
			break;
		case LVCFMT_CENTER:
			rx.left  = rx.left + rCell.Width() /2 - 6;
			rx.right = rx.left + 11;
			rx.top   = rx.top  + rCell.Height()/2 - 6;
			rx.bottom= rx.top  + 11;
			break;
		default:
			rx.right = rx.left + 11+3;
			rx.top   = rx.top  + rCell.Height()/2 - 6;
			rx.bottom= rx.top  + 11;
			break;
		}

		if (!rx.PtInRect(point))
			return;
		CString strItem = GetItemText (index, iColumnNumber);
		if (strItem.CompareNoCase (m_strUncheck) == 0)
		{
			strItem = m_strCheck;
			SetItemText (index, iColumnNumber, (LPTSTR)(LPCTSTR)strItem);
			OnCheckChange (index, iColumnNumber, TRUE);
		}
		else
		if (strItem.CompareNoCase (m_strCheck) == 0)
		{
			strItem = m_strUncheck;
			SetItemText (index, iColumnNumber, (LPTSTR)(LPCTSTR)strItem);
			OnCheckChange (index, iColumnNumber, FALSE);
		}
		else
		{
			SetItemText (index, iColumnNumber, (LPTSTR)(LPCTSTR)strItem);
		}
	}
}

void CuListCtrl::OnRButtonDown(UINT nFlags, CPoint point) 
{
	int   index, iColumnNumber;
	CRect rect, rCell;
	UINT  flags, nJustify = DT_LEFT;
	
	index = HitTest (point, &flags);
	if (index < 0)
		return;
	GetItemRect (index, rect, LVIR_BOUNDS);
	if (!GetCellRect (rect, point, rCell, iColumnNumber))
		return;
	CListCtrl::OnRButtonDown(nFlags, point);
}

void CuListCtrl::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	int   index, iColumnNumber;
	CRect rect, rCell;
	UINT  flags, nJustify = DT_LEFT;
	
	index = HitTest (point, &flags);
	if (index < 0)
		return;
	GetItemRect (index, rect, LVIR_BOUNDS);
	if (!GetCellRect (rect, point, rCell, iColumnNumber))
		return;
	CListCtrl::OnLButtonDblClk(nFlags, point);
}

void CuListCtrl::OnRButtonDblClk(UINT nFlags, CPoint point) 
{
	int   index, iColumnNumber;
	CRect rect, rCell;
	UINT  flags, nJustify = DT_LEFT;
	
	index = HitTest (point, &flags);
	if (index < 0)
		return;
	GetItemRect (index, rect, LVIR_BOUNDS);
	if (!GetCellRect (rect, point, rCell, iColumnNumber))
		return;
	CListCtrl::OnRButtonDblClk(nFlags, point);
}


void CuListCtrl::OnCheckChange (int iItem, int iSubItem, BOOL bCheck)
{

}

//
// Color Item ->
void CuListCtrl::OnInsertitem(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	try
	{
		if (m_bActivateColorItem)
		{
			CaListCtrlColorItemData* pData = new CaListCtrlColorItemData();
			pData->m_lUserData = pNMListView->lParam;
			CListCtrl::SetItemData (pNMListView->iItem, (DWORD)pData);
		}
	}
	catch (...)
	{
		CString strMsg = "Internal error\nCuListCtrl::OnInsertitem, Cannot Allocate Data";
		AfxMessageBox (strMsg);
	}
	*pResult = 0;
}

void CuListCtrl::OnDeleteallitems(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	*pResult = 0;
}

void CuListCtrl::OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	if (m_bActivateColorItem)
	{
		CaListCtrlColorItemData* pData = (CaListCtrlColorItemData*)pNMListView->lParam;
		if (pData)
			delete pData;
	}

	*pResult = 0;
}

void CuListCtrl::SetFgColor (int iIndex, COLORREF rgbForeground)
{
	//
	// You should call ActivateColorItem() first, after you have
	// constructed your object:
	ASSERT(m_bActivateColorItem);
	if (!m_bActivateColorItem)
		return;
	ASSERT (iIndex != -1);
	CaListCtrlColorItemData* pData = (CaListCtrlColorItemData*)CListCtrl::GetItemData (iIndex);
	ASSERT (pData);
	if (pData)
	{
		pData->SetFgColor (TRUE, rgbForeground);
	}
}

void CuListCtrl::SetBgColor (int iIndex, COLORREF rgbBackground)
{
	//
	// You should call ActivateColorItem() first, after you have
	// constructed your object:
	ASSERT(m_bActivateColorItem);
	if (!m_bActivateColorItem)
		return;
	ASSERT (iIndex != -1);
	CaListCtrlColorItemData* pData = (CaListCtrlColorItemData*)CListCtrl::GetItemData (iIndex);
	ASSERT (pData);
	if (pData)
	{
		pData->SetBgColor (TRUE, rgbBackground);
	}
}

void  CuListCtrl::UnSetFgColor(int iIndex)
{
	//
	// You should call ActivateColorItem() first, after you have
	// constructed your object:
	ASSERT(m_bActivateColorItem);
	if (!m_bActivateColorItem)
		return;
	ASSERT (iIndex != -1);
	CaListCtrlColorItemData* pData = (CaListCtrlColorItemData*)CListCtrl::GetItemData (iIndex);
	ASSERT (pData);
	if (pData)
	{
		pData->SetBgColor (FALSE, GetSysColor (COLOR_WINDOWTEXT));
	}
}


void  CuListCtrl::UnSetBgColor(int iIndex)
{
	//
	// You should call ActivateColorItem() first, after you have
	// constructed your object:
	ASSERT(m_bActivateColorItem);
	if (!m_bActivateColorItem)
		return;
	ASSERT (iIndex != -1);
	CaListCtrlColorItemData* pData = (CaListCtrlColorItemData*)CListCtrl::GetItemData (iIndex);
	ASSERT (pData);
	if (pData)
	{
		pData->SetBgColor (FALSE, GetSysColor (COLOR_WINDOW));
	}
}

void CuListCtrl::DrawUnderLine(int nItem, BOOL bLine)
{
	//
	// You should call ActivateColorItem() first, after you have
	// constructed your object:
	ASSERT(m_bActivateColorItem);
	if (!m_bActivateColorItem)
		return;
	CaListCtrlColorItemData* pData = (CaListCtrlColorItemData*)CListCtrl::GetItemData (nItem);
	if (pData)
	{
		pData->m_bDrawLine = bLine;
		CRect r;
		GetItemRect (nItem, r, LVIR_BOUNDS);
		InvalidateRect (r);
	}
}

BOOL CuListCtrl::SetItem (LV_ITEM* pItem)
{
	if (!m_bActivateColorItem)
		return CListCtrl::SetItem (pItem);

	CaListCtrlColorItemData* pData = (CaListCtrlColorItemData*)CListCtrl::GetItemData (pItem->iItem);
	ASSERT (pData);
	if (pData)
	{
		pData->m_lUserData = pItem->lParam;
		pItem->lParam = (LPARAM)pData;
	}
	return CListCtrl::SetItem (pItem);
}

BOOL CuListCtrl::SetItem (int nItem, int nSubItem, UINT nMask, LPCTSTR lpszItem, int nImage, UINT nState, UINT nStateMask, LPARAM lParam )
{
	if (!m_bActivateColorItem)
		return CListCtrl::SetItem (nItem, nSubItem, nMask, lpszItem, nImage, nState, nStateMask, lParam);

	CaListCtrlColorItemData* pData = (CaListCtrlColorItemData*)CListCtrl::GetItemData (nItem);
	ASSERT (pData);
	if (pData)
	{
		pData->m_lUserData = lParam;
	}
	return CListCtrl::SetItem (nItem, nSubItem, nMask, lpszItem, nImage, nState, nStateMask, (LPARAM)pData);
}

BOOL CuListCtrl::SetItemData(int nItem, DWORD dwData)
{
	if (!m_bActivateColorItem)
		return CListCtrl::SetItemData(nItem, dwData);

	CaListCtrlColorItemData* pData = (CaListCtrlColorItemData*)CListCtrl::GetItemData (nItem);
	ASSERT (pData);
	if (pData)
	{
		pData->m_lUserData = (LPARAM)dwData;
	}
	return TRUE;
}

DWORD CuListCtrl::GetItemData(int nItem ) const
{
	if (!m_bActivateColorItem)
		return CListCtrl::GetItemData(nItem);

	CaListCtrlColorItemData* pData = (CaListCtrlColorItemData*)CListCtrl::GetItemData (nItem);
	if (pData)
		return (DWORD)pData->m_lUserData;
	return 0;
}

void CuListCtrl::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if (nChar == VK_CONTROL || nChar == VK_SHIFT)
	{
		GetParent()->SendMessage (WMUSRMSG_CTRLxSHIFT_UP, (WPARAM)this, 0);
	}
	CListCtrl::OnKeyUp(nChar, nRepCnt, nFlags);
}

void CuListCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if (!m_bAllowSelect)
		return;
	CListCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}

int CuListCtrl::FindItem( LV_FINDINFO* pFindInfo, int nStart) const
{
	if (!m_bActivateColorItem)
		return CListCtrl::FindItem(pFindInfo, nStart);

	if (!(pFindInfo->flags & LVFI_PARAM))
		return CListCtrl::FindItem(pFindInfo, nStart);
	else
	{
		int nStartPos = (nStart == -1)? 0: nStart;
		int i, nCount = GetItemCount();
		for (i=nStartPos; i<nCount; i++)
		{
			if ((LPARAM)GetItemData(i) == pFindInfo->lParam)
				return i;
		}
		return -1;
	}
}


//
