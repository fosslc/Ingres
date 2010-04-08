/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : dgevsetr.h , Header File                                              //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II / Visual Manager                                            //
//    Author   : Sotheavut UK (uk$so01)                                                //
//                                                                                     //
//                                                                                     //
//    Purpose  : Modeless Dialog of the right pane of Event Setting Frame              //
****************************************************************************************/
/* History:
* --------
* uk$so01: (21-May-1999 created)
*
*
*/

#if !defined(AFX_DGEVSETR_H__E85F2D46_1373_11D3_A2EE_00609725DDAF__INCLUDED_)
#define AFX_DGEVSETR_H__E85F2D46_1373_11D3_A2EE_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "msgntree.h"
#include "msgtreec.h"

class CuDlgEventSettingRight : public CDialog
{
public:
	CuDlgEventSettingRight(CWnd* pParent = NULL);
	void OnOK() {return;}
	void OnCancel() {return;}
	CTreeCtrl* GetTreeCtrl(){return &m_cTree1;}

	// Dialog Data
	//{{AFX_DATA(CuDlgEventSettingRight)
	enum { IDD = IDD_EVENT_SETTING_RIGHT };
	CButton	m_cCheckShowStateBranches;
	CuMessageCategoryTreeCtrl	m_cTree1;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgEventSettingRight)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
//	CaCategoryData   tttx;
//	CaMessageMagager ttch;
//	CaCategoryDataUserManager ttmm;

//	CaTreeMessageCategory m_treeCategoryView;
	CImageList m_ImageList;

	// Generated message map functions
	//{{AFX_MSG(CuDlgEventSettingRight)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnCheckShowStateBranches();
	//}}AFX_MSG

	afx_msg LONG OnDisplayTree (WPARAM w, LPARAM l);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DGEVSETR_H__E85F2D46_1373_11D3_A2EE_00609725DDAF__INCLUDED_)
