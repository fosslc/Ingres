/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
** 
**    Source   : xdgevset.cpp , Implementation File
**    Project  : Ingres II / Visual Manager 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Dialog box for defining the preferences of events 
**
** History:
**
** 21-May-1999 (uk$so01)
**     Created
** 27-Jan-2000 (uk$so01)
**     (Bug #100157): Activate Help Topic
** 23-Mar-2000 (uk$so01)
**    SIR #100706. Provide the different context help id.
**    Additional changes, provide also the help id for corner icon and
**    Pop-up menu in the message category setting dialog box.
** 31-May-2000 (uk$so01)
**    bug 99242 Handle DBCS
** 16-Apr-2003 (hweho01)
**    Fixed buttons overlapping problem on AIX (rs4_us5) platform.  
**    Bug 110094, Star Issue 12606150.
** 21-Nov-2003 (uk$so01)
**    SIR #99596, Remove the class CuStaticResizeMark and the bitmap resizema.bmp
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "xdgevset.h"
#include "fevsetti.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CxDlgEventSetting dialog


CxDlgEventSetting::CxDlgEventSetting(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgEventSetting::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgEventSetting)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_pFrame = NULL;
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}


void CxDlgEventSetting::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgEventSetting)
	DDX_Control(pDX, IDC_STATIC2, m_cStaticProgress);
	DDX_Control(pDX, IDCANCEL, m_cButtonCancel);
	DDX_Control(pDX, IDOK, m_cButtonOK);
	DDX_Control(pDX, IDC_STATIC1, m_cStatic1);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgEventSetting, CDialog)
	//{{AFX_MSG_MAP(CxDlgEventSetting)
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_DESTROY()
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
	ON_WM_QUERYDRAGICON()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgEventSetting message handlers

BOOL CxDlgEventSetting::OnInitDialog() 
{
	CDialog::OnInitDialog();
	// Set the icon for this dialog.  The framework does this automatically
	// when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE); // Set big icon
	SetIcon(m_hIcon, FALSE);// Set small icon

	CdEventSetting* pDoc = new CdEventSetting();
	CfEventSetting* pFrame = new CfEventSetting (pDoc);
	m_pFrame = (CWnd*)pFrame;
	CRect r, rDlg;
	m_cStatic1.GetWindowRect (r);
	ScreenToClient (r);
	pFrame->Create (
		NULL,
		NULL,
		WS_CHILD,
		r,
		this);
	pFrame->ShowWindow (SW_SHOW);
	pFrame->InitialUpdateFrame (NULL, TRUE);
	ResizeControls();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgEventSetting::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	ResizeControls();
}

void CxDlgEventSetting::ResizeControls()
{
	if (!IsWindow (m_cButtonCancel.m_hWnd))
		return;
	CRect r, rDlg;
	GetClientRect (rDlg);
	//
	// Move the Button OK Cancel:
	int H, hTb;
	m_cButtonCancel.GetWindowRect (r);
	H = r.Height();
	ScreenToClient(r);
	r.bottom = rDlg.bottom -4;
	r.top = r.bottom - H;
	m_cButtonCancel.MoveWindow (r);

	m_cButtonOK.GetWindowRect (r);
	H = r.Height();
	ScreenToClient(r);
	r.bottom = rDlg.bottom -4;
	r.top = r.bottom - H;
	m_cButtonOK.MoveWindow (r);
	hTb = r.top;

	m_cStaticProgress.GetWindowRect (r);
	H = r.Height();
	ScreenToClient(r);
	r.bottom = rDlg.bottom -4;
	r.top   = r.bottom - H;
	r.right = rDlg.right -10;
	m_cStaticProgress.MoveWindow (r);

	//
	// Move the Frame:
	if (m_pFrame && IsWindow (m_pFrame->m_hWnd))
	{
		rDlg.bottom = hTb - 10;
		m_pFrame->MoveWindow (rDlg);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.
void CxDlgEventSetting::OnPaint() 
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
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CxDlgEventSetting::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CxDlgEventSetting::OnDestroy() 
{
	CDialog::OnDestroy();
	if (m_pFrame)
		m_pFrame->DestroyWindow();
}

void CxDlgEventSetting::OnOK() 
{
	if (m_pFrame)
	{
		TRACE0 ("CxDlgEventSetting::OnOK()\n");
#if defined (_DEBUG) && defined (SIMULATION)
		((CfEventSetting*)m_pFrame)->SaveMessageState();
		((CfEventSetting*)m_pFrame)->SaveUserCategory();
#else
		((CfEventSetting*)m_pFrame)->SaveSettings();
#endif
	}
	CDialog::OnOK();
	theApp.SetRequestInitializeEvents(TRUE);
}

BOOL CxDlgEventSetting::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	UINT nHelpID = GetHelpID();
	if (pHelpInfo->iContextType == HELPINFO_MENUITEM)
	{
		//
		// Help requested for a menu item:
		nHelpID = pHelpInfo->iCtrlId;
	}
	return APP_HelpInfo(nHelpID);
}
