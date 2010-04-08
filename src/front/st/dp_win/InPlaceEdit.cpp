/*
**
**  Name: InPlaceEdit.cpp
**
**  Description:
**	This file implements the concept of in-place editing of subitems (in,
**	for example, a list box).
**
**  History:
**	05-mar-1999 (somsa01)
**	    Created.
**	14-apr-1999 (somsa01)
**	    Slight changes to the creation of the spin control.
*/

#include "stdafx.h"
#include "dp.h"
#include "InPlaceEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
** CInPlaceEdit
*/
CInPlaceEdit::CInPlaceEdit(int iItem, int iSubItem, CString sInitText, BOOL NeedSpin)
:m_sInitText( sInitText )
{
    m_iItem = iItem;
    m_iSubItem = iSubItem;
    m_bESC = FALSE;
    m_NeedSpin = NeedSpin;
}

CInPlaceEdit::~CInPlaceEdit()
{
}

BEGIN_MESSAGE_MAP(CInPlaceEdit, CEdit)
    //{{AFX_MSG_MAP(CInPlaceEdit)
    ON_WM_KILLFOCUS()
    ON_WM_CHAR()
    ON_WM_CREATE()
    ON_WM_NCDESTROY()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
** CInPlaceEdit message handlers
*/
void CInPlaceEdit::OnKillFocus(CWnd* pNewWnd) 
{
    CString	str;
    LV_DISPINFO	dispinfo;

    CEdit::OnKillFocus(pNewWnd);
	
    GetWindowText(str);

    /* Send Notification to parent of ListView ctrl */
    dispinfo.hdr.hwndFrom = GetParent()->m_hWnd;
    dispinfo.hdr.idFrom = GetDlgCtrlID();
    dispinfo.hdr.code = LVN_ENDLABELEDIT;
    dispinfo.item.mask = LVIF_TEXT;
    dispinfo.item.iItem = m_iItem;
    dispinfo.item.iSubItem = m_iSubItem;
    dispinfo.item.pszText = m_bESC ? NULL : LPTSTR((LPCTSTR)str);
    dispinfo.item.cchTextMax = str.GetLength();
    GetParent()->GetParent()->SendMessage( WM_NOTIFY, GetParent()->GetDlgCtrlID(), 
					   (LPARAM)&dispinfo );

    if (m_NeedSpin)
	::DestroyWindow(pSpin->m_hWnd);

    DestroyWindow();	
}

void
CInPlaceEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
    CString	str;
    CRect	rect, parentrect;

    if( nChar == VK_ESCAPE || nChar == VK_RETURN)
    {
	if( nChar == VK_ESCAPE )
	    m_bESC = TRUE;
	GetParent()->SetFocus();
	return;
    }
	
    CEdit::OnChar(nChar, nRepCnt, nFlags);

    /* Resize edit control if needed */
    if (!m_NeedSpin)
    {
    /* Get text extent */
	GetWindowText( str );
	CWindowDC dc(this);
	CFont *pFont = GetParent()->GetFont();
	CFont *pFontDC = dc.SelectObject( pFont );
	CSize size = dc.GetTextExtent( str );
	dc.SelectObject( pFontDC );
	size.cx += 5;

	/* add some extra buffer */
	/* Get client rect */
	GetClientRect( &rect );
	GetParent()->GetClientRect( &parentrect );

	/* Transform rect to parent coordinates */
	ClientToScreen( &rect );
	GetParent()->ScreenToClient( &rect );

	/* Check whether control needs to be resized */
	/* and whether there is space to grow */
	if (size.cx > rect.Width())
	{
	    if (size.cx + rect.left < parentrect.right)
		rect.right = rect.left + size.cx;
	    else
		rect.right = parentrect.right;
	    MoveWindow( &rect );
	}
    }
}

int
CInPlaceEdit::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
    if (CEdit::OnCreate(lpCreateStruct) == -1)
	return -1;
	
    /* Set the proper font */
    CFont* font = GetParent()->GetFont();
    SetFont(font);

    /* Set the Spin Control, if required. */
    if (m_NeedSpin)
    {
	RECT rect;
	DWORD dwStyle;

	dwStyle = UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS |
		  UDS_SETBUDDYINT;
	GetClientRect(&rect);
	pSpin = new CSpinButtonCtrl();
	pSpin->Create(dwStyle, rect, GetParent(), IDC_IPSPIN);
	pSpin->SetBuddy(this);
	pSpin->SetBase(10);
	pSpin->SetRange(0,9999);
	pSpin->SetPos(atoi(m_sInitText));
	pSpin->ShowWindow(SW_SHOW);
    }
    else
	SetWindowText( m_sInitText );

    SetFocus();
    SetSel( 0, -1 );
	
    return 0;
}

void
CInPlaceEdit::OnNcDestroy() 
{
    CEdit::OnNcDestroy();

    if (m_NeedSpin)
	delete pSpin;

    delete this;	
}

BOOL
CInPlaceEdit::PreTranslateMessage(MSG* pMsg) 
{
    if (pMsg->message == WM_KEYDOWN)
    {
	if (pMsg->wParam == VK_RETURN ||
	    pMsg->wParam == VK_DELETE ||
	    pMsg->wParam == VK_ESCAPE ||
	    GetKeyState (VK_CONTROL))
	{
	    ::TranslateMessage(pMsg);
	    ::DispatchMessage(pMsg);
	    return TRUE;	/* DO NOT process further */
	}
    }
	
    return CEdit::PreTranslateMessage(pMsg);
}
