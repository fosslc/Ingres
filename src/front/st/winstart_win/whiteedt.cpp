// whiteedt.cpp : implementation file
//
// History:
//
//     17-nov-98 (cucjo01)
//        Created.
//        This overrides the gray color in an edit control
//        and forces the background to white.
//

#include "stdafx.h"
#include "winstart.h"
#include "whiteedt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWhiteEdit

CWhiteEdit::CWhiteEdit()
{  m_clrText = GetSysColor(COLOR_WINDOWTEXT);
   m_clrBkgnd = GetSysColor(COLOR_WINDOW);
   m_brBkgnd.CreateSolidBrush( m_clrBkgnd );
}

CWhiteEdit::~CWhiteEdit()
{
}


BEGIN_MESSAGE_MAP(CWhiteEdit, CEdit)
	//{{AFX_MSG_MAP(CWhiteEdit)
	ON_WM_CTLCOLOR_REFLECT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWhiteEdit message handlers

HBRUSH CWhiteEdit::CtlColor(CDC* pDC, UINT nCtlColor) 
{
    pDC->SetTextColor( m_clrText );    // text
    pDC->SetBkColor( m_clrBkgnd );    // text bkgnd
    return m_brBkgnd;                // ctl bkgnd
	// Returns a non-NULL brush since the parent's handler should not be called
	
}
