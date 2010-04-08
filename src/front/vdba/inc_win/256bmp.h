/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : 256Bmp.cpp : header file
**    Project  : IMPORT ASSISTANT 
**    Author   : 
**    Purpose  : STEP 1 of Import assistant
**
** History:
**
** 24-April-1999
**    Created for vdba sql assistant
** 07-Fev-2000 (uk$so01)
**    Move this class in a library 'libwctrl.lib' to be reusable.
**    Add a function SetBitmpapId() that set the bitmap ID after it has been constructed.
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 30-Jan-2002 (uk$so01)
**    SIR #106648, Enhance the position (center vertical & horizontal) of bitmap.
**/

#if !defined(AFX_256BMP_H__F3163F5E_E80C_11D0_9397_0020AF397415__INCLUDED_)
#define AFX_256BMP_H__F3163F5E_E80C_11D0_9397_0020AF397415__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class C256bmp : public CStatic
{
public:
	C256bmp();
	void SetBitmpapId(UINT nResourceID){m_nResourceID = nResourceID;}
	void SetBitmpapId(HINSTANCE hInstance, UINT nResourceID)
	{
		m_hExternalInstance = hInstance;
		m_nResourceID = nResourceID;
	}
	void SetCenterHorizontal(BOOL bSet){m_bCenterX = bSet;}
	void SetCenterVertical(BOOL bSet){m_bCenterY = bSet;}
	HINSTANCE GetExternalInstance(){return m_hExternalInstance;}
	UINT GetBitmatID(){return m_nResourceID;}
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(C256bmp)
	//}}AFX_VIRTUAL

	// Implementation
public:
	BITMAPINFO * GetBitmap();
	void GetBitmapDimension(CPoint & pt);
	virtual ~C256bmp();
protected:
	UINT m_nResourceID;
	HINSTANCE m_hModule;
	HINSTANCE m_hExternalInstance;
	BOOL m_bCenterX;
	BOOL m_bCenterY;

	// Generated message map functions
protected:
	//{{AFX_MSG(C256bmp)
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_256BMP_H__F3163F5E_E80C_11D0_9397_0020AF397415__INCLUDED_)
