/*
**  Copyright (C) 2005-2008 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xdgabout.cpp : implementation file
**    Project  : DLL (About Box). 
**    Author   : Sotheavut UK (uk$so01)
**
**
** History:
**
** 06-Nov-2001 (uk$so01)
**    created
** 08-Nov-2001 (uk$so01)
**    SIR #106290 (Handle the 255 palette)
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 11-Jan-2002 (schph01 for uk$so01)
**    (SIR 106290) allow the caller to pass the bitmap
** 20-Jun-2002 (uk$so01)
**    BUG # 108086, Tooltip blinks every second even if the mouse
**    is not in the right place.
** 23-Jun-2004 (schph01)
**   (SIR 112464) remove CA licensing references in About box splash screen.
** 30-Jul-2004 (uk$so01)
**    SIR #112708, Change the AboutBox due to UI Standard.
** 19-Jan-2005 (komve01)
**    BUG #113768 / ISSUE 13913697: 
**	  GUI tools display incorrect year in the Copyright Information.
** 07-Jan-2008 (drivi01)
**    Created copyright.h header for Visual DBA suite.
**    Redefine copyright year as constant defined in new
**    header file.  This will ensure single point of update
**    for variable year.
** 17-Apr-2008 (drivi01)
**	  Moved things around to make it work with new About box.
**	  Increased Font.
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "xdgabout.h"
#include ".\xdgabout.h"
#include "copyright.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define ABOUT_WIDTH    558     // The Widht of about box.
#define ABOUT_HEIGHT   362     // The height of about box

// Copyright upper-left pixel values.
#define EDIT_X         10     
#define EDIT_Y          250     

// Copyright height.
#define EDIT_HEIGHT    100     // ABOUT_HEIGHT - EDIT_Y - 20 - BUTTON_HEIGHT - 20 - BUTTON_HEIGHT - 20

// Logo upper-left, width and height information.
#define LOGO_WIDTH     164
#define LOGO_HEIGHT     34
#define LOGO_X         104     // 84 + 20
#define LOGO_Y          20

// OK button width and height information.
#define BUTTON_WIDTH    69
#define BUTTON_HEIGHT   25

#define SHOW_LICENSExSITE         0x00000001
#define SHOW_COPYRIGHT            0x00000002
#define SHOW_ENDxUSERxLICENSExA   0x00000004
#define SHOW_WARNING              0x00000008
#define SHOW_3xPARTYxNOTICE       0x00000010
#define SHOW_SYSTEMxINFO          0x00000020
#define SHOW_TECHxSUPPORT         0x00000040


/////////////////////////////////////////////////////////////////////////////
// CxDlgAbout dialog


CxDlgAbout::CxDlgAbout(CWnd* pParent)
	: CDialog(CxDlgAbout::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgAbout)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_bChangeCursor = FALSE;
	m_hIcon = NULL;
	m_strTitle = _T("");
	m_strProductVersion = _T("");
	m_nYear = COPYRIGHT_YEAR;
	m_hcArrow = NULL;
	m_hcHand = NULL;
	m_bitmap.SetBitmpapId (IDB_ADVANCED_INGRES_ABOUT);

	m_rgbBkColor = RGB (255, 255, 255); // White
	//m_rgbFgColor = GetSysColor(COLOR_WINDOWTEXT); //Backup color 
	m_rgbFgColor = RGB (1, 95, 133); //Ingres Blue
	m_pEditBkBrush = new CBrush(m_rgbBkColor);
	m_nShowMask = SHOW_WARNING|SHOW_ENDxUSERxLICENSExA|SHOW_LICENSExSITE|SHOW_COPYRIGHT|SHOW_3xPARTYxNOTICE|SHOW_SYSTEMxINFO|SHOW_TECHxSUPPORT;
}

void CxDlgAbout::SetExternalBitmap (HINSTANCE hExternalInstance, UINT nBitmap)
{
	m_bitmap.SetBitmpapId (hExternalInstance, nBitmap);
}


void CxDlgAbout::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgAbout)
	DDX_Control(pDX, IDOK, m_cButtonOK);
	//}}AFX_DATA_MAP

	DDX_Control(pDX, IDC_STATIC1, m_bitmap);
	DDX_Control(pDX, IDC_BUTTON1, m_cButton3Party);
	DDX_Control(pDX, IDC_BUTTON2, m_cButtonSystemInfo);
	DDX_Control(pDX, IDC_BUTTON3, m_cButtonTechSupport);
}


BEGIN_MESSAGE_MAP(CxDlgAbout, CDialog)
	//{{AFX_MSG_MAP(CxDlgAbout)
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_SETCURSOR()
	ON_WM_LBUTTONUP()
	ON_WM_SIZE()
	ON_WM_CTLCOLOR()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton3Party)
	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedButtonSystemInfo)
	ON_BN_CLICKED(IDC_BUTTON3, OnBnClickedButtonTechSupport)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgAbout message handlers

BOOL CxDlgAbout::OnInitDialog() 
{
	USES_CONVERSION;
	CDialog::OnInitDialog();
	m_hcArrow = theApp.LoadStandardCursor (IDC_ARROW);
	m_hcHand  = theApp.LoadCursor (IDC_HAND_RES);

	//
	// Create "Arial 10" font for the copyright information.
	LOGFONT lf;                            // Used to create the CFont.
	memset(&lf, 0, sizeof(LOGFONT));       // Clear out structure.
	lf.lfHeight = -18;                     // Request a 18-pixel-high font
	lstrcpy(lf.lfFaceName, _T("Arial"));  // with face name "Arial".
	m_font.CreateFontIndirect(&lf);        // Create the font.

	lf.lfWeight = FW_BOLD;
	lf.lfHeight = -20;                     // Request a 20-pixel-high font
	m_fontBold.CreateFontIndirect(&lf);    // Create the font.
	lf.lfWeight = 0;
	lf.lfHeight = -12;                     // Request a 12-pixel-high font
	m_font2.CreateFontIndirect(&lf);       // Create the font.

	CString strSiteID , strOrg;
	UINT nStyle = WS_CHILD/*|WS_BORDER*/;
	//
	// Display Product Copyright.
	CString strCopyright;
	CString strTemp, strYear;

	if (m_nShowMask & SHOW_LICENSExSITE)
	{
		/* License code has been removed
		strTemp.LoadString(IDS_LICENSE_TO);
		strCopyright += strTemp;
		strCopyright += strOrg + _T("\r\n");
		strTemp.LoadString(IDS_SITEID);
		strCopyright += strTemp + _T(" ") + strSiteID;
		*/
		strCopyright += _T("\r\n\r\n");
	}
	if (m_nShowMask & SHOW_COPYRIGHT)
	{
		if ((m_nShowMask & SHOW_LICENSExSITE) == 0)
			strCopyright += _T("\r\n\r\n");
		strTemp.LoadString(IDS_COPYRIGHT);
		strYear.Format (strTemp, m_nYear);
		strCopyright += strYear;
	}

	m_cStatic1.Create(m_strTitle, nStyle, CRect(0,0,0,0), this, 1000);
	m_cStatic1.SetFont(&m_fontBold);
	m_cStatic1.MoveWindow(
		EDIT_X, 
		EDIT_Y, 
		ABOUT_WIDTH - EDIT_X - 20 - GetSystemMetrics(SM_CXBORDER), 
		24);
	m_cStatic1.ShowWindow(SW_SHOW);

	//separate product version from the title
	m_cStatic1_5.Create(m_strProductVersion, nStyle, CRect(0,0,0,0), this, 1000);
	m_cStatic1_5.SetFont(&m_font);
	m_cStatic1_5.MoveWindow(
		EDIT_X,
		EDIT_Y+24,
		ABOUT_WIDTH - EDIT_X - 20 - GetSystemMetrics(SM_CXBORDER),
		24);
	m_cStatic1_5.ShowWindow(SW_SHOW);

	m_cStatic2.Create(strCopyright, nStyle, CRect(0,0,0,0), this, 1001);
	m_cStatic2.SetFont(&m_font2);
	m_cStatic2.MoveWindow(
		EDIT_X, 
		EDIT_Y+24+24+6, 
		ABOUT_WIDTH - EDIT_X - 20 - GetSystemMetrics(SM_CXBORDER)-BUTTON_WIDTH, 
		60);
	m_cStatic2.ShowWindow(SW_SHOW);

	if (m_nShowMask & SHOW_WARNING)
	{
		CString s;
		s.LoadString(IDS_WARNING);

		m_cStatic3.Create(s, nStyle, CRect(0,0,0,0), this, 1002);
		m_cStatic3.SetFont(&m_font2);
		m_cStatic3.MoveWindow(
			EDIT_X, 
			EDIT_Y+20+10+80+20+20, 
			ABOUT_WIDTH - EDIT_X - 20 - GetSystemMetrics(SM_CXBORDER), 
			60);
		m_cStatic3.ShowWindow(SW_SHOW);
	}

	if (m_nShowMask & SHOW_ENDxUSERxLICENSExA)
	{
		m_bmpHyperlink.LoadBitmap(IDB_HYPERLINK);
		m_cStaticHyperLink.Create("", WS_CHILD|WS_EX_STATICEDGE, CRect(0,0,0,0), this, 1003);
		m_cStaticHyperLink.SetBitmap((HBITMAP)m_bmpHyperlink);
		m_cStaticHyperLink.MoveWindow(
			EDIT_X, 
			EDIT_Y+18+10+80, 
			184, 
			16);
		m_cStaticHyperLink.ShowWindow(SW_SHOW);
	}

	CRect rcClient;
	GetClientRect (rcClient);
	m_bitmap.MoveWindow (rcClient);
	m_bitmap.SetWindowPos(&wndTop, 0, 0, 0, 0, SWP_NOSIZE);

	m_cButtonOK.MoveWindow(
		ABOUT_WIDTH  - BUTTON_WIDTH  - 20 - GetSystemMetrics(SM_CXBORDER), 
		ABOUT_HEIGHT - 20 - BUTTON_HEIGHT, 
		BUTTON_WIDTH, 
		BUTTON_HEIGHT);
	m_cButtonOK.ShowWindow (SW_SHOW);

	//
	// Display Extra Buttons:
	CRect rb;
	int nW, nBX = EDIT_X;
	if (m_nShowMask & SHOW_3xPARTYxNOTICE)
	{
		m_cButton3Party.GetWindowRect(rb);
		ClientToScreen(rb);
		nW = rb.Width();
		rb.top    = ABOUT_HEIGHT - 2*(20 + BUTTON_HEIGHT);
		rb.bottom = rb.top + BUTTON_HEIGHT;
		rb.left   = nBX;
		rb.right  = rb.left + nW;
		nBX = rb.right + 4; // for the next button X-start
		m_cButton3Party.MoveWindow(rb);
		m_cButton3Party.ShowWindow(SW_SHOW);
	}

	if (m_nShowMask & SHOW_SYSTEMxINFO)
	{
		m_cButtonSystemInfo.GetWindowRect(rb);
		ClientToScreen(rb);
		nW = rb.Width();
		rb.top    = ABOUT_HEIGHT - 2*(20 + BUTTON_HEIGHT);
		rb.bottom = rb.top + BUTTON_HEIGHT;
		rb.left   = nBX;
		rb.right  = rb.left + nW;
		nBX = rb.right + 4; // for the next button X-start
		m_cButtonSystemInfo.MoveWindow(rb);
		m_cButtonSystemInfo.ShowWindow(SW_SHOW);
	}
	if (m_nShowMask & SHOW_TECHxSUPPORT)
	{
		m_cButtonTechSupport.GetWindowRect(rb);
		ClientToScreen(rb);
		nW = rb.Width();
		rb.top    = ABOUT_HEIGHT - 2*(20 + BUTTON_HEIGHT);
		rb.bottom = rb.top + BUTTON_HEIGHT;
		rb.left   = nBX;
		rb.right  = rb.left + nW;
		nBX = rb.right + 4; // for the next button X-start
		m_cButtonTechSupport.MoveWindow(rb);
		m_cButtonTechSupport.ShowWindow(SW_SHOW);
	}

	//
	// Create the ToolTip control.
	m_tooltip.Create(this);
	m_tooltip.Activate(TRUE);

	CRect r (LOGO_X, LOGO_Y, LOGO_X + LOGO_WIDTH, LOGO_Y + LOGO_HEIGHT);
	if(!m_tooltip.AddTool(this, IDS_CAWEB, r, 1010))
	{
		TRACE0("Unable to add Dialog to the tooltip\n");
	}
	if (!m_strTitle.IsEmpty())
	{
		CString strTitle;
		AfxFormatString1 (strTitle, IDS_TITLE, (LPCTSTR)m_strTitle);
		SetWindowText (strTitle);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgAbout::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	m_bitmap.SetWindowPos(&wndTop, 0, 0, 0, 0, SWP_NOSIZE);
}

BOOL CxDlgAbout::PreTranslateMessage(MSG* pMsg)
{
	//
	// Let the ToolTip process this message.
	m_tooltip.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg); // CG: This was added by the ToolTips component.
}

void CxDlgAbout::OnMouseMove(UINT nFlags, CPoint point) 
{
	m_bChangeCursor = FALSE;
	if((point.x > LOGO_X &&
	    point.x < LOGO_X + LOGO_WIDTH &&
	    point.y > LOGO_Y &&
	    point.y < LOGO_Y + LOGO_HEIGHT) ||
	  ((m_nShowMask & SHOW_ENDxUSERxLICENSExA) &&
	   (point.x > EDIT_X &&
	    point.x < EDIT_X + 184 &&
	    point.y > (EDIT_Y+18+10+80) &&
	    point.y < (EDIT_Y+18+10+80 + 16))))
	{
		// It is ready to change mouse and show the tooltip.
		m_bChangeCursor = TRUE;
	}
	else
	{
		m_bChangeCursor = FALSE;
	}

	CDialog::OnMouseMove(nFlags, point);
}

BOOL CxDlgAbout::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
	// When user's mouse moves over the CA Logo. A hand cursor should appear with a tooltip.
	TRACE0("CxDlgAbout::OnSetCursor\n");

	if(m_bChangeCursor)
	{
		::SetCursor(m_hcHand);
		return TRUE;
	}
	else
	{
		::SetCursor(m_hcArrow);
	}
	return CDialog::OnSetCursor(pWnd, nHitTest, message);
}

void CxDlgAbout::OnLButtonUp(UINT nFlags, CPoint point) 
{
	// Check the mouse is on top of CA Logo
	CDialog::OnLButtonUp(nFlags, point);
	if (point.x > LOGO_X &&
	    point.x < LOGO_X + LOGO_WIDTH &&
	    point.y > LOGO_Y &&
	    point.y < LOGO_Y + LOGO_HEIGHT)
	{
		// It is ready to launch the internet explorer.
		ShellExecute(NULL, _T("open"), _T("http://www.ingres.com"), NULL, NULL, SW_SHOW);
	}
	else
	if((m_nShowMask & SHOW_ENDxUSERxLICENSExA) &&
	   (point.x > EDIT_X &&
	    point.x < EDIT_X + 184 &&
	    point.y > (EDIT_Y+18+10+80) &&
	    point.y < (EDIT_Y+18+10+80 + 16)))
	{
		AfxMessageBox("TODO: Popup End-User License Agreement");
	}
}


void CxDlgAbout::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	CPoint p;
	m_bitmap.GetBitmapDimension(p);

	// The about box contains the bitmap, frame and border. A bigger size of windows must
	// be created in order to fit these items.
	int x = p.x + GetSystemMetrics(SM_CXBORDER) + GetSystemMetrics(SM_CXFRAME);
	int y = p.y + GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFRAME);
	CRect r(0, 0, x, y);
	MoveWindow (r);
	CenterWindow();
}

HBRUSH CxDlgAbout::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = NULL;
	
	switch (nCtlColor) 
	{
	case CTLCOLOR_STATIC:
		switch (pWnd->GetDlgCtrlID())
		{
		case 1000:
		case 1001:
		case 1002:
			pDC->SetBkColor(m_rgbBkColor);    // change the background color
			pDC->SetTextColor(m_rgbFgColor);  // change the text color
			hbr = (HBRUSH)(m_pEditBkBrush->GetSafeHandle());
			break;
		default:
			hbr=CDialog::OnCtlColor(pDC, pWnd, nCtlColor); 
			break;
		}
		break;
	default:
		hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	}
	return hbr;
}

void CxDlgAbout::OnDestroy() 
{
	CDialog::OnDestroy();
	if (m_pEditBkBrush)
		delete m_pEditBkBrush;
}

void CxDlgAbout::OnBnClickedButton3Party()
{
	AfxMessageBox("TODO: Popup Third Party Notices");
}

void CxDlgAbout::OnBnClickedButtonSystemInfo()
{
	AfxMessageBox("TODO: Popup System Info");
}

void CxDlgAbout::OnBnClickedButtonTechSupport()
{
	AfxMessageBox("TODO: Popup Tech Support");
}


extern "C" void PASCAL EXPORT AboutBox(LPCTSTR lpszTitle, LPCTSTR lpszProductVersion, short nYear, UINT nSHowMask)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CxDlgAbout dlg;
	if (lpszTitle != NULL)
		dlg.SetTitle(lpszTitle);
	dlg.SetVersion(lpszProductVersion);
	dlg.SetYear (nYear);
	dlg.SetShowMask(nSHowMask);
	dlg.DoModal();
}

extern "C" void PASCAL EXPORT AboutBox2(
	LPCTSTR lpszTitle, 
	LPCTSTR lpszProductVersion, 
	short nYear,
	HINSTANCE hExternalInstance, 
	UINT nBitmap,
	UINT nSHowMask)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CxDlgAbout dlg;
	dlg.SetExternalBitmap (hExternalInstance, nBitmap);
	if (lpszTitle != NULL)
		dlg.SetTitle(lpszTitle);
	dlg.SetVersion(lpszProductVersion);
	dlg.SetYear (nYear);
	dlg.SetShowMask(nSHowMask);
	dlg.DoModal();
}


