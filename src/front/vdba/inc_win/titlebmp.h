/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : titlebmp.h, header file 
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

#if !defined(AFX_TITLEBMP_H__229723B3_16FD_11D2_A292_00609725DDAF__INCLUDED_)
#define AFX_TITLEBMP_H__229723B3_16FD_11D2_A292_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class CuTitleBitmap : public CStatic
{
public:
	CuTitleBitmap();
	CuTitleBitmap(UINT nImageList, COLORREF crMask, int nWidth = 16 , int nImage = 0);

	void SetImageLeftMagin(int nVal){m_nImageLeftMagin = nVal;}
	//
	// When the image list contains more than one image, it choose the image
	// of index 'nImage' to draw.
	void SetImage (int nImage);

	//
	// nBitmapID: a single image in the image list
	// This function sets Image list to have a single image with the zero base index id = 0
	// The image's width is the same as the one provide at Construct time (2)
	// The default contructor (1) creates the image of width = 16 and mask = RGB(255,0,255)
	void ResetBitmap (UINT nBitmapID,  COLORREF crMask);
	void ResetBitmap (HBITMAP hBitmap, COLORREF crMask);
	void ResetBitmap (HICON   hIcon);

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuTitleBitmap)
	//}}AFX_VIRTUAL

	//
	// Implementation
public:
	virtual ~CuTitleBitmap();

protected:
	int        m_nImage;
	int        m_nImageLeftMagin;
	CImageList m_ImageList;

	//
	// Generated message map functions
protected:
	//{{AFX_MSG(CuTitleBitmap)
	afx_msg void OnPaint();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TITLEBMP_H__229723B3_16FD_11D2_A292_00609725DDAF__INCLUDED_)
