/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xdlgwait.cpp, Implementation file 
**    Project  : Extension DLL (Task Animation). 
**    Author   : Sotheavut UK (uk$so01)
**
**    Purpose  : Dialog that shows the progression of Task (Interruptible)
**
** History:
** xx-Aug-1998 (uk$so01)
**    created
** 16-Mar-2001 (uk$so01)
**    Changed to be an Extension DLL
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 30-Jan-2002 (uk$so01)
**    SIR #106648, Enhance the dll. Add new Thread to control the thread that
**    does the job.
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
*/


#include "stdafx.h"
#include "resource.h"
#include "xdlgwait.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

typedef struct tagTHREADPARAM
{
	LPVOID pParam;
	HWND   hWndDlgWait;

} THREADPARAM;


//
// Proc Thread Control that run CaExecParam::Run():
UINT ThreadControlExecParam(LPVOID pParam)
{
	THREADPARAM* pObj = (THREADPARAM*)pParam;
	ASSERT(pObj);
	if (!pObj)
		return 1;
	CaExecParam* pExecParam = (CaExecParam*)pObj->pParam;
	ASSERT(pExecParam);
	if (!pExecParam)
	{
		delete pObj;
		return 1;
	}

	//
	// Begin of Execution the statement:
	int nRun = pExecParam->Run(pObj->hWndDlgWait);

	if (pObj->hWndDlgWait && IsWindow (pObj->hWndDlgWait))
	{
		::PostMessage (pObj->hWndDlgWait, WM_EXECUTE_TASK, (WPARAM)W_STARTSTOP_ANIMATION, 0);
		Sleep(100);
		::PostMessage (pObj->hWndDlgWait, WM_EXECUTE_TASK, (WPARAM)W_TASK_TERMINATE, (LPARAM)nRun);
	}

	delete pObj;
	return nRun;
}

UINT ThreadControlTerminateExecParam(LPVOID pParam)
{
	THREADPARAM* pObj = (THREADPARAM*)pParam;
	ASSERT(pObj);
	if (!pObj)
		return 1;
	CWinThread* pThread = (CWinThread*)pObj->pParam;
	ASSERT(pThread);
	if (!pThread)
	{
		delete pObj;
		return 1;
	}
	HWND hWndDlgWait = pObj->hWndDlgWait;
	ASSERT(hWndDlgWait);
	if (hWndDlgWait)
	{
		DWORD dwWaitResult = WaitForSingleObject (pThread->m_hThread, INFINITE);
		::PostMessage (hWndDlgWait, WM_EXECUTE_TASK, (WPARAM)W_STARTSTOP_ANIMATION, 0);
		Sleep(100);
		::PostMessage (hWndDlgWait, WM_EXECUTE_TASK, (WPARAM)W_TASK_TERMINATE, (LPARAM)0);
	}

	delete pObj;
	return 0;
}

void CxDlgWaitTask::SetUseAnimateAvi (int nAviType)
{
	m_bUseAnimateIcon = FALSE;
	m_nAviType = nAviType;
}

void CxDlgWaitTask::Init()
{
	m_bHideAnimateDlgWhenTerminate = TRUE;
	m_bShowProgressCtrl = FALSE;
	m_bUseExtraInfo    = FALSE;
	m_bUseAnimateIcon  = TRUE;
	m_nAviType = IDR_ANCLOCK;
	m_pExecParam = NULL;
	m_bDeleteExecParam = FALSE;

	m_bCencerAtMouse = FALSE;
	m_nCenterType = CxDlgWait::OPEN_AT_MOUSE;
	//
	// Not has been interrupted:
	m_bAlreadyInterrupted = FALSE;

	m_bHideCancelButtonAlways = FALSE;
}



CxDlgWaitTask::CxDlgWaitTask(LPCTSTR lpszCaption, CWnd* pParent)
	: CDialog(IDD_XTASK_WAIT_WITH_TITLE, pParent)
{
	ASSERT (lpszCaption != NULL);
	m_strCaption = lpszCaption? lpszCaption: _T("");
	m_bShowTitle = TRUE;
	m_strStaticText1 = _T("");
	m_bBeingCancel = FALSE;

	Init();
}



int CxDlgWaitTask::DoModal()
{
	//
	// You should call the CxDlgWait::SetExecParam() first.
	ASSERT(m_pExecParam);
	if (!m_pExecParam)
		return IDCANCEL;

	//
	// Display the dialog immediately before running the Thread that
	// call m_pExecParam->Run():
	if (m_pExecParam && m_pExecParam->IsExecuteImmediately())
	{
		return CDialog::DoModal();
	}
	else
	{
		//
		// Run the Thread that calls m_pExecParam->Run().
		// Wait for a given elapse time Tm.
		// If after that time Tm and the Thread is still alive, we will show the dialog box.
		// Otherwise we return immediately.

		THREADPARAM* pObj = new THREADPARAM;
		pObj->hWndDlgWait = NULL;
		pObj->pParam = m_pExecParam;
		m_pThread1 = AfxBeginThread((AFX_THREADPROC)ThreadControlExecParam, pObj, THREAD_PRIORITY_LOWEST);

		DWORD dwWaitResult = WaitForSingleObject (m_pThread1->m_hThread, m_pExecParam->GetDelayExecution());
		switch (dwWaitResult)
		{
		case WAIT_OBJECT_0:
			if (m_bDeleteExecParam)
			{
				if (m_pExecParam)
					delete m_pExecParam;
				m_pExecParam = NULL;
			}
			return IDOK;
			break;

		case WAIT_FAILED:
		case WAIT_ABANDONED:
			if (m_bDeleteExecParam)
			{
				if (m_pExecParam)
					delete m_pExecParam;
				m_pExecParam = NULL;
			}
			return IDCANCEL;

		default:
			return CDialog::DoModal();
			break;
		}
	
		return CDialog::DoModal();
	}
}


void CxDlgWaitTask::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgWaitTask)
	DDX_Control(pDX, IDCANCEL, m_cButtonCancel);
	DDX_Control(pDX, IDC_STATIC1, m_cStaticText1);
	DDX_Control(pDX, IDC_ANIMATE1, m_cAnimate1);
	DDX_Text(pDX, IDC_STATIC1, m_strStaticText1);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_PROGRESS1, m_cProgress);
}


BEGIN_MESSAGE_MAP(CxDlgWaitTask, CDialog)
	//{{AFX_MSG_MAP(CxDlgWaitTask)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_EXECUTE_TASK,   OnExecuteTask)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgWaitTask message handlers


//
// The IIbreak() never returns (bug ?)
// I have to create a new thread executing IIbreak and SUPPOSE that
// 1 second should be enought to execute IIbreak, and after that I
// can kill the thread:
void CxDlgWaitTask::OnCancel() 
{
	DWORD dwWaitResult = 0;
	ASSERT(m_pExecParam);
	if (!m_pExecParam)
		return;
	if (m_bBeingCancel)
		return;

	//
	// Check to see if Interrupt is allowed
	UINT nInterruptType = m_pExecParam->GetInterruptType();
	if (nInterruptType == INTERRUPT_NOT_ALLOWED || m_bAlreadyInterrupted)
	{
		MessageBeep ((UINT)-1);
		return;
	}

	m_bBeingCancel = TRUE;
	switch (nInterruptType)
	{
	case INTERRUPT_USERPRECIFY:
		m_cAnimate1.Stop();
		m_bAlreadyInterrupted = m_pExecParam->OnCancelled(m_hWnd);
		if (!m_bAlreadyInterrupted)
			m_cAnimate1.Play(0, 0xFFFF, (UINT)-1);
		break;
	case INTERRUPT_KILLTHREAD:
		if (m_pThread1)
		{
			m_cAnimate1.Stop();
			m_bAlreadyInterrupted = m_pExecParam->OnCancelled(m_hWnd);
			if (!m_bAlreadyInterrupted)
				m_cAnimate1.Play(0, 0xFFFF, (UINT)-1);
			if (m_bAlreadyInterrupted)
			{
				EndDialog (IDCANCEL);
			}
		}
		break;

	default:
		break;
	}

	m_bBeingCancel = FALSE;
}

BOOL CxDlgWaitTask::OnInitDialog() 
{
	CDialog::OnInitDialog();
	int nShow = m_bShowProgressCtrl? SW_SHOW: SW_HIDE;
	m_cProgress.ShowWindow(nShow);
	m_cProgress.SetPos (0);

	if (m_bUseExtraInfo)
		m_cStaticText1.ShowWindow (SW_SHOW);
	else
		m_cStaticText1.ShowWindow (SW_HIDE);

	m_cAnimate1.ShowWindow (SW_SHOW);
	
	if (m_cAnimate1.Open(m_nAviType))
		m_cAnimate1.Play(0, 0xFFFF, (UINT)-1);

	if (m_bCencerAtMouse)
	{
		POINT p;
		CRect r;
		if (GetCursorPos (&p))
		{
			GetWindowRect (r);
			if (m_nCenterType == CxDlgWait::OPEN_CENTER_AT_MOUSE)
			{
				p.x -= (r.Width()/2);
				p.y -= (r.Height()/2);
			}
			SetWindowPos (NULL, p.x, p.y, 0, 0, SWP_NOZORDER|SWP_NOSIZE|SWP_SHOWWINDOW);
		}
	}
	else
		CenterWindow();

	if (m_bShowTitle)
	{
		SetWindowText (m_strCaption);
	}
	ShowWindow (SW_SHOW);

	if (m_pExecParam)
	{
		BOOL bCenterAnimateCtrl = FALSE;
		if (m_pExecParam->GetInterruptType() != INTERRUPT_NOT_ALLOWED)
		{
			if (m_bHideCancelButtonAlways)
				m_cButtonCancel.ShowWindow (SW_HIDE);
			else
				m_cButtonCancel.ShowWindow (SW_SHOW);
			//
			// Put the cancel button in the place of Progress Control:
			if (!m_bUseExtraInfo)
				bCenterAnimateCtrl = TRUE;
		}
		else
		{
			m_cButtonCancel.ShowWindow (SW_HIDE);
			if (!m_bShowProgressCtrl && !m_bUseExtraInfo)
				bCenterAnimateCtrl = TRUE;
		}

		if (bCenterAnimateCtrl)
		{
			//
			// Center the animation control:
			CRect rDlg, rCtrl;
			GetClientRect (rDlg);
			m_cAnimate1.GetWindowRect (rCtrl);
			ScreenToClient(rCtrl);
			int nTop = rDlg.top + rDlg.Height()/2, nHeight = rCtrl.Height();
			nTop -= (nHeight/2);
			if (!m_bShowProgressCtrl)
			{
				m_cAnimate1.SetWindowPos (NULL, rCtrl.left, nTop, 0, 0, SWP_NOSIZE);
			}
			else
			{
				CRect r;
				int nBottom, nHeight = rCtrl.Height();
				m_cProgress.GetWindowRect (r);
				ScreenToClient (r);
				nBottom = r.top - 2;
				nTop = nBottom - nHeight;
				m_cAnimate1.SetWindowPos (NULL, rCtrl.left, nTop, 0, 0, SWP_NOSIZE);
			}
		}

		if (m_pExecParam->IsExecuteImmediately())
		{
			//
			// Run the Thread:
			PostMessage (WM_EXECUTE_TASK, (WPARAM)W_RUN_TASK, 0);
		}
		else
		{
			THREADPARAM* pObj = new THREADPARAM;
			pObj->hWndDlgWait = m_hWnd;
			pObj->pParam = m_pThread1;
			AfxBeginThread((AFX_THREADPROC)ThreadControlTerminateExecParam, pObj, THREAD_PRIORITY_LOWEST);
		}
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgWaitTask::OnDestroy() 
{
	if (m_bDeleteExecParam)
	{
		if (m_pExecParam)
			delete m_pExecParam;
		m_pExecParam = NULL;
	}
	CWnd* pParent = GetParent();
	if (pParent)
		pParent->SetFocus();
	CDialog::OnDestroy();
	TRACE0 ("CxDlgWaitTask::OnDestroy() ...\n");
}

//  WPARAM
//	W_EXTRA_TEXTINFO = 0,   LPARAM is (LPTSTR)
//	W_RUN_TASK,             LPARAM is 0, Start the Thread funtion
//	W_STARTSTOP_ANIMATION,  LPARAM is 0, Stop playing the animation, 1 Start playing the animation
//	W_TASK_TERMINATE,       LPARAM is m_nReturnCode,  The Thread funtion has terminate
//	W_TASK_PROGRESS,        LPARAM is current_job * (100/total_job)
//	W_KILLTASK              LPARAM is 0 (kill the thread through TerminateThread(handle).
LONG CxDlgWaitTask::OnExecuteTask (WPARAM wParam, LPARAM lParam)
{
	THREADPARAM* pObj = NULL;
	LPTSTR lpszText = NULL;
	int nMode =(int)wParam;
	switch (nMode)
	{
	case W_EXTRA_TEXTINFO:
		lpszText = (LPTSTR)lParam;
		m_strStaticText1 = lpszText;
		UpdateData (FALSE);
		if (lpszText)
			delete lpszText;
		break;
	case W_RUN_TASK:
		//
		// Execute the Thread:
		pObj = new THREADPARAM;
		pObj->hWndDlgWait = m_hWnd;
		pObj->pParam = m_pExecParam;
		m_pThread1 = AfxBeginThread((AFX_THREADPROC)ThreadControlExecParam, pObj, THREAD_PRIORITY_LOWEST);
		break;
	case W_STARTSTOP_ANIMATION:
		if ((int)lParam == 0)
			m_cAnimate1.Stop();
		else
			m_cAnimate1.Play(0, 0xFFFF, (UINT)-1);
		break;
	case W_TASK_TERMINATE:
		if (m_bHideAnimateDlgWhenTerminate)
			ShowWindow (SW_HIDE);
		m_pExecParam->OnTerminate(m_hWnd);
		if ((int)lParam == 0)
			EndDialog (IDOK);
		else
			EndDialog (IDCANCEL);
		break;
	case W_TASK_PROGRESS:
		{
			int nNewJob = (int)lParam;
			if (m_cProgress.GetPos() < nNewJob)
				m_cProgress.SetPos (nNewJob);
		}
		break;
	case W_KILLTASK:
		if (m_pThread1 && m_pThread1->m_hThread)
			TerminateThread(m_pThread1->m_hThread, 1);
		m_pThread1 = NULL;
		m_pExecParam->OnTerminate(m_hWnd);
		EndDialog (IDCANCEL);
		break;

	default:
		ASSERT(FALSE);
		break;
	}
	return 0;
}


void CxDlgWaitTask::PostNcDestroy() 
{
	CDialog::PostNcDestroy();
}
