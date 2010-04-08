/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qreview.h, Header file 
**    Project  : CA-OpenIngres Visual DBA.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : View window (CRichEditView that can format the text)
**
** History:
**
** xx-Oct-1997 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
*/

#if !defined(AFX_QREVIEW_H__3053D92A_4EEA_11D1_A214_00609725DDAF__INCLUDED_)
#define AFX_QREVIEW_H__3053D92A_4EEA_11D1_A214_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "qlowview.h"


class CvSqlQueryRichEditView : public CRichEditView
{
protected: 
	CvSqlQueryRichEditView();
	DECLARE_DYNCREATE(CvSqlQueryRichEditView)

public:

	void SetColor  (LONG nStart, LONG nEnd, COLORREF crColor);
	void SetHighlight (BOOL bHighlight = TRUE);
	BOOL GetHighlight ();
	void OnCancel(){return;}
	void OnOK(){return;}
	virtual HRESULT QueryAcceptData(LPDATAOBJECT lpdataobj, CLIPFORMAT* lpcfFormat, DWORD dwReco, BOOL bReally, HGLOBAL hMetaPict);


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CvSqlQueryRichEditView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

	//
	// Implementation
public:
	virtual ~CvSqlQueryRichEditView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	CvSqlQueryLowerView* m_pLowerView; 

private:

	// Generated message map functions
	//{{AFX_MSG(CvSqlQueryRichEditView)
	afx_msg void OnChange();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnDestroy();
	afx_msg void OnEditPaste();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnEditCopy();
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnEditCut();
	afx_msg void OnEditUndo();
	//}}AFX_MSG
	afx_msg long OnChangeSetting(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QREVIEW_H__3053D92A_4EEA_11D1_A214_00609725DDAF__INCLUDED_)
