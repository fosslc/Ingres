/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dgdpaulo.cpp, Implementation file 
**    Project  : INGRES II/ VDBA (Installation Level Auditing).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Page of Table control. The DOM PROP Audit Log Page 
**
** 
** Histoty:
** xx-Oct-1998 (uk$so01)
**    Created.
** 26-Fev-2002 (uk$so01)
**    SIR #106648, Integrate ipm.ocx.
** 02-Jun-2010 (drivi01)
**    Removed BUFSIZE redefinition.  It's not needed here anymore.
**    the constant is defined in main.h now. and main.h is included
**    in dom.h.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgdpaulo.h"
#include "domseri.h"
#include "vtree.h"

extern "C"
{
#include "dbaginfo.h"
#include "getdata.h"
#include "domdata.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define FETCH_ROWS_INFO


CuDlgDomPropInstallationLevelAuditLog::CuDlgDomPropInstallationLevelAuditLog(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgDomPropInstallationLevelAuditLog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgDomPropInstallationLevelAuditLog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_nNodeHandle = -1;
	m_pDlgRowLayout = NULL;
}


void CuDlgDomPropInstallationLevelAuditLog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDomPropInstallationLevelAuditLog)
	DDX_Control(pDX, IDC_MFC_COMBO8, m_cComboEnd);
	DDX_Control(pDX, IDC_MFC_COMBO7, m_cComboBegin);
	DDX_Control(pDX, IDC_MFC_COMBO6, m_cComboRealUser);
	DDX_Control(pDX, IDC_MFC_COMBO5, m_cComboUser);
	DDX_Control(pDX, IDC_MFC_STATIC1, m_cStatic1);
	DDX_Control(pDX, IDC_MFC_COMBO4, m_cComboObjectName);
	DDX_Control(pDX, IDC_MFC_COMBO3, m_cComboObjectType);
	DDX_Control(pDX, IDC_MFC_COMBO2, m_cComboDatabase);
	DDX_Control(pDX, IDC_MFC_COMBO1, m_cComboEventType);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropInstallationLevelAuditLog, CDialog)
	//{{AFX_MSG_MAP(CuDlgDomPropInstallationLevelAuditLog)
	ON_WM_SIZE()
	ON_CBN_SELCHANGE(IDC_MFC_COMBO2, OnSelchangeComboDatabase)
	ON_WM_DESTROY()
	ON_CBN_SELCHANGE(IDC_MFC_COMBO3, OnSelchangeComboObjectType)
	ON_BN_CLICKED(IDC_MFC_BUTTON1, OnButtonView)
	ON_CBN_SELCHANGE(IDC_MFC_COMBO1, OnSelchangeComboEventType)
	ON_CBN_SELCHANGE(IDC_MFC_COMBO4, OnSelchangeComboObjectName)
	ON_CBN_SELCHANGE(IDC_MFC_COMBO5, OnSelchangeComboUser)
	ON_CBN_SELCHANGE(IDC_MFC_COMBO6, OnSelchangeComboRealUser)
	ON_CBN_SELCHANGE(IDC_MFC_COMBO7, OnSelchangeComboBegin)
	ON_CBN_SELCHANGE(IDC_MFC_COMBO8, OnSelchangeComboEnd)
	ON_CBN_EDITCHANGE(IDC_MFC_COMBO7, OnEditchangeComboBegin)
	ON_CBN_EDITCHANGE(IDC_MFC_COMBO8, OnEditchangeComboEnd)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
	ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropInstallationLevelAuditLog message handlers

void CuDlgDomPropInstallationLevelAuditLog::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropInstallationLevelAuditLog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_pDlgRowLayout = new CuDlgDomPropTableRows (this);
	m_pDlgRowLayout->Create (IDD_DOMPROP_TABLE_ROWS, this);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropInstallationLevelAuditLog::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (m_pDlgRowLayout && IsWindow (m_pDlgRowLayout->m_hWnd))
	{
		CRect rDlg, r;
		GetClientRect (rDlg);
		m_cStatic1.GetWindowRect (r);
		ScreenToClient (r);
		r.right  = rDlg.right  - r.left;
		r.bottom = rDlg.bottom - r.left;
		m_pDlgRowLayout->MoveWindow (r);
	}
}

LONG CuDlgDomPropInstallationLevelAuditLog::OnQueryType(WPARAM wParam, LPARAM lParam)
{
	ASSERT (wParam == 0);
	ASSERT (lParam == 0);
	return DOMPAGE_SPECIAL;
}


LONG CuDlgDomPropInstallationLevelAuditLog::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	CString strMsg;
	LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;

	m_nNodeHandle = (int)wParam;
	ASSERT (pUps);

	switch (pUps->nIpmHint)
	{
	case 0:
		break;
	case FILTER_SETTING_CHANGE:
		//
		// Setting Change ??
		m_pDlgRowLayout->SendMessage (WM_USER_IPMPAGE_UPDATEING, (WPARAM)0, (LPARAM)pUps);
		return 0;

	default:
		return 0L;
	}

	try
	{
		//ResetDisplay();
		//
		// Set the default event type to: <all>
		if (m_cComboEventType.FindString(-1,VDBA_MfcResourceString(IDS_ALL_AUDIT_EVENT)) == CB_ERR)
			m_cComboEventType.InsertString (0,VDBA_MfcResourceString(IDS_ALL_AUDIT_EVENT));//_T("<all>")
		m_cComboEventType.SetCurSel (0);
		//
		// Set the default object type to: <all>
		if (m_cComboObjectType.FindString(-1,VDBA_MfcResourceString(IDS_ALL_OBJECT_TYPE)) == CB_ERR)
			m_cComboObjectType.InsertString (0,VDBA_MfcResourceString(IDS_ALL_OBJECT_TYPE));//_T("<all>")
		m_cComboObjectType.SetCurSel (0);
		//
		// Set the default audittime: <all>
		if (m_cComboBegin.FindString(-1,VDBA_MfcResourceString(IDS_BEGIN_FIRST)) == CB_ERR)
			m_cComboBegin.InsertString (0,VDBA_MfcResourceString(IDS_BEGIN_FIRST));
		m_cComboBegin.SetCurSel (0);
		if (m_cComboEnd.FindString(-1,VDBA_MfcResourceString(IDS_END_LAST)) == CB_ERR)
			m_cComboEnd.InsertString (0,VDBA_MfcResourceString(IDS_END_LAST));
		m_cComboEnd.SetCurSel (0);
		//
		// Query the databases and fill them in the combo database: 'm_cComboDatabase'
		m_cComboDatabase.ResetContent();
		CStringList listDB;
		if (!QueryDatabase (listDB))
		{
			strMsg = _T("Cannot query databases\n");
			TRACE0 (strMsg);
			return 0;
		}
		m_cComboDatabase.ResetContent();
		m_cComboDatabase.AddString (VDBA_MfcResourceString(IDS_ALL_DB));//_T("<all>")
		while (!listDB.IsEmpty())
		{
			CString strDB = listDB.RemoveHead();
			m_cComboDatabase.AddString (strDB);
		}
		m_cComboDatabase.SetCurSel (0);

		//
		// Empty the combo object names and select the item <all>
		CleanComboObjectName();
		m_cComboObjectName.SetCurSel (0);

		//
		// Initialize the combo User & Real User:
		CStringList listUsers;
		m_cComboUser.ResetContent();
		m_cComboRealUser.ResetContent();
		m_cComboUser.AddString (VDBA_MfcResourceString(IDS_ALL_USER));//_T("<all>")
		m_cComboRealUser.AddString (VDBA_MfcResourceString(IDS_ALL_REAL_USER));//_T("<all>")
		m_cComboUser.SetCurSel (0);
		m_cComboRealUser.SetCurSel (0);
		QueryUsers (listUsers);
		while (!listUsers.IsEmpty())
		{
			CString strUser = listUsers.RemoveHead();
			m_cComboUser.AddString (strUser);
			m_cComboRealUser.AddString (strUser);
		}

	}
	catch (CMemoryException* e)
	{
		VDBA_OutOfMemoryMessage();
		e->Delete();
	}
	return 0L;
}

LONG CuDlgDomPropInstallationLevelAuditLog::OnLoad (WPARAM wParam, LPARAM lParam)
{
	int nIdx;
	CString strItem;
	LPCTSTR pClass = (LPCTSTR)wParam;
	ASSERT (lstrcmp (pClass, _T("CuDomPropDataPageInstallationLevelAuditLog")) == 0);
	CuDomPropDataPageInstallationLevelAuditLog* pData = (CuDomPropDataPageInstallationLevelAuditLog*)lParam;
	ASSERT (pData);
	ResetDisplay();
	if (!pData)
		return 0;
	try
	{
		//
		// Event type:
		while (!pData->m_listEventType.IsEmpty())
		{
			strItem = pData->m_listEventType.RemoveHead();
			m_cComboEventType.AddString (strItem);
		}
		//
		// Database:
		while (!pData->m_listDatabase.IsEmpty())
		{
			strItem = pData->m_listDatabase.RemoveHead();
			m_cComboDatabase.AddString (strItem);
		}
		//
		// Object Type:
		while (!pData->m_listObjectType.IsEmpty())
		{
			strItem = pData->m_listObjectType.RemoveHead();
			m_cComboObjectType.AddString (strItem);
		}
		//
		// Object Name:
		m_cComboObjectName.AddString (VDBA_MfcResourceString(IDS_ALL_OBJ_NAME));//_T("<all>")
		while (!pData->m_listObjectName.IsEmpty())
		{
			CaDBObject* pObj = (CaDBObject*)pData->m_listObjectName.RemoveHead();
			nIdx = m_cComboObjectName.AddString (pObj->m_strItem);
			if (nIdx != CB_ERR)
				m_cComboObjectName.SetItemData (nIdx, (DWORD)pObj);
		}
		//
		// User:
		while (!pData->m_listUser.IsEmpty())
		{
			strItem = pData->m_listUser.RemoveHead();
			m_cComboUser.AddString (strItem);
		}
		//
		// Real User:
		while (!pData->m_listRealUser.IsEmpty())
		{
			strItem = pData->m_listRealUser.RemoveHead();
			m_cComboRealUser.AddString (strItem);
		}
		//
		// Real Begin:
		while (!pData->m_listBegin.IsEmpty())
		{
			strItem = pData->m_listBegin.RemoveHead();
			m_cComboBegin.AddString (strItem);
		}
		//
		// Real End:
		while (!pData->m_listEnd.IsEmpty())
		{
			strItem = pData->m_listEnd.RemoveHead();
			m_cComboEnd.AddString (strItem);
		}
		//
		// Combo:
		m_cComboEventType.SetCurSel (pData->m_nSelEventType);
		m_cComboDatabase.SetCurSel (pData->m_nSelDatabase);
		m_cComboObjectType.SetCurSel (pData->m_nSelObjectType);
		m_cComboObjectName.SetCurSel (pData->m_nSelObjectName);
		m_cComboUser.SetCurSel (pData->m_nSelUser);
		m_cComboRealUser.SetCurSel (pData->m_nSelRealUser);
		m_cComboBegin.SetWindowText (pData->m_strBegin);
		m_cComboEnd.SetWindowText (pData->m_strEnd);
		//
		// Row:
		if (m_pDlgRowLayout && IsWindow (m_pDlgRowLayout->m_hWnd))
		{
			m_pDlgRowLayout->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)_T("CuDomPropDataTableRows"), (LPARAM)pData->m_pRowData);
			m_pDlgRowLayout->ShowWindow (SW_SHOW);
		}
	}
	catch (CMemoryException* e)
	{
		VDBA_OutOfMemoryMessage();
		e->Delete();
	}
	return 0L;
}

LONG CuDlgDomPropInstallationLevelAuditLog::OnGetData (WPARAM wParam, LPARAM lParam)
{
	int i, nCount;
	CString strItem;
	CaDBObject* pObj = NULL;
	CuDomPropDataPageInstallationLevelAuditLog* pData = NULL;
	try
	{
		pData = new CuDomPropDataPageInstallationLevelAuditLog ();
		pData->m_nSelEventType = m_cComboEventType.GetCurSel();
		pData->m_nSelDatabase  = m_cComboDatabase.GetCurSel();
		pData->m_nSelObjectType= m_cComboObjectType.GetCurSel();
		pData->m_nSelObjectName= m_cComboObjectName.GetCurSel();
		pData->m_nSelUser      = m_cComboUser.GetCurSel();
		pData->m_nSelRealUser  = m_cComboRealUser.GetCurSel();

		m_cComboBegin.GetWindowText (pData->m_strBegin);
		m_cComboEnd.GetWindowText (pData->m_strEnd);

		//
		// Event type:
		nCount = m_cComboEventType.GetCount();
		for (i=0; i<nCount; i++)
		{
			m_cComboEventType.GetLBText (i, strItem);
			pData->m_listEventType.AddTail (strItem);
		}
		//
		// Database:
		nCount = m_cComboDatabase.GetCount();
		for (i=0; i<nCount; i++)
		{
			m_cComboDatabase.GetLBText (i, strItem);
			pData->m_listDatabase.AddTail (strItem);
		}
		//
		// Object Type:
		nCount = m_cComboObjectType.GetCount();
		for (i=0; i<nCount; i++)
		{
			m_cComboObjectType.GetLBText (i, strItem);
			pData->m_listObjectType.AddTail (strItem);
		}
		//
		// Object Name:
		nCount = m_cComboObjectName.GetCount();
		for (i=0; i<nCount; i++)
		{
			pObj = (CaDBObject*)m_cComboObjectName.GetItemData (i);
			if (pObj)
			{
				CaDBObject* pObjNew = new CaDBObject(pObj->m_strItem, pObj->m_strItemOwner);
				pData->m_listObjectName.AddTail (pObjNew);
			}
		}
		//
		// User:
		nCount = m_cComboUser.GetCount();
		for (i=0; i<nCount; i++)
		{
			m_cComboUser.GetLBText (i, strItem);
			pData->m_listUser.AddTail (strItem);
		}
		//
		// Real User:
		nCount = m_cComboRealUser.GetCount();
		for (i=0; i<nCount; i++)
		{
			m_cComboRealUser.GetLBText (i, strItem);
			pData->m_listRealUser.AddTail (strItem);
		}
		//
		// Real Begin:
		nCount = m_cComboBegin.GetCount();
		for (i=0; i<nCount; i++)
		{
			m_cComboBegin.GetLBText (i, strItem);
			pData->m_listBegin.AddTail (strItem);
		}
		//
		// Real End:
		nCount = m_cComboEnd.GetCount();
		for (i=0; i<nCount; i++)
		{
			m_cComboEnd.GetLBText (i, strItem);
			pData->m_listEnd.AddTail (strItem);
		}
		//
		// Row:
		if (m_pDlgRowLayout && IsWindow (m_pDlgRowLayout->m_hWnd))
		{
			pData->m_pRowData = (CuDomPropDataTableRows*)m_pDlgRowLayout->SendMessage (WM_USER_IPMPAGE_GETDATA, 0, 0);
		}
	}
	catch (CMemoryException* e)
	{
		VDBA_OutOfMemoryMessage();
		e->Delete();
	}
	return (LRESULT)pData;
}



BOOL CuDlgDomPropInstallationLevelAuditLog::QueryDatabase (CStringList& listDB)
{
	int   ires = RES_ERR;
	TCHAR buf       [MAXOBJECTNAME];
	TCHAR buffilter [MAXOBJECTNAME];
	CWaitCursor waitCursor;
	
	
	memset (buf, 0, sizeof (buf));
	memset (buffilter, 0, sizeof (buffilter));
	ires    = DOMGetFirstObject (m_nNodeHandle, OT_DATABASE, 0, NULL, TRUE, NULL, (LPUCHAR)buf, NULL, NULL);
	if (ires != RES_SUCCESS)
		return FALSE;
	listDB.RemoveAll();
	while (ires == RES_SUCCESS)
	{
		listDB.AddTail (buf);
		ires  = DOMGetNextObject ((LPUCHAR)buf, (LPUCHAR)buffilter, NULL);
	}
	return TRUE;
}

void CuDlgDomPropInstallationLevelAuditLog::OnSelchangeComboDatabase() 
{
	int nObjectType =-1;
	CString strDatabase;
	CString strObjectType;
	MaskResultLayout();

	CleanComboObjectName();
	m_cComboDatabase.GetWindowText (strDatabase);
	if (strDatabase.IsEmpty() || strDatabase.CompareNoCase (VDBA_MfcResourceString(IDS_ALL_DB)) == 0)//_T("<all>")
		return;
	m_cComboObjectType.GetWindowText (strObjectType);
	if (strObjectType.IsEmpty() || strObjectType.CompareNoCase (VDBA_MfcResourceString(IDS_ALL_OBJ_NAME)) == 0)//_T("<all>")
		return;
	else
	if (strObjectType.CompareNoCase (strDatabase) == 0)
		return;
	else
	{
		if (strObjectType.CompareNoCase (_T("application (role)")) == 0)
			nObjectType = OT_ROLE;
		else
		if (strObjectType.CompareNoCase (_T("procedure")) == 0)
			nObjectType = OT_PROCEDURE;
		else
		if (strObjectType.CompareNoCase (_T("table")) == 0)
			nObjectType = OT_TABLE;
		else
		if (strObjectType.CompareNoCase (_T("location")) == 0)
			nObjectType = OT_LOCATION;
		else
		if (strObjectType.CompareNoCase (_T("view")) == 0)
			nObjectType = OT_VIEW;
		else
		if (strObjectType.CompareNoCase (_T("security")) == 0)
			nObjectType = 0;
		else
		if (strObjectType.CompareNoCase (_T("user")) == 0)
			nObjectType = OT_USER;
		else
		if (strObjectType.CompareNoCase (_T("security alarm")) == 0)
			nObjectType = 0;
		else
		if (strObjectType.CompareNoCase (_T("rule")) == 0)
			nObjectType = OT_RULE;
		else
		if (strObjectType.CompareNoCase (_T("dbevent")) == 0)
			nObjectType = OT_DBEVENT;
		else
			nObjectType = -1;
	}

	if (!ComboObjectName_Requery(strDatabase, nObjectType))
		CleanComboObjectName();
}

void CuDlgDomPropInstallationLevelAuditLog::CleanComboObjectName()
{
	int i, nCount = m_cComboObjectName.GetCount();
	for (i=0; i<nCount; i++)
	{
		CaDBObject* lpObj = (CaDBObject*)m_cComboObjectName.GetItemData (i);
		if (lpObj)
			delete lpObj;
	}
	m_cComboObjectName.ResetContent();
	m_cComboObjectName.AddString (VDBA_MfcResourceString(IDS_ALL_OBJ_NAME));//_T("<all>")
	m_cComboObjectName.SetCurSel (0);
}

BOOL CuDlgDomPropInstallationLevelAuditLog::ComboObjectName_Requery(LPCTSTR lpszDatabase, int nObjectType)
{
	int idx, ires, nLevel = 0;
	LPUCHAR aparents [MAXPLEVEL];
	TCHAR   tchszBuf       [MAXOBJECTNAME];
	TCHAR   tchszOwner   [MAXOBJECTNAME];
	TCHAR   tchszBufComplim[MAXOBJECTNAME];
	LPTSTR  lpszOwner = NULL;
	BOOL    bSystem = GetSystemFlag();
	CString strDB;

	m_cComboDatabase.GetWindowText (strDB);
	if (strDB.CompareNoCase(VDBA_MfcResourceString(IDS_ALL_DB)) == 0)//_T("<all>")
		return FALSE;
	switch (nObjectType)
	{
	case OT_ROLE:
	case OT_LOCATION:
	case OT_USER:
		aparents[0] = aparents[1] = aparents[2] = NULL;
		break;
	case OT_PROCEDURE:
	case OT_TABLE:
	case OT_DBEVENT:
	case OT_VIEW:
		aparents[0] = (LPUCHAR)(LPCTSTR)strDB;
		nLevel = 1;
		break;
	case OT_RULE:
		//
		// No information to construct the level (= 2)
		ASSERT (FALSE);
		return FALSE;
	default:
		//
		// No OT_XX for these kinds of objects
		ASSERT (FALSE);
		return FALSE;
	}
	tchszBuf[0] = 0;
	tchszOwner[0] = 0;
	tchszBufComplim[0] = 0;

	ires =  DOMGetFirstObject(
		m_nNodeHandle,
		nObjectType,
		nLevel,
		aparents,
		bSystem,
		NULL,
		(LPUCHAR)tchszBuf,
		(LPUCHAR)tchszOwner,
		(LPUCHAR)tchszBufComplim);
	if (ires != RES_SUCCESS && ires != RES_ENDOFDATA) 
		return FALSE;

	while (ires == RES_SUCCESS)
	{
		idx = m_cComboObjectName.AddString (tchszBuf);
		if (idx != CB_ERR && tchszOwner[0])
		{
			CaDBObject* pObj = new CaDBObject(tchszBuf, tchszOwner);
			m_cComboObjectName.SetItemData (idx, (DWORD)pObj);
		}
		tchszBuf[0] = 0;
		tchszOwner[0] = 0;
		tchszBufComplim[0] = 0;
		ires = DOMGetNextObject ((LPUCHAR)tchszBuf, (LPUCHAR)tchszOwner, (LPUCHAR)tchszBufComplim);
	}
	return TRUE;
}

void CuDlgDomPropInstallationLevelAuditLog::OnDestroy() 
{
	CleanComboObjectName();
	CDialog::OnDestroy();
}

void CuDlgDomPropInstallationLevelAuditLog::OnSelchangeComboObjectType() 
{
	MaskResultLayout();
	OnSelchangeComboDatabase();
}

void CuDlgDomPropInstallationLevelAuditLog::OnButtonView() 
{
	if (!m_pDlgRowLayout)
		return;
	if (!IsWindow (m_pDlgRowLayout->m_hWnd))
		return;

	IPMUPDATEPARAMS ups;
	memset (&ups, 0, sizeof (ups));
	try
	{
		BOOL bUseCriteria = FALSE;
		CString strCriteria;
		CString strStatement;
		CString strWhere;
		CString strQuoteText;

		//strStatement = _T("SELECT AUDITTIME, AUDITEVENT, OBJECTTYPE, DATABASE, OBJECTNAME, OBJECTOWNER, ");
		//strStatement+= _T("USER_NAME, REAL_NAME, AUDITSTATUS, DESCRIPTION, USERPRIVILEGES, OBJPRIVILEGES ");
		//strStatement+= _T("FROM IIAUDIT ");
		strStatement = _T("SELECT AUDITTIME");
		//
		// Filter on 'AUDITEVENT':
		m_cComboEventType.GetWindowText (strCriteria);
		strCriteria.MakeUpper();
		if (strCriteria.CompareNoCase (VDBA_MfcResourceString(IDS_ALL_AUDIT_EVENT)) != 0)//_T("<all>")
		{
			strQuoteText.Format (_T("'%s'"), (LPCTSTR)strCriteria);
			if (!bUseCriteria)
				strWhere = _T(" WHERE ");
			strWhere += _T("AUDITEVENT = ");
			strWhere += strQuoteText;
			bUseCriteria = TRUE;
		}
		else
			strStatement += _T(", AUDITEVENT");
		//
		// Filter on 'OBJECTTYPE':
		m_cComboObjectType.GetWindowText (strCriteria);
		strCriteria.MakeUpper();
		if (strCriteria.CompareNoCase (VDBA_MfcResourceString(IDS_ALL_OBJECT_TYPE)) != 0)//_T("<all>")
		{
			strQuoteText.Format (_T("'%s'"), (LPCTSTR)strCriteria);
			if (!bUseCriteria)
				strWhere = _T("WHERE ");
			else
				strWhere += _T(" AND ");
			strWhere += _T("OBJECTTYPE = ");
			strWhere += strQuoteText;
			bUseCriteria = TRUE;
		}
		else
			strStatement += _T(", OBJECTTYPE");
		//
		// Filter on 'DATABASE':
		m_cComboDatabase.GetWindowText (strCriteria);
		if (strCriteria.CompareNoCase (VDBA_MfcResourceString(IDS_ALL_DB)) != 0)//_T("<all>")
		{
			strQuoteText.Format (_T("'%s'"), (LPCTSTR)strCriteria);
			if (!bUseCriteria)
				strWhere = _T("WHERE ");
			else
				strWhere += _T(" AND ");
			strWhere += _T("DATABASE = ");
			strWhere += strQuoteText;
			bUseCriteria = TRUE;
		}
		else
			strStatement += _T(", DATABASE");
		//
		// Filter on 'OBJECTNAME':
		m_cComboObjectName.GetWindowText (strCriteria);
		if (strCriteria.CompareNoCase (VDBA_MfcResourceString(IDS_ALL_OBJ_NAME)) != 0)//_T("<all>")
		{
			CaDBObject* pObj = NULL;
			int nSel = m_cComboObjectName.GetCurSel();
			if (nSel != CB_ERR)
				pObj = (CaDBObject*)m_cComboObjectName.GetItemData (nSel);

			strQuoteText.Format (_T("'%s'"), (LPCTSTR)strCriteria);
			if (!bUseCriteria)
				strWhere = _T("WHERE ");
			else
				strWhere += _T(" AND ");
			strWhere += _T("OBJECTNAME = ");
			strWhere += strQuoteText;
			if (pObj && !pObj->m_strItemOwner.IsEmpty())
			{
				strWhere += _T(" AND OBJECTOWNER = ");
				strQuoteText.Format (_T("'%s'"), (LPCTSTR)pObj->m_strItemOwner);
				strWhere += strQuoteText;
			}
			bUseCriteria = TRUE;
		}
		else
			strStatement += _T(", OBJECTNAME, OBJECTOWNER");
		//
		// Filter on 'USER':
		m_cComboUser.GetWindowText (strCriteria);
		if (strCriteria.CompareNoCase (VDBA_MfcResourceString(IDS_ALL_USER)) != 0)//_T("<all>")
		{
			strQuoteText.Format (_T("'%s'"), (LPCTSTR)strCriteria);
			if (!bUseCriteria)
				strWhere = _T("WHERE ");
			else
				strWhere += _T(" AND ");
			strWhere += _T("USER_NAME = ");
			strWhere += strQuoteText;
			bUseCriteria = TRUE;
		}
		else
			strStatement += _T(", USER_NAME");
		//
		// Filter on 'REAL_USER':
		m_cComboRealUser.GetWindowText (strCriteria);
		if (strCriteria.CompareNoCase (VDBA_MfcResourceString(IDS_ALL_REAL_USER)) != 0)//_T("<all>")
		{
			strQuoteText.Format (_T("'%s'"), (LPCTSTR)strCriteria);
			if (!bUseCriteria)
				strWhere = _T("WHERE ");
			else
				strWhere += _T(" AND ");
			strWhere += _T("REAL_NAME = ");
			strWhere += strQuoteText;
			bUseCriteria = TRUE;
		}
		else
			strStatement += _T(", REAL_NAME");
		//
		// Filter on 'BEGIN':
		m_cComboBegin.GetWindowText (strCriteria);
		strCriteria.TrimLeft();
		strCriteria.TrimRight();
		if (strCriteria.IsEmpty() || strCriteria.CompareNoCase (VDBA_MfcResourceString(IDS_BEGIN_FIRST)) != 0)//_T("<first>"
		{
			strQuoteText.Format (_T("date ('%s')"), (LPCTSTR)strCriteria);
			if (!bUseCriteria)
				strWhere = _T("WHERE ");
			else
				strWhere += _T(" AND ");
			strWhere += _T("AUDITTIME >= ");
			strWhere += strQuoteText;
			bUseCriteria = TRUE;
		}
		//
		// Filter on 'END':
		m_cComboEnd.GetWindowText (strCriteria);
		strCriteria.TrimLeft();
		strCriteria.TrimRight();
		if (strCriteria.IsEmpty() || strCriteria.CompareNoCase (VDBA_MfcResourceString(IDS_END_LAST)) != 0)//_T("<last>")
		{
			strQuoteText.Format (_T("date ('%s')"), (LPCTSTR)strCriteria);
			if (!bUseCriteria)
				strWhere = _T("WHERE ");
			else
				strWhere += _T(" AND ");
			strWhere += _T("AUDITTIME <= ");
			strWhere += strQuoteText;
			bUseCriteria = TRUE;
		}


		strStatement+= _T(", AUDITSTATUS, DESCRIPTION, USERPRIVILEGES, OBJPRIVILEGES FROM IIAUDIT ");
		strStatement += strWhere;
		//
		// Always 'iidbdb':
		m_pDlgRowLayout->SetComponent (_T("iidbdb"), strStatement);
		m_pDlgRowLayout->SendMessage (WM_USER_IPMPAGE_UPDATEING, (WPARAM)m_nNodeHandle, (LPARAM)&ups);
		m_pDlgRowLayout->ShowWindow (SW_SHOW);
	}
	catch (...)
	{
		CString strMsgError = _T("Internal error: CuDlgDomPropInstallationLevelAuditLog::OnButtonView\n");
		TRACE0 (strMsgError);
	}
}


BOOL CuDlgDomPropInstallationLevelAuditLog::QueryUsers (CStringList& listUsers)
{
	int ires;
	try
	{
		LPUCHAR aparentsTemp[MAXPLEVEL];
		TCHAR   buf[MAXOBJECTNAME];
		TCHAR   bufOwner[MAXOBJECTNAME];
		TCHAR   bufComplim[MAXOBJECTNAME];

		aparentsTemp[0] = aparentsTemp[1] = aparentsTemp[2] = NULL;
		memset (&bufComplim, '\0', sizeof(bufComplim));
		memset (&bufOwner, '\0', sizeof(bufOwner));

		ires =  DOMGetFirstObject(
			m_nNodeHandle,
			OT_USER,
			0,
			aparentsTemp,
			GetSystemFlag(),
			NULL,
			(LPUCHAR)buf,
			(LPUCHAR)bufOwner,
			(LPUCHAR)bufComplim);

		while (ires == RES_SUCCESS) 
		{
			if (lstrcmpi(buf, (LPCTSTR)lppublicsysstring) != 0)
				listUsers.AddTail ((LPCTSTR)buf);
			ires = DOMGetNextObject((LPUCHAR)buf, (LPUCHAR)bufOwner, (LPUCHAR)bufComplim);
		}
	}
	catch (CMemoryException* e)
	{
		VDBA_OutOfMemoryMessage();
		e->Delete();
		return FALSE;
	}
	catch (...)
	{
		TRACE0 ("Internal error: CuDlgDomPropInstallationLevelAuditLog::QueryUsers\n");
		return FALSE;
	}
	return TRUE;
}

void CuDlgDomPropInstallationLevelAuditLog::MaskResultLayout(BOOL bMask)
{
	if (m_pDlgRowLayout && IsWindow (m_pDlgRowLayout->m_hWnd))
	{
		m_pDlgRowLayout->ShowWindow (bMask? SW_HIDE: SW_SHOW);
	}
}

void CuDlgDomPropInstallationLevelAuditLog::OnSelchangeComboEventType() 
{
	MaskResultLayout();
}

void CuDlgDomPropInstallationLevelAuditLog::OnSelchangeComboObjectName() 
{
	MaskResultLayout();
}

void CuDlgDomPropInstallationLevelAuditLog::OnSelchangeComboUser() 
{
	MaskResultLayout();
}

void CuDlgDomPropInstallationLevelAuditLog::OnSelchangeComboRealUser() 
{
	MaskResultLayout();
}

void CuDlgDomPropInstallationLevelAuditLog::OnSelchangeComboBegin() 
{
	MaskResultLayout();
}

void CuDlgDomPropInstallationLevelAuditLog::OnSelchangeComboEnd() 
{
	MaskResultLayout();
}

void CuDlgDomPropInstallationLevelAuditLog::OnEditchangeComboBegin() 
{
	MaskResultLayout();
}

void CuDlgDomPropInstallationLevelAuditLog::OnEditchangeComboEnd() 
{
	MaskResultLayout();
}

void CuDlgDomPropInstallationLevelAuditLog::ResetDisplay()
{
	UpdateData (FALSE);   // Mandatory!

	m_cComboEnd.ResetContent();
	m_cComboBegin.ResetContent();
	m_cComboRealUser.ResetContent();
	m_cComboUser.ResetContent();
	m_cComboObjectName.ResetContent();
	m_cComboObjectType.ResetContent();
	m_cComboDatabase.ResetContent();
	m_cComboEventType.ResetContent();
	// Force immediate update of all children controls
	RedrawAllChildWindows(m_hWnd);
}