/******************************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**   Source   : xdlgwait.h, Header file
**
**
**    Project  : Ingres II/VDBA
**    Author   : UK Sotheavut
**
**
**   Purpose  : Allow interrupt task
**
**  History:
** 
**  20-Mar-2001 (noifr01)
**    (bug 104246) disabled constructor of  CxDlgWait that was resulting in
**    launching the old-style animation
*******************************************************************************/
#if !defined(AFX_XDLGWAIT_H__56937720_287A_11D2_A295_00609725DDAF__INCLUDED_)
#define AFX_XDLGWAIT_H__56937720_287A_11D2_A295_00609725DDAF__INCLUDED_
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "resmfc.h"
#include "taskxprm.h"



class CxDlgWait : public CDialog
{
public:
	enum CenterType {OPEN_AT_MOUSE, OPEN_CENTER_AT_MOUSE};
#if 0
	CxDlgWait(CWnd* pParent = NULL);
#endif
	virtual int DoModal();
	//
	// The second constructor must be called with lpszCaption not null.
	// NOTE: IDD_XTASK_WAIT_WITH_TITLE is a duplicated DLG ID from IDD_XTASK_WAIT.
	//       It has title, ... (I canot use neither ModifyStyle() nor SetWindowLong()
	//       to change the dialog style on creation !!!.
	CxDlgWait(LPCTSTR lpszCaption = NULL, CWnd* pParent = NULL);

	void SetUseAnimateIcon(){m_bUseAnimateIcon = TRUE;}
	void SetUseAnimateAvi (int nAviType);
	//
	// Specify the WIDTH and HEIGHT of a dialog box (in pixels)
	void SetSize (int nWidth, int nHeight);
	//
	// Specify the WIDTH and HEIGHT of a dialog box:
	// (percentage of the original size, dpWidth and dpHeight must be in ]0, 1])
	void SetSize2(double dpWidth = 1.0, double dpHeight = 1.0);
	//
	// Show the static control of two lines of text under the animate control:
	// The text can be specified using the member 'm_strStaticText1':
	void SetUseExtraInfo(){m_bUseExtraInfo = TRUE;}
	//
	// Force the dialog to open and center at the mouse pointer:
	void CenterDialog(CenterType nType = OPEN_AT_MOUSE)
	{
		m_bCencerAtMouse = TRUE;
		m_nCenterType    = nType;
	}

	void SetExecParam (CaExecParam* pExecParam)
	{
		m_pExecParam = pExecParam;
		m_bSetParam = TRUE;
	}
	CaExecParam* GetExecParam ()
	{
		ASSERT (m_bSetParam);
		ASSERT (m_pExecParam);
		return m_pExecParam;
	}

	UINT ThreadProcControlS1 (LPVOID pParam);
	UINT ThreadProcControlS2 (LPVOID pParam);

	CWinThread* m_pThread1;
	CWinThread* m_pThread2;
	HANDLE      m_hMutexT1;
	HANDLE      m_hMutexT2;
	HANDLE      m_hMutexV1;

	// Dialog Data
	//{{AFX_DATA(CxDlgWait)
	enum { IDD = IDD_XTASK_WAIT };
	CButton	m_cButtonCancel;
	CStatic	m_cStaticImage1;
	CStatic	m_cStaticText1;
	CAnimateCtrl	m_cAnimate1;
	CString	m_strStaticText1;
	//}}AFX_DATA

	int  m_nReturnCode;
	BOOL m_bDeleteExecParam;
	//
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgWait)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	BOOL   m_bFinish;
	BOOL   m_bActionSetWindowPos;
	BOOL   m_bRatio;       // TRUE if specify the new size in percentage of the original one
	double m_dXratio;
	double m_dYratio;
	BOOL   m_bSpecifySize; // TRUE if the size of Dlg is given from the outside
	CSize  m_sizeDlg;
	CSize  m_sizeDlgMin;

	CSize  m_sizeDown;
	BOOL   m_bSetParam;
	CaExecParam* m_pExecParam;
	HANDLE m_hEventTerminateComplete;

	BOOL  m_bUseAnimateIcon;
	BOOL  m_bUseExtraInfo;
	UINT  m_nTimer1;
	HICON m_hIcon;
	HICON m_hIconCounter [16];
	int   m_nCounter;
	int   m_nAviType;

	CString m_strCaption;
	BOOL  m_bShowTitle;
	BOOL  m_bCencerAtMouse;
	CenterType m_nCenterType;

	void ResizeControls();
	void Init();
	// Generated message map functions
	//{{AFX_MSG(CxDlgWait)
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG

	//
	// wParam = 0, interrupt. lParam = IDCANCEL or IDOK
	//        = 1, Display extra information. lParam = null terminate string.
	afx_msg LONG OnExecuteTask (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XDLGWAIT_H__56937720_287A_11D2_A295_00609725DDAF__INCLUDED_)

