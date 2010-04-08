/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : xdlgwait.cpp, Implementation file                                     //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II/VDBA                                                        //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Allow interupt task                                                   //
****************************************************************************************/
/*
** History:
**
**  03-Feb-2000
**      (BUG #100324)
**      Replace the named Mutex by the unnamed Mutex.
**  20-Mar-2001 (noifr01)
**    (bug 104246) disabled constructor of CxDlgWait that was resulting in
**    launching the old-style animation. Also now provide a default caption if
**    none is passed in the other constructor
**  01-Aug-2005 (komve01)
**    Bug #114962/Issue #14061474
**    Added a while loop around WaitForSingleObject, to try until the thread gets the 
**    required resource.
**  23-Aug-2005 (fanra01)
**    Bug
**    Modified the 01-Aug-2005 change to handle the WAIT_TIMEOUT condition
**    waiting for a thread.
**  06-Sep-2005 (drivi01)
**    Updating Ray's previous change due to bad propagation, truncated file.
**  16-Mar-2006 (fanra01)
**    Bug 115852
**    Animation dialog does not display during lengthy operations.
**    The loop introduced by change 478230 and the subsequent change 478810
**    prevented the initialization of the dialog on an event timeout.
**    Allow the animation dialog to display on event timeout.
**/

#include "stdafx.h"
#include "mainmfc.h"
#include "xdlgwait.h"
#include "sqlquery.h"
#include "extccall.h"

extern "C" 
{
#include "dbaginfo.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/*
** Name: ThreadProcControlS1
**
** Description:
**  Thread function to perform specified action defined in CaExecParam.  Each
**  instance of the class creates a mutex to synchronize multiple threads.
**  The thread performs the action and when complete sends a custom
**  windows message WM_EXECUTE_TASK with OK or cancel to the animation dialog
**  window.
**  
** Inputs:
**      pParam  pointer to a CaExecParam.
**
** Outputs:
**      None.
**
** Returns:
**      Always returns 0.
**
** History:
**      16-Mar-2006 (farna01)
**          Add some comments to aid understanding.
**          Add WAIT_ABANDONED cases for mutex completion.
*/
UINT CxDlgWait::ThreadProcControlS1 (LPVOID pParam)
{
	CaExecParam* pObj = (CaExecParam*)pParam;
	int nResult = 0;
	TCHAR tchszRetern [] = {0x0D, 0x0A, 0x0};
	DWORD dwWaitResult;
	dwWaitResult = WaitForSingleObject (m_hMutexV1, 100L);
	switch (dwWaitResult)
	{
	case WAIT_OBJECT_0:
		m_bFinish = FALSE;
	case WAIT_ABANDONED:
		ReleaseMutex (m_hMutexV1);
		break;
	default:
		break;
	}
	//
	// Begin of Execution the statement:
	if (m_pExecParam)
	{
		DBAThreadEnableCurrentSession();
		nResult = pObj->Run(m_hWnd);
		DBAThreadDisableCurrentSession();
		m_nReturnCode = nResult;
	}
	//
	// End of execution of the statement:
	BOOL bCanExit = FALSE;
	dwWaitResult = WaitForSingleObject (m_hMutexV1, 100L);
	switch (dwWaitResult)
	{
	case WAIT_OBJECT_0:
		if (!m_bFinish)
			bCanExit = TRUE;
	case WAIT_ABANDONED:
		ReleaseMutex (m_hMutexV1);
		break;
	default:
		break;
	}

	if (bCanExit)
		::PostMessage (m_hWnd, WM_EXECUTE_TASK, (WPARAM)0, (LPARAM)IDOK);
	else
		::PostMessage (m_hWnd, WM_EXECUTE_TASK, (WPARAM)0, (LPARAM)IDCANCEL);

	return nResult;
}

UINT CxDlgWait::ThreadProcControlS2 (LPVOID pParam)
{
	DWORD dwWaitResult = WaitForSingleObject (m_hMutexV1, 100L);
	switch (dwWaitResult)
	{
	case WAIT_OBJECT_0:
		if (m_bFinish == FALSE)
		{
			m_bFinish = TRUE;
		}
		ReleaseMutex (m_hMutexV1);
		TRACE0 ("Execute IIbreak ...\n");
		IIbreak();
		break;
	default:
		break;
	}

	dwWaitResult = WaitForSingleObject (m_hMutexT2, INFINITE);
	switch (dwWaitResult)
	{
	case WAIT_OBJECT_0:
		m_pThread2 = NULL;
		ReleaseMutex (m_hMutexT2);
		break;
	default:
		break;
	}
	return 0;
}


DWORD IFThreadControlS1(LPVOID pParam)
{
	CxDlgWait*   pDlg = (CxDlgWait*)pParam;
	CaExecParam* pExecParam = pDlg->GetExecParam();
	return pDlg->ThreadProcControlS1((LPVOID)pExecParam);
}


DWORD IFThreadControlS2(LPVOID pParam)
{
	CxDlgWait* pDlg = (CxDlgWait*)pParam;
	return pDlg->ThreadProcControlS2(NULL);
}




/////////////////////////////////////////////////////////////////////////////
// CxDlgWait dialog

void CxDlgWait::SetUseAnimateAvi (int nAviType)
{
	m_bUseAnimateIcon = FALSE;
	m_nAviType = nAviType;
}

void CxDlgWait::Init()
{
	m_bCencerAtMouse = FALSE;
	m_nCenterType = OPEN_AT_MOUSE;

	m_bActionSetWindowPos = FALSE;
	m_bRatio = FALSE;
	m_dXratio=1.0;
	m_dYratio=1.0;
	m_bSpecifySize = FALSE;
	m_nAviType = IDR_ANCLOCK;
	m_sizeDlgMin.cx = 20;
	m_sizeDlgMin.cy = 20;

	m_pExecParam = NULL;
	m_nReturnCode= -1;
	m_bSetParam = FALSE;
	m_bDeleteExecParam = TRUE;
	m_bFinish = FALSE;

	m_bUseExtraInfo    = FALSE;
	m_bUseAnimateIcon  = TRUE;
	m_nTimer1 = 1;
	m_nCounter    = 0;

	UINT nIcon[16] = 
	{
		IDI_ICON_COUNTER01,
		IDI_ICON_COUNTER02,
		IDI_ICON_COUNTER03,
		IDI_ICON_COUNTER04,
		IDI_ICON_COUNTER05,
		IDI_ICON_COUNTER06,
		IDI_ICON_COUNTER07,
		IDI_ICON_COUNTER08,
		IDI_ICON_COUNTER09,
		IDI_ICON_COUNTER10,
		IDI_ICON_COUNTER11,
		IDI_ICON_COUNTER12,
		IDI_ICON_COUNTER13,
		IDI_ICON_COUNTER14,
		IDI_ICON_COUNTER15,
		IDI_ICON_COUNTER16
	};

	for (int i = 0; i<16; i++)
	{
		m_hIconCounter [i] = AfxGetApp()->LoadIcon(nIcon[i]);
	}
	m_hIcon = m_hIconCounter[15];

	m_hMutexV1 = CreateMutex (NULL, FALSE, NULL);
	if (m_hMutexV1 == NULL)
	{
		CString strMsg = VDBA_MfcResourceString(IDS_E_MUTEX);//_T("Cannot create Mutex");
		AfxMessageBox (strMsg);
	}

	m_hMutexT2 = CreateMutex (NULL, FALSE, NULL);
	if (m_hMutexT2 == NULL)
	{
		CString strMsg = VDBA_MfcResourceString(IDS_E_MUTEX);//_T("Cannot create Mutex");
		AfxMessageBox (strMsg);
	}
}

#if 0
CxDlgWait::CxDlgWait(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgWait::IDD, pParent)
{
	m_strCaption = _T("...");
	m_bShowTitle = FALSE;
	//{{AFX_DATA_INIT(CxDlgWait)
	m_strStaticText1 = _T("");
	//}}AFX_DATA_INIT

	Init();
}
#endif


CxDlgWait::CxDlgWait(LPCTSTR lpszCaption, CWnd* pParent)
	: CDialog(IDD_XTASK_WAIT_WITH_TITLE, pParent)
{
	m_strCaption = lpszCaption? lpszCaption: VDBA_MfcResourceString(IDS_PLEASEWAIT);
	m_bShowTitle = TRUE;
	m_strStaticText1 = _T("");
	
	Init();
}

int CxDlgWait::DoModal()
{
    int nAnswer = IDCANCEL;
    BOOL bCallBase = FALSE;
    BOOL waitretry = FALSE;
    BOOL callModal = TRUE;
    DWORD dwWaitResult;
    
    theApp.SetInterruptType (INTERRUPT_NOT_ALLOWED);
    if (m_pExecParam && m_pExecParam->IsExecuteImmediately())
    {
        bCallBase = TRUE;
        nAnswer = CDialog::DoModal();
    }

    if (!bCallBase)
    {
        DBAThreadDisableCurrentSession();
        //
        // Execute the Thread:
        m_pThread1 = AfxBeginThread((AFX_THREADPROC)IFThreadControlS1, this, THREAD_PRIORITY_BELOW_NORMAL);
        do
        {
            dwWaitResult = WaitForSingleObject (m_pThread1->m_hThread,
                theApp.DlgWaitGetDelayExecution());
            switch (dwWaitResult)
            {
            case WAIT_OBJECT_0:
                if (m_bDeleteExecParam)
                {
                    if (m_pExecParam)
                        delete m_pExecParam;
                    m_pExecParam = NULL;
                    theApp.SetInterruptType (INTERRUPT_NOT_ALLOWED);
                }
                nAnswer = IDOK;
                waitretry = FALSE;
                break;
            case WAIT_FAILED:
            case WAIT_ABANDONED:
                if (m_bDeleteExecParam)
                {
                    if (m_pExecParam)
                        delete m_pExecParam;
                    m_pExecParam = NULL;
                    theApp.SetInterruptType (INTERRUPT_NOT_ALLOWED);
                }
                nAnswer = IDCANCEL;
                waitretry = FALSE;
            case WAIT_TIMEOUT:
                /*
                ** Thread execution has not completed within the timeout.
                ** Either the query is taking a long time or is blocked.
                ** Wait for the thread to complete.  There is nothing else for
                ** it at this point, except maybe prevent a forever loop.
                */
            default:
                /*
                ** An unexpected return code.
                */
                if (callModal == TRUE)
                {
                    /*
                    ** No return from here until the dialog receives an
                    ** WM_EXECUTE_TASK message with a parameter of IDOK or
                    ** IDCANCEL.
                    */
                    nAnswer = CDialog::DoModal();
                    callModal = FALSE;
                }
                /*
                ** waitretry value should be FALSE, ensure that it is 
                ** FALSE to satisfy the loop exit condition.
                */
                waitretry = (dwWaitResult == WAIT_TIMEOUT) ? TRUE : FALSE;
                break;
            }
        }
        while(waitretry);
    }
    DBAThreadEnableCurrentSession();
    return nAnswer;
}


void CxDlgWait::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgWait)
	DDX_Control(pDX, IDCANCEL, m_cButtonCancel);
	DDX_Control(pDX, IDC_STATIC_IMAGE, m_cStaticImage1);
	DDX_Control(pDX, IDC_MFC_STATIC1, m_cStaticText1);
	DDX_Control(pDX, IDC_ANIMATE1, m_cAnimate1);
	DDX_Text(pDX, IDC_MFC_STATIC1, m_strStaticText1);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgWait, CDialog)
	//{{AFX_MSG_MAP(CxDlgWait)
	ON_WM_TIMER()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_EXECUTE_TASK,   OnExecuteTask)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgWait message handlers


//
// The IIbreak() never returns (bug ?)
// I have to create a new thread executing IIbreak and SUPPOSE that
// 1 second should be enought to execute IIbreak, and after that I
// can kill the thread:
void CxDlgWait::OnCancel() 
{
	//
	// Check to see if Interrupt is allowed
	UINT nInterruptType = m_pExecParam->GetInterruptType();
	if (nInterruptType == INTERRUPT_NOT_ALLOWED || m_pExecParam->IsInterrupted())
	{
		MessageBeep ((UINT)-1);
	}
	else
	{
		theApp.SetInterruptType (nInterruptType);
		m_pExecParam->Lock();
		m_pExecParam->SetInterrupted();
		m_pExecParam->Unlock();

		if (nInterruptType & INTERRUPT_QUERY)
		{
			TRACE0 ("TODO: CxDlgWait::OnCancel() ==> Iterrupt the Query ...\n");
		}
	}
}

BOOL CxDlgWait::OnInitDialog() 
{
	CDialog::OnInitDialog();

	theApp.SetInterruptType (INTERRUPT_NOT_ALLOWED);
	ResizeControls();
	if (m_bUseAnimateIcon)
	{
		m_cAnimate1.ShowWindow (SW_HIDE);
		m_cStaticImage1.ShowWindow (SW_SHOW);
		SetTimer (m_nTimer1, 200L, NULL);

		if (m_bUseExtraInfo)
			m_cStaticText1.ShowWindow (SW_SHOW);
		else
			m_cStaticText1.ShowWindow (SW_HIDE);
	}

	if (!m_bUseAnimateIcon)
	{
		if (m_bUseExtraInfo)
			m_cStaticText1.ShowWindow (SW_SHOW);
		else
			m_cStaticText1.ShowWindow (SW_HIDE);

		m_cAnimate1.ShowWindow (SW_SHOW);
		m_cStaticImage1.ShowWindow (SW_HIDE);
		
		if (m_cAnimate1.Open(m_nAviType))
			m_cAnimate1.Play(0, 0xFFFF, (UINT)-1);
		else
		{
			m_bUseAnimateIcon = TRUE;
			m_cAnimate1.ShowWindow (SW_HIDE);
			m_cStaticImage1.ShowWindow (SW_SHOW);
			SetTimer (m_nTimer1, 200L, NULL);
		}
	}

	if (m_pExecParam && m_pExecParam->IsExecuteImmediately())
	{
		DBAThreadDisableCurrentSession();
		//
		// Execute the Thread:
		m_pThread1 = AfxBeginThread((AFX_THREADPROC)IFThreadControlS1, this, THREAD_PRIORITY_BELOW_NORMAL);
	}

	if (m_bCencerAtMouse)
	{
		POINT p;
		CRect r;
		if (GetCursorPos (&p))
		{
			GetWindowRect (r);
			if (m_nCenterType == OPEN_CENTER_AT_MOUSE)
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
		m_cButtonCancel.ShowWindow (SW_HIDE);
		SetWindowText (m_strCaption);
	}
	ShowWindow (SW_SHOW);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgWait::OnTimer(UINT nIDEvent) 
{
	if (nIDEvent == m_nTimer1 && m_bUseAnimateIcon)
	{
		m_hIcon = m_hIconCounter [m_nCounter];
		m_cStaticImage1.SetIcon (m_hIcon);
		m_nCounter++;
		if (m_nCounter == 16)
			m_nCounter = 0;
	}
	CDialog::OnTimer(nIDEvent);
}

void CxDlgWait::OnLButtonDown(UINT nFlags, CPoint point) 
{
	if (m_bShowTitle)
	{
		CDialog::OnLButtonDown(nFlags, point);
		return;
	}

	SetCapture();
	CPoint p = point;
	CRect r;
	GetWindowRect (r);
	ClientToScreen (&p);
	m_sizeDown = CSize (p.x - r.left, p.y - r.top);
	CDialog::OnLButtonDown(nFlags, point);
}

void CxDlgWait::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if (m_bShowTitle)
	{
		CDialog::OnLButtonUp(nFlags, point);
		return;
	}

	ReleaseCapture();
	CDialog::OnLButtonUp(nFlags, point);
	CPoint p = point;
	CRect r, r2;
	GetWindowRect (r);
	ClientToScreen (&p);
	SetWindowPos (NULL, p.x - m_sizeDown.cx, p.y - m_sizeDown.cy, 0, 0, SWP_NOZORDER|SWP_NOSIZE|SWP_SHOWWINDOW);
	CDialog::OnLButtonUp(nFlags, point);
}

void CxDlgWait::OnMouseMove(UINT nFlags, CPoint point) 
{
	if (m_bShowTitle)
	{
		CDialog::OnMouseMove(nFlags, point);
		return;
	}
	CPoint p = point;
	if (nFlags & MK_LBUTTON)
	{
		CRect r;
		GetWindowRect (r);
		ClientToScreen (&p);
		SetWindowPos (NULL, p.x - m_sizeDown.cx, p.y - m_sizeDown.cy, 0, 0, SWP_NOZORDER|SWP_NOSIZE|SWP_SHOWWINDOW);
	}
	CDialog::OnMouseMove(nFlags, point);
}

void CxDlgWait::OnDestroy() 
{
	if (m_bDeleteExecParam)
	{
		if (m_pExecParam)
			delete m_pExecParam;
		m_pExecParam = NULL;
	}
	CDialog::OnDestroy();
	theApp.SetInterruptType (INTERRUPT_NOT_ALLOWED);
	TRACE0 ("CxDlgWait::OnDestroy() ...\n");
}

//
// wParam = 0, interrupt. lParam = IDCANCEL or IDOK
//        = 1, Display extra information. lParam = null terminate string.
//        = 2, Set text on the title bar. lParam = null terminate string.
LONG CxDlgWait::OnExecuteTask (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR lpszText = NULL;
	switch ((int)wParam)
	{
	case 0:
		//
		// Wait for the termination of the thread T1:
		WaitForSingleObject (m_pThread1->m_hThread, INFINITE);
		EndDialog ((int)lParam);
		break;
	case 1:
		m_strStaticText1 = (LPTSTR)lParam;
		UpdateData (FALSE);
		break;
	case 2:
		lpszText = (LPTSTR)lParam;
		SetWindowText (lpszText? lpszText: _T("Wait ..."));
		break;
	}
	return 0;
}


void CxDlgWait::SetSize (int nWidth, int nHeight)
{
	m_bSpecifySize = TRUE;
	m_sizeDlg.cx = (nWidth  < m_sizeDlgMin.cx)? m_sizeDlgMin.cx: nWidth;
	m_sizeDlg.cy = (nHeight < m_sizeDlgMin.cy)? m_sizeDlgMin.cy: nHeight;
}

void CxDlgWait::SetSize2(double dpWidth, double dpHeight)
{
	m_bSpecifySize = TRUE;
	m_bRatio = TRUE;
	if (dpWidth  > 1.0) dpWidth  = 1.0;
	if (dpHeight > 1.0) dpHeight = 1.0;
	m_dXratio= (dpWidth  < 0.001)? 0.1: dpWidth;
	m_dYratio= (dpHeight < 0.001)? 0.1: dpHeight;
}


void CxDlgWait::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (m_bSpecifySize && m_bRatio && !m_bActionSetWindowPos)
	{
		int x, y;
		CRect rDlg;
		GetClientRect (rDlg);
		x = (int)(m_dXratio * (double)rDlg.Width());
		y = (int)(m_dYratio * (double)rDlg.Height());
		m_sizeDlg.cx = (x < m_sizeDlgMin.cx)? m_sizeDlgMin.cx: x;
		m_sizeDlg.cy = (y < m_sizeDlgMin.cy)? m_sizeDlgMin.cy: y;
	}
	if (m_bSpecifySize && !m_bActionSetWindowPos)
	{
		m_bActionSetWindowPos = TRUE;
		SetWindowPos (NULL, 0, 0, m_sizeDlg.cx, m_sizeDlg.cy, SWP_NOZORDER|SWP_NOMOVE|SWP_HIDEWINDOW);
	}
	m_bActionSetWindowPos = FALSE;
	ResizeControls();
}

void CxDlgWait::ResizeControls()
{
	if (!IsWindow (m_cAnimate1.m_hWnd))
		return;
	if (m_bSpecifySize)
	{
		int nX, nY;
		CRect rDlg, r;
		GetClientRect (rDlg);
		m_cButtonCancel.GetClientRect (r);
		nX = r.Width();
		nY = r.Height();
		m_cStaticImage1.CenterWindow();
		if (m_bUseExtraInfo)
		{
			int nHeight;
			m_cStaticText1.GetClientRect (r);
			nHeight = r.Height();
			r = rDlg;
			r.bottom = rDlg.bottom;
			r.top    = r.bottom - nHeight;
			m_cStaticText1.MoveWindow (r);
			
			int nY1 = r.top;
			r = rDlg;
			r.bottom = nY1 - 2;
			r.left  += (nX+2);
			r.right -= (nX+2);
			m_cAnimate1.MoveWindow (r);
		}
		else
		{
			r = rDlg;
			r.left  += (nX+2);
			r.right -= (nX+2);
			m_cAnimate1.MoveWindow (r);
		}
		m_cButtonCancel.SetWindowPos (NULL, rDlg.right - nX -2, rDlg.top +2, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
	}
}
