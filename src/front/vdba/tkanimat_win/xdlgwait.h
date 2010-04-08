/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xdlgwait.h, Header file 
**    Project  : Extension DLL (Task Animation).
**    Author   : Sotheavut UK (uk$so01)
**
**    Purpose  : Dialog that shows the progression of Task (Interruptible)
**
** History:
**
** xx-Aug-1998 (uk$so01)
**    created
** 16-Mar-2001 (uk$so01)
**    Changed to be an Extension DLL
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
** 30-Jan-2002 (uk$so01)
**    SIR #106648, Enhance the dll. Add new Thread to control the thread that
**    does the job.
*/

#if !defined(AFX_XDLGWAIT_H__56937720_287A_11D2_A295_00609725DDAF__INCLUDED_)
#define AFX_XDLGWAIT_H__56937720_287A_11D2_A295_00609725DDAF__INCLUDED_
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "resource.h"
#include "tkwait.h"



class CxDlgWaitTask : public CDialog
{
public:
	//
	// The second constructor must be called with lpszCaption not null.
	// NOTE: IDD_XTASK_WAIT_WITH_TITLE is a duplicated DLG ID from IDD_XTASK_WAIT.
	//       It has title, ... (I canot use neither ModifyStyle() nor SetWindowLong()
	//       to change the dialog style on creation !!!.
	CxDlgWaitTask(LPCTSTR lpszCaption, CWnd* pParent = NULL);
	virtual int DoModal();

	void SetUseAnimateAvi (int nAviType);
	//
	// Show the static control of two lines of text under the animate control:
	// The text can be specified using the member 'm_strStaticText1':
	void SetUseExtraInfo(){m_bUseExtraInfo = TRUE;}
	//
	// Force the dialog to open and center at the mouse pointer:
	void CenterDialog(CxDlgWait::CenterType nType = CxDlgWait::OPEN_AT_MOUSE)
	{
		m_bCencerAtMouse = TRUE;
		m_nCenterType    = nType;
	}

	void SetExecParam (CaExecParam* pExecParam){m_pExecParam = pExecParam;}
	CaExecParam* GetExecParam (){return m_pExecParam;}
	void SetDeleteParam(BOOL bSet){m_bDeleteExecParam = bSet;}
	void SetShowProgressBar(BOOL bShow){m_bShowProgressCtrl = bShow;}
	void SetHideDialogWhenTerminate(BOOL bHide){m_bHideAnimateDlgWhenTerminate = bHide;}
	void SetHideCancelBottonAlways(BOOL bSet){m_bHideCancelButtonAlways = bSet;}


	CWinThread* m_pThread1;

	// Dialog Data
	//{{AFX_DATA(CxDlgWaitTask)
	enum { IDD = IDD_XTASK_WAIT };
	CButton	m_cButtonCancel;
	CStatic	m_cStaticText1;
	CAnimateCtrl	m_cAnimate1;
	CString	m_strStaticText1;
	//}}AFX_DATA
	CProgressCtrl m_cProgress;
	//
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgWaitTask)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	BOOL m_bHideAnimateDlgWhenTerminate;
	BOOL m_bShowProgressCtrl;
	BOOL m_bAlreadyInterrupted;
	BOOL m_bUseAnimateIcon;
	BOOL m_bUseExtraInfo;
	BOOL m_bHideCancelButtonAlways;
	int  m_nAviType;
	CaExecParam* m_pExecParam;
	BOOL m_bDeleteExecParam;


	void ResizeControls();
	void Init();
	// Generated message map functions
	//{{AFX_MSG(CxDlgWaitTask)
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG

	//
	// wParam = 0, interrupt. lParam = IDCANCEL or IDOK
	//        = 1, Display extra information. lParam = null terminate string.
	afx_msg LONG OnExecuteTask (WPARAM wParam, LPARAM lParam);

private:
	CString m_strCaption;
	BOOL  m_bShowTitle;
	BOOL  m_bCencerAtMouse;
	CxDlgWait::CenterType m_nCenterType;
	BOOL m_bBeingCancel;

	DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XDLGWAIT_H__56937720_287A_11D2_A295_00609725DDAF__INCLUDED_)

