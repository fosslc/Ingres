/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : sbdbebar.cpp, Implementation file 
**    Project  : CA-OpenIngres/Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
** 
*    Purpose  : DBEvent Dialog Bar. 
**              When the control bar is floating, its size is the vertical dialog bar template.
**
** HISTORY:
** xx-Feb-1997 (uk$so01)
**    created.
** 21-Fev-2002 (uk$so01)
**    SIR #106648, Integrate ipm.ocx.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "sbdbebar.h"
#include "bmpbtn.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuDbeDlgBar::CuDbeDlgBar()
{
	UINT nIdIcon[2] = {IDI_REFRESH, IDI_RESET};
	m_HdlgSize.cx = m_HdlgSize.cy = 0;
	HINSTANCE hInst = AfxGetInstanceHandle();
	for (int i=0; i<2; i++)
	{
		m_arrayIcon[i] = (HICON)::LoadImage(hInst, MAKEINTRESOURCE(nIdIcon[i]), IMAGE_ICON, 16, 16, LR_SHARED);
	}
	m_Button01Position.x = m_Button01Position.y = 0;
}

CuDbeDlgBar::~CuDbeDlgBar()
{
}


BOOL CuDbeDlgBar::PreCreateWindow(CREATESTRUCT& cs) 
{
	cs.style |= WS_CLIPCHILDREN;
	cs.style |= WS_THICKFRAME;
	return CDialogBar::PreCreateWindow(cs);
}


BOOL CuDbeDlgBar::Create (CWnd* pParent, UINT nIDTemplate, UINT nStyle, UINT nID, BOOL bChange)
{
	if (!CDialogBar::Create (pParent, nIDTemplate, nStyle, nID))
		return FALSE;
	ASSERT (pParent);
	m_pParent = pParent;
	OnInitDialog();
	return TRUE;
}

BOOL CuDbeDlgBar::Create (CWnd* pParent, LPCTSTR lpszTemplateName, UINT nStyle, UINT nID, BOOL bChange)
{
	if (!CDialogBar::Create (pParent, lpszTemplateName, nStyle, nID))
		return FALSE;
	ASSERT (pParent);
	m_pParent = pParent;
	OnInitDialog();
	return TRUE;
}

CSize CuDbeDlgBar::CalcDynamicLayout (int nLength, DWORD dwMode)
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

void CuDbeDlgBar::DoDataExchange(CDataExchange* pDX)
{
	CDialogBar::DoDataExchange(pDX);
	DDX_Text(pDX, IDM_DBEVENT_TRACE_MAXLINE, m_strMaxLine);
	DDV_MaxChars(pDX, m_strMaxLine, 4);
}


BEGIN_MESSAGE_MAP(CuDbeDlgBar, CDialogBar)
	ON_WM_SIZE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()


BOOL CuDbeDlgBar::OnInitDialog() 
{
	CRect r, r2;
	VERIFY(m_Button01.SubclassDlgItem   (IDM_DBEVENT_TRACE_REFRESH,  this));
	VERIFY(m_Button02.SubclassDlgItem   (IDM_DBEVENT_TRACE_RESET, this));
	VERIFY(m_ComboDB.SubclassDlgItem    (IDM_DBEVENT_TRACE_DATABASE, this));
	VERIFY(m_Check1.SubclassDlgItem     (IDM_DBEVENT_TRACE_POPUP, this));
	VERIFY(m_Check2.SubclassDlgItem     (IDM_DBEVENT_TRACE_SYSDBEVENT, this));
	VERIFY(m_Check3.SubclassDlgItem     (IDM_DBEVENT_TRACE_CLEARFIRST, this));
	VERIFY(m_MaxLineText.SubclassDlgItem(IDM_DBEVENT_TRACE_MAXLINETEXT, this));
	VERIFY(m_MaxLine.SubclassDlgItem    (IDM_DBEVENT_TRACE_MAXLINE, this));
	VERIFY(m_Spin1.SubclassDlgItem      (IDC_MFC_SPIN1, this));
	CButton* btnArray [2] = {&m_Button01, &m_Button02};
	CSize buttonSize = GetToolBarButtonSize(this, IDR_MAINFRAME);
	UINT nFlags = SWP_NOMOVE|SWP_NOZORDER|SWP_NOREDRAW|SWP_NOACTIVATE;
	for (int i=0; i<2; i++)
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

	CuDbeBarDlgTemplate* pDlg = new CuDbeBarDlgTemplate(this);
	pDlg->Create (IDD_DBEVENTBARV, this);
	pDlg->GetWindowRect (r);
	m_VdlgSize.cx = r.Width();
	m_VdlgSize.cy = r.Height();

	//
	// Retrive the controls placement when the Dialog bar is Vertical docked
	// or floating.
	CWnd* pCtrl = pDlg->GetDlgItem (IDM_DBEVENT_TRACE_REFRESH);
	pCtrl->GetWindowRect  (ctrlVRect [0]);
	pCtrl->ScreenToClient (ctrlVRect [0]);

	pCtrl = pDlg->GetDlgItem (IDM_DBEVENT_TRACE_RESET);
	pCtrl->GetWindowRect (ctrlVRect [1]);
	pDlg->ScreenToClient (ctrlVRect [1]);
	pCtrl = pDlg->GetDlgItem (IDM_DBEVENT_TRACE_DATABASE);
	pCtrl->GetWindowRect (ctrlVRect [2]);
	pDlg->ScreenToClient (ctrlVRect [2]);
	pCtrl = pDlg->GetDlgItem (IDM_DBEVENT_TRACE_POPUP);
	pCtrl->GetWindowRect (ctrlVRect [3]);
	pDlg->ScreenToClient (ctrlVRect [3]);
	pCtrl = pDlg->GetDlgItem (IDM_DBEVENT_TRACE_MAXLINETEXT);
	pCtrl->GetWindowRect (ctrlVRect [4]);
	pDlg->ScreenToClient (ctrlVRect [4]);
	pCtrl = pDlg->GetDlgItem (IDM_DBEVENT_TRACE_MAXLINE);
	pCtrl->GetWindowRect (ctrlVRect [5]);
	pDlg->ScreenToClient (ctrlVRect [5]);
	pCtrl = pDlg->GetDlgItem (IDC_MFC_SPIN1);
	pCtrl->GetWindowRect (ctrlVRect [6]);
	pDlg->ScreenToClient (ctrlVRect [6]);
	pCtrl = pDlg->GetDlgItem (IDM_DBEVENT_TRACE_SYSDBEVENT);
	pCtrl->GetWindowRect (ctrlVRect [7]);
	pDlg->ScreenToClient (ctrlVRect [7]);
	pCtrl = pDlg->GetDlgItem (IDM_DBEVENT_TRACE_CLEARFIRST);
	pCtrl->GetWindowRect (ctrlVRect [8]);
	pDlg->ScreenToClient (ctrlVRect [8]);
	pDlg->DestroyWindow();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDbeDlgBar::OnSize(UINT nType, int cx, int cy) 
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
		m_ComboDB.GetWindowRect (ctrlHRect [2]);
		ScreenToClient (ctrlHRect [2]);
		m_Check1.GetWindowRect (ctrlHRect [3]);
		ScreenToClient (ctrlHRect [3]);
		m_MaxLineText.GetWindowRect (ctrlHRect [4]);
		ScreenToClient (ctrlHRect [4]);
		m_MaxLine.GetWindowRect (ctrlHRect [5]);
		ScreenToClient (ctrlHRect [5]);
		m_Spin1.GetWindowRect (ctrlHRect [6]);
		ScreenToClient (ctrlHRect [6]);
		m_Check2.GetWindowRect (ctrlHRect [7]);
		ScreenToClient (ctrlHRect [7]);
		m_Check3.GetWindowRect (ctrlHRect [8]);
		ScreenToClient (ctrlHRect [8]);
	}

	//
	// Size the 2 buttons and replace then at the appropriate place.
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
		m_ComboDB.MoveWindow (ctrlVRect [2]);
		m_Check1.MoveWindow (ctrlVRect [3]);
		m_MaxLineText.MoveWindow (ctrlVRect [4]);
		m_MaxLine.MoveWindow (ctrlVRect [5]);
		m_Spin1.MoveWindow (ctrlVRect [6]);
		m_Check2.MoveWindow (ctrlVRect [7]);
		m_Check3.MoveWindow (ctrlVRect [8]);
	}
	else
	{
		m_ComboDB.MoveWindow (ctrlHRect [2]);
		m_Check1.MoveWindow (ctrlHRect [3]);
		m_MaxLineText.MoveWindow (ctrlHRect [4]);
		m_MaxLine.MoveWindow (ctrlHRect [5]);
		m_Spin1.MoveWindow (ctrlHRect [6]);
		m_Check2.MoveWindow (ctrlHRect [7]);
		m_Check3.MoveWindow (ctrlHRect [8]);
	}
}

void CuDbeDlgBar::UpdateDisplay()
{
	CRect r;
	GetClientRect (r);
	OnSize (SIZE_RESTORED, r.Width(), r.Height());
}



void CuDbeDlgBar::OnDestroy()
{
	for (int i=0; i<2; i++)
	{
		DestroyIcon(m_arrayIcon[i]);
	}
	CDialogBar::OnDestroy();
}
/////////////////////////////////////////////////////////////////////////////
// CuDbeBarDlgTemplate dialog


CuDbeBarDlgTemplate::CuDbeBarDlgTemplate(CWnd* pParent /*=NULL*/)
    : CDialog(CuDbeBarDlgTemplate::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDbeBarDlgTemplate)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDbeBarDlgTemplate::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDbeBarDlgTemplate)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDbeBarDlgTemplate, CDialog)
	//{{AFX_MSG_MAP(CuDbeBarDlgTemplate)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDbeBarDlgTemplate message handlers

void CuDbeBarDlgTemplate::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}
