/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vrepcolf.h, Header file
**    Project  : INGRESII / Monitoring.
**    Author   : UK Sotheavut
**    Purpose  : Tree View to display the transaction having collision
**
** History:
**
** xx-May-1998 (uk$so01)
**    Created
**  20-Aug-2008 (whiro01)
**    Removed redundant include of <afxcview.h>
*/



#if !defined(AFX_VREPCOLF_H__09BFF512_EB0A_11D1_A27C_00609725DDAF__INCLUDED_)
#define AFX_VREPCOLF_H__09BFF512_EB0A_11D1_A27C_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "collidta.h"


class CvReplicationPageCollisionViewLeft : public CTreeView
{
protected:
	CvReplicationPageCollisionViewLeft(); 
	DECLARE_DYNCREATE(CvReplicationPageCollisionViewLeft)

public:
	void CurrentSelectedUpdateImage();
	void ResolveCurrentTransaction();
	//
	// Verify if all sequences have been marked as SOURCE|TARGET prevails.
	BOOL CheckResolveCurrentTransaction();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CvReplicationPageCollisionViewLeft)
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	CImageList m_ImageList;

	virtual ~CvReplicationPageCollisionViewLeft();
	void UpdateDisplay (CaCollision* pData);

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	//
	// Generated message map functions
protected:
	//{{AFX_MSG(CvReplicationPageCollisionViewLeft)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemexpanded(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VREPCOLF_H__09BFF512_EB0A_11D1_A27C_00609725DDAF__INCLUDED_)
