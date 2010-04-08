/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : xinstenl.cpp, Implementation file                                     //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II/VDBA                                                        //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Enable/Disable of Install Security Auditing Level ( IISECURITY_STATE) //
****************************************************************************************/

#include "stdafx.h"
#include "mainmfc.h"
#include "dgdpenle.h"
#include "xinstenl.h"
#include "sqlselec.h"
extern BOOL INGRESII_QuerySecurityState (CTypedPtrList<CObList, CaAuditEnabledLevel*>& listEnableLevel);

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#if !defined (BUFSIZE)
#define BUFSIZE 256
#endif

extern "C"
{
#include "getdata.h"
}

/////////////////////////////////////////////////////////////////////////////
// CxDlgInstallLevelSecurityEnableLevel dialog


CxDlgInstallLevelSecurityEnableLevel::CxDlgInstallLevelSecurityEnableLevel(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgInstallLevelSecurityEnableLevel::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgInstallLevelSecurityEnableLevel)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CxDlgInstallLevelSecurityEnableLevel::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgInstallLevelSecurityEnableLevel)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgInstallLevelSecurityEnableLevel, CDialog)
	//{{AFX_MSG_MAP(CxDlgInstallLevelSecurityEnableLevel)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgInstallLevelSecurityEnableLevel message handlers

void CxDlgInstallLevelSecurityEnableLevel::OnOK() 
{
	BOOL bError = FALSE;
	BOOL bSet  = FALSE;
	CWaitCursor doWaitCursor;
	CaAuditEnabledLevel* pData = NULL;
	CTypedPtrList<CObList, CaAuditEnabledLevel*> listEnableLevel;
	try
	{
		BOOL bEnabled = FALSE;
		int nNodeHandle = GetCurMdiNodeHandle();
		int i, nCount = m_cListCtrl.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			bEnabled = m_cListCtrl.GetCheck (i, 1);
			pData = (CaAuditEnabledLevel*)m_cListCtrl.GetItemData (i);
			if (pData->m_bEnabled != bEnabled)
			{
				pData->m_bEnabled =bEnabled;
				listEnableLevel.AddTail (pData);
			}
		}
		bSet = TRUE;

		CaInstallLevelSecurityState securityState(nNodeHandle);
		BOOL bOK = securityState.UpdateState (listEnableLevel);
		if (!bOK)
		{
			//CString strMsg = _T("Enable/Disable Audit Levels failed");
			AfxMessageBox (VDBA_MfcResourceString(IDS_E_AUDIT_LEVEL));
			//
			// Reset the audit flag:
			while (!listEnableLevel.IsEmpty())
			{
				pData = listEnableLevel.RemoveHead();
				pData->m_bEnabled = !pData->m_bEnabled;
			}
			return;
		}
	}
	catch (CMemoryException* e)
	{
		VDBA_OutOfMemoryMessage();
		e->Delete();
		bError = TRUE;
	}
	catch (CeSQLException e)
	{
		AfxMessageBox (e.m_strReason);
		bError = TRUE;
	}
	catch (...)
	{
		CString strMsg = _T("Unknown error occured in: CuDlgDomPropInstallationLevelEnabledLevel::OnUpdateData\n");
		TRACE0 (strMsg);
		bError = TRUE;
	}
	if (bError)
	{
		//
		// Reset the audit flag:
		if (!bSet)
			return;
		while (!listEnableLevel.IsEmpty())
		{
			pData = listEnableLevel.RemoveHead();
			pData->m_bEnabled = !pData->m_bEnabled;
		}
		return;
	}
	CDialog::OnOK();
}

BOOL CxDlgInstallLevelSecurityEnableLevel::OnInitDialog() 
{
	CDialog::OnInitDialog();
	try
	{
		VERIFY (m_cListCtrl.SubclassDlgItem (IDC_MFC_LIST1, this));
		m_ImageCheck.Create (IDB_CHECK, 16, 1, RGB (255, 0, 0));
		m_cListCtrl.SetCheckImageList(&m_ImageCheck);
		m_ImageList.Create(IDB_SECALARM, 16, 1, RGB(255, 0, 255));
		m_cListCtrl.SetImageList(&m_ImageList, LVSIL_SMALL);
		//
		// Initalize the Column Header of CListCtrl (CuListCtrl)
		//
		LV_COLUMN lvcolumn;
		LSCTRLHEADERPARAMS lsp[2] =
		{
			{_T(""),     90,  LVCFMT_LEFT,  FALSE},
			{_T(""),    100,  LVCFMT_CENTER, TRUE}
		};
		lstrcpy( lsp[0].m_tchszHeader,VDBA_MfcResourceString(IDS_TC_AUDIT_FLAG));  // _T("Audit Flag")
		lstrcpy( lsp[1].m_tchszHeader,VDBA_MfcResourceString(IDS_TC_ENABLED));     // _T("Enabled")

		memset (&lvcolumn, 0, sizeof (LV_COLUMN));
		lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
		for (int i=0; i<2; i++)
		{
			lvcolumn.fmt      = lsp[i].m_fmt;
			lvcolumn.pszText  = lsp[i].m_tchszHeader;
			lvcolumn.iSubItem = i;
			lvcolumn.cx       = lsp[i].m_cxWidth;
			m_cListCtrl.InsertColumn (i, &lvcolumn, (i==1)? TRUE: FALSE);
		}

		int nNodeHandle = GetCurMdiNodeHandle();
		CTypedPtrList<CObList, CaAuditEnabledLevel*> listEnableLevel;
		CaInstallLevelSecurityState securityState(nNodeHandle);
		if (!securityState.QueryState (listEnableLevel))
			return 0;

		while (!listEnableLevel.IsEmpty())
		{
			CaAuditEnabledLevel* pData = listEnableLevel.RemoveHead();
			AddItem (_T(""), pData);
		}
		DisplayItems();
	}
	catch (CMemoryException* e)
	{
		VDBA_OutOfMemoryMessage();
		e->Delete();
		return FALSE;
	}
	catch (CeSQLException e)
	{
		AfxMessageBox (e.m_strReason);
		return FALSE;
	}
	catch (...)
	{
		CString strMsg = _T("Unknown error occured in: CuDlgDomPropInstallationLevelEnabledLevel::OnUpdateData\n");
		TRACE0 (strMsg);
		return FALSE;
	}
	PushHelpId (IDD_XENABLE_SECURITY_LEVEL);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgInstallLevelSecurityEnableLevel::AddItem (LPCTSTR lpszItem, CaAuditEnabledLevel* pData)
{
	int nPos = m_cListCtrl.InsertItem (m_cListCtrl.GetItemCount(), lpszItem, 0);
	if (nPos != -1)
		m_cListCtrl.SetItemData (nPos, (DWORD)pData);
}

void CxDlgInstallLevelSecurityEnableLevel::DisplayItems()
{
	CaAuditEnabledLevel* p = NULL;
	int i, nCount = m_cListCtrl.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		p = (CaAuditEnabledLevel*)m_cListCtrl.GetItemData (i);
		if (!p)
			continue;
		m_cListCtrl.SetItemText (i, 0, p->m_strAuditFlag);
		m_cListCtrl.SetCheck    (i, 1, p->m_bEnabled);
	}
}

void CxDlgInstallLevelSecurityEnableLevel::OnDestroy() 
{
	CaAuditEnabledLevel* pData = NULL;
	int i, nCount = m_cListCtrl.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		pData = (CaAuditEnabledLevel*)m_cListCtrl.GetItemData (i);
		if (pData)
			delete pData;
	}
	CDialog::OnDestroy();
	PopHelpId();
}
