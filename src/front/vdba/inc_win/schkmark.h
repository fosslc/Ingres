/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : 2schkmark.cpp : implementation file 
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : control check mark that looks like a check box
**
** History:
**
** 06-Mar-2001 (uk$so01)
**    Created
**/

#if !defined(AFX_SCHKMARK_H__3BEBA2E4_123C_11D5_8741_00C04F1F754A__INCLUDED_)
#define AFX_SCHKMARK_H__3BEBA2E4_123C_11D5_8741_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CuStaticCheckMark : public CStatic
{
public:
	CuStaticCheckMark();
	void SetImageWidth(int nWidth){m_cxImage=nWidth;}
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuStaticCheckMark)
	protected:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CuStaticCheckMark();
protected:
	BOOL m_bCreateImage;
	int m_cxImage;
	CImageList m_ImageList;
	// Generated message map functions
protected:
	//{{AFX_MSG(CuStaticCheckMark)
	afx_msg void OnPaint();
	afx_msg void OnDestroy();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCHKMARK_H__3BEBA2E4_123C_11D5_8741_00C04F1F754A__INCLUDED_)
