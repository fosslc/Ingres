/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : viewleft.h : header file
**    Project  : INGRESII / Visual Schema Diff Control (vsda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : TreeView of the left pane
**
** History:
**
** 26-Aug-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
*/

#if !defined(AFX_VIEWLEFT_H__CC2DA2C8_B8F1_11D6_87D8_00C04F1F754A__INCLUDED_)
#define AFX_VIEWLEFT_H__CC2DA2C8_B8F1_11D6_87D8_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "vsdtree.h"
class CvSdaLeft : public CTreeView
{
protected:
	CvSdaLeft();
	DECLARE_DYNCREATE(CvSdaLeft)

	// Operations
public:
	BOOL DoCompare (short nMode);
	void Reset ();
	void UpdateDisplayFilter(LPCTSTR lpszFilter);
	BOOL IsEnableDiscard();
	void DiscardItem();
	BOOL IsEnableUndiscard();
	void UndiscardItem();
	BOOL IsEnableAccessVdba();
	void AccessVdba();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CvSdaLeft)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

	// Implementation
protected:
	BOOL m_bInstallation;

	virtual ~CvSdaLeft();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	CImageList m_ImageList;
	CaVsdRoot m_tree;

	BOOL IsValidParameters (UINT& uiStar);
	void UpdateDisplayTree();
	// Generated message map functions
protected:
	//{{AFX_MSG(CvSdaLeft)
	afx_msg void OnItemexpanded(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnItemexpanding(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnPopupDiscard();
	afx_msg void OnPopupUndiscard();
	afx_msg void OnSelchanging(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnPopupAccessVdba();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VIEWLEFT_H__CC2DA2C8_B8F1_11D6_87D8_00C04F1F754A__INCLUDED_)
