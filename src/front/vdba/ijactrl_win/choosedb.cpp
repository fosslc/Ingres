/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : choosedb.cpp , Implementation File
**    Project  : Ingres Journal Analyser
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Implementation of Dialog Box to allow to choose "Node" and "Database".
**
** History:
**
** 03-Apr-2000 (uk$so01)
**    created
** 14-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management and put string into the resource files
**
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "choosedb.h"
#include "ijactdml.h"
#include "rcrdtool.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CxDlgChooseDatabase::CxDlgChooseDatabase(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgChooseDatabase::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgChooseDatabase)
	//}}AFX_DATA_INIT
	m_strNode = _T("");
	m_strDatabase = _T("");
}


void CxDlgChooseDatabase::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgChooseDatabase)
	DDX_Control(pDX, IDOK, m_cButtonOK);
	DDX_Control(pDX, IDC_COMBO2, m_cComboDatabase);
	DDX_Control(pDX, IDC_COMBO1, m_cComboNode);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgChooseDatabase, CDialog)
	//{{AFX_MSG_MAP(CxDlgChooseDatabase)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnSelchangeComboNode)
	ON_CBN_SELCHANGE(IDC_COMBO2, OnSelchangeComboDatabase)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgChooseDatabase message handlers

BOOL CxDlgChooseDatabase::OnInitDialog() 
{
	CDialog::OnInitDialog();

	try
	{
		CString strNode;
		POSITION pos = theApp.m_listNode.GetHeadPosition();
		while (pos != NULL)
		{
			strNode =  theApp.m_listNode.GetNext(pos);
			m_cComboNode.AddString(strNode);
		}

		//
		// Preselect node and database:
		int nSel = m_cComboNode.FindStringExact (-1, m_strNode);
		if (nSel != CB_ERR)
			m_cComboNode.SetCurSel (nSel);

		OnSelchangeComboNode();
		nSel = m_cComboDatabase.FindStringExact (-1, m_strDatabase);
		if (nSel != CB_ERR)
			m_cComboDatabase.SetCurSel (nSel);

		EnableButtons();
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason(), MB_OK|MB_ICONEXCLAMATION);
	}
	catch (...)
	{

	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgChooseDatabase::OnOK() 
{
	int nSel = m_cComboNode.GetCurSel();
	if (nSel != CB_ERR)
	{
		m_cComboNode.GetLBText (nSel, m_strNode);
	}
	
	nSel = m_cComboDatabase.GetCurSel();
	if (nSel != CB_ERR)
	{
		m_cComboDatabase.GetLBText (nSel, m_strDatabase);
	}

	CDialog::OnOK();
}

void CxDlgChooseDatabase::OnSelchangeComboNode() 
{
	CWaitCursor doWaitCursor;
	try
	{
		int nSel = m_cComboNode.GetCurSel();
		if (nSel == CB_ERR)
			return;
		m_cComboDatabase.ResetContent();
		CString strNode;
		m_cComboNode.GetLBText (nSel, strNode);
		FillComboDatabase (strNode);
		EnableButtons();
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason());
	}
	catch (...)
	{
		AfxMessageBox (_T("System error, CxDlgChooseDatabase::OnSelchangeComboNode, query database failed"));
	}
}

void CxDlgChooseDatabase::OnSelchangeComboDatabase() 
{
	EnableButtons();
}


void CxDlgChooseDatabase::FillComboDatabase(LPCTSTR lpszNode)
{
	CTypedPtrList<CObList, CaDatabase*> listDatabase;
	if (IJA_QueryDatabase (lpszNode, listDatabase))
	{
		POSITION pos = listDatabase.GetHeadPosition();
		while (pos != NULL)
		{
			CaDatabase* pDb = listDatabase.GetNext(pos);
			m_cComboDatabase.AddString(pDb->GetItem());
			delete pDb;
		}
	}
}

void CxDlgChooseDatabase::EnableButtons()
{
	CString strNode = _T("");
	CString strDatabase = _T("");
	int nSel = m_cComboNode.GetCurSel();
	if (nSel != CB_ERR)
	{
		m_cComboNode.GetLBText (nSel, strNode);
	}
	
	nSel = m_cComboDatabase.GetCurSel();
	if (nSel != CB_ERR)
	{
		m_cComboDatabase.GetLBText (nSel, strDatabase);
	}

	if (strNode.IsEmpty() || strDatabase.IsEmpty())
		m_cButtonOK.EnableWindow (FALSE);
	else
		m_cButtonOK.EnableWindow (TRUE);
}



BOOL CxDlgChooseDatabase::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	APP_HelpInfo (m_hWnd, IDD_XDATABASE);
	return TRUE;
}
