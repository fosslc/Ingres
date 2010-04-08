/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqleopar.cpp, Implementation File 
**    Project  : Ingres II/Vdba.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Wizard for generate the sql expression (Database Object page) 
**
**
** History:
** xx-Jun-1998 (uk$so01)
**    Created.
** 01-Mars-2000 (schph01)
**    bug 97680 : management of objects names including special characters
**    in Sql Assistant.
** 21-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 14-Fev-2002 (uk$so01)
**    SIR #106648, Fix memory leak.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "sqlepsht.h"
#include "sqleopar.h"
#include "edlssele.h"
#include "ingobdml.h" // Low level query objects


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgPropertyPageSqlExpressionDBObjectParams, CPropertyPage)

CuDlgPropertyPageSqlExpressionDBObjectParams::CuDlgPropertyPageSqlExpressionDBObjectParams() : CPropertyPage(CuDlgPropertyPageSqlExpressionDBObjectParams::IDD)
{
	//{{AFX_DATA_INIT(CuDlgPropertyPageSqlExpressionDBObjectParams)
	m_strFunctionName = _T("");
	m_strHelp1 = _T("");
	m_strHelp2 = _T("");
	m_strHelp3 = _T("");
	//}}AFX_DATA_INIT
}

CuDlgPropertyPageSqlExpressionDBObjectParams::~CuDlgPropertyPageSqlExpressionDBObjectParams()
{
}

void CuDlgPropertyPageSqlExpressionDBObjectParams::UpdateDisplay (LPGENFUNCPARAMS lpFparam)
{
	//
	// Function Name:
	if (lpFparam->FuncName[0])
		m_strFunctionName = lpFparam->FuncName;
	else
		m_strFunctionName = _T("");
	//
	// Help Text:
	if (lpFparam->HelpText1[0])
		m_strHelp1= lpFparam->HelpText1;
	else
		m_strHelp1= _T("");
	if (lpFparam->HelpText2[0])
		m_strHelp2= lpFparam->HelpText2;
	else
		m_strHelp2= _T("");
	if (lpFparam->HelpText3[0])
		m_strHelp3= lpFparam->HelpText3;
	else
		m_strHelp3 = _T("");
	FillListBoxObjects();
	UpdateData (FALSE);
}

void CuDlgPropertyPageSqlExpressionDBObjectParams::EnableWizardButtons()
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();


	int nCount = m_cListBox1.GetCount();
	if (nCount > 0)
	{
		if (m_cListBox1.GetCurSel() == LB_ERR)
			pParent->SetWizardButtons(PSWIZB_DISABLEDFINISH|PSWIZB_BACK);
		else
			pParent->SetWizardButtons(PSWIZB_FINISH|PSWIZB_BACK);
		return;
	}
	pParent->SetWizardButtons(PSWIZB_DISABLEDFINISH|PSWIZB_BACK);
}


void CuDlgPropertyPageSqlExpressionDBObjectParams::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgPropertyPageSqlExpressionDBObjectParams)
	DDX_Control(pDX, IDC_LIST1, m_cListBox1);
	DDX_Text(pDX, IDC_STATIC1, m_strFunctionName);
	DDX_Text(pDX, IDC_STATIC2, m_strHelp1);
	DDX_Text(pDX, IDC_STATIC3, m_strHelp2);
	DDX_Text(pDX, IDC_STATIC4, m_strHelp3);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgPropertyPageSqlExpressionDBObjectParams, CPropertyPage)
	//{{AFX_MSG_MAP(CuDlgPropertyPageSqlExpressionDBObjectParams)
	ON_LBN_SELCHANGE(IDC_LIST1, OnSelchangeList1)
	ON_WM_DESTROY()
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgPropertyPageSqlExpressionDBObjectParams message handlers

BOOL CuDlgPropertyPageSqlExpressionDBObjectParams::OnSetActive() 
{
	try
	{
		CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
		pParent->SetWizardButtons(PSWIZB_FINISH|PSWIZB_BACK);
		CuDlgPropertyPageSqlExpressionMain* pMainPage = &(pParent->m_PageMain);
		GENFUNCPARAMS fparam;
		CaSQLComponent* lpComp = (CaSQLComponent*)pMainPage->GetComponentCategory();
		if (lpComp)
		{
			memset (&fparam, 0, sizeof(fparam));
			FillFunctionParameters (&fparam, lpComp);
			UpdateDisplay (&fparam);
		}
		EnableWizardButtons();
		return CPropertyPage::OnSetActive();
	}
	catch (CMemoryException* e)
	{
		e->Delete();
		theApp.OutOfMemoryMessage();
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		//_T("Cannot query database objects");
		AfxMessageBox (IDS_MSG_FAIL_2_QUERY_OBJECT, MB_ICONEXCLAMATION|MB_OK);
	}
	return FALSE;
}


BOOL CuDlgPropertyPageSqlExpressionDBObjectParams::FillListBoxObjects()
{
	int idx, iOTtype = -1;
	CString strItem;
	CWaitCursor doWaitCursor;
	POSITION pos;
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	CuDlgPropertyPageSqlExpressionMain* pMainPage = &(pParent->m_PageMain);
	CaSQLComponent* lpComp = (CaSQLComponent*)pMainPage->GetComponentCategory();
	CaLLQueryInfo info(pParent->m_queryInfo);

	int nCount = m_cListBox1.GetCount();
	for (int i=0; i<nCount; i++)
	{
		CObject* pItemData = (CObject*)m_cListBox1.GetItemData(i);
		if (pItemData)
			delete pItemData;
	}
	m_cListBox1.ResetContent();
	if (!lpComp)
		return FALSE;

	switch (lpComp->GetID()) 
	{
	case F_DB_COLUMNS:
		pos = pParent->m_listStrColumn.GetHeadPosition();
		if (pos != NULL)
		{
			//
			// The columns are given:
			while (pos != NULL)
			{
				strItem = pParent->m_listStrColumn.GetNext (pos);
				m_cListBox1.AddString (strItem);
			}
			return TRUE;
		}
		else
		{
			//
			// 'pParent->m_listObject' contains the tables or views:
			int nTables = pParent->m_listObject.GetCount(); 
			//
			// Query columns of the specified tables:
			CaDBObject* pObj = NULL;
			pos = pParent->m_listObject.GetHeadPosition();
			while (pos != NULL)
			{
				pObj = pParent->m_listObject.GetNext (pos);
				iOTtype  = (pObj->GetObjectID() == OBT_TABLE)? OBT_TABLECOLUMN: OBT_VIEWCOLUMN;
				info.SetObjectType(iOTtype);
				//
				// info already contains Node, Server, Database, a,d we add the table name.
				info.SetItem2(pObj->GetName(), pObj->GetOwner());
				CTypedPtrList<CObList, CaDBObject*> listObject;
				BOOL bOk = theApp.INGRESII_QueryObject (&info, listObject);
				if (!bOk)
					return FALSE;

				POSITION p = listObject.GetHeadPosition();
				while (p != NULL)
				{
					CaDBObject* pCol = listObject.GetNext(p);
					if (nTables < 2)
						strItem = pCol->GetName();
					else
						strItem.Format (_T("%s.%s"), (LPCTSTR)pObj->GetName(), (LPCTSTR)pCol->GetName());

					idx = m_cListBox1.AddString (strItem);
					if (idx != LB_ERR)
					{
						CaSqlWizardDataField* pData = new CaSqlWizardDataField (CaSqlWizardDataField::COLUMN_NORMAL, pCol->GetName());
						pData->SetTable(pObj->GetName(), pObj->GetOwner(), pObj->GetObjectID());
						m_cListBox1.SetItemData(idx, (LPARAM)pData);
					}
					delete pCol;
				}
			}
			return TRUE;
		}
		return TRUE;

	case F_DB_TABLES:
		info.SetObjectType(OBT_TABLE);
		break;
	case F_DB_DATABASES:
		info.SetObjectType(OBT_DATABASE);
		info.SetDatabase(_T(""));
		break;
	case F_DB_USERS:
		info.SetObjectType(OBT_USER);
		info.SetDatabase(_T(""));
		break;
	case F_DB_GROUPS:
		info.SetObjectType(OBT_GROUP);
		info.SetDatabase(_T(""));
		break;
	case F_DB_ROLES:
		info.SetObjectType(OBT_ROLE);
		info.SetDatabase(_T(""));
		break;
	case F_DB_PROFILES:
		info.SetObjectType(OBT_PROFILE);
		info.SetDatabase(_T(""));
		break;
	case F_DB_LOCATIONS:
		info.SetObjectType(OBT_LOCATION);
		info.SetDatabase(_T(""));
		break;
	default:
		return FALSE;
	}

	CTypedPtrList<CObList, CaDBObject*> listObject;
	BOOL bOk = theApp.INGRESII_QueryObject (&info, listObject);
	if (!bOk)
		return FALSE;

	POSITION p = listObject.GetHeadPosition();
	while (p != NULL)
	{
		CaDBObject* pObj = listObject.GetNext(p);
		if (pObj->GetObjectID() == OBT_TABLE || pObj->GetObjectID() == OBT_VIEW)
		{
#if defined (_DISPLAY_OWNERxITEM)
			AfxFormatString2 (strItem, IDS_OWNERxITEM, (LPCTSTR)pObj->GetOwner(), (LPCTSTR)pObj->GetItem());
#else
			strItem = pObj->GetName();
#endif
		}
		else
			strItem = pObj->GetName();

		idx = m_cListBox1.AddString (strItem);
		if (idx != LB_ERR)
		{
			if (pObj->GetObjectID() == OBT_TABLE || pObj->GetObjectID() == OBT_VIEW)
			{
				m_cListBox1.SetItemData (idx, (LPARAM)pObj);
				pObj = NULL;
			}
			else
				m_cListBox1.SetItemData (idx, (LPARAM)0);
		}
		if (pObj)
			delete pObj;
	}
	return TRUE;
}


BOOL CuDlgPropertyPageSqlExpressionDBObjectParams::OnWizardFinish() 
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	CuDlgPropertyPageSqlExpressionMain* pMainPage = &(pParent->m_PageMain);
	GENFUNCPARAMS fparam;
	CaSQLComponent* lpComp = (CaSQLComponent*)pMainPage->GetComponentCategory();
	if (lpComp)
	{
		memset (&fparam, 0, sizeof(fparam));
		FillFunctionParameters (&fparam, lpComp);
		UpdateData (TRUE);
		int nSel = m_cListBox1.GetCurSel();
		if (nSel == LB_ERR)
			pParent->SetStatement (NULL);
		else
		{
			//
			// Database Object always has 1 argument:
			CString strStatement;
			CString strItem;
			m_cListBox1.GetText (nSel, strItem);
			if (lpComp->GetID() == F_DB_COLUMNS)
			{
				CaSqlWizardDataField* pData;
				pData =(CaSqlWizardDataField*) m_cListBox1.GetItemData (nSel);
				if (pData)
					strStatement.Format ((LPCTSTR)fparam.resultformat1, (LPCTSTR)pData->FormatString4SQL());
				else
				{
					//
					// strItem may have the [owner.] prefixed, so do not use INGRESII_llQuoteIfNeeded because
					// it will quote the whole string.
					strStatement.Format ((LPCTSTR)fparam.resultformat1, (LPCTSTR)strItem);
				}
			}
			else 
			if (lpComp->GetID() == F_DB_TABLES )
			{
				CaDBObject* pObject;
				pObject =(CaDBObject*) m_cListBox1.GetItemData (nSel);
				if (pObject)
				{
					CString strTemp;
					if (pObject->GetOwner().IsEmpty())
						strTemp = INGRESII_llQuoteIfNeeded(pObject->GetName()); // no owner in Object Name
					else
					{
						strTemp = INGRESII_llQuoteIfNeeded (pObject->GetOwner());
						strTemp += _T(".");
						strTemp += INGRESII_llQuoteIfNeeded(pObject->GetName());
					}
					strStatement.Format ((LPCTSTR)fparam.resultformat1, (LPCTSTR)strTemp);
				}
			}
			else
				strStatement.Format ((LPCTSTR)fparam.resultformat1, (LPCTSTR)INGRESII_llQuoteIfNeeded(strItem));
			pParent->SetStatement (strStatement);
		}
	}
	else
		pParent->SetStatement (NULL);
	return TRUE;
}

void CuDlgPropertyPageSqlExpressionDBObjectParams::OnSelchangeList1() 
{
	EnableWizardButtons();
}

BOOL CuDlgPropertyPageSqlExpressionDBObjectParams::OnKillActive() 
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	return CPropertyPage::OnKillActive();
}

void CuDlgPropertyPageSqlExpressionDBObjectParams::OnCancel() 
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	CPropertyPage::OnCancel();
}

void CuDlgPropertyPageSqlExpressionDBObjectParams::OnDestroy()
{
	int i,nCount = m_cListBox1.GetCount();
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	CuDlgPropertyPageSqlExpressionMain* pMainPage = &(pParent->m_PageMain);
	CaSQLComponent* lpComp = (CaSQLComponent*)pMainPage->GetComponentCategory();

	for (i=0; i<nCount; i++)
	{
		if ( lpComp->GetID() == F_DB_COLUMNS)
		{
			CaSqlWizardDataField* pObject = (CaSqlWizardDataField*)m_cListBox1.GetItemData (i);
			if (pObject)
				delete pObject;
			m_cListBox1.SetItemData (i, (DWORD)0);
		}
		else 
		{
			CaDBObject* pObject = (CaDBObject*)m_cListBox1.GetItemData (i);
			if (pObject)
				delete pObject;
			m_cListBox1.SetItemData (i, (DWORD)0);
		}
	}
	m_cListBox1.ResetContent();
	CPropertyPage::OnDestroy();
}

BOOL CuDlgPropertyPageSqlExpressionDBObjectParams::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	CuDlgPropertyPageSqlExpressionMain* pMainPage = &(pParent->m_PageMain);
	CaSQLComponent* lpComp = (CaSQLComponent*)pMainPage->GetComponentCategory();
	if (lpComp)
		return APP_HelpInfo(m_hWnd, lpComp->GetID());
	return FALSE;
}
