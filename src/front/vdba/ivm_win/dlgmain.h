/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : dlgmain.h , Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II / Visual Manager                                            //
//    Author   : Sotheavut UK (uk$so01)                                                //
//                                                                                     //
//                                                                                     //
//    Purpose  : main dialog                                                           //
****************************************************************************************/
/* History:
* --------
* uk$so01: (14-Dec-1998 created)
*
*
*/

#if !defined(AFX_DLGMAIN_H__63A2E055_936D_11D2_A2B5_00609725DDAF__INCLUDED_)
#define AFX_DLGMAIN_H__63A2E055_936D_11D2_A2B5_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "mgrfrm.h"

class CuDlgMain : public CDialog
{
public:
	CuDlgMain(CWnd* pParent = NULL);
	void OnOK() {return;}
	void OnCancel() {return;}
	CfManagerFrame* GetManagerFrame(){return m_pManagerFrame;}

	// Dialog Data
	//{{AFX_DATA(CuDlgMain)
	enum { IDD = IDD_MAIN };
	CAnimateCtrl	m_cAnimate1;
	CStatic	m_cStaticIISystem;
	CStatic	m_cStaticFrame;
	CString	m_strInstallation;
	CString	m_strHost;
	CString	m_strIISystem;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgMain)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void StartAnimation();
	void StopAnimation();
	UINT m_nStartCount;
	BOOL m_bAnimationStarted;
	CfManagerFrame* m_pManagerFrame;
	// Generated message map functions
	//{{AFX_MSG(CuDlgMain)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LONG OnStartStop (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGMAIN_H__63A2E055_936D_11D2_A2B5_00609725DDAF__INCLUDED_)
