/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : imagbtn.h, Header file.
**    Project  : Extension DLL (VNode)
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Owner draw button that uses image list as button bitmaps
**             : The image should contain 4 images,
**             : (0: normal, 1: select, 2: forcus, 3: disable)
**
** History:
**
** 08-Oct-1999 (uk$so01)
**    Created
**
**/


#if !defined (IMAGE_BUTTON_HEADER)
#define IMAGE_BUTTON_HEADER
class CuImageButton : public CButton
{
	DECLARE_DYNAMIC(CuImageButton)
public:
	CuImageButton();
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuImageButton)
	//}}AFX_VIRTUAL
	BOOL m_bIgnoreKeyRETURNxSPACE;

	virtual ~CuImageButton();
	BOOL SetImages (CImageList* pImages);
	void SizeToContent();
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDIS);
protected:
	CImageList m_imageList;
	// Generated message map functions
protected:
	//{{AFX_MSG(CuImageButton)
	afx_msg UINT OnGetDlgCode();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

#endif // IMAGE_BUTTON_HEADER
