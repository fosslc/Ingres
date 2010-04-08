/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**
**    Source   : taskanim.cpp, Implementation file 
**    Project  : Ingres Import Assistant
**    Author   : Sotheavut UK (uk$so01)
**
**    Purpose  : Dialog that run thread with animation, allow interupt task 
**               (Parameters and execute task)
**
**  History:
**  15-Mar-2001 (uk$so01)
**     created
**
*/


#include "stdafx.h"
#include "taskanim.h"
#include "qryinfo.h"
#include "ingobdml.h" // INGRESII Low level
#include "rcdepend.h"
#include "iapsheet.h"
extern void INGRESII_llSetCopyHandler(PfnIISQLHandler handler);


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// RUN THE COPY STATEMENT EXEC SQL COPY ....
// ************************************************************************************************

//
// For testing only, it causes to import slowly (one row per second) 
// so that you have time to see and react with the interface.
//#define _TEST_BEHAVIOUR

static CString GetDisplayTime(CTimeSpan& time)
{
	CString strFormat = _T("");
	if (time.GetDays() > 0)
	{
		if (time.GetDays() > 1)
			strFormat+= _T("%D days");
		else
			strFormat+= _T("%D day");
	}
	if (time.GetDays() > 0 || time.GetHours() > 0)
	{
		if (!strFormat.IsEmpty() && time.GetHours() > 0)
			strFormat+= _T(", ");

		if (time.GetHours() > 1)
			strFormat+= _T("%H hours");
		if (time.GetHours() == 1)
			strFormat+= _T("%H hour");
	}
	if (time.GetDays() > 0 || time.GetHours() > 0 || time.GetMinutes() > 0)
	{
		if (!strFormat.IsEmpty() && time.GetMinutes() > 0)
			strFormat+= _T(", ");

		if (time.GetMinutes() > 1)
			strFormat+= _T("%M minutes");
		if (time.GetMinutes() == 1)
			strFormat+= _T("%M minute");
	}
	if (time.GetDays() > 0 || time.GetHours() > 0 || time.GetMinutes() > 0 || time.GetSeconds() > 0)
	{
		if (!strFormat.IsEmpty() && time.GetSeconds() > 0)
			strFormat+= _T(", ");

		if (time.GetSeconds() > 1)
			strFormat+= _T("%S seconds");
		if (time.GetSeconds() == 1)
			strFormat+= _T("%S second");
	}
	return time.Format (strFormat);
}


//
// GetRowCSV:
// 
// Summary: CALLBACK for SQL COPY (CSV and Fixed Width format)
// Parameters:
//    rowLength : maximum number of bytes that can be returned
//    lpRow     : pointer to a contiguous block of memory at least rowLength 
//                in size in which to return record(s)
//    returnByte: number of bytes actually returned in the buffer.
// RETURN VALUE: 
//     0: successful outcome
//     1: failed
//  9900: (ENDFILE). End of data reached. This should be accompanied with a returnByte value of 0
extern "C" long GetRowCSV (long* rowLength, char* lpRow, long* returnByte)
{
	CaUserData4GetRow* pUserData4CopyHander = theApp.GetUserData4GetRow();
	ASSERT(pUserData4CopyHander);
	if (!pUserData4CopyHander)
		return 9900; // ENDFILE
	CaIIAInfo* pIIAData = pUserData4CopyHander->GetIIAData();
	ASSERT(pIIAData);
	if (!pIIAData)
	{
		pUserData4CopyHander->SetCopyCompleteState(TRUE);
		return 9900; // ENDFILE
	}
#if defined (_TEST_BEHAVIOUR)
	//
	// Sleep is for tesing procedure only:
	Sleep (1*1000);
#endif
	int& nRowCopied = pUserData4CopyHander->GetCopyRowcount();
	//
	// Partial commit
	int& nCommitCounter = pUserData4CopyHander->GetCommitCounter();
	int nCommitStep = pUserData4CopyHander->GetCommitStep();
	if ((nCommitCounter > 0) && (nCommitStep > 0) && (nRowCopied >= nCommitStep) && (nRowCopied%nCommitStep == 0))
	{
		*returnByte  = 0;
		pUserData4CopyHander->SetCopyCompleteState(FALSE);
		nCommitCounter = 0;
		return 9900; // ENDFILE
	}

	CaDataPage1& dataPage1 = pIIAData->m_dataPage1;
	CaDataPage2& dataPage2 = pIIAData->m_dataPage2;
	CaDataPage3& dataPage3 = pIIAData->m_dataPage3;
	//
	// Order and effective  of columns:
	CaIngresHeaderRowItemData& commonItemData = dataPage3.GetUpperCtrlCommonData();

	int nInterruptConfirm = CaUserData4GetRow::INTERRUPT_CANCEL; // Not interrupted
	if (theApp.m_synchronizeInterrupt.IsRequestCancel())
	{
		theApp.m_synchronizeInterrupt.BlockUserInterface (FALSE); // Release user interface
		theApp.m_synchronizeInterrupt.WaitWorkerThread ();        // Wait itself until user interface releases it.
		nInterruptConfirm = pUserData4CopyHander->GetInterruptConfirm();
	}

	if (nInterruptConfirm != CaUserData4GetRow::INTERRUPT_CANCEL)
	{
		*returnByte  = 0;
		pUserData4CopyHander->SetCopyCompleteState(TRUE);
		return 9900; // ENDFILE
	}

	//
	// Check to see if we have finished reading the record:
	BOOL bEnd = FALSE;
	if (dataPage1.GetKBToScan() == 0)
	{
		int nMax = dataPage1.GetArrayTextLine().GetSize();
		if (pUserData4CopyHander->GetCurrentRecord() >= nMax)
			bEnd = TRUE;
	}
	else
	{
		istream* pDataFile = pUserData4CopyHander->GetDataFile();
		ASSERT (pDataFile);

		if (!pDataFile)
			bEnd = TRUE;
		if (!bEnd && pDataFile->peek() == _TEOF)
			bEnd = TRUE;
	}
	if (bEnd)
	{
		*returnByte  = 0;
		pUserData4CopyHander->SetCopyCompleteState(TRUE);
		return 9900; // ENDFILE
	}

	//
	// The records are already fetched into memory:
	if (dataPage1.GetKBToScan() == 0) // The Total file has been scanned:
	{
		int& nCurrent = pUserData4CopyHander->GetCurrentRecord();
		if (pUserData4CopyHander->GetUseFileDirectly())
		{
			CStringArray& arrayLine = dataPage1.GetArrayTextLine();
			CString strRecord = arrayLine.GetAt(nCurrent);

			lstrcpy(lpRow, strRecord);
			lstrcat (lpRow, _T("\n"));
			*returnByte  = strRecord.GetLength() +1;
		}
		else
		{
			CaRecord record;
			CaRecord* pRecord = NULL;
			//
			// Formating the record:
			CString strVarField;
			CString strRecord = _T("");

			if (pIIAData->GetInterrupted()) 
			{
				//
				// The Total file has been scanned, but user has interrupted
				// the preview rows. So not all formatted data is in memory:
				// NO FIXED WITDH can be interrupted, it is only the CSV:
				//
				// Formating the record:
				ColumnSeparatorType sepType = dataPage2.GetCurrentSeparatorType();
				CaSeparatorSet* pSet = NULL; // CSV only
				CaSeparator* pSep = NULL;    // CSV only
				ASSERT(sepType == FMT_FIELDSEPARATOR);
				dataPage2.GetCurrentFormat (pSet, pSep);
				ASSERT(pSet);
				CStringArray& arrayLine = dataPage1.GetArrayTextLine();
				CString strRecord = arrayLine.GetAt(nCurrent);
				CStringArray& arrayFields = record.GetArrayFields();
				GetFieldsFromLine (strRecord, arrayFields, pSet);
				pRecord = &record;
			}
			else
			{
				CPtrArray& arrayRecord = dataPage2.GetArrayRecord();
				pRecord = (CaRecord*)arrayRecord.GetAt(nCurrent);
			}

			if (pRecord)
			{
				CStringArray& arrayFields = pRecord->GetArrayFields();
				int nFields = arrayFields.GetSize();

				strRecord = _T("");
				for (int j=0; j<nFields; j++)
				{
					CString strField = arrayFields.GetAt(j);
					strField.TrimRight(_T(' ')); // Remove only spaces, not tabs
					if (commonItemData.m_arrayColumnOrder.GetAt(j+1) != NULL)
					{
						//
						// Only if the column is not skipped:
						strVarField.Format (_T("%05d%s"), strField.GetLength(), (LPCTSTR)strField);
						strRecord += strVarField;
					}
				}
			}

			lstrcpy (lpRow, strRecord);
			lstrcat (lpRow, _T("\n"));
			*returnByte  = strRecord.GetLength() +1;
		}

		nCurrent++;
	}
	else // The partial file has been scanned (or interrupted, in this case the records are not all in memory):
	{
		//
		// Must read the record from file:
		int nLen = 0;
		int& nCurrent = pUserData4CopyHander->GetCurrentRecord();
		TCHAR tchszText  [3501];
		istream* pDataFile = pUserData4CopyHander->GetDataFile();
		ASSERT (pDataFile);
		if (!pDataFile)
		{
			*returnByte = 0;
			pUserData4CopyHander->SetCopyCompleteState(TRUE);
			return 9900; // ENDFILE
		}

		BOOL bGetRecord = FALSE;
		while (pDataFile->peek() != _TEOF)
		{
			pDataFile->getline(tchszText, 3501);
			if (!tchszText[0])
				continue;
			nLen = lstrlen (tchszText);
			//
			// We limit to 2500 characters per line:
			if (nLen <= 3500)
			{
				bGetRecord = TRUE;
				CString strText = tchszText;
				if (dataPage1.GetCodePage() == CaDataPage1::OEM)
				{
					strText.OemToAnsi();
				}

				if (pUserData4CopyHander->GetUseFileDirectly())
				{
					lstrcpy(lpRow, strText);
					lstrcat (lpRow, _T("\n"));
					*returnByte  = strText.GetLength() + 1;

					nCurrent+= strText.GetLength() + 1;
				}
				else
				{
					//
					// Formating the record:
					ColumnSeparatorType sepType = dataPage2.GetCurrentSeparatorType();
					CaSeparatorSet* pSet = NULL; // CSV only
					CaSeparator* pSep = NULL;    // CSV only
					int* pArrayDividerPos = NULL;
					CArray < int , int >& arrayDividerPos = dataPage2.GetArrayDividerPos(); // Fixed width only

					if (sepType == FMT_FIELDSEPARATOR)
						dataPage2.GetCurrentFormat (pSet, pSep);
					else
					if (sepType == FMT_FIXEDWIDTH && arrayDividerPos.GetSize() > 0)
					{
						pArrayDividerPos = new int [arrayDividerPos.GetSize()];
						for (int i=0; i<arrayDividerPos.GetSize(); i++)
						{
							pArrayDividerPos[i] = arrayDividerPos.GetAt(i);
						}
					}

					//
					// Query records:
					CString strVarField;
					CString strRecord = _T("");
					CStringArray arrayFields;
					if (sepType == FMT_FIELDSEPARATOR)
					{
						GetFieldsFromLine (strText, arrayFields, pSet);
					}
					else
					if (sepType == FMT_FIXEDWIDTH)
					{
						GetFieldsFromLineFixedWidth (strText, arrayFields, pArrayDividerPos, arrayDividerPos.GetSize());
					}
					else
					{
						//
						// Wrong data flow:
						ASSERT(FALSE);
					}

					int nFields = arrayFields.GetSize();
					//
					// The assert bellow may be the cause of:
					//  - you decided dot to scan the whole file, 
					//    thus the description of the solution is related to what the program has scanned.
					//  - During the import, the program will scan the whole file, thus something may be 
					//    wright during the pre-scan but may become wrong during the import !!!
					ASSERT (nFields == (commonItemData.m_arrayColumnOrder.GetSize() -1));
					if (nFields != (commonItemData.m_arrayColumnOrder.GetSize() -1))
					{
						pUserData4CopyHander->SetError(CSVxERROR_TABLE_WRONGDESC);
						pUserData4CopyHander->SetCopyCompleteState(TRUE);
						return 1;
					}

					for (int i=0; i<nFields; i++)
					{
						CString& strField = arrayFields.GetAt(i);
						strField.TrimRight(_T(' ')); // Remove only spaces, not tabs.
						if (commonItemData.m_arrayColumnOrder.GetAt(i+1) != NULL)
						{
							//
							// Only if the column is not skipped:
							strVarField.Format (_T("%05d%s"), strField.GetLength(), (LPCTSTR)strField);
							strRecord += strVarField;
						}
					}
					arrayFields.RemoveAll();

					lstrcpy(lpRow, strRecord);
					lstrcat (lpRow, _T("\n"));
					*returnByte  = strRecord.GetLength() + 1;

					nCurrent+= strText.GetLength() + 1;
				}
			}
			else
			{
				pUserData4CopyHander->SetError(CSVxERROR_LINE_TOOLONG);
				pUserData4CopyHander->SetCopyCompleteState(TRUE);
				return 1;
			}

			break;
		}

		if (!bGetRecord)
		{
			*returnByte  = 0;
			pUserData4CopyHander->SetCopyCompleteState(TRUE);
			return 9900; // ENDFILE
		}
	}

	if (*rowLength < *returnByte)
	{
		pUserData4CopyHander->SetBufferTooSmall(TRUE);
		pUserData4CopyHander->SetCopyCompleteState(TRUE);
		return 1;
	}

	nRowCopied++;
	if (nCommitStep > 0)
		nCommitCounter++;
	HWND hwndAnnimate = pUserData4CopyHander->GetAnimateDlg();
	if (hwndAnnimate)
	{
		double dRatio = 100.0 / pUserData4CopyHander->GetTotalSize();
		int nCur = (int)(dRatio * (double)pUserData4CopyHander->GetCurrentRecord());
		int& nProgressPos = pUserData4CopyHander->GetCurrentProgressPos();
		if (nCur > nProgressPos)
		{
			nProgressPos = nCur;
			::PostMessage (hwndAnnimate, WM_EXECUTE_TASK, W_TASK_PROGRESS, (LPARAM)nCur);
		}

		CString strMsg;
		LPTSTR lpszMsg = NULL;
		if (dataPage1.GetKBToScan() == 0)
		{
			strMsg.Format (pUserData4CopyHander->GetMsgImportInfo1(), 
				pUserData4CopyHander->GetCurrentRecord(), 
				pUserData4CopyHander->GetTotalSize());
			lpszMsg = new TCHAR[strMsg.GetLength()+1];
			lstrcpy (lpszMsg, strMsg);
		}
		else
		{
			strMsg.Format (pUserData4CopyHander->GetMsgImportInfo2(), 
				pUserData4CopyHander->GetCurrentRecord(), 
				pUserData4CopyHander->GetTotalSize());
			lpszMsg = new TCHAR[strMsg.GetLength()+1];
			lstrcpy (lpszMsg, strMsg);
		}
		::PostMessage (hwndAnnimate, WM_EXECUTE_TASK, W_EXTRA_TEXTINFO, (LPARAM)lpszMsg);
	}
	return 0;
}


//
// GetRowDBF:
// 
// Summary: CALLBACK for SQL COPY (dBASE format)
// Parameters:
//    rowLength : maximum number of bytes that can be returned
//    lpRow     : pointer to a contiguous block of memory at least rowLength 
//                in size in which to return record(s)
//    returnByte: number of bytes actually returned in the buffer.
// RETURN VALUE: 
//     0: successful outcome
//     1: failed
//  9900: (ENDFILE). End of data reached. This should be accompanied with a returnByte value of 0
extern "C" long GetRowDBF (long* rowLength, char* lpRow, long* returnByte)
{
	CaUserData4GetRow* pUserData4CopyHander = theApp.GetUserData4GetRow();
	ASSERT(pUserData4CopyHander);
	if (!pUserData4CopyHander)
		return 9900; // ENDFILE
	CaIIAInfo* pIIAData = pUserData4CopyHander->GetIIAData();
	ASSERT(pIIAData);
	if (!pIIAData)
	{
		pUserData4CopyHander->SetCopyCompleteState(TRUE);
		return 9900; // ENDFILE
	}
#if defined (_TEST_BEHAVIOUR)
	//
	// Sleep is for tesing procedure only:
	Sleep (1*1000);
#endif
	int& nRowCopied = pUserData4CopyHander->GetCopyRowcount();
	//
	// Partial commit
	int& nCommitCounter = pUserData4CopyHander->GetCommitCounter();
	int nCommitStep = pUserData4CopyHander->GetCommitStep();
	if ((nCommitCounter > 0) && (nCommitStep > 0) && (nRowCopied >= nCommitStep) && (nRowCopied%nCommitStep == 0))
	{
		*returnByte  = 0;
		pUserData4CopyHander->SetCopyCompleteState(FALSE);
		nCommitCounter = 0;
		return 9900; // ENDFILE
	}
	CaDataPage1& dataPage1 = pIIAData->m_dataPage1;
	CaDataPage2& dataPage2 = pIIAData->m_dataPage2;
	CaDataPage3& dataPage3 = pIIAData->m_dataPage3;
	//
	// Order and effective  of columns:
	CaIngresHeaderRowItemData& commonItemData = dataPage3.GetUpperCtrlCommonData();

	int nInterruptConfirm = CaUserData4GetRow::INTERRUPT_CANCEL; // Not interrupted
	if (theApp.m_synchronizeInterrupt.IsRequestCancel())
	{
		theApp.m_synchronizeInterrupt.BlockUserInterface (FALSE); // Release user interface
		theApp.m_synchronizeInterrupt.WaitWorkerThread ();        // Wait itself until user interface releases it.
		nInterruptConfirm = pUserData4CopyHander->GetInterruptConfirm();
	}

	if (nInterruptConfirm != CaUserData4GetRow::INTERRUPT_CANCEL)
	{
		*returnByte  = 0;
		pUserData4CopyHander->SetCopyCompleteState(TRUE);
		return 9900; // ENDFILE
	}

	//
	// Check to see if we have finished reading the record:
	BOOL bEnd = FALSE;
	if (dataPage1.GetKBToScan() == 0)
	{
		int nMax = dataPage2.GetArrayRecord().GetSize();
		if (pUserData4CopyHander->GetCurrentRecord() >= nMax)
			bEnd = TRUE;
	}
	else
	{
		CFile* pDataFile =(CFile*)pUserData4CopyHander->GetDataFile();
		ASSERT (pDataFile);

		if (!pDataFile)
			bEnd = TRUE;
		if (!bEnd && pUserData4CopyHander->GetCurrentRecord() >= dataPage2.GetDBFTheoricRecordSize())
			bEnd = TRUE;
	}
	if (bEnd)
	{
		*returnByte  = 0;
		pUserData4CopyHander->SetCopyCompleteState(TRUE);
		return 9900; // ENDFILE
	}

	//
	// The records are already fetched into memory:
	if (dataPage1.GetKBToScan() == 0 && !pIIAData->GetInterrupted()) // The Total file has been scanned:
	{
		int& nCurrent = pUserData4CopyHander->GetCurrentRecord();
		//
		// Formating the record:
		CString strVarField;
		CString strRecord = _T("");

		CPtrArray& arrayRecord = dataPage2.GetArrayRecord();
		CaRecord* pRecord = (CaRecord*)arrayRecord.GetAt(nCurrent);
		if (pRecord)
		{
			CStringArray& arrayFields = pRecord->GetArrayFields();
			int nFields = arrayFields.GetSize();

			strRecord = _T("");
			for (int j=0; j<nFields; j++)
			{
				CString strField = arrayFields.GetAt(j);
				if (commonItemData.m_arrayColumnOrder.GetAt(j+1) != NULL)
				{
					//
					// Only if the column is not skipped:
					strVarField.Format (_T("%05d%s"), strField.GetLength(), (LPCTSTR)strField);
					strRecord += strVarField;
				}
			}
		}

		lstrcpy (lpRow, strRecord);
		lstrcat (lpRow, _T("\n"));
		*returnByte  = strRecord.GetLength() +1;

		nCurrent++;
	}
	else // The partial file has been scanned (or interrupted, in this case the records are not all in memory):
	{
		//
		// Must read the record from file:
		int nLen = 0;
		int& nCurrent = pUserData4CopyHander->GetCurrentRecord();
		CFile* pDataFile =(CFile*)pUserData4CopyHander->GetDataFile();
		ASSERT (pDataFile);
		if (!pDataFile)
		{
			*returnByte = 0;
			pUserData4CopyHander->SetCopyCompleteState(TRUE);
			return 9900; // ENDFILE
		}
		CString strOutRecord = _T("");
		TRACE1("DBF next fetched=%d\n", nCurrent);
		DBF_QueryRecord(NULL, pIIAData, pDataFile, strOutRecord, 2);
		if (strOutRecord.IsEmpty())
		{
			*returnByte = 0;
			pUserData4CopyHander->SetCopyCompleteState(TRUE);
			return 9900; // ENDFILE
		}

		lstrcpy (lpRow, strOutRecord);
		lstrcat (lpRow, _T("\n"));
		*returnByte  = strOutRecord.GetLength() + 1;

		nCurrent++;
	}

	if (*rowLength < *returnByte)
	{
		pUserData4CopyHander->SetBufferTooSmall(TRUE);
		pUserData4CopyHander->SetCopyCompleteState(TRUE);
		return 1;
	}

	nRowCopied++;
	if (nCommitStep > 0)
		nCommitCounter++;

	HWND hwndAnnimate = pUserData4CopyHander->GetAnimateDlg();
	if (hwndAnnimate)
	{
		double dRatio = 100.0 / pUserData4CopyHander->GetTotalSize();
		int nCur = (int)(dRatio * (double)pUserData4CopyHander->GetCurrentRecord());
		int& nProgressPos = pUserData4CopyHander->GetCurrentProgressPos();
		if (nCur > nProgressPos)
		{
			nProgressPos = nCur;
			::PostMessage (hwndAnnimate, WM_EXECUTE_TASK, W_TASK_PROGRESS, (LPARAM)nCur);
		}

		CString strMsg;
		LPTSTR lpszMsg = NULL;
		strMsg.Format (
			pUserData4CopyHander->GetMsgImportInfo1(), 
			pUserData4CopyHander->GetCurrentRecord(), 
			pUserData4CopyHander->GetTotalSize());
		lpszMsg = new TCHAR[strMsg.GetLength()+1];
		lstrcpy (lpszMsg, strMsg);
		::PostMessage (hwndAnnimate, WM_EXECUTE_TASK, W_EXTRA_TEXTINFO, (LPARAM)lpszMsg);
	}
	return 0;
}


CaExecParamSQLCopy::CaExecParamSQLCopy():CaExecParam(INTERRUPT_USERPRECIFY)
{
	m_pAadParam = NULL;

	m_strMsgBoxTitle.LoadString(AFX_IDS_APP_TITLE);

	// The Import operation has been cancelled.
	m_strMsgFail.LoadString(IDS_IMPORT_OPERATION_CANCELLED);
	// The Import operation has been stopped.
	m_strMsgStop.LoadString(IDS_IMPORT_OPERATION_STOPPED);

	// The Import has completed successfully (%d row(s)).
	m_strMsgOperationSuccess1.LoadString(IDS_IMPORT_OPERATION_CUCCESS);

	// The Import has been interrupted.\n
	// But you have asked to commit the already imported rows: %d row(s) were imported and committed.
	m_strMsgOperationSuccess2.LoadString (IDS_IMPORT_OPERATION_INTERRUPTxCOMMIT);

	// The import operation has been interrupted.\nAll data were rollbacked successfully.
	m_strMsgOperationSuccess3.LoadString (IDS_IMPORT_OPERATION_INTERRUPTxROLLBACK);
	// The import operation has been interrupted.\n%d rows were committed successfully.
	m_strMsgOperationSuccess4.LoadString (IDS_IMPORT_OPERATION_INTERRUPTxCOMMIT2);

	//
	// YES NO CANCEL
	// %d row(s) were already imported but not committed.\n
	// Do you want to commit these row(s) before interrupting the import ?
	m_strMsgConfirmInterrupt1.LoadString(IDS_IMPORT_OPERATION_INTERRUPTxCONFIRM1);
	//
	// YES NO
	// No row has been imported yet.\nDo you want to cancel the import operation ?
	m_strMsgConfirmInterrupt2.LoadString(IDS_IMPORT_OPERATION_INTERRUPTxCONFIRM2);
	// %d row(s) were already committed beacause you checked the option to commit every %d rows.\n
	// Do you want to interrupt the import operation ?
	m_strMsgConfirmInterrupt3.LoadString(IDS_IMPORT_OPERATION_INTERRUPTxCONFIRM3);
	// %d row(s) were already committed beacause you checked the option to commit every %d rows.\n\n
	// %d additional row(s) were also imported but not committed yet.
	// Do you want to commit these row(s) before interrupting the import ?
	m_strMsgConfirmInterrupt4.LoadString(IDS_IMPORT_OPERATION_INTERRUPTxCONFIRM4);

	//
	// The length of buffer returned by SQL CopyRow callback is too short.\n
	m_strSQLCOPYReturnSmallLen.LoadString(IDS_SQLCOPY_RETURN_SMALL_LEN);

	//
	// The task has been killed.\nThere is no warranty that the table has been rollbacked properly.
	m_strMsgKillTask.LoadString(IDS_INTERRUPT_KILL_TASK);

	//
	// The task does not respond since %s. Would you like to kill it ?
	m_strMsgTaskNotResponds1.LoadString (IDS_TASK_NOT_RESPOND1);
	//
	// The task does not respond since %s. There might be a lock on database or table.
	m_strMsgTaskNotResponds2.LoadString(IDS_TASK_NOT_RESPOND2);

	//
	//%d rows were committed beacause you checked the option to commit every %d rows.
	m_strErrorWithPartialCommit.LoadString (IDS_IMPORT_ERRORxPARTIAL_COMMIT);

	m_strIIDecimalFormat  = _T("");
	m_strIIDateFormat = _T("");
	m_strStatementDeleteRows = _T("");
	m_strStatementCreateTable = _T("");
	m_bSuccess = TRUE;
}

int CaExecParamSQLCopy::Run(HWND hWndTaskDlg)
{
	CString strMsg = _T("");
	CaUserData4GetRow* pUserData4CopyHander = theApp.GetUserData4GetRow();
	ASSERT(pUserData4CopyHander);
	if (!pUserData4CopyHander)
		return 1;
	pUserData4CopyHander->GetCopyRowcount() = 0;
	pUserData4CopyHander->GetCommitCounter() = 0;
	pUserData4CopyHander->SetCopyCompleteState(FALSE);
	pUserData4CopyHander->GetCommittedCount() = 0;
	pUserData4CopyHander->SetError(0);

	CaIIAInfo* pIIAData = pUserData4CopyHander->GetIIAData();
	ASSERT(pIIAData);
	if (!pIIAData)
		return 1;
	CaDataPage1& dataPage1 = pIIAData->m_dataPage1;
	CaDataPage2& dataPage2 = pIIAData->m_dataPage2;
	CaIeaTree& treeData = dataPage1.GetTreeData();
	CaSessionManager* pSessionManager = treeData.GetSessionManager();
	ColumnSeparatorType sepType = dataPage2.GetCurrentSeparatorType();

	ASSERT (pSessionManager);
	if (!pSessionManager)
		return 1;

	try
	{
		pUserData4CopyHander->SetAnimateDlg (hWndTaskDlg);
		//
		// If user has interrupted before comming to the finish step, then 
		// we will re-read the whole file.
		BOOL bInterrupted = pIIAData->GetInterrupted();

		if (bInterrupted || (dataPage1.GetKBToScan() > 0 && dataPage1.GetKBToScan() < dataPage1.GetFileSize()))
		{
			if (sepType == FMT_FIELDSEPARATOR || sepType == FMT_FIXEDWIDTH)
			{
				ifstream* pDataFile = new  ifstream(dataPage1.GetFile2BeImported(), ios::nocreate, filebuf::sh_read);
				if ( pDataFile && !File_IsOpen ((ifstream*)pDataFile) )
				{
					delete pDataFile;
					throw (int)100; // cannot open file
				}
				pUserData4CopyHander->SetDataFile (pDataFile, FALSE);
			}
			else
			{
				CFile* pDataFile = new CFile (dataPage1.GetFile2BeImported(), CFile::modeRead|CFile::shareDenyWrite|CFile::typeBinary);
				if (!pDataFile)
					throw (int)100; // cannot open file
				pUserData4CopyHander->SetDataFile ((ifstream*)pDataFile, TRUE);
				CString strOutRecord; // Dummy
				DBF_QueryRecord(NULL, pIIAData, pDataFile, strOutRecord, 1); // Skip the header until the first record.
			}

			//
			// SKIP the amount of lines ?
			if (dataPage1.GetLineCountToSkip() > 0 && sepType != FMT_DBF)
			{
				int nLen = 0, nLineToSkip = 0;
				TCHAR tchszText  [3501];
				istream* pDataFile = pUserData4CopyHander->GetDataFile();
				ASSERT (pDataFile);
				if (!pDataFile)
					return 1;

				//
				// Must read the record from file:
				while (pDataFile->peek() != _TEOF)
				{
					pDataFile->getline(tchszText, 3501);
					if (!tchszText[0])
						continue;
					nLen = lstrlen (tchszText);
					//
					// We limit to 2500 characters per line:
					if (nLen <= 3500)
					{
						if (nLineToSkip < dataPage1.GetLineCountToSkip())
						{
							nLineToSkip++;
							continue;
						}

						break; // Finish skipping
					}
					else
					{
						pUserData4CopyHander->SetError(CSVxERROR_LINE_TOOLONG);
						pUserData4CopyHander->SetCopyCompleteState(TRUE);
						return 1;
					}
				}
			}
		}

		CString strEnv;
		CString strCopyStatement = m_pAadParam->GetStatement();
		//
		// Set the variable environment prior to the Conection to the DBMS Server:
		if (!m_strIIDecimalFormat.IsEmpty())
		{
			SetEnvironmentVariable(_T("II_DECIMAL"), m_strIIDecimalFormat);
		}
		if (!m_strIIDateFormat.IsEmpty())
		{
			SetEnvironmentVariable("II_DATE_FORMAT", m_strIIDateFormat);
		}

		//
		// Prepare and Create a session (Connect to the DBMS Server):
		CaSession session;
		session.SetFlag (0);
		session.SetIndependent(TRUE);
		session.SetNode(m_pAadParam->GetNode());
		session.SetDatabase(m_pAadParam->GetDatabase());
		session.SetServerClass(m_pAadParam->GetServerClass());

		CaSessionUsage sessionUsage(pSessionManager, &session);

		//
		// Create New Table:
		if (!m_strStatementCreateTable.IsEmpty())
		{
			m_pAadParam->SetStatement(m_strStatementCreateTable);
			INGRESII_llExecuteImmediate (m_pAadParam, NULL);
		}

		//
		// Delete Old Rows:
		if (!m_strStatementDeleteRows.IsEmpty())
		{
			m_pAadParam->SetStatement(m_strStatementDeleteRows);
			INGRESII_llExecuteImmediate (m_pAadParam, NULL);
		}

		//
		// Run the copy statement:
		m_pAadParam->SetStatement(strCopyStatement);

		BOOL bOk = TRUE;
		if (dataPage1.GetImportedFileType() == IMPORT_CSV || dataPage1.GetImportedFileType() == IMPORT_TXT)
			INGRESII_llSetCopyHandler((PfnIISQLHandler)GetRowCSV);
		else
		if (dataPage1.GetImportedFileType() == IMPORT_DBF)
			INGRESII_llSetCopyHandler((PfnIISQLHandler)GetRowDBF);
		else
		{
			bOk = FALSE;
			ASSERT (FALSE);
			//
			// Any other types are not handled for the moment, to do it ?
		}

		if (bOk)
		{
			BOOL bCopiedComplete = FALSE;
			while (!bCopiedComplete)
			{
				m_pAadParam->SetStatement(strCopyStatement);
				INGRESII_llExecuteImmediate (m_pAadParam, NULL);
				if (pUserData4CopyHander->GetError() != 0)
					throw (int)pUserData4CopyHander->GetError();
				//
				// Commit:
				if (pUserData4CopyHander->GetCommitStep() > 0)
				{
					if (pUserData4CopyHander->GetInterruptConfirm() != CaUserData4GetRow::INTERRUPT_NOCOMMIT)
					{
						m_pAadParam->SetStatement(_T("commit"));
						INGRESII_llExecuteImmediate (m_pAadParam, NULL);
						int& nCommitedCount = pUserData4CopyHander->GetCommittedCount();
						nCommitedCount = pUserData4CopyHander->GetCopyRowcount();

						TRACE1("CaExecParamSQLCopy::Run, comit at: %d rows\n", pUserData4CopyHander->GetCommittedCount());
					}
				}
				bCopiedComplete = pUserData4CopyHander->GetCopyCompleteState();
			}
			INGRESII_llSetCopyHandler((PfnIISQLHandler)NULL);
		}

		int nInterruptConfirm = pUserData4CopyHander->GetInterruptConfirm();
		switch (nInterruptConfirm)
		{
		case CaUserData4GetRow::INTERRUPT_NOCOMMIT:
			throw 1000;
			break;
		default:
			break;
		}

		//
		// Disconnect the session:
		sessionUsage.Release (TRUE); // Release and Commit:
		pUserData4CopyHander->SetAnimateDlg (NULL);

		if (sepType == FMT_FIELDSEPARATOR || sepType == FMT_FIXEDWIDTH)
		{
			istream* pDataFile = pUserData4CopyHander->GetDataFile();
			if (pDataFile)
			{
				delete pDataFile;
				pUserData4CopyHander->SetDataFile(NULL);
			}
		}
		else
		{
			CFile* pDataFile = (CFile*)pUserData4CopyHander->GetDataFile();
			if (pDataFile)
			{
				pDataFile->Close();
				delete pDataFile;
				pUserData4CopyHander->SetDataFile(NULL);
			}
		}
		return 0;
	}
	catch(CeSqlException e)
	{
		strMsg  = e.GetReason();
		int nCommitStep  = pUserData4CopyHander->GetCommitStep();
		int nCommitCount = pUserData4CopyHander->GetCommittedCount();
		if (nCommitStep > 0 && nCommitCount > 0)
		{
			CString strMsgPartialCommit;
			strMsgPartialCommit.Format (m_strErrorWithPartialCommit, nCommitCount, nCommitStep);
			strMsg += strMsgPartialCommit;
		}

		if (pUserData4CopyHander->IsBufferTooSmall())
		{
			strMsg+= m_strSQLCOPYReturnSmallLen;
		}

		if (nCommitStep > 0 && nCommitCount > 0)
			strMsg += m_strMsgStop;
		else
			strMsg += m_strMsgFail;
		m_bSuccess = FALSE;
	}
	catch (int nException)
	{
		//
		// Interrupt and not commit (CaUserData4GetRow::INTERRUPT_NOCOMMIT):
		if (nException == 1000)
		{
			int nCommitStep  = pUserData4CopyHander->GetCommitStep();
			int nCommitCount = pUserData4CopyHander->GetCommittedCount();
			if (nCommitStep > 0 && nCommitCount > 0)
			{
				strMsg.Format (m_strMsgOperationSuccess4, nCommitCount);
			}
			else
			{
				strMsg = m_strMsgOperationSuccess3;
				strMsg+= m_strMsgFail;
			}
		}
		else
		{
			strMsg = MSG_DetectFileError (nException);
			strMsg+= m_strMsgFail;
			m_bSuccess = FALSE;
		}
	}
	catch(...)
	{
		strMsg = _T("System error(CaExecParamSQLCopy::Run)\n");
		strMsg+= m_strMsgFail;
		m_bSuccess = FALSE;
	}
	m_strMsgFail = strMsg;
	pUserData4CopyHander->SetAnimateDlg (NULL);
	INGRESII_llSetCopyHandler((PfnIISQLHandler)NULL);
	return 1;
}


void CaExecParamSQLCopy::OnTerminate(HWND hWndTaskDlg)
{
	CaUserData4GetRow* pUserData4CopyHander = theApp.GetUserData4GetRow();
	ASSERT(pUserData4CopyHander);
	if (!pUserData4CopyHander)
		return;
	CString strMsg = _T("");
	int& nRowCopied = pUserData4CopyHander->GetCopyRowcount();

	int nInterruptConfirm = pUserData4CopyHander->GetInterruptConfirm();
	if (m_bSuccess)
	{
		switch (nInterruptConfirm)
		{
		case -1:
		case CaUserData4GetRow::INTERRUPT_CANCEL:
			strMsg.Format (m_strMsgOperationSuccess1, nRowCopied);
			::MessageBox(hWndTaskDlg, strMsg, m_strMsgBoxTitle, MB_ICONINFORMATION | MB_OK);
			break;
		case CaUserData4GetRow::INTERRUPT_COMMIT:
			strMsg.Format (m_strMsgOperationSuccess2, nRowCopied);
			::MessageBox(hWndTaskDlg, strMsg, m_strMsgBoxTitle, MB_ICONINFORMATION | MB_OK);
			break;
		}
	}
	else
	{
		if (nInterruptConfirm == CaUserData4GetRow::INTERRUPT_KILL)
		{
			::MessageBox(hWndTaskDlg, m_strMsgKillTask, m_strMsgBoxTitle, MB_ICONEXCLAMATION | MB_OK);
		}
		else
		{
			::MessageBox(hWndTaskDlg, m_strMsgFail, m_strMsgBoxTitle, MB_ICONEXCLAMATION | MB_OK);
		}
	}
}

BOOL CaExecParamSQLCopy::OnCancelled(HWND hWndTaskDlg)
{
	CaUserData4GetRow* pUserData4CopyHander = theApp.GetUserData4GetRow();
	ASSERT(pUserData4CopyHander);
	if (!pUserData4CopyHander)
		return FALSE;
	CString strMsg = _T("");

	theApp.m_synchronizeInterrupt.BlockUserInterface(TRUE);    // Block itself;
	theApp.m_synchronizeInterrupt.SetRequestCancel(TRUE);      // Request the cancel.
	theApp.m_synchronizeInterrupt.BlockWorkerThread(TRUE);     // Block worker thread (Run() method);
	theApp.m_synchronizeInterrupt.WaitUserInterface();         // Wait itself until worker thread allows to do something;

	BOOL bYesNO = TRUE;
	int nRes = IDCANCEL;
	int& nRowCopied = pUserData4CopyHander->GetCopyRowcount();
	if (nRowCopied > 0)
	{
		int nCommitStep  = pUserData4CopyHander->GetCommitStep();
		int nCommitCount = pUserData4CopyHander->GetCommittedCount();
		bYesNO = FALSE;
		if (nCommitStep > 0)
		{
			int nDiv = nRowCopied%nCommitStep;
			//
			// YES NO CANCEL
			if (nDiv == 0)
			{
				// %d row(s) were already committed beacause you checked the option to commit every %d rows.\n
				// Do you want to interrupt the import operation ?
				strMsg.Format (m_strMsgConfirmInterrupt3, nRowCopied, nCommitStep);
			}
			else
			if (nRowCopied > nCommitStep)
			{
				// %d row(s) were already committed beacause you checked the option to commit every %d rows.\n\n
				// %d additional row(s) were also imported but not committed yet.
				// Do you want to commit these row(s) before interrupting the import ?
				strMsg.Format (
					m_strMsgConfirmInterrupt4, nRowCopied-nDiv, nCommitStep, nDiv);
			}
			else
			{
				//
				// YES NO CANCEL
				// %d row(s) were already imported but not committed.\n
				// Do you want to commit these row(s) before interrupting the import ?
				strMsg.Format (m_strMsgConfirmInterrupt1, nRowCopied);
			}

			nRes = ::MessageBox(hWndTaskDlg, strMsg, m_strMsgBoxTitle, MB_ICONQUESTION | MB_YESNOCANCEL);
		}
		else
		{
			//
			// YES NO CANCEL
			// %d row(s) were already imported but not committed.\n
			// Do you want to commit these row(s) before interrupting the import ?
			strMsg.Format (m_strMsgConfirmInterrupt1, nRowCopied);
			nRes = ::MessageBox(hWndTaskDlg, strMsg, m_strMsgBoxTitle, MB_ICONQUESTION | MB_YESNOCANCEL);
		}
	}
	else
	{
		//
		// YES NO
		// No row has been imported yet.\nDo you want to cancel the import operation ?
		bYesNO = TRUE;
		nRes = ::MessageBox(hWndTaskDlg, m_strMsgConfirmInterrupt2, m_strMsgBoxTitle, MB_ICONQUESTION | MB_YESNO);
	}
	
	if (bYesNO)
	{
		if (nRes == IDYES)
			pUserData4CopyHander->SetInterruptConfirm (CaUserData4GetRow::INTERRUPT_NOCOMMIT);
		else
			pUserData4CopyHander->SetInterruptConfirm (CaUserData4GetRow::INTERRUPT_CANCEL);
	}
	else
	{
		if (nRes == IDYES)
			pUserData4CopyHander->SetInterruptConfirm (CaUserData4GetRow::INTERRUPT_COMMIT);
		else
		if (nRes == IDNO)
			pUserData4CopyHander->SetInterruptConfirm (CaUserData4GetRow::INTERRUPT_NOCOMMIT);
		else
			pUserData4CopyHander->SetInterruptConfirm (CaUserData4GetRow::INTERRUPT_CANCEL);
	}

	theApp.m_synchronizeInterrupt.BlockWorkerThread(FALSE);    // Release worker thread (Run() method);
	theApp.m_synchronizeInterrupt.SetRequestCancel(FALSE);     // Un-Request the cancel.
	if (pUserData4CopyHander->GetInterruptConfirm() == CaUserData4GetRow::INTERRUPT_CANCEL)
	{
		::PostMessage (hWndTaskDlg, WM_EXECUTE_TASK, (WPARAM)W_STARTSTOP_ANIMATION, 1);
		return FALSE;
	}
	return TRUE;
}





//
// RUN THE AUTOMATIC DETECTION OF SOLUTION ....
// ************************************************************************************************
CaExecParamDetectSolution::CaExecParamDetectSolution(CaIIAInfo* pIIAInfo):CaExecParam(INTERRUPT_USERPRECIFY)
{
	m_pIIAInfo = pIIAInfo;
	m_strMsgBoxTitle.LoadString(AFX_IDS_APP_TITLE);
	m_strMsgFailed.LoadString (IDS_FAIL_TO_CONSTRUCT_FORMAT_IDENTIFICATION);
	m_strInterruptDetection.LoadString(IDS_INTERRUPT_DETECTING);

	m_strException = _T("");
	m_bOutOfMemory = FALSE;
	m_bInterrupted = FALSE;
}

int CaExecParamDetectSolution::Run(HWND hWndTaskDlg)
{
	ASSERT (m_pIIAInfo);
	if (!m_pIIAInfo)
		return 1;
	CaDataPage1& dataPage1 = m_pIIAInfo->m_dataPage1;
	CaDataPage2& dataPage2 = m_pIIAInfo->m_dataPage2;

	try
	{
		switch (dataPage1.GetImportedFileType())
		{
		case IMPORT_TXT: // Treat as CSV file
		case IMPORT_CSV: // CSV file
			(dataPage1.GetArrayTextLine()).RemoveAll();
			AutoDetectFormatCsv (m_pIIAInfo);
			break;
		case IMPORT_DBF:
			{
				BOOL bOutRecord = FALSE; 
				CString strRecord;
				CString strFile = dataPage1.GetFile2BeImported();
				CFile f(strFile, CFile::modeRead|CFile::shareDenyWrite|CFile::typeBinary);
				DBF_QueryRecord(hWndTaskDlg, m_pIIAInfo, &f, strRecord, FALSE); // bOutRecord = FALSE (strRecord is not valid)
			}
			break;
		case IMPORT_XML: 
			break;
		default:
			ASSERT(FALSE);
			break;
		}
		dataPage1.SetNewDetection();

		return 0;
	}
	catch(CMemoryException* e)
	{
		e->Delete();
		m_bOutOfMemory = TRUE;;
	}
	catch(CeSqlException e)
	{
		m_strException = e.GetReason();
		dataPage2.Cleanup();
	}
	catch (CFileException* e)
	{
		CString strErr;
		e->GetErrorMessage(strErr.GetBuffer(_MAX_PATH), _MAX_PATH);
		strErr.ReleaseBuffer();
		m_strException = strErr;
		e->Delete();
		dataPage2.Cleanup();
	}
	catch (int nException)
	{
		dataPage2.Cleanup();
		m_strException = MSG_DetectFileError(nException);
	}
	catch(...)
	{
		dataPage2.Cleanup();
		//
		// Failed to construct file format identification.
		m_strException = m_strMsgFailed;
	}
	return 1;
}


void CaExecParamDetectSolution::OnTerminate(HWND hWndTaskDlg)
{
}

BOOL CaExecParamDetectSolution::OnCancelled(HWND hWndTaskDlg)
{
	ASSERT (m_pIIAInfo);
	if (!m_pIIAInfo)
		return FALSE;

	theApp.m_synchronizeInterrupt.BlockUserInterface(TRUE);    // Block itself;
	theApp.m_synchronizeInterrupt.SetRequestCancel(TRUE);      // Request the cancel.
	theApp.m_synchronizeInterrupt.BlockWorkerThread(TRUE);     // Block worker thread (Run() method);
	theApp.m_synchronizeInterrupt.WaitUserInterface();         // Wait itself until worker thread allows to do something;

	int nRes = ::MessageBox(hWndTaskDlg, m_strInterruptDetection, m_strMsgBoxTitle, MB_ICONQUESTION | MB_OKCANCEL);
	theApp.m_synchronizeInterrupt.BlockWorkerThread(FALSE);    // Release worker thread (Run() method);
	theApp.m_synchronizeInterrupt.SetRequestCancel(FALSE);     // Un-Request the cancel.
	if (nRes == IDOK)
	{
		m_pIIAInfo->SetInterrupted(TRUE);
		m_bInterrupted = TRUE;
		return TRUE;
	}
	else
		return FALSE;
}

//
// RUN THE THREAD OF QUERYING RECORDS FOR DISPLAYING ....
// ************************************************************************************************
CaExecParamQueryRecords::CaExecParamQueryRecords(
	CaIIAInfo* pIIAInfo, 
	CaSeparatorSet* pSet, 
	int* pColumnCount):CaExecParam(INTERRUPT_USERPRECIFY)
{
	m_pIIAInfo = pIIAInfo;
	m_pSet = pSet;
	m_pnColumnCount = pColumnCount;

	m_strMsgBoxTitle.LoadString(AFX_IDS_APP_TITLE);
	m_strQuerying.LoadString (IDS_QUERYING_RECORD_NofP);
	m_strStopQuerying1.LoadString (IDS_STOP_QUERYING_RECORD);
	m_strStopQuerying2.LoadString (IDS_STOP_QUERYING_RECORD2);
	m_bInterrupted = FALSE;
}

int CaExecParamQueryRecords::Run(HWND hWndTaskDlg)
{
	CString strMsg;
	LPTSTR lpszMsg = NULL;
	ASSERT (m_pIIAInfo);
	if (!m_pIIAInfo)
		return 1;

	CaDataPage1& dataPage1 = m_pIIAInfo->m_dataPage1;
	CaDataPage2& dataPage2 = m_pIIAInfo->m_dataPage2;
	int  nField= 0, nCoumnCount = -1;
	CPtrArray& arrayRecord = dataPage2.GetArrayRecord();
	int i, nSize;
	CStringArray& arrayString = dataPage1.GetArrayTextLine();
	nSize = arrayString.GetSize();

	//
	// Pre-allocated the array of records:
	arrayRecord.SetSize(nSize, 10);

	//
	// For the progress bar:
	double dRatio = 100.0 / nSize;

	//
	// Get fields from line and construct the record and add it to the array of record 'arrayRecord':
	for (i=0; i<nSize; i++)
	{
		CString& strLine = arrayString.GetAt(i);
		CaRecord* pRecord = new CaRecord();
		arrayRecord.SetAt (i, (void*)pRecord);
		CStringArray& arrayFields = pRecord->GetArrayFields();

		GetFieldsFromLine (strLine, arrayFields, m_pSet);
		nField = arrayFields.GetSize();
		if (nCoumnCount == -1)
		{
			nCoumnCount = nField;
			dataPage2.CleanFieldSizeInfo();
			dataPage2.SetFieldSizeInfo (nCoumnCount);
		}
		else
		if (nCoumnCount != nField)
		{
			//
			// Subsequent records do not have the same number of columns !!!
			ASSERT (FALSE);
		}

#if defined (MAXLENGTH_AND_EFFECTIVE)
		//
		// Calculate the effective length and the trailing spaces
		// for each fields:
		for (int j=0; j<nField; j++)
		{
			int nLen1, nLen2;
			CString strField = arrayFields.GetAt(j);
			nLen1 = strField.GetLength();
			strField.TrimRight(_T(' '));
			nLen2 = strField.GetLength();

			int&  nFieldMax = dataPage2.GetFieldSizeMax(j);
			int&  nFieldEffMax = dataPage2.GetFieldSizeEffectiveMax(j);
			BOOL& bTrailingSpace = dataPage2.IsTrailingSpace(j);
			if (nLen1 > 0 && nLen2 < nLen1)
				bTrailingSpace = TRUE;

			if (nLen1 > nFieldMax)
				nFieldMax = nLen1;
			if (nLen2 > nFieldEffMax)
				nFieldEffMax = nLen2;
		}
#endif

		int nCur = (int)(dRatio * (i+1));
		::PostMessage (hWndTaskDlg, WM_EXECUTE_TASK, W_TASK_PROGRESS, (LPARAM)nCur);

		strMsg.Format (m_strQuerying, i+1, nSize);
		lpszMsg = new TCHAR[strMsg.GetLength()+1];
		lstrcpy (lpszMsg, strMsg);
		::PostMessage (hWndTaskDlg, WM_EXECUTE_TASK, W_EXTRA_TEXTINFO, (LPARAM)lpszMsg);

		BOOL bInterruped = FALSE;
		if (theApp.m_synchronizeInterrupt.IsRequestCancel())
		{
			theApp.m_synchronizeInterrupt.BlockUserInterface (FALSE); // Release user interface
			theApp.m_synchronizeInterrupt.WaitWorkerThread ();        // Wait itself until user interface releases it.
			bInterruped = m_pIIAInfo->GetInterrupted();
		}
		else
		{
			bInterruped = m_pIIAInfo->GetInterrupted();
		}

		if (bInterruped)
		{
			arrayRecord.SetSize(i+1, 10);
			break;
		}
	}

	if (m_pnColumnCount)
		*m_pnColumnCount = nCoumnCount;
	return 0;
}


void CaExecParamQueryRecords::OnTerminate(HWND hWndTaskDlg)
{
}

BOOL CaExecParamQueryRecords::OnCancelled(HWND hWndTaskDlg)
{
	ASSERT (m_pIIAInfo);
	if (!m_pIIAInfo)
		return FALSE;
	theApp.m_synchronizeInterrupt.BlockUserInterface(TRUE);    // Block itself;
	theApp.m_synchronizeInterrupt.SetRequestCancel(TRUE);      // Request the cancel.
	theApp.m_synchronizeInterrupt.BlockWorkerThread(TRUE);     // Block worker thread (Run() method);
	theApp.m_synchronizeInterrupt.WaitUserInterface();         // Wait itself until worker thread allows to do something;

	int nRes = -1;
	CaDataPage1& dataPage1 = m_pIIAInfo->m_dataPage1;
	CaDataPage2& dataPage2 = m_pIIAInfo->m_dataPage2;

	if (dataPage1.IsExistingTable())
		nRes = ::MessageBox(hWndTaskDlg, m_strStopQuerying1, m_strMsgBoxTitle, MB_ICONQUESTION | MB_OKCANCEL);
	else
		nRes = ::MessageBox(hWndTaskDlg, m_strStopQuerying2, m_strMsgBoxTitle, MB_ICONQUESTION | MB_OKCANCEL);

	theApp.m_synchronizeInterrupt.BlockWorkerThread(FALSE);    // Release worker thread (Run() method);
	theApp.m_synchronizeInterrupt.SetRequestCancel(FALSE);     // Un-Request the cancel.

	if (nRes == IDOK)
	{
		m_pIIAInfo->SetInterrupted(TRUE);
		m_bInterrupted = TRUE;
		return TRUE;
	}
	else
		return FALSE;
	return TRUE;
}
