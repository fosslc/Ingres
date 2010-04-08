/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlerror.cpp : implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : Schalk Philippe (schph01)
**    Purpose  : CDlgSqlErrorHistory dialog
**
** History:
**
** 30-May-2002 (schph01)
**    Created
** 04-Jun-2004 (uk$so01)
**    SIR #111701, Connect Help to History of SQL Statement error.
** 10-Jun-2004 (schph01)
**    BUG #113003, change the implementation for gray/ungray the "Next",
**    "Prev" buttons.
*/
#include "stdafx.h"
#include "xipm.h"
#include "mainfrm.h"
#include "sqlerror.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CDlgSqlErrorHistory dialog


CDlgSqlErrorHistory::CDlgSqlErrorHistory(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgSqlErrorHistory::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgSqlErrorHistory)
	m_csErrorStatement = _T("");
	m_csErrorCode = _T("");
	m_csErrorText = _T("");
	m_bBegingList = FALSE;
	m_bEndList = FALSE;
	//}}AFX_DATA_INIT

	m_bMidleList          = FALSE;
	m_posPrevErrorView    = NULL;
	m_posDisplayErrorView = NULL;
	m_posNextErrorView    = NULL;

}


void CDlgSqlErrorHistory::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgSqlErrorHistory)
	DDX_Control(pDX, IDOK, m_ctrlOK);
	DDX_Control(pDX, IDC_SQLERR_PREV, m_ctrlPrev);
	DDX_Control(pDX, IDC_SQLERR_NEXT, m_ctrlNext);
	DDX_Control(pDX, IDC_SQLERR_LAST, m_ctrlLast);
	DDX_Control(pDX, IDC_SQLERR_FIRST, m_ctrlFirst);
	DDX_Text(pDX, IDC_SQLERR_STMT, m_csErrorStatement);
	DDX_Text(pDX, IDC_SQLERR_ERRCODE, m_csErrorCode);
	DDX_Text(pDX, IDC_SQLERR_ERRTXT, m_csErrorText);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgSqlErrorHistory, CDialog)
	//{{AFX_MSG_MAP(CDlgSqlErrorHistory)
	ON_BN_CLICKED(IDC_SQLERR_FIRST, OnSqlerrFirst)
	ON_BN_CLICKED(IDC_SQLERR_LAST, OnSqlerrLast)
	ON_BN_CLICKED(IDC_SQLERR_NEXT, OnSqlerrNext)
	ON_BN_CLICKED(IDC_SQLERR_PREV, OnSqlerrPrev)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_WM_HELPINFO()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgSqlErrorHistory message handlers

void CDlgSqlErrorHistory::OnSqlerrFirst() 
{
	CaSqlErrorInfo* SqlError;
	if (m_listSqlError.IsEmpty())
		return;
	m_posDisplayErrorView = m_listSqlError.GetHeadPosition();
	m_posNextErrorView = m_posDisplayErrorView;
	m_posPrevErrorView = m_posDisplayErrorView;

	SqlError = m_listSqlError.GetNext(m_posNextErrorView);
	RefreshDisplay(SqlError);
	m_bEndList    = FALSE;
	m_bBegingList = TRUE;
	m_bMidleList  = FALSE;
	SqlErrUpdButtonsStates();
}

void CDlgSqlErrorHistory::OnSqlerrLast() 
{
	CaSqlErrorInfo* SqlError;
	if (m_listSqlError.IsEmpty())
		return;

	m_posDisplayErrorView = m_listSqlError.GetTailPosition();
	m_posPrevErrorView    = m_posDisplayErrorView;
	m_posNextErrorView    = m_posDisplayErrorView;

	SqlError = m_listSqlError.GetPrev(m_posPrevErrorView);
	if (!m_posPrevErrorView)
		m_posPrevErrorView = m_posDisplayErrorView;
	RefreshDisplay(SqlError);
	m_bBegingList = FALSE;
	m_bEndList    = TRUE;
	m_bMidleList  = FALSE;

	SqlErrUpdButtonsStates();
}

void CDlgSqlErrorHistory::OnSqlerrNext() 
{
	CaSqlErrorInfo* SqlError;
	if (m_listSqlError.IsEmpty() || !m_posNextErrorView)
		return;
	m_posDisplayErrorView = m_posNextErrorView;
	m_posPrevErrorView    = m_posDisplayErrorView;
	m_listSqlError.GetPrev(m_posPrevErrorView);

	SqlError = m_listSqlError.GetNext(m_posNextErrorView);
	if (SqlError)
		RefreshDisplay(SqlError);
	if (!m_posNextErrorView)// End of List is reached
	{
		m_bBegingList = FALSE;
		m_bEndList    = TRUE;
		m_bMidleList  = FALSE;
		m_posNextErrorView = m_listSqlError.GetTailPosition();
	}
	else
		m_bMidleList = TRUE;

	SqlErrUpdButtonsStates();
}

void CDlgSqlErrorHistory::OnSqlerrPrev() 
{
	CaSqlErrorInfo* SqlError;
	if (m_listSqlError.IsEmpty() || !m_posPrevErrorView)
		return;
	m_posDisplayErrorView = m_posPrevErrorView;
	m_posNextErrorView = m_posDisplayErrorView;
	m_listSqlError.GetNext(m_posNextErrorView); 

	SqlError = m_listSqlError.GetPrev(m_posPrevErrorView);
	if (SqlError)
		RefreshDisplay(SqlError);
	if (!m_posPrevErrorView)// Begining of List is reached
	{
		m_bBegingList = TRUE;
		m_bEndList    = FALSE;
		m_bMidleList  = FALSE;
		m_posPrevErrorView = m_listSqlError.GetHeadPosition();
	}
	else
		m_bMidleList = TRUE;
	SqlErrUpdButtonsStates();
}

BOOL CDlgSqlErrorHistory::OnInitDialog() 
{
	int iRes;
	CString csMess;
	CDialog::OnInitDialog();
	iRes = m_FilesErrorClass->ReadSqlErrorFromFile(&m_listSqlError);

	if (iRes)
		//_T("Error while accessing the temporary file for the SQL Errors History.");
		AfxMessageBox(IDS_S_ACCESS_ERROR);

	if(m_listSqlError.IsEmpty())
		SqlErrUpdButtonsStates();
	else
		OnSqlerrLast(); // Display the last One

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgSqlErrorHistory::OnDestroy() 
{
	CDialog::OnDestroy();
	while (!m_listSqlError.IsEmpty())
		delete m_listSqlError.RemoveHead();
}

/////////////////////////////////////////////////////////////////////////////
// CaSqlErrorInfo message handlers

void CDlgSqlErrorHistory::RefreshDisplay(CaSqlErrorInfo *SqlErr)
{
	m_csErrorCode = SqlErr->GetSqlCode();
	m_csErrorText = SqlErr->GetErrorText();
	m_csErrorStatement = SqlErr->GetSqlStatement();

	UpdateData(FALSE);

}

int CDlgSqlErrorHistory::DoModal() 
{
	return CDialog::DoModal();
}

void CDlgSqlErrorHistory::SqlErrUpdButtonsStates()
{
	if (m_listSqlError.GetCount() > 1)
	{
		if (m_bBegingList)
		{
			if (m_bMidleList)
				m_ctrlPrev.EnableWindow(TRUE);
			else
				m_ctrlPrev.EnableWindow(FALSE);
			m_ctrlNext.EnableWindow(TRUE);
			m_ctrlFirst.EnableWindow(FALSE);
			m_ctrlLast.EnableWindow(TRUE);
		}
		else if (m_bEndList)
		{
			m_ctrlPrev.EnableWindow(TRUE);
			if (m_bMidleList)
				m_ctrlNext.EnableWindow(TRUE);
			else
				m_ctrlNext.EnableWindow(FALSE);
			m_ctrlFirst.EnableWindow(TRUE);
			m_ctrlLast.EnableWindow(FALSE);
		}
		else
		{
			m_ctrlPrev.EnableWindow(FALSE);
			m_ctrlNext.EnableWindow(FALSE);
			m_ctrlFirst.EnableWindow(FALSE);
			m_ctrlLast.EnableWindow(FALSE);
		}
	}
	else
	{
		m_ctrlPrev.EnableWindow(FALSE);
		m_ctrlNext.EnableWindow(FALSE);
		m_ctrlFirst.EnableWindow(FALSE);
		m_ctrlLast.EnableWindow(FALSE);
	}
	// post-manage focus lost (current focused button disabled)
	if (GetFocus() == NULL)
		m_ctrlOK.SetFocus();
}

BOOL CDlgSqlErrorHistory::OnHelpInfo(HELPINFO* pHelpInfo)
{
	UINT nHelpID = IDD_SQLERR;
	return APP_HelpInfo(m_hWnd, nHelpID);
}
