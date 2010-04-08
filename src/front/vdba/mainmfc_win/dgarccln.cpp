/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dgarccln cpp : implementation file 
**    Project  : IngresII / VDBA.
**    Author   : Emmanuel Blattes
**
** History after 15-06-2001
**
** 15-Jun-2001 (uk$so01)
**    BUG #104993, Manage the push/pop help ID.
**
**/

#include "stdafx.h"
#include "mainmfc.h"
#include "DgArccln.h"
extern "C" {
#include "dba.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// interface to dom.c
extern "C" int MfcDlgArcClean(char*  pBufDate, int iLen)
{
  ASSERT (pBufDate);
  ASSERT (iLen > 0);
  memset (pBufDate, 0, iLen);

	CuDlgReplicArcclean dlg;
	int iret = dlg.DoModal();
  
  if (iret == IDOK)
    x_strncpy(pBufDate, (LPCTSTR)dlg.m_csBeforeTime, iLen);

  return iret;
}

/////////////////////////////////////////////////////////////////////////////
// CuDlgReplicArcclean dialog


CuDlgReplicArcclean::CuDlgReplicArcclean(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgReplicArcclean::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgReplicArcclean)
	m_csBeforeTime = _T("");
	//}}AFX_DATA_INIT
}


void CuDlgReplicArcclean::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgReplicArcclean)
	DDX_Control(pDX, IDC_EDIT_BEFTIME, m_edBeforeTime);
	DDX_Text(pDX, IDC_EDIT_BEFTIME, m_csBeforeTime);
	DDV_MaxChars(pDX, m_csBeforeTime, 30);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgReplicArcclean, CDialog)
	//{{AFX_MSG_MAP(CuDlgReplicArcclean)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgReplicArcclean message handlers

void CuDlgReplicArcclean::OnOK() 
{
  CString csBef;
  m_edBeforeTime.GetWindowText(csBef);
  csBef.TrimLeft();
  csBef.TrimRight();
  if (csBef.IsEmpty()) {
    MessageBox(VDBA_MfcResourceString(IDS_E_BEFORE_TIME), NULL, MB_ICONEXCLAMATION | MB_OK);//_T("Before Time not Valid")
    return;
  }
	CDialog::OnOK();
}

BOOL CuDlgReplicArcclean::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	PushHelpId(IDD_REPLICATION_ARCCLEAN);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgReplicArcclean::OnDestroy() 
{
	CDialog::OnDestroy();
	
	PopHelpId();
}
