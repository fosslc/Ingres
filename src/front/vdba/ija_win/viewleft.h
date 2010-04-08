/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : viewleft.h , Header File 
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : MFC (Frame View Doc) Tree View of Node, Database, Table
**
** History:
**
** 22-Dec-1999 (uk$so01)
**    Created
** 14-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management and put string into the resource files
**
**/

#if !defined(AFX_VIEWLEFT_H__DA2AADA6_AF05_11D3_A322_00C04F1F754A__INCLUDED_)
#define AFX_VIEWLEFT_H__DA2AADA6_AF05_11D3_A322_00C04F1F754A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "ijatree.h"

typedef enum {REFRESH_CURRENT = 1, REFRESH_CURRENT_SUB, REFRESH_CURRENT_ALL} RefreshType;

class CvViewLeft : public CTreeView
{
protected:
	CvViewLeft();
	DECLARE_DYNCREATE(CvViewLeft)

public:
	void RefreshData(RefreshType Mode = REFRESH_CURRENT);
	void ShowHelp();
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CvViewLeft)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

	// Implementation
protected:
	virtual ~CvViewLeft();
	void SetPaneDatabase(CTreeCtrl* pTree, HTREEITEM hSelected, CaIjaTreeItemData* pItem);
	void SetPaneTable(CTreeCtrl* pTree, HTREEITEM hSelected, CaIjaTreeItemData* pItem);
	void SetPaneNull(CTreeCtrl* pTree, HTREEITEM hSelected, CaIjaTreeItemData* pItem);

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	CaIjaTreeData m_treeData;
	CImageList m_ImageList;
	BOOL m_bInitialize;
	HTREEITEM m_hHitRButton;

	// Generated message map functions
protected:
	//{{AFX_MSG(CvViewLeft)
	afx_msg void OnItemexpanded(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRefreshCurrent();
	afx_msg void OnRefreshCurrentSub();
	afx_msg void OnSelchanging(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemexpanding(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VIEWLEFT_H__DA2AADA6_AF05_11D3_A322_00C04F1F754A__INCLUDED_)
