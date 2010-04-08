/*****************************************************************************
**
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**  Source   : dlgrmcmd.cpp, Implementation File
**
**
**    Project  : Ingres II/Vdba.
**    Author   : UK Sotheavut
**
**    Purpose  : Popup Dialog Box for the RMCMD Execution
**
**  History after 01-01-2000:
**
**  04-Apr-2000 (noifr01)
**     (bug 101430) manage the new carriage return information available 
**     through rmcmd. fixed incidently a minor problem where empty lines
**     could not be displayed in this dialog
**  14-Dec-2001 (noifr01)
**   (sir 99596) removal of obsolete code and resources
**  10-Mar-2010 (drivi01)
**   On Generate Statistics if there's no output from command,
**   instead of just closing the window after statistics were successfully
**   generated, popup a message box to let user know that operation
**   completed.
*****************************************************************************/

#include "stdafx.h"
#include "mainmfc.h"
#include "dlgrmcmd.h"
#include "dlgrmans.h"
extern "C"
{
#include "resource.h"
#include "dba.h"
#include "rmcmd.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


extern "C" int DlgExecuteRemoteCommand (LPRMCMDPARAMS lpParam)
{
	CxDlgRemoteCommand dlg;
	dlg.SetParams((LPVOID)lpParam);

	return dlg.DoModal();
}

CxDlgRemoteCommand::CxDlgRemoteCommand(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgRemoteCommand::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgRemoteCommand)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_pParam  = NULL;
	m_nTimer1 = 0;
	m_nEllapse= 1000L;
	m_iTimerSubCount=0;
	m_iTimerCount   =0;
	m_bServerActive = FALSE;
	m_sizeInitial.cx = m_sizeInitial.cy = 0;
}


void CxDlgRemoteCommand::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgRemoteCommand)
	DDX_Control(pDX, IDC_MFC_STATIC1, m_cStatic1);
	DDX_Control(pDX, IDC_MFC_EDIT1, m_cEdit1);
	DDX_Control(pDX, IDOK, m_cButtonOK);
	DDX_Control(pDX, IDC_MFC_BUTTON1, m_cButtonSendAnswer);
	DDX_Control(pDX, IDC_ANIMATE1, m_cAnimate1);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgRemoteCommand, CDialog)
	//{{AFX_MSG_MAP(CxDlgRemoteCommand)
	ON_BN_CLICKED(IDC_MFC_BUTTON1, OnSendAnswer)
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_WM_SIZE()
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgRemoteCommand message handlers

void CxDlgRemoteCommand::OnSendAnswer() 
{
	CString strMsg;
	CxDlgRemoteCommandInterface dlg;
	KillTimer(m_nTimer1);
	if (dlg.DoModal() != IDOK)
		return;
	LPRMCMDPARAMS lpRemoteCmd = (LPRMCMDPARAMS)m_pParam;
	//
	// Send the answer to the remote command
	dlg.m_strEdit1.TrimLeft();
	dlg.m_strEdit1.TrimRight();
	PutRmcmdInput(lpRemoteCmd, (LPUCHAR)(LPCTSTR)dlg.m_strEdit1, FALSE);
	
	m_cEdit1.SetFocus();
	if (!SetTimer(m_nTimer1, m_nEllapse, 0)) 
	{
		strMsg.LoadString (IDS_ERR_TIMER_REMOTECMD);
		AfxMessageBox(strMsg, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
		OnCancel();
		return;
	}
	// Self - post Timer message for immediate echo of input
	PostMessage(WM_TIMER, m_nTimer1, (LPARAM)NULL);
}

void CxDlgRemoteCommand::OnOK() 
{
	// TODO: Add extra validation here
	
	CDialog::OnOK();
}

BOOL CxDlgRemoteCommand::OnInitDialog() 
{
	CDialog::OnInitDialog();
	ASSERT (m_pParam);
	if (!m_pParam)
		return FALSE;

	int nXPos;
	CRect r, rDlg;
	GetClientRect (rDlg);
	m_sizeInitial.cx = rDlg.Width();
	m_sizeInitial.cy = rDlg.Height();
	m_cEdit1.GetWindowRect (r);
	ScreenToClient (r);
	nXPos = r.right;
	m_sizePadding.cy = rDlg.bottom - r.bottom;
	m_cButtonSendAnswer.GetWindowRect (r);
	ScreenToClient (r);
	m_sizePadding.cx = rDlg.right - r.right;
	m_nCxEditMargin = rDlg.right - nXPos; 

	m_cButtonOK.GetWindowRect (r);
	m_sizeButton[0].cx = r.Width();
	m_sizeButton[0].cy = r.Height();
	m_cButtonSendAnswer.GetWindowRect (r);
	m_sizeButton[1].cx = r.Width();
	m_sizeButton[1].cy = r.Height();
	m_cAnimate1.GetWindowRect (r);
	m_sizeButton[2].cx = r.Width();
	m_sizeButton[2].cy = r.Height();


	if (m_cAnimate1.Open(IDR_ANCLOCK))
		m_cAnimate1.Play(0, 0xFFFF, (UINT)-1);
	m_cEdit1.LimitText();
	//
	// Allocate space for future incoming lines
	memset(m_feedbackLines, 0, sizeof(m_feedbackLines));
	for (int i=0; i<FEEDBACK_MAXLINES; i++) 
	{
		m_feedbackLines[i] = (LPUCHAR)ESL_AllocMem(RMCMDLINELEN);
		if (!m_feedbackLines[i]) {
			for (int j=0; j<i; j++)
			{
				ESL_FreeMem(m_feedbackLines[j]);
			}
			//
			// Do not use LoadString, it might fail when lack of memory !!!
			VDBA_OutOfMemoryMessage();
			OnCancel();
			return FALSE;
		}
	}
	//
	// Activate the Timer:
	SetTimer (m_nTimer1, m_nEllapse, NULL);

	LPRMCMDPARAMS pParam = (LPRMCMDPARAMS)m_pParam;
	//
	// Set the Dialog Title:
	SetWindowText((LPCTSTR)pParam->Title);
	m_cButtonOK.EnableWindow (FALSE);
	m_cButtonSendAnswer.EnableWindow (FALSE);
	PushHelpId (REMOTECOMMANDDIALOG);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgRemoteCommand::OnDestroy() 
{
	KillTimer(m_nTimer1);
	CDialog::OnDestroy();
	for (int i=0; i<FEEDBACK_MAXLINES; i++)
		ESL_FreeMem(m_feedbackLines[i]);
	PopHelpId();
}

void CxDlgRemoteCommand::OnTimer(UINT nIDEvent) 
{
	int cpt;
	int retval;
	int len;
	LPRMCMDPARAMS lpRemoteCmd = (LPRMCMDPARAMS)m_pParam;
	CString strMsg;
	if (nIDEvent == m_nTimer1)
	{
		// check on incoming lines
		if (!HasRmcmdEventOccured(lpRemoteCmd)) {
			m_iTimerSubCount++;
			if (m_iTimerSubCount<5)
				return;
			m_iTimerSubCount=0;
		}
		KillTimer(m_nTimer1); 
		retval = GetRmcmdOutput(lpRemoteCmd, FEEDBACK_MAXLINES, m_feedbackLines,m_bNoCR);
		if (!SetTimer(m_nTimer1, m_nEllapse, 0)) {
			strMsg.LoadString(IDS_ERR_TIMER_REMOTECMD);
			AfxMessageBox(strMsg, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
			CleanupRemoteCommandTuples(lpRemoteCmd);
			OnCancel();
			return;
		}

		if (retval < 0) 
		{
			// the command execution is terminated!
			KillTimer(m_nTimer1); 
			// delete the tuples on the server side
			CleanupRemoteCommandTuples(lpRemoteCmd);
			// manage the box end
			if (lpRemoteCmd->lastlineread == -1) {
				// No lines received at all : immediately close the box
				OnOK();
				m_cButtonOK.EnableWindow(TRUE);
				m_cButtonSendAnswer.EnableWindow(FALSE);
				strMsg.LoadString(IDS_COMMAND_COMPLETED);
				AfxMessageBox(strMsg, MB_OK|MB_ICONINFORMATION);
				m_cAnimate1.Stop();
				return;
			}
			else 
			{
				// at least one line was received : wait for the user to click OK
				m_cButtonOK.EnableWindow(TRUE);
				m_cButtonSendAnswer.EnableWindow(FALSE);
				strMsg.LoadString (IDS_REMOTECMD_FINISHED);
				m_cStatic1.SetWindowText(strMsg);
				m_cAnimate1.Stop();
				return;
			}
		}

		if (retval > 0) 
		{
			// Add the received lines in the multiline edit
			len = m_cEdit1.GetWindowTextLength();
			m_cEdit1.SetSel (len, len);
			for (cpt=0; cpt<retval; cpt++)
			{
				if (m_feedbackLines[cpt])
				{
					strMsg = (LPCTSTR)m_feedbackLines[cpt];
					if (!m_bNoCR[cpt])
						strMsg+= _T("\r\n");
					m_cEdit1.ReplaceSel (strMsg);
					len = m_cEdit1.GetWindowTextLength();
					m_cEdit1.SetSel (len, len);
				}
			}
			// Authorize the user to send an answer
			m_cButtonSendAnswer.EnableWindow(TRUE);
			// change the message above the multiline edit
			strMsg.LoadString (IDS_REMOTECMD_INPROGRESS);
			m_cStatic1.SetWindowText(strMsg);
		}

		if (retval == 0 && !m_bServerActive) 
		{
			m_iTimerCount++;
			if (m_iTimerCount>5) {
				int istatus;
				m_iTimerCount=0;
				istatus=GetRmCmdStatus(lpRemoteCmd);
				
				if (istatus==RMCMDSTATUS_ERROR)  {
					KillTimer(m_nTimer1);
					strMsg.LoadString (IDS_REMOTECMD_NOTACCEPTED);
					AfxMessageBox(strMsg, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
					// delete the tuples on the server side
					CleanupRemoteCommandTuples(lpRemoteCmd);
					OnCancel();
					return;
				}
				if (istatus!=RMCMDSTATUS_SENT)
					m_bServerActive=TRUE;
				else  {
					KillTimer(m_nTimer1);
					CString strW;
					strMsg.LoadString (IDS_REMOTECMD_NOANSWER);
					if (AfxMessageBox(strMsg, MB_ICONQUESTION | MB_YESNO | MB_TASKMODAL) == IDNO)
					{
						// delete the tuples on the server side
						CleanupRemoteCommandTuples(lpRemoteCmd);
						OnCancel();
						return;
					}
					if (!SetTimer(m_nTimer1, m_nEllapse, 0)) {
						strMsg.LoadString (IDS_ERR_TIMER_REMOTECMD);
						AfxMessageBox(strMsg, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
						CleanupRemoteCommandTuples(lpRemoteCmd);
						OnCancel();
						return;
					}
				}
			}
		}
	}
	CDialog::OnTimer(nIDEvent);
}

void CxDlgRemoteCommand::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	CRect r, rDlg;
	GetClientRect (rDlg);
	if (!IsWindow (m_cButtonOK.m_hWnd))
		return;

	m_cButtonOK.GetWindowRect (r);
	ScreenToClient (r);
	r.right = rDlg.right - m_sizePadding.cx;
	r.left  = rDlg.right - m_sizeButton[0].cx -8;
	m_cButtonOK.MoveWindow (r);

	m_cButtonSendAnswer.GetWindowRect (r);
	ScreenToClient (r);
	r.right = rDlg.right - m_sizePadding.cx;
	r.left  = rDlg.right - m_sizeButton[1].cx - 8;
	m_cButtonSendAnswer.MoveWindow (r);
	int nYbas = r.bottom;

	m_cEdit1.GetWindowRect (r);
	ScreenToClient (r);
	r.right = rDlg.right - m_nCxEditMargin;
	r.bottom= rDlg.bottom - m_sizePadding.cy;
	m_cEdit1.MoveWindow (r);
	
	m_cAnimate1.GetWindowRect (r);
	ScreenToClient (r);
	r.right = rDlg.right - m_sizePadding.cx;
	r.left  = rDlg.right - m_sizeButton[2].cx -4;
	r.bottom = rDlg.bottom - m_sizePadding.cy;
	r.top  = rDlg.bottom - m_sizeButton[2].cy -4;
	if (r.top > (nYbas + 4))
	{
		m_cAnimate1.MoveWindow (r);
		m_cAnimate1.Invalidate();
		m_cAnimate1.ShowWindow (SW_SHOW);
	}
	else
		m_cAnimate1.ShowWindow (SW_HIDE);

}

void CxDlgRemoteCommand::OnClose() 
{
	if (!m_cButtonOK.IsWindowEnabled())
		return;
	CDialog::OnClose();
}
