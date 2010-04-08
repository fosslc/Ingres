/****************************************************************************************
//                                                                                     //
//  Copyright (C) November, 1998 Computer Associates International, Inc.               //
//                                                                                     //
//    Source   : dgbkrefr.h, Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II/Vdba.                                                       //
//    Author   : UK Sotheavut                       .                                  //
//                                                                                     //
//    Purpose  : Modeless Dialog Box for Background Refresh Info                       //
****************************************************************************************/
#if !defined(AFX_DGBKREFR_H__8A3C9921_7E27_11D2_A2AF_00609725DDAF__INCLUDED_)
#define AFX_DGBKREFR_H__8A3C9921_7E27_11D2_A2AF_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CuDlgBackgroundRefresh : public CDialog
{
public:
	CuDlgBackgroundRefresh(CWnd* pParent = NULL);

	// Dialog Data
	//{{AFX_DATA(CuDlgBackgroundRefresh)
	enum { IDD = IDD_BACKGROUND_REFRESH };
	CAnimateCtrl	m_cAnimate1;
	CString	m_strStatic1;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgBackgroundRefresh)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	BOOL m_bAnimateOK;

	// Generated message map functions
	//{{AFX_MSG(CuDlgBackgroundRefresh)
	virtual BOOL OnInitDialog();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	//}}AFX_MSG
	afx_msg LONG OnUpdateInfo (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DGBKREFR_H__8A3C9921_7E27_11D2_A2AF_00609725DDAF__INCLUDED_)
