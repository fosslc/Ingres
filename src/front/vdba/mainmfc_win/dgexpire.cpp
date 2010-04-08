/****************************************************************************
**
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**  Project  : Visual DBA
**
** dgexpire.cpp : implementation file
**
**  History after 24-Jun-2002:
**	24-Jun-2002 (hanje04)
**	    Cast all CStrings being passed to other functions or methods
**	    using %s with (LPCTSTR) and made calls more readable on UNIX.
**	27-aug-2002 (somsa01)
**	    Corrected previous cross-integration.
*/

#include "stdafx.h"
#include "mainmfc.h"
#include "dgexpire.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "dgerrh.h"     // MessageWithHistoryButton

extern "C"
{
#include "dba.h"
#include "dbaset.h"
#include "dbaginfo.h"
#include "dbadlg1.h"
};

/////////////////////////////////////////////////////////////////////////////
// CuDlgExpireDate dialog


CuDlgExpireDate::CuDlgExpireDate(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgExpireDate::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgExpireDate)
	m_csExpireDate = _T("");
	m_iRad = -1;
	//}}AFX_DATA_INIT
  m_csTableName = _T("");
  m_csOwnerName = _T("");
  m_csVirtNodeName = _T("");
  m_csParentDbName = _T("");
}


void CuDlgExpireDate::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgExpireDate)
	DDX_Control(pDX, IDOK, m_btnOk);
	DDX_Control(pDX, IDC_EDIT_EXPIREDATE, m_edExpireDate);
	DDX_Text(pDX, IDC_EDIT_EXPIREDATE, m_csExpireDate);
	DDX_Radio(pDX, IDC_RAD_EXPIRE_NO, m_iRad);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgExpireDate, CDialog)
	//{{AFX_MSG_MAP(CuDlgExpireDate)
	ON_BN_CLICKED(IDC_RAD_EXPIRE_NO, OnRadExpireNo)
	ON_BN_CLICKED(IDC_RAD_EXPIRE_YES, OnRadExpireYes)
	ON_EN_CHANGE(IDC_EDIT_EXPIREDATE, OnChangeEditExpiredate)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgExpireDate message handlers

void CuDlgExpireDate::OnRadExpireNo() 
{
  CheckRadioButton(IDC_RAD_EXPIRE_NO, IDC_RAD_EXPIRE_YES, IDC_RAD_EXPIRE_NO);
  m_edExpireDate.EnableWindow(FALSE);
  OnChangeEditExpiredate();
}


void CuDlgExpireDate::OnRadExpireYes() 
{
  CheckRadioButton(IDC_RAD_EXPIRE_NO, IDC_RAD_EXPIRE_YES, IDC_RAD_EXPIRE_YES);
  m_edExpireDate.EnableWindow();
  if (m_edExpireDate.GetWindowTextLength() == 0)
    m_edExpireDate.SetWindowText(VDBA_MfcResourceString(IDS_FORMAT_DISPLAY));//"month dd yyyy"
  OnChangeEditExpiredate();
}

void CuDlgExpireDate::OnChangeEditExpiredate() 
{
  BOOL bEnableOk = FALSE;
  if (IsDlgButtonChecked(IDC_RAD_EXPIRE_NO) || m_edExpireDate.GetWindowTextLength())
    bEnableOk = TRUE;
  m_btnOk.EnableWindow(bEnableOk);
}

void CuDlgExpireDate::OnOK() 
{
  // Check the expiration date format
  if (IsDlgButtonChecked(IDC_RAD_EXPIRE_YES)) {
    CString csFormat;
    m_edExpireDate.GetWindowText(csFormat);
    csFormat.TrimLeft();
    csFormat.TrimRight();
    CString csMonth, csDay, csYear;
    BOOL bAllFound = FALSE;

    int idx = csFormat.Find(_T(' '));
    if (idx > 0) {
      csMonth = csFormat.Left(idx);
      csFormat = csFormat.Mid(idx+1);
      csFormat.TrimLeft();
      idx = csFormat.Find(_T(' '));
      if (idx > 0) {
        csDay = csFormat.Left(idx);
        csFormat = csFormat.Mid(idx+1);
        csFormat.TrimLeft();
        idx = csFormat.Find(_T(' '));
        if (idx < 0) {
          csYear = csFormat;
          bAllFound = TRUE;
        }
      }
    }
    if (bAllFound) {
      // Potential enhancement: Check each parameter?
    }
    else {
      AfxMessageBox(VDBA_MfcResourceString(IDS_E_BAD_DATE_FORMAT));//_T("Expiration date format : \"month dd yyyy\" or \"mm dd yyyy\""
      return;   // Stay in the box
    }
  }

  UpdateData(TRUE);

  if (ChangeExpirationDate())
    CDialog::OnOK();
}

BOOL CuDlgExpireDate::OnInitDialog() 
{
  CDialog::OnInitDialog();

  ASSERT (!m_csTableName.IsEmpty());
  ASSERT (!m_csOwnerName.IsEmpty());
  ASSERT (!m_csVirtNodeName.IsEmpty());
  ASSERT (!m_csParentDbName.IsEmpty());

  CString csCaption;
  //
  // Make up title:
  LPUCHAR parent [MAXPLEVEL];
  TCHAR buffer   [MAXOBJECTNAME];
  parent [0] = (LPUCHAR)(LPTSTR)(LPCTSTR)m_csParentDbName;
  parent [1] = NULL;
  GetExactDisplayString (m_csTableName, m_csOwnerName, OT_TABLE, parent, buffer);
  AfxFormatString1(csCaption,IDS_T_EXPIRE_DATE,buffer);//"Expiration Date of %1"
  SetWindowText(csCaption);

  if (IsDlgButtonChecked(IDC_RAD_EXPIRE_NO))
    m_edExpireDate.EnableWindow(FALSE);

  PushHelpId (IDD_EXPIREDATE);

  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CuDlgExpireDate::ChangeExpirationDate()
{
  CString csSave;
  csSave.Format(_T("save %s.%s"), QuoteIfNeeded(m_csOwnerName), QuoteIfNeeded(m_csTableName));
  if (m_iRad > 0) {
    csSave += _T(" until ");
    csSave += m_csExpireDate;
  }

  // Get a session on the database
  int ilocsession;
  CString connectname;
  // Getsession accepts (local) as a node name
  connectname.Format("%s::%s",
                     (LPCTSTR)m_csVirtNodeName,
                     (LPCTSTR)m_csParentDbName);
  CWaitCursor glassHour;
  int iret = Getsession((LPUCHAR)(LPCSTR)connectname, SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
  if (iret !=RES_SUCCESS) {
    // "Cannot get a Session"
    MessageWithHistoryButton(m_hWnd, VDBA_MfcResourceString (IDS_E_GET_SESSION));
    return FALSE;
  }

  // execute the statement
  iret=ExecSQLImm((LPUCHAR)(LPCSTR)csSave, FALSE, NULL, NULL, NULL);

  // release the session
  ReleaseSession(ilocsession, iret==RES_SUCCESS ? RELEASE_COMMIT : RELEASE_ROLLBACK);
  if (iret !=RES_SUCCESS) {
    // "Cannot Change Expiration Date"
    MessageWithHistoryButton(m_hWnd, VDBA_MfcResourceString (IDS_E_CHANGE_EXPIRATION_DATE));
    return FALSE;
  }

  return TRUE;
}

void CuDlgExpireDate::OnDestroy() 
{
	CDialog::OnDestroy();
	PopHelpId();
}
