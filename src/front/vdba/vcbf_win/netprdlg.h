/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : netprdlg.h, Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//               Emmanuel Blatte (Custom implementations)                              //
//                                                                                     //
//    Purpose  : Modeless Dialog, Page (Parameter) of NET Server                       //
//               See the CONCEPT.DOC for more detail                                   //
****************************************************************************************/
#if !defined(NETPROTOCOLDLG_HEADER)
#define NETPROTOCOLDLG_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// netprdlg.h : header file
//

#include  "editlnet.h"  // custom list control for net
#include  "ll.h"

/////////////////////////////////////////////////////////////////////////////
// CuDlgNetProtocol dialog

class CuDlgNetProtocol : public CDialog
{
private:
  void ResetList();

public:
	CuDlgNetProtocol(CWnd* pParent = NULL);

	void OnOK() {return;}
	void OnCancel() {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgNetProtocol)
	enum { IDD = IDD_NET_PAGE_PROTOCOL };
	CuEditableListCtrlNet	m_ListCtrl;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgNetProtocol)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
protected:
	BOOL AddProtocol(LPNETPROTLINEINFO lpProtocol);
	CImageList  m_ImageList;
	CImageList  m_ImageListCheck;

	// Generated message map functions
	//{{AFX_MSG(CuDlgNetProtocol)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnValidateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnEditListenAddress (UINT wParam, LONG lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PARAMDLG_H__05328425_2516_11D1_A1EE_00609725DDAF__INCLUDED_)
