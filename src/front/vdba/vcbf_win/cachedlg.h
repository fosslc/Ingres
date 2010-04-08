/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : cachedlg.h, Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : Modeless Dialog-DBMS Cache page                                       //
****************************************************************************************/

#if !defined(AFX_CACHEDLG_H__05328422_2516_11D1_A1EE_00609725DDAF__INCLUDED_)
#define AFX_CACHEDLG_H__05328422_2516_11D1_A1EE_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// cachedlg.h : header file
//
#include "cachefrm.h"	// Frame support for the splitter
#include "cachevi1.h"	// View CvDbmsCacheViewLeft
#include "cachevi2.h"	// View CvDbmsCacheViewRight


/////////////////////////////////////////////////////////////////////////////
// CuDlgDbmsCache dialog

class CuDlgDbmsCache : public CDialog
{
protected:

public:
	CuDlgDbmsCache(CWnd* pParent = NULL);
	void OnOK() {return;}
	void OnCancel() {return;}

	CvDbmsCacheViewLeft*  GetLeftPane (); 
	CvDbmsCacheViewRight* GetRightPane();
	// Dialog Data
	//{{AFX_DATA(CuDlgDbmsCache)
	enum { IDD = IDD_DBMS_PAGE_CACHE };
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDbmsCache)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	CfDbmsCacheFrame* m_pCacheFrame;

	// Generated message map functions
	//{{AFX_MSG(CuDlgDbmsCache)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData   (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnValidateData (WPARAM wParam, LPARAM lParam);
	//
	// Generic button: You must spesify the action of what you expected !!!
	afx_msg LONG OnButton1 (UINT wParam, LONG lParam);
	afx_msg LONG OnButton2 (UINT wParam, LONG lParam);
	afx_msg LONG OnButton3 (UINT wParam, LONG lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CACHEDLG_H__05328422_2516_11D1_A1EE_00609725DDAF__INCLUDED_)
