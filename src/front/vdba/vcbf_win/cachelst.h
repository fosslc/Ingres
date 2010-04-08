/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : cachelst.h, Header file                                               //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : Special Owner Draw List Control                                       //
//               Modeless dialog - left pane of page (DBMS Cache)                      //
****************************************************************************************/
#if !defined(AFX_CACHELST_H__6722F081_3B35_11D1_A201_00609725DDAF__INCLUDED_)
#define AFX_CACHELST_H__6722F081_3B35_11D1_A201_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// cachelst.h : header file
//
#include "calsctrl.h"

class CuCacheCheckListCtrl: public CuListCtrl
{
public:
	CuCacheCheckListCtrl();
	// Overrides
	//
	virtual ~CuCacheCheckListCtrl();

	virtual void OnCheckChange (int iItem, int iSubItem, BOOL bCheck);

	//
	// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CuDlgCacheList dialog

class CuDlgCacheList : public CDialog
{
// Construction
public:
	CuDlgCacheList(CWnd* pParent = NULL);   
	CuCacheCheckListCtrl* GetCacheListCtrl() {return &m_ListCtrl;}
	void OnOK() {return;}
	void OnCancel() {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgCacheList)
	enum { IDD = IDD_CACHE_LIST };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgCacheList)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CImageList  m_ImageList;
	CImageList  m_ImageListCheck;
	CuCacheCheckListCtrl m_ListCtrl;

	// Generated message map functions
	//{{AFX_MSG(CuDlgCacheList)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	//
	// Generic button: You must spesify the action of what you expected !!!
	afx_msg LONG OnButton1 (UINT wParam, LONG lParam);
	afx_msg LONG OnButton2 (UINT wParam, LONG lParam);
	afx_msg LONG OnButton3 (UINT wParam, LONG lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CACHELST_H__6722F081_3B35_11D1_A201_00609725DDAF__INCLUDED_)
