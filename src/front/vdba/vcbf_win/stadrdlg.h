/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : stadrdlg.h, Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : Modeless Dialog, Page (Derived) of Star                               //
//               See the CONCEPT.DOC for more detail                                   //
****************************************************************************************/
#if !defined(STADRDLG_HEADER)
#define STADRDLG_HEADER

// stadrdlg.h : header file
//
#include "editlsdp.h"

/////////////////////////////////////////////////////////////////////////////
// CuDlgStarDerived dialog

class CuDlgStarDerived : public CDialog
{
public:
	CuDlgStarDerived(CWnd* pParent = NULL);   // standard constructor

	void OnOK(){return;}
	void OnCancel(){return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgStarDerived)
	enum { IDD = IDD_STAR_PAGE_DERIVED };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgStarDerived)
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
	//{{AFX_MSG(CuDlgStarDerived)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
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


#endif 