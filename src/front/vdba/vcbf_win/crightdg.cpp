/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : crightdg.cpp, Implementation file 
**
**    Project  : OpenIngres Configuration Manager (Origin IPM Project) 
**    Author   : UK Sotheavut 
**               Blattes Emannuel (Adds additional pages, Custom Implementations)
** 
**    Purpose  : Modeless Dialog Box that will appear at the right pane of the  
**               page Configure 
**               It contains a TabCtrl and maintains the pages (Modeless Dialogs) 
**               that will appeare at each tab. It also keeps track of the current
**               object being displayed. When we get the new object to display,
**               we check if it is the same Class of the Current Object for the
**               purpose of just updating the data of the current page.
** History:
** xx-Sep-1997 (uk$so01)
**    created.
** 09-dec-98 (cucjo01)
**    Added ICE Server Page support
**    Added Gateway Pages for Oracle, Informix, Sybase, MSSQL, and ODBC
** 21-may-99 (cucjo01)
**    Passed pItem along with user defined messages for
**    right pane updates and validates.
** 16-may-00 (cucjo01)
**    Added JDBC Server support.
** 06-Jun-2000: (uk$so01) 
**    (BUG #99242)
**    Cleanup code for DBCS compliant
** 01-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
** 20-jun-2003 (uk$so01)
**    SIR #110389 / INGDBA210
**    Add functionalities for configuring the GCF Security and
**    the security mechanisms.
** 25-Sep-2003 (uk$so01)
**    SIR #105781 of cbf
** 17-Dec-2003 (schph01)
**    SIR #111462, Put string into resource files
** 12-Mar-2004 (uk$so01)
**    SIR #110013 (Handle the DAS Server)
** 06-May-2007 (drivi01)
**    SIR #118462
**    Add IDD_GW_DB2UDB_PAGE_PARAMETER to the list of modeless
**    dialog pages.
*/

#include "stdafx.h"
#include "vcbf.h"
#include "crightdg.h"

#include "conffrm.h"
#include "cbfdisp.h"

#include "paramdlg.h"    // DBMS Parameter 
#include "cachedlg.h"    // DBMS Cache
#include "derivdlg.h"    // DBMS Derived
#include "nampadlg.h"    // NAME Parameter
#include "netprdlg.h"    // NET Protocol
#include "netpadlg.h"    // NET Parameter
#include "netrgdlg.h"    // NET Registry Protocol

#include "jdbpadlg.h"    // JDBC Parameter
#include "daspadlg.h"    // DASVR Parameter
#include "dasprdlg.h"    // DASVR Protocol

#include "seoptdlg.h"    // SECURITY Option 
#include "secdlgsy.h"    // SECURITY System 
#include "secmech.h"     // SECURITY Mechanism 
#include "sedrvdlg.h"    // SECURITY Derived 
#include "seauddlg.h"    // SECURITY Audit 
#include "seaurdlg.h"    // SECURITY Audit Derived 
#include "selogdlg.h"    // SECURITY Log File  

#include "lksprdlg.h"    // LOCKING SYSTEM Parameter 
#include "lksdrdlg.h"    // LOCKING SYSTEM Derived 

#include "lgsprdlg.h"    // LOGGING SYSTEM Parameter 
#include "lgsdrdlg.h"    // LOGGING SYSTEM Derived 

#include "staprdlg.h"    // STAR Parameter 
#include "stadrdlg.h"    // STAR Derived 

#include "intprdlg.h"    // INTERNET Parameter 

#include "brdpadlg.h"    // BRIDGE Parameter 
#include "brdprdlg.h"    // BRIDGE Protocol 
#include "brdvndlg.h"    // BRIDGE Protocol 

#include "recpadlg.h"    // RECOVERY Parameter 
#include "recdrdlg.h"    // RECOVERY Derived 

#include "orapadlg.h"    // ORACLE GATEWAY Parameter 
#include "infpadlg.h"    // INFORMIX GATEWAY Parameter 
#include "sybpadlg.h"    // SYBASE GATEWAY Parameter 
#include "msqpadlg.h"    // MSSQL GATEWAY Parameter 
#include "odbpadlg.h"    // ODBC GATEWAY Parameter 

#include "genericstartup.h"     // Generic Startup

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


inline static int compare (const void* e1, const void* e2)
{
	UINT o1 = *((UINT*)e1);
	UINT o2 = *((UINT*)e2);
	return (o1 > o2)? 1: (o1 < o2)? -1: 0;
}

static int Find (UINT nIDD, DLGSTRUCT* pArray, int first, int last)
{
	if (first <= last)
	{
		int mid = (first + last) / 2;
		if (pArray [mid].nIDD == nIDD)
			return mid;
		else
		if (pArray [mid].nIDD > nIDD)
			return Find (nIDD, pArray, first, mid-1);
		else
			return Find (nIDD, pArray, mid+1, last);
	}
	return -1;
}


/////////////////////////////////////////////////////////////////////////////
// CConfRightDlg dialog

CWnd* CConfRightDlg::ConstructPage (UINT nIDD)
{
	CWnd* pPage = NULL;
	switch (nIDD)
	{
	//
	// Modeless Dialog Pages
	case IDD_DBMS_PAGE_PARAMETER:
		pPage = (CWnd*)new CuDlgDbmsParameter (&m_cTab1);
		if (!((CuDlgDbmsParameter*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;
	case IDD_DBMS_PAGE_CACHE:
		pPage = (CWnd*)new CuDlgDbmsCache (&m_cTab1);
		if (!((CuDlgDbmsCache*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;
	case IDD_DBMS_PAGE_DERIVED:
		pPage = (CWnd*)new CuDlgDbmsDerived (&m_cTab1);
		if (!((CuDlgDbmsDerived*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;

	case IDD_NAME_PAGE_PARAMETER:
		pPage = (CWnd*)new CuDlgNameParameter (&m_cTab1);
		if (!((CuDlgNameParameter*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;

	case IDD_NET_PAGE_PROTOCOL:
		pPage = (CWnd*)new CuDlgNetProtocol (&m_cTab1);
		if (!((CuDlgNetProtocol*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;
	case IDD_NET_PAGE_REGISTRY:
		pPage = (CWnd*)new CuDlgNetRegistry (&m_cTab1);
		if (!((CuDlgNetRegistry*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;

	case IDD_SECURITY_PAGE_OPTION:
		pPage = (CWnd*)new CuDlgSecurityOption (&m_cTab1);
		if (!((CuDlgSecurityOption*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;
	case IDD_SECURITY_PAGE_SYSTEM:
		pPage = (CWnd*)new CuDlgSecuritySystem (&m_cTab1);
		if (!((CuDlgSecuritySystem*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;
	case IDD_SECURITY_PAGE_MECHANISM:
		pPage = (CWnd*)new CuDlgSecurityMechanism (&m_cTab1);
		if (!((CuDlgSecurityMechanism*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;
	case IDD_SECURITY_PAGE_DERIVED:
		pPage = (CWnd*)new CuDlgSecurityDerived (&m_cTab1);
		if (!((CuDlgSecurityDerived*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;
	case IDD_SECURITY_PAGE_AUDITING:
		pPage = (CWnd*)new CuDlgSecurityAuditing (&m_cTab1);
		if (!((CuDlgSecurityAuditing*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;
	case IDD_SECURITY_PAGE_AUDITDERIVED:
		pPage = (CWnd*)new CuDlgSecurityAuditingDerived (&m_cTab1);
		if (!((CuDlgSecurityAuditingDerived*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;
	case IDD_SECURITY_PAGE_LOGFILE:
		pPage = (CWnd*)new CuDlgSecurityLogFile (&m_cTab1);
		if (!((CuDlgSecurityLogFile*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;

	case IDD_LOCKINGSYS_PAGE_PARAMETER:
		pPage = (CWnd*)new CuDlgLockingSystemParameter (&m_cTab1);
		if (!((CuDlgLockingSystemParameter*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;
	case IDD_LOCKINGSYS_PAGE_DERIVED:
		pPage = (CWnd*)new CuDlgLockingSystemDerived (&m_cTab1);
		if (!((CuDlgLockingSystemDerived*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;

	case IDD_LOGGINGSYS_PAGE_PARAMETER:
		pPage = (CWnd*)new CuDlgLoggingSystemParameter (&m_cTab1);
		if (!((CuDlgLoggingSystemParameter*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;
	case IDD_LOGGINGSYS_PAGE_DERIVED:
		pPage = (CWnd*)new CuDlgLoggingSystemDerived (&m_cTab1);
		if (!((CuDlgLoggingSystemDerived*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;

	case IDD_STAR_PAGE_PARAMETER:
		pPage = (CWnd*)new CuDlgStarParameter (&m_cTab1);
		if (!((CuDlgStarParameter*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;
	case IDD_STAR_PAGE_DERIVED:
		pPage = (CWnd*)new CuDlgStarDerived (&m_cTab1);
		if (!((CuDlgStarDerived*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;

	case IDD_INTERNET_PAGE_PARAMETER:
		pPage = (CWnd*)new CuDlgInternetParameter (&m_cTab1);
		if (!((CuDlgInternetParameter*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;
	case IDD_NET_PAGE_PARAMETER:
		pPage = (CWnd*)new CuDlgNetParameter (&m_cTab1);
		if (!((CuDlgNetParameter*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;
	case IDD_JDBC_PAGE_PARAMETER:
		pPage = (CWnd*)new CuDlgJDBCParameter (&m_cTab1);
		if (!((CuDlgJDBCParameter*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;
	case IDD_DASVR_PAGE_PARAMETER:
		pPage = (CWnd*)new CuDlgDASVRParameter (&m_cTab1);
		if (!((CuDlgDASVRParameter*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;
	case IDD_DASVR_PAGE_PROTOCOL:
		pPage = (CWnd*)new CuDlgDASVRProtocol (&m_cTab1);
		if (!((CuDlgDASVRProtocol*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;
	case IDD_BRIDGE_PAGE_PARAMETER:
		pPage = (CWnd*)new CuDlgBridgeParameter (&m_cTab1);
		if (!((CuDlgBridgeParameter*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;
	case IDD_BRIDGE_PAGE_PROTOCOL:
		pPage = (CWnd*)new CuDlgBridgeProtocol (&m_cTab1);
		if (!((CuDlgBridgeProtocol*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;
	case IDD_BRIDGE_PAGE_VNODE:
		pPage = (CWnd*)new CuDlgBridgeVNode (&m_cTab1);
		if (!((CuDlgBridgeVNode*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;
	case IDD_GENERIC_PAGE_STARTUP_COUNT:   // CREATES PAGE
		pPage = (CWnd*)new CuDlgGenericStartup (&m_cTab1);
		if (!((CuDlgGenericStartup*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;
	case IDD_RECOVERY_PAGE_PARAMETER:
		pPage = (CWnd*)new CuDlgRecoveryParameter (&m_cTab1);
		if (!((CuDlgRecoveryParameter*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;
	case IDD_RECOVERY_PAGE_DERIVED:
		pPage = (CWnd*)new CuDlgRecoveryDerived (&m_cTab1);
		if (!((CuDlgRecoveryDerived*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;

	case IDD_GW_ORACLE_PAGE_PARAMETER:
		pPage = (CWnd*)new CuDlgGW_OracleParameter (&m_cTab1);
		if (!((CuDlgGW_OracleParameter*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;
	case IDD_GW_DB2UDB_PAGE_PARAMETER:
		pPage = (CWnd*)new CuDlgGW_OracleParameter (&m_cTab1);
		if (!((CuDlgGW_OracleParameter*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;
	case IDD_GW_INFORMIX_PAGE_PARAMETER:
		pPage = (CWnd*)new CuDlgGW_InformixParameter (&m_cTab1);
		if (!((CuDlgGW_InformixParameter*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;
	case IDD_GW_SYBASE_PAGE_PARAMETER:
		pPage = (CWnd*)new CuDlgGW_SybaseParameter (&m_cTab1);
		if (!((CuDlgGW_SybaseParameter*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;
	case IDD_GW_MSSQL_PAGE_PARAMETER:
		pPage = (CWnd*)new CuDlgGW_MSSQLParameter (&m_cTab1);
		if (!((CuDlgGW_MSSQLParameter*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;
	case IDD_GW_ODBC_PAGE_PARAMETER:
		pPage = (CWnd*)new CuDlgGW_ODBCParameter (&m_cTab1);
		if (!((CuDlgGW_ODBCParameter*)pPage)->Create (nIDD, &m_cTab1))
			AfxThrowResourceException();
		return pPage;

	//
	// Form View Pages.
	default:
		return FormViewPage (nIDD);
	}
}

CWnd* CConfRightDlg::GetPage (UINT nIDD)
{
	int index = Find (nIDD, m_arrayDlg, 0, NUMBEROFPAGES -1);
	//
	// Call GetPage, Did you forget to define the page in this file ???..
	ASSERT (index != -1);
	if (index == -1)
		return NULL;
	else
	{
		if (m_arrayDlg[index].pPage)
			return m_arrayDlg[index].pPage;
		else
		{
			m_arrayDlg[index].pPage = ConstructPage (nIDD);
			return m_arrayDlg[index].pPage;
		}
	}
	
	for (int i=0; i<NUMBEROFPAGES; i++)
	{
		if (m_arrayDlg[i].nIDD == nIDD)
		{
			if (m_arrayDlg[i].pPage)
				return m_arrayDlg[i].pPage;
			try
			{
				m_arrayDlg[i].pPage = ConstructPage (nIDD);
				return m_arrayDlg[i].pPage;
			}
			catch (...)
			{
				throw;
				return NULL;
			}
		}
	}
	//
	// Call GetPage, you must define some dialog box.(Form view)..
	ASSERT (FALSE);
	return NULL;
}


CConfRightDlg::CConfRightDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CConfRightDlg::IDD, pParent)
{
	UINT nArrayIDD [NUMBEROFPAGES] = 
	{
		IDD_DBMS_PAGE_PARAMETER,        //  DBMS Parameter 
		IDD_DBMS_PAGE_DATABASE,         //  DBMS Database.
		IDD_DBMS_PAGE_DERIVED,          //  DBMS Derived
		IDD_DBMS_PAGE_CACHE,            //  DBMS Cache
		IDD_GENERIC_PAGE_STARTUP_COUNT, //  GENERIC Startup Count

		IDD_NET_PAGE_PARAMETER,         //  NET Parameter
		IDD_NET_PAGE_PROTOCOL,          //  NET Protocol
		IDD_NET_PAGE_REGISTRY,          //  NET Registry Protocol

		IDD_GENERIC_PAGE_STARTUP_COUNT, //  GENERIC Startup Count
		IDD_NAME_PAGE_PARAMETER,        //  NAME Parameter
		IDD_GENERIC_PAGE_STARTUP_COUNT, //  GENERIC Startup Count

		IDD_LOGFILE_PAGE_CONFIGURE,     //  Primary LOG File Configure
		IDD_LOGFILE_PAGE_SECONDARY,     //  Secondary LOG File Configure
		IDD_GENERIC_PAGE_STARTUP_COUNT, //  GENERIC Startup Count

		IDD_SECURITY_PAGE_OPTION,       //  SECURITY Option 
		IDD_SECURITY_PAGE_SYSTEM,       //  SECURITY System 
		IDD_SECURITY_PAGE_MECHANISM,    //  SECURITY Mechanism 
		IDD_SECURITY_PAGE_DERIVED,      //  SECURITY Derived 
		IDD_SECURITY_PAGE_AUDITING,     //  SECURITY Audit 
		IDD_SECURITY_PAGE_AUDITDERIVED, //  SECURITY Audit Derived 
		IDD_SECURITY_PAGE_LOGFILE,      //  SECURITY Log File 
		IDD_GENERIC_PAGE_STARTUP_COUNT, //  GENERIC Startup Count

		IDD_LOCKINGSYS_PAGE_PARAMETER,  //  LOCKING SYSTEM Parameter 
		IDD_LOCKINGSYS_PAGE_DERIVED,    //  LOCKING SYSTEM Derived 
		IDD_GENERIC_PAGE_STARTUP_COUNT, //  GENERIC Startup Count

		IDD_LOGGINGSYS_PAGE_PARAMETER,  //  LOGGING SYSTEM Parameter 
		IDD_LOGGINGSYS_PAGE_DERIVED,    //  LOGGING SYSTEM Derived 
		IDD_GENERIC_PAGE_STARTUP_COUNT, //  GENERIC Startup Count

		IDD_STAR_PAGE_PARAMETER,        //  STAR Parameter 
		IDD_STAR_PAGE_DERIVED,          //  STAR Derived 
		IDD_GENERIC_PAGE_STARTUP_COUNT, //  GENERIC Startup Count

		IDD_INTERNET_PAGE_PARAMETER,    //  INTERNET Parameter 
		IDD_GENERIC_PAGE_STARTUP_COUNT, //  GENERIC Startup Count

		IDD_BRIDGE_PAGE_PARAMETER,      //  BRIDGE Parameter
		IDD_BRIDGE_PAGE_PROTOCOL,       //  BRIDGE Protocol
		IDD_BRIDGE_PAGE_VNODE,          //  BRIDGE VNode
		IDD_GENERIC_PAGE_STARTUP_COUNT, //  GENERIC Startup Count

//		IDD_DBMSOIDT_PAGE_PARAMETER,    // 34 DBMS OIDT Parameter
//		IDD_DBMSOIDT_PAGE_ADVANCED,     // 35 DBMS OIDT Advance
		IDD_GENERIC_PAGE_STARTUP_COUNT, // 36 GENERIC Startup Count

		IDD_RECOVERY_PAGE_PARAMETER,    //  RECOVERY Parameter
		IDD_RECOVERY_PAGE_DERIVED,      //  RECOVERY Derived

		IDD_GW_ORACLE_PAGE_PARAMETER,   //  GW_ORACLE Parameter
		IDD_GW_DB2UDB_PAGE_PARAMETER, 	//  GW_DB2UDB Parameter
		IDD_GW_INFORMIX_PAGE_PARAMETER, //  GW_INFORMIX Parameter
		IDD_GW_SYBASE_PAGE_PARAMETER,   //  GW_SYBASE Parameter
		IDD_GW_MSSQL_PAGE_PARAMETER,    //  GW_MSSQL Parameter
		IDD_GW_ODBC_PAGE_PARAMETER,     //  GW_ODBC Parameter

		IDD_GENERIC_PAGE_STARTUP_COUNT, //  GENERIC Startup Count

		IDD_JDBC_PAGE_PARAMETER,        //  JDBC Parameter
		IDD_DASVR_PAGE_PARAMETER,       //  DASVR Parameter
		IDD_DASVR_PAGE_PROTOCOL,        //  DASVR Protocol
		IDD_GENERIC_PAGE_STARTUP_COUNT  //  GENERIC Startup Count


	};

	qsort ((void*)nArrayIDD, (size_t)NUMBEROFPAGES, (size_t)sizeof(UINT), compare);
	for (int i=0; i<NUMBEROFPAGES; i++)
	{
		m_arrayDlg[i].nIDD = nArrayIDD [i];
		m_arrayDlg[i].pPage= NULL;
	}
	m_pCurrentProperty = NULL;
	m_pCurrentPage = NULL;
	m_bIsLoading   = FALSE;


	//{{AFX_DATA_INIT(CConfRightDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CConfRightDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConfRightDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}




BEGIN_MESSAGE_MAP(CConfRightDlg, CDialog)
	//{{AFX_MSG_MAP(CConfRightDlg)
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, OnSelchangeTab1)
	ON_BN_CLICKED(IDC_BUTTON1, OnButton1)
	ON_BN_CLICKED(IDC_BUTTON2, OnButton2)
	ON_BN_CLICKED(IDC_BUTTON3, OnButton3)
	ON_BN_CLICKED(IDC_BUTTON4, OnButton4)
	ON_BN_CLICKED(IDC_BUTTON5, OnButton5)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_UPDATING, OnUpdateData)
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_VALIDATE, OnValidateData)
END_MESSAGE_MAP()


LONG CConfRightDlg::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	if (m_pCurrentPage)
	{
		m_pCurrentPage->SendMessage(WMUSRMSG_CBF_PAGE_UPDATING, (WPARAM)wParam, (LPARAM)lParam);
	}
	return 0L;
}

LONG CConfRightDlg::OnValidateData (WPARAM wParam, LPARAM lParam)
{
	if (m_pCurrentPage)
	{
		//
		// Try to save (and validate) the data that has been changed
		// in the old page before displaying a new page.
		m_pCurrentPage->SendMessage(WMUSRMSG_CBF_PAGE_VALIDATE, (WPARAM)wParam, (LPARAM)lParam);
	}
	return 0L;
}

void CConfRightDlg::DisplayPage (CuPageInformation* pPageInfo)
{
	CRect   r;
	TC_ITEM item;
	int     i, nPages;
	UINT*   nTabID;
	UINT*   nDlgID;
	CString strTab;
	CString strTitle;
	UINT    nIDD; 
	CWnd* pDlg;
	CView*   pView = (CView*)GetParent();               ASSERT (pView);
	
	//
	// Set the default label of the two buttons: (Derfault, Restore)
	CWnd* pB1 = GetDlgItem (IDC_BUTTON1);
	CWnd* pB2 = GetDlgItem (IDC_BUTTON2);
	CString csButtonTitle;
	csButtonTitle.LoadString(IDS_BUTTON_EDIT_VALUE);
	pB1->SetWindowText (csButtonTitle);

	csButtonTitle.LoadString(IDS_BUTTON_RESTORE);
	pB2->SetWindowText (csButtonTitle);

	pB1->EnableWindow  (FALSE);
	pB2->EnableWindow  (FALSE);


	// hide buttons 1 and 2 if no page
	int showFlag = SW_HIDE;
	if (pPageInfo && pPageInfo->GetNumberOfPage() > 0)
		showFlag = SW_SHOW;
	pB1->ShowWindow(showFlag);
	pB2->ShowWindow(showFlag);

	// Always Hide the other buttons
	CWnd* pB3 = GetDlgItem (IDC_BUTTON3);
	CWnd* pB4 = GetDlgItem (IDC_BUTTON4);
	CWnd* pB5 = GetDlgItem (IDC_BUTTON5);
	pB3->ShowWindow(SW_HIDE); pB3->SetWindowText(_T("Button &3"));
	pB4->ShowWindow(SW_HIDE); pB4->SetWindowText(_T("Button &4"));
	pB5->ShowWindow(SW_HIDE); pB5->SetWindowText(_T("Button &5"));

	// always enable all buttons
	pB1->EnableWindow(TRUE);
	pB2->EnableWindow(TRUE);
	pB3->EnableWindow(TRUE);
	pB4->EnableWindow(TRUE);
	pB5->EnableWindow(TRUE);

	if (m_bIsLoading)
	{
		m_bIsLoading = FALSE;
	}
	if (!pPageInfo)
	{
		if (m_pCurrentPage)
		{
			//
			// Try to save (and validate) the data that has been changed
			// in the old page before displaying a new page.
			m_pCurrentPage->SendMessage(WMUSRMSG_CBF_PAGE_VALIDATE, (WPARAM)0, (LPARAM)0);
			m_pCurrentPage->ShowWindow (SW_HIDE);
		}
		m_cTab1.DeleteAllItems();
		if (m_pCurrentProperty)
		{
			m_pCurrentProperty->SetPageInfo (NULL);
			delete m_pCurrentProperty;
			m_pCurrentProperty = NULL;
			m_pCurrentPage     = NULL;
		}
		return;
	}
	if (!m_pCurrentProperty)
	{
		try
		{
			m_pCurrentProperty = new CuCbfProperty (0, pPageInfo);
		}
		catch (CMemoryException* e)
		{
			VCBF_OutOfMemoryMessage();
			e->Delete();
			m_pCurrentPage = NULL;
			m_pCurrentProperty = NULL;
			return;
		}
		//
		// Set up the Title:
		pPageInfo->GetTitle (strTitle);
		
		nPages = pPageInfo->GetNumberOfPage();
		if (nPages < 1)
		{
			m_pCurrentPage = NULL;
			return;
		}
		//
		// Construct the Tab(s)
		nTabID = pPageInfo->GetTabID();
		nDlgID = pPageInfo->GetDlgID();
		memset (&item, 0, sizeof (item));
		item.mask       = TCIF_TEXT;
		item.cchTextMax = 32;
		m_cTab1.DeleteAllItems();
		for (i=0; i<nPages; i++)
		{
			strTab.LoadString (nTabID [i]);
			item.pszText = (LPTSTR)(LPCTSTR)strTab;
			m_cTab1.InsertItem (i, &item);
		}
		//
		// Display the default (the first in general) page,
		int iPage = pPageInfo->GetDefaultPage ();
		nIDD    = pPageInfo->GetDlgID (iPage);
		try 
		{
			pDlg= GetPage (nIDD);
			if (!pDlg)
				return;
		}
		catch (CMemoryException* e)
		{
			VCBF_OutOfMemoryMessage ();
			e->Delete();
			return;
		}
		catch (CResourceException* e)
		{
			AfxMessageBox (_T("Load resource failed"));
			e->Delete();
			return;
		}
		m_cTab1.SetCurSel (iPage);
		m_cTab1.GetClientRect (r);
		m_cTab1.AdjustRect (FALSE, r);
		pDlg->MoveWindow (r);
		pDlg->ShowWindow(SW_SHOW);
		m_pCurrentPage = pDlg;
		m_pCurrentPage->SendMessage (WMUSRMSG_CBF_PAGE_UPDATING, (WPARAM)0, (LPARAM)pPageInfo->GetCbfItem());
	}
	else
	{
		CuPageInformation* pCurrentPageInfo = m_pCurrentProperty->GetPageInfo();
		if (pCurrentPageInfo->GetClassName()==pPageInfo->GetClassName())
		{
			m_pCurrentProperty->SetPageInfo (pPageInfo);
			//
			// In general, when the selection changes on the left-pane, 
			// and if the new selected item if the same class as the old one
			// then we keep the current selected page (Tab) at the right-pane.
			// Special case:
			// ------------
			// For the class LOGFILE, we use the default page setting !!!
			if (pCurrentPageInfo->GetClassName()== "LOGFILE")
			{
				int iPage = pPageInfo->GetDefaultPage ();
				nIDD      = pPageInfo->GetDlgID (iPage);
				CWnd* pDlg= GetPage (nIDD);
				if (!pDlg)
					return;
				m_cTab1.SetCurSel (iPage);
				m_cTab1.GetClientRect (r);
				m_cTab1.AdjustRect (FALSE, r);
				if (m_pCurrentPage)
				{
					//
					// Try to save (and validate) the data that has been changed
					// in the old page before displaying a new page.
					m_pCurrentPage->SendMessage(WMUSRMSG_CBF_PAGE_VALIDATE, (WPARAM)0, (LPARAM)0);
					m_pCurrentPage->ShowWindow (SW_HIDE);
				}
				pDlg->MoveWindow (r);
				pDlg->ShowWindow(SW_SHOW);
				m_pCurrentPage = pDlg;
			}
			//
			// wParam, and lParam will contain the information needed
			// by the dialog to refresh itself.
			if (m_pCurrentPage) 
				m_pCurrentPage->SendMessage (WMUSRMSG_CBF_PAGE_UPDATING, (WPARAM)0, (LPARAM)pPageInfo->GetCbfItem());
		}
		else
		{
			if (m_pCurrentPage) 
			{
				//
				// Try to save (and validate) the data that has been changed
				// in the old page before displaying a new page.
				m_pCurrentPage->SendMessage(WMUSRMSG_CBF_PAGE_VALIDATE, (WPARAM)0, (LPARAM)0);
				m_pCurrentPage->ShowWindow (SW_HIDE);
			}
			m_cTab1.DeleteAllItems();
			nPages = pPageInfo->GetNumberOfPage();
			m_pCurrentProperty->SetPageInfo (pPageInfo);
			m_pCurrentProperty->SetCurSel (0);
			
			if (nPages < 1)
			{
				m_pCurrentPage = NULL;
				return;
			}
			UINT nIDD; 
			CWnd* pDlg;
			//
			// Construct the Tab(s)
			nTabID = pPageInfo->GetTabID();
			nDlgID = pPageInfo->GetDlgID();
			memset (&item, 0, sizeof (item));
			item.mask       = TCIF_TEXT;
			item.cchTextMax = 32;
			m_cTab1.DeleteAllItems();
			for (i=0; i<nPages; i++)
			{
				strTab.LoadString (nTabID [i]);
				item.pszText = (LPTSTR)(LPCTSTR)strTab;
				m_cTab1.InsertItem (i, &item);
			}
			//
			// Display the default (the first) page.
			int iPage = pPageInfo->GetDefaultPage ();
			nIDD    = pPageInfo->GetDlgID (iPage);
			try 
			{
				pDlg= GetPage (nIDD);
				if (!pDlg)
					return;
			}
			catch (CMemoryException* e)
			{
				VCBF_OutOfMemoryMessage();
				e->Delete();
				return;
			}
			catch (CResourceException* e)
			{
				AfxMessageBox (_T("Load resource failed"));
				e->Delete();
				return;
			}
			m_cTab1.SetCurSel (iPage);
			m_cTab1.GetClientRect (r);
			m_cTab1.AdjustRect (FALSE, r);
			pDlg->MoveWindow (r);
			pDlg->ShowWindow(SW_SHOW);
			m_pCurrentPage = pDlg;
			m_pCurrentPage->SendMessage (WMUSRMSG_CBF_PAGE_UPDATING, (WPARAM)0, (LPARAM)pPageInfo->GetCbfItem());
		}
	}
}



/////////////////////////////////////////////////////////////////////////////
// CConfRightDlg message handlers

BOOL CConfRightDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	VERIFY (m_cTab1.SubclassDlgItem (IDC_TAB1, this));

	//
	// hide all buttons
	GetDlgItem (IDC_BUTTON1)->ShowWindow(SW_HIDE);
	GetDlgItem (IDC_BUTTON2)->ShowWindow(SW_HIDE);
	GetDlgItem (IDC_BUTTON3)->ShowWindow(SW_HIDE);
	GetDlgItem (IDC_BUTTON4)->ShowWindow(SW_HIDE);
	GetDlgItem (IDC_BUTTON5)->ShowWindow(SW_HIDE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CConfRightDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);

	if (!IsWindow (m_cTab1.m_hWnd))
		return;
	
	CRect newRect, r, rDlg;
	GetClientRect (rDlg);
	//
	// Resize the 5 buttons (Default and Restore, plus 3 other)
	CButton* pB1 = (CButton*)GetDlgItem (IDC_BUTTON1);
	CButton* pB2 = (CButton*)GetDlgItem (IDC_BUTTON2);
	CButton* pB3 = (CButton*)GetDlgItem (IDC_BUTTON3);
	CButton* pB4 = (CButton*)GetDlgItem (IDC_BUTTON4);
	CButton* pB5 = (CButton*)GetDlgItem (IDC_BUTTON5);
	
	pB1->GetWindowRect (r);
	ScreenToClient (r);
	newRect = r;
	newRect.top    =rDlg.bottom - 2 - r.Height();
	newRect.bottom = newRect.top + r.Height();
	pB1->MoveWindow (newRect);

	pB2->GetWindowRect (r);
	ScreenToClient (r);
	newRect.left = r.left;
	newRect.right = r.right;
	pB2->MoveWindow (newRect);  // keep top and bottom from button1

	pB3->GetWindowRect (r);
	ScreenToClient (r);
	newRect.left = r.left;
	newRect.right = r.right;
	pB3->MoveWindow (newRect);  // keep top and bottom from button1

	pB4->GetWindowRect (r);
	ScreenToClient (r);
	newRect.left = r.left;
	newRect.right = r.right;
	pB4->MoveWindow (newRect);  // keep top and bottom from button1

	pB5->GetWindowRect (r);
	ScreenToClient (r);
	newRect.left = r.left;
	newRect.right = r.right;
	pB5->MoveWindow (newRect);  // keep top and bottom from button1

	//
	// Resize the Tab Control
	newRect.bottom  = newRect.top - 10;
	newRect.top     = rDlg.top;
	newRect.left    = rDlg.left;
	newRect.right   = rDlg.right;
	m_cTab1.MoveWindow (newRect);
	m_cTab1.GetClientRect (r);
	m_cTab1.AdjustRect (FALSE, r);
	if (m_pCurrentPage)
		m_pCurrentPage->MoveWindow (r);
}

void CConfRightDlg::OnDestroy() 
{
	if (m_pCurrentProperty)
	{
		m_pCurrentProperty->SetPageInfo (NULL);
		delete m_pCurrentProperty;
	}
	CDialog::OnDestroy();
	
}

void CConfRightDlg::OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	int nSel;
	CRect r;
	CWnd* pNewPage;
	CuPageInformation* pPageInfo;
	CWnd* pParent1 = GetParent();    // The view: CConfViewRight
	ASSERT (pParent1);
	CSplitterWnd* pParent2 = (CSplitterWnd*)pParent1->GetParent();  // The Splitter
	ASSERT (pParent2);
	CConfigFrame* pFrame = (CConfigFrame*)pParent2->GetParent();	// The Frame Window
	ASSERT (pFrame);

	if (!pFrame->IsAllViewsCreated())
		return;
	if (!m_pCurrentProperty)
		return;
	CWaitCursor waitCursor;
	m_cTab1.GetClientRect (r);
	m_cTab1.AdjustRect (FALSE, r);
	nSel = m_cTab1.GetCurSel();
	m_pCurrentProperty->SetCurSel(nSel);
	pPageInfo = m_pCurrentProperty->GetPageInfo();
	try
	{
		pNewPage  = GetPage (pPageInfo->GetDlgID (nSel));
		if (!pNewPage)
			return;
	}
	catch (CMemoryException* e)
	{
		VCBF_OutOfMemoryMessage();
		e->Delete();
		return;
	}
	catch (CResourceException* e)
	{
		AfxMessageBox (_T("Fail to load resource"));
		e->Delete();
		return;
	}
	if (m_pCurrentPage)
	{
		//
		// Try to save (and validate) the data that has been changed
		// in the old page before displaying a new page.
		m_pCurrentPage->SendMessage(WMUSRMSG_CBF_PAGE_VALIDATE, (WPARAM)0, (LPARAM)pPageInfo->GetCbfItem());
		m_pCurrentPage->ShowWindow (SW_HIDE);
	}
	m_pCurrentPage = pNewPage;
	m_pCurrentPage->MoveWindow (r);
	m_pCurrentPage->ShowWindow(SW_SHOW);
	m_pCurrentPage->SendMessage (WMUSRMSG_CBF_PAGE_UPDATING, 0, (LPARAM)pPageInfo->GetCbfItem());

	*pResult = 0;
}

void CConfRightDlg::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}


CWnd* CConfRightDlg::FormViewPage(UINT nIDD)
{
	CRect r;
	m_cTab1.GetClientRect (r);
	m_cTab1.AdjustRect (FALSE, r);
	
	CuDlgViewFrame* pDlgFrame = new CuDlgViewFrame (nIDD);
	BOOL bCreated = pDlgFrame->Create (
		NULL,
		NULL,
		WS_CHILD,
		r,
		&m_cTab1
		);
	if (!bCreated)
		AfxThrowResourceException();
	pDlgFrame->InitialUpdateFrame (NULL, TRUE);
	return (CWnd*)pDlgFrame;
}


void CConfRightDlg::OnButton1() 
{
	TRACE0 ("CConfRightDlg::OnButton1()... (serves as default) \n");	
	if (m_pCurrentPage)
		m_pCurrentPage->SendMessage (WM_CONFIGRIGHT_DLG_BUTTON1, 0, 0);
}

void CConfRightDlg::OnButton2() 
{
	TRACE0 ("CConfRightDlg::OnButton2()... (serves as restore) \n");	
	if (m_pCurrentPage)
		m_pCurrentPage->SendMessage (WM_CONFIGRIGHT_DLG_BUTTON2, 0, 0);
}

void CConfRightDlg::OnButton3() 
{
	TRACE0 ("CConfRightDlg::OnButton3()...\n");	
	if (m_pCurrentPage)
		m_pCurrentPage->SendMessage (WM_CONFIGRIGHT_DLG_BUTTON3, 0, 0);
}

void CConfRightDlg::OnButton4() 
{
	TRACE0 ("CConfRightDlg::OnButton4()...\n");	
	if (m_pCurrentPage)
		m_pCurrentPage->SendMessage (WM_CONFIGRIGHT_DLG_BUTTON4, 0, 0);
}

void CConfRightDlg::OnButton5() 
{
	TRACE0 ("CConfRightDlg::OnButton5()...\n");	
	if (m_pCurrentPage)
		m_pCurrentPage->SendMessage (WM_CONFIGRIGHT_DLG_BUTTON5, 0, 0);
}

