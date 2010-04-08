/*
**  Copyright (c) 2001 Ingres Corporation
*/

/*
**  Name: 256bmp.h : header file
**
**  History:
**
**	05-Jun-2001 (penga03)
**	    Created.
**	01-Mar-2007 (drivi01)
**		Modified code to overwrite painting of banner at the top
c**	    of the wizard.
*/

#if !defined(AFX_256BMP_H__2A373724_7BB6_4C72_9F39_840F689C46A5__INCLUDED_)
#define AFX_256BMP_H__2A373724_7BB6_4C72_9F39_840F689C46A5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// C256bmp window

class C256bmp : public CStatic
{
	friend class CBmpPalette;

	BITMAPINFO *	m_pInfo;
	BYTE *			m_pPixels;
	CBmpPalette *	m_pPal;
	BOOL			m_bIsPadded;

// Construction
public:
		C256bmp();
		virtual ~C256bmp();
// Attributes
public:
		BITMAPINFO *	GetHeaderPtr() const;
		BYTE *			GetPixelPtr() const;
		RGBQUAD *		GetColorTablePtr() const;
		int				GetWidth() const;
		int				GetHeight() const;
		CBmpPalette *	GetPalette() { return m_pPal; }
// Operations
public:
		BOOL			CreatePalette();	// auto. made by "Load()" and "CreateFromBitmap()"
		void			ClearPalette();		// destroy the palette associated with this image
		BOOL			CreateFromBitmap( CDC * pDC, CBitmap * pSrcBitmap ) ;
		BOOL			LoadOneResource(LPCTSTR pszID);
		BOOL			LoadOneResource(UINT ID) { return LoadOneResource(MAKEINTRESOURCE(ID)); }
		BOOL			IsEmpty(void) const { return (m_pInfo == NULL); }

// Overrides
public:  // ClassWizard generated virtual function overrides
	    // draw the bitmap at the specified location
		virtual void	DrawDIB( CDC * pDC, int x=0, int y=0 );

		// draw the bitmap and stretch/compress it to the desired size
		virtual void	DrawDIB( CDC * pDC, int x, int y, int width, int height );

		// draw parts of the dib into a given area of the DC
		virtual int		DrawDIB( CDC * pDC, CRect & rectDC, CRect & rectDIB );
	//{{AFX_VIRTUAL(C256bmp)
	//}}AFX_VIRTUAL

// Implementation
public:
		void			GetBitmapDimension(CPoint &pt);
		BITMAPINFO *	GetBitmap();
	
	
	


	// Generated message map functions
protected:
		int				GetPalEntries() const;
		int				GetPalEntries( BITMAPINFOHEADER& infoHeader ) const;
		//{{AFX_MSG(C256bmp)
		afx_msg BOOL OnEraseBkgnd(CDC* pDC);
		afx_msg void OnPaint();
		//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_256BMP_H__2A373724_7BB6_4C72_9F39_840F689C46A5__INCLUDED_)
