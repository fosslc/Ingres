/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : formdata.cpp: implementation of the CaFormatData class
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : FILE FORMAT IDENTIFICATION DATA 
**
** History:
**
** 08-Nov-2000 (uk$so01)
**    Created
** 09-Jan-2001 (uk$so01)
**    (String Qualifier)
**    According to francois noirot-nerin (noifr01):
**
**    A string qualifier if exists must qualify the TOTALITY of the column 
**    AND must precede IMMEDIATELY a separator or a begin of line AND must 
**    follow IMMEDIATELY by a separator or end of line.
**    For axample a line below: 
**       ABCD;"XYZ;0123";ABC
**       has a string qualifier "XYZ;0123" and the semicolon inside the string
**       qualifier will not interprete as a separator and the algo generates:
**       Col1 =ABCD
**       Col2 =XYZ;0123
**       Col3 =ABC
**
**    BUT the following line (only semicolon is a UNIQUE separator)
**       ABC;BEGIN X= "if a > b the 10; else 100" END;   "do you see?"
**    will not have any string qualifier. So it is possible to generate the 
**    following:
**       Col1 =ABC
**       Col2 =BEGIN X= "if a > b the 10
**       Col3 = else 100" END
**       Col4 =   "do you see?"
**    because the first text qualifier preceded by a space, and only semicolon
**    is considered to be a separator.
** 25-Jun-2001 (uk$so01)
**    BUG #105125, changing the decimal (8,2) to (5,0)
** 02-Aug-2001 (hanje04)
**    BUG 105411
**    For UNIX don't check for Line Feed afer cariage return as it will
**    not exist.
** 17-Oct-2001 (uk$so01)
**    SIR #103852, miss some behaviour.
** 31-Oct-2001 (hanje04)
**    Replaced out of date 'CString& name=' constructs with CString name() as
**    they was causing problems on Linux.
** 17-Jan-2002 (uk$so01)
**    (bug 106844). Add BOOL m_bStopScanning to indicate if we really stop
**     scanning the file due to the limitation.
** 21-Jan-2002 (uk$so01)
**    (bug 106844). Additional fix: Replace ifstream and some of CFile.by FILE*.
** 04-Apr-2002 (uk$so01)
**    BUG #107505, import column that has length 0, record length != sum (column's length)
** 21-Nov-2003 (uk$so01)
**    SIR  #111320, Construct default headers from the "skipped first line"
** 17-May-2004 (schph01)
**    SIR 111507 Add management for new column type bigint(int8)
** 10-Mar-2010 (drivi01) SIR 123397
**    If a user tries to import an empty file into IIA, the utility 
**    needs to provide a better error message.  Before this change
**    the error said something about missing carriage return/line feed.
**    Add function File_Empty to check for the empty file.
**/


#include "stdafx.h"
#include "dmlcolum.h"
#include "formdata.h"
#include "rcdepend.h"
#include "ingobdml.h"
#include "usrexcep.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BOOL File_Empty(LPCTSTR lpszFile)
{
	BOOL bEmpty=TRUE;
	TCHAR buf[16];
	int nRead = 0;
	CFile f(lpszFile, CFile::modeRead|CFile::shareDenyWrite|CFile::typeBinary);
	nRead = f.Read((void*)buf, 1);
	if (nRead > 0)
		bEmpty=FALSE;
	
	return bEmpty;
}

//
// Check to see if the file has CR LF in the first nLimitLen.
BOOL File_HasCRLF(LPCTSTR lpszFile, int nLimitLen)
{
	BOOL bOk=FALSE;
	TCHAR buf[16];
	int nRead = 0;
	CFile f(lpszFile, CFile::modeRead|CFile::shareDenyWrite|CFile::typeBinary);
	for (int i=0; i<=nLimitLen; i++)
	{
		nRead = f.Read((void*)buf, 1);
		if (nRead == 0)
			break; // EOF
#ifndef MAINWIN
		if (buf[0] == 0x0D)
#endif
		{
			nRead = f.Read((void*)buf, 1);
			if (nRead == 0)
				break; // EOF
			if (buf[0] == 0x0A)
				return TRUE;
		}
	}
	return bOk;
}

//
// Always rounds to the greatest size of KB unit.
DWORD File_GetSize(LPCTSTR lpszFile)
{
	//
	// Failed to calculate file size.
	CString strMsg;
	strMsg.LoadString (IDS_FAIL_TO_CALCULATE_FILE_SIZE);

	HANDLE hFile = CreateFile(
		lpszFile,               // File name
		GENERIC_READ,           // Read-only
		FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL,                   // No security attributes
		OPEN_EXISTING,          // Only open an existing file
		FILE_ATTRIBUTE_NORMAL,  // Ignore file attributes
		NULL);                  // Ignore hTemplateFile

	if (hFile == INVALID_HANDLE_VALUE)
	{
		hFile = NULL;
		AfxMessageBox (strMsg);
		return 0;
	}

	DWORD dwSize = GetFileSize(hFile, NULL);
	DWORD dwResSize = dwSize % 1024;
	TRACE2 ("The exact size of file %s is= %d Bytes\n", lpszFile, dwSize);

	dwSize = dwSize/1024;
	if (dwResSize > 0)
		dwSize++;
	CloseHandle (hFile);
	return dwSize;
}


void CaIngresHeaderRowItemData::Display(CuListCtrlDoubleUpper* pListCtrl, int i)
{
	int j, nSize = m_arrayColumnOrder.GetSize();
	CString strItem;
	CaColumn* pCol = NULL;

	switch (i)
	{
	case 0: // Ingres Table column.
		strItem.LoadString(IDS_INGRES_COLUMN_NAME);
		pListCtrl->SetItemText (i, 0, strItem);
		strItem.LoadString (IDS_NOT_IMPORTED);
		for (j=1; j<nSize; j++)
		{
			pCol = m_arrayColumnOrder.GetAt(j);
			if (pCol)
				pListCtrl->SetItemText (i, j, pCol->GetName());
			else
				pListCtrl->SetItemText (i, j, strItem);
		}
		break;

	case 1: // Ingres Column Type
		strItem.LoadString(IDS_INGRES_COLUMN_TYPE);
		pListCtrl->SetItemText (i, 0, strItem);
		for (j=1; j<nSize; j++)
		{
			pCol = m_arrayColumnOrder.GetAt(j);
			if (pCol)
				pListCtrl->SetItemText (i, j, pCol->GetTypeStr());
			else
				pListCtrl->SetItemText (i, j, _T(""));
		}
		break;

	case 2: // Ingres Column Length
		strItem.LoadString(IDS_INGRES_COLUMN_LENGTH);
		pListCtrl->SetItemText (i, 0, strItem);
		for (j=1; j<nSize; j++)
		{
			pCol = m_arrayColumnOrder.GetAt(j);
			if (pCol)
			{
				strItem = _T("");
				int nNeedLength = pCol->NeedLength();
				switch (nNeedLength)
				{
				case CaColumn::COLUMN_LENGTH:
					strItem.Format(_T("%d"), pCol->GetLength());
					break;
				case CaColumn::COLUMN_PRECISION:
					strItem = _T("");
					break;
				case CaColumn::COLUMN_PRECISIONxSCALE:
					strItem.Format(_T("%d,%d"), pCol->GetLength(), pCol->GetScale());
					break;
				default:
					strItem = _T("");
					break;
				}

				pListCtrl->SetItemText (i, j, strItem);
			}
			else
				pListCtrl->SetItemText (i, j, _T(""));
		}
		break;

	case 3: // Ingres Column Nullability
		strItem.LoadString(IDS_INGRES_COLUMN_NULLABILITY);
		pListCtrl->SetItemText (i, 0, strItem);
		for (j=1; j<nSize; j++)
		{
			pCol = m_arrayColumnOrder.GetAt(j);
			if (pCol)
			{
				if (pCol->IsNullable())
					strItem = _T("with null");
				else
					strItem = _T("not null");
				pListCtrl->SetItemText (i, j, strItem);
			}
			else
				pListCtrl->SetItemText (i, j, _T(""));
		}
		break;

	case 4: // Ingres Column Default
		strItem.LoadString(IDS_INGRES_COLUMN_DEFAULT);
		pListCtrl->SetItemText (i, 0, strItem);
		for (j=1; j<nSize; j++)
		{
			pCol = m_arrayColumnOrder.GetAt(j);
			if (pCol)
			{
				if (pCol->IsDefault())
				{
					strItem = pCol->GetDefaultValue();
					if (strItem.IsEmpty())
						strItem = _T("with default");
				}
				else
					strItem = _T("not default");
				pListCtrl->SetItemText (i, j, strItem);
			}
			else
				pListCtrl->SetItemText (i, j, _T(""));
		}
		break;

	case 5: // Source File Format
		strItem.LoadString(IDS_SOURCE_FILE_FORMAT);
		pListCtrl->SetItemText (i, 0, strItem);
		break;

	default: // Unexpected line
		ASSERT(FALSE);
		break;
	}
}


void CaIngresHeaderRowItemData::EditValue (CuListCtrlDoubleUpper* pListCtrl, int iItem, int iSubItem, CRect rcCell)
{
	if (iSubItem == 0)
		return;
	if (!m_bAllowChange)
		return;
	int j, nSize = m_arrayColumn.GetSize();
	CString strItem;
	CaColumn* pCol = NULL;

	switch (iItem)
	{
	case 0: // Ingres Table column.
		strItem = pListCtrl->GetItemText (iItem, iSubItem);
		if (m_bExistingTable)
		{
			pListCtrl->COMBO_SetEditableMode(FALSE);
			CComboBox* pCombo = pListCtrl->GetComboBox();
			ASSERT (pCombo);
			if (!pCombo)
				break;
			if (!IsWindow (pCombo->m_hWnd))
				break;
			pCombo->ResetContent();

			for (j=1; j<nSize; j++)
			{
				CaColumn* pCol = (CaColumn*)m_arrayColumn.GetAt(j);
				pCombo->AddString (pCol->GetName());
			}
			CString strNotImported;
			strNotImported.LoadString(IDS_NOT_IMPORTED);
			pCombo->AddString (strNotImported);
			pListCtrl->SetComboBox (iItem, iSubItem, rcCell, strItem);
		}
		else
		{
			pListCtrl->SetEditText (iItem, iSubItem, rcCell, strItem);
		}
		break;

	case 1: // Ingres column Type
		if (m_bExistingTable)
			break; // Cannot change the property of column of an existing table.
		else
		{
			const int nMaxType = 18;
			TCHAR tchType[nMaxType][32]=
			{
				_T("bigint"),
				_T("byte"),
				_T("byte varying"),
				_T("c"),
				_T("char"),
				_T("date"),
				_T("decimal"),
				_T("float"),
				_T("float4"),
				_T("float8"),
				_T("int1"),
				_T("int2"),
				_T("int4"),
				_T("int8"),
				_T("integer"),
				_T("money"),
				_T("text"),
				_T("varchar")
			};
			
			BOOL bArrayRemoveType4Gateway[nMaxType] =
			{
				1, // bigint
				1, // byte
				1, // byte varying
				1, // c
				0, // char
				0, // date
				0, // decimal
				0, // float
				1, // float4
				1, // float8
				1, // int1
				1, // int2
				1, // int3
				1, // int8
				0, // integer
				1, // money
				1, // text
				0  // varchar
			};


			strItem = pListCtrl->GetItemText (iItem, iSubItem);
			pListCtrl->COMBO_SetEditableMode(FALSE);
			CComboBox* pCombo = pListCtrl->GetComboBox();
			ASSERT (pCombo);
			if (!pCombo)
				break;
			if (!IsWindow (pCombo->m_hWnd))
				break;
			pCombo->ResetContent();

			for (j=0; j<nMaxType; j++)
			{
				if (m_bGateway)
				{
					if (!bArrayRemoveType4Gateway[j])
						pCombo->AddString (tchType[j]);
				}
				else
					pCombo->AddString (tchType[j]);
			}
			pListCtrl->SetComboBox (iItem, iSubItem, rcCell, strItem);
		}
		break;

	case 2: // Ingres column Length
		if (m_bExistingTable)
			break; // Cannot change the property of column of an existing table.
		//
		// Get the type name:
		strItem = pListCtrl->GetItemText (1, iSubItem);
		if (strItem == _T("date")    || 
		    strItem == _T("float4")  || 
		    strItem == _T("float8")  || 
		    strItem == _T("int1")    || 
		    strItem == _T("int2")    || 
		    strItem == _T("int4")    || 
		    strItem == _T("integer") || 
		    strItem == _T("bigint")  || 
		    strItem == _T("int8")    || 
		    strItem == _T("money"))
		{
			//
			// These types do not need length;
			break;
		}

		if (strItem == _T("decimal"))
		{
			strItem = pListCtrl->GetItemText (iItem, iSubItem);
			pListCtrl->SetEditText (iItem, iSubItem, rcCell, strItem);
		}
		else
		{
			strItem = pListCtrl->GetItemText (iItem, iSubItem);
			pListCtrl->SetEditNumber (iItem, iSubItem, rcCell);
		}
		break;

	case 3: // Ingres column Nullability
		if (m_bExistingTable)
			break; // Cannot change the property of column of an existing table.
		else
		{
			const int nMaxType = 2;
			TCHAR tchType[nMaxType][32]=
			{
				_T("with null"),
				_T("not null")
			};

			strItem = pListCtrl->GetItemText (iItem, iSubItem);
			pListCtrl->COMBO_SetEditableMode(FALSE);
			CComboBox* pCombo = pListCtrl->GetComboBox();
			ASSERT (pCombo);
			if (!pCombo)
				break;
			if (!IsWindow (pCombo->m_hWnd))
				break;
			pCombo->ResetContent();

			for (j=0; j<nMaxType; j++)
			{
				pCombo->AddString (tchType[j]);
			}
			pListCtrl->SetComboBox (iItem, iSubItem, rcCell, strItem);
		}
		break;

	case 4: // Ingres column Default
		if (m_bExistingTable)
			break; // Cannot change the property of column of an existing table.
		if (m_bExistingTable)
			break; // Cannot change the property of column of an existing table.
		else
		{
			const int nMaxType = 3;
			TCHAR tchType[nMaxType][32]=
			{
				_T("with default"),
				_T("not default"),
				_T("NULL")
			};

			strItem = pListCtrl->GetItemText (iItem, iSubItem);
			pListCtrl->COMBO_SetEditableMode(TRUE);
			CComboBox* pCombo = pListCtrl->GetComboBox();
			ASSERT (pCombo);
			if (!pCombo)
				break;
			if (!IsWindow (pCombo->m_hWnd))
				break;
			pCombo->ResetContent();

			for (j=0; j<nMaxType; j++)
			{
				pCombo->AddString (tchType[j]);
			}
			pListCtrl->SetComboBox (iItem, iSubItem, rcCell, strItem);
		}
		break;
	default:
		ASSERT(FALSE);
		break;
	}
}


BOOL CaIngresHeaderRowItemData::UpdateValue (CuListCtrlDoubleUpper* pListCtrl, int iItem, int iSubItem, CString& strNewValue)
{
	if (iSubItem == 0)
		return TRUE;

	int i, nSize = m_arrayColumn.GetSize();

	//
	// Only column name can be re-arranged:
	if (m_bExistingTable)
	{
		if (iItem != 0)
			return TRUE;
		//
		// Only the first row can be update when the existing table is used:
		BOOL bFound = FALSE;
		for (i=1; i<nSize; i++)
		{
			CaColumn* pCol = m_arrayColumn.GetAt(i);
			if (pCol->GetName().CompareNoCase(strNewValue) == 0)
			{
				m_arrayColumnOrder.SetAt (iSubItem, pCol);
				bFound = TRUE;
				break;
			}
		}

		if (!bFound)
		{
			//
			// This line will cause to display the text "<not imported>":
			m_arrayColumnOrder.SetAt (iSubItem, NULL);
		}
		pListCtrl->DisplayItem();
		return TRUE;
	}

	CString strItem;
	CaColumn* pCol = m_arrayColumn.GetAt (iSubItem);
	if (!pCol)
		return FALSE;

	switch (iItem)
	{
	case 0: // Ingres Table column.
		pCol->SetItem(strNewValue);
		break;

	case 1: // Ingres column Type
		pCol->SetTypeStr(strNewValue);
		if (strNewValue == _T("date")    || 
		    strNewValue == _T("float4")  || 
		    strNewValue == _T("float8")  || 
		    strNewValue == _T("int1")    || 
		    strNewValue == _T("int2")    || 
		    strNewValue == _T("int4")    || 
		    strNewValue == _T("integer") || 
		    strNewValue == _T("bigint")  || 
		    strNewValue == _T("int8")    || 
		    strNewValue == _T("money"))
		{
			//
			// These types do not need length;
			pCol->SetLength (0);
			Display (pListCtrl, 2);
		}
		else
		{
			if (strNewValue == _T("decimal"))
			{
				//
				// When user selects DECIMAL then display "5,0" in the edit
				// so that user can understand that this is the format of decimal.
				pCol->SetLength (5);
				pCol->SetScale  (0);
			}
			Display (pListCtrl, 2);
		}
		break;

	case 2: // Ingres column length
		strItem = pListCtrl->GetItemText (1, iSubItem);
		if (strItem == _T("decimal"))
		{
			int nComma = strNewValue.Find (_T(','));
			ASSERT (nComma != -1);
			if (nComma != -1)
			{
				CString stPrec = strNewValue.Left (nComma);
				CString strScale= strNewValue.Mid (nComma+1);
				pCol->SetLength(_ttoi(stPrec));
				pCol->SetScale (_ttoi(strScale));
			}
		}
		else
		{
			pCol->SetLength(_ttoi(strNewValue));
		}
		break;

	case 3: // Ingres column Nullability
		if (strNewValue.CompareNoCase (_T("with null")) == 0)
			pCol->SetNullable (TRUE);
		else
			pCol->SetNullable (FALSE);
		break;

	case 4: // Ingres column Default
		if (strNewValue.CompareNoCase (_T("not default")) == 0)
		{
			pCol->SetDefault (FALSE);
		}
		else
		{
			pCol->SetDefault (TRUE);
			if (strNewValue.CompareNoCase (_T("with default")) == 0)
				pCol->SetDefaultValue (_T("null"));
			else
				pCol->SetDefaultValue (strNewValue);
		}
		break;
	}
	return TRUE;
}



CaDataPage1::CaDataPage1()
{
	m_ieaTree.SetSessionManager(&theApp.m_sessionManager);
	m_bExistingTable = FALSE;
	
	m_bDatabaseGateway = FALSE;
	m_strServerClass = _T("");
	m_strNode = _T("");
	m_strDatabase = _T("");
	m_strTable = _T("");
	m_strTableOwner = _T("");
	m_strFileImportParameter = _T("");
	m_strFile2BeImported = _T("");
	m_nCodePage = 0;
	m_nFileMatchTable = 1;
	SetImportedFileType(IMPORT_UNKNOWN);
	m_strOthersSeparators = _T("");
	m_strNewTable = _T("");
	m_nMaxRecordLen = 0;
	m_nMinRecordLen = 0;
	m_nFileSize = 0;
	m_nKB2Scan = 0;
	m_nLineCountToSkip = 0;
	m_bStopScanning = FALSE;
	m_bFirstLineHeader = TRUE;

	for (int i=0; i<MAX_ENUMPAGESIZE; i++) 
		m_bArrayPageSize[i] = FALSE;

	m_nLastDetection = 0;
}


void CaDataPage1::CleanupFailureInfo()
{
	while (!m_listFailureInfo.IsEmpty())
		delete m_listFailureInfo.RemoveHead();
}


void CaDataPage2::CleanFieldSizeInfo()
{
	int i, nSize = m_arrayFieldSizeInfo.GetSize();
	for (i=0; i<nSize; i++)
	{
		CaColumnSize* pObj = (CaColumnSize*)m_arrayFieldSizeInfo.GetAt(i);
		delete pObj;
	}
	m_arrayFieldSizeInfo.RemoveAll();
}

void CaDataPage2::SetFieldSizeInfo (int nNewSize)
{
	ASSERT(m_arrayFieldSizeInfo.GetSize() == 0); // You must call CleanFieldSizeInfo first
	m_arrayFieldSizeInfo.SetSize(nNewSize);
	int i, nSize = nNewSize;
	for (i=0; i<nSize; i++)
	{
		CaColumnSize* pNewSize = new CaColumnSize();
		m_arrayFieldSizeInfo.SetAt(i, (void*)pNewSize);
	}
}

int& CaDataPage2::GetFieldSizeMax(int nIndex)
{
	ASSERT(nIndex>=0 && nIndex<m_arrayFieldSizeInfo.GetSize());
	CaColumnSize* pNewSize = (CaColumnSize*)m_arrayFieldSizeInfo.GetAt(nIndex);
	return pNewSize->m_nMaxLength;
}

int& CaDataPage2::GetFieldSizeEffectiveMax(int nIndex)
{
	ASSERT(nIndex>=0 && nIndex<m_arrayFieldSizeInfo.GetSize());
	CaColumnSize* pNewSize = (CaColumnSize*)m_arrayFieldSizeInfo.GetAt(nIndex);
	return pNewSize->m_nMaxEffectiveLength;
}

BOOL& CaDataPage2::IsTrailingSpace(int nIndex)
{
	ASSERT(nIndex>=0 && nIndex<m_arrayFieldSizeInfo.GetSize());
	CaColumnSize* pNewSize = (CaColumnSize*)m_arrayFieldSizeInfo.GetAt(nIndex);
	return pNewSize->m_bTrailingSpace;
}



CaIIAInfo::CaIIAInfo()
{
	m_bInterrupted = FALSE;
	m_pLoadSaveData = NULL;
}

CaIIAInfo::~CaIIAInfo()
{
	CleanLoadSaveData();
}

void CaIIAInfo::CleanLoadSaveData()
{
	if (m_pLoadSaveData)
		delete m_pLoadSaveData;
	m_pLoadSaveData = NULL;
}

void CaIIAInfo::LoadData(CString& strFile)
{
	CleanLoadSaveData();
	m_pLoadSaveData = new CaDataForLoadSave();
	m_pLoadSaveData->LoadData (this, strFile);
}

void CaIIAInfo::SaveData(CString& strFile)
{
	CaDataForLoadSave loadSave;
	loadSave.SaveData(this, strFile);
}



//
// Global in theApp (CSvriiaApp)
// ************************************************************************************************
CaUserData4GetRow::CaUserData4GetRow()
{
	m_bCopiedComplete = FALSE;
	m_nCommitCounter  = 0;
	m_nCommitStep     = 0;
	m_nProgressCommit = 0;
	m_nCopyRowcount = 0;
	m_nCurrentRecord = 0;
	m_nTotalSize = 0;
	m_bUseFileDirectly = TRUE;
	m_pIIAData = NULL;
	m_hwndAnimateDlg = NULL;
	m_nConfirm = INTERRUPT_CANCEL;
	m_pFile = NULL;

	m_bBufferTooSmall   = FALSE;
	m_nError = 0;
}

CaUserData4GetRow::~CaUserData4GetRow()
{
	Cleanup();
}

void CaUserData4GetRow::Cleanup()
{
	if (m_pFile)
	{
		fclose (m_pFile);
		m_pFile = NULL;
	}

	m_pFile = NULL;
	m_nCopyRowcount = 0;
	m_nCurrentRecord = 0;
	m_nTotalSize = 0;
	m_bUseFileDirectly = TRUE;
	m_pIIAData = NULL;
	m_hwndAnimateDlg = NULL;
	m_nConfirm = INTERRUPT_CANCEL;
}

void CaUserData4GetRow::InitResourceString()
{
	m_strMsgImportInfo1.LoadString(IDS_ALREADY_IMPORT_ROWS);  // Already imported: %d of %d row(s)
	m_strMsgImportInfo2.LoadString(IDS_ALREADY_IMPORT_BYTES); // Already imported: %d of %d byte(s)
}




//
// CLASS CaDataForLoadSave (data for load / save)
// ************************************************************************************************
IMPLEMENT_SERIAL (CaDataForLoadSave, CObject, 2)
CaDataForLoadSave::CaDataForLoadSave()
{
	Cleanup();
	m_strHostName = _T("");
	m_strIISystem = _T("");
}

CaDataForLoadSave::~CaDataForLoadSave()
{
	Cleanup();
}

void CaDataForLoadSave::Cleanup()
{
	m_nUsedByPage = 1;

	//
	// Page 1:
	// *******
	m_strServerClass = _T("");
	m_strNode = _T("");
	m_strDatabase = _T("");
	m_strTable = _T("");
	m_strTableOwner = _T("");
	m_strFile2BeImported = _T("");
	m_strNewTable = _T("");
	m_bExistingTable = FALSE;
	m_nCodePage = 0;
	//
	// Number and order of columns of the file expected to match
	// exactly those of the Ingres Table ?
	m_nFileMatchTable = 1;
	SetImportedFileType(IMPORT_UNKNOWN);
	m_strOthersSeparators = _T("");
	m_nKB2Scan = 0;
	m_nLineCountToSkip = 0;
	m_bFirstLineHeader = FALSE;
	m_arrayTreeItemPath.RemoveAll();
	//
	// Page 2:
	// *******
	m_nSeparatorType = FMT_FIELDSEPARATOR;
	m_nColumnCount = 0;
	//
	// For m_nSeparatorType = FMT_FIELDSEPARATOR:
	m_strSeparator = _T("");
	m_strTQ = _T("");
	m_bCS = FALSE;
	//
	// For m_nSeparatorType = FMT_FIXEDWIDTH:
	m_bUser = FALSE;
	m_arrayDividerPos.RemoveAll();

	//
	// Page 3:
	// *******
	int i, nSize = m_arrayColumnOrder.GetSize();
	for (i=0; i<nSize; i++)
	{
		CaColumn* pCol = m_arrayColumnOrder.GetAt(i);
		if (!pCol)
			continue;
		delete pCol;
	}
	m_arrayColumnOrder.RemoveAll();

	m_strIIDateFormat = _T("");
	m_strIIDecimalFormat = _T("");
	//
	// Page 4:
	// *******
	m_bAppend = TRUE;
	m_nCommit = 0;
	m_bTableIsEmpty = TRUE;
}

void CaDataForLoadSave::LoadData (CaIIAInfo* pIIAData, CString& strFile)
{
	try
	{
		CFile f((LPCTSTR)strFile, CFile::shareDenyNone|CFile::modeRead);
		CArchive ar(&f, CArchive::load);
		Serialize (ar);
		ar.Flush();
		ar.Close();
		f.Close();

		CaDataPage1& dataPage1 = pIIAData->m_dataPage1;
		CaDataPage2& dataPage2 = pIIAData->m_dataPage2;
		CaDataPage3& dataPage3 = pIIAData->m_dataPage3;
		CaDataPage4& dataPage4 = pIIAData->m_dataPage4;

		//
		// Page 1: Load data only affects the global pIIAData->m_dataPage1
		//         for the step 1. The data in pIIAData->m_dataPage(2,3,4) will be
		//         set when user press the next button and check to see if exists
		//         something that matches the data loaded for page (2,3,4).
		dataPage1.Cleanup();
		dataPage1.SetServerClass(m_strServerClass);
		dataPage1.SetNode(m_strNode);
		dataPage1.SetDatabase(m_strDatabase);
		dataPage1.SetTable(m_strTable, m_strTableOwner);
		dataPage1.SetFile2BeImported(m_strFile2BeImported);
		dataPage1.SetCodePage(m_nCodePage);
		dataPage1.SetFileMatchTable(m_nFileMatchTable);
		dataPage1.SetExistingTable(m_bExistingTable);
		dataPage1.SetImportedFileType(m_typeImportedFile);
		dataPage1.SetCustomSeparator (m_strOthersSeparators);
		dataPage1.SetNewTable(m_strNewTable);
		dataPage1.SetKBToScan(m_nKB2Scan);
		dataPage1.SetLineCountToSkip(m_nLineCountToSkip);
		dataPage1.SetFirstLineAsHeader(m_bFirstLineHeader);
	
		CStringArray& arrayTreePath = dataPage1.GetTreeItemPath();
		int i, nSize = m_arrayTreeItemPath.GetSize();
		for (i=0; i<nSize; i++)
		{
			arrayTreePath.Add(m_arrayTreeItemPath.GetAt(i));
		}

		//
		// Page 2: Clean the Data already set in step 2.
		dataPage2.Cleanup();
		//
		// Page 3: Clean the Data already set in step 3.
		dataPage3.Cleanup();
		//
		// Page 4: Clean the Data already set in step 4
		dataPage4.Cleanup();
	}
	catch (CFileException* e)
	{
		MSGBOX_FileExceptionMessage (e);
		e->Delete();
	}
	catch (CArchiveException* e)
	{
		MSGBOX_ArchiveExceptionMessage (e);
		e->Delete();
	}
	catch(...)
	{
		AfxMessageBox (_T("Serialization error. \nCause: Unknown."), MB_OK|MB_ICONEXCLAMATION);
	}
}


void CaDataForLoadSave::SaveData (CaIIAInfo* pIIAData, CString& strFile)
{
	try
	{
		CaDataPage1& dataPage1 = pIIAData->m_dataPage1;
		CaDataPage2& dataPage2 = pIIAData->m_dataPage2;
		CaDataPage3& dataPage3 = pIIAData->m_dataPage3;
		CaDataPage4& dataPage4 = pIIAData->m_dataPage4;

		TCHAR* pEnv = _tgetenv(_T("II_SYSTEM"));
		if (pEnv)
			m_strIISystem = pEnv;
		else
			m_strIISystem = _T("");
		m_strHostName = GetLocalHostName();

		//
		// Page 1:
		m_strServerClass      = dataPage1.GetServerClass();
		m_strNode             = dataPage1.GetNode();
		m_strDatabase         = dataPage1.GetDatabase();
		m_strTable            = dataPage1.GetTable();
		m_strTableOwner       = dataPage1.GetTableOwner();
		m_strFile2BeImported  = dataPage1.GetFile2BeImported();
		m_nCodePage           = dataPage1.GetCodePage();
		m_nFileMatchTable     = dataPage1.GetFileMatchTable();
		m_bExistingTable      = dataPage1.IsExistingTable();
		m_typeImportedFile    = dataPage1.GetImportedFileType();
		m_strOthersSeparators = dataPage1.GetCustomSeparator ();
		m_strNewTable         = dataPage1.GetNewTable();
		m_nKB2Scan            = dataPage1.GetKBToScan();
		m_nLineCountToSkip    = dataPage1.GetLineCountToSkip();
		m_bFirstLineHeader    = dataPage1.GetFirstLineAsHeader();

		m_arrayTreeItemPath.RemoveAll();

		CStringArray& arrayTreePath = dataPage1.GetTreeItemPath();
		int i, nSize = arrayTreePath.GetSize();
		for (i=0; i<nSize; i++)
		{
			m_arrayTreeItemPath.Add(arrayTreePath.GetAt(i));
		}

		//
		// Page 2:
		m_nSeparatorType = dataPage2.GetCurrentSeparatorType();
		m_nColumnCount = dataPage2.GetColumnCount();

		if (m_nSeparatorType == FMT_FIELDSEPARATOR)
		{
			CaSeparatorSet* pSet = NULL;
			CaSeparator* pSep = NULL;

			//
			// Be careful, if pSep = NULL then then we are in the special case.
			// That is each line is considered as a single column:
			dataPage2.GetCurrentFormat (pSet, pSep);
			m_strTQ          = pSet->GetTextQualifier();
			m_bCS            = pSet->GetConsecutiveSeparatorsAsOne();
			if (!pSep)
				m_strSeparator = _T("");
			else
				m_strSeparator   = pSep->GetSeparator();
		}
		else
		if (m_nSeparatorType == FMT_FIXEDWIDTH)
		{
			m_bUser = dataPage2.GetFixedWidthUser();
			CArray < int, int >& arrDividerPos = dataPage2.GetArrayDividerPos();
			int i, nSize = arrDividerPos.GetSize();
			for (i=0; i<nSize; i++)
				m_arrayDividerPos.Add(arrDividerPos.GetAt(i));
		}

		//
		// Page 3:
		m_strIIDateFormat    = dataPage3.GetIIDateFormat ();
		m_strIIDecimalFormat = dataPage3.GetIIDecimalFormat ();
		CaIngresHeaderRowItemData& commonItemData = dataPage3.GetUpperCtrlCommonData();
		//
		// The index = 0 is a dummy column !
		nSize = commonItemData.m_arrayColumnOrder.GetSize();
		for (i=0; i<nSize; i++)
		{
			CaColumn* pCol = commonItemData.m_arrayColumnOrder.GetAt(i);
			if (pCol)
				m_arrayColumnOrder.Add (new CaColumn (*pCol));
			else
				m_arrayColumnOrder.Add (NULL);
		}

		//
		// Page 4:
		m_bAppend = dataPage4.GetAppend();
		m_nCommit = dataPage4.GetCommitStep();
		m_bTableIsEmpty = dataPage4.GetTableState();

		CFile f((LPCTSTR)strFile, CFile::modeCreate|CFile::modeWrite);
		CArchive ar(&f, CArchive::store);
		Serialize (ar);
		ar.Flush();
		ar.Close();
		f.Close();
	}
	catch (CFileException* e)
	{
		MSGBOX_FileExceptionMessage (e);
		e->Delete();
	}
	catch (CArchiveException* e)
	{
		MSGBOX_ArchiveExceptionMessage (e);
		e->Delete();
	}
	catch(...)
	{
		AfxMessageBox (_T("Serialization error. \nCause: Unknown."), MB_OK|MB_ICONEXCLAMATION);
	}
}

void CaDataForLoadSave::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << (int)1; // Version
		ar << m_strHostName;
		ar << m_strIISystem;
		ar << m_nUsedByPage;

		//
		// Page1:
		ar << m_arrayTreeItemPath.GetSize();
		m_arrayTreeItemPath.Serialize(ar);
		ar << m_bExistingTable ;
		ar << m_strServerClass;
		ar << m_strNode;
		ar << m_strDatabase;
		ar << m_strTable;
		ar << m_strTableOwner;
		ar << m_strFile2BeImported;
		ar << m_nCodePage;
		ar << m_nFileMatchTable;
		ar << (int)m_typeImportedFile;
		ar << m_strOthersSeparators ;
		ar << m_strNewTable;
		ar << m_nKB2Scan;
		ar << m_nLineCountToSkip;
		ar << m_bFirstLineHeader;

		//
		// Page2:
		ar << (int)m_nSeparatorType;
		ar << m_nColumnCount;
		ar << m_strSeparator;
		ar << m_strTQ;
		ar << m_bCS;
		ar << m_bUser;
		ar << m_arrayDividerPos.GetSize();
		m_arrayDividerPos.Serialize(ar);

		//
		// Page 3:
		ar << m_arrayColumnOrder.GetSize();
		m_arrayColumnOrder.Serialize(ar);
		ar << m_strIIDateFormat;
		ar << m_strIIDecimalFormat;

		//
		// Page 4:
		ar << m_bAppend;
		ar << m_nCommit;
		ar << m_bTableIsEmpty;
	}
	else
	{
		int nVersion;
		ar >> nVersion;
		ar >> m_strHostName;
		ar >> m_strIISystem;
		ar >> m_nUsedByPage;

		//
		// Page 1:
		int nNum;
		m_arrayTreeItemPath.RemoveAll();
		ar >> nNum;
		m_arrayTreeItemPath.SetSize(nNum);
		m_arrayTreeItemPath.Serialize(ar);
		ar >> m_bExistingTable ;
		ar >> m_strServerClass;
		ar >> m_strNode;
		ar >> m_strDatabase;
		ar >> m_strTable;
		ar >> m_strTableOwner;
		ar >> m_strFile2BeImported;
		ar >> m_nCodePage;
		ar >> m_nFileMatchTable;
		ar >> nNum; m_typeImportedFile  = ImportedFileType(nNum);
		ar >> m_strOthersSeparators ;
		ar >> m_strNewTable;
		ar >> m_nKB2Scan;
		ar >> m_nLineCountToSkip;
		ar >> m_bFirstLineHeader;

		//
		// Page2:
		ar >> nNum; m_nSeparatorType = ColumnSeparatorType(nNum);
		ar >> m_nColumnCount;
		ar >> m_strSeparator;
		ar >> m_strTQ;
		ar >> m_bCS;
		ar >> m_bUser;
		ar >> nNum;
		m_arrayDividerPos.SetSize(nNum);
		m_arrayDividerPos.Serialize(ar);

		//
		// Page 3:
		ar >> nNum;
		m_arrayColumnOrder.SetSize(nNum);
		m_arrayColumnOrder.Serialize(ar);
		ar >> m_strIIDateFormat;
		ar >> m_strIIDecimalFormat;
		//
		// Page 4:
		ar >> m_bAppend;
		ar >> m_nCommit;
		ar >> m_bTableIsEmpty;
	}
}













void AutoDetectFormatTxt (CaIIAInfo* pData)
{
	ASSERT (FALSE);
}



//
// Detect the fixted width.
// Only the space is used is detected and used to place the column dividers.
// If all records have the same spaces at the same position P then we can place 
// the divider at the position P.
// If exist [P0, Pn] where all records have spaces then choose the Maximum one i.e the Pn.
// Let M the minnimum length of the records.
// We allow to place the divier only at the position in range [1, M-1]. This ensures that
// the las column will contain at least one character of the record where its length is minimal.
void AutoDetectFixedWidth (CaIIAInfo* pData, TCHAR*& pArrayPos, int& nArraySize)
{
	CaDataPage1& dataPage1 = pData->m_dataPage1;

	int i, nSize = 0;
	CStringArray& arrayString = dataPage1.GetArrayTextLine();
	int nMin = dataPage1.GetMinRecordLength();
	int nMax = dataPage1.GetMaxRecordLength();

	nSize = arrayString.GetSize();
	if (nSize == 0)
		return;
	//
	// Empty record must be skipped while reading the file.
	ASSERT (nMin > 0);
	if (nMin == 0)
		return;
	TCHAR* pArrayDivider = new TCHAR [nMin+1];
	for (i = 0; i<nMin; i++)
	{
		//
		// No position for column divider:
		pArrayDivider[i] = _T('x');
	}

	//
	// Initial and possible positions of column dividers are
	// sub-set of the first record position of SPACE.
	CString strLine(arrayString.GetAt(0));
	for (i = 0; i<nMin; i++)
	{
		if (strLine[i] == _T(' '))
			pArrayDivider[i] = _T(' ');
	}

	BOOL bHasPosition4Divider = FALSE;
	TCHAR* pArrayTempDivider = new TCHAR [nMin+1];
	for (i=1; i<nSize; i++)
	{
		memset (pArrayTempDivider, _T('x'), nMin);
		CString strCurLine(arrayString.GetAt(i));

		bHasPosition4Divider = FALSE;
		for (int j = 0; j<nMin; j++)
		{
			if (strCurLine[j] != _T(' '))
				pArrayDivider[j] = _T('x');

			//
			// Exist at least one space for column divider ?
			if (pArrayDivider[j] == _T(' '))
				bHasPosition4Divider = TRUE;
		}

		if (!bHasPosition4Divider)
			break;
	}
	delete pArrayTempDivider;
	//
	// At this point, if bHasPosition4Divider = TRUE then every SPACE in pArrayDivider
	// is candidate to be a column divider.
	nArraySize = nMin+1;
	pArrayPos  = pArrayDivider;
	pArrayPos[nMin] = _T('\0');

	//
	// Remove the left consecutive spaces:
	nSize = lstrlen(pArrayPos) -1;
	while (nSize >= 0)
	{
		if (pArrayPos[nSize] == _T(' '))
		{
			nSize--;
			while (nSize >= 0 && pArrayPos[nSize] == _T(' '))
			{
				pArrayPos[nSize] = _T('x');
				nSize--;
			}
			continue;
		}

		nSize--;
	}
}






void AutoDetectFormatCsv (CaIIAInfo* pData)
{
	CaDataPage1& dataPage1 = pData->m_dataPage1;
	ASSERT (dataPage1.GetImportedFileType() == IMPORT_CSV || dataPage1.GetImportedFileType() == IMPORT_TXT);
	if (dataPage1.GetImportedFileType() != IMPORT_CSV && dataPage1.GetImportedFileType() != IMPORT_TXT)
		return;

	CString strFile = dataPage1.GetFile2BeImported();
	//
	// Construct new data:
	File_GetLines(pData);

	CaDataPage2& dataPage2 = pData->m_dataPage2;
	dataPage2.Cleanup();
	int nTableColumns = 0;

	//
	// If the number and order of columns of the file expected to match
	// exactly those of the Ingres Table ?
	if (dataPage1.IsExistingTable() && dataPage1.GetFileMatchTable() == 1)
	{
		//
		// YES:
		nTableColumns = dataPage1.GetTableColumns().GetCount();
	}

	CaTreeSolution treeFindSolution(dataPage1.GetCustomSeparator(), nTableColumns);
	treeFindSolution.FindSolution (pData);
}


void AutoDetectFormatXml (CaIIAInfo* pData)
{
	ASSERT (FALSE);

}


CString MSG_DetectFileError(int nError)
{
	//
	// Unknow error has occurred while scanning the file.
	CString strMsg;
	strMsg.LoadString(IDS_SCANNING_FILE_ERROR);

	switch (nError)
	{
	case DBFxERROR_UNSUPORT:
		// This version of DBF file is not supported.
		strMsg.LoadString (IDS_UNSUPPORT_DBF_FILE);
		break;
	case DBFxERROR_HASMEMO_FILE:
		// This version of Import Assistant cannot handle DBF with a memo file.
		strMsg.LoadString (IDS_UNSUPPORT_DBF_WITH_MEMO);
		break;
	case DBFxERROR_CANNOT_READ: 
		// Unknow error has occurred while scanning the file.
		strMsg.LoadString(IDS_SCANNING_FILE_ERROR);
		break;
	case DBFxERROR_WRONGCOLUMN_COUNT:
		// The number of columns in dbf file is not equal to the number of columns in the table.
		strMsg.LoadString(IDS_COLUMNCOUNT_ERROR_IN_DBFxTABLE);
		break;
	case DBFxERROR_UNSUPORT_DATATYPE:
		// The DBF file has the column data type that cannot be supported.\n
		// The only supported data types are C (char), D (data), F (float), L (bool), N (number).
		strMsg.LoadString(IDS_UNSUPPORT_DBF_DATATYPE);
		break;
	case CSVxERROR_CANNOT_OPEN:
		// Failed to open file.
		strMsg.LoadString(IDS_FAIL_TO_OPEN_FILE);
		break;
	case CSVxERROR_LINE_TOOLONG:
		// Line too long in the file...
		strMsg.LoadString(IDS_LINE_TOO_LONG_ERROR);
		break;
	case DBFxERROR_RECORDLEN:
		// SUM of column's length is not equal to record length in the header of .dbf file
		strMsg.LoadString(IDS_DBFRECORDLEN_ERROR);
		break;
	default:
		ASSERT(FALSE);
		break;
	}

	return strMsg;
}

