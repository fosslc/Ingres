/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : parse.h, Header File 
**    Project  : Visual DBA.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Bitmap assumed to have RGB(255,0,255) pixels to be 
**               drawn as transparent, an be enhanced by adding a 
**               new member with RGB to be drawn as transparent.
**
** History:
**
** 24-Jul-2001 (uk$so01)
**    Created. Split the old code from VDBA (mainmfc\parse.cpp)
** 03-Sep-2001 (uk$so01)
**    Transform code to be an ActiveX Control
*/

#ifndef TRANSPARENT_BMP_HEADER
#define TRANSPARENT_BMP_HEADER
/////////////////////////////////////////////////////////////////////////////
// Utility functions
void DrawTransparentBitmap(HDC hdc, HBITMAP hBitmap, short xStart, short yStart, COLORREF cTransparentColor);

class CStaticTransparentBitmap : public CStatic
{
public:
	void SetBitmap (HBITMAP hBmp);
	CStaticTransparentBitmap();


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStaticTransparentBitmap)
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CStaticTransparentBitmap();

	// Generated message map functions
protected:
	//{{AFX_MSG(CStaticTransparentBitmap)
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
private:
	HBITMAP m_hBmp;
};

#endif  // TRANSPARENT_BMP_HEADER