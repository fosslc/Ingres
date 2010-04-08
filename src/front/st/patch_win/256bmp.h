/*
**  Copyright (c) 2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Name: 256bmp.h : header file
**
**  History:
**
**	05-Jun-2001 (penga03)
**	    Created.
**	01-Mar-2006 (drivi01)
**	    Reused for MSI Patch Installer on Windows.
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
// Construction
public:
	C256bmp();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(C256bmp)
	//}}AFX_VIRTUAL

// Implementation
public:
	void GetBitmapDimension(CPoint &pt);
	BITMAPINFO * GetBitmap();
	virtual ~C256bmp();

	// Generated message map functions
protected:
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
