/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xdlgrule.cpp, Implementation file
**
**
**    Project  : Ingres II/VDBA
**    Author   : UK Sotheavut
**
**
**    Purpose  : Create Rule Dialog Box
**
**  History :
**  05-Mars-2001 (schph01)
**      BUG #103972
**      Add message map for the check box "insert" and "delete", for each
**      ON_BN_CLICKED launch the function EnableButtonOK().
**      Change the function EnableButtonOK() for verified if unless one of
**      "check box" is checked (Insert, Update, Delete).
**  26-Mar-2001 (noifr01)
**     (sir 104270) removal of code for managing Ingres/Desktop
** 14-Fev-2002 (uk$so01)
**    SIR #106648, Integrate SQL Assistant In-Process COM Server
** 13-Jun-2003 (uk$so01)
**    SIR #106648, Take into account the STAR database for connection.
** 17-Jul-2003 (uk$so01)
**    SIR #106648, Vdba-Split, ensure that the ingres sessions 
**    have the descriptions.
** 18-Apr-2005 (komve01)
**    BUG#114328/Issue#14092741
**    For creating a rule after update, if we uncheck "Specify Columns", we
**    should ensure that the columns are not checked, else we end up creating
**    an undesired rule.
*/

#include "stdafx.h"
#include "afxpriv.h"
#include "mainmfc.h"
#include "xdlgrule.h"
#include "sqlasinf.h" // sql assistant interface
#include "dmlbase.h"
#include "extccall.h"

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
#include "dba.h"
#include "dbaset.h"
#include "getdata.h"
#include "dlgres.h"
#include "dbadlg1.h"
#include "domdata.h"
#include "lexical.h"
#include "msghandl.h"
}


/////////////////////////////////////////////////////////////////////////////
// CxDlgRule dialog


CxDlgRule::CxDlgRule(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgRule::IDD, pParent)
{
	m_nNodeHandle = GetCurMdiNodeHandle();
	m_bSpecifiedColumnChecked = FALSE;
	m_pRuleParam = NULL;
	//{{AFX_DATA_INIT(CxDlgRule)
	m_bSpecifiedColumn = FALSE;
	m_bInsert = FALSE;
	m_bUpdate = FALSE;
	m_bDelete = FALSE;
	m_nProcedure = 0;
	m_strRule = _T("");
	m_strWhere = _T("");
	m_strProcParam = _T("");
	//}}AFX_DATA_INIT
}


void CxDlgRule::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgRule)
	DDX_Control(pDX, IDOK, m_cButtonOK);
	DDX_Control(pDX, IDC_MFC_EDIT1, m_cEditRuleName);
	DDX_Control(pDX, IDC_MFC_CHECK1, m_cCheckSpecifiedColumn);
	DDX_Control(pDX, IDC_MFC_COMBOPROC, m_cComboProcedure);
	DDX_Control(pDX, IDC_MFC_CHECK4, m_cCheckDelete);
	DDX_Control(pDX, IDC_MFC_CHECK3, m_cCheckUpdate);
	DDX_Control(pDX, IDC_MFC_CHECK2, m_cCheckInsert);
	DDX_Check(pDX, IDC_MFC_CHECK1, m_bSpecifiedColumn);
	DDX_Check(pDX, IDC_MFC_CHECK2, m_bInsert);
	DDX_Check(pDX, IDC_MFC_CHECK3, m_bUpdate);
	DDX_Check(pDX, IDC_MFC_CHECK4, m_bDelete);
	DDX_Text(pDX, IDC_MFC_EDIT1, m_strRule);
	DDX_Text(pDX, IDC_MFC_EDIT2, m_strWhere);
	DDX_Text(pDX, IDC_MFC_EDIT3, m_strProcParam);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgRule, CDialog)
	//{{AFX_MSG_MAP(CxDlgRule)
	ON_BN_CLICKED(IDC_MFC_BUTTON1, OnAssistant)
	ON_BN_CLICKED(IDC_MFC_CHECK1, OnCheckSpecifyColumns)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_MFC_CHECK3, OnCheckUpdate)
	ON_EN_CHANGE(IDC_MFC_EDIT1, OnChangeEditRuleName)
	ON_CBN_SELCHANGE(IDC_MFC_COMBOPROC, OnSelchangeComboproc)
	ON_BN_CLICKED(IDC_MFC_CHECK2, OnCheckInsert)
	ON_BN_CLICKED(IDC_MFC_CHECK4, OnCheckDelete)
	//}}AFX_MSG_MAP
	ON_CLBN_CHKCHANGE (IDC_MFC_LIST1, OnCheckChangeColumn)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgRule message handlers

BOOL CxDlgRule::OnInitDialog() 
{
	CDialog::OnInitDialog();
	ASSERT (m_pRuleParam && m_nNodeHandle != -1);
	if (!m_pRuleParam || m_nNodeHandle == -1)
		return FALSE;
	
	VERIFY (m_cCheckListBox.SubclassDlgItem (IDC_MFC_LIST1, this));
	if (!m_bSpecifiedColumnChecked)
	{
		m_cCheckSpecifiedColumn.EnableWindow (FALSE);
		m_cCheckListBox.EnableWindow (FALSE);
	}
	InitializeComboProcedures();
	InitializeTableColumns();
	CheckRadioButton (IDC_MFC_RADIO1, IDC_MFC_RADIO2, IDC_MFC_RADIO1);
	EnableButtonOK();

	//
	// Make up title:
	LPUCHAR parent [MAXPLEVEL];
	TCHAR buffer   [MAXOBJECTNAME];
	CString strFormat, strCap;
	strFormat.LoadString (IDS_T_CREATE_RULE);
	LPRULEPARAMS pRule = (LPRULEPARAMS)m_pRuleParam;
	parent [0] = pRule->DBName;
	parent [1] = NULL;
	GetExactDisplayString ((LPCTSTR)pRule->TableName, (LPCTSTR)pRule->TableOwner, OT_TABLE, parent, buffer);
	strCap.Format (strFormat, (LPTSTR)GetVirtNodeName(m_nNodeHandle), (LPTSTR)pRule->DBName, buffer);
	SetWindowText (strCap);
	m_cEditRuleName.SetLimitText( MAXOBJECTNAMENOSCHEMA-1 );

	PushHelpId ((UINT)IDD_RULE);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgRule::OnAssistant() 
{
	USES_CONVERSION;
	LPTSTR lpszVnodeName = (LPTSTR)GetVirtNodeName (m_nNodeHandle);
	ASSERT (lpszVnodeName);
	if (!lpszVnodeName)
		return;
	LPRULEPARAMS pRule = (LPRULEPARAMS)m_pRuleParam;
	//
	// Restricted features if gateway
	if (GetOIVers() == OIVERS_NOTOI) 
	{
		CString strMsg = VDBA_MfcResourceString(IDS_E_NODE);//_T("Not Available for this Node");
		AfxMessageBox (strMsg, MB_ICONEXCLAMATION | MB_OK);
		return;
	}
	try
	{
		SQLASSISTANTSTRUCT iiparam;
		memset(&iiparam, 0, sizeof(iiparam));
		wcscpy (iiparam.wczNode, T2W(lpszVnodeName));
		wcscpy (iiparam.wczDatabase, T2W((LPTSTR)pRule->DBName));
		wcscpy (iiparam.wczSessionDescription, L"Ingres Visual DBA invokes SQL Assistant");
		if (IsStarDatabase(GetCurMdiNodeHandle(), pRule->DBName))
			iiparam.nDbFlag = DBFLAG_STARNATIVE;
		iiparam.nSessionStart = 500;
		iiparam.nActivation = 2; // Wizard expression

		TOBJECTLIST* lpListTable = NULL;
		TOBJECTLIST obj;
		memset (&obj, 0, sizeof(obj));
		wcscpy (obj.wczObject, T2W((LPTSTR)Quote4DisplayIfNeeded((LPTSTR)pRule->TableName)));
		wcscpy (obj.wczObjectOwner, T2W((LPTSTR)Quote4DisplayIfNeeded((LPTSTR)pRule->TableOwner)));
		obj.nObjType = OBT_TABLE;
		lpListTable = TOBJECTLIST_Add (lpListTable, &obj);
		iiparam.pListTables = lpListTable;

		m_strWhere = SqlWizard(m_hWnd, &iiparam);

		m_strWhere.TrimLeft();
		m_strWhere.TrimRight();
		CEdit* pE = (CEdit*)GetDlgItem (IDC_MFC_EDIT2);
		if (pE)
			pE->SetWindowText(m_strWhere);
		lpListTable = TOBJECTLIST_Done (lpListTable);
	}
	catch (CMemoryException* e)
	{
		VDBA_OutOfMemoryMessage();
		e->Delete();
	}
	catch(...)
	{
		CString strMsg = VDBA_MfcResourceString(IDS_E_SQL_ASSISTANT);//_T("Cannot initialize the SQL assistant.");
		AfxMessageBox (strMsg);
	}
}

void CxDlgRule::OnCheckSpecifyColumns()
{
	m_bSpecifiedColumnChecked = (m_cCheckSpecifiedColumn.GetCheck() == 1)? TRUE: FALSE;
	m_cCheckListBox.EnableWindow (m_bSpecifiedColumnChecked);
	//komve01: If "specify columns" is unchecked, make sure that all the
	//columns are also unchecked, else we end up in creating a rule with 
	//the selected columns.
	if(!m_bSpecifiedColumnChecked)
	{
		int nCount = m_cCheckListBox.GetCount();
		for(int nLoop = 0;nLoop < nCount;nLoop++)
			m_cCheckListBox.SetCheck(nLoop,FALSE);
	}
	EnableButtonOK();
}

void CxDlgRule::OnOK()
{
	try
	{
		CString strTblNameWithOwner;
		int nStatement = 0;
		BOOL bOne = TRUE;
		CString strColumnList = _T("");
		CString strTableCodition = _T("AFTER ");
		UpdateData (TRUE);
		LPRULEPARAMS pRule = (LPRULEPARAMS)m_pRuleParam;
		LPTSTR lpszVnode = (LPTSTR)GetVirtNodeName (m_nNodeHandle);

		if (!IsObjectNameValid (m_strRule, OT_UNKNOWN))
		{
			AfxMessageBox( IDS_E_NOT_A_VALIDE_OBJECT, MB_ICONSTOP | MB_OK, 0 );
			m_cEditRuleName.SetSel( 0, -1,FALSE );
			m_cEditRuleName.SetFocus ();
			return;
		}
		lstrcpy ((LPTSTR)pRule->RuleName, m_strRule);
		//
		// Table Conndition;
		// AFTER INSERT, UPDATE (a, b, c), DELETE:
		if (m_bInsert)
		{
			strTableCodition += _T("INSERT ");
			bOne = FALSE;
			nStatement++;
		}
		if (m_bUpdate)
		{
			CString strColumn;
			BOOL bOneCol = TRUE;
			if (!bOne)
				strTableCodition += _T(", ");
			strTableCodition += _T("UPDATE ");
			int i, nCount = m_cCheckListBox.GetCount();
			for (i=0; i<nCount; i++)
			{
				if (m_cCheckListBox.GetCheck(i))
				{
					m_cCheckListBox.GetText (i, strColumn);
					if (!bOneCol)
						strColumnList += _T(", ");
					strColumnList += QuoteIfNeeded(strColumn);
					bOneCol = FALSE;
				}
			}
			if (!strColumnList.IsEmpty())
			{
				strTableCodition += _T("(");
				strTableCodition += strColumnList;
				strTableCodition += _T(") ");
			}
			bOne = FALSE;
			nStatement++;
		}
		if (m_bDelete)
		{
			if (!bOne)
				strTableCodition += _T(", ");
			strTableCodition += _T("Delete ");
			nStatement++;
		}
		//
		// ON | OF | FROM | INTO
		// Update -> ON | OF
		// Insert -> INTO
		// Delete -> FROM
		// If more than one stattements are specified, ON is choosen.
		if (nStatement > 1)
			strTableCodition += _T("ON ");
		else
		{
			if (m_bInsert)
				strTableCodition += _T("INTO ");
			else
			if (m_bUpdate)
				strTableCodition += strColumnList.IsEmpty()? _T("ON "): _T("OF ");
			else
				strTableCodition += _T("FROM ");
		}
		//
		// [Schema.]Table:
		strTableCodition += QuoteIfNeeded((LPCTSTR)pRule->TableOwner);
		strTableCodition += _T(".");
		strTableCodition += QuoteIfNeeded((LPCTSTR)pRule->TableName);
		strTableCodition += _T(" ");
		//
		// Where clause:
		m_strWhere.TrimLeft();
		m_strWhere.TrimRight();
		if (!m_strWhere.IsEmpty())
		{
			strTableCodition += _T("WHERE ");
			strTableCodition += m_strWhere;
			strTableCodition += _T(" ");
		}
		//
		// For each row|statement:
		int nEachRow = GetCheckedRadioButton (IDC_MFC_RADIO1, IDC_MFC_RADIO2); 
		if (nEachRow == IDC_MFC_RADIO1)
			strTableCodition += _T("FOR EACH ROW ");
		else
			strTableCodition += _T("FOR EACH STATEMENT ");

		pRule->lpTableCondition = (LPUCHAR)ESL_AllocMem (strTableCodition.GetLength() +1);
		if (!pRule->lpTableCondition)
		{
			VDBA_OutOfMemoryMessage();
			return;
		}
		lstrcpy ((LPTSTR)pRule->lpTableCondition, strTableCodition);
		m_nProcedure = m_cComboProcedure.GetCurSel();
		if (m_nProcedure != CB_ERR)
		{
			CaDBObject* pObject = (CaDBObject*)m_cComboProcedure.GetItemData(m_nProcedure);
			if (pObject)
			{
				m_strProcParam.TrimLeft();
				m_strProcParam.TrimRight();
				strTblNameWithOwner  = QuoteIfNeeded(pObject->m_strItemOwner);
				strTblNameWithOwner += _T(".");
				strTblNameWithOwner += QuoteIfNeeded(RemoveDisplayQuotesIfAny((LPCTSTR)StringWithoutOwner((LPUCHAR)(LPCTSTR)pObject->m_strItem)));
				pRule->lpProcedure = (LPUCHAR)ESL_AllocMem (strTblNameWithOwner.GetLength() + m_strProcParam.GetLength() + 2);
				if (pRule->lpProcedure)
				{
					lstrcpy ((LPTSTR)pRule->lpProcedure, strTblNameWithOwner);
					if (!m_strProcParam.IsEmpty())
						lstrcat ((LPTSTR)pRule->lpProcedure, m_strProcParam);
				}
				else
				{
					VDBA_OutOfMemoryMessage();
					return;
				}
			}
		}
		int nRes = DBAAddObject ((LPUCHAR)lpszVnode, OT_RULE, (void *)pRule);
		FreeAttachedPointers (pRule, OT_RULE);
		if (nRes != RES_SUCCESS)
		{
			ErrorMessage ((UINT) IDS_E_CREATE_RULE_FAILED, nRes);
			return;
		}
	}
	catch (CMemoryException* e)
	{
		VDBA_OutOfMemoryMessage();
		e->Delete();
		return;
	}
	catch(...)
	{
		AfxMessageBox( IDS_E_CREATE_RULE_FAILED, MB_ICONSTOP | MB_OK, 0 );
		return;
	}
	CDialog::OnOK();
}

BOOL CxDlgRule::InitializeComboProcedures()
{
	LPRULEPARAMS pRule = (LPRULEPARAMS)m_pRuleParam;
	int idx, ires, nLevel = 0;
	LPUCHAR aparents [MAXPLEVEL];
	TCHAR   tchszBuf       [MAXOBJECTNAME];
	TCHAR   tchszOwner   [MAXOBJECTNAME];
	TCHAR   tchszBufComplim[MAXOBJECTNAME];
	LPTSTR  lpszOwner = NULL;
	BOOL    bSystem = GetSystemFlag();
	
	aparents[0] = (LPUCHAR)(LPCTSTR)pRule->DBName;
	aparents[1] = NULL;
	nLevel = 1;
	tchszBuf[0] = 0;
	tchszOwner[0] = 0;
	tchszBufComplim[0] = 0;

	ires =  DOMGetFirstObject(
		m_nNodeHandle,
		OT_PROCEDURE,
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
		idx = m_cComboProcedure.AddString (tchszBuf);
		if (idx != CB_ERR && tchszOwner[0])
		{
			CaDBObject* pObj = new CaDBObject(tchszBuf, tchszOwner);
			m_cComboProcedure.SetItemData (idx, (DWORD)pObj);
		}
		tchszBuf[0] = 0;
		tchszOwner[0] = 0;
		tchszBufComplim[0] = 0;
		ires = DOMGetNextObject ((LPUCHAR)tchszBuf, (LPUCHAR)tchszOwner, (LPUCHAR)tchszBufComplim);
	}
	m_cComboProcedure.SetCurSel (-1);
	return TRUE;
}

BOOL CxDlgRule::InitializeTableColumns()
{
	TCHAR tblNameWithOwner[MAXOBJECTNAME];
	LPRULEPARAMS pRule = (LPRULEPARAMS)m_pRuleParam;
	int ires, nLevel = 2;
	LPUCHAR aparents [MAXPLEVEL];
	TCHAR   tchszBuf       [MAXOBJECTNAME];
	TCHAR   tchszOwner   [MAXOBJECTNAME];
	TCHAR   tchszBufComplim[MAXOBJECTNAME];
	LPTSTR  lpszOwner = NULL;
	BOOL    bSystem = GetSystemFlag();
	StringWithOwner((LPUCHAR)Quote4DisplayIfNeeded((LPTSTR)pRule->TableName), pRule->TableOwner, (LPUCHAR)tblNameWithOwner);
		
	aparents[0] = (LPUCHAR)(LPCTSTR)pRule->DBName;
	aparents[1] = (LPUCHAR)tblNameWithOwner;
	aparents[2] = NULL;
	nLevel = 2;
	tchszBuf[0] = 0;
	tchszOwner[0] = 0;
	tchszBufComplim[0] = 0;

	ires =  DOMGetFirstObject(
		m_nNodeHandle,
		OT_COLUMN,
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
		m_cCheckListBox.AddString (tchszBuf);
		tchszBuf[0] = 0;
		tchszOwner[0] = 0;
		tchszBufComplim[0] = 0;
		ires = DOMGetNextObject ((LPUCHAR)tchszBuf, (LPUCHAR)tchszOwner, (LPUCHAR)tchszBufComplim);
	}
	return TRUE;
}

void CxDlgRule::OnDestroy() 
{
	int i, nCount = m_cComboProcedure.GetCount();
	for (i=0; i<nCount; i++)
	{
		CaDBObject* pObj = (CaDBObject*)m_cComboProcedure.GetItemData(i);
		if (pObj)
			delete pObj;
	}
	CDialog::OnDestroy();
	PopHelpId();
}

void CxDlgRule::OnCheckUpdate()
{
	if (m_cCheckUpdate.GetCheck() == 1)
	{
		if (m_bSpecifiedColumnChecked)
		{
			m_cCheckSpecifiedColumn.EnableWindow (TRUE);
			m_cCheckListBox.EnableWindow (TRUE);
		}
		else
			m_cCheckSpecifiedColumn.EnableWindow (TRUE);
	}
	else
	{
		m_cCheckSpecifiedColumn.EnableWindow (FALSE);
		m_cCheckListBox.EnableWindow (FALSE);
	}
	EnableButtonOK();
}

void CxDlgRule::OnCheckInsert()
{
	EnableButtonOK();
}

void CxDlgRule::OnCheckDelete()
{
	EnableButtonOK();
}


void CxDlgRule::EnableButtonOK()
{
	BOOL B[3];
	int i, nCheck = 0, nCount, nSel = m_cComboProcedure.GetCurSel();
	nCount = m_cCheckListBox.GetCount();
	for (i=0; i<nCount; i++)
	{
		if (m_cCheckListBox.GetCheck(i))
		{
			nCheck = 1;
			break;
		}
	}
	B[0] = (m_bSpecifiedColumnChecked && nCheck == 0);
	B[1] = (m_cEditRuleName.GetWindowTextLength() > 0 && m_cComboProcedure.GetWindowTextLength() > 0);
	B[2] = (m_cCheckInsert.GetCheck() == 1 || m_cCheckUpdate.GetCheck() == 1 || m_cCheckDelete.GetCheck() == 1 );

	if  (!B[0] && B[1] && B[2])
		m_cButtonOK.EnableWindow (TRUE);
	else
		m_cButtonOK.EnableWindow (FALSE);
}

void CxDlgRule::OnChangeEditRuleName() 
{
	EnableButtonOK();
}

void CxDlgRule::OnSelchangeComboproc() 
{
	EnableButtonOK();
}

void CxDlgRule::OnCheckChangeColumn()
{
	EnableButtonOK();
}