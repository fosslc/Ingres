/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : vevsettb.h , Header File                                              //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II / Visual Manager                                            //
//    Author   : Sotheavut UK (uk$so01)                                                //
//                                                                                     //
//                                                                                     //
//    Purpose  : View of the bottom pane of Event Setting Frame                        //
****************************************************************************************/
/* History:
* --------
* uk$so01: (21-May-1999 created)
*
*
*/

#if !defined(AFX_VEVSETTB_H__0A2EF7D5_129C_11D3_A2EE_00609725DDAF__INCLUDED_)
#define AFX_VEVSETTB_H__0A2EF7D5_129C_11D3_A2EE_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "dgevsetb.h"

/////////////////////////////////////////////////////////////////////////////
// CvEventSettingBottom view

class CvEventSettingBottom : public CView
{
protected:
	CvEventSettingBottom();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CvEventSettingBottom)

public:
	CListCtrl* GetListCtrl(){return m_pDlg? m_pDlg->GetListCtrl(): NULL;}
	CuDlgEventSettingBottom* GetDialog(){return m_pDlg;}

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CvEventSettingBottom)
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CuDlgEventSettingBottom* m_pDlg;

	virtual ~CvEventSettingBottom();
	

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CvEventSettingBottom)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VEVSETTB_H__0A2EF7D5_129C_11D3_A2EE_00609725DDAF__INCLUDED_)
