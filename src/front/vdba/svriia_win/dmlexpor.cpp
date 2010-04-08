/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmlexpor.cpp: implementation file
**    Project  : EXPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Manipulation of data to be exported
**
** History:
**	16-Jan-2002 (uk$so01)
**	    Created
**	    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
**	26-Feb-2003 (uk$so01)
**	    SIR  #106952, Date conversion and numeric display format.
**	17-Jul-2003 (uk$so01)
**	    SIR #106648, Vdba-Split, ensure that the ingres sessions 
**	    have the descriptions.
**	22-Aug-2003 (uk$so01)
**	    SIR #106648, Add silent mode in IEA. 
**	02-feb-2004 (somsa01)
**	    Changed CFile::WriteHuge()/ReadHuge() to CFile::Write()/Read(), as
**	    WriteHuge()/ReadHuge() is obsolete and in WIN32 programming
**	    Write()/Read() can also write more than 64K-1 bytes of data.
** 17-Mar-2004 (uk$so01)
**    BUG #111971 / ISSUE 13305363
**    Remove the online editing of "export format" for float/decimal
**    data types.
** 22-Mar-2004 (uk$so01)
**    BUG #111995 / ISSUE 13314121
**    The DBF file has limitation. IEA should display the message when 
**    the limitaion has been exceeded:
**    Column length     = 254
**    Record length     = 4000
**    Number of columns = 128
** 26-Apr-2004 (uk$so01)
**    BUG #111995 / ISSUE 13314121 (additional fixes to change #467343)
** 12-May-2004 (uk$so01)
**    SIR  #106952, Additional change: when exporting to DBF file, don't
**    put 'n/a' as default for N,D, data type if the value from table is (null),
**    instead put the empty string '' as default.
** 17-May-2004 (schph01)
**    SIR 111507 Add management for new column type bigint(int8)
** 14-Oct-2004 (uk$so01)
**    BUG #113226 / ISSUE 13720088: The DBMS connections should not be in the cache
**    for IIA & IEA.
** 21-May-2008 (smeke01) b118452
**    Remove unwanted extra space between columns in Fixed Width export mode.
*/

#include "stdafx.h"
#include "svriia.h"
#include "dmlexpor.h"
#include "taskanim.h"
#include "lsselres.h"
#include "sqlselec.h"
#include "constdef.h"
#include "ingobdml.h" // INGRESII Low level
#include "datecnvt.h"
#include "environm.h"
#include "wmusrmsg.h"
#include "eapstep2.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

class CaTempSession // Release a session when out of scope:
{
public:
	CaTempSession(CaSession* pSession){m_pSession = pSession;}
	~CaTempSession()
	{
		if (m_pSession)
		{
			m_pSession->Activate();
			m_pSession->Release(SESSION_ROLLBACK);
		}
	}
private:
	CaSession* m_pSession;

};

static BOOL DoExport (CaIEAInfo* pIEAInfo, CaGenerator* lpGenerator, PfnUserManageQueryResult pGenerateFileFunction);
//
// Base class CaGenerator: base class for generating row of CSV, DBF, XML, ...
// ************************************************************************************************
class CaGeneratorExport: public CaGenerator
{
public:
	CaGeneratorExport(CaIEAInfo* pInfo): m_pIeaInfo(pInfo){}
	~CaGeneratorExport()
	{
	}
protected:
	CaIEAInfo* m_pIeaInfo;
};


//
// CLASS: CaExecParamQueryRows4Export (manage the animation of exporting row)
// ************************************************************************************************
class CaExecParamQueryRows4Export: public CaExecParamSelectCursor
{
public:
	CaExecParamQueryRows4Export(UINT nInteruptType = INTERRUPT_USERPRECIFY);
	virtual ~CaExecParamQueryRows4Export();

	virtual int Run(HWND hWndTaskDlg = NULL);
	virtual void OnTerminate(HWND hWndTaskDlg = NULL);
	virtual BOOL OnCancelled(HWND hWndTaskDlg = NULL);

	int GetAccumulation(){return m_nAccumulation;}
protected:

};


CaExecParamQueryRows4Export::CaExecParamQueryRows4Export(UINT nInterupted):CaExecParamSelectCursor(nInterupted)
{
}

CaExecParamQueryRows4Export::~CaExecParamQueryRows4Export()
{
}


int CaExecParamQueryRows4Export::Run(HWND hWndTaskDlg)
{
	return CaExecParamSelectCursor::Run(hWndTaskDlg);
}

void CaExecParamQueryRows4Export::OnTerminate(HWND hWndTaskDlg)
{
	::SendMessage(m_hWnd, WMUSRMSG_SQL_FETCH, (LPARAM)m_nEndFetchStatus, 0);
	switch (m_nEndFetchStatus)
	{
	case CaExecParamQueryRows::FETCH_ERROR:
		if (!m_strMsgFailed.IsEmpty())
		{
			::MessageBox(hWndTaskDlg, m_strMsgFailed, m_strMsgBoxTitle, MB_ICONEXCLAMATION | MB_OK);
		}
		break;
	case CaExecParamQueryRows::FETCH_NORMAL_ENDING:
		break;
	case CaExecParamQueryRows::FETCH_INTERRUPTED:
		break;
	default:
		break;
	}
}

BOOL CaExecParamQueryRows4Export::OnCancelled(HWND hWndTaskDlg)
{
	m_synchronizeInterrupt.BlockUserInterface(TRUE);    // Block itself;
	m_synchronizeInterrupt.SetRequestCancel(TRUE);      // Request the cancel.
	m_synchronizeInterrupt.BlockWorkerThread(TRUE);     // Block worker thread (Run() method);
	m_synchronizeInterrupt.WaitUserInterface();         // Wait itself until worker thread allows to do something;

	CString strMsg;
	strMsg.Format (m_strInterruptFetching, GetAccumulation());
	int nRes = ::MessageBox(hWndTaskDlg, strMsg, m_strMsgBoxTitle, MB_ICONQUESTION | MB_YESNOCANCEL);
	m_synchronizeInterrupt.BlockWorkerThread(FALSE);    // Release worker thread (Run() method);
	m_synchronizeInterrupt.SetRequestCancel(FALSE);     // Un-Request the cancel.
	if (nRes == IDYES || nRes == IDNO)
	{
		SetInterrupted (TRUE);
		if (nRes == IDNO)
		{
			//
			// To indicate not to save the file
			m_nEndFetchStatus = CaExecParamQueryRows::FETCH_ERROR;
		}
		
		return TRUE;
	}
	else
		return FALSE;
}


//
// Pre-fetch for displaying rows section:
// ************************************************************************************************
void CALLBACK StoreResultSet (LPVOID lpUserData, CaRowTransfer* pResultRow)
{
	CaIeaDataPage1* pDataPage1 = (CaIeaDataPage1*)lpUserData;
	CaQueryResult& result = pDataPage1->GetQueryResult();
	CPtrArray& listRow = result.GetListRecord();

	listRow.Add(pResultRow);
}


BOOL IEA_FetchRows (CaIEAInfo* pIEAInfo, HWND hwndCaller)
{
	CaIeaDataPage1& dataPage1 = pIEAInfo->m_dataPage1;
	CaSession* pCurrentSession = NULL;
	CaExecParamQueryRows4Export* pQueryRowParam = NULL;

	try
	{
		//
		// Check if we need the new session:
		CmtSessionManager& sessionMgr = theApp.GetSessionManager();
		CaSession session;
		session.SetFlag (0);
		session.SetIndependent(TRUE);
		session.SetNode(dataPage1.GetNode());
		session.SetDatabase(dataPage1.GetDatabase());
		session.SetServerClass(dataPage1.GetServerClass());
		session.SetUser(_T(""));
		session.SetOptions(_T(""));
		session.SetDescription(sessionMgr.GetDescription());

		SETLOCKMODE lockmode;
		memset (&lockmode, 0, sizeof(lockmode));
		lockmode.nReadLock = LM_NOLOCK;
		if (!pCurrentSession)
		{
			pCurrentSession = sessionMgr.GetSession(&session);
			if (pCurrentSession)
			{
				pCurrentSession->Activate();
				pCurrentSession->Commit();
				pCurrentSession->SetLockMode(&lockmode);
				pCurrentSession->Commit();
			}
		}

		if (!pCurrentSession)
			return FALSE;
		CaTempSession tempSession(pCurrentSession);
		CaCursor* pCursor = new CaCursor (theApp.GetCursorSequenceNumber(), dataPage1.GetStatement());
		CaMoneyFormat& moneyFormat = pCursor->GetMoneyFormat();
		CaCursorInfo& cursorInfo = pCursor->GetCursorInfo();
		if (dataPage1.GetExportedFileType() == EXPORT_DBF)
		{
			//
			// Export to the N data type in the DBF file, the money symbol cannot be handled:
			moneyFormat.SetUsedMoneySymbol(FALSE);
		}
		cursorInfo = dataPage1.GetFloatFormat();

		CString strTitle;
		CString strFetchInfo;
		strTitle.LoadString(IDS_TITLE_EXPORT);
		strFetchInfo.LoadString(IDS_FETCHING_ROWS);

		pQueryRowParam = new CaExecParamQueryRows4Export(INTERRUPT_NOT_ALLOWED);
		pQueryRowParam->SetCursor (pCursor);
		pQueryRowParam->SetSession (pCurrentSession);
		pQueryRowParam->SetRowLimit (100);
		pQueryRowParam->SetStrings(strTitle, _T(""), strFetchInfo);
		pQueryRowParam->SetUserHandlerResult((LPVOID)&dataPage1, StoreResultSet);

		// 
		// Just open the cursor to get the characteristics of columns:
		pQueryRowParam->OpenCursor();

		CTypedPtrList< CObList, CaColumn* >& listLeader = pCursor->GetListHeader();
		//
		// Check if all columns can be exported, otherwise show the message error and 
		// cancel the export operation:
		POSITION pos = listLeader.GetHeadPosition();
		while (pos != NULL)
		{
			CaColumn* pCol = listLeader.GetNext(pos);
			switch (INGRESII_llIngresColumnType2AppType(pCol))
			{
			case INGTYPE_ERROR:
			case INGTYPE_LONGVARCHAR:
			case INGTYPE_BYTE:
			case INGTYPE_BYTEVAR:
			case INGTYPE_LONGBYTE:
			case INGTYPE_SECURITYLBL:
			case INGTYPE_SHORTSECLBL:
			case INGTYPE_UNICODE_NCHR:
			case INGTYPE_UNICODE_NVCHR:
			case INGTYPE_UNICODE_LNVCHR:
				// _T("The result of select contains data type that cannot be exported."));
				AfxMessageBox (IDS_MSG_CANNOT_EXPORT_DATATYPE);
				delete pQueryRowParam;
				return FALSE;
				break;
			default:
				break;
			}
		}

		CaQueryResult& result = dataPage1.GetQueryResult();
		ConfigureLayout (dataPage1.GetExportedFileType(), result, &listLeader);

		//
		// Disable the current session so that the thread can re-activate it:
		pCurrentSession->SetSessionNone();

#if defined (_ANIMATION_)
		strTitle.LoadString(IDS_TITLE_EXPORT_ANIMATION_PREFETCH);
		CxDlgWait dlgWait(strTitle, NULL);
		dlgWait.SetDeleteParam(FALSE);
		dlgWait.SetExecParam (pQueryRowParam);
		dlgWait.SetUseAnimateAvi(AVI_FETCHR);
		dlgWait.SetUseExtraInfo();
		dlgWait.SetShowProgressBar(FALSE);
		dlgWait.SetHideCancelBottonAlways(TRUE);
		dlgWait.DoModal();
#else
		pQueryRowParam->Run();
		pQueryRowParam->OnTerminate(hwndCaller);
#endif
		BOOL bInterrupted = pQueryRowParam->IsInterrupted();
		delete pQueryRowParam;
		pQueryRowParam = NULL;
		SetForegroundWindow (hwndCaller);
		if (bInterrupted)
		{
			return FALSE;
		}
		return TRUE;
	}
	catch (CeSqlException e)
	{
		AfxMessageBox(e.GetReason());
	}
	catch(...)
	{

	}
	if (pQueryRowParam)
		delete pQueryRowParam;
	return FALSE;
}


//
// Export to CSV section:
// ************************************************************************************************
class CaGenerateCsv: public CaGeneratorExport
{
public:
	CaGenerateCsv(CaIEAInfo* pInfo):CaGeneratorExport(pInfo) {}
	~CaGenerateCsv(){}
	virtual void Out(CaRowTransfer* pRow);
	virtual void End(BOOL bSuccess = TRUE);

	void ConstructRow (CaRowTransfer* pRow, CString& strLine);
	CString FieldValue (CString& strField, CaIeaDataPage2* pDataPage2, BOOL bIsNull, CString strNull);
};


void CaGenerateCsv::Out(CaRowTransfer* pRow)
{
	CaIeaDataPage1& dataPage1 = m_pIeaInfo->m_dataPage1;
	CaIeaDataPage2& dataPage2 = m_pIeaInfo->m_dataPage2;
	CString strLine;
	if (!m_bFileCreated)
	{
		//
		// Check to see if header of the select statement has been changed
		// since the last pre-fetched for displaying :
		CaQueryResult& previewResult = dataPage1.GetQueryResult();
		CTypedPtrList< CObList, CaColumn* >& listColumn = previewResult.GetListColumn();
		ASSERT(m_listColumn.GetCount() == listColumn.GetCount());
		if (m_listColumn.GetCount() != listColumn.GetCount())
		{
			//
			// The result of the select statement has been changed since
			// the last fetch for previewing:
			m_nIDSError = IDS_MSG_EXPORT_RESULT_CHANGE;
			throw (int)1;
		}

		//
		// Create csv file.
		m_pFile = new CFile (dataPage1.GetFile2BeExported(), CFile::modeCreate|CFile::modeWrite);
		m_bFileCreated = TRUE;

		//
		// Export CSV Header one line in the file:[ {<column name><separator>...<column name>}+ ]
		if (dataPage2.IsExportHeader())
		{
			CTypedPtrList< CObList, CaColumnExport* >& listPreviewColumn = previewResult.GetListColumnLayout();
			POSITION pos = listPreviewColumn.GetHeadPosition();
			CaRowTransfer* pHeaderRow = new CaRowTransfer();
			CStringList& listHeader = pHeaderRow->GetRecord();
			CArray <UINT, UINT>& arrayNullIndicator = pHeaderRow->GetArrayFlag();
			arrayNullIndicator.SetSize(m_listColumn.GetCount());
			for (int i=0; i<m_listColumn.GetCount(); i++)
				arrayNullIndicator[i] = 0;
			while (pos != NULL)
			{
				CaColumnExport* pCol = listPreviewColumn.GetNext(pos);
				listHeader.AddTail(pCol->GetName());
			}
		
			ConstructRow (pHeaderRow, strLine);
			//
			// Write the CSV row into the CSV file:
			strLine+= consttchszReturn;
			m_pFile->Write((LPCTSTR)strLine, strLine.GetLength());
			delete pHeaderRow;
		}
	}

	ConstructRow (pRow, strLine);
	if (!strLine.IsEmpty())
	{
		//
		// Write the CSV row into the CSV file:
		strLine+= consttchszReturn;
		m_pFile->Write((LPCTSTR)strLine, strLine.GetLength());
	}
}

void CaGenerateCsv::End(BOOL bSuccess)
{
	if (m_pFile)
	{
		if (bSuccess)
		{
			m_pFile->Flush();
			delete m_pFile;
		}
		else
		{
			CString strFileName = m_pFile->GetFilePath();
			delete m_pFile;
			DeleteFile(strFileName);
		}
	}
	m_pFile = NULL;
}

void CaGenerateCsv::ConstructRow (CaRowTransfer* pRow, CString& strLine)
{
	TCHAR tchszTab = _T('\t');
	strLine = _T("");
	CaIeaDataPage1& dataPage1 = m_pIeaInfo->m_dataPage1;
	CaIeaDataPage2& dataPage2 = m_pIeaInfo->m_dataPage2;
	CStringList& strlListTuple = pRow->GetRecord();
	CArray <UINT, UINT>& arrayNullIndicator = pRow->GetArrayFlag();

	CaQueryResult& previewResult = dataPage1.GetQueryResult();
	CTypedPtrList< CObList, CaColumnExport* >& listPreviewColumn = previewResult.GetListColumnLayout();
	ASSERT(listPreviewColumn.GetCount() == strlListTuple.GetCount());
	if (listPreviewColumn.GetCount() != strlListTuple.GetCount())
	{
		//
		// The result of the select statement has been changed since
		// the last fetch for previewing:
		AfxMessageBox (IDS_MSG_EXPORT_RESULT_CHANGE);
		return;
	}

	POSITION posField  = strlListTuple.GetHeadPosition();
	POSITION posColumn = listPreviewColumn.GetHeadPosition();
	TCHAR tchszSep = dataPage2.GetDelimiter();
	BOOL bOne = TRUE;
	int nIndex = 0;
	//
	// Each field (strField) is corresponding (one-to-one) to a header (pCol):
	while (posField != NULL && posColumn != NULL)
	{
		CaColumnExport* pCol = listPreviewColumn.GetNext(posColumn);
		CString strField = strlListTuple.GetNext(posField);
		//
		// UKS: convert field to reflect the preview of column layout.
		BOOL bIsNull = ((arrayNullIndicator[nIndex] & CAROW_FIELD_NULL) != 0);
		if (theApp.m_strNullNA.CompareNoCase (pCol->GetNull()) == 0)
			bIsNull = FALSE;
		if (bOne)
			strLine += FieldValue(strField, &dataPage2, bIsNull, pCol->GetNull());
		else
		{
			strLine += tchszSep;
			strLine += FieldValue(strField, &dataPage2, bIsNull, pCol->GetNull());
		}

		bOne = FALSE;
		nIndex++;
	}
}

CString CaGenerateCsv::FieldValue (CString& strField, CaIeaDataPage2* pDataPage2, BOOL bIsNull, CString strNull)
{
	BOOL bAlwaysQuote  = !pDataPage2->IsQuoteIfDelimiter();
	TCHAR tchszSep = pDataPage2->GetDelimiter();
	TCHAR tchszTQ = pDataPage2->GetQuote();

	int nExistSeparator = bIsNull? strNull.Find (tchszSep): strField.Find (tchszSep);
	BOOL b2Quote = bAlwaysQuote || (nExistSeparator != -1);

	if (b2Quote)
	{
		int nExistTQ = bIsNull? strNull.Find(tchszTQ): strField.Find(tchszTQ);
		if (nExistTQ != -1)
		{
			// 
			// Any quote in the field is quoted by itself. Ex: AB"DC => AB""CD
			CString strOldTQ = tchszTQ;
			CString strNewTQ = tchszTQ;

			strNewTQ += tchszTQ;
			if (bIsNull)
				strNull.Replace (strOldTQ, strNewTQ);
			else
				strField.Replace (strOldTQ, strNewTQ);
		}

		CString strValue;
		if (bIsNull)
			strValue.Format (_T("%c%s%c"), tchszTQ, (LPCTSTR)strNull, tchszTQ);
		else
			strValue.Format (_T("%c%s%c"), tchszTQ, (LPCTSTR)strField, tchszTQ);
		return strValue;
	}
	else
	{
		return bIsNull? strNull: strField;
	}
}

void CALLBACK GenerateCsvFile (LPVOID lpUserData, CaRowTransfer* pResultRow)
{
	CaGenerateCsv* pData = (CaGenerateCsv*)lpUserData;
	pData->Out(pResultRow);
	delete pResultRow;
}


BOOL ExportToCsv(CaIEAInfo* pIEAInfo)
{
	CaGenerateCsv generateCsv (pIEAInfo);
	BOOL bOK = DoExport (pIEAInfo, &generateCsv, GenerateCsvFile);
	return bOK;
}


//
// Export to DBF section:
// ************************************************************************************************
class CaGenerateDbf: public CaGeneratorExport
{
public:
	CaGenerateDbf(CaIEAInfo* pInfo);
	~CaGenerateDbf(){}
	virtual void Out(CaRowTransfer* pRow);
	virtual void End(BOOL bSuccess = TRUE);
	void UpdateRecordCount (DWORD dwRecordCount, FILE* pFile);
protected:
	DWORD m_dwRecordExported;
	IIDateType m_inputDateFormat;
	int m_nCentury;
};

CaGenerateDbf::CaGenerateDbf(CaIEAInfo* pInfo):CaGeneratorExport(pInfo)
{
	m_dwRecordExported = 0;
	m_inputDateFormat = II_DATE_UNKNOWN;
	m_nCentury = 0;

	CTypedPtrList<CObList, CaNameValue*> lev;
	CaNameValue* pEnvIIDateFormat  = new CaNameValue(_T("II_DATE_FORMAT"), _T(""));
	CaNameValue* pEnvIIDateCentury = new CaNameValue(_T("II_DATE_CENTURY_BOUNDARY"), _T(""));

	lev.AddTail(pEnvIIDateFormat);
	lev.AddTail(pEnvIIDateCentury);
	INGRESII_CheckVariable (lev);

	CString strCentury = pEnvIIDateCentury->GetValue();
	CString strIIDate = pEnvIIDateFormat->GetValue();
	m_nCentury = strCentury.IsEmpty()? 0: _ttoi(strCentury);
	if (strIIDate.IsEmpty())
		m_inputDateFormat = II_DATE_US; // Default to US if II_DATE_FORMAT is not set
	else
		INGRESII_IIDateFromString(strIIDate, m_inputDateFormat);
	while (!lev.IsEmpty())
		delete lev.RemoveHead();
}


//
// dBASE III:
//      Record Length       = 4000 max
//      Number of fields    =  128 max
//      Field length        =  254 max
//      Date                =    8 bytes
void CaGenerateDbf::Out(CaRowTransfer* pRow)
{
	USES_CONVERSION;
	CaIeaDataPage1& dataPage1 = m_pIeaInfo->m_dataPage1;
	CaIeaDataPage2& dataPage2 = m_pIeaInfo->m_dataPage2;
	CString strItem;
	UINT nByte2Write = 1;
	BYTE nByte = 3;
	BYTE nByteArray[32];
	char szBuff[32];
	POSITION pos = NULL;
	CaQueryResult& previewResult = dataPage1.GetQueryResult();
	CTypedPtrList< CObList, CaColumnExport* >& listColumnLayout = previewResult.GetListColumnLayout();

	if (!m_bFileCreated)
	{
		//
		// Check to see if header of the select statement has been changed
		// since the last pre-fetched for displaying :
		CTypedPtrList< CObList, CaColumn* >& listColumn = previewResult.GetListColumn();
		ASSERT(m_listColumn.GetCount() == listColumn.GetCount());
		if (m_listColumn.GetCount() != listColumn.GetCount())
		{
			//
			// The result of the select statement has been changed since
			// the last fetch for previewing:
			m_nIDSError = IDS_MSG_EXPORT_RESULT_CHANGE;
			throw (int)1;
		}

		//
		// Calculate record length:
		WORD wRecLength = 1; // +1 because of a 0x20 or 0x2A for delete mark
		pos = listColumnLayout.GetHeadPosition();
		while (pos != NULL)
		{
			CaColumnExport* pCol = listColumnLayout.GetNext(pos);
			wRecLength += (WORD)pCol->GetExportLength();
		}

		//
		// Create DBF file.
		m_pFile = new CFile (dataPage1.GetFile2BeExported(), CFile::modeCreate|CFile::modeWrite);
		//
		// Generate the DBF Header file:
		// See the information in URL: http://community.borland.com/article/print/0,1772,15838,00.html

		// Version number:
		nByte2Write = 1;
		nByte = 3;
		m_pFile->Write ((const void*)&nByte, nByte2Write);

		// Date of last update:
		nByte2Write = 3;
		CTime t = CTime::GetCurrentTime();
		strItem = t.Format( _T("%y%m%d") );
		strItem = strItem.Left(2);
		nByteArray[0] = (BYTE)(_ttoi(strItem) + 100); // for year 03, we write 103
		nByteArray[1] = (BYTE)t.GetMonth();
		nByteArray[2] = (BYTE)t.GetDay();
		m_pFile->Write ((const void*)nByteArray, nByte2Write);

		// Number of records in the file:
		nByte2Write = 4;
		DWORD dwRecordNum = 1; 
		m_pFile->Write ((const void*)&dwRecordNum, nByte2Write);

		// Number of bytes in the header: 32 + 32*(number of fields) +1
		nByte2Write = 2;
		WORD wHeaderSize = 32 + 32 * m_listColumn.GetCount() + 1; 
		m_pFile->Write ((const void*)&wHeaderSize, nByte2Write);

		// Number of bytes in the record:
		nByte2Write = 2;
		WORD wRecordSize = wRecLength;
		m_pFile->Write ((const void*)&wRecordSize, nByte2Write);

		// Reserved bytes: (20, 3 = Reserved, 13 = for dBASE III PLUS on a LAN, 4 = reserved)
		memset (szBuff, 0, sizeof(szBuff));
		nByte2Write = 20;
		m_pFile->Write ((const void*)szBuff, nByte2Write);

		// 32-bytes each for field descriptor:
		char szFieldName[12];
		pos = listColumnLayout.GetHeadPosition();
		while (pos != NULL)
		{
			memset (szFieldName, 0, sizeof(szFieldName));
			CaColumnExport* pCol = listColumnLayout.GetNext(pos);

			// Field name in ASCII (zero filled):
			CString strColName = pCol->GetName();
			char* pFieldName = T2A((LPTSTR)(LPCTSTR)strColName);
			int i, nLen = strlen (pFieldName);
			for (i=0; (i<nLen && i<11); i++)
			{
				szFieldName[i] = pFieldName[i];
			}
			int k = i;
			for (i=k; (i<11); i++)
				szFieldName[i] = '\0';
			szFieldName[11] = '\0';
			nByte2Write = 11;
			m_pFile->Write ((const void*)szFieldName, nByte2Write);

			// Field type in ASCII (C, D, L, M, or N):
			nByte2Write = 1;
			memset (szBuff, 0, sizeof(szBuff));
			lstrcpy (szBuff, pCol->GetExportFormat());
			m_pFile->Write ((const void*)&szBuff[0], nByte2Write);

			// Field data address (address is set on memory; not useful, disk):
			nByte2Write = 4;
			DWORD dwAddress = 0;
			m_pFile->Write ((const void*)&dwAddress, nByte2Write);

			// Field length in binary:
			nByte2Write = 1;
			nByte = pCol->GetExportLength();
			m_pFile->Write ((const void*)&nByte, nByte2Write);

			// Field decimal count in binary:
			nByte2Write = 1;
			switch (INGRESII_llIngresColumnType2AppType(pCol))
			{
			case INGTYPE_FLOAT:
			case INGTYPE_FLOAT4:
			case INGTYPE_FLOAT8:
			case INGTYPE_DECIMAL:
				nByte = pCol->GetScale();
				break;
			case INGTYPE_MONEY:
				nByte = 2;
				break;
			default:
				nByte = 0;
			}
			m_pFile->Write ((const void*)&nByte, nByte2Write);

			// 
			// Reserved bytes (14). 
			// (2 = dBASE III PLUS on a LAN, 1 = Work area ID, 2 = dBASE III PLUS on a LAN,
			//  1 = Set Field Flags, 7 = Reserved:)
			nByte2Write = 14;
			memset (nByteArray, 0, sizeof(nByteArray));
			m_pFile->Write ((const void*)nByteArray, nByte2Write);
		}

		// 0Dh stored as field terminator:
		nByte2Write = 1;
		szBuff[0] = 0x0D;
		m_pFile->Write ((const void*)szBuff, nByte2Write);

		m_bFileCreated = TRUE;
	}


	CStringList& listFields = pRow->GetRecord();
	POSITION posField  = listFields.GetHeadPosition();
	POSITION posColumn = listColumnLayout.GetHeadPosition();
	
	TCHAR BuffField[256];
	TCHAR BuffField2[512];
	//
	// Write the record marker:
	nByte2Write = 1; // Marker of record (0x20 or 0x2A for delete mark)
	nByte = 0x20;
	m_pFile->Write((const void*)&nByte, nByte2Write);

	int nIndex = 0;
	CArray <UINT, UINT>& arrayNullIndicator = pRow->GetArrayFlag();
	//
	// Each field (strField) is corresponding (one-to-one) to a header (pCol):
	while (posField != NULL && posColumn != NULL)
	{
		CaColumnExport* pCol = listColumnLayout.GetNext(posColumn);
		CString strField = listFields.GetNext(posField);
		//
		// UKS: convert field to reflect the preview of column layout.
		BOOL bIsNull = ((arrayNullIndicator[nIndex] & CAROW_FIELD_NULL) != 0);
		if (theApp.m_strNullNA.CompareNoCase (pCol->GetNull()) == 0)
			bIsNull = FALSE;

		//
		// Each field is stored as fixed length
		int nFieldLength = pCol->GetExportLength();
		if (pCol->GetExportFormat().CompareNoCase (_T("N")) == 0)
		{
			nFieldLength = 19;
		}
		else
		if (pCol->GetExportFormat().CompareNoCase (_T("D")) == 0)
		{
			nFieldLength = 8;
			//
			// Convert field to date format ISO4 (yyyymmdd)
			if (m_inputDateFormat != II_DATE_UNKNOWN)
			{
				INGRESII_ConvertDate (strField, m_inputDateFormat, II_DATE_ISO4, m_nCentury);
			}
		}
		BuffField[0] = _T('\0');

		if (!bIsNull)
		{
			lstrcpy (BuffField, strField);
		}
		else
		{
			CString strNull = pCol->GetNull();
			lstrcpy (BuffField, strNull);
		}
		for (int i=lstrlen(BuffField); (i<nFieldLength); i++)
			BuffField[i] = _T(' ');
		BuffField[nFieldLength] = _T('\0');
		CharToOem(BuffField, BuffField2);
		m_pFile->Write ((const void*)BuffField2, nFieldLength);
		nIndex++;
	}

	m_dwRecordExported++;
}

void CaGenerateDbf::End(BOOL bSuccess)
{
	if (m_pFile)
	{
		if (bSuccess)
		{
			CString strFile = m_pFile->GetFilePath();
			m_pFile->Flush();
			delete m_pFile;

			FILE* pFile = _tfopen (strFile, _T("r+"));
			if (pFile != NULL)
			{
				UpdateRecordCount (m_dwRecordExported, pFile);
				fclose (pFile);
			}
		}
		else
		{
			CString strFileName = m_pFile->GetFilePath();
			delete m_pFile;
			DeleteFile(strFileName);
		}
	}
	m_pFile = NULL;
}

// 
// We don't know the total number of records to be exported in advance.
// This function is called at the end of the export operation to modify the number of records
// in the file.
void CaGenerateDbf::UpdateRecordCount (DWORD dwRecordCount, FILE* pFile)
{
	int  nByte = 0;
	//
	// 1 byte for version and 3 bytes for date of last update
	fseek (pFile, 4, 0);
	//
	// Number of records Byte [4 7] (4 bytes):
	nByte = fwrite ((void*)&dwRecordCount, sizeof(DWORD), 1, pFile); 
	fseek (pFile, 0, SEEK_END);
	BYTE bf = 0x1A; // found this at the end of DBF file that works well, so I just write '0x1A' at the end of file.
	nByte = fwrite ((void*)&bf, sizeof(BYTE), 1, pFile); 
	fflush (pFile);
}

void CALLBACK GenerateDbfFile (LPVOID lpUserData, CaRowTransfer* pResultRow)
{
	CaGenerateDbf* pData = (CaGenerateDbf*)lpUserData;
	pData->Out(pResultRow);
	delete pResultRow;
}


BOOL ExportToDbf(CaIEAInfo* pIEAInfo)
{
	//
	// Check for the limitation of DBF
	int nFieldLength = 0;
	int nRecLength   = 0;
	UINT idsMsg = 0;
	CaIeaDataPage1& dataPage1 = pIEAInfo->m_dataPage1;
	CaQueryResult& result = dataPage1.GetQueryResult();
	CTypedPtrList< CObList, CaColumnExport* >& listLayout = result.GetListColumnLayout();
	CTypedPtrList< CObList, CaColumn* >& listColum  = result.GetListColumn();
	POSITION pos = listLayout.GetHeadPosition();
	while (pos != NULL)
	{
		CaColumnExport* pCol = (CaColumnExport*)listLayout.GetNext (pos);
		nFieldLength = pCol->GetExportLength();
		if(nFieldLength > 254)
		{
			idsMsg = IDS_MSG_DBF_LIMIT_FIELDLENGTH;
			break;
		}
		nRecLength += nFieldLength;
		if(nRecLength > 4000)
		{
			idsMsg = IDS_MSG_DBF_LIMIT_RECORDLENGTH;
			break;
		}
	}
	if (listColum.GetCount() > 128)
	{
		idsMsg = IDS_MSG_DBF_LIMIT_FIELDCOUNT;
	}
	if (idsMsg > 0)
	{
		AfxMessageBox (idsMsg);
		return FALSE;
	}

	CaGenerateDbf generateDbf (pIEAInfo);
	BOOL bOK = DoExport (pIEAInfo, &generateDbf, GenerateDbfFile);
	return bOK;
}

BOOL ExportToXml(CaIEAInfo* pIEAInfo)
{
	CaIeaDataPage1& dataPage1 = pIEAInfo->m_dataPage1;
	CaGenerateXml generateXml;
	generateXml.SetIDSError(IDS_MSG_EXPORT_RESULT_CHANGE);
	generateXml.SetNullNA(theApp.m_strNullNA);
	generateXml.SetOutput(dataPage1.GetFile2BeExported());

	CaQueryResult& previewResult = dataPage1.GetQueryResult();
	CTypedPtrList< CObList, CaColumnExport* >& listPreviewColumn = previewResult.GetListColumnLayout();
	CTypedPtrList< CObList, CaColumnExport* >& listLayout = generateXml.GetListColumnLayout();
	POSITION pos = listPreviewColumn.GetHeadPosition();
	while (pos != NULL)
	{
		CaColumnExport* pCol = listPreviewColumn.GetNext(pos);
		CaColumnExport* pNewCol = new CaColumnExport ((const CaColumnExport&)(*pCol));
		listLayout.AddTail(pNewCol);
	}

	BOOL bOK = DoExport (pIEAInfo, &generateXml, XML_fnGenerateXmlFile);
	return bOK;
}


//
// Export to Fixed Width section:
// ************************************************************************************************
class CaGenerateFixedWidth: public CaGeneratorExport
{
public:
	CaGenerateFixedWidth(CaIEAInfo* pInfo):CaGeneratorExport(pInfo) {}
	~CaGenerateFixedWidth(){}
	
	virtual void Out(CaRowTransfer* pRow);
	virtual void End(BOOL bSuccess = TRUE);
	void ConstructRow (CaRowTransfer* pRow, CString& strLine);
	void FieldValue (CString& strField, CaColumnExport* pCol, BOOL bLastField);
};


void CaGenerateFixedWidth::Out(CaRowTransfer* pRow)
{
	CaIeaDataPage1& dataPage1 = m_pIeaInfo->m_dataPage1;
	CaIeaDataPage2& dataPage2 = m_pIeaInfo->m_dataPage2;
	CString strLine;
	if (!m_bFileCreated)
	{
		//
		// Check to see if header of the select statement has been changed
		// since the last pre-fetched for displaying :
		CaQueryResult& previewResult = dataPage1.GetQueryResult();
		CTypedPtrList< CObList, CaColumn* >& listColumn = previewResult.GetListColumn();
		ASSERT(m_listColumn.GetCount() == listColumn.GetCount());
		if (m_listColumn.GetCount() != listColumn.GetCount())
		{
			//
			// The result of the select statement has been changed since
			// the last fetch for previewing:
			m_nIDSError = IDS_MSG_EXPORT_RESULT_CHANGE;
			throw (int)1;
		}

		//
		// Create csv file.
		m_pFile = new CFile (dataPage1.GetFile2BeExported(), CFile::modeCreate|CFile::modeWrite);
		m_bFileCreated = TRUE;
		//
		// Export CSV Header one line in the file:[ {<column name><separator>...<column name>}+ ]
		if (dataPage2.IsExportHeader())
		{
			CTypedPtrList< CObList, CaColumnExport* >& listPreviewColumn = previewResult.GetListColumnLayout();
			POSITION pos = listPreviewColumn.GetHeadPosition();
			CaRowTransfer* pHeaderRow = new CaRowTransfer();
			CStringList& listHeader = pHeaderRow->GetRecord();
			CArray <UINT, UINT>& arrayNullIndicator = pHeaderRow->GetArrayFlag();
			arrayNullIndicator.SetSize(m_listColumn.GetCount());
			for (int i=0; i<m_listColumn.GetCount(); i++)
				arrayNullIndicator[i] = 0;
			while (pos != NULL)
			{
				CaColumnExport* pCol = listPreviewColumn.GetNext(pos);
				listHeader.AddTail(pCol->GetName());
			}
		
			ConstructRow (pHeaderRow, strLine);
			//
			// Write the Header row:
			m_pFile->Write((LPCTSTR)strLine, strLine.GetLength());
			delete pHeaderRow;
		}
	}

	ConstructRow (pRow, strLine);
	if (!strLine.IsEmpty())
	{
		//
		// Write the fixed width row into the fixed width file:
		m_pFile->Write((const void*)(LPCTSTR)strLine, strLine.GetLength());
	}
}

void CaGenerateFixedWidth::End(BOOL bSuccess)
{
	if (m_pFile)
	{
		if (bSuccess)
		{
			m_pFile->Flush();
			delete m_pFile;
		}
		else
		{
			CString strFileName = m_pFile->GetFilePath();
			delete m_pFile;
			DeleteFile(strFileName);
		}
	}
	m_pFile = NULL;
}

void CaGenerateFixedWidth::ConstructRow (CaRowTransfer* pRow, CString& strLine)
{
	strLine = _T("");
	CaIeaDataPage1& dataPage1 = m_pIeaInfo->m_dataPage1;
	CaIeaDataPage2& dataPage2 = m_pIeaInfo->m_dataPage2;
	CStringList& strlListTuple = pRow->GetRecord();
	CArray <UINT, UINT>& arrayNullIndicator = pRow->GetArrayFlag();
	int nIndex = 0;
	CaQueryResult& previewResult = dataPage1.GetQueryResult();
	CTypedPtrList< CObList, CaColumnExport* >& listPreviewColumn = previewResult.GetListColumnLayout();

	ASSERT(listPreviewColumn.GetCount() == strlListTuple.GetCount());
	if (listPreviewColumn.GetCount() != strlListTuple.GetCount())
	{
		//
		// The result of the select statement has been changed since
		// the last fetch for previewing:
		m_nIDSError = IDS_MSG_EXPORT_RESULT_CHANGE;
		throw (int)1;
	}

	POSITION posField  = strlListTuple.GetHeadPosition();
	POSITION posColumn = listPreviewColumn.GetHeadPosition();
	//
	// Each field (strField) is corresponding (one-to-one) to a header (pCol):
	while (posField != NULL && posColumn != NULL)
	{
		CaColumnExport* pCol = listPreviewColumn.GetNext(posColumn);
		CString strField = strlListTuple.GetNext(posField);
		BOOL bIsNull = ((arrayNullIndicator[nIndex] & CAROW_FIELD_NULL) != 0);
		if (theApp.m_strNullNA.CompareNoCase (pCol->GetNull()) == 0)
			bIsNull = FALSE;
		if (bIsNull)
			strField = pCol->GetNull();

		FieldValue(strField, pCol, (posField==NULL));

		strLine += strField;
		nIndex++;
	}

	strLine+= consttchszReturn;
}

void CaGenerateFixedWidth::FieldValue (CString& strField, CaColumnExport* pCol, BOOL bLastField)
{
	int nLen = strField.GetLength();
	if (bLastField)
		return;
	int nColWidth = pCol->GetExportLength();
	if (nLen <= nColWidth)
	{
		int nSpace = nColWidth - nLen;
		TCHAR* pSpace = new TCHAR[nSpace +1];
		memset (pSpace, _T(' '), nSpace);
		pSpace[nSpace] = _T('\0');

		switch (INGRESII_llIngresColumnType2AppType(pCol))
		{
		case INGTYPE_FLOAT:
		case INGTYPE_FLOAT4:
		case INGTYPE_FLOAT8:
		case INGTYPE_DECIMAL:
		case INGTYPE_INT1:
		case INGTYPE_INT2:
		case INGTYPE_INT4:
		case INGTYPE_INTEGER:
		case INGTYPE_INT8:
		case INGTYPE_MONEY:
			{
				CString strSpace = pSpace;
				strSpace += strField;
				strField = strSpace;
			}
			break;
		default:
			strField += pSpace;
			break;
		}
		delete pSpace;
	}
	else
	if (nLen > nColWidth)
	{
		strField = strField.Left(nColWidth);
		m_bTruncatedFields = TRUE;
	}
}

void CALLBACK GenerateFixedWidthFile (LPVOID lpUserData, CaRowTransfer* pResultRow)
{
	CaGenerateFixedWidth* pData = (CaGenerateFixedWidth*)lpUserData;
	pData->Out(pResultRow);
	delete pResultRow;
}

BOOL ExportToFixedWidth(CaIEAInfo* pIEAInfo)
{
	CaGenerateFixedWidth generateFixedWidth (pIEAInfo);
	BOOL bOK = DoExport (pIEAInfo, &generateFixedWidth, GenerateFixedWidthFile);
	return bOK;
}


//
// Export to Ingres Copy Statement section:
// ************************************************************************************************
BOOL ExportToCopyStatement(CaIEAInfo* pIEAInfo)
{


	return TRUE;
}


//
// Global & static functions:
// ************************************************************************************************
BOOL MatchColumn (CaColumn* pCol1, CaColumn* pCol2)
{
	if (pCol1->GetName().CompareNoCase(pCol2->GetName()) != 0)
		return FALSE;
	if (pCol1->GetType() != pCol2->GetType())
		return FALSE;
	if (pCol1->GetLength() != pCol2->GetLength())
		return FALSE;
	if (pCol1->GetScale() != pCol2->GetScale())
		return FALSE;

	return TRUE;
}

void ConfigureLayout (ExportedFileType expt, CaQueryResult& result, CTypedPtrList< CObList, CaColumn* >* pListLeader)
{
	CTypedPtrList< CObList, CaColumn* >& listColumn = result.GetListColumn();             // Keep the initial value
	CTypedPtrList< CObList, CaColumnExport* >& listLayout = result.GetListColumnLayout(); // User can change the characteristics.
	CTypedPtrList< CObList, CaColumn* >* pListInitialColumn = NULL;
	if (pListLeader)
		pListInitialColumn = pListLeader;
	else
	{
		pListInitialColumn = &listColumn;
		while (!listLayout.IsEmpty())
			delete listLayout.RemoveHead();
	}

	CaColumn* pNewColumn = NULL;
	CaColumnExport* pLayout = NULL;
	POSITION pos = pListInitialColumn->GetHeadPosition();
	while (pos != NULL)
	{
		CaColumn* pHeader = pListInitialColumn->GetNext(pos);
		if (pListLeader)
		{
			pNewColumn = new CaColumn(*pHeader);
			listColumn.AddTail(pNewColumn);
		}
		
		pLayout = new CaColumnExport(*pHeader);
		listLayout.AddTail(pLayout);
		pLayout->SetNull(_T("n/a")); // Null exported as "n/a" (exported as native value)

		switch (expt)
		{
		case EXPORT_CSV:
		case EXPORT_XML:
			switch (INGRESII_llIngresColumnType2AppType(pHeader))
			{
			case INGTYPE_FLOAT:
			case INGTYPE_FLOAT4:
			case INGTYPE_FLOAT8:
			case INGTYPE_DECIMAL:
				/* As the Float4/8 Preferences are available in step 1,
				   there is no need to display the explicit export format!
				   Just display the generic term "text".
				if (pHeader->GetScale() > 0)
				{
					CString strFormat;
					strFormat.Format (_T("decimal (%d, %d)"), pHeader->GetLength(), pHeader->GetScale());
					pLayout->SetExportFormat(strFormat);
				}
				else
					pLayout->SetExportFormat(_T("decimal"));
				*/
				break;
			default:
				break;
			}
			break;

		case EXPORT_FIXED_WIDTH: 
			switch (INGRESII_llIngresColumnType2AppType(pHeader))
			{
			case INGTYPE_CHAR:
			case INGTYPE_VARCHAR:
			case INGTYPE_C:
			case INGTYPE_TEXT:
				pLayout->SetExportLength(pHeader->GetLength());
				break;
			case INGTYPE_DATE:
				pLayout->SetExportLength(25);
				break;
			case INGTYPE_FLOAT:
			case INGTYPE_FLOAT4:
			case INGTYPE_FLOAT8:
			case INGTYPE_DECIMAL:
				/* As the Float4/8 Preferences are available in step 1,
				   there is no need to display the explicit export format!
				   Just display the generic term "text".
				if (pHeader->GetScale() > 0)
				{
					CString strFormat;
					strFormat.Format (_T("decimal (%d, %d)"), pHeader->GetLength(), pHeader->GetScale());
					pLayout->SetExportFormat(strFormat);
				}
				else
					pLayout->SetExportFormat(_T("decimal"));
				*/
				pLayout->SetExportLength(20);
				break;
			case INGTYPE_INT1:
			case INGTYPE_INT2:
			case INGTYPE_INT4:
			case INGTYPE_INTEGER:
			case INGTYPE_INT8:
				pLayout->SetExportLength(20);
				break;
			default:
				pLayout->SetExportLength(pLayout->GetLength());
				break;
			}
			break;

		case EXPORT_DBF:
			switch (INGRESII_llIngresColumnType2AppType(pHeader))
			{
			case INGTYPE_C:
			case INGTYPE_CHAR:
			case INGTYPE_TEXT:
			case INGTYPE_VARCHAR:
				pLayout->SetExportFormat(_T("C"));
				pLayout->SetExportLength(pLayout->GetLength());
				break;
			case INGTYPE_INT1:
			case INGTYPE_INT2:
			case INGTYPE_INTEGER:
			case INGTYPE_INT8:
				pLayout->SetExportFormat(_T("N"));
				pLayout->SetExportLength(19);
				pLayout->SetNull(_T(""));
				break;
			case INGTYPE_FLOAT:
			case INGTYPE_FLOAT4:
			case INGTYPE_FLOAT8:
			case INGTYPE_DECIMAL:
				pLayout->SetExportFormat(_T("N"));
				pLayout->SetExportLength(19);
				pLayout->SetExportScale(pHeader->GetScale());
				pLayout->SetNull(_T(""));
				break;
			case INGTYPE_MONEY:
				pLayout->SetExportFormat(_T("N"));
				pLayout->SetExportLength(19);
				pLayout->SetExportScale(pHeader->GetScale());
				pLayout->SetNull(_T(""));
				break;
			case INGTYPE_DATE:
				pLayout->SetExportFormat(_T("D"));
				pLayout->SetExportLength(8);
				pLayout->SetNull(_T(""));
				break;
			default:
				break;
			}
		}
	}
}

BOOL DoExport (CaIEAInfo* pIEAInfo, CaGenerator* lpGenerator, PfnUserManageQueryResult pGenerateFileFunction)
{
	CaSession* pCurrentSession = NULL;
	CaExecParamQueryRows4Export* pQueryRowParam = NULL;
	BOOL bSilent, bOverWrite;
	CString strLogFile = _T("");
	CString strLogError = _T("");
	try
	{
		CaIeaDataPage1& dataPage1 = pIEAInfo->m_dataPage1;
		CaIeaDataPage2& dataPage2 = pIEAInfo->m_dataPage2;
		pIEAInfo->GetBatchMode(bSilent, bOverWrite, strLogFile);
		CString strExportFile = dataPage1.GetFile2BeExported();
		if (bSilent && _taccess(strExportFile, 0) != -1 && !bOverWrite)
		{
			CString strMsg;
			AfxFormatString1(strMsg, IDS_EXPORTFILE_EXIST, (LPCTSTR)strExportFile);
			strLogError += strMsg;
			strLogError += consttchszReturn;
			strLogError += consttchszReturn;
			throw (int)1;
		}

		//
		// Check if we need the new session:
		CmtSessionManager& sessionMgr = theApp.GetSessionManager();
		CaSession session;
		session.SetFlag (0);
		session.SetIndependent(TRUE);
		session.SetNode(dataPage1.GetNode());
		session.SetDatabase(dataPage1.GetDatabase());
		session.SetServerClass(dataPage1.GetServerClass());
		session.SetUser(_T(""));
		session.SetOptions(_T(""));
		session.SetDescription(sessionMgr.GetDescription());

		SETLOCKMODE lockmode;
		memset (&lockmode, 0, sizeof(lockmode));
		lockmode.nReadLock = dataPage2.IsReadLock()? LM_EXCLUSIVE: LM_NOLOCK;
		if (!pCurrentSession)
		{
			pCurrentSession = sessionMgr.GetSession(&session);
			if (pCurrentSession)
			{
				pCurrentSession->Activate();
				pCurrentSession->Commit();
				pCurrentSession->SetLockMode(&lockmode);
				pCurrentSession->Commit();
			}
		}

		if (!pCurrentSession)
			return FALSE;
		CaTempSession tempSession(pCurrentSession);
		CaCursor* pCursor = new CaCursor (theApp.GetCursorSequenceNumber(), dataPage1.GetStatement());
		CaCursorInfo& cursorInfo = pCursor->GetCursorInfo();
		CaMoneyFormat& moneyFormat = pCursor->GetMoneyFormat();
		if (dataPage1.GetExportedFileType() == EXPORT_DBF)
		{
			//
			// Export to the N data type in the DBF file, the money symbol cannot be handled:
			moneyFormat.SetUsedMoneySymbol(FALSE);
		}
		cursorInfo = dataPage1.GetFloatFormat();

		CString strTitle;
		CString strInterrupt;
		CString strFetchInfo;
		strTitle.LoadString(IDS_TITLE_EXPORT);
		strInterrupt.LoadString(IDS_MSG_INTERRUPT_EXPORT);
		strFetchInfo.LoadString(IDS_FETCH_EXPORTING);

		//
		// NOTE: pQueryRowParam uses the float format in CaCursor 'cursorInfo' and ignore
		//       the inheritant
		pQueryRowParam = new CaExecParamQueryRows4Export();
		pQueryRowParam->SetCursor (pCursor);
		pQueryRowParam->SetSession (pCurrentSession);
		pQueryRowParam->SetStrings(strTitle, strInterrupt, strFetchInfo);
		pQueryRowParam->SetUserHandlerResult((LPVOID)lpGenerator, pGenerateFileFunction);

		// 
		// Just open the cursor to get the characteristics of columns:
		pQueryRowParam->OpenCursor();
		CTypedPtrList< CObList, CaColumn* >& listLeader = pCursor->GetListHeader();
		CTypedPtrList< CObList, CaColumn* >& listColumn = lpGenerator->GetListColumn();
		POSITION pos = listLeader.GetHeadPosition();
	
		//
		// Check if the headers have been changed since the last pre-fetched:
		CaQueryResult& result = dataPage1.GetQueryResult();
		CTypedPtrList< CObList, CaColumn* >& listInialColumn = result.GetListColumn();
		if (listInialColumn.GetCount() != listLeader.GetCount())
		{
			CString strMsg;
			strMsg.LoadString(IDS_MSG_EXPORT_RESULT_CHANGE);
			if (!bSilent)
				AfxMessageBox (strMsg);
			else
			{
				strLogError += strMsg;
				strLogError += consttchszReturn;
				strLogError += consttchszReturn;
				throw (int)1;
			}
			return FALSE;
		}
		POSITION px = listInialColumn.GetHeadPosition();
		while (pos != NULL && px != NULL)
		{
			CaColumn* pCol1 = listLeader.GetNext (pos);
			CaColumn* pCol2 = listInialColumn.GetNext (px);
			if (!MatchColumn(pCol1, pCol2))
			{
				CString strMsg;
				strMsg.LoadString(IDS_MSG_EXPORT_RESULT_CHANGE);
				if (!bSilent)
					AfxMessageBox (strMsg);
				else
				{
					strLogError += strMsg;
					strLogError += consttchszReturn;
					strLogError += consttchszReturn;
					throw (int)1;
				}
				return FALSE;
			}
		}

		pos = listLeader.GetHeadPosition();
		while (pos != NULL)
		{
			CaColumn* pHeader = listLeader.GetNext(pos);
			CaColumn* pNewColumn = new CaColumn(*pHeader);
			listColumn.AddTail(pNewColumn);
		}

		//
		// Disable the current session so that the thread can re-activate it:
		pCurrentSession->SetSessionNone();

		if (!bSilent)
		{
#if defined (_ANIMATION_)
			strTitle.LoadString(IDS_TITLE_EXPORT_ANIMATION);
			CxDlgWait dlgWait(strTitle, NULL);
			dlgWait.SetDeleteParam(FALSE);
			dlgWait.SetExecParam (pQueryRowParam);
			dlgWait.SetUseAnimateAvi(AVI_FETCHR);
			dlgWait.SetUseExtraInfo();
			dlgWait.SetShowProgressBar(FALSE);
			dlgWait.SetHideCancelBottonAlways(FALSE);
			dlgWait.DoModal();
#else
			pQueryRowParam->Run(NULL);
			CString strErr = pQueryRowParam->GetErrorMessageText();
			if (!strErr.IsEmpty())
			{
				strLogError += strErr;
				strLogError += consttchszReturn;
				strLogError += consttchszReturn;
			}
#endif
		}
		else
		{
			pQueryRowParam->Run(NULL);
			CString strErr = pQueryRowParam->GetErrorMessageText();
			if (!strErr.IsEmpty())
			{
				strLogError += strErr;
				strLogError += consttchszReturn;
				strLogError += consttchszReturn;
			}
		}

		BOOL bInterrupted = pQueryRowParam->IsInterrupted();
		int nFetchStatus = pQueryRowParam->GetFetchStatus();
		int nRowExported = pQueryRowParam->GetAccumulation();
		delete pQueryRowParam;
		pQueryRowParam = NULL;
		BOOL bSaveFile = (nFetchStatus != CaExecParamQueryRows::FETCH_ERROR);
		if (bSaveFile && lpGenerator->IsTruncatedFields())
		{
			if (!bSilent)
			{
				int nAnswer = AfxMessageBox (IDS_MSG_EXPORT_FILEDS_TRUNCATED, MB_ICONQUESTION|MB_OKCANCEL);
				if (nAnswer != IDOK)
					bSaveFile = FALSE;
			}
		}

		//
		// Check other errors during the export process:
		// The Thread in TkAnimat.dll calls the members of CaGenerateXXX class in its address space.
		// Any error occurs in member of CaGenerateXXX will throw the exception (int)1 to terminate
		// the thread. CaGenerateXXX sets its member m_nIDSError so that the caller of TkAnimat.dd
		// can load string and display the message.
		if (lpGenerator->GetIDSError() != 0)
		{
			ASSERT(bSaveFile == FALSE);
			bSaveFile = FALSE;
			if (!bSilent)
				AfxMessageBox (lpGenerator->GetIDSError());
			else
			{
			    CString	errcode;

			    errcode.Format("%d", lpGenerator->GetIDSError());
			    strLogError += errcode;
			    strLogError += consttchszReturn;
			    strLogError += consttchszReturn;
			}
		}

		lpGenerator->End(bSaveFile);
		if (!bSaveFile || bInterrupted)
		{
			if (!bSilent)
				return FALSE;
			else
			{
				//
				// In a silent mode, the error in SQL if any, is not shown.
				// so do not return FALSE !!
				throw (int)1; 
			}
		}

		CString strMsg;
		strMsg.Format (IDS_MSG_EXPORT_COMPLETE, nRowExported);
		if (!bSilent)
			AfxMessageBox (strMsg);
		else
		{
			strLogError += strMsg;
			strLogError += consttchszReturn;
			strLogError += consttchszReturn;

			if (!strLogError.IsEmpty() && !strLogFile.IsEmpty())
			{
				CFile f (strLogFile, CFile::modeCreate|CFile::modeNoTruncate|CFile::modeWrite);
				f.SeekToEnd();
				f.Write((LPCTSTR)strLogError, strLogError.GetLength());
				f.Close();
			}	
		}

		return TRUE;
	}
	catch (CeSqlException e)
	{
		if (!bSilent)
			AfxMessageBox(e.GetReason());
		else
		{
			strLogError += e.GetReason();
			strLogError += consttchszReturn;
			strLogError += consttchszReturn;
		}
	}
	catch(...)
	{

	}
	if (!strLogError.IsEmpty() && !strLogFile.IsEmpty())
	{
		CFile f (strLogFile, CFile::modeCreate|CFile::modeNoTruncate|CFile::modeWrite);
		f.SeekToEnd();
		f.Write((LPCTSTR)strLogError, strLogError.GetLength());
		f.Close();
	}

	if (pQueryRowParam)
		delete pQueryRowParam;

	return FALSE;
}

BOOL ExportInSilent(CaIEAInfo& data)
{
	CuIeaPPage2CommonData control;
	control.Findish(data);

	return TRUE;
}

