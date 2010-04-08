/****************************************************************************
**
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**  Project  : Visual DBA
**
** dgtbjnl.cpp : implementation file
**
** History after 24-Jun-2002:
**    24-Jun-2002 (hanje04)
**      Cast all CStrings being passed to other functions or methods using %s
**      with (LPCTSTR) and made calls more readable on UNIX.
*/

#include "stdafx.h"
#include "mainmfc.h"
#include "dgtbjnl.h"

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
// CuDlgTableJournaling dialog


CuDlgTableJournaling::CuDlgTableJournaling(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgTableJournaling::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgTableJournaling)
	//}}AFX_DATA_INIT
  m_csTableName = _T("");
  m_csOwnerName = _T("");
  m_csVirtNodeName = _T("");
  m_csParentDbName = _T("");
}


void CuDlgTableJournaling::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgTableJournaling)
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgTableJournaling, CDialog)
	//{{AFX_MSG_MAP(CuDlgTableJournaling)
	ON_BN_CLICKED(IDC_RAD_JNL_ACTIVE, OnRadJnlActive)
	ON_BN_CLICKED(IDC_RAD_JNL_AFTERCKPDB, OnRadJnlAfterckpdb)
	ON_BN_CLICKED(IDC_RAD_JNL_INACTIVE, OnRadJnlInactive)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgTableJournaling message handlers


BOOL CuDlgTableJournaling::OnInitDialog() 
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
  csCaption.Format(IDS_TABLE_CHANGEJOURNALING_CAPTION, buffer);

  SetWindowText(csCaption);

  int idcRad = -1;
  switch (m_cJournaling) {
    case _T('Y'):
      idcRad = IDC_RAD_JNL_ACTIVE;
      break;
    case _T('C'):
      idcRad = IDC_RAD_JNL_AFTERCKPDB;
      break;
    case _T('N'):
      idcRad = IDC_RAD_JNL_INACTIVE;
      break;
    default:
      ASSERT (FALSE);
      idcRad = -1;
  }
  CheckRadioButton(IDC_RAD_JNL_ACTIVE, IDC_RAD_JNL_AFTERCKPDB, idcRad);

  GetDlgItem(IDC_RAD_JNL_ACTIVE)->EnableWindow(FALSE);
  if (m_cJournaling == _T('N'))
    GetDlgItem(IDC_RAD_JNL_INACTIVE)->EnableWindow(FALSE);
  else
    GetDlgItem(IDC_RAD_JNL_AFTERCKPDB)->EnableWindow(FALSE);

  GetDlgItem(IDOK)->EnableWindow(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CuDlgTableJournaling::ChangeTableJournaling(BOOL bActivate)
{
  CString csJournal;
  CString csWhat;
  if (bActivate)
    csWhat = _T("journaling");
  else
    csWhat = _T("nojournaling");
  csJournal.Format(_T("set %s on %s.%s"), (LPCTSTR)csWhat, QuoteIfNeeded(m_csOwnerName), QuoteIfNeeded(m_csTableName));

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
    MessageWithHistoryButton(m_hWnd, VDBA_MfcResourceString (IDS_E_GET_SESSION));//_T("Cannot get a Session")
    return FALSE;
  }

  // execute the statement
  iret=ExecSQLImm((LPUCHAR)(LPCSTR)csJournal, FALSE, NULL, NULL, NULL);

  // release the session
  ReleaseSession(ilocsession, iret==RES_SUCCESS ? RELEASE_COMMIT : RELEASE_ROLLBACK);
  if (iret !=RES_SUCCESS) {
    MessageWithHistoryButton(m_hWnd, VDBA_MfcResourceString (IDS_E_CHANGE_JOURNALING));//_T("Cannot Change Journaling")
    return FALSE;
  }

  return TRUE;
}

void CuDlgTableJournaling::OnOK() 
{
  BOOL bActivate;

  // Check selection has been changed
  switch (m_cJournaling) {
    case _T('Y'):
      ASSERT (!IsDlgButtonChecked(IDC_RAD_JNL_ACTIVE));
      if (IsDlgButtonChecked(IDC_RAD_JNL_ACTIVE))
        return;
      break;
    case _T('C'):
      ASSERT (!IsDlgButtonChecked(IDC_RAD_JNL_AFTERCKPDB));
      if (IsDlgButtonChecked(IDC_RAD_JNL_AFTERCKPDB))
        return;
      break;
    case _T('N'):
      ASSERT (!IsDlgButtonChecked(IDC_RAD_JNL_INACTIVE));
      if (IsDlgButtonChecked(IDC_RAD_JNL_INACTIVE))
        return;
      break;
  }

  ASSERT (IsDlgButtonChecked(IDC_RAD_JNL_AFTERCKPDB) || IsDlgButtonChecked(IDC_RAD_JNL_INACTIVE));
  if (IsDlgButtonChecked(IDC_RAD_JNL_AFTERCKPDB))
    bActivate = TRUE;
  else
    bActivate = FALSE;

  BOOL bSuccess = ChangeTableJournaling(bActivate);
  if (bSuccess)
    CDialog::OnOK();
}

void CuDlgTableJournaling::OnRadJnlActive() 
{
  CheckRadioButton(IDC_RAD_JNL_ACTIVE, IDC_RAD_JNL_AFTERCKPDB, IDC_RAD_JNL_ACTIVE);
  GetDlgItem(IDOK)->EnableWindow(TRUE);
}

void CuDlgTableJournaling::OnRadJnlAfterckpdb() 
{
  CheckRadioButton(IDC_RAD_JNL_ACTIVE, IDC_RAD_JNL_AFTERCKPDB, IDC_RAD_JNL_AFTERCKPDB);
  GetDlgItem(IDOK)->EnableWindow(TRUE);
}

void CuDlgTableJournaling::OnRadJnlInactive() 
{
  CheckRadioButton(IDC_RAD_JNL_ACTIVE, IDC_RAD_JNL_AFTERCKPDB, IDC_RAD_JNL_INACTIVE);
  GetDlgItem(IDOK)->EnableWindow(TRUE);
}
