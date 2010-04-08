/*
**  Copyright (C) 2005-2006 Ingres Corporation. All rights reserved.
**
**    Source   : imagbtn.cpp, Implementation file
**    Project  : Extension DLL (VNode)
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Owner draw button that uses image list as button bitmaps
**
** History:
**
** 08-Oct-1999 (uk$so01)
**    Created
**
**/

#include "stdafx.h"
#include "imagbtn.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CuImageButton, CButton)
enum {IM_UP, IM_DOWN, IM_FOCUS, IM_DISABLE};

CuImageButton::CuImageButton()
{
	m_bIgnoreKeyRETURNxSPACE = FALSE;
}

CuImageButton::~CuImageButton()
{

}

BEGIN_MESSAGE_MAP(CuImageButton, CButton)
	//{{AFX_MSG_MAP(CuBitmapButton)
	ON_WM_GETDLGCODE()
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

UINT CuImageButton::OnGetDlgCode() 
{
	return DLGC_WANTALLKEYS;
}

void CuImageButton::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if (nChar == VK_SPACE || nChar == VK_RETURN)
		if (m_bIgnoreKeyRETURNxSPACE)
			return;
	CButton::OnKeyDown(nChar, nRepCnt, nFlags);
}


BOOL CuImageButton::SetImages(CImageList* pImageList)
{
	//
	// Delete old images (if present)
	ASSERT (pImageList);
	if (!pImageList)
		return FALSE;
	m_imageList.DeleteImageList();
	IMAGEINFO imageinfo;
	memset (&imageinfo, 0, sizeof(imageinfo));
	if (pImageList->GetImageInfo(0, &imageinfo))
	{
		int nW = imageinfo.rcImage.right  - imageinfo.rcImage.left;
		int nH = imageinfo.rcImage.bottom - imageinfo.rcImage.top;
		m_imageList.Create(nW, nH, TRUE, 1, 1);
	}
	else
	{
		//
		// Need image list;
		m_imageList.Create(23, 22, TRUE, 1, 1);
	}
	HICON hIcon = NULL;
	int i = 0, nCount = pImageList->GetImageCount();

	while (i < nCount)
	{
		hIcon = pImageList->ExtractIcon(i);
		m_imageList.Add (hIcon);
		DestroyIcon (hIcon);
		i++;
	}

	return TRUE;
}


// SizeToContent will resize the button to the size of the bitmap
void CuImageButton::SizeToContent()
{
	ASSERT(m_imageList.m_hImageList != NULL);
	IMAGEINFO imgInfo;
	int height = 16;
	int width  = 16;
	if (m_imageList.GetImageInfo(0, &imgInfo))
	{
		height = imgInfo.rcImage.bottom - imgInfo.rcImage.top;
		width  = imgInfo.rcImage.right  - imgInfo.rcImage.left;
	}
	VERIFY(SetWindowPos(NULL, -1, -1, width, height, SWP_NOMOVE|SWP_NOZORDER|SWP_NOREDRAW|SWP_NOACTIVATE));
}

//
// Draw the appropriate image
void CuImageButton::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
	int iImage = 0;
	ASSERT(lpDIS != NULL);
	int nCount = m_imageList.GetImageCount();
	//
	// Must have at least the first bitmap loaded before calling DrawItem
	ASSERT(m_imageList.m_hImageList != NULL);     // required
	//
	// Use the zero-based index index 0 bitmap for up, the selected bitmap for down
	UINT state = lpDIS->itemState;
	if ((state & ODS_SELECTED) && m_imageList.m_hImageList != NULL)
		iImage = (nCount > 1)? 1: 0;
	else 
	if ((state & ODS_FOCUS) &&  m_imageList.m_hImageList != NULL /*&& (GetParent()->GetStyle() & DS_WINDOWSUI)*/)
		iImage = (nCount > 2)? 2: 0;
	else 
	if ((state & ODS_DISABLED) && m_imageList.m_hImageList != NULL)
		iImage = (nCount > 3)? 3: 0;

	//
	// Draw the whole button
	CRect rect;
	rect.CopyRect(&lpDIS->rcItem);
	CDC* pDC = CDC::FromHandle(lpDIS->hDC);
	m_imageList.Draw (pDC, iImage, CPoint (rect.left, rect.top), ILD_TRANSPARENT);
}


void CuImageButton::OnLButtonDown(UINT nFlags, CPoint point)
{
	Invalidate();
	CButton::OnLButtonDown(nFlags, point);
	Invalidate();
}

void CuImageButton::OnLButtonUp(UINT nFlags, CPoint point) 
{
	Invalidate();
	CButton::OnLButtonUp(nFlags, point);
	Invalidate();
}