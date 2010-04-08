/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : iappage4.cpp : implementation file
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : STEP 4 of Import assistant
**
** History:
**
** 27-Oct-2000 (uk$so01)
**    Created
** 17-Oct-2001 (uk$so01)
**    SIR #103852, miss some behaviour.
** 19-Oct-2001 (uk$so01)
**    SIR #106103, Disable the radio in Step 4: Append... Replace...
**    if the table is currently empty. 
**    Correct the grammar in message of load/save (rc file)
** 24-Oct-2001 (uk$so01)
**    SIR #106103, Disable the radio in Step 4: Append... Replace...
**    if the table is created on the fly (so it is empty)
** 31-Oct-2001 (hanje04)
**    Replaced out of date 'CString& name=' constructs with CString name() as
**    they was causing problems on Linux.
** 21-Jan-2002 (uk$so01)
**    (bug 106844). Additional fix: Replace ifstream and some of CFile.by FILE*.
** 30-Jan-2002 (uk$so01)
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
** 03-Apr-2002 (uk$so01)
**    BUG #107488, Make svriia.dll enable to import empty .DBF file
**    It just create the table.
** 10-Apr-2002 (uk$so01)
**    BUG #107506, Quote automatically column name by using INGRESII_llQuote.
** 04-Jun-2002 (hanje04)
**    Define tchszReturn to be 0x0a 0x00 for MAINWIN builds to stop
**    unwanted ^M's in generated text files.
** 06-Feb-2003 (hanch04)
**    Don't do animation if built using MAINWIN
** 26-Feb-2003 (uk$so01)
**    SIR  #106952, Cleanup + make code homogeneous.
** 02-Feb-2004 (uk$so01)
**    SIR #106648, Vdba-Split, use a session independent to check if table is empty
**    because it hangs when importing with append mode
** 08-May-2008 (drivi01)
**    Apply wizard97 style to this wizard.
** 10-Mar-2010 (drivi01) SIR 123397
**    At the end of the import, suggest to users to run optimizedb or
**    generate statistics.
**/

#include "stdafx.h"
#include "resource.h"
#include "iappage4.h"
#include "iapsheet.h"
#include "rctools.h"
#include "sessimgr.h"
#include "usrexcep.h"
#include "ingobdml.h" // INGRESII Low level
#include "taskanim.h"
#include "dmlcsv.h"
extern "C"
{
#include "dll.h"
}



#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuPPage4, CPropertyPage)
CuPPage4::CuPPage4() : CPropertyPage(CuPPage4::IDD)
{
	//{{AFX_DATA_INIT(CuPPage4)
	//}}AFX_DATA_INIT
	m_bitmap.SetBitmpapId (IDB_IMPORTAS_1);
	m_bitmap.SetCenterVertical(TRUE);
	m_strNull = _T("''");
	m_bTableIsEmpty = TRUE;
	m_nAppendOrDelete = 0;

	m_hIcon = theApp.LoadIcon (IDI_ICON_SAVE);

	CString strStep;
	strStep.LoadString (IDS_IIA_STEP_4);
	m_strPageTitle.LoadString(AFX_IDS_APP_TITLE);
	m_strPageTitle += theApp.m_strInstallationID;
	m_strPageTitle += strStep;
	m_psp.dwFlags |= PSP_USETITLE;
	m_psp.pszTitle = (LPCTSTR)m_strPageTitle;

	m_psp.dwFlags |= PSP_DEFAULT|PSP_HIDEHEADER;
}

CuPPage4::~CuPPage4()
{
}

void CuPPage4::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuPPage4)
	DDX_Control(pDX, IDC_EDIT1, m_cEditProgressCommitStep);
	DDX_Control(pDX, IDC_CHECK3, m_cCheckProgressCommit);
	DDX_Control(pDX, IDC_BUTTON1, m_cButtonSave);
	DDX_Control(pDX, IDC_STATIC_EMPTYTABLE, m_cStaticEmptyTable);
	DDX_Control(pDX, IDC_STATIC1, m_cStatic1);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_STATIC_IMAGE1, m_bitmap);
}


BEGIN_MESSAGE_MAP(CuPPage4, CPropertyPage)
	//{{AFX_MSG_MAP(CuPPage4)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_RADIO1, OnRadioAppend)
	ON_BN_CLICKED(IDC_RADIO2, OnRadioReplace)
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonSaveImportParameter)
	ON_BN_CLICKED(IDC_CHECK3, OnCheckProgressCommit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



BOOL CuPPage4::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	m_cButtonSave.SetIcon(m_hIcon);
	try
	{
		//
		// Place the button save to fit the right margin:
		CRect r;
		m_cStatic1.GetWindowRect (r);
		ScreenToClient(r);
		int nRightMargin = r.right-1;
		m_cButtonSave.GetWindowRect (r);
		ScreenToClient(r);
		if (r.right > nRightMargin || r.right < nRightMargin)
		{
			int nNewPosX = nRightMargin - r.Width();
			m_cButtonSave.SetWindowPos (NULL, nNewPosX, r.top, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
		}
		CWnd* pStatic = GetDlgItem (IDC_STATIC2);
		if (pStatic && IsWindow (pStatic->m_hWnd))
		{
			m_cButtonSave.GetWindowRect (r);
			ScreenToClient(r);
			int nNewPosX = r.left - 4;
			
			pStatic->GetWindowRect (r);
			ScreenToClient(r);
			nNewPosX -= r.Width();
			pStatic->SetWindowPos (NULL, nNewPosX, r.top, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
		}

		CuIIAPropertySheet* pParent = (CuIIAPropertySheet*)GetParent();
		CaIIAInfo& data = pParent->GetData();
		CaDataPage1& dataPage1 = data.m_dataPage1;

		m_cStatic1.GetWindowRect (r);
		ScreenToClient (r);
		m_listCtrlHuge.CreateEx (WS_EX_CLIENTEDGE, NULL, NULL, WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS, r, this, IDC_LIST1);
		m_listCtrlHuge.MoveWindow (r);
		m_listCtrlHuge.ShowWindow (SW_SHOW);
		m_listCtrlHuge.SetFullRowSelected(TRUE);
		m_listCtrlHuge.SetLineSeperator(TRUE, TRUE, FALSE);
		m_listCtrlHuge.SetAllowSelect(FALSE);

		m_cEditProgressCommitStep.LimitText (6);
		m_cEditProgressCommitStep.SetWindowText (_T("1000"));
	}
	catch (...)
	{
		TRACE0("Raise exception at CuPPage4::OnInitDialog()\n");
		return FALSE;
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CuPPage4::OnSetActive() 
{
	CuIIAPropertySheet* pParent = (CuIIAPropertySheet*)GetParent();
	pParent->SetWizardButtons(PSWIZB_FINISH|PSWIZB_BACK);
	TRACE1 ("Previous page = %d\n", pParent->GetPreviousPage ());
	CString strMsg;

	try
	{
		int nHeader = 0;
		CaIIAInfo& data = pParent->GetData();
		CaDataPage1& dataPage1 = data.m_dataPage1;
		CaDataPage2& dataPage2 = data.m_dataPage2;
		CaDataPage3& dataPage3 = data.m_dataPage3;

		if (dataPage1.IsExistingTable())
		{
			m_cStaticEmptyTable.EnableWindow(TRUE);
			CaIeaTree& treeData = dataPage1.GetTreeData();
			CaLLQueryInfo info(-1, dataPage1.GetNode(), dataPage1.GetDatabase());
			info.SetItem2(dataPage1.GetTable(), dataPage1.GetTableOwner());
			info.SetServerClass(dataPage1.GetServerClass());
			info.SetIndependent(TRUE);

			CaSessionManager* pSessionManager = treeData.GetSessionManager();
			ASSERT (pSessionManager);
			if (!pSessionManager)
				return FALSE;
			CWnd* pRadioDelete = GetDlgItem (IDC_RADIO2);
			if (pRadioDelete)
				pRadioDelete->EnableWindow (TRUE);

			if (INGRESII_llIsTableEmpty(&info, pSessionManager))
			{
				// 
				// The table is currently empty
				strMsg.LoadString(IDS_EMPTY_TABLE);
				m_bTableIsEmpty = TRUE;
				CheckRadioButton (IDC_RADIO1, IDC_RADIO2, -1); 
				GetDlgItem(IDC_RADIO1)->EnableWindow(FALSE);
				GetDlgItem(IDC_RADIO2)->EnableWindow(FALSE);
			}
			else
			{
				GetDlgItem(IDC_RADIO1)->EnableWindow(TRUE);
				GetDlgItem(IDC_RADIO2)->EnableWindow(TRUE);
				// 
				// The table has existing data
				strMsg.LoadString(IDS_NOT_EMPTY_TABLE);
				m_bTableIsEmpty = FALSE;
				if (m_nAppendOrDelete == 0)
				{
					//
					// Default = "Append":
					CheckRadioButton (IDC_RADIO1, IDC_RADIO2, IDC_RADIO1); // Append
				}
			}
			m_cStaticEmptyTable.SetWindowText (strMsg);
		}
		else
		{
			// 
			// The table is currently empty
			strMsg.LoadString(IDS_EMPTY_TABLE);

			m_bTableIsEmpty = TRUE;
			m_cStaticEmptyTable.SetWindowText (strMsg);
			m_cStaticEmptyTable.EnableWindow(FALSE);

			CheckRadioButton (IDC_RADIO1, IDC_RADIO2, -1);
			GetDlgItem(IDC_RADIO1)->EnableWindow(FALSE);
			GetDlgItem(IDC_RADIO2)->EnableWindow(FALSE);
		}

		//
		// Initialize headers:
		// First, determine the number of columns to import.
		CaIngresHeaderRowItemData& commonItemData = dataPage3.GetUpperCtrlCommonData();
		int i, nSize = commonItemData.m_arrayColumnOrder.GetSize();
		nHeader = 0;
		for (i=1; i<nSize; i++)
		{
			CaColumn* pCol = (CaColumn*)commonItemData.m_arrayColumnOrder.GetAt(i);
			if (pCol)
				nHeader++;
		}
		//
		// At least one column !
		ASSERT (nHeader > 0);
		if (nHeader == 0)
			return FALSE;

		LSCTRLHEADERPARAMS* pLsp = new LSCTRLHEADERPARAMS[nHeader];
		int nEffectveCol = nHeader;
		nHeader = 0;
		for (i=1; i<nSize; i++)
		{
			CaColumn* pCol = (CaColumn*)commonItemData.m_arrayColumnOrder.GetAt(i);
			if (pCol && nHeader < nEffectveCol)
			{
				pLsp[nHeader].m_fmt = LVCFMT_LEFT;

				if (pCol->GetName().GetLength() > MAXLSCTRL_HEADERLENGTH)
					lstrcpyn (pLsp[nHeader].m_tchszHeader, pCol->GetName(), MAXLSCTRL_HEADERLENGTH);
				else
					lstrcpy  (pLsp[nHeader].m_tchszHeader, pCol->GetName());

				pLsp[nHeader].m_cxWidth= 100;
				pLsp[nHeader].m_bUseCheckMark = FALSE;
				nHeader++;
			}
		}
		m_listCtrlHuge.InitializeHeader(pLsp, nHeader);
		m_listCtrlHuge.SetDisplayFunction(DisplayListCtrlHugeItem, (void*)&commonItemData);
		delete pLsp;
		//
		// Display the new records:
		CPtrArray& arrayRecord = dataPage2.GetArrayRecord();
		m_listCtrlHuge.SetSharedData(&arrayRecord);
		m_listCtrlHuge.ResizeToFit();
		Loading();
	}
	catch(CeSqlException e)
	{
		AfxMessageBox(e.GetReason());
		return FALSE;
	}
	catch(...)
	{
		TRACE0("Raise exception at CuPPage4::OnSetActive()\n");
		return FALSE;
	}

	return CPropertyPage::OnSetActive();
}

BOOL CuPPage4::OnWizardFinish() 
{
	CString strMsg;
	CString strMsg2;

	try
	{
		CWaitCursor doWaitCursor;
		CuIIAPropertySheet* pParent = (CuIIAPropertySheet*)GetParent();
		CaIIAInfo& data = pParent->GetData();
		CaDataPage1& dataPage1 = data.m_dataPage1;
		CaDataPage3& dataPage3 = data.m_dataPage3;
		if (dataPage3.GetCreateTableOnly() && !dataPage1.IsExistingTable())
		{
			CaLLAddAlterDrop Aad(dataPage1.GetNode(), dataPage1.GetDatabase(), dataPage3.GetStatement());
			Aad.SetServerClass (dataPage1.GetServerClass());
			CmtSessionManager& smgr = theApp.GetSessionManager();
			Aad.ExecuteStatement(&smgr);
			AfxMessageBox (IDS_MSG_CREATE_TABLE_OK);
		}
		else
		{
			//
			// COPY
			if (!ImportByCopy(&data))
				return FALSE;
			AfxMessageBox(IDS_MSG_SUGGEST_OPTIMIZEDB, MB_OK);

		}
		return CPropertyPage::OnWizardFinish();
	}
	catch(CeSqlException e)
	{
		strMsg = e.GetReason();
	}
	catch (CFileException* e)
	{
		CString strErr;
		e->GetErrorMessage(strErr.GetBuffer(_MAX_PATH), _MAX_PATH);
		strErr.ReleaseBuffer();
		strMsg = strErr;
		e->Delete();
	}
	catch (int nException)
	{
		strMsg = MSG_DetectFileError(nException);
	}
	catch(...)
	{
		TRACE0("Raise exception at CuPPage4::OnWizardFinish()\n");
		strMsg = _T("System error (CuPPage4::OnWizardFinish).");
	}
	
	if (!strMsg.IsEmpty())
	{
		//
		// The Import operation has been cancelled.
		strMsg2.LoadString (IDS_IMPORT_OPERATION_CANCELLED);
		strMsg += strMsg2;
		AfxMessageBox(strMsg, MB_OK|MB_ICONSTOP);
	}

	CaUserData4GetRow* pUserData4CopyHander = theApp.GetUserData4GetRow();
	if (pUserData4CopyHander)
		pUserData4CopyHander->Cleanup();

	return FALSE;
}

BOOL CuPPage4::OnKillActive() 
{
	CuIIAPropertySheet* pParent = (CuIIAPropertySheet*)GetParent();
	pParent->SetPreviousPage (4);
	
	return CPropertyPage::OnKillActive();
}


BOOL CuPPage4::ImportByCopy(CaIIAInfo* pData)
{
	CString strMsg;
	CString strSyntax;
	CuIIAPropertySheet* pParent = (CuIIAPropertySheet*)GetParent();
	CaDataPage1& dataPage1 = pData->m_dataPage1;
	CaDataPage2& dataPage2 = pData->m_dataPage2;
	CaDataPage3& dataPage3 = pData->m_dataPage3;

	CTypedPtrList< CObList, CaDBObject* >& listColumn = dataPage1.GetTableColumns();
	BOOL bCanCopyUseFileDirectly = TRUE;

	//
	// If the ingres table contain the column with 'char' as data type AND
	// the correspondance column value in the file has a trailing space then
	// we must use the temporary file because we remove the trailing space.
	CaIngresHeaderRowItemData& commonItemData = dataPage3.GetUpperCtrlCommonData();
	CTypedPtrArray < CPtrArray, CaColumn* >& arrayOrderColumn = commonItemData.m_arrayColumnOrder;
	int i, nSize = arrayOrderColumn.GetSize();
	BOOL bCharSpace = FALSE; // Ingres table contain the column with 'char' and data with trailing spaces
	//
	// Start from 1 because of the <dummy column> at position 0.
	for (i=1; i<nSize; i++)
	{
		CaColumn* pCol = arrayOrderColumn.GetAt(i);
		if (!pCol)
			continue;
		if (pCol->GetTypeStr().CompareNoCase(_T("char")) == 0 && dataPage2.IsTrailingSpace(i-1))
		{
			bCharSpace = TRUE;
			break;
		}
	}

	if (bCharSpace)
	{
		bCanCopyUseFileDirectly = FALSE;
	}
	else
	{
		if (dataPage2.GetCurrentSeparatorType() == FMT_FIELDSEPARATOR)
		{
			CaSeparatorSet* pSet = NULL;
			CaSeparator* pSep = NULL;
			dataPage2.GetCurrentFormat (pSet, pSep);
			bCanCopyUseFileDirectly = CanCopyUseFileDirectly(pSet, pSep, listColumn);
		}
		else
		if (dataPage2.GetCurrentSeparatorType() == FMT_FIXEDWIDTH)
		{
			bCanCopyUseFileDirectly = TRUE;
			CArray <int, int>& arrayDividerPos = dataPage2.GetArrayDividerPos();
			int i, nSize = arrayDividerPos.GetSize();
			int nMenLen = dataPage1.GetMinRecordLength();
			for (i=0; i<nSize; i++)
			{
				if (nMenLen <= arrayDividerPos.GetAt(i))
				{
					bCanCopyUseFileDirectly = FALSE;
					break;
				}
			}
		}
		else
		if (dataPage2.GetCurrentSeparatorType() == FMT_DBF)
		{
			//
			// We need the temporary file for the DBF format.
			bCanCopyUseFileDirectly = FALSE;
		}
		else
		{
			//
			// TODO: Add another type ?
			TRACE0 ("TODO (CuPPage4::ImportByCopy): Add another type ?\n");
			ASSERT (FALSE);
		}
	}
	
	TRACE1 ("CuPPage4::ImportByCopy(), use file directly (0=no, 1=yes): %d\n", bCanCopyUseFileDirectly);
	if (bCanCopyUseFileDirectly)
	{
		//
		// Generate the COPY syntax:
		CString strFile(dataPage1.GetFile2BeImported());
		if (dataPage2.GetCurrentSeparatorType() == FMT_FIELDSEPARATOR)
		{
			CaSeparatorSet* pSet = NULL;
			CaSeparator* pSep = NULL;
			dataPage2.GetCurrentFormat (pSet, pSep);

			CString strSeparator = _T("");
			if (pSep)
			{
				strSeparator = pSep->GetSeparator();
				//
				// Generate COPY syntax with read as CHAR(0) format:
				strSyntax = GenerateCopySyntax(strSeparator[0], strFile);
			}
			else
			{
				//
				// Special case (each line is considered as single column):
				strSyntax = GenerateCopySyntax(_T('\0'), strFile, 1);
			}
		}
		else
		if (dataPage2.GetCurrentSeparatorType() == FMT_FIXEDWIDTH)
		{
			//
			// Generate COPY syntax with read as CHAR(n) format:
			strSyntax = GenerateCopySyntax(_T('\0'), strFile, 0);
		}
	}
	else
	{
		strSyntax = GenerateCopySyntax(_T('\0'), NULL, 2);
	}
	
	int nMaxSizeToRead = 0;
	if (dataPage2.GetCurrentSeparatorType() == FMT_DBF)
		nMaxSizeToRead = dataPage2.GetDBFTheoricRecordSize();
	else
		nMaxSizeToRead = (dataPage1.GetKBToScan() > 0)? 1024*dataPage1.GetFileSize(): dataPage1.GetArrayTextLine().GetSize();
	ASSERT (nMaxSizeToRead > 0);
	CaUserData4GetRow* pUserData4CopyHander = theApp.GetUserData4GetRow();
	ASSERT (pUserData4CopyHander);
	pUserData4CopyHander->Cleanup();
	pUserData4CopyHander->SetUseFileDirectly(bCanCopyUseFileDirectly);
	pUserData4CopyHander->SetIIAData(pData);
	pUserData4CopyHander->SetCurrentRecord(0);
	pUserData4CopyHander->SetTotalSize(nMaxSizeToRead);
	if (m_cCheckProgressCommit.GetCheck() == 1)
	{
		int nStep = 0;
		CString strItem;
		m_cEditProgressCommitStep.GetWindowText(strItem);
		nStep = _ttoi(strItem);
		pUserData4CopyHander->SetCommitStep (nStep);
	}
	else
	{
		pUserData4CopyHander->SetCommitStep (0);
	}

	//
	// Execute the COPY Statement:
	CaExecParamSQLCopy* p = new CaExecParamSQLCopy();
	CaLLAddAlterDrop Aad(dataPage1.GetNode(), dataPage1.GetDatabase(), strSyntax);
	Aad.SetServerClass (dataPage1.GetServerClass());

	//
	// NEED to create a new table ?
	if (!dataPage1.IsExistingTable())
	{
		CString& strStatement = dataPage3.GetStatement();
		p->SetNewTable(strStatement);
	}
	//
	// Delete Old Rows ?
	int nSelected = GetCheckedRadioButton (IDC_RADIO1, IDC_RADIO2);
	if (dataPage1.IsExistingTable() && !m_bTableIsEmpty && nSelected == IDC_RADIO2)
	{
		CString strDeleteSyntax = _T("");
		strDeleteSyntax.Format(
			_T("delete from %s.%s"), 
			(LPCTSTR)INGRESII_llQuoteIfNeeded(dataPage1.GetTableOwner()), 
			(LPCTSTR)INGRESII_llQuoteIfNeeded(dataPage1.GetTable()));
		p->SetDeleteRow(strDeleteSyntax);
	}
	
	p->SetAadParam(&Aad);
#if defined (_ANIMATION_)
	CString strMsgAnimateTitle;
	strMsgAnimateTitle.LoadString(IDS_ANIMATE_TITLE_IMPORTING);
	CxDlgWait dlg (strMsgAnimateTitle);
	dlg.SetUseAnimateAvi(AVI_FETCHF);
	dlg.SetExecParam (p);
	dlg.SetDeleteParam(FALSE);
	dlg.SetShowProgressBar(TRUE);
	dlg.SetUseExtraInfo();
	dlg.DoModal();
#else
	p->Run();
	p->OnTerminate(m_hWnd);
#endif
	BOOL bSuccess = p->IsSuccess();
	pUserData4CopyHander->Cleanup();
	delete p;
	if (!bSuccess)
		SetForegroundWindow ();
	return bSuccess;
}



void CuPPage4::OnDestroy() 
{
	if (m_hIcon)
		DestroyIcon(m_hIcon);
	m_hIcon = NULL;
	CPropertyPage::OnDestroy();
}

BOOL CuPPage4::CanCopyUseFileDirectly (
	CaSeparatorSet* pSet, 
	CaSeparator* pSep, 
	CTypedPtrList< CObList, CaDBObject* >& listColumn)
{
	CuIIAPropertySheet* pParent = (CuIIAPropertySheet*)GetParent();
	CaIIAInfo& data = pParent->GetData();
	CaDataPage1& dataPage1 = data.m_dataPage1;
	CaDataPage2& dataPage2 = data.m_dataPage2;
	//
	// COPY can use the imported file directly if the following conditions ara met:
	// 0) Special case.
	// 1) Not interprete consecutive separators as one.
	// 2) File does not contain text qualifier.
	//    COPY does not remove text qualifiers. 
	//    Ex: If a column "JKL,LMP" is in the file, then COPY inserts "JKL,LMP" in the table column.
	// 3) The scan process does not skip lines.
	// 4) File has single character (non-numeric) as separator.
	// 5) Not ignore trailing separators.
	// 6) Fixed width format that has no line where its len < position of divider.

	// Condition 1:
	if (pSet->GetConsecutiveSeparatorsAsOne())
		return FALSE;
	// Condition 2:
	if (pSet->GetTextQualifier() != _T('\0'))
		return FALSE;
	// Condition 3:
	if (dataPage1.GetLineCountToSkip() != 0)
		return FALSE;
	//
	// Condition 0) Handle the special case (each line is considered as single column):
	if (!pSep)
		return TRUE;

	// Condition 4:
	CString srtSep = pSep->GetSeparator();
	if (srtSep.GetLength() != 1)
		return FALSE;
	if (_istdigit(srtSep[0]))
		return FALSE;
	if (srtSep[0] == _T(' '))
		return FALSE; // As we use the format char(0) delimiter, space cannot be a separator.
	// Condition 5:
	if (pSet->GetIgnoreTrailingSeparator())
		return FALSE;
	// Condition 6:
	if (dataPage2.GetCurrentSeparatorType() == FMT_FIXEDWIDTH)
	{
		CArray <int, int>& arrayDividerPos = dataPage2.GetArrayDividerPos();
		int i, nSize = arrayDividerPos.GetSize();
		int nMenLen = dataPage1.GetMinRecordLength();
		for (i=0; i<nSize; i++)
		{
			if (nMenLen <= arrayDividerPos[i])
				return FALSE;
		}
	}
	return TRUE;
}



CString CuPPage4::GenerateCopySyntax(TCHAR tchDelimiter, LPCTSTR lpszFile, int nMode)
{
	CuIIAPropertySheet* pParent = (CuIIAPropertySheet*)GetParent();
	CaIIAInfo& data = pParent->GetData();
	CaDataPage1& dataPage1 = data.m_dataPage1;
	CaDataPage2& dataPage2 = data.m_dataPage2;
	CaDataPage3& dataPage3 = data.m_dataPage3;
	CaIngresHeaderRowItemData& commonItemData = dataPage3.GetUpperCtrlCommonData();
	CTypedPtrArray < CPtrArray, CaColumn* >& arrayOrderColumn = commonItemData.m_arrayColumnOrder;
	CTypedPtrList < CObList, CaColumnFormat* > listColumnFormat;

	int* pArrayFixedLength = NULL;
	CString strReadFormat;

	if (tchDelimiter != _T('\0'))
		strReadFormat = _T("=char(0) ");
	else
	{
		if (nMode == -1 || nMode == 2) // Ignore mode
			strReadFormat = _T("=varchar(0) ");
		else
		if (nMode == 0)  // Fixed width
		{
			//
			// For the fixed width, the record field lengths determine the
			// widths of the whole columns:
			CPtrArray& arrayRecord = dataPage2.GetArrayRecord();
			int i, nSize = arrayRecord.GetSize();
			for (i=0; i<nSize; i++)
			{
				CaRecord* pRecord = (CaRecord*)arrayRecord.GetAt(i);
				CStringArray& arrayField = pRecord->GetArrayFields();
				int j, nField = arrayField.GetSize();
				if (nField < 2)
					break;
				pArrayFixedLength = new int [nField-1];
				ASSERT ((arrayOrderColumn.GetSize() -1) == nField);
				if ((arrayOrderColumn.GetSize() -1) != nField)
				{
					delete pArrayFixedLength;
					return _T("Error");
				}

				for (j=0; j<nField-1; j++)
				{
					CString strField(arrayField.GetAt(j));
					pArrayFixedLength[j] = strField.GetLength();
				}
				break;
			}
		}
		else
		if (nMode == 1)  // Special case
		{
			strReadFormat = _T("=char(0) ");
		}
	}

	CString strColList = _T("(");
	CString strCopySyntax = _T("copy table ");
	CString strTable;

	if (dataPage1.IsExistingTable())
	{
		strTable.Format (
			_T("%s.%s "), 
			(LPCTSTR)INGRESII_llQuoteIfNeeded(dataPage1.GetTableOwner()), 
			(LPCTSTR)INGRESII_llQuoteIfNeeded(dataPage1.GetTable()));
	}
	else
	{
		//
		// TODO: use the current connected user as table owner.
		TRACE0 ("TODO (CuPPage4::GenerateCopySyntax): Use the current connected user as table owner\n");
		strTable = INGRESII_llQuoteIfNeeded(dataPage1.GetNewTable());
	}

	strCopySyntax+= strTable;
	strColList += consttchszReturn;
	strColList += _T("\t");
	CString strType;
	int i, nSize = arrayOrderColumn.GetSize();
	//
	// Start from 1 because of the <dummy column> at position 0.
	for (i=1; i<nSize; i++)
	{
		CaColumn* pCol = arrayOrderColumn.GetAt(i);
		if (pCol)
		{
			CString strItem = INGRESII_llQuote(pCol->GetName());

			CaColumnFormat* pColFormat = new CaColumnFormat();
			pColFormat->m_bDummy = FALSE;
			listColumnFormat.AddTail (pColFormat);

			if (nMode != 0)
				strItem+= strReadFormat;
			else 
			{
				//
				// Fixed width format
				if (pArrayFixedLength)
				{
					if (i != (nSize-1))
						strReadFormat.Format(_T("=char(%d) "), pArrayFixedLength[i-1]);
					else
						strReadFormat = _T("=char(0) ");
				}
				else
					strReadFormat = _T("=char(0) ");

				strItem+= strReadFormat;
			}

			if (i != (nSize-1) && tchDelimiter != _T('\0'))
			{
				if (tchDelimiter != _T('\t'))
				{
					CString strDelimiter;
					strDelimiter.Format (_T("'%c' "), tchDelimiter);
					strItem+= strDelimiter;
				}
				else
				{
					strItem+= _T("tab ");
				}
			}
			pColFormat->m_strName_Format = strItem;
			//
			// With NULL ?
			if (pCol->IsNullable())
			{
				strItem.Format (_T("with null (%s)"), (LPCTSTR)m_strNull);
				pColFormat->m_strNullOption = strItem;
			}
		}
		else // Skip the column. Use the dummy column d0 (only if not special case)
		if (nMode != 1 && nMode != 2)
		{
			CString strDummyCol;
			strDummyCol.Format (_T("dumycol%d"), i);
			CaColumnFormat* pColFormat = new CaColumnFormat();
			pColFormat->m_bDummy = TRUE;
			listColumnFormat.AddTail (pColFormat);
		
			if (i != (nSize-1))
			{
				if (nMode != 0)
				{
					//
					// Skip the current column (current char until the next delimiter):
					if (tchDelimiter != _T('\t'))
					{
						CString strDelimiter;
						strDelimiter.Format (_T("='d0%c'"), tchDelimiter);
						strDummyCol+= strDelimiter;
					}
					else
					{
						strDummyCol += _T("='d0tab'");
					}
				}
				else // Fixed width
				{
					//
					// Skip the current column (N characters):
					CString strDummy;
					strDummy.Format (_T("=d(%d)"), pArrayFixedLength[i]);
					strDummyCol+= strDummy;
				}
			}
			else
			{
				//
				// Skip the last column:
				strDummyCol += _T("='d0nl'");
			}
			pColFormat->m_strName_Format = strDummyCol;
		}
	}

	BOOL bOne = TRUE;
	POSITION pos = listColumnFormat.GetHeadPosition();
	while (pos != NULL)
	{
		CaColumnFormat* pColF = listColumnFormat.GetNext (pos);
		if (!bOne)
		{
			strColList+= _T(", ");
			strColList+= consttchszReturn;
			strColList+= _T("\t");
		}
		strColList += pColF->m_strName_Format;
		if (pos == NULL && !pColF->m_bDummy)
			strColList += _T("nl ");
		if (!pColF->m_strNullOption.IsEmpty())
			strColList += pColF->m_strNullOption;
		bOne = FALSE;
	}

	strColList += _T(") ");
	strColList += consttchszReturn;
	strCopySyntax += strColList;
	strCopySyntax += _T("from ");

#if defined (_SQLCOPY_CALLBACK)
	strCopySyntax += _T("program ");
#else
	CString strFile;
	strFile.Format (_T("'%s'"), lpszFile);
	strCopySyntax+=strFile;
#endif

	if (pArrayFixedLength)
		delete pArrayFixedLength;
	while (!listColumnFormat.IsEmpty())
		delete listColumnFormat.RemoveHead();
	return strCopySyntax;
}


void CuPPage4::OnRadioAppend() 
{
	m_nAppendOrDelete = IDC_RADIO1;
}

void CuPPage4::OnRadioReplace() 
{
	m_nAppendOrDelete = IDC_RADIO2;
}

void CuPPage4::OnButtonSaveImportParameter() 
{
	CuIIAPropertySheet* pParent = (CuIIAPropertySheet*)GetParent();
	CaIIAInfo& data = pParent->GetData();
	CaDataPage4& dataPage4 = data.m_dataPage4;

	//
	// Set data to the page4 before invoking save parameter:
	int nSelected = GetCheckedRadioButton (IDC_RADIO1, IDC_RADIO2);
	if (nSelected == IDC_RADIO1)
		dataPage4.SetAppend (TRUE);
	else
		dataPage4.SetAppend(FALSE);
	nSelected = m_cCheckProgressCommit.GetCheck();
	if (nSelected == 1)
	{
		CString strCommitStep = _T("");
		m_cEditProgressCommitStep.GetWindowText (strCommitStep);
		int nCommitStep = _ttoi (strCommitStep);
		if (nCommitStep > 0 )
			dataPage4.SetCommitStep(nCommitStep);
	}
	dataPage4.SetTableState(m_bTableIsEmpty);

	CString strFullName;
	CFileDialog dlg(
	    FALSE,
	    NULL,
	    NULL,
	    OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
	    _T("Import Parameter Files (*.ii_imp)|*.ii_imp||"));

	if (dlg.DoModal() != IDOK)
		return; 
	strFullName = dlg.GetPathName ();
	int nDot = strFullName.ReverseFind(_T('.'));
	if (nDot==-1)
	{
		strFullName += _T(".ii_imp");
	}
	else
	{
		CString strExt = strFullName.Mid(nDot+1);
		if (strExt.IsEmpty())
			strFullName += _T("ii_imp");
		else
		if (strExt.CompareNoCase (_T("ii_imp")) != 0)
			strFullName += _T(".ii_imp");
	}
	data.SaveData (strFullName);
}

void CuPPage4::Loading()
{
	CString strMsg;
	CuIIAPropertySheet* pParent = (CuIIAPropertySheet*)GetParent();
	CaIIAInfo& data = pParent->GetData();
	CaDataPage4& dataPage4 = data.m_dataPage4;
	CaDataForLoadSave* pData4LoadSave = data.GetLoadSaveData();
	if (!pData4LoadSave)
		return;
	if (pData4LoadSave->GetUsedByPage() >= 4)
		return;

	pData4LoadSave->SetUsedByPage (4);
	int nRadio = pData4LoadSave->GetAppend()? IDC_RADIO1: IDC_RADIO2;
	BOOL bLoadEmpty = pData4LoadSave->GetTableState();

	if (m_bTableIsEmpty)
	{
		//
		// If the table is empty then disable the radio Append... Replace...:
		GetDlgItem(IDC_RADIO1)->EnableWindow(FALSE);
		GetDlgItem(IDC_RADIO2)->EnableWindow(FALSE);
		CheckRadioButton (IDC_RADIO1, IDC_RADIO2, -1);
	}
	else
	{
		GetDlgItem(IDC_RADIO1)->EnableWindow(TRUE);
		GetDlgItem(IDC_RADIO2)->EnableWindow(TRUE);
		if (bLoadEmpty)
		{
			// MSG = 
			// The table was empty when the Import Parameters were saved. 
			// It is now no more empty, and the Append imported rows to the Existing Ones option has been selected.
			AfxMessageBox (IDS_LOADPARAM_ERR_STEP4_COPYOPTION, MB_ICONEXCLAMATION|MB_OK);
			CheckRadioButton (IDC_RADIO1, IDC_RADIO2, IDC_RADIO1);
		}
		else
			CheckRadioButton (IDC_RADIO1, IDC_RADIO2, nRadio);
	}

	int nCommitStep = pData4LoadSave->GetCommitStep();
	if (nCommitStep > 0)
	{
		CString strCommitStep;
		strCommitStep.Format (_T("%d"), nCommitStep);
		m_cEditProgressCommitStep.SetWindowText (strCommitStep);
		m_cCheckProgressCommit.SetCheck (1);
	}
}

void CuPPage4::OnCheckProgressCommit() 
{
	BOOL bEnable = FALSE;
	int nCheck = m_cCheckProgressCommit.GetCheck();
	if (nCheck == 1)
		bEnable = TRUE;
	m_cEditProgressCommitStep.EnableWindow(bEnable);
}

