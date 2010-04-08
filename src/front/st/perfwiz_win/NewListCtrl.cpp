/*
**
**  Name: NewListCtrl.cpp
**
**  Description:
**	This file subclasses a CListCtrl with extra functions.
**
**  History:
**	11-mar-1999 (somsa01)
**	    Created.
**	13-apr-1999 (somsa01)
**	    Added HitTestEx() to act like SubItemHitTest. Also, mimicked
**	    GetSubItemRect() rather than using it.
**	29-jul-1999 (somsa01)
**	    Added functionality such that the offsets would get updated
**	    automatically when the user selects a different
**	    internal/external format for a column.
*/

#include "stdafx.h"
#include "perfwiz.h"
#include "NewListCtrl.h"
#include "InPlaceEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
** CNewListCtrl
*/
CNewListCtrl::CNewListCtrl()
{
}

CNewListCtrl::~CNewListCtrl()
{
}

BEGIN_MESSAGE_MAP(CNewListCtrl, CListCtrl)
    //{{AFX_MSG_MAP(CNewListCtrl)
    ON_WM_HSCROLL()
    ON_WM_VSCROLL()
    ON_NOTIFY_REFLECT(LVN_ENDLABELEDIT, OnEndlabeledit)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
** CNewListCtrl message handlers
*/
CEdit*
CNewListCtrl::EditSubLabel (int nItem, int nCol)
{
    CHeaderCtrl	*pHeader = (CHeaderCtrl*)GetDlgItem(0);
    int		nColumnCount, offset = 0, offset_left = 0, i;
    CRect	rect, rcClient;
    LV_COLUMN	lvcol;
    CSize	size;
    DWORD	dwStyle;
    BOOL	NeedSpin = FALSE;

    /*
    ** The returned pointer should not be saved!
    */

    /* Make sure that the item is visible */
    if (!EnsureVisible(nItem, TRUE))
	return NULL;

    /* Make sure that nCol is valid */
    nColumnCount = pHeader->GetItemCount();
    if ((nCol >= nColumnCount) || (GetColumnWidth(nCol) < 5))
	return NULL;

    /* Get the column offset */
    for (i = 0; i < nCol; i++)
	offset += GetColumnWidth(i);

    for (i = nCol+1; i < nColumnCount; i++)
	offset_left += GetColumnWidth(i);

    /* Now scroll if we need to expose the column */
    GetItemRect(nItem, &rect, LVIR_BOUNDS);
    GetClientRect(&rcClient);
    size.cx = size.cy = 0;
    if (rect.right - offset_left > rcClient.right)
	size.cx = rect.right - offset_left - rcClient.right;
    else if (rect.left + offset < rcClient.left)
	size.cx = rect.left + offset - rcClient.left;
    Scroll(size);

    GetItemRect(nItem, &rect, LVIR_BOUNDS);

    /* Get Column alignment */
    lvcol.mask = LVCF_FMT;
    GetColumn(nCol, &lvcol);
    if ((lvcol.fmt&LVCFMT_JUSTIFYMASK) == LVCFMT_LEFT)
	dwStyle = ES_LEFT;
    else if ((lvcol.fmt&LVCFMT_JUSTIFYMASK) == LVCFMT_RIGHT)
	dwStyle = ES_RIGHT;
    else
	dwStyle = ES_CENTER;
    rect.left += offset;
    rect.right = rect.left + GetColumnWidth( nCol ) ;
    if (rect.right > rcClient.right)
	rect.right = rcClient.right;
    dwStyle |= WS_BORDER|WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL;

    NeedSpin = FALSE;

    CEdit *pEdit = new CInPlaceEdit(nItem, nCol,
				    GetItemText(nItem, nCol), NeedSpin);
    pEdit->Create(dwStyle, rect, this, IDC_IPEDIT);

    return pEdit;
}

void
CNewListCtrl::OnEndlabeledit(NMHDR* pNMHDR, LRESULT* pResult)
{
    LV_DISPINFO		*plvDispInfo = (LV_DISPINFO *)pNMHDR;
    LV_ITEM		*plvItem = &plvDispInfo->item;

    if (plvItem->pszText != NULL)
    {
	SetItemText(plvItem->iItem, plvItem->iSubItem,
					plvItem->pszText);
	GetParent()->SendMessage(WM_APP, 0, plvItem->iItem);
    }
    else
	GetParent()->SendMessage(WM_APP, 0, -1);

    *pResult = 0;
}

void
CNewListCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
    if(GetFocus() != this)
	SetFocus();
	
    CListCtrl::OnHScroll(nSBCode, nPos, pScrollBar);
}

void
CNewListCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
    /* Don't look at OTHER scroll bar messages from child windows. */
    if (pScrollBar != GetScrollBarCtrl(SB_VERT))
	return;

    if(GetFocus() != this)
	SetFocus();
	
    CListCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
}

int
CNewListCtrl::HitTestEx(CPoint &point, int *col)
{
    int colnum = 0, row, bottom, nColumnCount, colwidth;
    CHeaderCtrl* pHeader;
    CRect rect;

    row = HitTest(point, NULL);
    if (col)
	*col = 0;

    /* Make sure that the ListView is in LVS_REPORT */
    if ((GetWindowLong(m_hWnd, GWL_STYLE) & LVS_TYPEMASK) != LVS_REPORT)
	return row;

    /* Get the top and bottom row visible */
    row = GetTopIndex();
    bottom = row + GetCountPerPage();
    if (bottom > GetItemCount())
	bottom = GetItemCount();

    /* Get the number of columns */
    pHeader = (CHeaderCtrl*)GetDlgItem(0);
    nColumnCount = pHeader->GetItemCount();

    /* Loop through the visible rows */
    for (;row <= bottom;row++)
    {
	/* Get bounding rect of item and check whether point falls in it. */
	GetItemRect(row, &rect, LVIR_BOUNDS);
	if (rect.PtInRect(point))
	{
	    /* Now find the column */
	    for(colnum = 0; colnum < nColumnCount; colnum++)
	    {
		colwidth = GetColumnWidth(colnum);
		if ((point.x >= rect.left) &&
		    (point.x <= (rect.left + colwidth)))
		{
		    if (col)
			*col = colnum;
		    return row;
		}
		rect.left += colwidth;
	    }
	}
    }
    return -1;
}
