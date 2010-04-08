/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : recdrdlg.h, Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : Modeless Dialog, Page (Derived) of Recovery                           //
//               See the CONCEPT.DOC for more detail                                   //
****************************************************************************************/
#if !defined(AFX_RECDRDLG_H__E51FBA53_328E_11D2_A29A_00609725DDAF__INCLUDED_)
#define AFX_RECDRDLG_H__E51FBA53_328E_11D2_A29A_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// recdrdlg.h : header file
//

class CuDlgRecoveryDerived : public CDialog
{
public:
	CuDlgRecoveryDerived(CWnd* pParent = NULL);
	void OnOK(){return;}
	void OnCancel(){return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgRecoveryDerived)
	enum { IDD = IDD_RECOVERY_PAGE_DERIVED };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgRecoveryDerived)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	CImageList m_ImageList;
	CImageList m_ImageListCheck;
	CuEditableListCtrlGenericDerived m_ListCtrl;

	// Generated message map functions
	//{{AFX_MSG(CuDlgRecoveryDerived)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LONG OnButton1   (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnButton2   (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnButton3   (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnButton4   (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnButton5   (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnUpdateData   (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnValidateData (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RECDRDLG_H__E51FBA53_328E_11D2_A29A_00609725DDAF__INCLUDED_)
