/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : brdpadlg.h, Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : Parameter Page of Bridge                          .                   //
****************************************************************************************/
#if !defined(AFX_BRDPADLG_H__66106CA2_3E27_11D1_A202_00609725DDAF__INCLUDED_)
#define AFX_BRDPADLG_H__66106CA2_3E27_11D1_A202_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// brdpadlg.h : header file
//
#include "editlsgn.h"

/////////////////////////////////////////////////////////////////////////////
// CuDlgBridgeParameter dialog

class CuDlgBridgeParameter : public CDialog
{
public:
	CuDlgBridgeParameter(CWnd* pParent = NULL);   // standard constructor
	void OnOK() {return;}
	void OnCancel() {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgBridgeParameter)
	enum { IDD = IDD_BRIDGE_PAGE_PARAMETER };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgBridgeParameter)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CImageList m_ImageList;
	CImageList m_ImageListCheck;
	CuEditableListCtrlGeneric m_ListCtrl;

	// Generated message map functions
	//{{AFX_MSG(CuDlgBridgeParameter)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LONG OnButton1      (UINT wParam, LONG lParam);     // Edit Value	
	afx_msg LONG OnButton2      (UINT wParam, LONG lParam);     // Default
	afx_msg LONG OnButton3      (UINT wParam, LONG lParam);     // Not Used
	afx_msg LONG OnButton4      (UINT wParam, LONG lParam);     // Not Used
	afx_msg LONG OnButton5      (UINT wParam, LONG lParam);     // Not Used
	afx_msg LONG OnUpdateData   (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnValidateData (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BRDPADLG_H__66106CA2_3E27_11D1_A202_00609725DDAF__INCLUDED_)
