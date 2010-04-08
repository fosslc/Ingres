/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : Splitter.cpp, Implementation file                                     //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Special Splitter Control for Window NT.                               //
****************************************************************************************/
#include "stdafx.h"
#include "mainmfc.h"
#include "splitter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DWORD CuSplitter::GetWindowVersion ()
{
    OSVERSIONINFO vsinfo;
    vsinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx (&vsinfo))
        return 0;
    return vsinfo.dwPlatformId;
}

CuSplitter::CuSplitter()
{
    m_Alignment = 0;
}

CuSplitter::CuSplitter(UINT alignment)
{
    m_Alignment = alignment;
}

CuSplitter::~CuSplitter()
{
}


BEGIN_MESSAGE_MAP(CuSplitter, CStatic)
    //{{AFX_MSG_MAP(CuSplitter)
    ON_WM_CREATE()
    ON_WM_PAINT()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuSplitter message handlers

int CuSplitter::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
    if (CStatic::OnCreate(lpCreateStruct) == -1)
        return -1;
    return 0;
}

void CuSplitter::OnPaint() 
{
    CRect r;
    CPaintDC dc(this); 
    CPen  penBlack, penWhite, penGray, penDarkGrey;
    CPen* oldPen;
#if !defined (MAINWIN)
    if (GetWindowVersion() != VER_PLATFORM_WIN32_NT)
        return;
#endif        
    GetWindowRect (r);
    ScreenToClient(r);
    penBlack.CreatePen    (PS_SOLID, 1, RGB (0, 0, 0));
    penWhite.CreatePen    (PS_SOLID, 1, RGB (255, 255, 255));
    penGray.CreatePen     (PS_SOLID, 2, RGB (192, 192, 192));
    penDarkGrey.CreatePen (PS_SOLID, 1, RGB (128, 128, 128));
    oldPen = dc.SelectObject (&penBlack);
    switch (m_Alignment)
    {
    case CBRS_ALIGN_LEFT:
    case CBRS_ALIGN_RIGHT:
        dc.SelectObject (&penBlack);
        dc.MoveTo (r.left, r.top);
        dc.LineTo (r.left, r.bottom);
        dc.MoveTo (r.left+5,r.top);
        dc.LineTo (r.left+5,r.bottom);
        dc.SelectObject (&penWhite);
        dc.MoveTo (r.left+1, r.top);
        dc.LineTo (r.left+1, r.bottom);
        dc.SelectObject (&penGray);
        dc.MoveTo (r.left+3, r.top);
        dc.LineTo (r.left+3, r.bottom);
        dc.SelectObject (&penDarkGrey);
        dc.MoveTo (r.left+4, r.top);
        dc.LineTo (r.left+4, r.bottom);
        break;
    case CBRS_ALIGN_TOP:
    case CBRS_ALIGN_BOTTOM:
        dc.SelectObject (&penBlack);
        dc.MoveTo (r.left,  r.top);
        dc.LineTo (r.right, r.top);
        dc.MoveTo (r.left,  r.top + 5);
        dc.LineTo (r.right, r.top + 5);
        dc.SelectObject (&penWhite);
        dc.MoveTo (r.left,  r.top+1);
        dc.LineTo (r.right, r.top+1);
        dc.SelectObject (&penGray);
        dc.MoveTo (r.left,  r.top+3);
        dc.LineTo (r.right, r.top+3);
        dc.SelectObject (&penDarkGrey);
        dc.MoveTo (r.left,  r.top+4);
        dc.LineTo (r.right, r.top+4);
        break;
    default:
        break;
    }
    dc.SelectObject (oldPen);
}

BOOL CuSplitter::PreCreateWindow(CREATESTRUCT& cs) 
{
    cs.style |= WM_NOTIFY;
    return CStatic::PreCreateWindow(cs);
}
