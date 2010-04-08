/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
** 
**    Source   : peditctl.cpp, Implementation File
**    Project  : OpenIngres Configuration Manager 
**    Author   : UK Sotheavut
**    Purpose  : Special parse edit to allow only a subset of characters 
**
** History:
**
** xx-Nov-1997 (uk$so01)
**    Created
** 29-May-2000 (uk$so01)
**    bug 99242 (DBCS), replace the functions
**    IsCharAlphaNumeric by _istalnum
**    IsCharAlpha by _istalpha
**/

#include "stdafx.h"
#include "peditctl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuParseEditCtrl

CuParseEditCtrl::CuParseEditCtrl()
{
	m_wParseStyle = 0;
	m_bParse      = TRUE;
}

CuParseEditCtrl::~CuParseEditCtrl()
{
}

BOOL CuParseEditCtrl::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
{
	m_wParseStyle = LOWORD(dwStyle);
	//
	// Figure out edit control style
	DWORD dwEditStyle = MAKELONG(ES_LEFT, HIWORD(dwStyle));
	return CWnd::Create(_T("EDIT"), NULL, dwEditStyle, rect, pParentWnd, nID);
}


BOOL CuParseEditCtrl::SubclassEdit(UINT nID, CWnd* pParent, WORD wParseStyle)
{
	m_wParseStyle = wParseStyle;
	return SubclassDlgItem(nID, pParent);
}

BEGIN_MESSAGE_MAP(CuParseEditCtrl, CEdit)
	//{{AFX_MSG_MAP(CuParseEditCtrl)
	ON_WM_CHAR()
	ON_WM_VSCROLL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuParseEditCtrl message handlers

void CuParseEditCtrl::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	WORD type;
	if (!m_bParse)
	{
		CEdit::OnChar(nChar, nRepCnt, nFlags);  
		return;
	}

	if (nChar < 0x20)
		type = PES_ALL;                         // always allow control chars
	else if (_istalnum(nChar) && !_istalpha(nChar))
		type = PES_NATURAL;
	else if (_istalpha(nChar))
		type = PES_LETTERS;
	else
	{
		CString strText;
		GetWindowText (strText);
		int nLen = strText.GetLength ();
		if ((TCHAR)nChar == _T('-') && (m_wParseStyle & PES_INTEGER) && nLen == 0)
			type = PES_INTEGER;
		else
		if ((TCHAR)nChar == _T('-') && (m_wParseStyle & PES_INTEGER) && nLen > 0)
		{
			int nStartChar, nEndChar;
			GetSel(nStartChar, nEndChar);
			if (nStartChar == 0 && nEndChar == 0 && strText.Find (_T('-'))== -1)
				type = PES_INTEGER;
			else
			if (nStartChar == 0 && nEndChar > 0)
			{
				//
				// If partial or all text is selected from the starting position
				// then allow typing the minus sign ('-') even if it has already
				// been in the edit.
				type = PES_INTEGER;
			}
			else
				type = PES_OTHERCHARS;
		}
		else
			type = PES_OTHERCHARS;
	}

	if (m_wParseStyle & type)
	{
		CEdit::OnChar(nChar, nRepCnt, nFlags);  // permitted
	}
	else
	{
		//
		// Illegal character - inform parent
		OnBadInput();
	}
}

void CuParseEditCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	TRACE0 ("CuParseEditCtrl::OnVScroll\n");	
	CEdit::OnVScroll(nSBCode, nPos, pScrollBar);
}



/////////////////////////////////////////////////////////////////////////////
// default bad input handler, beep (unless parent notification
//    returns -1.  Most parent dialogs will return 0 or 1 for command
//    handlers (i.e. Beep is the default)
void CuParseEditCtrl::OnBadInput()
{
	if (GetParent()->SendMessage(WM_COMMAND, MAKELONG(GetDlgCtrlID(), PEN_ILLEGALCHAR), (LPARAM)m_hWnd) != -1)
	{
		MessageBeep((UINT)-1);
	}
}
