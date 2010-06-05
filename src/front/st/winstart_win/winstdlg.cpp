/*
**  Copyright (C) 1997, 2004 Ingres Corporation
*/

/*
**
**  Name: winstdlg.cpp
**
**  Description:
**  This file implements the main dialog of winstart stopping and
**  Ingres services.
**
**  History:
**
**  06-may-97 (mcgem01)
**      Replace MB_ICONEXCLAMATION with MB_ICONASTERISK as
**      in everyday usage MB_ICONEXCLAMATION warns of an
**      error condition. 
**  25-sep-98 (cucjo01)
**      Added functionality to start & stop Ingres as a service
**      Added new AVI's & Icons for Start/Stop/Fail
**      Added status text for "Started", "Stoppped", etc below AVI
**      Check for process iigcn using PCget_PidFromName() instead of
**        using ipcclean.exe to detect whether or not Ingres is running
**      Set the checkbox accordingly to Start & Stop Ingres
**        as a Service or through "ingstart" and "ingstop" commands
**      Handle "Fail" conditions correctly
**      Aesthetic Messages, dialog layout, spelling / grammar, etc
**      Hide "Start Ingres as a service" checkbox if Win95 or Win98
**      Accept command line argument "/Start" to enable only the "Exit" 
**        button on successful startup (for VDBA use)
**      Combine winstart & winstop into one program
**  05-oct-98 (cucjo01)
**      Added preprocessor def INGRES4UNI to start the service "Ingres Client"
**        for Unicenter Installations
**  08-oct-98 (cucjo01)
**      Added multithreaded starting of service so dialog refreshes properly
**      Added command line option /start and /stop to make application
**      be a "start-only" or "stop-only" program.
**  09-oct-98 (cucjo01)
**      Added support to start service as follows:
**        * If Ingres Server and Ingres Client are present, start Ingres Server
**        * If Ingres Client is the only service present, start Ingres Client
**        * If command line option /client is set, override and start Ingres Client
**      Removed dependency for INGRES4UNI preprocessor definition since it
**        selects which service to start automatically.
**  26-oct-98 (cucjo01)
**      Set the cursor to the end of the last line before adding text
**  23-dec-98 (cucjo01)
**      When executing with /start or /stop option, automatically begin so user
**      doesn't have to click the start or stop button.
**  02-feb-99 (cucjo01)
**      Added check for II_SYSTEM before starting up
**      Fixed error message string to add CR/LF and display in correct form
**      Added checkbox to start Ingres as a client 
**  30-mar-99 (cucjo01)
**      Added command line option /param="xxxx" where xxxx can be -client or -iigcn, etc.
**      Removed /client option which can now be replaced by "/param="-client"
**      This is the command line parameter passed to ingstart & ingstart.
**      Separated the "first time" parameters to work properly
**  31-mar-99 (cucjo01)
**      Added support for other products with preprocessor definition
**	"<product>". This means:
**      - Checking for "<product>_ROOT" instead of "II_SYSTEM"
**      - When checking the state of Ingres, we first check for "edbcgcn.exe"
**      - The "Client Service" checkbox is hidden
**      - If checking for the service, we check for <product>_Client (which is
**        in a separate .rc file which replaces the winstart.rc file; replacing
**        occurences of "Ingres" with "<product>"
**      - Now uses #define for the executable so that:
**        1 - It executes edbcstart instead of ingstart when starting up
**        2 - It executes edbcstop instead of ingstop when shutting down
**  15-apr-99 (cucjo01)
**      Added NT error text to GetLastError() messages.
**  24-jan-2000 (cucjo01)
**      Made "Start Ingres as a service" checkbox read only and "selected".
**      We no longer need to start Ingres as a service since "ingstart" does
**      it automatically.
**  30-jan-2000 (cucjo01)
**      Backed out prior change to always start ingres as a service.
**  30-jan-2000 (cucjo01)
**      Updated all occurences of short names and long names for the services
**      to reflect the installation identifier.  Also made 'Start as a service'
**      option the default for the checkbox.
**  31-jan-2000 (cucjo01)
**      When stopping ingres as a service run ingstart -service so messages display
**      as the service is starting.  The old method of starting through the SCM has
**      been replaced.
**  02-feb-2000 (cucjo01)
**      Default the "Start Ingres as a service" checkbox to off.
**  08-feb-2000 (cucjo01)
**      Replaced PCget_PidFromName() with ping_iigcn() to detect name server.
**      Removed suffix ".exe" from *_EXE names to support UNIX platforms as well.
**      Hide the "Start Ingres as a service" and "Client only" checkboxes on UNIX.
**      Typecasted CString.Format() calls to work with MAINWIN.
**      Updated title bar caption to display the installation idenitifier.
**  09-may-2000 (somsa01)
**	Removed the ".exe" extension for other product executable runs.
**  14-jun-2000 (mcgem01)
**	Made generic to accomodate other products.
**  01-dec-2000 (penga03)
**      Destory the semaphore when exit winstart.
**  08-feb-2001 (somsa01)
**              On i64_win, we use GCLP_HICON rather than GCL_HICON.
**      22-nov-2001 (somsa01)
**              Further corrected i64_win warnings.
**  05-Dec-2001 (hweho01)
**      Declared WBCheckActiveInstallation() before it is referenced,   
**      avoid compiler error.
**  25-Oct-2006 (drivi01)
**	When winstart is launched from within another program such as
**	installer, it will close dialog automatically now. 
**	This will only happen if user passed /start or /stop flag.
**  02-May-2007 (drivi01)
**      Change Service Manager title to not contain Ingres Version
**      string.  Pre-installer has problems finding the window
**      on the desktop when name changes.
**  25-Jul-2007 (drivi01)
**	On Vista, Ingres should always start as a service.
**	On Vista, service checkbox will always be checked.
**	Update the code to integrate it with ivm. On Vista,
**	ivm will now use winstart to start/stop the server
**	or individual servers, winstart has been updated
**	to take individual servers as paramters.
**  14-Apr-2008 (drivi01)
**	IDS_MUST_START_AS_SERVICE error should only be thrown
**	if one attempts to start Ingres on Vista without 
**	service.  However, the error should not show up
**	when Ingres is being shut down.
**  06-Apr-2010 (drivi01)
**	Update OnTimer to take UINT_PTR as a parameter as UINT_PTR
**	will be unsigned int on x86 and unsigned int64 on x64.
**	This will cleanup warnings.
**	
*/

#include "stdafx.h"
#include "winstart.h"
#include "winstdlg.h"
#include <compat.h>
#include <er.h>
#include <nm.h>
#include <st.h>
#include <gl.h>
#include <gvcl.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

extern "C" long ping_iigcn();

#define _INGSTART_EXE "ingstart"
#define _INGSTOP_EXE "ingstop"

/////////////////////////////////////////////////////////////////////////////
// CWinstartDlg dialog

CWinstartDlg::CWinstartDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CWinstartDlg::IDD, pParent), 
		m_csBuffer('\0', BUFSIZE), m_bCancelRequested(FALSE),
		m_csCmdLine('\0', 256) , m_bIngStartCompleted(FALSE)
{
	//{{AFX_DATA_INIT(CWinstartDlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CWinstartDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWinstartDlg)
	DDX_Control(pDX, IDC_CLIENT, m_cbStartAsClient);
	DDX_Control(pDX, IDC_START_AS_SERVICE, m_cbStartAsService);
	DDX_Control(pDX, IDC_STATUS, m_txStatusText);
	DDX_Control(pDX, IDC_START, m_bnStart);
	DDX_Control(pDX, IDR_MAINFRAME, m_icIcon);
	DDX_Control(pDX, IDC_MESSAGE, m_txMessage);
	DDX_Control(pDX, IDCANCEL, m_bnCancel);
	DDX_Control(pDX, IDC_LOG, m_ceLog);
	DDX_Control(pDX, IDC_KILL, m_cbKill);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CWinstartDlg, CDialog)
	//{{AFX_MSG_MAP(CWinstartDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_START, OnStart)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_START_AS_SERVICE, OnStartAsService)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

DWORD WBCheckActiveInstallation(LPVOID pParam)
{
	return ((CWinstartDlg*)pParam)->CheckActiveInstallation();
}

/////////////////////////////////////////////////////////////////////////////
// CWinstartDlg message handlers

BOOL CWinstartDlg::OnInitDialog()
{   static char	*ii_installation;	/* installation code */
    CString strText;

	CDialog::OnInitDialog();
	CenterWindow();

    char szII_SYSTEM[512];
    
    if ( GetEnvironmentVariable("II_SYSTEM", szII_SYSTEM, sizeof(szII_SYSTEM)) )
      m_II_SYSTEM=szII_SYSTEM;
    else	
    { 
		strText.Format (IDS_II_SYSTEMNOTFOUND, SYSTEM_LOCATION_VARIABLE);
		AfxMessageBox(strText);
      exit(1);
    }

	/* look up the installation code */
	NMgtAt( ERx( "II_INSTALLATION" ), &ii_installation );
	if( ii_installation == NULL || *ii_installation == EOS )
		ii_installation = ERx( "II" );
	else
		ii_installation = STalloc( ii_installation ); 

	m_II_INSTALLATION = ii_installation;

    strText.Format(IDS_CAPTIONTEXT, (LPCTSTR)m_II_INSTALLATION);
    SetWindowText(strText);

	m_bFirstTime=TRUE;
	m_cWinBat[0].WinBatSetCmdLine("ipcclean"); 
	m_cWinBat[1].WinBatSetCmdLine(_INGSTART_EXE); 
	m_cWinBat[2].WinBatSetCmdLine(_INGSTOP_EXE);
	InitializeCriticalSection(&m_cs);
	SetState(STATE_CHECKING);
    m_pcWinBat->WinBatSetThread(AfxBeginThread((AFX_THREADPROC)WBCheckActiveInstallation, this));

    m_cbKill.EnableWindow(FALSE);

    m_cbStartAsService.EnableWindow(FALSE);
    m_cbStartAsClient.EnableWindow(FALSE);

	m_bnStart.SetFocus();
    
    OSVERSIONINFO lpvers;
    lpvers.dwOSVersionInfoSize= sizeof(OSVERSIONINFO);
    GetVersionEx(&lpvers);
    BOOL Is_w95=(lpvers.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) ? TRUE : FALSE;

    #ifdef MAINWIN
    { m_cbStartAsService.SetCheck(0);
      m_cbStartAsService.ShowWindow(SW_HIDE);

      m_cbStartAsClient.SetCheck(0);
      m_cbStartAsClient.ShowWindow(SW_HIDE);
    }
    #else
    if (Is_w95 == TRUE)
    { m_cbStartAsService.SetCheck(0);
      m_cbStartAsService.ShowWindow(SW_HIDE);

      m_cbStartAsClient.SetCheck(0);
      m_cbStartAsClient.ShowWindow(SW_HIDE);
    }
    else
    { m_cbStartAsService.SetCheck(0);
      m_cbStartAsService.ShowWindow(SW_SHOW);
      
      m_cbStartAsClient.SetCheck(0);
      m_cbStartAsClient.ShowWindow(SW_SHOW);
	  m_cbStartAsClient.EnableWindow(TRUE);
    }
    #endif  // (MAINWIN)

    if (m_bStartFlag)
      m_cbKill.ShowWindow(SW_HIDE);

	if (GVvista())
		m_cbStartAsService.SetCheck(1);

	return FALSE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CWinstartDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		EnterCriticalSection(&m_cs);
		dc.DrawIcon(x, y, m_hIcon);
		LeaveCriticalSection(&m_cs);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to
// display while the user drags the minimized window.
HCURSOR CWinstartDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CWinstartDlg::SetState(DIALOG_STATE st)
{
	int iStartButtonText = 0, iMessage = 0, iStatusText = 0;
	BOOL bEnableStartButton = TRUE, bEnableKill = TRUE;
    BOOL bEnableStartAsService=TRUE, bEnableStartAsClient = TRUE;

	KillTimer(m_timerEvent);
	EnterCriticalSection(&m_cs);
	switch (st)
    {
     	case STATE_CHECKING:
			m_bRunning = TRUE;
			iStartButtonText = IDS_START;
			iMessage = IDS_CHECKINGMESSAGE;
            iStatusText = IDS_CHECKING;
			m_iIcon = IDI_WAIT;
			m_iAvi = IDR_YELLOW_AVI;
            m_iAviRun = TRUE;
			bEnableStartButton = FALSE;
			bEnableKill = FALSE;
            bEnableStartAsService = FALSE;
            bEnableStartAsClient = FALSE;
			m_pcWinBat = &m_cWinBat[0];
			m_bCloseDialog = FALSE;
			break;

		case STATE_WAITINGTOSTART:
			m_bRunning = FALSE;
			iStartButtonText = IDS_START;
			iMessage = IDS_STARTMESSAGE;
            iStatusText = IDS_STOPPED;
			m_iIcon = IDI_STOP;
			m_iAvi = IDR_RED_AVI;
			m_iAviRun = FALSE;
			bEnableStartButton = TRUE;
			bEnableKill = FALSE;
			bEnableStartAsService = (ServerServiceExists() ? TRUE : FALSE);
            bEnableStartAsClient = (ClientServiceExists() ? TRUE : FALSE);
           
            if (m_bStopFlag)
            { m_bnStart.ShowWindow(SW_HIDE);
              m_cbStartAsService.ShowWindow(SW_HIDE);
              m_cbStartAsClient.ShowWindow(SW_HIDE);
            }
            break;

		case STATE_STARTING:
			m_bRunning = TRUE;
			iStartButtonText = IDS_START;
			iMessage = IDS_STARTINGMESSAGE;
            iStatusText = IDS_STARTING;
			m_iIcon = IDI_WAIT;
			m_iAvi = IDR_GREEN_AVI;  
            m_iAviRun = TRUE;
			bEnableStartButton = FALSE;
			bEnableKill = FALSE;
			bEnableStartAsService = FALSE;
			bEnableStartAsClient = FALSE;
			m_pcWinBat = &m_cWinBat[1];
			if (m_bStartFlag)
				m_bCloseDialog = TRUE;
			break;

		case STATE_WAITINGTOSTOP:
			m_bRunning = FALSE;
			iStartButtonText = IDS_STOP;
			iMessage = IDS_RUNNINGMESSAGE;
            iStatusText = IDS_RUNNING;
			m_iIcon = IDR_START;
			m_iAvi = IDR_GREEN_AVI;
			m_iAviRun = FALSE;
			bEnableStartButton = TRUE;
			bEnableKill = TRUE;
			bEnableStartAsService = FALSE;
			bEnableStartAsClient = FALSE;
            if (m_bStartFlag)
               m_bnStart.ShowWindow(SW_HIDE);
            break;

		case STATE_STOPPING:
			m_bRunning = TRUE;
			iStartButtonText = IDS_STOP;
			iMessage = IDS_TERMINATINGMESSAGE;
            iStatusText = IDS_STOPPING;
			m_iIcon = IDI_WAIT;
			m_iAvi = IDR_RED_AVI;
			m_iAviRun = TRUE;
			bEnableStartButton = FALSE;
			bEnableKill = FALSE;
			bEnableStartAsService = FALSE;
			bEnableStartAsClient = FALSE;
			m_pcWinBat = &m_cWinBat[2];
			if (m_bStopFlag)
				m_bCloseDialog = TRUE;
			break;

		case STATE_FAILED:
			
            if (IngresAlreadyRunning())
        	{  m_bRunning = TRUE;
               iStartButtonText = IDS_STOP;
               bEnableStartAsService = FALSE;
               bEnableStartAsClient = FALSE;
               bEnableKill = TRUE; 

               if (IngresRunningAsService())
                  m_cbStartAsService.SetCheck(1);
               else
                  m_cbStartAsService.SetCheck(0);

               if (IngresRunningAsClientService())
               {  m_cbStartAsClient.SetCheck(1);
                  m_cbStartAsService.SetCheck(1);
               }
               else       
                  m_cbStartAsClient.SetCheck(0);

            }
            else
        	{  m_bRunning = FALSE;
               iStartButtonText = IDS_START;
               bEnableStartAsService = TRUE;
               bEnableKill = FALSE;
            }

			iMessage = IDS_FAILEDMESSAGE;
            iStatusText = IDS_FAILED;
			m_iIcon = IDI_STOP;
			m_iAvi = IDR_YELLOW_AVI;
			m_iAviRun = FALSE;
			bEnableStartButton = TRUE;
			break;
	} 
	
    if (iStartButtonText)
    { m_csBuffer.LoadString(iStartButtonText);
	  m_bnStart.SetWindowText(m_csBuffer.GetBuffer(1));
	}

	if (iMessage)
		m_csBuffer.Format(iMessage, SYSTEM_PRODUCT_NAME);
	else
		m_csBuffer = "";

	m_txMessage.SetWindowText(m_csBuffer.GetBuffer(1));

    if (iStatusText)
		m_csBuffer.LoadString(iStatusText);
	else
		m_csBuffer = "";

	m_txStatusText.SetWindowText(m_csBuffer.GetBuffer(1));

	m_csBuffer.LoadString(m_bRunning ? IDS_CANCEL : IDS_EXIT);
	m_bnCancel.SetWindowText(m_csBuffer.GetBuffer(1));

	m_hIcon = AfxGetApp()->LoadIcon(m_iIcon);
#ifdef WIN64
	SetClassLongPtr(m_hWnd, GCLP_HICON, (LONG_PTR)m_hIcon);
#else
	SetClassLong(m_hWnd, GCL_HICON, (LONG)m_hIcon);
#endif
	m_icIcon.SetIcon(m_hIcon);

	if (m_strParam.Find("-kill")>=0)
		m_cbKill.SetCheck(1);
	m_cbKill.EnableWindow(bEnableKill);
    m_cbStartAsService.EnableWindow(bEnableStartAsService);
    m_cbStartAsClient.EnableWindow(bEnableStartAsClient);
    m_bnStart.EnableWindow(bEnableStartButton);	
	
    LeaveCriticalSection(&m_cs);
    
    CWnd *hAniWnd = GetDlgItem(IDC_AVI);
    if (!hAniWnd)
      return;

    hAniWnd->SendMessage(ACM_OPEN, 0, (LPARAM)(LPTSTR)(MAKEINTRESOURCE(m_iAvi)));
    
    if (m_iAviRun)
      hAniWnd->SendMessage(ACM_PLAY, (WPARAM)(UINT)(-1), (LPARAM)MAKELONG(0, 0xFFFF));
    else
      hAniWnd->SendMessage(ACM_STOP, 0, 0);

	
}

void CWinstartDlg::SetMessage(WINBATRC wbrc, CWinBat *pcWinBat, CString *lpcsMessage /*=NULL*/)
{
	CString csMessageText('\0', MAXSTRINGSIZE);
			
	if (wbrc == WINBAT_OK)
	  csMessageText = *lpcsMessage;
	else
	{ csMessageText.Format(wbrc, pcWinBat->WinBatGetError());
      csMessageText+="\r\n";
    }

 	int nLen = m_ceLog.GetWindowTextLength();
 	m_ceLog.SetSel(nLen, nLen);
	m_ceLog.ReplaceSel(csMessageText.GetBuffer(1));
}

DWORD CWinstartDlg::ThreadControl(void)
{
	CString csBuffer('\0', CWinstartDlg::BUFSIZE);
	WINBATRC wbrc;
	CWinBat *pcWinBat = m_pcWinBat;

	pcWinBat->SetThreadTerminated(FALSE);
	wbrc = pcWinBat->WinBatStart();
	if (wbrc != WINBAT_OK)
    {
		SetMessage(wbrc, pcWinBat);	
		pcWinBat->SetThreadTerminated(TRUE);
		SetState(STATE_FAILED);
		pcWinBat->WinBatCloseAll();
		return 0;
	}

	CString csStarted, csRunning, csFailed;
	csStarted.Format(IDS_SERVERSTARTED, SYSTEM_PRODUCT_NAME);
	csRunning.Format(IDS_ALREADYRUNNING, SYSTEM_PRODUCT_NAME, SYSTEM_EXEC_NAME);
	csFailed.LoadString(IDS_SERVERFAILED);

	while (!m_bCancelRequested && (m_rc = 
		pcWinBat->WinBatRead(csBuffer.GetBuffer(CWinstartDlg::MINDATALEN), 
							CWinstartDlg::BUFSIZE - 1, &m_dwBytesRead)) == WINBAT_MOREDATA) {
		if (strlen(csBuffer.GetBuffer(CWinstartDlg::MINDATALEN))) {
			SetMessage(WINBAT_OK, pcWinBat, &csBuffer);
			if (csBuffer.Find(csStarted.GetBuffer(1)) != -1 ||
				csBuffer.Find(csRunning.GetBuffer(1)) != -1)
            {
             	m_bIngStartCompleted = TRUE;
				SetState(STATE_WAITINGTOSTOP);
				if (!m_bRunning && m_bCloseDialog && m_bStartFlag)
					OnCancel();
			}
            else if (csBuffer.Find(csFailed.GetBuffer(1)) != -1)
            {
				m_bIngStartCompleted = FALSE;
				SetState(STATE_FAILED);
			}
			else if (m_bParamFlag && m_bStartFlag)
			{
				SetState(STATE_WAITINGTOSTOP);
			}
			else if (m_bParamFlag && m_bStopFlag)
			{
				SetState(STATE_WAITINGTOSTART);
			}
			*csBuffer.GetBuffer(CWinstartDlg::MINDATALEN) = '\0';
		}
	}

 	pcWinBat->SetThreadTerminated(TRUE);
	
    if (m_bCancelRequested) 
	{	m_bCancelRequested = FALSE;
		csBuffer.LoadString(IDS_PROCESSABORTED);
		SetMessage(WINBAT_OK, pcWinBat, &csBuffer);
	}

 	CheckActiveInstallation();   // Reset Dialog to correct values

	pcWinBat->WinBatCloseAll();

	return 0;
}

// If Ingres is running, we will check if it running as a service,
// so we know whether to stop the service or use the "ingstop" command.
// This also sets the initial dialog status of "stopped" or "started"
DWORD CWinstartDlg::CheckActiveInstallation(void)
{
    if (IngresAlreadyRunning())
    { 
	  SetState(STATE_WAITINGTOSTOP);
      m_bIngStartCompleted = TRUE;
 
      m_cbStartAsService.EnableWindow(FALSE);
      m_cbStartAsClient.EnableWindow(FALSE);

      if (IngresRunningAsService())
         m_cbStartAsService.SetCheck(1);
      else       
         m_cbStartAsService.SetCheck(0);

      if (IngresRunningAsClientService())
      {  m_cbStartAsClient.SetCheck(1);
         m_cbStartAsService.SetCheck(1);
      }
      else       
         m_cbStartAsClient.SetCheck(0);

      if (m_bFirstTime && m_bStopFlag)
      { m_bFirstTime=FALSE;
        OnStart();
      }
	  if (m_bFirstTime && m_bParamFlag && m_bStartFlag)
	  {
		SetState(STATE_WAITINGTOSTART);
		m_bIngStartCompleted = FALSE;
		m_bFirstTime = FALSE;
		OnStart();
	  }

    }
	else
    {
		
		SetState(STATE_WAITINGTOSTART);
		m_bIngStartCompleted = FALSE;

		m_cbStartAsService.EnableWindow(TRUE);

		if (m_cbStartAsService.GetCheck())
			m_cbStartAsClient.EnableWindow(FALSE);
		else
			m_cbStartAsClient.EnableWindow(TRUE);
   
		if (m_bFirstTime && m_bStartFlag)
		{ m_bFirstTime=FALSE;
			OnStart();
		}

		if (!m_bRunning && m_bCloseDialog && m_bStopFlag)
			OnCancel();

	}

	
	
    
    return 0;
}


DWORD WBThreadControl(LPVOID pParam)
{
	return ((CWinstartDlg*)pParam)->ThreadControl();
}

void CWinstartDlg::OnCancel() 
{
	CloseHandle(hSemaphore);
	// TODO: Add extra cleanup here
    if (m_bRunning)
    {
		MessageBeep(MB_ICONQUESTION);
		if (AfxMessageBox(WINBAT_AREYOUSURE, MB_YESNO) == IDNO) 
			return;
		m_bCancelRequested = TRUE;
	}

	KillTimer(m_timerEvent);	

	if (m_bCancelRequested)
    {
		for (int i = 0; i < sizeof(m_cWinBat) / sizeof(m_cWinBat[0]); i++)
		{	CWinBat *pcWinBat = &m_cWinBat[i];
			if (!pcWinBat->GetThreadTerminated())
            { CString csBuffer;
				
			  csBuffer.LoadString(WINBAT_TERMINATINGEXEC);
			  SetMessage(WINBAT_OK, pcWinBat, &csBuffer);
			  DWORD rc = pcWinBat->WinBatTerminate();
			  Sleep(100);
			}
		}
	}

	DeleteCriticalSection(&m_cs);
	CDialog::OnCancel();
}

UINT ThreadCall_StartIngresAsService(LPVOID pParam)
{ ASSERT(pParam != NULL);
  
  ((CWinstartDlg*)pParam)->StartIngresAsService();
  
  return (0);
}

UINT ThreadCall_StopIngresAsService(LPVOID pParam)
{ ASSERT(pParam != NULL);
  
  ((CWinstartDlg*)pParam)->StopIngresAsService();
  
  return (0);
}

void CWinstartDlg::OnStart() 
{  CString strCmdLine;
	
	if (GVvista() && !m_cbStartAsClient.GetCheck()
			&& !m_cbStartAsService.GetCheck() && !m_bIngStartCompleted)
	{
		strCmdLine.Format(IDS_MUST_START_AS_SERVICE);
		AfxMessageBox(strCmdLine, MB_OK);
		return;
	}

	if (!m_bIngStartCompleted)
    {	// Starting CA-OpenIngres
		SetState(STATE_STARTING);

		if (m_bParamFlag)
		{ strCmdLine.Format("%s %s", _INGSTART_EXE, (LPCTSTR)m_strParam);
          m_cWinBat[1].WinBatSetCmdLine(strCmdLine.GetBuffer(255));
             
          // SetMessage(WINBAT_OK, NULL, &strCmdLine);  // Display Command Line In Window
        }
		else if (m_cbStartAsService.GetCheck())
        { strCmdLine.Format("%s -service", _INGSTART_EXE);
		  m_cWinBat[1].WinBatSetCmdLine(strCmdLine.GetBuffer(255));
        }
        else if (m_cbStartAsClient.GetCheck())
        { strCmdLine.Format("%s -client", _INGSTART_EXE);
		  m_cWinBat[1].WinBatSetCmdLine(strCmdLine.GetBuffer(255));
        }
		else
		  m_cWinBat[1].WinBatSetCmdLine(_INGSTART_EXE);
           
        m_pcWinBat = &m_cWinBat[1];
        m_pcWinBat->WinBatSetThread(AfxBeginThread((AFX_THREADPROC)WBThreadControl, this));
	}
    else
    {	// Terminating CA-OpenIngres
        SetState(STATE_STOPPING);   				
        m_bIngStartCompleted = FALSE;
		
        if (m_bParamFlag)
		{ strCmdLine.Format("%s %s", _INGSTOP_EXE, (LPCTSTR)m_strParam);
          m_cWinBat[2].WinBatSetCmdLine(strCmdLine.GetBuffer(255));

          // SetMessage(WINBAT_OK, NULL, &strCmdLine);  // Display Command Line In Window

        }
        else if (m_cbKill.GetCheck())
        { strCmdLine.Format ("%s %s", _INGSTOP_EXE, " -kill");
		  m_cWinBat[2].WinBatSetCmdLine(strCmdLine.GetBuffer(255));
        }
		else
		  m_cWinBat[2].WinBatSetCmdLine(_INGSTOP_EXE);
		
        m_pcWinBat = &m_cWinBat[2];
 		m_pcWinBat->WinBatSetThread(AfxBeginThread((AFX_THREADPROC)WBThreadControl, this));
	}

    return;

}

void CWinstartDlg::OnTimer(UINT_PTR nIDEvent) 
{
	// TODO: Add your message handler code here and/or call default
	
	EnterCriticalSection(&m_cs);
	if (m_iIcon == IDI_WAIT || m_iIcon == IDI_TLIGHT) {
		if (m_iIcon == IDI_WAIT)	{
			m_iIcon = IDI_TLIGHT;
		} else {
			m_iIcon = IDI_WAIT;
		}
		m_hIcon = AfxGetApp()->LoadIcon(m_iIcon);
#ifdef WIN64
		SetClassLongPtr(m_hWnd, GCLP_HICON, (LONG_PTR)m_hIcon);
#else
		SetClassLong(m_hWnd, GCL_HICON, (LONG)m_hIcon);
#endif
		m_icIcon.SetIcon(m_hIcon);
	}
	LeaveCriticalSection(&m_cs);

	m_timerEvent = (UINT)nIDEvent;
	CDialog::OnTimer(nIDEvent);

}

void CWinstartDlg::StartIngresAsService()
{
  DWORD dwError = 0;
  SC_HANDLE schService;
  SC_HANDLE schSCManager;
  SERVICE_STATUS ssStatus;
  CString strBuffer;
  CString strServiceLongName, strServiceShortName;
  bool bStartSuccessful=TRUE;
  int rc, iCount = 0;

  m_bnCancel.EnableWindow(FALSE);	

  BOOL bStartAsClient = m_cbStartAsClient.GetCheck();
  if (bStartAsClient)
  { strServiceLongName.LoadString(IDS_SERVICE_CLIENT_LONGNAME);
    strServiceShortName.LoadString(IDS_SERVICE_CLIENT_SHORTNAME);
  } 
  else
  { strServiceLongName.Format(IDS_SERVICE_SERVER_LONGNAME, m_II_INSTALLATION);
    strServiceShortName.Format(IDS_SERVICE_SERVER_SHORTNAME, m_II_INSTALLATION);
  } 
  
  strBuffer.Format(IDS_STARTING_INGRES_AS_SERVICE, SYSTEM_PRODUCT_NAME, strServiceLongName);
  SetMessage(WINBAT_OK, NULL, &strBuffer);
   
  if ((schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS)))   // local machine
  {
      if ((schService = OpenService(schSCManager, strServiceShortName, SERVICE_ALL_ACCESS)))
      {
          if ( QueryServiceStatus( schService, &ssStatus ) )
          {
              if (ssStatus.dwCurrentState == SERVICE_STOPPED)
              { int rc;
                
                rc = StartService(schService, 0, NULL);
                
                if (!rc)
                { MessageBeep(MB_ICONEXCLAMATION);
                  rc = GetLastError();
                  GetNTErrorText(rc, strBuffer);
                  SetMessage(WINBAT_OK, NULL, &strBuffer);

                  if (rc == ERROR_SERVICE_LOGON_FAILED)
                  { strBuffer.Format(IDS_ERROR_LOGON_FAILED, strServiceLongName);
                    SetMessage(WINBAT_OK, NULL, &strBuffer);
                  }

                  bStartSuccessful=FALSE;
                }
    
                if ( QueryServiceStatus(schService, &ssStatus) )
                { while (ssStatus.dwCurrentState == SERVICE_START_PENDING)
                  { QueryServiceStatus(schService, &ssStatus);
                    Sleep(500);
                    iCount+=1;
                    if (iCount > 360)  // 3 Minutes (180 Seconds) until timeout
                    { MessageBeep(MB_ICONEXCLAMATION);
                      strBuffer.Format(IDS_ERROR_SERVICE_TIMEOUT, SYSTEM_PRODUCT_NAME);
                      SetMessage(WINBAT_OK, NULL, &strBuffer);
                      bStartSuccessful=FALSE;
                    }
                  }
                                   
                }
              }  
          }  // if ( QueryServiceStatus( schService, &ssStatus ) )
      }
      else
      { MessageBeep(MB_ICONEXCLAMATION);
        rc = GetLastError();
        GetNTErrorText(rc, strBuffer);
        SetMessage(WINBAT_OK, NULL, &strBuffer);
        
        bStartSuccessful=FALSE;
      }
         
  }

  if (bStartSuccessful)
  { strBuffer.Format(IDS_SERVICE_SUCCESSFULLY_STARTED, SYSTEM_PRODUCT_NAME);
    SetMessage(WINBAT_OK, NULL, &strBuffer);
    SetState(STATE_WAITINGTOSTOP);
    m_bIngStartCompleted = TRUE;
  }
  else
  { SetState(STATE_FAILED);
  }

  if (schService)
    CloseServiceHandle(schService);
  
  if (schSCManager)
    CloseServiceHandle(schSCManager);

  m_bnCancel.EnableWindow(TRUE);

  return;
}

void CWinstartDlg::StopIngresAsService()
{
  DWORD dwError = 0;
  SC_HANDLE schService;
  SC_HANDLE schSCManager;
  SERVICE_STATUS ssStatus;
  CString strBuffer;
  CString strServiceLongName, strServiceShortName;
  bool bStopSuccessful=TRUE;
  int rc, iCount = 0;

  m_bnCancel.EnableWindow(FALSE);
 
  BOOL bStartAsClient = m_cbStartAsClient.GetCheck();
  if (!bStartAsClient)
  { strServiceLongName.Format(IDS_SERVICE_SERVER_LONGNAME, m_II_INSTALLATION);
    strServiceShortName.Format(IDS_SERVICE_SERVER_SHORTNAME, m_II_INSTALLATION);
  } 
  else
  { strServiceLongName.LoadString(IDS_SERVICE_CLIENT_LONGNAME);
    strServiceShortName.LoadString(IDS_SERVICE_CLIENT_SHORTNAME);
  } 

  strBuffer.Format(IDS_STOPPING_INGRES_AS_SERVICE, SYSTEM_PRODUCT_NAME, strServiceLongName);
  SetMessage(WINBAT_OK, NULL, &strBuffer);

  if ((schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS)))   // local machine
  {
      if (schService = OpenService(schSCManager, strServiceShortName, SERVICE_ALL_ACCESS))
      {  
          if ( QueryServiceStatus(schService, &ssStatus) )
          { 
              if (ssStatus.dwCurrentState != SERVICE_STOPPED)
              { 
                rc = ControlService(schService, SERVICE_CONTROL_STOP, &ssStatus);
              
                if (!rc)
                { MessageBeep(MB_ICONEXCLAMATION);
                  rc = GetLastError();
                  GetNTErrorText(rc, strBuffer);
                  SetMessage(WINBAT_OK, NULL, &strBuffer);
 
                  bStopSuccessful=FALSE;
                }
    
              }  
          }  // if ( QueryServiceStatus( schService, &ssStatus ) )
      }
      else
      { MessageBeep(MB_ICONEXCLAMATION);
        rc = GetLastError();
        GetNTErrorText(rc, strBuffer);
        SetMessage(WINBAT_OK, NULL, &strBuffer);
        
        bStopSuccessful=FALSE;
      }

  }

  if (bStopSuccessful)
  { strBuffer.Format(IDS_SERVICE_SUCCESSFULLY_STOPPED, SYSTEM_PRODUCT_NAME);
    SetMessage(WINBAT_OK, NULL, &strBuffer);
    SetState(STATE_WAITINGTOSTART);
  }
  else
  { SetState(STATE_FAILED);
  }

  if (schService)
    CloseServiceHandle(schService);
  
  if (schSCManager)
    CloseServiceHandle(schSCManager);

  m_bnCancel.EnableWindow(TRUE);
    
  return;
} 

BOOL CWinstartDlg::IngresAlreadyRunning()
{
  if (!ping_iigcn())
    return TRUE;
  else
    return FALSE;
}

BOOL CWinstartDlg::IngresRunningAsService()
{
  DWORD dwError = 0;
  SC_HANDLE schService;
  SC_HANDLE schSCManager;
  SERVICE_STATUS ssStatus;
  CString strServiceName;
  BOOL bServiceRunning = FALSE;
  
  strServiceName.Format(IDS_SERVICE_SERVER_SHORTNAME, m_II_INSTALLATION);
  
  if ((schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS)))   // local machine
  {
      if ((schService = OpenService(schSCManager, strServiceName, SERVICE_ALL_ACCESS)))
      {
          if ( QueryServiceStatus( schService, &ssStatus ) )
          {
              if (ssStatus.dwCurrentState == SERVICE_RUNNING)
                 bServiceRunning = TRUE;
    	  }
      }
  }
 
  if (schService)
    CloseServiceHandle(schService);
  
  if (schSCManager)
    CloseServiceHandle(schSCManager);

  return bServiceRunning;

}

BOOL CWinstartDlg::IngresRunningAsClientService()
{
  DWORD dwError = 0;
  SC_HANDLE schService;
  SC_HANDLE schSCManager;
  SERVICE_STATUS ssStatus;
  CString strServiceName;
  BOOL bServiceRunning = FALSE;
  
  strServiceName.LoadString(IDS_SERVICE_CLIENT_SHORTNAME);
  
  if ((schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS)))   // local machine
  {
      if ((schService = OpenService(schSCManager, strServiceName, SERVICE_ALL_ACCESS)))
      {
          if ( QueryServiceStatus( schService, &ssStatus ) )
          {
              if (ssStatus.dwCurrentState == SERVICE_RUNNING)
                 bServiceRunning = TRUE;
    	  }
      }
  }
 
  if (schService)
    CloseServiceHandle(schService);
  
  if (schSCManager)
    CloseServiceHandle(schSCManager);

  return bServiceRunning;

}

BOOL CWinstartDlg::ClientServiceExists()
{
  SC_HANDLE schService;
  SC_HANDLE schSCManager;
  CString strServiceName;
  BOOL bClientServiceExists = FALSE;

  if ((schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS)))   // local machine
  {
    strServiceName.LoadString(IDS_SERVICE_CLIENT_SHORTNAME);
    if ((schService = OpenService(schSCManager, strServiceName, GENERIC_READ)))
    { bClientServiceExists=TRUE;
      CloseServiceHandle(schService);
    }
  }

  if (schSCManager)
    CloseServiceHandle(schSCManager);

  return bClientServiceExists;
}

BOOL CWinstartDlg::ServerServiceExists()
{
  SC_HANDLE schService;
  SC_HANDLE schSCManager;
  CString strServiceName;
  BOOL bDatabaseServiceExists = FALSE;

  if ((schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS)))   // local machine
  {
    strServiceName.Format(IDS_SERVICE_SERVER_SHORTNAME, m_II_INSTALLATION);
    if ((schService = OpenService(schSCManager, strServiceName, GENERIC_READ)))
    { bDatabaseServiceExists=TRUE;
      CloseServiceHandle(schService);
    }
  }

  if (schSCManager)
    CloseServiceHandle(schSCManager);

  return bDatabaseServiceExists;
}

void CWinstartDlg::OnStartAsService() 
{
   if (m_cbStartAsService.GetCheck())
   {  m_cbStartAsClient.EnableWindow(ClientServiceExists() ? TRUE : FALSE);	
      m_cbStartAsClient.SetCheck(0);
   }
   else
      m_cbStartAsClient.EnableWindow(TRUE);	
}

// Display NT error text (based on function from brawi05)
void CWinstartDlg::GetNTErrorText(int code, CString &strErrText)
{
    DWORD dwRet;
    LPTSTR lpszTemp = NULL;

    dwRet = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_ARGUMENT_ARRAY,
                           NULL,
                           code,
                           LANG_NEUTRAL,
                           (LPTSTR)&lpszTemp,
                           0,
                           NULL );

    // supplied buffer is not long enough
    if ( !dwRet )
        strErrText.Format("Cannot obtain error information for NT error #%i\r\n", code);
    else
    {
        lpszTemp[_tcslen(lpszTemp)-2] = _T('\0');  //remove cr and newline character
        strErrText.Format("%s (%i)\r\n", lpszTemp, code);
    }

    if ( lpszTemp )
        LocalFree((HLOCAL) lpszTemp );

    return;
}
