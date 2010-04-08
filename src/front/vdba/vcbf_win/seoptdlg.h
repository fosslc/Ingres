/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : seoptdlg.h, Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : Modeless Dialog, Page (Options) of Security                           //
//               See the CONCEPT.DOC for more detail                                   //
****************************************************************************************/
#if !defined(SEOPTDLG_HEADER)
#define SEOPTDLG_HEADER

// seoptdlg.h : header file
//
#include "editlsgn.h"

/////////////////////////////////////////////////////////////////////////////
// CuDlgSecurityOption dialog

class CuDlgSecurityOption : public CDialog
{
public:
	CuDlgSecurityOption(CWnd* pParent = NULL);
	void OnOK() {return;}
	void OnCancel() {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgSecurityOption)
	enum { IDD = IDD_SECURITY_PAGE_OPTION };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgSecurityOption)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	CImageList m_ImageListCheck;
	CImageList m_ImageList;
	CuEditableListCtrlGeneric m_ListCtrl;

	// Generated message map functions
	//{{AFX_MSG(CuDlgSecurityOption)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LONG OnButton1   (UINT wParam, LONG lParam);     // Edit Value	
	afx_msg LONG OnButton2   (UINT wParam, LONG lParam);     // Default
	afx_msg LONG OnButton3   (UINT wParam, LONG lParam);     // Not Used
	afx_msg LONG OnButton4   (UINT wParam, LONG lParam);     // Not Used
	afx_msg LONG OnButton5   (UINT wParam, LONG lParam);     // Not Used
	afx_msg LONG OnUpdateData(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};

#endif
