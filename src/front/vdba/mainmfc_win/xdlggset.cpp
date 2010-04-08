/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xdlggset.cpp Implementation File 
**    Project  : INGRES II / VDBA 
**    Author   : UK Sotheavut
**    Purpose  : Dialog for General Display Settings 
**
** Histoty:
** xx-Nov-1998 (uk$so01)
**    Created.
** 04-Fev-2002 (uk$so01)
**    SIR #106648, Integrate (ipm && sqlquery).ocx.
**    Save properties ad default.
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "xdlggset.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CxDlgGeneralDisplaySetting dialog


CxDlgGeneralDisplaySetting::CxDlgGeneralDisplaySetting(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgGeneralDisplaySetting::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgGeneralDisplaySetting)
	m_bUseGrid = TRUE;
	m_strDelayValue = _T("2");
	m_bMaximizeDocs = FALSE;
	//}}AFX_DATA_INIT

	CaGeneralDisplaySetting* pGeneralDisplaySetting = theApp.GetGeneralDisplaySetting();
	if (pGeneralDisplaySetting)
	{
		m_strDelayValue.Format (_T("%ld"), pGeneralDisplaySetting->m_nDlgWaitDelayExecution);
		m_bUseGrid = pGeneralDisplaySetting->m_bListCtrlGrid;
		m_bMaximizeDocs = pGeneralDisplaySetting->m_bMaximizeDocs;
	}
}


void CxDlgGeneralDisplaySetting::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgGeneralDisplaySetting)
	DDX_Control(pDX, IDC_MFC_EDIT1, m_cEditDelayValue);
	DDX_Control(pDX, IDC_MFC_SPIN1, m_cSpinDelayValue);
	DDX_Check(pDX, IDC_MFC_CHECK1, m_bUseGrid);
	DDX_Text(pDX, IDC_MFC_EDIT1, m_strDelayValue);
	DDX_Check(pDX, IDC_MFC_MAXDOCS, m_bMaximizeDocs);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgGeneralDisplaySetting, CDialog)
	//{{AFX_MSG_MAP(CxDlgGeneralDisplaySetting)
	ON_EN_KILLFOCUS(IDC_MFC_EDIT1, OnKillfocusEditDelayValue)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgGeneralDisplaySetting message handlers

void CxDlgGeneralDisplaySetting::OnOK() 
{
	CDialog::OnOK();
	CaGeneralDisplaySetting* pGeneralDisplaySetting = theApp.GetGeneralDisplaySetting();
	if (pGeneralDisplaySetting)
	{
		pGeneralDisplaySetting->m_nDlgWaitDelayExecution = atol (m_strDelayValue);	
		pGeneralDisplaySetting->m_bListCtrlGrid = m_bUseGrid;
		pGeneralDisplaySetting->m_bMaximizeDocs = m_bMaximizeDocs;

		pGeneralDisplaySetting->NotifyChanges();
		if (theApp.IsSavePropertyAsDefault())
			pGeneralDisplaySetting->Save();
	}
}

void CxDlgGeneralDisplaySetting::OnKillfocusEditDelayValue() 
{
	CString strDelayValue;

	m_cEditDelayValue.GetWindowText (strDelayValue);
	strDelayValue.TrimLeft();
	strDelayValue.TrimRight();
	if (strDelayValue.IsEmpty())
	{
		strDelayValue.Format (_T("%ld"), 0);
		m_strDelayValue = strDelayValue;
		UpdateData (FALSE);
	}
}

BOOL CxDlgGeneralDisplaySetting::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_cSpinDelayValue.SetRange (0, UD_MAXVAL);
	PushHelpId (IDD_XGENERAL_DISPLAY_SETTING);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL VDBA_DlgGeneralDisplaySetting()
{
	CxDlgGeneralDisplaySetting dlg;
	return (dlg.DoModal() == IDOK)? TRUE: FALSE;
}

void CxDlgGeneralDisplaySetting::OnDestroy() 
{
	PopHelpId();
	CDialog::OnDestroy();
}
