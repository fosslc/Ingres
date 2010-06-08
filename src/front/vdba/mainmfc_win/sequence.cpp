/****************************************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : sequence.cpp, implementation file
**
**
**    Project  : Ingres II / Visual DBA.
**    Author   : Schalk Philippe
**
**    Purpose  : implementation of the CxDlgDBSequence class.
**
** 26-Mar-2003 (schph01)
**    SIR #107523 created, Management to Create and Alter the Database Sequences.
** 07-Jun-2010 (drivi01)
**    Set sequence limit in the edit box.
****************************************************************************************/

#include "stdafx.h"
#include "mainmfc.h"
#include "sequence.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
extern "C"
{
#include "dbaginfo.h"
#include "msghandl.h"
}
/////////////////////////////////////////////////////////////////////////////
// interface to dom.c
extern "C" BOOL MfcDlgCreateSequence( LPSEQUENCEPARAMS lpsequencedb )
{
	CxDlgDBSequence dlg;
	dlg.m_StructSequenceInfo = lpsequencedb;
	int iret = dlg.DoModal();
	if (iret == IDOK)
		return TRUE;
	else
		return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// CxDlgDBSequence dialog


CxDlgDBSequence::CxDlgDBSequence(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgDBSequence::IDD, pParent)
{
	memset(&m_StructSeqBeforeChange,_T('\0'),sizeof (SEQUENCEPARAMS));
	//{{AFX_DATA_INIT(CxDlgDBSequence)
	m_SequenceName = _T("");
	m_bCycle = FALSE;
	m_bCache = FALSE;
	m_MaxValue = _T("");
	m_MinValue = _T("");
	m_csStartWith = _T("");
	m_csCacheSize = _T("");
	m_csIncrementBy = _T("");
	m_csDecimalPrecision = _T("");
	//}}AFX_DATA_INIT
}


void CxDlgDBSequence::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgDBSequence)
	DDX_Control(pDX, IDOK, m_ctrlOK);
	DDX_Control(pDX, IDC_EDIT_CACHE, m_ctrlEditCache);
	DDX_Control(pDX, IDC_CHECK_CACHE, m_ctrlCache);
	DDX_Control(pDX, IDC_EDIT_NAME, m_ctrlEditName);
	DDX_Control(pDX, IDC_STATIC_PREC, m_ctrlStaticPrecision);
	DDX_Control(pDX, IDC_STATIC_START_WITH, m_ctrlStartWith);
	DDX_Control(pDX, IDC_EDIT_DECIMAL, m_ctrlEditDecimal);
	DDX_Text(pDX, IDC_EDIT_NAME, m_SequenceName);
	DDX_Check(pDX, IDC_CHECK_CYCLE, m_bCycle);
	DDX_Check(pDX, IDC_CHECK_CACHE, m_bCache);
	DDX_Text(pDX, IDC_EDIT_MAX, m_MaxValue);
	DDX_Text(pDX, IDC_EDIT_MIN, m_MinValue);
	DDX_Text(pDX, IDC_EDIT_START_WITH, m_csStartWith);
	DDX_Text(pDX, IDC_EDIT_CACHE, m_csCacheSize);
	DDX_Text(pDX, IDC_EDIT_INCREMENT_BY, m_csIncrementBy);
	DDX_Text(pDX, IDC_EDIT_DECIMAL, m_csDecimalPrecision);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgDBSequence, CDialog)
	//{{AFX_MSG_MAP(CxDlgDBSequence)
	ON_BN_CLICKED(IDC_RADIO_DECIMAL, OnRadioDecimal)
	ON_BN_CLICKED(IDC_RADIO_INTEGER, OnRadioInteger)
	ON_BN_CLICKED(IDC_CHECK_CACHE, OnCheckCache)
	ON_EN_CHANGE(IDC_EDIT_NAME, OnChangeEditSequenceName)
	ON_EN_UPDATE(IDC_EDIT_NAME, OnUpdateEditSequenceName)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgDBSequence message handlers

BOOL CxDlgDBSequence::OnInitDialog() 
{
	CString csTitle,csFormat;
	CDialog::OnInitDialog();

	// Title 
	if ( m_StructSequenceInfo->bCreate )
	{
		ASSERT (m_StructSequenceInfo->DBName[0] != 0);
		// Update the title to 'Create Sequence'
		GetWindowText(csFormat);
		csTitle.Format( csFormat,
		                (char*)m_StructSequenceInfo->szVNode,
		                (char*)m_StructSequenceInfo->DBName);
		SetWindowText(csTitle);
		PushHelpId(HELPID_IDD_CREATEDB_SEQUENCE);
		m_ctrlEditName.SetLimitText(MAXOBJECTNAME-1);
	}
	else
	{
		ASSERT (m_StructSequenceInfo->DBName[0] != 0);
		// Change the title to 'Alter Sequence'
		csFormat.LoadString(IDS_T_ALTER_SEQUENCE);
		csTitle.Format( csFormat,
		                (char*)m_StructSequenceInfo->objectname,
		                (char*)m_StructSequenceInfo->szVNode,
		                (char*)m_StructSequenceInfo->DBName);
		SetWindowText(csTitle);
		Copy2SequenceParam (m_StructSequenceInfo, &m_StructSeqBeforeChange);
		PushHelpId(HELPID_ALTER_IDD_CREATEDB_SEQUENCE);
	}


	InitClassMember();
	OnChangeEditSequenceName();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgDBSequence::OnOK() 
{
	// Create database
	FillStructureWithCtrlInfo();

	if (!m_StructSequenceInfo->bCreate)
	{
		if (!AlterSequence())
			return;
		else
			EndDialog( TRUE);
	}
	else
	{
		int ires;
		// Create Sequence
		ires = DBAAddObjectLL (m_StructSequenceInfo->szVNode, OT_SEQUENCE, (void *) m_StructSequenceInfo );
		if (ires != RES_SUCCESS)
		{
			// Create database failed
			ErrorMessage ((UINT) IDS_E_CREATE_SEQUENCE_FAILED, ires);
			return;
		}
		else
			EndDialog( TRUE);
		
	}
	CDialog::OnOK();
}

void CxDlgDBSequence::OnRadioDecimal() 
{
	m_ctrlEditDecimal.EnableWindow(TRUE);
	m_ctrlStaticPrecision.EnableWindow(TRUE);
}

void CxDlgDBSequence::OnRadioInteger() 
{
	m_ctrlEditDecimal.EnableWindow(FALSE);
	m_ctrlStaticPrecision.EnableWindow(FALSE);
	
}

void CxDlgDBSequence::OnCheckCache() 
{
	if (m_ctrlCache.GetCheck() )
		m_ctrlEditCache.EnableWindow(TRUE);
	else
		m_ctrlEditCache.EnableWindow(FALSE);
}

BOOL CxDlgDBSequence::AlterSequence()
{
	BOOL Success = TRUE, bOverwrite = FALSE;

	SEQUENCEPARAMS seq2;
	SEQUENCEPARAMS seq3;
	int  ires;
	int  hdl;
	LPSEQUENCEPARAMS lpSeq1 = &m_StructSeqBeforeChange;
	BOOL bChangedbyOtherMessage = FALSE;
	int irestmp;
	memset (&seq2, 0, sizeof(SEQUENCEPARAMS));
	memset (&seq3, 0, sizeof(SEQUENCEPARAMS));

	_tcscpy ((LPTSTR)seq2.objectname, (LPCTSTR)lpSeq1->objectname);
	_tcscpy ((LPTSTR)seq3.objectname, (LPCTSTR)lpSeq1->objectname);
	_tcscpy ((LPTSTR)seq2.objectowner, (LPCTSTR)lpSeq1->objectowner);
	_tcscpy ((LPTSTR)seq3.objectowner, (LPCTSTR)lpSeq1->objectowner);
	_tcscpy ((LPTSTR)seq2.DBName, (LPCTSTR)lpSeq1->DBName);
	_tcscpy ((LPTSTR)seq3.DBName, (LPCTSTR)lpSeq1->DBName);
	Success = TRUE;
	ires    = GetDetailInfo ((LPUCHAR)(LPCTSTR)m_StructSequenceInfo->szVNode, OT_SEQUENCE, (void*)&seq2, TRUE, &hdl);
	if (ires != RES_SUCCESS)
	{
		ReleaseSession(hdl,RELEASE_ROLLBACK);
		switch (ires) 
		{
		case RES_ENDOFDATA:
			ires = ConfirmedMessage ((UINT)IDS_C_ALTER_CONFIRM_CREATE);
			if (ires == IDYES) 
			{
				//
				// Fill structure:
				Success = Copy2SequenceParam (m_StructSequenceInfo, &seq3);
				if (Success) 
				{
					ires = DBAAddObject ((LPUCHAR)(LPCTSTR)m_StructSequenceInfo->szVNode, OT_SEQUENCE, (void *)&seq3 );
					if (ires != RES_SUCCESS) 
					{
						ErrorMessage ((UINT) IDS_E_CREATE_SEQUENCE_FAILED, ires);// "Create Sequence Failed."
						Success=FALSE;
					}
				}
			}
			break;
		default:
			Success=FALSE;
			ErrorMessage ((UINT)IDS_E_ALTER_ACCESS_PROBLEM, RES_ERR);
			break;
		}
		FreeAttachedPointers ((void*)&seq2, OT_SEQUENCE);
		FreeAttachedPointers ((void*)&seq3, OT_SEQUENCE);
		return Success;
	}

	if (!AreObjectsIdentical ((void*)lpSeq1, (void*)&seq2, OT_SEQUENCE))
	{
		ReleaseSession(hdl, RELEASE_ROLLBACK);
		ires   = ConfirmedMessage ((UINT)IDS_C_ALTER_RESTART_LASTCHANGES);
		bChangedbyOtherMessage = TRUE;
		irestmp=ires;
		if (ires == IDYES) 
		{
			ires = GetDetailInfo ((LPUCHAR)(LPCTSTR)m_StructSequenceInfo->szVNode, OT_SEQUENCE, (void*)lpSeq1, FALSE, &hdl);
			if (ires != RES_SUCCESS) 
			{
				Success =FALSE;
				ErrorMessage ((UINT)IDS_E_ALTER_ACCESS_PROBLEM, RES_ERR);
			}
			else 
			{
				//
				// Fill controls:
				InitClassMember();
				FreeAttachedPointers ((void*)&seq2, OT_SEQUENCE);
				ReleaseSession(hdl,RELEASE_ROLLBACK);
				return FALSE;
			}
		}
		else
		{
			// Double Confirmation ?
			ires   = ConfirmedMessage ((UINT)IDS_C_ALTER_CONFIRM_OVERWRITE);
			if (ires != IDYES)
				Success=FALSE;
			else
			{
				TCHAR buf [MAXOBJECTNAME];
				if (GetDBNameFromObjType (OT_SEQUENCE,  buf, (LPTSTR)m_StructSequenceInfo->szVNode))
				{
					CString strSess;
					strSess.Format (_T("%s::%s"), (LPCTSTR)m_StructSequenceInfo->szVNode, (LPTSTR)buf);
					ires = Getsession ((LPUCHAR)(LPCTSTR)strSess, SESSION_TYPE_CACHEREADLOCK, &hdl);
					if (ires != RES_SUCCESS)
						Success = FALSE;
				}
			}
		}
		if (!Success)
		{
			FreeAttachedPointers ((void*)&seq2, OT_SEQUENCE);
			return Success;
		}
	}

	//
	// Fill structure:
	Success = Copy2SequenceParam (m_StructSequenceInfo, &seq3);
	if (Success) 
	{
		ires = DBAAlterObject ((void*)lpSeq1, (void*)&seq3, OT_SEQUENCE, hdl);
		if (ires != RES_SUCCESS)
		{
			Success=FALSE;
			ErrorMessage ((UINT) IDS_E_ALTER_SEQUENCE_FAILED, ires);
			ires = GetDetailInfo ((LPUCHAR)(LPCTSTR)m_StructSequenceInfo->szVNode, OT_SEQUENCE, (void*)&seq2, FALSE, &hdl);
			if (ires != RES_SUCCESS)
				ErrorMessage ((UINT)IDS_E_ALTER_ACCESS_PROBLEM, RES_ERR);
			else 
			{
				if (!AreObjectsIdentical ((void*)lpSeq1, (void*)&seq2, OT_SEQUENCE))
				{
					if (!bChangedbyOtherMessage) 
					{
						ires  = ConfirmedMessage ((UINT)IDS_C_ALTER_RESTART_LASTCHANGES);
					}
					else
						ires=irestmp;

					if (ires == IDYES) 
					{
						ires = GetDetailInfo ((LPUCHAR)(LPCTSTR)m_StructSequenceInfo->szVNode, OT_SEQUENCE, (void*)lpSeq1, FALSE, &hdl);
						if (ires == RES_SUCCESS)
						{
							//
							// Fill controls:
							InitClassMember();
							UpdateData(FALSE);
						}
					}
					else
						bOverwrite = TRUE;
				}
			}
		}
	}

	FreeAttachedPointers ((void*)&seq2, OT_SEQUENCE);
	FreeAttachedPointers ((void*)&seq3, OT_SEQUENCE);

	return Success;
}

void CxDlgDBSequence::FillStructureWithCtrlInfo()
{
	UpdateData(TRUE);

	if (m_MaxValue.IsEmpty())
		m_StructSequenceInfo->szMaxValue[0]=_T('\0');
	else
		_tcscpy((LPTSTR)m_StructSequenceInfo->szMaxValue,m_MaxValue);

	if (m_MinValue.IsEmpty())
		m_StructSequenceInfo->szMinValue[0]=_T('\0');
	else
		_tcscpy((LPTSTR)m_StructSequenceInfo->szMinValue,m_MinValue);

	if (m_csStartWith.IsEmpty())
		m_StructSequenceInfo->szStartWith[0]=_T('\0');
	else
		_tcscpy((LPTSTR)m_StructSequenceInfo->szStartWith,m_csStartWith);

	if (m_csIncrementBy.IsEmpty())
		m_StructSequenceInfo->szIncrementBy[0]=_T('\0');
	else
		_tcscpy((LPTSTR)m_StructSequenceInfo->szIncrementBy,m_csIncrementBy);

	if (m_ctrlCache.GetCheck() != BST_CHECKED)
		m_StructSequenceInfo->szCacheSize[0]=_T('\0');
	else
	{
		if (m_csCacheSize.IsEmpty())
			_tcscpy((LPTSTR)m_StructSequenceInfo->szCacheSize,_T(" "));
		else
			_tcscpy((LPTSTR)m_StructSequenceInfo->szCacheSize,m_csCacheSize);
	}
	if ( GetCheckedRadioButton(IDC_RADIO_INTEGER, IDC_RADIO_DECIMAL) == IDC_RADIO_DECIMAL)
	{
		m_StructSequenceInfo->bDecimalType = TRUE;
		if (m_csDecimalPrecision.IsEmpty())
			m_StructSequenceInfo->szDecimalPrecision[0]=_T('\0');
		else
			_tcscpy((LPTSTR)m_StructSequenceInfo->szDecimalPrecision,m_csDecimalPrecision);
	}
	else
	{
		m_StructSequenceInfo->bDecimalType = FALSE;
		m_StructSequenceInfo->szDecimalPrecision[0]=_T('\0');
	}

	m_StructSequenceInfo->bCycle = m_bCycle;

	_tcscpy((LPTSTR)(LPCTSTR)m_StructSequenceInfo->objectname, m_SequenceName);
}

void CxDlgDBSequence::InitClassMember()
{
	CString csTemp;
	if ( m_StructSequenceInfo->bCreate )
	{
		CheckRadioButton( IDC_RADIO_INTEGER, IDC_RADIO_DECIMAL, IDC_RADIO_INTEGER );
		OnRadioInteger();
		m_bCycle      = FALSE;
		m_bCache      = TRUE;
		m_csCacheSize = _T("20");
		m_ctrlEditCache.EnableWindow(TRUE);
	}
	else
	{
		m_SequenceName = m_StructSequenceInfo->objectname;
		m_ctrlEditName.EnableWindow(FALSE);
		m_csStartWith= m_StructSequenceInfo->szStartWith;
		m_csIncrementBy=m_StructSequenceInfo->szIncrementBy;
		m_MinValue= m_StructSequenceInfo->szMinValue;
		m_MaxValue= m_StructSequenceInfo->szMaxValue;
		
		csTemp.LoadString(IDS_STATIC_START_WITH);
		m_ctrlStartWith.SetWindowText(csTemp);
		// Check Integer or Decimal radio button
		if (!m_StructSequenceInfo->bDecimalType)
		{
			CheckRadioButton( IDC_RADIO_INTEGER, IDC_RADIO_DECIMAL, IDC_RADIO_INTEGER );
			OnRadioInteger();
		}
		else
		{
			CheckRadioButton( IDC_RADIO_INTEGER, IDC_RADIO_DECIMAL, IDC_RADIO_DECIMAL );
			OnRadioDecimal();
			m_csDecimalPrecision = m_StructSequenceInfo->szDecimalPrecision;
		}
		// Alter Sequence no change for data type
		GetDlgItem(IDC_RADIO_INTEGER)->EnableWindow(FALSE);
		GetDlgItem(IDC_RADIO_DECIMAL)->EnableWindow(FALSE);
		m_ctrlEditDecimal.EnableWindow(FALSE);
		if (_tcscmp((LPTSTR)m_StructSequenceInfo->szCacheSize,_T("0") ) != 0)
		{
			m_bCache = TRUE;
			m_ctrlEditCache.EnableWindow(TRUE);
			m_csCacheSize = m_StructSequenceInfo->szCacheSize;
		}
		else
		{
				m_ctrlEditCache.EnableWindow(FALSE);
				m_bCache = FALSE;
		}
		m_bCycle = m_StructSequenceInfo->bCycle;
	}
	UpdateData(FALSE);
}

BOOL CxDlgDBSequence::Copy2SequenceParam(LPSEQUENCEPARAMS lpSeqSrcParam, LPSEQUENCEPARAMS lpSeqDestParam)
{
	lpSeqDestParam->bDecimalType     = lpSeqSrcParam->bDecimalType;
	lpSeqDestParam->iseq_length      = lpSeqSrcParam->iseq_length;
	lpSeqDestParam->bCycle           = lpSeqSrcParam->bCycle;
	lpSeqDestParam->bOrder           = lpSeqSrcParam->bOrder;
	lpSeqDestParam->bCreate          = lpSeqSrcParam->bCreate;

	_tcscpy ( (LPTSTR)lpSeqDestParam->szVNode      , (LPTSTR)lpSeqSrcParam->szVNode);
	_tcscpy ( (LPTSTR)lpSeqDestParam->DBName       , (LPTSTR)lpSeqSrcParam->DBName  );
	_tcscpy ( (LPTSTR)lpSeqDestParam->objectname   , (LPTSTR)lpSeqSrcParam->objectname);
	_tcscpy ( (LPTSTR)lpSeqDestParam->objectowner  , (LPTSTR)lpSeqSrcParam->objectowner);
	_tcscpy ( (LPTSTR)lpSeqDestParam->szMaxValue   , (LPTSTR)lpSeqSrcParam->szMaxValue);
	_tcscpy ( (LPTSTR)lpSeqDestParam->szMinValue   , (LPTSTR)lpSeqSrcParam->szMinValue);
	_tcscpy ( (LPTSTR)lpSeqDestParam->szStartWith  , (LPTSTR)lpSeqSrcParam->szStartWith);
	_tcscpy ( (LPTSTR)lpSeqDestParam->szIncrementBy, (LPTSTR)lpSeqSrcParam->szIncrementBy);
	_tcscpy ( (LPTSTR)lpSeqDestParam->szNextValue  , (LPTSTR)lpSeqSrcParam->szNextValue);
	_tcscpy ( (LPTSTR)lpSeqDestParam->szCacheSize  , (LPTSTR)lpSeqSrcParam->szCacheSize);
	_tcscpy ( (LPTSTR)lpSeqDestParam->szDecimalPrecision , (LPTSTR)lpSeqSrcParam->szDecimalPrecision);

	return TRUE;
}

void CxDlgDBSequence::OnChangeEditSequenceName() 
{
	UpdateData(TRUE);
	if ( m_SequenceName.IsEmpty() )
		m_ctrlOK.EnableWindow(FALSE);
	else
		m_ctrlOK.EnableWindow(TRUE);
}

void CxDlgDBSequence::OnUpdateEditSequenceName() 
{
	UpdateData(TRUE);
	if ( m_SequenceName.IsEmpty() )
		m_ctrlOK.EnableWindow(FALSE);
	else
		m_ctrlOK.EnableWindow(TRUE);
}

void CxDlgDBSequence::OnDestroy() 
{
	CDialog::OnDestroy();
	PopHelpId();
}
