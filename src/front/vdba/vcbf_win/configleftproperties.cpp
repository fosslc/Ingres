/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**
**    Source   : ConfigLeftProperites.cpp, implementation file
**
** History:
**	22-nov-98 (cucjo01)
**             Added focus on count option for properties page
**             Added maximum text limit of text fields
** 	       Added Extra validation for text fields
** 06-Jun-2000: (uk$so01) 
**    (BUG #99242)
**    Cleanup code for DBCS compliant
** 17-Dec-2003 (schph01)
**    SIR #111462, Put string into resource files
**
**/

#include "stdafx.h"
#include "vcbf.h"
#include "ConfigLeftProperties.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConfigLeftProperties dialog


CConfigLeftProperties::CConfigLeftProperties(CWnd* pParent /*=NULL*/)
	: CDialog(CConfigLeftProperties::IDD, pParent)
{
	//{{AFX_DATA_INIT(CConfigLeftProperties)
	m_Component = _T("");
	m_Name = _T("");
	m_StartupCount = _T("");
	//}}AFX_DATA_INIT
}


void CConfigLeftProperties::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConfigLeftProperties)
	DDX_Text(pDX, IDC_COMPONENT, m_Component);
	DDX_Text(pDX, IDC_NAME, m_Name);
	DDX_Text(pDX, IDC_STARTUP_COUNT, m_StartupCount);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConfigLeftProperties, CDialog)
	//{{AFX_MSG_MAP(CConfigLeftProperties)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfigLeftProperties message handlers

BOOL CConfigLeftProperties::OnInitDialog() 
{ CEdit* ctlEdit;
  CString strTitle;
  
  CDialog::OnInitDialog();
  strTitle.Format(IDS_TITLE_PROPERTIES, m_ItemData->m_componentInfo.sztype);
  SetWindowText(strTitle);

  ctlEdit = (CEdit*)GetDlgItem (IDC_COMPONENT);
  if (ctlEdit)
  { ctlEdit->SetWindowText((LPCTSTR)m_ItemData->m_componentInfo.sztype);
    ctlEdit->EnableWindow(FALSE);
  }

  ctlEdit = (CEdit*)GetDlgItem (IDC_NAME);
  if (ctlEdit)
  { ctlEdit->SetWindowText((LPCTSTR)m_ItemData->m_componentInfo.szname);
    if (!VCBFllCanNameBeEdited (&m_ItemData->m_componentInfo))
      ctlEdit->EnableWindow(FALSE);

    ctlEdit->SetLimitText(79);  /// Use constant

  }
  
  ctlEdit = (CEdit*)GetDlgItem (IDC_STARTUP_COUNT);
  if (ctlEdit)
  { ctlEdit->SetWindowText((LPCTSTR)m_ItemData->m_componentInfo.szcount);
    if (!VCBFllCanCountBeEdited (&m_ItemData->m_componentInfo))
      ctlEdit->EnableWindow(FALSE);
  
    ctlEdit->SetLimitText(10);

    if (m_bFocusCount)
    { ctlEdit->SetFocus();
      ctlEdit->SetSel(0, -1);
      return FALSE;
    }
  }
  
  return TRUE;  // return TRUE unless you set the focus to a control
}

void CConfigLeftProperties::OnOK() 
{ CEdit* ctlEdit;
  CString strText;

  ctlEdit = (CEdit*)GetDlgItem (IDC_NAME);
  if (ctlEdit)
  { ctlEdit->GetWindowText(strText);
    if (strText.IsEmpty())
    { MessageBeep(MB_ICONEXCLAMATION);
      AfxMessageBox(IDS_ENTER_NAME);
      ctlEdit->SetFocus();
      ctlEdit->SetSel(0, -1);
      return;
    }
  }

  ctlEdit = (CEdit*)GetDlgItem (IDC_STARTUP_COUNT);
  if (ctlEdit)
  { ctlEdit->GetWindowText(strText);
    if (strText.IsEmpty())
    { MessageBeep(MB_ICONEXCLAMATION);
      AfxMessageBox(IDS_ENTER_STARTUP_COUNT);
      ctlEdit->SetFocus();
      ctlEdit->SetSel(0, -1);
      return;
    }
  }

  CDialog::OnOK();
}
