/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : exportdt.cpp: Implementation file
**    Project  : EXPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Manage data for exporting 
**
** History:
**
** 30-Jan-2002 (uk$so01)
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
**    Created
** 26-Feb-2003 (uk$so01)
**    SIR  #106952, Date conversion and numeric display format.
**    And DBF data type mapping.
** 22-Aug-2003 (uk$so01)
**    SIR #106648, Add silent mode in IEA. 
** 17-Mar-2004 (uk$so01)
**    BUG #111971 / ISSUE 13305363
**    Remove the online editing of "export format" for float/decimal
**    data types.
** 20-Jan-2005 (komve01)
**    BUG #113774 / ISSUE 13919436
**    Show an error message (IDS_MSG_POSITIVE_INTEGER_ONLY)and reset 
**	  the display value to previous value if invalid export width is specified.
**	  Also, changed the Copyright year to 2005.
** 23-Mar-2005 (komve01)
**    Issue# 13919436, Bug#113774. 
**    If the return value of UpdateValueFixedWidth is false return it to the caller,
**    indicating a failure somewhere.
**/

#include "stdafx.h"
#include "exportdt.h"
#include "rcdepend.h"
#include "ingobdml.h"
#include "sqlselec.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// CLASS CaIeaDataPage1 (data for load / save)
// ************************************************************************************************
IMPLEMENT_SERIAL (CaIeaDataPage1, CObject, 1)
CaIeaDataPage1::CaIeaDataPage1()
{
	m_bDatabaseGateway = FALSE;
	m_strServerClass = _T("");
	m_strNode = _T("");
	m_strDatabase = _T(""); 
	m_strFile2BeExported = _T("");
	m_strStatement = _T("");
	n_ExportFormat = EXPORT_UNKNOWN;
}

void CaIeaDataPage1::Copy (const CaIeaDataPage1& c)
{
	m_bDatabaseGateway = c.GetGatewayDatabase();
	m_strServerClass = c.GetServerClass();
	m_strNode = c.GetNode();
	m_strDatabase = c.GetDatabase(); 
	m_strFile2BeExported = c.GetFile2BeExported();
	m_strStatement = c.GetStatement();
	n_ExportFormat = c.GetExportedFileType();

	CaIeaDataPage1* pD1 = (CaIeaDataPage1*)&c;
	m_QueryResult.Cleanup();
	CaQueryResult& rs = pD1->GetQueryResult();
	m_QueryResult = rs;

	CaIeaDataPage1& c1 = (CaIeaDataPage1&)c;
	CStringArray& a = c1.GetTreeItemPath();
	m_arrayTreeItemPath.RemoveAll();
	int i, nSize = a.GetSize();
	for (i=0; i<nSize; i++)
		m_arrayTreeItemPath.Add(a.GetAt(i));

	m_floatFormat = c.m_floatFormat;
}



void CaIeaDataPage1::Serialize (CArchive& ar)
{
	m_QueryResult.Serialize(ar);
	m_arrayTreeItemPath.Serialize(ar);
	m_floatFormat.Serialize(ar);

	if (ar.IsStoring())
	{
		ar << GetObjectVersion();

		ar << m_bDatabaseGateway;
		ar << m_strServerClass;
		ar << m_strNode;
		ar << m_strDatabase;
		ar << m_strFile2BeExported;
		ar << m_strStatement;
		ar << (int)n_ExportFormat;
	}
	else
	{
		int nFormat;
		UINT nVersion;
		ar >> nVersion;

		ar >> m_bDatabaseGateway;
		ar >> m_strServerClass;
		ar >> m_strNode;
		ar >> m_strDatabase;
		ar >> m_strFile2BeExported;
		ar >> m_strStatement;
		ar >> nFormat; n_ExportFormat = ExportedFileType(nFormat);
	}
}


//
// CLASS CaIeaDataPage2 (data for load / save)
// ************************************************************************************************
IMPLEMENT_SERIAL (CaIeaDataPage2, CObject, 1)
CaIeaDataPage2::CaIeaDataPage2()
{
	m_tchDelimiter = _T(',');
	m_tchQuote = _T('"');
	m_bQuoteIfDelimiter = TRUE; 
	m_bExportHeader = FALSE;
	m_bReadLock = FALSE;
}

void CaIeaDataPage2::Copy (const CaIeaDataPage2& c)
{
	m_tchDelimiter = c.GetDelimiter();
	m_tchQuote = c.GetQuote();
	m_bQuoteIfDelimiter = c.IsQuoteIfDelimiter(); 
	m_bExportHeader = c.IsExportHeader();
	m_bReadLock = c.IsReadLock();
}

void CaIeaDataPage2::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << GetObjectVersion();

		ar << m_tchDelimiter;
		ar << m_tchQuote;
		ar << m_bQuoteIfDelimiter;
		ar << m_bExportHeader;
		ar << m_bReadLock;
	}
	else
	{
		UINT nVersion;
		ar >> nVersion;

		ar >> m_tchDelimiter;
		ar >> m_tchQuote;
		ar >> m_bQuoteIfDelimiter;
		ar >> m_bExportHeader;
		ar >> m_bReadLock;
	}
}

//
// CLASS CaIeaDataForLoadSave (data for load / save)
// ************************************************************************************************
IMPLEMENT_SERIAL (CaIeaDataForLoadSave, CObject, 1)
CaIeaDataForLoadSave::CaIeaDataForLoadSave()
{
}

CaIeaDataForLoadSave::~CaIeaDataForLoadSave()
{
	m_dataPage1.Cleanup();
	m_dataPage2.Cleanup();
}

void CaIeaDataForLoadSave::Serialize (CArchive& ar)
{
	m_dataPage1.Serialize(ar);
	m_dataPage2.Serialize(ar);
}

void CaIeaDataForLoadSave::Cleanup()
{
	m_dataPage1.Cleanup();
	m_dataPage2.Cleanup();
}


void CaIeaDataForLoadSave::LoadData (CaIEAInfo* pIEAData, CString& strFile)
{
	try
	{
		CFile f((LPCTSTR)strFile, CFile::shareDenyNone|CFile::modeRead);
		CArchive ar(&f, CArchive::load);
		Serialize (ar);
		ar.Flush();
		ar.Close();
		f.Close();
	}
	catch (CFileException* e)
	{
		throw e;
	}
	catch (CArchiveException* e)
	{
		throw e;
	}
	catch(...)
	{
		AfxMessageBox (_T("Serialization error. \nCause: Unknown."), MB_OK|MB_ICONEXCLAMATION);
	}
}

void CaIeaDataForLoadSave::SaveData (CaIEAInfo* pIEAData, CString& strFile)
{
	try
	{
		CaIeaDataPage1& dataPage1 = pIEAData->m_dataPage1;
		CaIeaDataPage2& dataPage2 = pIEAData->m_dataPage2;

		//
		// Page 1:
		m_dataPage1 = dataPage1;
		m_dataPage2 = dataPage2;

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



//
// Global Export Assistant Data
// CLASS: CaIEAInfo
// ************************************************************************************************
CaIEAInfo::CaIEAInfo()
{
	m_bInterrupted = FALSE;
	m_pLoadSaveData = NULL;
	m_bSilent = FALSE;
	m_bOverWrite = FALSE;
	m_strLogFile = _T("");

}

CaIEAInfo::~CaIEAInfo()
{
	CleanLoadSaveData();
}

void CaIEAInfo::CleanLoadSaveData()
{
	if (m_pLoadSaveData)
		delete m_pLoadSaveData;
	m_pLoadSaveData = NULL;
}

void CaIEAInfo::LoadData(CString& strFile)
{
	CleanLoadSaveData();
	m_pLoadSaveData = new CaIeaDataForLoadSave();
	m_pLoadSaveData->LoadData (this, strFile);
}

void CaIEAInfo::SaveData(CString& strFile)
{
	CaIeaDataForLoadSave loadSave;
	loadSave.SaveData(this, strFile);
}

void CaIEAInfo::UpdateFromLoadData()
{
	ASSERT (m_pLoadSaveData);
	if (!m_pLoadSaveData)
		return;
	m_dataPage1 = m_pLoadSaveData->m_dataPage1;
	m_dataPage2 = m_pLoadSaveData->m_dataPage2;
}

void CaIEAInfo::SetBatchMode (BOOL bSilent, BOOL bOverWrite, LPCTSTR lpszLogFile)
{
	m_bSilent = bSilent;
	m_bOverWrite = bOverWrite;
	m_strLogFile = lpszLogFile;
}

void CaIEAInfo::GetBatchMode (BOOL& bSilent, BOOL& bOverWrite, CString& strLogFile)
{
	bSilent = m_bSilent;
	bOverWrite = m_bOverWrite;
	strLogFile = m_strLogFile;
}

//
// CLASS: CaQueryResult (Result of the select statement)
// ************************************************************************************************
IMPLEMENT_SERIAL (CaQueryResult, CObject, 1)
CaQueryResult::CaQueryResult()
{

}

CaQueryResult::~CaQueryResult()
{
	Cleanup();
}


void CaQueryResult::Cleanup()
{
	while (!m_listColumn.IsEmpty())
		delete m_listColumn.RemoveHead();
	while (!m_listLayout.IsEmpty())
		delete m_listLayout.RemoveHead();

	int i, nSize = m_listRow.GetSize();
	for (i=0; i<nSize; i++)
	{
		CaRowTransfer* pRow = (CaRowTransfer*)m_listRow.GetAt(i);
		delete pRow;
	}
	m_listRow.RemoveAll();
}


void CaQueryResult::Copy(const CaQueryResult& c)
{
	Cleanup();
	CaQueryResult& c1 = (CaQueryResult&)c;

	CTypedPtrList< CObList, CaColumn* >& lc = c1.GetListColumn();
	CTypedPtrList< CObList, CaColumnExport* >& lh = c1.GetListColumnLayout();
	POSITION p1, p2;
	p1 = lc.GetHeadPosition();
	p2 = lh .GetHeadPosition();

	while (p1 != NULL && p2 != NULL)
	{
		CaColumn* pCol = lc.GetNext(p1);
		CaColumnExport* pHeader = lh.GetNext (p2);

		m_listColumn.AddTail (new CaColumn (*pCol));
		m_listLayout.AddTail (new CaColumnExport (*pHeader));
	}
}

void CaQueryResult::Serialize (CArchive& ar)
{
	m_listColumn.Serialize (ar);
	m_listLayout.Serialize (ar);
	//
	// Note: m_listRow is not serialized 
	if (ar.IsStoring())
	{

	}
	else
	{


	}
}




//
// CLASS: CaIngresExportHeaderRowItemData
// ************************************************************************************************
void CaIngresExportHeaderRowItemData::LayoutHeaderCsv(CuListCtrlDoubleUpper* pListCtrl, int i)
{
	int j, nSize = m_arrayColumn.GetSize();
	CString strItem;
	CaColumnExport* pCol = NULL;
	switch (i)
	{
	case 0: // Header Text
		strItem.LoadString(IDS_EXPORT_HEADER_TEXT);
		pListCtrl->SetItemText (i, 0, strItem);
		for (j=1; j<nSize; j++)
		{
			pCol = m_arrayColumn.GetAt(j);
			if (pCol)
				pListCtrl->SetItemText (i, j, pCol->GetName());
		}
		break;

	case 1: // Source Format
		strItem.LoadString(IDS_SOURCE_FORMAT);
		pListCtrl->SetItemText (i, 0, strItem);
		for (j=1; j<nSize; j++)
		{
			pCol = m_arrayColumn.GetAt(j);
			if (pCol)
			{
				strItem = INGRESII_llIngresColumnType2Str(pCol->GetType(), pCol->GetLength());
				pListCtrl->SetItemText (i, j, strItem);
			}
		}
		break;

	case 2: // Export Format
		strItem.LoadString(IDS_EXPORT_FORMAT);
		pListCtrl->SetItemText (i, 0, strItem);
		for (j=1; j<nSize; j++)
		{
			pCol = m_arrayColumn.GetAt(j);
			if (pCol)
			{
				pListCtrl->SetItemText (i, j, pCol->GetExportFormat());
			}
		}
		break;


	case 3: // Null Exported As:
		strItem.LoadString(IDS_EXPORT_NULL_AS);
		pListCtrl->SetItemText (i, 0, strItem);
		for (j=1; j<nSize; j++)
		{
			pCol = m_arrayColumn.GetAt(j);
			if (pCol)
			{
				pListCtrl->SetItemText (i, j, pCol->GetNull());
			}
		}
		break;

	default: // Unexpected line
		ASSERT(FALSE);
		break;
	}
}

void CaIngresExportHeaderRowItemData::LayoutHeaderXml(CuListCtrlDoubleUpper* pListCtrl, int i)
{
	int j, nSize = m_arrayColumn.GetSize();
	CString strItem;
	CaColumnExport* pCol = NULL;
	switch (i)
	{
	case 0: // Column Name
		strItem.LoadString(IDS_EXPORT_COLUMN);
		pListCtrl->SetItemText (i, 0, strItem);
		for (j=1; j<nSize; j++)
		{
			pCol = m_arrayColumn.GetAt(j);
			if (pCol)
				pListCtrl->SetItemText (i, j, pCol->GetName());
		}
		break;

	case 1: // Source Format
		strItem.LoadString(IDS_SOURCE_FORMAT);
		pListCtrl->SetItemText (i, 0, strItem);
		for (j=1; j<nSize; j++)
		{
			pCol = m_arrayColumn.GetAt(j);
			if (pCol)
			{
				strItem = INGRESII_llIngresColumnType2Str(pCol->GetType(), pCol->GetLength());
				pListCtrl->SetItemText (i, j, strItem);
			}
		}
		break;

	case 2: // Export Format
		strItem.LoadString(IDS_EXPORT_FORMAT);
		pListCtrl->SetItemText (i, 0, strItem);
		for (j=1; j<nSize; j++)
		{
			pCol = m_arrayColumn.GetAt(j);
			if (pCol)
			{
				pListCtrl->SetItemText (i, j, pCol->GetExportFormat());
			}
		}
		break;
	case 3: // Export Length:
		strItem.LoadString(IDS_EXPORT_LENGTH);
		pListCtrl->SetItemText (i, 0, strItem);

		strItem.LoadString(IDS_AUTO);
		for (j=1; j<nSize; j++)
		{
			pCol = m_arrayColumn.GetAt(j);
			if (pCol)
			{
				pListCtrl->SetItemText (i, j, strItem);
			}
		}
		break;

	case 4: // Null Exported As:
		strItem.LoadString(IDS_EXPORT_NULL_AS);
		pListCtrl->SetItemText (i, 0, strItem);
		for (j=1; j<nSize; j++)
		{
			pCol = m_arrayColumn.GetAt(j);
			if (pCol)
			{
				pListCtrl->SetItemText (i, j, pCol->GetNull());
			}
		}
		break;

	default: // Unexpected line
		ASSERT(FALSE);
		break;
	}
}

void CaIngresExportHeaderRowItemData::LayoutHeaderDbf(CuListCtrlDoubleUpper* pListCtrl, int i)
{
	int j, nSize = m_arrayColumn.GetSize();
	CString strItem;
	CaColumnExport* pCol = NULL;
	switch (i)
	{
	case 0: // Column Name
		strItem.LoadString(IDS_EXPORT_COLUMN);
		pListCtrl->SetItemText (i, 0, strItem);
		for (j=1; j<nSize; j++)
		{
			pCol = m_arrayColumn.GetAt(j);
			if (pCol)
				pListCtrl->SetItemText (i, j, pCol->GetName());
		}
		break;

	case 1: // Source Format
		strItem.LoadString(IDS_SOURCE_FORMAT);
		pListCtrl->SetItemText (i, 0, strItem);
		for (j=1; j<nSize; j++)
		{
			pCol = m_arrayColumn.GetAt(j);
			if (pCol)
			{
				strItem = INGRESII_llIngresColumnType2Str(pCol->GetType(), pCol->GetLength());
				pListCtrl->SetItemText (i, j, strItem);
			}
		}
		break;

	case 2: // Export Format
		strItem.LoadString(IDS_EXPORT_FORMAT);
		pListCtrl->SetItemText (i, 0, strItem);
		for (j=1; j<nSize; j++)
		{
			pCol = m_arrayColumn.GetAt(j);
			if (pCol)
			{
				pListCtrl->SetItemText (i, j, pCol->GetExportFormat());
			}
		}
		break;

	case 3: // Column Length
		strItem.LoadString(IDS_EXPORT_LENGTH);
		pListCtrl->SetItemText (i, 0, strItem);
		for (j=1; j<nSize; j++)
		{
			pCol = m_arrayColumn.GetAt(j);
			if (pCol)
			{
				strItem.Format (_T("%d"), pCol->GetExportLength());
				pListCtrl->SetItemText (i, j,strItem);
			}
		}
		break;

	case 4: // Null Exported As:
		strItem.LoadString(IDS_EXPORT_NULL_AS);
		pListCtrl->SetItemText (i, 0, strItem);
		for (j=1; j<nSize; j++)
		{
			pCol = m_arrayColumn.GetAt(j);
			if (pCol)
			{
				pListCtrl->SetItemText (i, j, pCol->GetNull());
			}
		}
		break;

	default: // Unexpected line
		ASSERT(FALSE);
		break;
	}
}

void CaIngresExportHeaderRowItemData::LayoutHeaderIngresCopy(CuListCtrlDoubleUpper* pListCtrl, int i)
{

}

void CaIngresExportHeaderRowItemData::LayoutHeaderFixedWidth(CuListCtrlDoubleUpper* pListCtrl, int i)
{
	int j, nSize = m_arrayColumn.GetSize();
	CString strItem;
	CaColumnExport* pCol = NULL;
	switch (i)
	{
	case 0: // Header Text
		strItem.LoadString(IDS_EXPORT_HEADER_TEXT);
		pListCtrl->SetItemText (i, 0, strItem);
		for (j=1; j<nSize; j++)
		{
			pCol = m_arrayColumn.GetAt(j);
			if (pCol)
				pListCtrl->SetItemText (i, j, pCol->GetName());
		}
		break;

	case 1: // Source Format
		strItem.LoadString(IDS_SOURCE_FORMAT);
		pListCtrl->SetItemText (i, 0, strItem);
		for (j=1; j<nSize; j++)
		{
			pCol = m_arrayColumn.GetAt(j);
			if (pCol)
			{
				strItem = INGRESII_llIngresColumnType2Str(pCol->GetType(), pCol->GetLength());
				pListCtrl->SetItemText (i, j, strItem);
			}
		}
		break;

	case 2: // Export Format
		strItem.LoadString(IDS_EXPORT_FORMAT);
		pListCtrl->SetItemText (i, 0, strItem);
		for (j=1; j<nSize; j++)
		{
			pCol = m_arrayColumn.GetAt(j);
			if (pCol)
			{
				pListCtrl->SetItemText (i, j, pCol->GetExportFormat());
			}
		}
		break;

	case 3: // Field Width
		strItem.LoadString(IDS_EXPORT_LENGTH);
		pListCtrl->SetItemText (i, 0, strItem);
		for (j=1; j<nSize; j++)
		{
			pCol = m_arrayColumn.GetAt(j);
			strItem.Format (_T("%d"), pCol->GetExportLength());
			pListCtrl->SetItemText (i, j, strItem);
		}
		break;

	case 4: // Null Exported As:
		strItem.LoadString(IDS_EXPORT_NULL_AS);
		pListCtrl->SetItemText (i, 0, strItem);
		for (j=1; j<nSize; j++)
		{
			pCol = m_arrayColumn.GetAt(j);
			if (pCol)
			{
				pListCtrl->SetItemText (i, j, pCol->GetNull());
			}
		}
		break;

	default: // Unexpected line
		ASSERT(FALSE);
		break;
	}
}

void CaIngresExportHeaderRowItemData::Display(CuListCtrlDoubleUpper* pListCtrl, int i)
{
	switch (m_exportType)
	{
	case EXPORT_CSV:
		LayoutHeaderCsv(pListCtrl, i);
		break;
	case EXPORT_XML:
		LayoutHeaderXml(pListCtrl, i);
		break;
	case EXPORT_DBF:
		LayoutHeaderDbf(pListCtrl, i);
		break;
	case EXPORT_COPY_STATEMENT:
		LayoutHeaderIngresCopy(pListCtrl, i);
		break;
	case EXPORT_FIXED_WIDTH:
		LayoutHeaderFixedWidth(pListCtrl, i);
		break;
	default:
		break;
	}
}


void CaIngresExportHeaderRowItemData::EditValue (CuListCtrlDoubleUpper* pListCtrl, int iItem, int iSubItem, CRect rcCell)
{
	if (iSubItem == 0)
		return;
	if (!m_bAllowChange)
		return;

	switch (m_exportType)
	{
	case EXPORT_CSV:
		EditValueCsv(pListCtrl, iItem, iSubItem, rcCell);
		break;
	case EXPORT_XML:
		EditValueXml(pListCtrl, iItem, iSubItem, rcCell);
		break;
	case EXPORT_DBF:
		EditValueDbf(pListCtrl, iItem, iSubItem, rcCell);
		break;
	case EXPORT_COPY_STATEMENT:
		EditValueIngresCopy(pListCtrl, iItem, iSubItem, rcCell);
		break;
	case EXPORT_FIXED_WIDTH:
		EditValueFixedWidth(pListCtrl, iItem, iSubItem, rcCell);
		break;
	default:
		break;
	}
}


BOOL CaIngresExportHeaderRowItemData::UpdateValue (CuListCtrlDoubleUpper* pListCtrl, int iItem, int iSubItem, CString& strNewValue)
{
	switch (m_exportType)
	{
	case EXPORT_CSV:
		UpdateValueCsv(pListCtrl, iItem, iSubItem, strNewValue);
		break;
	case EXPORT_XML:
		UpdateValueXml(pListCtrl, iItem, iSubItem, strNewValue);
		break;
	case EXPORT_DBF:
		UpdateValueDbf(pListCtrl, iItem, iSubItem, strNewValue);
		break;
	case EXPORT_COPY_STATEMENT:
		UpdateValueIngresCopy(pListCtrl, iItem, iSubItem, strNewValue);
		break;
	case EXPORT_FIXED_WIDTH:
		if(!UpdateValueFixedWidth(pListCtrl, iItem, iSubItem, strNewValue))
			return FALSE;
		break;
	default:
		break;
	}

	return TRUE;
}


void CaIngresExportHeaderRowItemData::EditValueCsv(CuListCtrlDoubleUpper* pListCtrl, int iItem, int iSubItem, CRect rcCell)
{
	CString strItem;
	CaColumnExport* pCol = m_arrayColumn[iSubItem];

	switch (iItem)
	{
	case 0: // Header Text.
		strItem = pListCtrl->GetItemText (iItem, iSubItem);
		pListCtrl->SetEditText (iItem, iSubItem, rcCell, strItem);
		break;
	case 1: // Source Format
		// Not editable.
		break;
	case 2: // Display Format
		switch (INGRESII_llIngresColumnType2AppType(pCol))
		{
		case INGTYPE_FLOAT:
		case INGTYPE_FLOAT4:
		case INGTYPE_FLOAT8:
		case INGTYPE_DECIMAL:
			/* As the Float4/8 Preferences are available in step 1,
			   there is no need to edit the export format!
			strItem = pListCtrl->GetItemText (iItem, iSubItem);
			pListCtrl->SetEditText (iItem, iSubItem, rcCell, strItem);
			*/
			break;
		default:
			break;
		}

		break;
	case 3: // Null Exported as:
		strItem = pListCtrl->GetItemText (iItem, iSubItem);
		pListCtrl->SetEditText (iItem, iSubItem, rcCell, strItem);
		break;
	default:
		break;
	}
}

void CaIngresExportHeaderRowItemData::EditValueXml(CuListCtrlDoubleUpper* pListCtrl, int iItem, int iSubItem, CRect rcCell)
{
	CString strItem;
	CaColumnExport* pCol = m_arrayColumn[iSubItem];

	switch (iItem)
	{
	case 0: // Column Name
		strItem = pListCtrl->GetItemText (iItem, iSubItem);
		pListCtrl->SetEditText (iItem, iSubItem, rcCell, strItem);
		break;
	case 1: // Source Format
		// Not editable.
		break;
	case 2: // Export Format
		switch (INGRESII_llIngresColumnType2AppType(pCol))
		{
		case INGTYPE_FLOAT:
		case INGTYPE_FLOAT4:
		case INGTYPE_FLOAT8:
		case INGTYPE_DECIMAL:
			/* As the Float4/8 Preferences are available in step 1,
			   there is no need to edit the export format!
			strItem = pListCtrl->GetItemText (iItem, iSubItem);
			pListCtrl->SetEditText (iItem, iSubItem, rcCell, strItem);
			*/
			break;
		default:
			break;
		}

		break;
	case 3: // Export Length:
		break;
	case 4: // Null Exported as:
		strItem = pListCtrl->GetItemText (iItem, iSubItem);
		pListCtrl->SetEditText (iItem, iSubItem, rcCell, strItem);
		break;
	}
}

void CaIngresExportHeaderRowItemData::EditValueDbf(CuListCtrlDoubleUpper* pListCtrl, int iItem, int iSubItem, CRect rcCell)
{
	CString strItem;
	CaColumnExport* pCol = m_arrayColumn[iSubItem];

	switch (iItem)
	{
	case 0: // Column Name
		strItem = pListCtrl->GetItemText (iItem, iSubItem);
		pListCtrl->SetEditText (iItem, iSubItem, rcCell, strItem);
		break;
	case 1: // Source Format
		// Not editable.
		break;
	case 2: // Export Format
#if defined (_CHOICE_DBF_DATATYPE)
		{
			int j;
			const int nMaxType = 4;
			TCHAR tchType[nMaxType][32]=
			{
				_T("C"),
				_T("D"),
				_T("N"),
				_T("L")
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
#endif
		break;
	case 3: // Exported Length:
#if defined (_CHOICE_DBF_DATATYPE)
		strItem = pListCtrl->GetItemText (iItem, iSubItem);
		pListCtrl->SetEditText (iItem, iSubItem, rcCell, strItem);
#endif
		break;
	case 4: // Null Exported as:
		strItem = pListCtrl->GetItemText (iItem, iSubItem);
		pListCtrl->SetEditText (iItem, iSubItem, rcCell, strItem);
		break;
	default:
		break;
	}
}

void CaIngresExportHeaderRowItemData::EditValueIngresCopy(CuListCtrlDoubleUpper* pListCtrl, int iItem, int iSubItem, CRect rcCell)
{

}

void CaIngresExportHeaderRowItemData::EditValueFixedWidth(CuListCtrlDoubleUpper* pListCtrl, int iItem, int iSubItem, CRect rcCell)
{
	CString strItem;
	CaColumnExport* pCol = m_arrayColumn[iSubItem];

	switch (iItem)
	{
	case 0: // Column Name
		strItem = pListCtrl->GetItemText (iItem, iSubItem);
		pListCtrl->SetEditText (iItem, iSubItem, rcCell, strItem);
		break;
	case 1: // Source Format
		// Not editable.
		break;
	case 2: // Export Format
		switch (INGRESII_llIngresColumnType2AppType(pCol))
		{
		case INGTYPE_FLOAT:
		case INGTYPE_FLOAT4:
		case INGTYPE_FLOAT8:
		case INGTYPE_DECIMAL:
			/* As the Float4/8 Preferences are available in step 1,
			   there is no need to edit the export format!
			strItem = pListCtrl->GetItemText (iItem, iSubItem);
			pListCtrl->SetEditText (iItem, iSubItem, rcCell, strItem);
			*/
			break;
		default:
			break;
		}

		break;
	case 3: // Export Length:
		strItem = pListCtrl->GetItemText (iItem, iSubItem);
		pListCtrl->SetEditText (iItem, iSubItem, rcCell, strItem);
		break;
	case 4: // Null Exported as:
		strItem = pListCtrl->GetItemText (iItem, iSubItem);
		pListCtrl->SetEditText (iItem, iSubItem, rcCell, strItem);
		break;
	}
}


void CaIngresExportHeaderRowItemData::UpdateValueCsv(CuListCtrlDoubleUpper* pListCtrl, int iItem, int iSubItem, CString& strNewValue)
{
	if (iSubItem == 0)
		return;
	CString strItem;
	CaColumnExport* pCol = m_arrayColumn.GetAt (iSubItem);
	if (!pCol)
		return;
	switch (iItem)
	{
	case 0: // Header Text.
		pCol->SetItem(strNewValue);
		break;
	case 2:
		pCol->SetExportFormat(strNewValue);
		break;
	case 3: // Null Exported as:
		pCol->SetNull(strNewValue);
		break;
	default:
		break;
	}
}

void CaIngresExportHeaderRowItemData::UpdateValueXml(CuListCtrlDoubleUpper* pListCtrl, int iItem, int iSubItem, CString& strNewValue)
{
	if (iSubItem == 0)
		return;
	CString strItem;
	CaColumnExport* pCol = m_arrayColumn.GetAt (iSubItem);
	if (!pCol)
		return;
	switch (iItem)
	{
	case 0: // Column Name.
		pCol->SetItem(strNewValue);
		break;
	case 2:
		pCol->SetExportFormat(strNewValue);
		break;
	case 3: // Export Length:
		if (!strNewValue.IsEmpty())
		{
			int nLen = _ttoi(strNewValue);
			if (nLen > 0)
			{
				pCol->SetExportLength(nLen);
				//update the input parameter so that we display the correct previous unedited value.
				strNewValue.Format("%d",nLen);
			}
			else
			{
				AfxMessageBox (IDS_MSG_POSITIVE_INTEGER_ONLY, MB_OK|MB_ICONEXCLAMATION);
				//update the input parameter so that we display the correct previous unedited value.
				strNewValue.Format("%d",pCol->GetExportLength()); 
			}
		}
		break;
	case 4: // Null Exported as:
		pCol->SetNull(strNewValue);
		break;
	default:
		break;
	}
}

void CaIngresExportHeaderRowItemData::UpdateValueDbf(CuListCtrlDoubleUpper* pListCtrl, int iItem, int iSubItem, CString& strNewValue)
{
	if (iSubItem == 0)
		return;
	CString strItem;
	CaColumnExport* pCol = m_arrayColumn.GetAt (iSubItem);
	if (!pCol)
		return;
	switch (iItem)
	{
	case 0: // Column Name.
		pCol->SetItem(strNewValue);
		break;
	case 2:
		pCol->SetExportFormat(strNewValue);
		break;
	case 3: // Export Length:
		if (!strNewValue.IsEmpty())
		{
			int nLen = _ttoi(strNewValue);
			if (nLen > 0)
			{
				pCol->SetExportLength(nLen);
				//update the input parameter so that we display the correct previous unedited value.
				strNewValue.Format("%d",nLen);
			}
			else
			{
				AfxMessageBox (IDS_MSG_POSITIVE_INTEGER_ONLY, MB_OK|MB_ICONEXCLAMATION);
				//update the input parameter so that we display the correct previous unedited value.
				strNewValue.Format("%d",pCol->GetExportLength()); 
			}
		}
		break;
	case 4: // Null Exported as:
		pCol->SetNull(strNewValue);
		break;
	default:
		break;
	}
}

void CaIngresExportHeaderRowItemData::UpdateValueIngresCopy(CuListCtrlDoubleUpper* pListCtrl, int iItem, int iSubItem, CString& strNewValue)
{

}

BOOL CaIngresExportHeaderRowItemData::UpdateValueFixedWidth(CuListCtrlDoubleUpper* pListCtrl, int iItem, int iSubItem, CString& strNewValue)
{
	BOOL bRetVal = TRUE;
	if (iSubItem == 0)
		return bRetVal;
	CString strItem;
	CaColumnExport* pCol = m_arrayColumn.GetAt (iSubItem);
	if (!pCol)
		return bRetVal;
	switch (iItem)
	{
	case 0: // Header Text.
		pCol->SetItem(strNewValue);
		break;
	case 2:
		pCol->SetExportFormat(strNewValue);
		break;
	case 3: // Export Length:
		if (!strNewValue.IsEmpty())
		{
			int nLen = _ttoi(strNewValue);
			if (nLen > 0)
			{
				pCol->SetExportLength(nLen);
				//update the input parameter so that we display the correct previous unedited value.
				strNewValue.Format("%d",nLen);
			}
			else
			{
				AfxMessageBox (IDS_MSG_POSITIVE_INTEGER_ONLY, MB_OK|MB_ICONEXCLAMATION);
				//update the input parameter so that we display the correct previous unedited value.
				strNewValue.Format("%d",pCol->GetExportLength()); 
				bRetVal = FALSE;
			}
		}
		break;
	case 4: // Null Exported as:
		pCol->SetNull(strNewValue);
		break;
	default:
		break;
	}
	return bRetVal;
}


