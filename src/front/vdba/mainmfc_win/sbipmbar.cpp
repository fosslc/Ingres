/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sbipmbar.cpp, Implementation file 
**    Project  : CA-OpenIngres/Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
** 
*    Purpose  : IPM Dialog Bar. When the control bar is floating, 
**              its size is the vertical dialog bar template.
**
** HISTORY:
** xx-Feb-1997 (uk$so01)
**    created.
** 24-Jan-2000 (uk$so01)
**    SIR #100103, Remove the IPM checkbox "only if window is uppermost" 
**    when running through the context driven mode.
** 21-Fev-2002 (uk$so01)
**    SIR #106648, Integrate ipm.ocx.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "sbipmbar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuIpmDlgBar::CuIpmDlgBar()
{
	m_bContextDriven = FALSE;
	UINT nIdIcon[6] = {IDI_REFRESH, IDI_KILLSESSION, IDI_EXPANDBRANCH, IDI_EXPANDALL, IDI_COLLAPSEBRANCH, IDI_COLLAPSEALL};
	m_HdlgSize.cx = m_HdlgSize.cy = 0;
	HINSTANCE hInst = AfxGetInstanceHandle();
	for (int i=0; i<6; i++)
	{
		m_arrayIcon[i] = (HICON)::LoadImage(hInst, MAKEINTRESOURCE(nIdIcon[i]), IMAGE_ICON, 16, 16, LR_SHARED);
	}
	m_Button01Position.x = m_Button01Position.y = 0;
}

CuIpmDlgBar::~CuIpmDlgBar()
{
}


BOOL CuIpmDlgBar::PreCreateWindow(CREATESTRUCT& cs) 
{
	cs.style |= WS_CLIPCHILDREN;
	cs.style |= WS_THICKFRAME;
	return CDialogBar::PreCreateWindow(cs);
}


BOOL CuIpmDlgBar::Create (CWnd* pParent, UINT nIDTemplate, UINT nStyle, UINT nID, BOOL bChange)
{
	if (!CDialogBar::Create (pParent, nIDTemplate, nStyle, nID))
		return FALSE;
	ASSERT (pParent);
	m_pParent = pParent;
	OnInitDialog();
	return TRUE;
}

BOOL CuIpmDlgBar::Create (CWnd* pParent, LPCTSTR lpszTemplateName, UINT nStyle, UINT nID, BOOL bChange)
{
	if (!CDialogBar::Create (pParent, lpszTemplateName, nStyle, nID))
		return FALSE;
	ASSERT (pParent);
	m_pParent = pParent;
	OnInitDialog();
	return TRUE;
}

CSize CuIpmDlgBar::CalcDynamicLayout (int nLength, DWORD dwMode)
{
	if (m_HdlgSize.cx == 0 && m_HdlgSize.cy == 0)
		return CDialogBar::CalcDynamicLayout (nLength, dwMode);

	if ((dwMode & LM_VERTDOCK) || (dwMode & LM_HORZDOCK))
	{
		if (dwMode & LM_STRETCH)
			return (dwMode & LM_VERTDOCK)? m_VdlgSize: m_HdlgSize;
		else
		{
			CRect r;
			m_pParent->GetWindowRect (r);
			return (dwMode & LM_VERTDOCK)? CSize (m_VdlgSize.cx, r.Height()): m_HdlgSize;
		}
	}
	return m_VdlgSize;
}

BEGIN_MESSAGE_MAP(CuIpmDlgBar, CDialogBar)
	ON_WM_SIZE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()


BOOL CuIpmDlgBar::OnInitDialog() 
{
	CRect r, r2;
	VERIFY(m_Button01.SubclassDlgItem (IDM_IPMBAR_REFRESH,  this));
	VERIFY(m_Button02.SubclassDlgItem (IDM_IPMBAR_SHUTDOWN, this));
	VERIFY(m_Check1.SubclassDlgItem (IDM_IPMBAR_NULLRESOURCE, this));
	VERIFY(m_Check2.SubclassDlgItem (IDM_IPMBAR_INTERNALSESSION, this));
	VERIFY(m_Check3.SubclassDlgItem (IDM_IPMBAR_SYSLOCKLISTS, this));
	VERIFY(m_Check4.SubclassDlgItem (IDM_IPMBAR_INACTIVETRANS, this));
	VERIFY(m_ComboRCType.SubclassDlgItem (IDM_IPMBAR_ALLRESOURCES, this));

	VERIFY(m_Button03.SubclassDlgItem (ID_BUTTON_CLASSB_EXPANDBRANCH,   this));
	VERIFY(m_Button04.SubclassDlgItem (ID_BUTTON_CLASSB_EXPANDALL,      this));
	VERIFY(m_Button05.SubclassDlgItem (ID_BUTTON_CLASSB_COLLAPSEBRANCH, this));
	VERIFY(m_Button06.SubclassDlgItem (ID_BUTTON_CLASSB_COLLAPSEALL,    this));

	VERIFY(m_Check5.SubclassDlgItem (IDM_IPMBAR_EXPRESS_REFRESH,       this));
	VERIFY(m_cEditFrequency.SubclassDlgItem (IDM_IPMBAR_REFRESH_FREQUENCY, this));
	VERIFY(m_Check6.SubclassDlgItem (IDM_IPMBAR_EXPRESS_REFRESH_ONLYIF, this));
	m_cEditFrequency.EnableWindow (FALSE);

	CuBitmapButton* btnArray [6] = {&m_Button01, &m_Button02, &m_Button03, &m_Button04, &m_Button05, &m_Button06};
	CSize buttonSize = GetToolBarButtonSize(this, IDR_MAINFRAME);
	UINT nFlags = SWP_NOMOVE|SWP_NOZORDER|SWP_NOREDRAW|SWP_NOACTIVATE;
	for (int i=0; i<6; i++)
	{
		btnArray[i]->SetIcon(m_arrayIcon[i]);
		btnArray[i]->SetWindowPos(NULL, -1, -1, buttonSize.cx, buttonSize.cy, nFlags);
	}

	m_Button01.GetWindowRect (r);
	ScreenToClient (r);
	m_Button01Position.x = r.left;
	m_Button01Position.y = r.top;
	GetWindowRect (r);
	m_HdlgSize.cx = r.Width();
	m_HdlgSize.cy = r.Height();

	CuIpmBarDlgTemplate* pDlg = new CuIpmBarDlgTemplate(this);
	pDlg->Create (IDD_IPMBARV, this);
	pDlg->GetWindowRect (r);
	m_VdlgSize.cx = r.Width();
	m_VdlgSize.cy = r.Height();

	//
	// Retrieve the controls placement when the Dialog bar is Vertical docked
	// or floating.
	CWnd* pCtrl = pDlg->GetDlgItem (IDM_IPMBAR_REFRESH);
	pCtrl->GetWindowRect  (ctrlVRect [0]);
	pCtrl->ScreenToClient (ctrlVRect [0]);

	pCtrl = pDlg->GetDlgItem (IDM_IPMBAR_SHUTDOWN);
	pCtrl->GetWindowRect (ctrlVRect [1]);
	pDlg->ScreenToClient (ctrlVRect [1]);
	pCtrl = pDlg->GetDlgItem (IDM_IPMBAR_ALLRESOURCES);
	pCtrl->GetWindowRect (ctrlVRect [2]);
	pDlg->ScreenToClient (ctrlVRect [2]);
	pCtrl = pDlg->GetDlgItem (IDM_IPMBAR_NULLRESOURCE);
	pCtrl->GetWindowRect (ctrlVRect [3]);
	pDlg->ScreenToClient (ctrlVRect [3]);
	pCtrl = pDlg->GetDlgItem (IDM_IPMBAR_INTERNALSESSION);
	pCtrl->GetWindowRect (ctrlVRect [4]);
	pDlg->ScreenToClient (ctrlVRect [4]);
	pCtrl = pDlg->GetDlgItem (IDM_IPMBAR_SYSLOCKLISTS);
	pCtrl->GetWindowRect (ctrlVRect [5]);
	pDlg->ScreenToClient (ctrlVRect [5]);
	pCtrl = pDlg->GetDlgItem (IDM_IPMBAR_INACTIVETRANS);
	pCtrl->GetWindowRect (ctrlVRect [6]);
	pDlg->ScreenToClient (ctrlVRect [6]);

	pCtrl = pDlg->GetDlgItem (ID_BUTTON_CLASSB_EXPANDBRANCH);
	pCtrl->GetWindowRect (ctrlVRect [7]);
	pDlg->ScreenToClient (ctrlVRect [7]);
	pCtrl = pDlg->GetDlgItem (ID_BUTTON_CLASSB_EXPANDALL);
	pCtrl->GetWindowRect (ctrlVRect [8]);
	pDlg->ScreenToClient (ctrlVRect [8]);
	pCtrl = pDlg->GetDlgItem (ID_BUTTON_CLASSB_COLLAPSEBRANCH);
	pCtrl->GetWindowRect (ctrlVRect [9]);
	pDlg->ScreenToClient (ctrlVRect [9]);
	pCtrl = pDlg->GetDlgItem (ID_BUTTON_CLASSB_COLLAPSEALL);
	pCtrl->GetWindowRect (ctrlVRect [10]);
	pDlg->ScreenToClient (ctrlVRect [10]);

	pCtrl = pDlg->GetDlgItem (IDM_IPMBAR_EXPRESS_REFRESH);
	pCtrl->GetWindowRect (ctrlVRect [11]);
	pDlg->ScreenToClient (ctrlVRect [11]);
	pCtrl = pDlg->GetDlgItem (IDM_IPMBAR_REFRESH_FREQUENCY);
	pCtrl->GetWindowRect (ctrlVRect [12]);
	pDlg->ScreenToClient (ctrlVRect [12]);
	pCtrl = pDlg->GetDlgItem (IDC_MFC_STATIC1);
	pCtrl->GetWindowRect (ctrlVRect [13]);
	pDlg->ScreenToClient (ctrlVRect [13]);
	pCtrl = pDlg->GetDlgItem (IDM_IPMBAR_EXPRESS_REFRESH_ONLYIF);
	pCtrl->GetWindowRect (ctrlVRect [14]);
	pDlg->ScreenToClient (ctrlVRect [14]);

	pDlg->DestroyWindow();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuIpmDlgBar::OnSize(UINT nType, int cx, int cy) 
{
	int x, y, i;
	CDialogBar::OnSize (nType, cx, cy);
	if (!IsWindow (m_Button01.m_hWnd))
		return;
	CRect r, rb1, rDlg;
	CButton* tab [] = 
	{
		&m_Button01, 
		&m_Button02 
	};

	if (m_HdlgSize.cx == 0 && m_HdlgSize.cy == 0)
	{
		GetWindowRect (r);
		m_HdlgSize.cx = r.Width();
		m_HdlgSize.cy = r.Height();
		m_Button01.GetWindowRect (ctrlHRect [0]);
		ScreenToClient (ctrlHRect [0]);
		m_Button02.GetWindowRect (ctrlHRect [1]);
		ScreenToClient (ctrlHRect [1]);
		m_ComboRCType.GetWindowRect (ctrlHRect [2]);
		ScreenToClient (ctrlHRect [2]);
		m_Check1.GetWindowRect (ctrlHRect [3]);
		ScreenToClient (ctrlHRect [3]);
		m_Check2.GetWindowRect (ctrlHRect [4]);
		ScreenToClient (ctrlHRect [4]);
		m_Check3.GetWindowRect (ctrlHRect [5]);
		ScreenToClient (ctrlHRect [5]);
		m_Check4.GetWindowRect (ctrlHRect [6]);
		ScreenToClient (ctrlHRect [6]);

		m_Button03.GetWindowRect (ctrlHRect [7]);
		ScreenToClient (ctrlHRect [7]);
		m_Button04.GetWindowRect (ctrlHRect [8]);
		ScreenToClient (ctrlHRect [8]);
		m_Button05.GetWindowRect (ctrlHRect [9]);
		ScreenToClient (ctrlHRect [9]);
		m_Button06.GetWindowRect (ctrlHRect [10]);
		ScreenToClient (ctrlHRect [10]);

		m_Check5.GetWindowRect (ctrlHRect [11]);
		ScreenToClient (ctrlHRect [11]);
		m_cEditFrequency.GetWindowRect (ctrlHRect [12]);
		ScreenToClient (ctrlHRect [12]);
		GetDlgItem(IDC_MFC_STATIC1)->GetWindowRect (ctrlHRect [13]);
		ScreenToClient (ctrlHRect [13]);
		m_Check6.GetWindowRect (ctrlHRect [14]);
		ScreenToClient (ctrlHRect [14]);
	}

	//
	// Size the buttons and replace then at the appropriate place.
	m_Button01.GetClientRect (rb1);
	x = m_Button01Position.x;
	y = m_Button01Position.y;
	GetClientRect (rDlg);
	for (i=1; i<2; i++)
	{
		if ((x + 2*rb1.Width()) > (rDlg.Width()-m_Button01Position.x))
		{
			x  = m_Button01Position.x;
			y += rb1.Height();
		}
		else
		{
			x += rb1.Width();
		}
		tab [i]->SetWindowPos (NULL, x, y, 0, 0, SWP_NOSIZE);
	}
	tab [1]->Invalidate();
	if (rDlg.Width() != m_HdlgSize.cx)
	{
		m_ComboRCType.MoveWindow (ctrlVRect [2]);
		m_Check1.MoveWindow (ctrlVRect [3]);
		m_Check2.MoveWindow (ctrlVRect [4]);
		m_Check3.MoveWindow (ctrlVRect [5]);
		m_Check4.MoveWindow (ctrlVRect [6]);

		m_Button03.MoveWindow (ctrlVRect [7]);
		m_Button04.MoveWindow (ctrlVRect [8]);
		m_Button05.MoveWindow (ctrlVRect [9]);
		m_Button06.MoveWindow (ctrlVRect [10]);

		m_Check5.MoveWindow (ctrlVRect [11]);
		m_cEditFrequency.MoveWindow (ctrlVRect [12]);
		GetDlgItem(IDC_MFC_STATIC1)->MoveWindow (ctrlVRect [13]);
		m_Check6.MoveWindow (ctrlVRect [14]);
	}
	else
	{
		m_ComboRCType.MoveWindow (ctrlHRect [2]);
		m_Check1.MoveWindow (ctrlHRect [3]);
		m_Check2.MoveWindow (ctrlHRect [4]);
		m_Check3.MoveWindow (ctrlHRect [5]);
		m_Check4.MoveWindow (ctrlHRect [6]);

		m_Button03.MoveWindow (ctrlHRect [7]);
		m_Button04.MoveWindow (ctrlHRect [8]);
		m_Button05.MoveWindow (ctrlHRect [9]);
		m_Button06.MoveWindow (ctrlHRect [10]);

		m_Check5.MoveWindow (ctrlHRect [11]);
		m_cEditFrequency.MoveWindow (ctrlHRect [12]);
		GetDlgItem(IDC_MFC_STATIC1)->MoveWindow (ctrlHRect [13]);
		m_Check6.MoveWindow (ctrlHRect [14]);
	}

	//
	// The application is running in the context driven mode:
	if (m_bContextDriven)
	{
		m_Check6.ShowWindow (SW_HIDE);
	}
}

void CuIpmDlgBar::UpdateDisplay()
{
	CRect r;
	GetClientRect (r);
	OnSize (SIZE_RESTORED, r.Width(), r.Height());
}



void CuIpmDlgBar::OnDestroy()
{
	for (int i=0; i<6; i++)
	{
		DestroyIcon(m_arrayIcon[i]);
	}

	CDialogBar::OnDestroy();
}
/////////////////////////////////////////////////////////////////////////////
// CuIpmBarDlgTemplate dialog


CuIpmBarDlgTemplate::CuIpmBarDlgTemplate(CWnd* pParent /*=NULL*/)
    : CDialog(CuIpmBarDlgTemplate::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuIpmBarDlgTemplate)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuIpmBarDlgTemplate::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuIpmBarDlgTemplate)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuIpmBarDlgTemplate, CDialog)
	//{{AFX_MSG_MAP(CuIpmBarDlgTemplate)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuIpmBarDlgTemplate message handlers

void CuIpmBarDlgTemplate::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

