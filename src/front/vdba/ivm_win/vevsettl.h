/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : vevsettl.h , Header File                                              //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II / Visual Manager                                            //
//    Author   : Sotheavut UK (uk$so01)                                                //
//                                                                                     //
//                                                                                     //
//    Purpose  : View of the left pane of Event Setting Frame                          //
****************************************************************************************/
/* History:
* --------
* uk$so01: (21-May-1999 created)
*
*
*/

#if !defined(AFX_VEVSETTL_H__0A2EF7D3_129C_11D3_A2EE_00609725DDAF__INCLUDED_)
#define AFX_VEVSETTL_H__0A2EF7D3_129C_11D3_A2EE_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "dgevsetl.h"


/////////////////////////////////////////////////////////////////////////////
// CvEventSettingLeft view

class CvEventSettingLeft : public CView
{
protected:
	CvEventSettingLeft();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CvEventSettingLeft)

public:

	CTreeCtrl* GetTreeCtrl(){return m_pDlg? m_pDlg->GetTreeCtrl(): NULL;}
	CuDlgEventSettingLeft* GetDialog(){return m_pDlg;}

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CvEventSettingLeft)
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL

	// Implementation
protected:
	virtual ~CvEventSettingLeft();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	CuDlgEventSettingLeft* m_pDlg;


	// Generated message map functions
protected:
	//{{AFX_MSG(CvEventSettingLeft)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VEVSETTL_H__0A2EF7D3_129C_11D3_A2EE_00609725DDAF__INCLUDED_)
