/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : titlebmp.cpp, Implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Owner draw Static Control that can display a bitmap on the left
**               The bitmap is loaded by CImageList and we use CImageList::Draw to 
**               display the bitmap
** History:
**
** xx-Jul-1998 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
*/

#include "stdafx.h"
#include "titlebmp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuTitleBitmap::CuTitleBitmap()
{
	int nCx = 16, nCy = 15, nGrow = 1;
	m_ImageList.Create (nCx, nCy, ILC_MASK, 0, nGrow);
	m_nImage = 0;
	m_nImageLeftMagin = 0;
}


CuTitleBitmap::CuTitleBitmap(UINT nImageList, COLORREF crMask, int nWidth , int nImage)
{
	m_ImageList.DeleteImageList();
	VERIFY (m_ImageList.Create (nImageList, nWidth, 0, crMask));
	m_nImage = nImage;
	m_nImageLeftMagin = 0;
}


CuTitleBitmap::~CuTitleBitmap()
{
}

void CuTitleBitmap::SetImage (int nImage)
{
	try
	{
		m_nImage = nImage;
		if (m_hWnd)
			Invalidate();
	}
	catch (...)
	{
#if defined (_DEBUG)
		//_T("Invalid image index of Image list.");
		AfxMessageBox (_T("CuTitleBitmap::SetImage(): Invalid image index of Image list."));
#endif
	}
}

void CuTitleBitmap::ResetBitmap (UINT nBitmapID, COLORREF crMask)
{
	m_nImage = -1;
	while (m_ImageList.m_hImageList && m_ImageList.Remove(0));
	if (nBitmapID <= 0)
		return;
	m_nImage = 0;
	CBitmap bmp;
	bmp.LoadBitmap (nBitmapID);
	m_ImageList.Add (&bmp, crMask);
	if (m_hWnd)
		Invalidate();
}

void CuTitleBitmap::ResetBitmap (HBITMAP hBitmap, COLORREF crMask)
{
	m_nImage = -1;
	while (m_ImageList.m_hImageList && m_ImageList.Remove(0));
	if (!hBitmap)
		return;
	m_nImage = 0;
	CBitmap* pBmp = CBitmap::FromHandle(hBitmap);
	m_ImageList.Add (pBmp, crMask);
	if (m_hWnd)
		Invalidate();
}

void CuTitleBitmap::ResetBitmap (HICON hIcon)
{
	m_nImage = -1;
	while (m_ImageList.m_hImageList && m_ImageList.Remove(0));
	if (!hIcon)
		return;
	m_nImage = 0;
	m_ImageList.Add (hIcon);
	if (m_hWnd)
		Invalidate();
}


BEGIN_MESSAGE_MAP(CuTitleBitmap, CStatic)
	//{{AFX_MSG_MAP(CuTitleBitmap)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuTitleBitmap message handlers

void CuTitleBitmap::OnPaint() 
{
	CPaintDC dc(this);
	CRect r;
	GetClientRect (r);

	int nyTop = 0 + (r.Height() - 15)/2;
	CString strText;
	GetWindowText (strText);

	if (m_ImageList.m_hImageList && m_nImage != -1)
	{
		m_ImageList.Draw (&dc, m_nImage, CPoint (m_nImageLeftMagin, nyTop), ILD_NORMAL);
		r.left += 20;
	}
	CFont* pDlgFont = GetParent()->GetFont();
	int nOldBkMode  = dc.SetBkMode (TRANSPARENT);
	CFont* pOldFont = dc.SelectObject(pDlgFont);
	if (!strText.IsEmpty())
	{
		dc.DrawText (strText, r, DT_LEFT|DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER);
	}
	dc.SetBkMode(nOldBkMode);
	dc.SelectObject(pOldFont);
}


