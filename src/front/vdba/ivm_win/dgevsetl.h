/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : dgevsetl.h , Header File                                              //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II / Visual Manager                                            //
//    Author   : Sotheavut UK (uk$so01)                                                //
//                                                                                     //
//                                                                                     //
//    Purpose  : Modeless Dialog of the left pane of Event Setting Frame               //
****************************************************************************************/
/* History:
* --------
* uk$so01: (21-May-1999 created)
*
*
*/

#if !defined(AFX_DGEVSETL_H__E85F2D45_1373_11D3_A2EE_00609725DDAF__INCLUDED_)
#define AFX_DGEVSETL_H__E85F2D45_1373_11D3_A2EE_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "msgntree.h"
#include "msgtreec.h"

class CuDlgEventSettingLeft : public CDialog
{
public:
	CuDlgEventSettingLeft(CWnd* pParent = NULL);
	void OnOK() {return;}
	void OnCancel() {return;}
	CTreeCtrl* GetTreeCtrl(){return &m_cTree1;}

	// Dialog Data
	//{{AFX_DATA(CuDlgEventSettingLeft)
	enum { IDD = IDD_EVENT_SETTING_LEFT };
	CButton	m_cCheckShowCategories;
	CuMessageCategoryTreeCtrl	m_cTree1;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgEventSettingLeft)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
//	CaTreeMessageState m_treeStateView;
	CImageList m_ImageList;

	// Generated message map functions
	//{{AFX_MSG(CuDlgEventSettingLeft)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnCheckShowCategories();
	//}}AFX_MSG
	afx_msg LONG OnDisplayTree (WPARAM w, LPARAM l);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DGEVSETL_H__E85F2D45_1373_11D3_A2EE_00609725DDAF__INCLUDED_)
