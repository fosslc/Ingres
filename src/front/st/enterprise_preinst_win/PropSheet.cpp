/*
**  Copyright (c) 2001 Ingres Corporation
*/

/*
**  Name: PropSheet.cpp : implementation file
**
**  History:
**
**	05-Jun-2001 (penga03)
**	    Created.
**	13-jun-2001 (somsa01)
**	    Modified the removing of the HELP button.
**  15-Nov-2006 (drivi01)
**		SIR 116599
**		Enhanced pre-installer in effort to improve installer usability.
**		Added 4 new pages, removed the old 2 in property sheet.
**		Setup Wizard97 style wizard.
**	01-Mar-2007 (drivi01)
**		Modified code to overwrite painting of banner at the top
**	    of the wizard.
**  04-Apr-2008 (drivi01)
**		Adding functionality for DBA Tools installer to the Ingres 
**		code. DBA Tools installer is a simplified package of Ingres 
**		install containing only NET and Visual tools component.  
**		Add a spetial case for the DBA Tools installer in property 
**		sheet to go directly to page 2 which is the page with 
**		instance list. InstanceList page is the only dialog used by 
**		DBA tools installer in pre-installation phase.
**  04-Nov-2009 (drivi01)
**	    Add a license page to property sheet.
*/

#include "stdafx.h"
#include "enterprise.h"
#include "PropSheet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern HINSTANCE g_hinst;
/////////////////////////////////////////////////////////////////////////////
// CPropSheet

IMPLEMENT_DYNAMIC(CPropSheet, CPropertySheet)

CPropSheet::CPropSheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
	SetWizardMode();
   
	AddPage(new CWelcome());
	AddPage(new LicenseInformation()); /* Users must agree to the license */
	AddPage(new InstallType()); /* Allows user to choose action to modify/upgrade/install new */
	AddPage(new InstanceList()); /* Displays the list of existing installations, only displays for modify/upgrade types */
	AddPage(new ConfigType());
	AddPage(new InstallMode()); /* Allows installer to run in two modes: express/advanced */
	AddPage(new ChooseInstallCode()); /* Allows user to accept default instance id or pick their own */

	m_psh.dwFlags &= ~(PSH_HASHELP);
	m_psh.dwFlags |= PSH_WIZARD97|PSH_USEHBMHEADER|PSH_WATERMARK ;

	thePreInstall.m_eHeaderStyle = WIZ97_STRETCH_BANNER;
	thePreInstall.m_uHeaderID=IDB_BITMAPTOP;
	thePreInstall.m_bmpHeader.LoadOneResource(thePreInstall.m_uHeaderID);
	

	m_psh.pszbmWatermark = MAKEINTRESOURCE(IDB_200);
	m_psh.hbmHeader = NULL;

	m_psh.hInstance = AfxGetInstanceHandle(); 
}


CPropSheet::~CPropSheet()
{
	for (int i=0; i<GetPageCount(); i++)
		delete GetPage(i);
}


BEGIN_MESSAGE_MAP(CPropSheet, CPropertySheet)
	//{{AFX_MSG_MAP(CPropSheet)
	ON_WM_ACTIVATE()
	ON_WM_CLOSE()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR()
	ON_WM_QUERYNEWPALETTE()
	ON_WM_PALETTECHANGED()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////
// CPropSheet message handlers

BOOL CPropSheet::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	switch(wParam)
	{
	case IDCANCEL: 
		if (!AskUserYN(IDS_AREYOUSURE))
			return TRUE;
	}


	return CPropertySheet::OnCommand(wParam, lParam);
}

BOOL CPropSheet::OnInitDialog() 
{

	BOOL bResult = CPropertySheet::OnInitDialog();

	CRect rect;
	GetTabControl()->GetWindowRect(&rect);
	rect.left=rect.left+100;
	GetTabControl()->MoveWindow(&rect, 1);

	SetIcon(theIcon, TRUE);
	SetIcon(theIcon, FALSE);

	if (thePreInstall.m_DBATools)
		property->SetActivePage(2);

	return bResult;
}


void CPropSheet::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized) 
{
	CPropertySheet::OnActivate(nState, pWndOther, bMinimized);

	CString s;
	s.LoadString(IDS_TITLE);
	SetWindowText(s);	

}


void CPropSheet::OnClose() 
{
	if(AskUserYN(IDS_AREYOUSURE))
		CPropertySheet::OnClose();
}

void CPropSheet::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); /* device context for painting */
		
		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);
		
		/* Center icon in client rectangle */
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;
		
		/* Draw the icon */
		dc.DrawIcon(x, y, theIcon);
	}
	else
	{
	CPropertySheet::OnPaint();
	}
    
}


HCURSOR CPropSheet::OnQueryDragIcon() 
{
	return (HCURSOR) theIcon;
}

BOOL CPropSheet::OnEraseBkgnd(CDC *pDC)
{
	BOOL bResult = CPropertySheet::OnEraseBkgnd(pDC);
	if (!thePreInstall.m_bmpHeader.IsEmpty() )
	{
		CPropertyPageEx* pPage = (CPropertyPageEx*)property->GetActivePage( );
		if (NULL != pPage)
		{
			BOOL bHasWizard97Header = ( (pPage->m_psp.dwFlags & PSP_HIDEHEADER) == 0);
			printf("blah");
			if (bHasWizard97Header && (thePreInstall.m_bmpHeader.GetPixelPtr() != NULL) )
			{
				// If the page has a header, we need to paint the area above the border.
				// By inspection (Spy++), the border is a static control with ID 0x3027
				CWnd* pTopBorder = property->GetDlgItem(0x3027);
				ASSERT(NULL != pTopBorder);
				if (NULL != pTopBorder)
				{
					CRect rectTopBorder;
					pTopBorder->GetWindowRect(rectTopBorder);

					CRect rc;
					GetClientRect(rc);

					ScreenToClient(rectTopBorder);

					rc.bottom = rectTopBorder.top - 1;

					int x = 0, y = 0;
					switch (thePreInstall.m_eHeaderStyle)
					{
						case WIZ97_STRETCH_BANNER:
							// Stretch bitmap so it will best fit to the dialog
							thePreInstall.m_bmpHeader.DrawDIB(pDC, 0, 0, rc.Width(), rc.Height());
							bResult = TRUE;
							break;

						default:
							// Tile the bitmap
							while (y < rc.Height())
							{
								while (x < rc.Width())
								{
									thePreInstall.m_bmpHeader.DrawDIB(pDC, x, y);
									x += thePreInstall.m_bmpHeader.GetWidth();
								}
								x = 0;
								y += thePreInstall.m_bmpHeader.GetHeight();
							}
							bResult = TRUE;
							break;
					}


				}
			}
		}
	}

	return bResult;
}

HBRUSH CPropSheet::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	if (!thePreInstall.m_bmpHeader.IsEmpty() )
	{
		CPropertyPageEx* pPage = (CPropertyPageEx*)GetActivePage( );
		if (NULL != pPage)
		{
			BOOL bHasWizard97Header = ( (pPage->m_psp.dwFlags & PSP_HIDEHEADER) == 0);
			if (bHasWizard97Header && (thePreInstall.m_bmpHeader.GetPixelPtr() != NULL) )
			{
				// If the page has a header, we need to paint the area above the border.
				// By inspection (Spy++), the border is a static control with ID 0x3027
				CWnd* pTopBorder = GetDlgItem(0x3027);
				ASSERT(NULL != pTopBorder);
				if (NULL != pTopBorder)
				{
					CRect rectTopBorder;
					pTopBorder->GetWindowRect(rectTopBorder);
					ScreenToClient(rectTopBorder);
		
					CRect rectCtrl;
					pWnd->GetWindowRect(rectCtrl);
		
					ScreenToClient(rectCtrl);

					if (rectCtrl.top < rectTopBorder.bottom)
					{
						switch(nCtlColor)
						{
							case CTLCOLOR_STATIC:
								// The Slider Control has CTLCOLOR_STATIC, but doesn't let
								// the background shine through,
								TCHAR lpszClassName[255];
								GetClassName(pWnd->m_hWnd, lpszClassName, 255);
								if (_tcscmp(lpszClassName, TRACKBAR_CLASS) == 0)
								{
									return CPropertySheet::OnCtlColor(pDC, pWnd, nCtlColor);
								}

							case CTLCOLOR_BTN:
								// let static controls shine through
								pDC->SetBkMode(TRANSPARENT);
								return HBRUSH(thePreInstall.m_HollowBrush);

							default:
								break;
						}
					}
				}
			}
		}
	}
	// if we reach this line, we haven't set a brush so far
	return CPropertySheet::OnCtlColor(pDC, pWnd, nCtlColor);
}


BOOL CPropSheet::OnQueryNewPalette(void) 
{
	if (!thePreInstall.m_bmpHeader.IsEmpty() )
	{
		CPalette* pPal = thePreInstall.m_bmpHeader.GetPalette();
		if ( (NULL != pPal) && (NULL != GetSafeHwnd()) )
		{
			CClientDC dc(this);
			CPalette* pOldPalette = dc.SelectPalette(pPal, FALSE);
			UINT nChanged = dc.RealizePalette();
			dc.SelectPalette(pOldPalette, TRUE);

			if (nChanged == 0)
			{
				return FALSE;
			}
			Invalidate();

			return TRUE;
		}
	}
	return CPropertySheet::OnQueryNewPalette();
}


void CPropSheet::OnPaletteChanged(CWnd* pFocusWnd) 
{
	if (!thePreInstall.m_bmpHeader.IsEmpty() )
	{
		CPalette* pPal = thePreInstall.m_bmpHeader.GetPalette();
		if ( (NULL != pPal) &&
			(NULL != GetSafeHwnd()) &&
			(this != pFocusWnd) &&
			!IsChild(pFocusWnd) )
		{
			CClientDC dc(this);
			CPalette* pOldPalette = dc.SelectPalette(pPal, TRUE);
			UINT nChanged = dc.RealizePalette();
			dc.SelectPalette(pOldPalette, TRUE);

			if (0 != nChanged)
			{
				Invalidate();
			}
		}
	}
	else
	{
		CPropertySheet::OnPaletteChanged(pFocusWnd);
	}
}
