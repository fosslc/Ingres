/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : viewrigh.h , Header File
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : MFC (Frame View Doc) CView of Right Pane that contains the Dialogs
**
** History:
**
** 22-Dec-1999 (uk$so01)
**    Created
** 22-Dec-2000 (noifr01)
**    (SIR 103548) show the value of II_INSTALLATION in the title
** 21-Nov-2003 (uk$so01)
**    SIR #99596, Use the aboutbox provided by ijactrl.ocx. and remove
**    the local about.bmp.
** 23-Sep-2004 (uk$so01)
**    BUG #113114 
**    Force the SendMessageToDescendants(WM_SYSCOLORCHANGE) to reflect 
**    immediately the system color changed.
**/

#if !defined(AFX_VIEWRIGH_H__DA2AADA7_AF05_11D3_A322_00C04F1F754A__INCLUDED_)
#define AFX_VIEWRIGH_H__DA2AADA7_AF05_11D3_A322_00C04F1F754A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "ijactrl.h"



class CvViewRight : public CView
{
protected:
	CvViewRight();
	DECLARE_DYNCREATE(CvViewRight)

public:
	void OnAbout();
	CIjaCtrl* m_pCtrl;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CvViewRight)
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	virtual ~CvViewRight();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	CString m_csii_installation;

	// Generated message map functions
protected:
	//{{AFX_MSG(CvViewRight)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnSysColorChange();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VIEWRIGH_H__DA2AADA7_AF05_11D3_A322_00C04F1F754A__INCLUDED_)
