/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmlcsv.cpp: implementation file
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Manipulation of data of dBASE file 
**
** History:
**
** 15-Feb-2001 (uk$so01)
**    Created
** 05-Jul-2001 (hanje04)
**    Added (LPTSTR) cast to strLastSeparator and made useof filebuf::sh_read
**    conditional on ~MAINWIN becuase of compile errors on Solaris.
** 19-Jul-2001 (uk$so01)
**    BUG #105289, Fixe the skip first line while detecting the CSV file.
** 31-Oct-2001 (hanje04)
**    Replaced out of date 'CString& name=' constructs with CString name() as
**    they was causing problems on Linux.
** 13-Dec-2001 (hanje04)
**    BUG 106614 - LINUX ONLY
**    Due to a bug in ifstream in the stdlibc++ that Mainwin 4.02 was built 
**    with on Linux, the reading of the csv data file in File_GetLines is now 
**    done with the equivalent C funtions. (fopen(), fgets() etc.)
** 17-Jan-2002 (uk$so01)
**    (bug 106844). Add BOOL m_bStopScanning to indicate if we really stop
**     scanning the file due to the limitation.
** 21-Jan-2002 (uk$so01)
**    (bug 106844). Additional fix: Replace ifstream and some of CFile.by FILE*.
** 21-Jan-2002 (uk$so01)
**    (bug 106844). Additional fix: Also remove the '\n' at the end of line
**    because 'fgets' includes the '\n' in the buffer.
** 30-Jan-2002 (uk$so01)
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
** 02-Aug-2002 (uk$so01)
**    BUG  #108442, IIA proposes duplicated solutions when importing the CSV file 
**    where the fields are quoted.
** 26-Feb-2003 (uk$so01)
**    SIR  #106952, Move function to library
** 11-Jun-2003 (uk$so01)
**    BUG #110390 / INGDBA211, IIA crashes when importing the CSV file where 
**    the first line ends with the separator sign (typically a comma).
** 21-Nov-2003 (uk$so01)
**    SIR  #111320, Construct default headers from the "skipped first line"
** 25-Oct-2004 (noifr01)
**    (bug 113319) fixed error in DBCS character management in GetFieldsFromLine()
** 16-Nov-2004 (uk$so01)
**    IIA BUG #113466
**    If the field in a CSV file contains the quoting character " then the iia might
**    fail to detect the solution.
**    Example, if the file contains the following iia fails:
**    A;B;
**    P;"create procedure p1 as begin INSERT INTO ""uk$so01"".tbx (c) VALUES ('a');commit; end"
** 25-Nov-2004 (uk$so01)
**    BUG #113532: IIA displays incorrectly the separator character if it has an accent.
**    Exclude some characters from candidate separators (the characters that have accent). 
**/


#include "stdafx.h"
#include "rcdepend.h"
#include "dmlcolum.h"
#include "formdata.h"
#include "dmlcsv.h"
#include "tlsfunct.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Get the next character:
// It does not modify the parameter 'lpcText'
// ************************************************************************************************
static TCHAR PeekNextChar (const TCHAR* lpcText)
{
	TCHAR* pch = (TCHAR*)lpcText;
	if (!pch)
		return _T('\0');
	pch = _tcsinc(pch);
	if (!pch)
		return _T('\0');
	return *pch;
}

static BOOL CandidateSeparator(TCHAR tchszSep)
{
	switch(tchszSep)
	{
	case _T('\0'):
	case _T('\"'):
	case _T('\''):
		return FALSE;
	default:
		break;
	}
	if ((int)tchszSep >  0 && (int)tchszSep < 48)
		return TRUE;
	if ((int)tchszSep > 57 && (int)tchszSep < 65)
		return TRUE;
	if ((int)tchszSep > 90 && (int)tchszSep < 97)
		return TRUE;
	if ((int)tchszSep >122 && (int)tchszSep <127)
		return TRUE;
	return FALSE;
}


//
//  Function: IsValidBeginTextQualifier
//
//  Summary:  check to see if the Text Qualifier is valid
//  NOTE:     We consider a valid begin text qualifier if it follows the conditions:
//            - The begin of line or separator must precede the Text Qualifier 
//  Args:
//     CaSeparatorSet* pSet     : [in]  Current set of separators
//     CString& strPrevChar     : [in]  The token precedes the Text Qualifier (TQ) 
//     TCHAR tchszTextQualifier,: [in]  Text Qualifier (TQ) [', "]
//     BOOL bFirstLine          : [in]  TRUE, if lpCurText is the first line in the file.
//                                      If bFirstLine = TRUE then any possible separator immediately 
//                                      before the last Text qualifier character will be considered 
//                                      valid (included those in 'pSet'). Otherwise only the list of 
//                                      separators in 'pSet' parameter will be considered valid.
//  Returns:  BOOL (TRUE) If the Text Qualifier.
//  ***********************************************************************************************
static BOOL IsValidBeginTextQualifier(CaSeparatorSet* pSet, CString& strPrevChar, TCHAR tchszTextQualifier, BOOL bFirstLine)
{
	//
	// The previous token is a begin of line:
	if (strPrevChar.IsEmpty())
		return TRUE;

	if (pSet->GetSeparator(strPrevChar))
		return TRUE; // The previous token ia a separator.
	//
	// We are not at first line, so all potential separators must already be in 'pSet'.
	// And the token strPrevChar is not in the 'pSet', we conclude that 'tchszTextQualifier' is not valid.
	if (!bFirstLine)
		return FALSE;
	//
	// If it is a user specified separator (multiple characters), it should be already in 'pSet'.
	// So we return FALSE:
	if (strPrevChar.GetLength() != 1)
		return FALSE;
	//
	// Assume it can be a candidate:
	if (CandidateSeparator(strPrevChar.GetAt(0)))
		return TRUE;
	return FALSE;
}



//
//  Function: IsValidEndTextQualifier
//
//  Summary:  seeks for the valid Text Qualifier from the position of <lpCurText+1>
//  NOTE:     We consider a valid end text qualifier if it follows the conditions:
//            - The end text qualifier must follow by a separator or end of line.
//            This function should be called when you detect the begin text qualifier.
//  Args:
//     CaSeparatorSet* pSet     : [in]  Current set of separators
//     const TCHAR* lpCurText,  : [in]  Current string to check 
//     TCHAR tchszTextQualifier,: [in]  Text Qualifier (TQ) [', "]
//     CString& strLastSeparator: [in]  The separator precedes the TQ 
//                                [out] The separator follows the TQ.
//     BOOL& bSeparatorInside   : [in]  TRUE, perform the check for enclosed separators in TQ
//                                [out] TRUE, if exists the separators enclosed in TQ
//     BOOL bFirstLine          : [in]  TRUE, if lpCurText is the first line in the file.
//                                      If bFirstLine = TRUE then any possible separator immediately 
//                                      after the last Text qualifier character will be considered 
//                                      valid (included those in 'pSet'). Otherwise only the list of 
//                                      separators in 'pSet' parameter will be considered valid.
//  Returns:  BOOL (TRUE) if the is a valid text qualifier.
//  ***********************************************************************************************
static BOOL IsValidEndTextQualifier(
	CaSeparatorSet* pSet, 
	const TCHAR* lpCurText, 
	TCHAR tchszTextQualifier, 
	CString& strLastSeparator, 
	BOOL& bSeparatorInside,
	BOOL bFirstLine)
{
	BOOL bCheckEnclosed = bSeparatorInside;
	CString strFirstSep = strLastSeparator;
	CString strPrev = _T("");
	TCHAR tchszSep  = _T('\0');
	TCHAR* pch  = (LPTSTR)lpCurText;

	//
	// NOTE: The separators inside the TQ, if any, must be the same as those precede and follow the TQ.
	bSeparatorInside = FALSE;
	strPrev = *pch;
	pch = _tcsinc(pch); // Skip the begin text qualifier.

	while (pch && *pch != _T('\0'))
	{
		tchszSep = *pch;
		if (*pch == tchszTextQualifier)
		{
			pch = _tcsinc(pch);
			if (pch && *pch == _T('\0'))
			{
				strLastSeparator = _T("");
				return TRUE; // End of line (end of record) immediately after the last Text Qualifier.
			}

			if (pch && *pch != _T('\0'))
			{
				//
				// If the character text qualifier has been "escaped" ? ie preceded immediately by a Text Qualifier ?
				// If so, the character *pch is considered as normal character:
				if (*pch == tchszTextQualifier)
				{
					strPrev = tchszTextQualifier;
					pch = _tcsinc(pch);
					continue;
				}
				//
				// Can 'pch' be separator ?
				CaSeparator* pSep = pSet->CanBeSeparator(pch);
				if (pSep)
				{
					strLastSeparator = pSep->GetSeparator();
					if (!bSeparatorInside && bCheckEnclosed)
					{
						TCHAR* pFound = _tcsstr ((LPTSTR)lpCurText, (LPTSTR)(LPCTSTR)strLastSeparator);
						if (pFound && pFound < pch)
							bSeparatorInside = TRUE;
					}
					return TRUE;
				}

				if (!bFirstLine)
					return FALSE;
				if (CandidateSeparator(*pch))
				{
					strLastSeparator = *pch;
					if (!bSeparatorInside && bCheckEnclosed)
					{
						TCHAR* pFound = _tcsstr ((LPTSTR)lpCurText, (LPTSTR)(LPCTSTR)strLastSeparator);
						if (pFound && pFound <= pch)
							bSeparatorInside = TRUE;
					}
					return TRUE;
				}
			}
		}
		else
		if (!bSeparatorInside && bCheckEnclosed) // If we must check for the enclose separator and we have not fount it yet.
		{
			//
			// Is there separator inside the text qualifier ?
			int nInc = 1;
			CaSeparator* pSep = pSet->CanBeSeparator(pch);
			if (pSep)
			{
				nInc = pSep->GetSeparator().GetLength();
				if (!strFirstSep.IsEmpty())
				{
					if (pSep->GetSeparator().CompareNoCase (strFirstSep) == 0)
						bSeparatorInside = TRUE;
				}

				strPrev = pSep->GetSeparator();
				pch = _tcsninc (pch, nInc);
				continue;
			}
			else
			{
				if (bFirstLine && CandidateSeparator(*pch))
				{
					TCHAR chsep = *pch;
					if (strFirstSep.GetLength() == 1 && strFirstSep[0] == chsep)
						bSeparatorInside = TRUE;
				}
			}
		}
	
		strPrev = *pch;
		pch = _tcsinc(pch);
	}
	
	return FALSE;
}


//
// The set of field separators:
// ************************************************************************************************
CaSeparatorSet::CaSeparatorSet()
{
	m_bUserSet = FALSE;
	m_bIgnoreTrailingSeparator = FALSE;
	m_tchszTextQualifier = _T('\0');
	m_bConsecutiveSeparatorsAsOne = FALSE;
}


CaSeparatorSet::~CaSeparatorSet()
{
	Cleanup();
}

void CaSeparatorSet::Cleanup()
{
	while (!m_listCharSeparator.IsEmpty())
		delete m_listCharSeparator.RemoveHead();
	while (!m_listStrSeparator.IsEmpty())
		delete m_listStrSeparator.RemoveHead();
	while (!m_listSeparatorConsecutive.IsEmpty())
		delete m_listSeparatorConsecutive.RemoveHead();
}


CaSeparatorSet::CaSeparatorSet(const CaSeparatorSet& c)
{
	Copy (c);
}

CaSeparatorSet CaSeparatorSet::operator = (const CaSeparatorSet& c)
{
	ASSERT (&c != this); // A=A
	if (&c == this)
		return *this;
	Copy(c);
	return *this;
}

void CaSeparatorSet::Copy(const CaSeparatorSet& c)
{
	Cleanup();

	m_bUserSet = c.m_bUserSet;
	m_bIgnoreTrailingSeparator = c.m_bIgnoreTrailingSeparator;
	m_tchszTextQualifier = c.m_tchszTextQualifier;
	m_bConsecutiveSeparatorsAsOne = c.m_bConsecutiveSeparatorsAsOne;


	POSITION pos = c.m_listCharSeparator.GetHeadPosition();
	while (pos != NULL)
	{
		CaSeparator* pObj = c.m_listCharSeparator.GetNext (pos);
		m_listCharSeparator.AddTail (new CaSeparator (*pObj));
	}
	pos = c.m_listStrSeparator.GetHeadPosition();
	while (pos != NULL)
	{
		CaSeparator* pObj = c.m_listStrSeparator.GetNext (pos);
		m_listStrSeparator.AddTail (new CaSeparator (*pObj));
	}
	pos = c.m_listSeparatorConsecutive.GetHeadPosition();
	while (pos != NULL)
	{
		CaSeparatorConsecutive* pObj = c.m_listSeparatorConsecutive.GetNext (pos);
		CaSeparatorConsecutive* pNewObj = new CaSeparatorConsecutive (pObj->m_first.GetSeparator(), pObj->m_next.GetSeparator());
		m_listSeparatorConsecutive.AddTail (pNewObj);
	}
}

void CaSeparatorSet::NewSeparator (CString& strSep)
{
	if (strSep.IsEmpty())
		return;
	if (strSep.GetLength() == 1)
		m_listCharSeparator.AddTail (new CaSeparator (strSep[0]));
	else
		m_listStrSeparator.AddTail (new CaSeparator (strSep));
}

void CaSeparatorSet::RemoveSeparator (CaSeparator* pObj)
{
	POSITION pos = m_listStrSeparator.Find (pObj);
	if (pos)
	{
		m_listStrSeparator.RemoveAt (pos);
		delete pObj;
		return;
	}

	pos = m_listCharSeparator.Find (pObj);
	if (pos)
	{
		m_listCharSeparator.RemoveAt (pos);
		delete pObj;
	}
}


BOOL CaSeparatorSet::TextQualifier(TCHAR tchszChar)
{
	if (tchszChar != _T('\0') && tchszChar == m_tchszTextQualifier)
		return TRUE;
	return FALSE;
}

CaSeparator* CaSeparatorSet::CanBeSeparator (LPTSTR lpszText)
{
	TCHAR tchszSep  = _T('\0');
	TCHAR* pch  = (LPTSTR)lpszText;
	CString strPrev = _T("");

	if (!pch)
		return NULL;
	if (pch && *pch == _T('\0'))
		return NULL;
	//
	// Handle the mutiple characters as a separator first:
	POSITION pos = m_listStrSeparator.GetHeadPosition();
	while (pos != NULL)
	{
		CaSeparator* pObj = m_listStrSeparator.GetNext(pos);
		CString& strSep = pObj->GetSeparator();
		int nLen = strSep.GetLength();
		if (lstrlen (pch) >= nLen)
		{
			CString strCur = pch;
			strCur = strCur.Left (nLen);
			if (strCur.CompareNoCase (strSep) == 0)
			{
				return pObj;
			}
		}
	}

	tchszSep = *pch;
	CaSeparator* pObj = GetSeparator(tchszSep);
	return pObj;
}



CaSeparator* CaSeparatorSet::GetSeparator(TCHAR tchChar)
{
	POSITION pos = m_listCharSeparator.GetHeadPosition();
	while (pos != NULL)
	{
		CaSeparator* pObj = m_listCharSeparator.GetNext (pos);
		if (pObj->GetSeparator().GetAt(0) == tchChar)
		{
			return pObj;
		}
	}

	return NULL;
}


CaSeparator* CaSeparatorSet::GetSeparator(LPCTSTR lpSep)
{
	POSITION pos = m_listStrSeparator.GetHeadPosition();
	while (pos != NULL)
	{
		CaSeparator* pObj = m_listStrSeparator.GetNext (pos);
		if (pObj->GetSeparator().CompareNoCase(lpSep) == 0)
		{
			return pObj;
		}
	}

	if (lpSep && lstrlen (lpSep) != 1)
		return NULL;
	TCHAR tchSep = *lpSep;
	return GetSeparator(tchSep);
}


void CaSeparatorSet::InitUserSet(LPCTSTR lpszInputSeparators)
{
	CString strTemp = lpszInputSeparators;
	TCHAR tchszSep  = _T('\0');
	TCHAR* lpstr = (TCHAR*)(LPCTSTR)strTemp;
	TCHAR* token = NULL;
	TCHAR  sep[] = {_T(' ')};
	int nLen;

	// 
	// Establish string and get the first token:
	token = _tcstok (lpstr, sep);
	while (token != NULL )
	{
		nLen  = lstrlen (token);
		tchszSep = *token;
		if (nLen == 1 && tchszSep != _T('\'') && tchszSep != _T('\"'))
		{
			CaSeparator* pSep = GetSeparator(tchszSep);
			if (pSep)
				pSep->GetCount()++;
			else
			{
				pSep = new CaSeparator (tchszSep, 0);
				m_listCharSeparator.AddTail(pSep);
			}
		}
		else
		if (nLen > 1)
		{
			CaSeparator* pSep = GetSeparator(token);
			if (pSep)
				pSep->GetCount()++;
			else
			{
				pSep = new CaSeparator (token, 0);
				m_listStrSeparator.AddTail(pSep);
			}
		}

		token = _tcstok(NULL, sep);
	}
	m_bUserSet = TRUE;
}



void CaSeparatorSet::AddNewConsecutiveSeparators (LPCTSTR strPrev, LPCTSTR strCurrent)
{
	BOOL bExist = FALSE;
	POSITION pos = m_listSeparatorConsecutive.GetHeadPosition();
	while (pos != NULL)
	{
		CaSeparatorConsecutive* pObj = m_listSeparatorConsecutive.GetNext (pos);
		//
		// Check if it is there:
		CString strFirst = pObj->m_first.GetSeparator();
		CString strNext  = pObj->m_next.GetSeparator();
		if (strFirst.CompareNoCase (strPrev) == 0 && strNext.CompareNoCase (strCurrent) == 0)
			bExist = TRUE;
		if (bExist)
			break;
	}
	
	if (!bExist)
		m_listSeparatorConsecutive.AddTail (new CaSeparatorConsecutive (strPrev, strCurrent));
}

void CaSeparatorSet::RemoveConsecutiveSeparators (LPCTSTR strSep)
{
	POSITION p, pos = m_listSeparatorConsecutive.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		CaSeparatorConsecutive* pObj = m_listSeparatorConsecutive.GetNext (pos);
		//
		// Check if it is there:
		BOOL bExist = FALSE;
		CString strFirst = pObj->m_first.GetSeparator();
		CString strNext  = pObj->m_next.GetSeparator();
		if (strFirst.CompareNoCase (strSep) == 0 || strNext.CompareNoCase (strSep) == 0)
		{
			m_listSeparatorConsecutive.RemoveAt (p);
			delete pObj;
		}
	}
}

BOOL CaSeparatorSet::IsConsecutiveSeparator (LPCTSTR strSep)
{
	POSITION pos = m_listSeparatorConsecutive.GetHeadPosition();
	while (pos != NULL)
	{
		CaSeparatorConsecutive* pObj = m_listSeparatorConsecutive.GetNext (pos);
		//
		// Check if it is there:
		BOOL bExist = FALSE;
		CString strFirst = pObj->m_first.GetSeparator();
		CString strNext  = pObj->m_next.GetSeparator();
		if (strFirst.CompareNoCase (strSep) == 0 || strNext.CompareNoCase (strSep) == 0)
			return TRUE;
	}
	return FALSE;
}

// Throw CSVxERROR_CANNOT_OPEN  : cannot open file
//       CSVxERROR_LINE_TOOLONG : Line too long
void CSV_GetHeaderLines(LPCTSTR lpszFile, int nAnsiOem, CString& strLine)
{
	USES_CONVERSION;
	const int nCount = 4096;
	int nLen = 0;
	TCHAR tchszText  [nCount];
	strLine = _T("");

	FILE* in;
	in = _tfopen(lpszFile, _T("r"));
	if (in == NULL)
	{
		throw (int)CSVxERROR_CANNOT_OPEN;
	}

	CString strText = _T("");
	tchszText[0] = _T('\0');
	while ( !feof(in) ) 
	{
		TCHAR* pLine = _fgetts(tchszText, nCount, in);
		if (!pLine)
			break;
		nLen = lstrlen (tchszText);

		RemoveEndingCRLF(tchszText);
		if (!tchszText[0])
			continue;
		strText += tchszText;
		if (strText.GetLength() > MAX_RECORDLEN)
		{
			fclose(in);
			throw (int)CSVxERROR_LINE_TOOLONG;
		}

		//
		// We limit to MAX_RECORDLEN characters per line:
		if (nLen < (nCount-1))
		{
			if (nAnsiOem == CaDataPage1::OEM)
			{
				strText.OemToAnsi();
			}
			strLine = strText;
			break;
		}
		else
		{
			continue;
		}
	}

	fclose(in);
}

//
// Get the text line from the file:
// Throw CSVxERROR_CANNOT_OPEN  : cannot open file
//       CSVxERROR_LINE_TOOLONG : Line too long
// ************************************************************************************************
void File_GetLines(CaIIAInfo* pData)
{
	USES_CONVERSION;
	const int nCount = 4096;
	int nKBScanned = 0;
	int nMinLen  = INT_MAX-1;
	int nMaxLen  = 0;
	int nLen = 0;
	int nLineToSkip = 0;
	unsigned long lLine = 0;
	BOOL bStop = FALSE;
	CaDataPage1& dataPage1 = pData->m_dataPage1;
	dataPage1.SetStopScanning(FALSE);

	CString strFile = dataPage1.GetFile2BeImported();
	CStringArray& arrayString = dataPage1.GetArrayTextLine();
	TCHAR tchszText  [nCount];

	FILE* in;
	in = _tfopen((LPCTSTR)strFile, _T("r"));
	if (in == NULL)
	{
		throw (int)CSVxERROR_CANNOT_OPEN;
	}

	CString strText = _T("");
	tchszText[0] = _T('\0');
	while ( !feof(in) ) 
	{
		if (theApp.m_synchronizeInterrupt.IsRequestCancel())
		{
			theApp.m_synchronizeInterrupt.BlockUserInterface (FALSE); // Release user interface
			theApp.m_synchronizeInterrupt.WaitWorkerThread ();        // Wait itself until user interface releases it.
			if (pData->GetInterrupted())
				return;
		}
		else
		{
			if (pData->GetInterrupted())
				return;
		}

		TCHAR* pLine = _fgetts(tchszText, nCount, in);
		if (!pLine)
			break;
		nLen = lstrlen (tchszText);
		RemoveEndingCRLF(tchszText);
		if (!tchszText[0])
			continue;
		strText += tchszText;
		if (strText.GetLength() > MAX_RECORDLEN)
		{
			fclose(in);
			throw (int)CSVxERROR_LINE_TOOLONG;
		}

		//
		// We limit to MAX_RECORDLEN characters per line:
		if (nLen < (nCount-1))
		{
			int nRecordLength = strText.GetLength();
			if (nLineToSkip < dataPage1.GetLineCountToSkip())
			{
				nLineToSkip++;
				strText = _T("");
				continue;
			}

			if (nRecordLength > nMaxLen)
				nMaxLen = nRecordLength;
			if (nRecordLength < nMinLen)
				nMinLen = nRecordLength;
			if (dataPage1.GetCodePage() == CaDataPage1::OEM)
			{
				strText.OemToAnsi();
			}

			arrayString.Add(strText);
			strText = _T("");

			lLine++;
			nKBScanned+= ((nRecordLength+1)*sizeof(TCHAR));

			if (dataPage1.GetKBToScan() > 0 && (nKBScanned / 1024) >= dataPage1.GetKBToScan())
			{
				if (!feof(in))
				{
					bStop = TRUE;
				}
				break;
			}
		}
		else
		{
			continue;
		}
	}

	fclose(in);
	dataPage1.SetMaxRecordLength(nMaxLen);
	dataPage1.SetMinRecordLength(nMinLen);
	TRACE1("Number of bytes scanned=%d\n", nKBScanned);
	//
	// Stop scanning due to limit reached:
	if (bStop)
	{
		dataPage1.SetStopScanning(TRUE);
		TRACE0("Stop scanning due to limit reached\n");
	}
}



//
// Get Fields from line:
// pSeparatorSet : It contains the information (TQ, CS, TS, ...) to query the fields.
//                 It contains ONLY one separator in the list 'm_listStrSeparator' and
//                 m_listCharSeparator is ALWAYS empty.
// ************************************************************************************************
void GetFieldsFromLine (CString& strLine, CStringArray& arrayFields, CaSeparatorSet* pSeparatorSet)
{
	CString strField = _T("");
	TCHAR tchszSep  = _T('\0');
	CString strPrev = _T("");
	struct StateStack
	{
		BOOL IsEmpty(){return m_stack.IsEmpty();}
		TCHAR Top()
		{
			TCHAR state;
			if (IsEmpty())
				return _T('\0');
			state = m_stack.GetHead(); 
			return state;
		}
		void PushState(TCHAR state){ m_stack.AddHead(state); };
		void PopState() { m_stack.RemoveHead(); }
	
		CList< TCHAR, TCHAR > m_stack;
	};

	CTypedPtrList< CObList, CaSeparator* >& lchr = pSeparatorSet->GetListCharSeparator();
	CTypedPtrList< CObList, CaSeparator* >& lstr = pSeparatorSet->GetListStrSeparator();
	ASSERT (lchr.IsEmpty());
	ASSERT (lstr.GetCount() <= 1);

	//
	// Special handling (each line is considered as single column):
	if (lstr.IsEmpty() && lchr.IsEmpty())
	{
		arrayFields.Add(strLine);
		return;
	}

	StateStack stack;
	POSITION pHeadPos = lstr.GetHeadPosition();
	CaSeparator* pSeparator = lstr.GetNext (pHeadPos);
	TCHAR tchszTextQualifier = pSeparatorSet->GetTextQualifier();

	//
	// Remove the trailing separators from the record line:
	CString strNewLine =_T("");
	if (pSeparatorSet->GetIgnoreTrailingSeparator())
	{
		CString strTS = pSeparator->GetSeparator();
		int nFound, nLen = strTS.GetLength();
		int nSumLen = nLen;
		CString strTrailSeparator = strLine.Right(nSumLen);
		nFound = strTrailSeparator.Find(strTS, 0);
		BOOL bFound = (nFound == 0);

		while (nFound == 0)
		{
			CString strTemp = strLine.Right(nSumLen + nLen);
			nFound = strTemp.Find(strTS, 0);
			if (nFound == 0)
			{
				nSumLen += nLen;
				strTrailSeparator = strTemp;
			}
		}

		if (bFound)
		{
			strNewLine = strLine.Left (strLine.GetLength() - nSumLen);
		}
	}

	TCHAR* pch = NULL;
	if (strNewLine.IsEmpty())
		pch = (LPTSTR)(LPCTSTR)strLine;
	else
		pch = (LPTSTR)(LPCTSTR)strNewLine;

	while (pch && *pch != _T('\0'))
	{
		tchszSep = *pch;
		if (_istleadbyte(tchszSep))
		{
			strField += tchszSep;
			strField += *(pch+1); /* trailing byte of effective DBCS char */
			pch = _tcsinc(pch);
			if (pch && *pch == _T('\0'))
			{
				arrayFields.Add (strField);
				strField = _T("");
			}
			continue;
		}
		if (!stack.IsEmpty())
		{
			TCHAR tchTop = stack.Top();
			if (tchTop == tchszSep && PeekNextChar(pch) != tchTop)
				stack.PopState();
			else
			{
				strField += tchszSep;
				if (tchTop == tchszSep && PeekNextChar(pch) == tchTop)
				{
					pch = _tcsinc(pch);
					pch = _tcsinc(pch);
					continue;
				}
			}
		}
		else
		if (tchszTextQualifier != _T('\0') && (tchszSep == _T('\'') || tchszSep == _T('\"')))
		{
			//
			// We found the text qualifier, check if it is valid due to the specific rule.
			// See the comment at "(String Qualifier)" above:
			BOOL bValidTextQualifier = FALSE;
			if (IsValidBeginTextQualifier (pSeparatorSet, strPrev, tchszSep, FALSE))
			{
				CString strLast = _T("");
				BOOL bSeparatorInside = FALSE; // FALSE (Not perform the check existence of separators enclosed in TQ
				if (IsValidEndTextQualifier (pSeparatorSet, pch, tchszSep, strLast, bSeparatorInside, FALSE))
				{
					bValidTextQualifier = TRUE;
					// 
					// Is the separator 'strPrev' found is valid for this TQ ?
					if (!strPrev.IsEmpty() && !pSeparatorSet->GetSeparator (strPrev))
						bValidTextQualifier = FALSE;
					// 
					// Is the separator 'strLast' found is valid for this TQ ?
					if (bValidTextQualifier && !strLast.IsEmpty() && !pSeparatorSet->GetSeparator (strLast))
						bValidTextQualifier = FALSE;
					if (!strPrev.IsEmpty() && !strLast.IsEmpty())
						bValidTextQualifier = (strPrev.CompareNoCase (strLast) == 0);
				}
			}

			if (!bValidTextQualifier)
			{
				strField += tchszSep; // The TQ is considered as normal character;
				strPrev = tchszSep;
				pch = _tcsinc(pch);
				continue;
			}
			//
			// The text qualifier is valid and even though the 'pSeparatorSet' does not
			// tell us to handle the text qualifier, we handle it any way, because all
			// walid TQ without ambiguity, the 'pSeparatorSet' does not tell to handle the TQ.
			// EX: line "ABCD","RRR" has the TQ without ambiguity. 
			//     And if such a line presents we must produce the result 
			//     col1=ABCD
			//     col2=RRR.

			// So we push it on the stack 
			if (bValidTextQualifier && tchszTextQualifier == tchszSep)
				stack.PushState(tchszSep);
			else
				strField += tchszSep;

			strPrev = tchszSep;
			pch = _tcsinc(pch);
			continue;
		}
		else
		{
			CaSeparator* pSep = pSeparatorSet->CanBeSeparator (pch);
			if (pSep)
			{
				BOOL bAddField = FALSE;
				BOOL bCS = pSeparatorSet->GetConsecutiveSeparatorsAsOne();
				//
				// We have met a separator:
				if (bCS && pSeparatorSet->GetSeparator (strPrev)) // Consecutive separator considered as one.
				{
					//
					// Nothing to do;
				}
				else
				{
					arrayFields.Add (strField);
					strField = _T("");
					bAddField = TRUE;
				}
				
				//
				// Just skip the separator:
				strPrev = pSep->GetSeparator();
				pch = _tcsninc (pch, strPrev.GetLength());
				tchszSep = *pch;

				//
				// We found a separator and the previous field has been added and if the next
				// character is the end of line, then we consider the next field is an empty field.
				if (bAddField && pch && *pch == _T('\0')) 
					arrayFields.Add (strField);
				continue;
			}
			else
				strField+= tchszSep;
		}


		strPrev = tchszSep;
		pch = _tcsinc(pch);

		if (pch && *pch == _T('\0'))
		{
			arrayFields.Add (strField);
			strField = _T("");
		}
	}
}


void GetFieldsFromLineFixedWidth (CString& strLine, CStringArray& arrayFields, int* pArrayDividerPos, int nSize)
{
	CString strField;
	CString strTemp = strLine;
	int i = (nSize-1);
	int nLen = strTemp.GetLength();

	if (!pArrayDividerPos)
	{
		//
		// User does not set up the divider, just consider each line as single column:
		arrayFields.Add(strLine);
	}
	else
	{
		arrayFields.SetSize(nSize+1);
		while (i >= 0)
		{
			if (nLen <= pArrayDividerPos[i])
			{
				strField = _T("");
			}
			else
			{
				strField= strTemp.Mid  (pArrayDividerPos[i]);
				strTemp = strTemp.Left (pArrayDividerPos[i]);
			}
			arrayFields.SetAt (i+1, strField);

			if (i == 0 && !strTemp.IsEmpty())
			{
				arrayFields.SetAt (0, strTemp);
				break;
			}
			i--;
		}
	}
}


CaTreeSolution::CaExploredInfo::~CaExploredInfo()
{
	while (!m_lSQ0yz.IsEmpty())
		delete m_lSQ0yz.RemoveHead();
	while (!m_lDQ0yz.IsEmpty())
		delete m_lDQ0yz.RemoveHead();
	while (!m_lSQ1yz.IsEmpty())
		delete m_lSQ1yz.RemoveHead();
	while (!m_lDQ1yz.IsEmpty())
		delete m_lDQ1yz.RemoveHead();
	while (!m_lSQ2yz.IsEmpty())
		delete m_lSQ2yz.RemoveHead();
	while (!m_lDQ2yz.IsEmpty())
		delete m_lDQ2yz.RemoveHead();
	while (!m_lCS0yz.IsEmpty())
		delete m_lCS0yz.RemoveHead();
	while (!m_lCS1yz.IsEmpty())
		delete m_lCS1yz.RemoveHead();
	while (!m_lCS2yz.IsEmpty())
		delete m_lCS2yz.RemoveHead();
	while (!m_lTS0yz.IsEmpty())
		delete m_lTS0yz.RemoveHead();
	while (!m_lTS1yz.IsEmpty())
		delete m_lTS1yz.RemoveHead();
	while (!m_lTS2yz.IsEmpty())
		delete m_lTS2yz.RemoveHead();

}


// N = 0: for node (0, y)
// N = 1: for node (1, y)
// N = 2: for node (2, y)
void CaTreeSolution::CaExploredInfo::NewTQBranchNyz (int N, TCHAR tchTQ, CString& strSep1, CString& strSep2)
{
	CTypedPtrList< CObList, CaSeparator* >* pListX = NULL;
	switch (N)
	{
	case 0:
		switch (tchTQ)
		{
		case _T('\''):
			pListX = &m_lSQ0yz;
			break;
		case _T('\"'):
			pListX = &m_lDQ0yz;
			break;
		default:
			ASSERT(FALSE);
			break;
		}
		break;
	case 1:
		switch (tchTQ)
		{
		case _T('\''):
			pListX = &m_lSQ1yz;
			break;
		case _T('\"'):
			pListX = &m_lDQ1yz;
			break;
		default:
			ASSERT(FALSE);
			break;
		}
		break;
	case 2:
		switch (tchTQ)
		{
		case _T('\''):
			pListX = &m_lSQ2yz;
			break;
		case _T('\"'):
			pListX = &m_lDQ2yz;
			break;
		default:
			ASSERT(FALSE);
			break;
		}
		break;
	default:
		ASSERT(FALSE);
		break;
	}
	if (!pListX)
		return;

	BOOL bFound1 = FALSE;
	BOOL bFound2 = FALSE;
	POSITION pos = pListX->GetHeadPosition();
	while (pos != NULL)
	{
		CaSeparator* pSep = pListX->GetNext (pos);
		if (!bFound1 && !strSep1.IsEmpty() && strSep1.CompareNoCase (pSep->GetSeparator()) == 0)
			bFound1 = TRUE;
		if (!bFound2 && !strSep2.IsEmpty() && strSep2.CompareNoCase (pSep->GetSeparator()) == 0)
			bFound2 = TRUE;
		if (bFound1 && bFound2)
			break;
	}

	if (!bFound1 && !strSep1.IsEmpty())
		pListX->AddTail (new CaSeparator (strSep1));
	if (!bFound2 && !strSep2.IsEmpty() && strSep2.CompareNoCase (strSep1) != 0)
		pListX->AddTail (new CaSeparator (strSep2));
}

void CaTreeSolution::CaExploredInfo::NewCSBranchNyz (int N, CString& strSep1, CString& strSep2)
{
	CTypedPtrList< CObList, CaSeparator* >* pListX = NULL;
	switch (N)
	{
	case 0:
		pListX = &m_lCS0yz;
		break;
	case 1:
		pListX = &m_lCS1yz;
		break;
	case 2:
		pListX = &m_lCS2yz;
		break;
	default:
		ASSERT(FALSE);
		break;
	}
	if (!pListX)
		return;

	BOOL bFound1 = FALSE;
	BOOL bFound2 = FALSE;
	POSITION pos = pListX->GetHeadPosition();
	while (pos != NULL)
	{
		CaSeparator* pSep = pListX->GetNext (pos);
		if (!bFound1 && !strSep1.IsEmpty() && strSep1.CompareNoCase (pSep->GetSeparator()) == 0)
			bFound1 = TRUE;
		if (!bFound2 && !strSep2.IsEmpty() && strSep2.CompareNoCase (pSep->GetSeparator()) == 0)
			bFound2 = TRUE;
		if (bFound1 && bFound2)
			break;
	}

	if (!bFound1 && !strSep1.IsEmpty())
		pListX->AddTail (new CaSeparator (strSep1));
	if (!bFound2 && !strSep2.IsEmpty() && strSep2.CompareNoCase (strSep1) != 0)
		pListX->AddTail (new CaSeparator (strSep2));
}



//
// nCheck = 0: for node (0, y, z)
// nCheck = 1: for node (1, y, z)
// nCheck = 2: for node (2, y, z)
BOOL CaTreeSolution::CaExploredInfo::ExistTQ (TCHAR tchTQ, CString& strSeparator, int nCheck)
{
	CTypedPtrList< CObList, CaSeparator* >* pListX = NULL;
	switch (nCheck)
	{
	case 0:
		switch (tchTQ)
		{
		case _T('\''):
			pListX = &m_lSQ0yz;
			break;
		case _T('\"'):
			pListX = &m_lDQ0yz;
			break;
		default:
			ASSERT(FALSE);
			break;
		}
		break;
	case 1:
		switch (tchTQ)
		{
		case _T('\''):
			pListX = &m_lSQ1yz;
			break;
		case _T('\"'):
			pListX = &m_lDQ1yz;
			break;
		default:
			ASSERT(FALSE);
			break;
		}
		break;
	case 2:
		switch (tchTQ)
		{
		case _T('\''):
			pListX = &m_lSQ2yz;
			break;
		case _T('\"'):
			pListX = &m_lDQ2yz;
			break;
		default:
			ASSERT(FALSE);
			break;
		}
		break;
	default:
		ASSERT(FALSE);
		break;
	}
	if (!pListX)
		return FALSE;
	
	POSITION pos = pListX->GetHeadPosition();
	while (pos != NULL)
	{
		CaSeparator* pSep = pListX->GetNext (pos);
		if (strSeparator.CompareNoCase (pSep->GetSeparator()) == 0)
			return TRUE;
	}

	return FALSE;
}

//
// nCheck = 0: for node (0, y, z)
// nCheck = 1: for node (1, y, z)
// nCheck = 2: for node (2, y, z)
BOOL CaTreeSolution::CaExploredInfo::ExistCS (CString& strSeparator, int nCheck)
{
	CTypedPtrList< CObList, CaSeparator* >* pListX = NULL;
	switch (nCheck)
	{
	case 0:
		pListX = &m_lCS0yz;
		break;
	case 1:
		pListX = &m_lCS1yz;
		break;
	case 2:
		pListX = &m_lCS2yz;
		break;
	default:
		ASSERT(FALSE);
		break;
	}
	if (!pListX)
		return FALSE;

	POSITION pos = pListX->GetHeadPosition();
	while (pos != NULL)
	{
		CaSeparator* pSep = pListX->GetNext (pos);
		if (strSeparator.CompareNoCase (pSep->GetSeparator()) == 0)
			return TRUE;
	}

	return FALSE;
}


void CaTreeSolution::CaExploredInfo::NewTSBranchNyz (int N, CString& strTS)
{
	CTypedPtrList< CObList, CaSeparator* >* pListX = NULL;
	switch (N)
	{
	case 0:
		pListX = &m_lTS0yz;
		break;
	case 1:
		pListX = &m_lTS1yz;
		break;
	case 2:
		pListX = &m_lTS2yz;
		break;
	default:
		ASSERT(FALSE);
		break;
	}
	if (!pListX)
		return;

	BOOL bFound = FALSE;
	POSITION pos = pListX->GetHeadPosition();
	while (pos != NULL)
	{
		CaSeparator* pSep = pListX->GetNext (pos);
		if (strTS.CompareNoCase (pSep->GetSeparator()) == 0)
		{
			bFound = TRUE;
			break;
		}
	}
	if (bFound)
		return;
	if (!strTS.IsEmpty())
		pListX->AddTail (new CaSeparator (strTS));
}


//
// nCheck = 0: for node (0, y, z)
// nCheck = 1: for node (1, y, z)
// nCheck = 2: for node (2, y, z)
BOOL CaTreeSolution::CaExploredInfo::ExistTS (CString& strSeparator, int nCheck)
{
	CTypedPtrList< CObList, CaSeparator* >* pListX = NULL;
	switch (nCheck)
	{
	case 0:
		pListX = &m_lTS0yz;
		break;
	case 1:
		pListX = &m_lTS1yz;
		break;
	case 2:
		pListX = &m_lTS2yz;
		break;
	default:
		ASSERT(FALSE);
		break;
	}
	if (!pListX)
		return FALSE;

	POSITION pos = pListX->GetHeadPosition();
	while (pos != NULL)
	{
		CaSeparator* pSep = pListX->GetNext (pos);
		if (strSeparator.CompareNoCase (pSep->GetSeparator()) == 0)
			return TRUE;
	}

	return FALSE;
}

CaTreeSolution::CaTreeNodeDetection:: ~CaTreeNodeDetection()
{
	while (!m_listChildren.IsEmpty())
		delete m_listChildren.RemoveHead();

	if (m_pListFailureInfo)
	{
		while (!m_pListFailureInfo->IsEmpty())
			delete m_pListFailureInfo->RemoveHead();
		delete m_pListFailureInfo;
		m_pListFailureInfo = NULL;
	}
}



void CaTreeSolution::CaTreeNodeDetection::InitializeRoot()
{
	//
	// Create the chidren of this node, it can be only one child:
	int nSize = m_pArrayLine->GetSize();
	ASSERT (nSize > 0);
	if (nSize <= 0)
		return;
	//
	// All possible combinaition to be explored:

	//
	// One branch which does considers nothing as Text qualifier:
	CaTreeNodeDetection* pInternal_0_1_1 = new CaTreeNodeDetection (m_pArrayLine, 0, -1, -1); // Internal node (0, -1, -1)
	m_listChildren.AddTail (pInternal_0_1_1);
		pInternal_0_1_1->m_listChildren.AddTail (new CaTreeNodeDetection (m_pArrayLine, 0, 0, 0)); // (0,0,0)
		pInternal_0_1_1->m_listChildren.AddTail (new CaTreeNodeDetection (m_pArrayLine, 0, 0, 1)); // (0,0,1)
		pInternal_0_1_1->m_listChildren.AddTail (new CaTreeNodeDetection (m_pArrayLine, 0, 1, 0)); // (0,1,0)
		pInternal_0_1_1->m_listChildren.AddTail (new CaTreeNodeDetection (m_pArrayLine, 0, 1, 1)); // (0,1,1)
	//
	// One branch which does  considers the character simple quote <'> as Text qualifier:
	CaTreeNodeDetection* pInternal_1_1_1 = new CaTreeNodeDetection (m_pArrayLine, 1, -1, -1); // Internal node (1, -1, -1)
	m_listChildren.AddTail (pInternal_1_1_1);
		pInternal_1_1_1->m_listChildren.AddTail (new CaTreeNodeDetection (m_pArrayLine, 1, 0, 0)); // (1,0,0)
		pInternal_1_1_1->m_listChildren.AddTail (new CaTreeNodeDetection (m_pArrayLine, 1, 0, 1)); // (1,0,1)
		pInternal_1_1_1->m_listChildren.AddTail (new CaTreeNodeDetection (m_pArrayLine, 1, 1, 0)); // (1,1,0)
		pInternal_1_1_1->m_listChildren.AddTail (new CaTreeNodeDetection (m_pArrayLine, 1, 1, 1)); // (1,1,1)
	//
	// One branch which does considers the character double quote <"> as Text qualifier:
	CaTreeNodeDetection* pInternal_2_1_1 = new CaTreeNodeDetection (m_pArrayLine, 2, -1, -1); // Internal node (2, -1, -1)
	m_listChildren.AddTail (pInternal_2_1_1);
		pInternal_2_1_1->m_listChildren.AddTail (new CaTreeNodeDetection (m_pArrayLine, 2, 0, 0)); // (2,0,0)
		pInternal_2_1_1->m_listChildren.AddTail (new CaTreeNodeDetection (m_pArrayLine, 2, 0, 1)); // (2,0,1)
		pInternal_2_1_1->m_listChildren.AddTail (new CaTreeNodeDetection (m_pArrayLine, 2, 1, 0)); // (2,1,0)
		pInternal_2_1_1->m_listChildren.AddTail (new CaTreeNodeDetection (m_pArrayLine, 2, 1, 1)); // (2,1,1)
}


//
// The member 'pSeparatorSet' is used only if we initialize the line (pInfo->m_nLine = 0)
// If the pInfo->m_nLine > 0 then the member m_separatorSet of this is used instead.
// ************************************************************************************************
void CaTreeSolution::CaTreeNodeDetection::CountSeparator(
	CaSeparatorSet* pSeparatorSet, 
	CaSeparatorSet* pTemplate,
	CaExploredInfo* pExploredInfo,
	CString& strLine, 
	CaDetectLineInfo* pInfo)
{
	int nl = pInfo->GetLine();
	TCHAR tchszSep  = _T('\0');
	CString strPrev = _T("");
	TCHAR tchTextQualifier = pTemplate->GetTextQualifier();
	BOOL  bCSO = pTemplate->GetConsecutiveSeparatorsAsOne();
	BOOL  bTS  = pTemplate->GetIgnoreTrailingSeparator();
	StateStack stack;

	//
	// Count all the known separators for each text line:
	TCHAR* pch = (LPTSTR)(LPCTSTR)strLine;
	while (pch && *pch != _T('\0'))
	{
		tchszSep = *pch;
		if (theApp.m_synchronizeInterrupt.IsRequestCancel())
		{
			theApp.m_synchronizeInterrupt.BlockUserInterface (FALSE); // Release user interface
			theApp.m_synchronizeInterrupt.WaitWorkerThread ();        // Wait itself until user interface releases it.
			if (pExploredInfo->GetIIAInfo()->GetInterrupted())
				return;
		}
		else
		{
			if (pExploredInfo->GetIIAInfo()->GetInterrupted())
				return;
		}

		//
		// Handle the text qualifiers:
		if (!stack.IsEmpty()) // We're inside the text qualifier
		{
			TCHAR tchTop = stack.Top();
			if (tchTop == tchszSep && PeekNextChar(pch) != tchTop)
			{
				//
				// The current token is a TQ and is the same as the on the top of the Stack, and the next token
				// is not the same as the current token, so we have matched the begin and end of TQ.
				stack.PopState();
			}

			//
			// Skip everything inside the text qualifier and skip the text qualifier itself.
			strPrev = tchszSep;
			if (tchTop == tchszSep && PeekNextChar(pch) == tchTop)
			{
				pch = _tcsinc(pch);
				strPrev = *pch;
				pch = _tcsinc(pch);
			}
			else
				pch = _tcsinc(pch);
			continue;
		}

		BOOL bValidTextQualifier = FALSE;
		if (tchszSep == _T('\'') || tchszSep == _T('\"'))
		{
			CString strLast = strPrev;
			BOOL bSeparatorInside = TRUE; // Perform the check existence of Separators enclosed in TQ
			TCHAR tchTQ = tchszSep;
			CaSeparatorSet* pSet = (nl == 0)? pSeparatorSet: pTemplate;
			if (tchTextQualifier != _T('\0')) // We're exploring the branch (0, x, y)
				bSeparatorInside = FALSE; 

			if (IsValidBeginTextQualifier (pSet, strPrev, tchTQ, nl == 0))
			{
				if (IsValidEndTextQualifier (pSet, pch, tchTQ, strLast, bSeparatorInside, nl == 0))
				{
					if (!strPrev.IsEmpty() && !strLast.IsEmpty())
					{
						if (strPrev.CompareNoCase (strLast) == 0)
							bValidTextQualifier = TRUE;
						else
							bValidTextQualifier = FALSE;
					}
					else
					{
						bValidTextQualifier = TRUE;
					}
				}
			}

			if (bValidTextQualifier)
			{
				if (tchTextQualifier == _T('\0'))
				{
					//
					// We detect for the possible solutions of BRANCH No Text Qualifier (0, y, z), 
					// (tchTextQualifier == _T('\0')).  But, when we're doing this, 
					// we found that there is also valid Text Qualifier for some Separaters.
					if (bSeparatorInside) 
					{
						//
						// There exist an alternative solution.
						// We mark that the branch (1,y,z) or (2,y,z) must be explored:
						if (tchszSep == _T('\'') && pExploredInfo->GetLineSQ() == -1)
							pExploredInfo->SetLineSQ(nl);
						if (tchszSep == _T('\"') && pExploredInfo->GetLineDQ() == -1)
							pExploredInfo->SetLineDQ(nl);
					}
					else
					{
						//
						// Add the possible separators for the Text Qualifier (no ambiguity)
						// of a node (0, y, z).
						// NOTE: If all TQ detected has no enclosed separators, (ie no ambiguity), then
						//       We don't need to explore neither the branches (1,y,z) nor the branch (2,y,z).
						//       But we have the solution with the text qualifier.
						pExploredInfo->NewTQBranchNyz (0, tchszSep, strPrev, strLast);
					}
				}
				else
				if (tchTextQualifier == _T('\''))
				{
					if (tchszSep == _T('\''))
					{
						//
						// Add the possible separators for the Text Qualifiers
						// of a node (1, y, z):
						pExploredInfo->NewTQBranchNyz (1, tchszSep, strPrev, strLast);
					}
				}
				else
				if (tchTextQualifier == _T('\"'))
				{
					if (tchszSep == _T('\"') )
					{
						//
						// Add the possible separators for the Text Qualifiers
						// of a node (2, y, z):
						pExploredInfo->NewTQBranchNyz (2, tchszSep, strPrev, strLast);
					}
				}
			}

			if (bValidTextQualifier && (tchszSep == tchTextQualifier))
			{
				//
				// We're exploring the branch (1, y, z) or (2, y,z):
				stack.PushState(tchszSep);
				strPrev = tchszSep;
				pch = _tcsinc(pch);
				continue;
			}
		}

		//
		// Check to see if the first character of 'pch' is a separator or part of 
		// separator (if user specifies separator which composes of multiple characters)

		// If 'pSep' is NOT NULL, then 'pSep' can be either a single character or multiple characters:
		CaSeparator* pSep = pTemplate->CanBeSeparator (pch);
		if (pSep) // The first part of the current 'pch' is a separator 
		{
			CString& strSep = pSep->GetSeparator();
			//
			// We know that the first part of the current 'pch' is a separator,
			// has it already been counted for the current line ?
			CaSeparator* pCurrentSep = pSeparatorSet->GetSeparator(strSep);
			if (pCurrentSep) // 'pCurrentSep' has already been counted for the current line or user specified:
			{
				BOOL bFoundCS = FALSE; 
				//
				// 'pCurrentSep'can be consecutive separator only if the previous token is a separator:
				CaSeparator* pPrevSeparator = pSeparatorSet->GetSeparator (strPrev);
				if (pPrevSeparator && pCurrentSep->GetSeparator().CompareNoCase(strPrev) == 0)
					bFoundCS = TRUE;

				if (bCSO) // We're exploring the brabch (x,1,z)
				{
					if (bFoundCS)
					{
						if (pCurrentSep->GetCount() == 0) // Normally, pCurrentSep->GetCount() should be >= 1
							pCurrentSep->GetCount()++; // Count it only once
					}
					else
						pCurrentSep->GetCount()++; // Count it again
				}
				else // We're exploring the brabch (x,0,z)
				{
					pCurrentSep->GetCount()++; // Count it again
				}

				if (bFoundCS)
				{
					switch (tchTextQualifier)
					{
					case _T('\0'): // We are exploring the branch (0,y,z). ie, No Text Qualifier is to be considered:
						if (bCSO)  // We're exploring the branch (0,1,z)
						{
							pExploredInfo->NewCSBranchNyz (0, strPrev, strSep);
						}
						else  // We're exploring the branch (0,0,z)
						if (pInfo->GetDetectCS())
						{
							//
							// WE ONLY DETECT THE POSSIBLE ALTERNATIVE SOLUTION WITH CS.
							if (pExploredInfo->GetLineCS0yz() == -1)
								pExploredInfo->SetLineCS0yz(pInfo->GetLine());
						}
						break;
					case _T('\''): // We are exploring the branch (1, y, z). ie, Single Quote is considered as Text Qualifier.
						if (bCSO)  // We're exploring the branch (1,1,z)
						{
							pExploredInfo->NewCSBranchNyz (1, strPrev, strSep);
						}
						else
						if (pInfo->GetDetectCS())
						{
							//
							// WE ONLY DETECT THE POSSIBLE ALTERNATIVE SOLUTION WITH CS.
							if (pExploredInfo->GetLineCS1yz() == -1)
								pExploredInfo->SetLineCS1yz(pInfo->GetLine());
						}
						break;
					case _T('\"'): // We are exploring the branch (2, y, z). ie, Double Quote is considered as Text Qualifier.
						if (bCSO)  // We are exploring the branch (2,1,z)
						{
							pExploredInfo->NewCSBranchNyz (2, strPrev, strSep);
						}
						else
						if (pInfo->GetDetectCS())
						{
							//
							// WE ONLY DETECT THE POSSIBLE ALTERNATIVE SOLUTION WITH CS.
							if (pExploredInfo->GetLineCS2yz() == -1)
								pExploredInfo->SetLineCS2yz(pInfo->GetLine());
						}
						break;
					default:
						ASSERT(FALSE);
						break;
					}
				}
			}
			else
			{
				//
				// 'pCurrentSep' has not yet counted for the current line:
				// Thus, it cannot be consecutive since it is the first time we meet it.
				pSeparatorSet->NewSeparator (strSep);
			}

			int nLen = strSep.GetLength();
			strPrev = strSep;
			pch = _tcsninc (pch, nLen);
			tchszSep = *pch;
			continue;
		}
		else
		if (nl == 0 && CandidateSeparator (tchszSep)) // Only single charactor !
		{
			CString strNewSep = tchszSep;
			pTemplate->NewSeparator (strNewSep);
		}

		strPrev = tchszSep;
		pch = _tcsinc(pch);
	}

	if (bTS || pInfo->GetDetectTS()) // If we need to detect the trailing separators.
	{
		CString strTrailSeparator;
		CString strTS = _T("");
		CTypedPtrList< CObList, CaSeparator* >& lch = pSeparatorSet->GetListCharSeparator();
		CTypedPtrList< CObList, CaSeparator* >& lst = pSeparatorSet->GetListStrSeparator();
		POSITION pos = lst.GetHeadPosition();
		while (pos != NULL)
		{
			CaSeparator* pSep = lst.GetNext(pos);
			CString& strSep = pSep->GetSeparator();
			int nLen = strSep.GetLength();
			strTrailSeparator = strLine.Right(nLen);
			if (strSep.CompareNoCase (strTrailSeparator) == 0)
			{
				strTS = strSep;
				break;
			}
		}

		strTrailSeparator = strLine.Right(1);
		pos = lch.GetHeadPosition();
		while (pos != NULL && strTS.IsEmpty())
		{
			CaSeparator* pSep = lch.GetNext(pos);
			CString& strSep = pSep->GetSeparator();
			if (strSep.CompareNoCase (strTrailSeparator) == 0)
			{
				strTS = strSep;
				break;
			}
		}
		
		if (!strTS.IsEmpty())
		{
			if (pInfo->GetDetectTS())
			{
				switch (tchTextQualifier)
				{
				case _T('\0'): // We are exploring the branch (0 ,y, z).
					pExploredInfo->NewTSBranchNyz (0, strTS);
					if (pExploredInfo->GetLineTS0yz() == -1)
						pExploredInfo->SetLineTS0yz(pInfo->GetLine());
					break;
				case _T('\''): // We are exploring the branch (1, y, z).
					pExploredInfo->NewTSBranchNyz (1, strTS);
					if (pExploredInfo->GetLineTS1yz() == -1)
						pExploredInfo->SetLineTS1yz(pInfo->GetLine());
					break;
				case _T('\"'): // We are exploring the branch (2, y, z).
					pExploredInfo->NewTSBranchNyz (2, strTS);
					if (pExploredInfo->GetLineTS2yz() == -1)
						pExploredInfo->SetLineTS2yz(pInfo->GetLine());
					break;
				default:
					ASSERT(FALSE);
					break;
				}
			}

			if (bTS) // Ignore trailing separators
			{
				CaSeparator* pTrailingSeparator = pSeparatorSet->GetSeparator (strTS);
				if (bCSO) // Consecutive separators are considered as one.
				{
					pTrailingSeparator->GetCount()--;
					if (pTrailingSeparator->GetCount() <= 0) // Normally it should not happen
						pSeparatorSet->RemoveSeparator (pTrailingSeparator);
				}
				else // Not consider Consecutive separators as one.
				{
					int nFound, nLen = strTS.GetLength();
					int nSumLen = nLen;
					strTrailSeparator = strLine.Right(nSumLen);
					nFound = strTrailSeparator.Find(strTS, 0);

					while (nFound == 0)
					{
						pTrailingSeparator->GetCount()--;

						strTrailSeparator = strLine.Right(nSumLen + nLen);
						nFound = strTrailSeparator.Find(strTS, 0);
						if (nFound == 0)
						{
							nSumLen += nLen;
						}
					}
					if (pTrailingSeparator->GetCount() <= 0) // Normally it should not happen
						pSeparatorSet->RemoveSeparator (pTrailingSeparator);
				}

				switch (tchTextQualifier)
				{
				case _T('\0'): // We are exploring the branch (0 ,y, z).
					pExploredInfo->NewTSBranchNyz (0, strTS);
					break;
				case _T('\''): // We are exploring the branch (1, y, z).
					pExploredInfo->NewTSBranchNyz (1, strTS);
					break;
				case _T('\"'): // We are exploring the branch (2, y, z).
					pExploredInfo->NewTSBranchNyz (2, strTS);
					break;
				default:
					ASSERT(FALSE);
					break;
				}
			}
		}
	}

	// If the number and order of columns of the file expected to match
	// exactly those of the Ingres Table ? 
	int nNumberOfColumn = pExploredInfo->GetColumnCount();
	if (nNumberOfColumn > 0) // YES
	{
		CTypedPtrList< CObList, CaSeparator* >& lch = pSeparatorSet->GetListCharSeparator();
		CTypedPtrList< CObList, CaSeparator* >& lst = pSeparatorSet->GetListStrSeparator();
		
		POSITION p, pos = lch.GetHeadPosition();
		while (pos != NULL)
		{
			p = pos;
			CaSeparator* pSep = lch.GetNext(pos);
			//
			// To match the table column, the number of separator must be
			// equal to number of (columns - 1).
			if (pSep->GetCount() != (nNumberOfColumn-1))
			{
				lch.RemoveAt(p); 
				delete pSep;
			}
		}

		pos = lst.GetHeadPosition();
		while (pos != NULL)
		{
			p = pos;
			CaSeparator* pSep = lst.GetNext(pos);
			//
			// To match the table column, the number of separator must be
			// equal to number of (columns - 1).
			if (pSep->GetCount() != (nNumberOfColumn-1))
			{
				lst.RemoveAt(p);
				delete pSep;
			}
		}
	}
}



void CaTreeSolution::CaTreeNodeDetection::Detection(CaExploredInfo* pExploredInfo)
{
	POSITION pos = m_listChildren.GetHeadPosition();
	while (pos != NULL)
	{
		if (theApp.m_synchronizeInterrupt.IsRequestCancel())
		{
			theApp.m_synchronizeInterrupt.BlockUserInterface (FALSE); // Release user interface
			theApp.m_synchronizeInterrupt.WaitWorkerThread ();        // Wait itself until user interface releases it.
			if (pExploredInfo->GetIIAInfo()->GetInterrupted())
				return;
		}
		else
		{
			if (pExploredInfo->GetIIAInfo()->GetInterrupted())
				return;
		}

		CaTreeNodeDetection* pChildNode = m_listChildren.GetNext(pos);
		int nTQ = pChildNode->GetTextQualifier();
		int nCS = pChildNode->GetConsecutiveSeparator();
		int nTS = pChildNode->GetTrailingSeparator();

		switch (nTQ)
		{
		case 0: // left most
			break;
		case 1: // <'> is text qualifier.
			ASSERT (pExploredInfo);
			if (pExploredInfo && pExploredInfo->GetLineSQ() == -1)
			{
				//
				// If we have not detected the Single Quote (SQ) while exploring
				// the node (0,0,0) then DO NOT explore the node (1,y,z).
				continue;
			}
			break;
		case 2: // <"> is text qualifier.
			ASSERT (pExploredInfo);
			if (pExploredInfo && pExploredInfo->GetLineDQ() == -1)
			{
				//
				// If we have not detected the Double Quote (DQ) while exploring
				// the node (0,0,0) then DO NOT explore the node (2,y,z).
				continue;
			}
			break;
		default:
			ASSERT(FALSE);
			break;
		}

		switch (nCS)
		{
		case -1: // Internal node
		case  0: // Ignore Consecutive Separators
			break;
		case 1: // Consecutive Separators are considered as one
			if (nTQ == 0 && pExploredInfo->GetLineCS0yz() == -1)
			{
				//
				// If we have not detected the Consecutive Separators (CS) while exploring
				// the node (0,0,0) then DO NOT explore the node (0,1,z).
				continue;
			}
			if (nTQ == 1 && pExploredInfo->GetLineCS1yz() == -1)
			{
				//
				// If we have not detected the Consecutive Separators (CS) while exploring
				// the node (1,0,0) then DO NOT explore the node (1,1,z).
				continue;
			}
			if (nTQ == 2 && pExploredInfo->GetLineCS2yz() == -1)
			{
				//
				// If we have not detected the Consecutive Separators (CS) while exploring
				// the node (2,0,0) then DO NOT explore the node (2,1,z).
				continue;
			}
			break;
		default:
			break;
		}

		switch (nTS)
		{
		case -1: // Internal node
		case  0: // Not ignore Trailing Separators
			break;
		case 1: // Ignore Trailing Separators
			if (nTQ == 0 && pExploredInfo->GetLineTS0yz() == -1)
			{
				//
				// If we have not detected the Trailing Separators (TS) while exploring
				// the node (0,0,0) then DO NOT explore the node (0,y,1).
				continue;
			}

			if (nTQ == 1 && pExploredInfo->GetLineTS1yz() == -1)
			{
				//
				// If we have not detected the Trailing Separators (TS) while exploring
				// the node (1,0,0) then DO NOT explore the node (1,y,1).
				continue;
			}
			if (nTQ == 2 && pExploredInfo->GetLineTS2yz() == -1)
			{
				//
				// If we have not detected the Trailing Separators (TS) while exploring
				// the node (2,0,0) then DO NOT explore the node (2,y,1).
				continue;
			}

			break;
		default:
			break;
		}

		pChildNode->Detection(pExploredInfo);
	}
	//
	// Process only leaf nodes (Leaf node don't have the children):
	if (!m_listChildren.IsEmpty())
		return;

	int  nInitSepSaratorSet = 0;
	int  nSize = m_pArrayLine->GetSize();
	BOOL bDetectSQ = FALSE;
	BOOL bDetectDQ = FALSE;
	BOOL bDetectCS = FALSE;
	BOOL bDetectTS = FALSE;

	switch (m_nTQ)
	{
	case 0: // NO Text Qualifier 
		m_separatorSet.SetTextQualifier(_T('\0'));
		if (m_nCSO == 0) // No CS and no TS. ie node (0,0,0)
		{
			m_separatorSet.SetConsecutiveSeparatorsAsOne(FALSE);
			if (m_nTS == 0) // No CS and no TS. ie node (0,0,0)
			{
				m_separatorSet.SetIgnoreTrailingSeparator(FALSE);
				m_nIndex = 0; // Start scanning from the begining (m_nIndex = 0)
				bDetectSQ = bDetectDQ = bDetectCS = bDetectTS = TRUE;
				CaDetectLineInfo detectInfo (nInitSepSaratorSet, bDetectSQ, bDetectDQ, bDetectCS, bDetectTS);
				CString strLine(m_pArrayLine->GetAt(nInitSepSaratorSet));
				m_separatorSet.InitUserSet(pExploredInfo->GetOtherSeparator());
				CountSeparator(&m_separatorSet, &m_separatorSet, pExploredInfo, strLine, &detectInfo);
			}
			else // No CS and TS. ie node (0,0,1)
			{
				m_separatorSet.SetIgnoreTrailingSeparator(TRUE);
				m_nIndex = pExploredInfo->GetLineTS0yz();
				if (pExploredInfo->GetLastScannedLine0yz() < (nSize-1))
				{
					//
					// If we have not already scanned the whole file, then continue detecting the following:
					bDetectSQ = TRUE; 
					bDetectDQ = TRUE;
					bDetectCS = TRUE;
				}
				CaDetectLineInfo detectInfo (nInitSepSaratorSet, bDetectSQ, bDetectDQ, bDetectCS, bDetectTS);
				CString strLine(m_pArrayLine->GetAt(nInitSepSaratorSet));
				m_separatorSet.InitUserSet(pExploredInfo->GetOtherSeparator());
				CountSeparator(&m_separatorSet, &m_separatorSet, pExploredInfo, strLine, &detectInfo);
			}
		}
		else // Consecutive separators considered as one.
		{
			m_separatorSet.SetConsecutiveSeparatorsAsOne(TRUE);
			if (m_nTS == 0) // CS and no TS. ie node (0,1,0)
			{
				m_separatorSet.SetIgnoreTrailingSeparator(FALSE);
				m_nIndex = pExploredInfo->GetLineCS0yz(); // Start scanning from the index at which we have detected CS.
				if (pExploredInfo->GetLastScannedLine0yz() < (nSize-1))
				{
					//
					// If we have not already scanned the whole file, then continue detecting the following:
					bDetectSQ = TRUE;
					bDetectDQ = TRUE;
					bDetectTS = TRUE;
				}
				CaDetectLineInfo detectInfo (nInitSepSaratorSet, bDetectSQ, bDetectDQ, bDetectCS, bDetectTS);
				CString strLine(m_pArrayLine->GetAt(nInitSepSaratorSet));
				m_separatorSet.InitUserSet(pExploredInfo->GetOtherSeparator());
				CountSeparator(&m_separatorSet, &m_separatorSet, pExploredInfo, strLine, &detectInfo);
			}
			else // CS and TS. ie node (0,1,1)
			{
				m_separatorSet.SetIgnoreTrailingSeparator(TRUE);
				//
				// Start scanning from the min index at which we have detected CS and TS
				m_nIndex = min (pExploredInfo->GetLineCS0yz(), pExploredInfo->GetLineTS0yz());
				if (pExploredInfo->GetLastScannedLine0yz() < (nSize-1))
				{
					//
					// If we have not already scanned the whole file, then continue detecting the following:
					bDetectSQ = TRUE;
					bDetectDQ = TRUE;
				}
				CString strLine(m_pArrayLine->GetAt(nInitSepSaratorSet));
				m_separatorSet.InitUserSet(pExploredInfo->GetOtherSeparator());
				CaDetectLineInfo detectInfo(nInitSepSaratorSet, bDetectSQ, bDetectDQ, bDetectCS, bDetectTS);
				CountSeparator(&m_separatorSet, &m_separatorSet, pExploredInfo, strLine, &detectInfo);
			}
		}
		break;
	case 1: // Text Qualifier is Single Quote 
		ASSERT (pExploredInfo->GetLineSQ() >= 0);
		m_separatorSet.SetTextQualifier(_T('\''));
		if (m_nCSO == 0) // No Consecutive separators considered as one.
		{
			m_separatorSet.SetConsecutiveSeparatorsAsOne(FALSE);
			m_separatorSet.InitUserSet(pExploredInfo->GetOtherSeparator());
			if (m_nTS == 0) // Not CS and not TS. ie node (1,0,0)
			{
				m_separatorSet.SetIgnoreTrailingSeparator(FALSE);
				bDetectCS = TRUE; // Always scan the CS because the CS found in (0,0,0) is not compatible with the one in (1,0,0).
				bDetectTS = TRUE; // Always scan the TS because the TS found in (0,0,0) is not compatible with the one in (1,0,0).
				m_nIndex = pExploredInfo->GetLineSQ();  // Start scanning from the index at which we have detected SQ.
				CString strLine(m_pArrayLine->GetAt(nInitSepSaratorSet));
				int nLineCS0y = pExploredInfo->GetLineCS0yz();
				int nLineTS0y = pExploredInfo->GetLineTS0yz();
				if (nLineCS0y >= 0 && m_nIndex >= nLineCS0y)
					pExploredInfo->SetLineCS1yz (nLineCS0y);
				if (nLineTS0y >= 0 && m_nIndex >= nLineTS0y)
					pExploredInfo->SetLineTS1yz (nLineTS0y);
				CaDetectLineInfo detectInfo(nInitSepSaratorSet, bDetectSQ, bDetectDQ, bDetectCS, bDetectTS);
				CountSeparator(&m_separatorSet, &m_separatorSet, pExploredInfo, strLine, &detectInfo);
			}
			else // Not CS and TS. ie node (1,0,1)
			{
				m_separatorSet.SetIgnoreTrailingSeparator(TRUE);
				if (pExploredInfo->GetLastScannedLine1yz() < (nSize-1))
					bDetectCS = TRUE;
				// 
				// Start scanning from the smallest index at which we have detected TS, and SQ
				m_nIndex = min (pExploredInfo->GetLineTS1yz(), pExploredInfo->GetLineSQ());
				CString strLine(m_pArrayLine->GetAt(m_nIndex));
				CaDetectLineInfo detectInfo(m_nIndex, bDetectSQ, bDetectDQ, bDetectCS, bDetectTS);
				CountSeparator(&m_separatorSet, &m_separatorSet, pExploredInfo, strLine, &detectInfo);
			}
		}
		else // Consecutive separators considered as one.
		{
			m_separatorSet.SetConsecutiveSeparatorsAsOne(TRUE);
			if (m_nTS == 0) // CS and not TS. ie node (1,1,0)
			{
				m_separatorSet.SetIgnoreTrailingSeparator(FALSE);
				// 
				// Start scanning from the smallest index at which we have detected CS, and SQ
				m_nIndex = min (pExploredInfo->GetLineSQ(), pExploredInfo->GetLineCS1yz());
				if (pExploredInfo->GetLastScannedLine1yz() < (nSize-1))
					bDetectTS = TRUE;
				CString strLine(m_pArrayLine->GetAt(nInitSepSaratorSet));
				m_separatorSet.InitUserSet(pExploredInfo->GetOtherSeparator());
				CaDetectLineInfo detectInfo(nInitSepSaratorSet, bDetectSQ, bDetectDQ, bDetectCS, bDetectTS);
				CountSeparator(&m_separatorSet, &m_separatorSet, pExploredInfo, strLine, &detectInfo);
			}
			else // CS and TS. ie node (1,1,1)
			{
				m_separatorSet.SetIgnoreTrailingSeparator(TRUE);
				// 
				// Start scanning from the smallest index at which we have detected CS, TS, and SQ
				m_nIndex = min (pExploredInfo->GetLineSQ(), pExploredInfo->GetLineCS1yz());
				m_nIndex = min (m_nIndex, pExploredInfo->GetLineTS1yz());
				CString strLine(m_pArrayLine->GetAt(nInitSepSaratorSet));
				m_separatorSet.InitUserSet(pExploredInfo->GetOtherSeparator());
				CaDetectLineInfo detectInfo(nInitSepSaratorSet, bDetectSQ, bDetectDQ, bDetectCS, bDetectTS);
				CountSeparator(&m_separatorSet, &m_separatorSet, pExploredInfo, strLine, &detectInfo);
			}
		}
		break;
	case 2: // Text Qualifier is Double Quote 
		m_separatorSet.SetTextQualifier(_T('\"'));
		if (m_nCSO == 0) // No Consecutive separators considered as one.
		{
			m_separatorSet.SetConsecutiveSeparatorsAsOne(FALSE);
			if (m_nTS == 0) // Not CS and not TS. ie node (2,0,0)
			{
				m_separatorSet.SetIgnoreTrailingSeparator(FALSE);
				bDetectCS = TRUE; // Always scan the CS because the CS found in (0,0,0) is not compatible with the one in (2,0,0).
				bDetectTS = TRUE; // Always scan the TS because the TS found in (0,0,0) is not compatible with the one in (2,0,0).
				m_nIndex = pExploredInfo->GetLineDQ(); // Start scanning from the index where we have detected DQ.
				CString strLine(m_pArrayLine->GetAt(nInitSepSaratorSet));
				int nLineCS0y = pExploredInfo->GetLineCS0yz();
				int nLineTS0y = pExploredInfo->GetLineTS0yz();
				if (nLineCS0y >= 0 && m_nIndex >= nLineCS0y)
					pExploredInfo->SetLineCS2yz (nLineCS0y);
				if (nLineTS0y >= 0 && m_nIndex >= nLineTS0y)
					pExploredInfo->SetLineTS2yz (nLineTS0y);

				m_separatorSet.InitUserSet(pExploredInfo->GetOtherSeparator());
				CaDetectLineInfo detectInfo(nInitSepSaratorSet, bDetectSQ, bDetectDQ, bDetectCS, bDetectTS);
				CountSeparator(&m_separatorSet, &m_separatorSet, pExploredInfo, strLine, &detectInfo);
			}
			else // Not CS and TS. ie node (2,0,1)
			{
				m_separatorSet.SetIgnoreTrailingSeparator(TRUE);
				if (pExploredInfo->GetLastScannedLine2yz() < (nSize-1))
					bDetectCS = TRUE;
				// 
				// Start scanning from the smallest index at which we have detected TS, and DQ
				m_nIndex = min (pExploredInfo->GetLineDQ(), pExploredInfo->GetLineTS2yz());
				CString strLine(m_pArrayLine->GetAt(nInitSepSaratorSet));
				CaDetectLineInfo detectInfo(nInitSepSaratorSet, bDetectSQ, bDetectDQ, bDetectCS, bDetectTS);
				CountSeparator(&m_separatorSet, &m_separatorSet, pExploredInfo, strLine, &detectInfo);
			}
		}
		else // Consecutive separators considered as one.
		{
			m_separatorSet.SetConsecutiveSeparatorsAsOne(TRUE);
			if (m_nTS == 0) // CS and not TS. ie node (2,1,0)
			{
				m_separatorSet.SetIgnoreTrailingSeparator(FALSE);
				// 
				// Start scanning from the smallest index at which we have detected CS, and DQ
				m_nIndex = min (pExploredInfo->GetLineDQ(), pExploredInfo->GetLineCS2yz());
				if (pExploredInfo->GetLastScannedLine2yz() < (nSize-1))
					bDetectTS = TRUE;
				CString strLine(m_pArrayLine->GetAt(nInitSepSaratorSet));
				m_separatorSet.InitUserSet(pExploredInfo->GetOtherSeparator());
				CaDetectLineInfo detectInfo(nInitSepSaratorSet, bDetectSQ, bDetectDQ, bDetectCS, bDetectTS);
				CountSeparator(&m_separatorSet, &m_separatorSet, pExploredInfo, strLine, &detectInfo);
			}
			else // CS and TS. ie node (2,1,1)
			{
				m_separatorSet.SetIgnoreTrailingSeparator(TRUE);
				// 
				// Start scanning from the smallest index at which we have detected CS, and DQ and TS
				m_nIndex = min (pExploredInfo->GetLineDQ(), pExploredInfo->GetLineCS2yz());
				m_nIndex = min (m_nIndex, pExploredInfo->GetLineTS2yz());
				CString strLine(m_pArrayLine->GetAt(nInitSepSaratorSet));
				m_separatorSet.InitUserSet(pExploredInfo->GetOtherSeparator());
				CaDetectLineInfo detectInfo(nInitSepSaratorSet, bDetectSQ, bDetectDQ, bDetectCS, bDetectTS);
				CountSeparator(&m_separatorSet, &m_separatorSet, pExploredInfo, strLine, &detectInfo);
			}
		}
		break;
	default:
		ASSERT (FALSE);
		break;
	}
	
	//
	// Prepare the failure info:
	m_pListFailureInfo = new CTypedPtrList< CObList, CaFailureInfo* >;


	CTypedPtrList< CObList, CaSeparator* >& listCharSep = m_separatorSet.GetListCharSeparator();
	CTypedPtrList< CObList, CaSeparator* >& listStrSep  = m_separatorSet.GetListStrSeparator();
	ASSERT (m_nIndex >= 0 && m_nIndex < nSize);
	if (m_nIndex < 0 || m_nIndex >= nSize)
		return;
	int nCurIndex = m_nIndex;
	if (nCurIndex == 0)
		nCurIndex++;
	while (nCurIndex < nSize && (!listCharSep.IsEmpty() || !listStrSep.IsEmpty()))
	{
		if (theApp.m_synchronizeInterrupt.IsRequestCancel())
		{
			theApp.m_synchronizeInterrupt.BlockUserInterface (FALSE); // Release user interface
			theApp.m_synchronizeInterrupt.WaitWorkerThread ();        // Wait itself until user interface releases it.
			if (pExploredInfo->GetIIAInfo()->GetInterrupted())
				return;
		}
		else
		{
			if (pExploredInfo->GetIIAInfo()->GetInterrupted())
				return;
		}

		CaSeparatorSet tempSet;
		CaDetectLineInfo detectInfo (nCurIndex, bDetectSQ, bDetectDQ, bDetectCS, bDetectTS);
		CString strLine = m_pArrayLine->GetAt(nCurIndex);
		tempSet.SetConsecutiveSeparatorsAsOne(m_separatorSet.GetConsecutiveSeparatorsAsOne());
		tempSet.SetIgnoreTrailingSeparator(m_separatorSet.GetIgnoreTrailingSeparator());
		tempSet.SetTextQualifier(m_separatorSet.GetTextQualifier());
		CountSeparator(&tempSet, &m_separatorSet, pExploredInfo, strLine, &detectInfo);

		CString strRejectedSeparators = _T("");
		POSITION p, ps = listStrSep.GetHeadPosition();
		while (ps != NULL)
		{
			p = ps;
			CaSeparator* pObj = listStrSep.GetNext (ps);
			CString& strSep = pObj->GetSeparator();
			CaSeparator* pCurSep = tempSet.GetSeparator (strSep);
		
			if (!pCurSep || (pCurSep && (pCurSep->GetCount() != pObj->GetCount())))
			{
				//
				// The candidate seperator 'pObj' found in the first line (record) is rejected
				// because there is no such a separator in the subsequent records:
				listStrSep.RemoveAt (p);

				//
				// Remove it too if it is part of a consecutive separator:
				if (m_separatorSet.GetConsecutiveSeparatorsAsOne())
					m_separatorSet.RemoveConsecutiveSeparators(pObj->GetSeparator());

				strRejectedSeparators += pObj->GetSeparator();
				strRejectedSeparators += _T(" ");

				delete pObj;
				continue;
			}
		}

		ps = listCharSep.GetHeadPosition();
		while (ps != NULL)
		{
			p = ps;
			CaSeparator* pObj = listCharSep.GetNext (ps);
			TCHAR tchszSep = pObj->GetSeparator().GetAt(0);
			CaSeparator* pCurSep = tempSet.GetSeparator (tchszSep);
		
			if (!pCurSep || (pCurSep && (pCurSep->GetCount() != pObj->GetCount())))
			{
				//
				// The candidate seperator 'pObj' found in the first line (record) is rejected
				// because there is no such a separator in the subsequent records:
				listCharSep.RemoveAt (p);
				//
				// Remove it too if it is part of a consecutive separator:
				if (m_separatorSet.GetConsecutiveSeparatorsAsOne())
					m_separatorSet.RemoveConsecutiveSeparators(pObj->GetSeparator());
				
				CString strSep = pObj->GetSeparator();
				if (strSep == _T(" "))
				{
					strRejectedSeparators += _T("<space>");
				}
				else
				if (strSep == _T("\t"))
				{
					strRejectedSeparators += _T("<tab>");
				}
				else
				{
					strRejectedSeparators += strSep;
				}
				strRejectedSeparators += _T(" ");

				delete pObj;
				continue;
			}
		}

		//
		// Allocate the failure info:
		if (!strRejectedSeparators.IsEmpty())
		{
			CaFailureInfo* pFailureInfo = new CaFailureInfo();
			pFailureInfo->m_nLine = nCurIndex+1;
			pFailureInfo->m_strLine = strLine;
			pFailureInfo->m_bCS = m_nCSO;
			pFailureInfo->m_bTS = m_nTS;
			if (m_nTQ == 1)
				pFailureInfo->m_tchTQ = _T('\'');
			else
			if (m_nTQ == 2)
				pFailureInfo->m_tchTQ = _T('\"');
			pFailureInfo->m_strSeparator = strRejectedSeparators;
			pFailureInfo->m_strLine = strLine;
			m_pListFailureInfo->AddHead(pFailureInfo);
		}

		switch (m_nTQ)
		{
		case 0:
			if (pExploredInfo->GetLastScannedLine0yz() < (nSize-1))
			{
				if (nCurIndex > pExploredInfo->GetLastScannedLine0yz())
					pExploredInfo->SetLastScannedLine0yz(nCurIndex);
			}
			break;
		case 1:
			if (pExploredInfo->GetLastScannedLine1yz() < (nSize-1))
			{
				if (nCurIndex > pExploredInfo->GetLastScannedLine1yz())
					pExploredInfo->SetLastScannedLine1yz(nCurIndex);
			}
			break;
		case 2:
			if (pExploredInfo->GetLastScannedLine2yz() < (nSize-1))
			{
				if (nCurIndex > pExploredInfo->GetLastScannedLine2yz())
					pExploredInfo->SetLastScannedLine2yz(nCurIndex);
			}
			break;
		default:
			ASSERT(FALSE);
			break;
		}

		nCurIndex++;
	}

	if (nCurIndex == nSize && (!listCharSep.IsEmpty() || !listStrSep.IsEmpty()))
	{
		m_nNodeType = BRANCH_SUCCEEDED; // Succeed.
		//
		// Some of the solutions found in the leaf may be not valid.
		// If such a solution exists, we remove it here.
		// Here is the tipical example: "ABC,JJK",JKL;MM
		// One of the possible solution in CaSeparatorSet 'pSet' is:
		// TQ = "
		// Separator = {, or ;}
		// The TQ is valid for , but not valid for ;
#if defined (_DEBUG) && defined (_TRACEINFO) && !defined (_UNICODE)
		CString strTrace;
		strTrace.Format (_T("SUCCEEDED NODE (TQ, CS, TS)=(%d, %d, %d):\n"), m_nTQ, m_nCSO, m_nTS);
		TRACE0(strTrace);
		POSITION px = listCharSep.GetHeadPosition();
		while (px != NULL)
		{
			CaSeparator* pObjSep = listCharSep.GetNext (px);
			strTrace.Format ("\tSEP = %s COUNT(%d)\n",
				pObjSep->GetSeparator(), 
				pObjSep->GetCount());;
			TRACE0(strTrace);
		}

		px = listStrSep.GetHeadPosition();
		while (px != NULL)
		{
			CaSeparator* pObjSep = listStrSep.GetNext (px);
			strTrace.Format ("\tSEP = %s COUNT(%d)\n",
				pObjSep->GetSeparator(), 
				pObjSep->GetCount());;
			TRACE0(strTrace);
		}
		TRACE0("-----------------------------------------------------------------------------------\n");
#endif
	}
	else
	{
		m_nNodeType = BRANCH_FAILED;    // Failed.
#if defined (_DEBUG) && defined (_TRACEINFO) && !defined (_UNICODE)
		CString strTrace;
		strTrace.Format (_T("FAILED NODE (TQ, CS, TS)=(%d, %d, %d):\n"), m_nTQ, m_nCSO, m_nTS);
		TRACE0(strTrace);
		POSITION px = m_pListFailureInfo->GetHeadPosition();
		while (px != NULL)
		{
			CaFailureInfo* pFailureInfo = m_pListFailureInfo->GetNext (px);
			strTrace.Format ("\tReject [%s] at Line=%d\n",
				pFailureInfo->m_strSeparator,
				pFailureInfo->m_nLine);
			TRACE0(strTrace);
		}
		TRACE0("-----------------------------------------------------------------------------------\n");
#endif
	}
}


void CaTreeSolution::CaTreeNodeDetection::Traverse (CaIIAInfo* pData, CTypedPtrList < CObList, CaSeparatorSet* >& listSolution)
{
	CaDataPage1& dataPage1 = pData->m_dataPage1;

	POSITION pos = m_listChildren.GetHeadPosition();
	while (pos != NULL)
	{
		CaTreeNodeDetection* pChildNode = m_listChildren.GetNext(pos);
		pChildNode->Traverse(pData, listSolution);
	}

	if (m_nNodeType == BRANCH_SUCCEEDED)
	{
		CaSeparatorSet* pSolution = new CaSeparatorSet(m_separatorSet);
		listSolution.AddTail(pSolution);
	}
	else
	if (m_nNodeType == BRANCH_FAILED && m_pListFailureInfo)
	{
		CTypedPtrList< CObList, CaFailureInfo* >& listFailureInfo = dataPage1.GetListFailureInfo();

		POSITION px = m_pListFailureInfo->GetHeadPosition();
		while (px != NULL)
		{
			CaFailureInfo* pFailureInfo = m_pListFailureInfo->GetNext (px);
			listFailureInfo.AddTail(pFailureInfo);
		}
		m_pListFailureInfo->RemoveAll();
	}
}




void CaTreeSolution::FindSolution (CaIIAInfo* pData)
{
	CaDataPage1& dataPage1 = pData->m_dataPage1;
	CaDataPage2& dataPage2 = pData->m_dataPage2;

	CStringArray& arrayLine = dataPage1.GetArrayTextLine();
	if (arrayLine.GetSize() == 0)
		return;

	CTypedPtrList < CObList, CaSeparatorSet* >& listChoice = dataPage2.GetListChoice();
	CaTreeNodeDetection treeSolution(&arrayLine, 0);
	treeSolution.InitializeRoot();

	//
	// When exploring the solution at the first branch, the left most one,
	// any embiguity (there exist an alternative depending on using Simple Quote, Double Quote, Consecutive Separator)
	// detected during this phase, is stored in the parameter 'exploredInfo'.
	// The parameter is a kind of heuristic that dictates the function 'Detection()' how to explore the remaining
	// right branches.
	// 1) If none of {SQ, DQ, CS} has been detected, then the right branches will not be explored.
	// 2) Let say M a subset of {SQ, DQ, CS}, nSize the size of the array 'arrayLine'.
	//    Explore the right branches corresponding to element e of M from the detected line.
	//    And make the detection of every x that is not in M but in {SQ, DQ, CS} if the detected line is
	//    < nSise.
	CaExploredInfo exploredInfo (pData, m_strOtherSeparator, m_nColumn);
	treeSolution.Detection(&exploredInfo);

	//
	// Traverse tree to get the list of solutions:
	CTypedPtrList < CObList, CaSeparatorSet* > listSolutionSet;
	dataPage1.CleanupFailureInfo();
	treeSolution.Traverse (pData, listSolutionSet);

	BOOL bWarning = FALSE;
	POSITION pDel, p, pos = listSolutionSet.GetHeadPosition();
	while (pos != NULL)
	{
		CaSeparatorSet* pSet = listSolutionSet.GetNext (pos);
		CTypedPtrList< CObList, CaSeparator* >& lch = pSet->GetListCharSeparator();
		CTypedPtrList< CObList, CaSeparator* >& lstr= pSet->GetListStrSeparator();
		//
		// Grouping the separators into one single list, the list 'lstr'
		while (!lch.IsEmpty())
		{
			CaSeparator* pObj = lch.RemoveHead();
			lstr.AddTail(pObj);
		}

		//
		// Create a new list 'listChoice' where each element is an object of 
		// class CaSeparatorSet. Let E the element of 'listChoice', E must verify the following:
		//    - E->m_listCharSeparator is empty.
		//    - E->m_listStrSeparator contains ONLY one element of class CaSeparator.
		BOOL  bCSO  = pSet->GetConsecutiveSeparatorsAsOne();
		BOOL  bTS   = pSet->GetIgnoreTrailingSeparator();
		TCHAR tchTQ = pSet->GetTextQualifier();
		p = lstr.GetHeadPosition();
		while (p != NULL)
		{
			pDel = p;
			CaSeparator* pSeparator = lstr.GetNext (p);
			lstr.RemoveAt (pDel);
			CaSeparatorSet* pNewSet = new CaSeparatorSet();

			if (tchTQ == _T('\0'))
			{
				//
				// At this branch node (0, y), we don't make a choice for the TQ that is not
				// ambigous. Thus if we detect that there can be both SQ and DQ not ambiguous
				// we will issue the message that some fields are qualified with SQ and some
				// fields are qualified with DQ. Such a solution is not suggested.
				BOOL bSq = exploredInfo.ExistTQ (_T('\''), pSeparator->GetSeparator(), 0);
				BOOL bDq = exploredInfo.ExistTQ (_T('\"'), pSeparator->GetSeparator(), 0);
				if (bSq && bDq)
				{
					delete pNewSet;
					delete pSeparator;
					bWarning = TRUE;
					continue;
				}
				//
				// If we found the SQ or DQ without ambiguity, then the solution will become
				// a Text Qualifier even though we are at the branch No Text Qualifier.
				if (bSq)
					pNewSet->SetTextQualifier (_T('\''));
				else
				if (bDq)
					pNewSet->SetTextQualifier (_T('\"'));
				else
					pNewSet->SetTextQualifier (_T('\0'));

				if (!bCSO) // Not Consecutive as one
				{
					pNewSet->SetConsecutiveSeparatorsAsOne (FALSE);
				}
				else // Consecutive as one
				{
					pNewSet->SetConsecutiveSeparatorsAsOne (TRUE);
					if (!exploredInfo.ExistCS (pSeparator->GetSeparator(), 0))
					{
						delete pNewSet;
						delete pSeparator;
						continue;
					}
				}
				if (bTS)
				{
					pNewSet->SetIgnoreTrailingSeparator(TRUE);
					if (!exploredInfo.ExistTS (pSeparator->GetSeparator(), 0))
					{
						delete pNewSet;
						delete pSeparator;
						continue;
					}
				}
			}
			else
			if (tchTQ == _T('\''))
			{
				//
				// At this branch node(1, y), the choice is SQ.
				// Any DQ found within this choice is ignored:
				pNewSet->SetTextQualifier (_T('\''));
				BOOL bSq1 = exploredInfo.ExistTQ (_T('\''), pSeparator->GetSeparator(), 1);
				if (!bSq1)
				{
					delete pNewSet;
					delete pSeparator;
					continue;
				}
				/*
				BOOL bDq0 = exploredInfo.ExistTQ (_T('\"'), pSeparator->GetSeparator(), 0);
				BOOL bDq1 = exploredInfo.ExistTQ (_T('\"'), pSeparator->GetSeparator(), 1);
				if (bDq0 || bDq1)
				{
					delete pNewSet;
					delete pSeparator;
					bWarning = TRUE;
					continue;
				}
				*/

				if (!bCSO) // Not Consecutive as one
				{
					pNewSet->SetConsecutiveSeparatorsAsOne (FALSE);
				}
				else // Consecutive as one
				{
					pNewSet->SetConsecutiveSeparatorsAsOne (TRUE);
					if (!exploredInfo.ExistCS (pSeparator->GetSeparator(), 1))
					{
						delete pNewSet;
						delete pSeparator;
						continue;
					}
				}
				if (bTS)
				{
					pNewSet->SetIgnoreTrailingSeparator(TRUE);
					if (!exploredInfo.ExistTS (pSeparator->GetSeparator(), 1))
					{
						delete pNewSet;
						delete pSeparator;
						continue;
					}
				}
			}
			else
			if (tchTQ == _T('\"'))
			{
				//
				// At this branch node(2, y), the choice is DQ.
				// Any SQ found within this choice is ignored:
				pNewSet->SetTextQualifier (_T('\"'));
				BOOL bDq2 = exploredInfo.ExistTQ (_T('\"'), pSeparator->GetSeparator(), 2);
				if (!bDq2)
				{
					delete pNewSet;
					delete pSeparator;
					continue;
				}
				/*
				BOOL bSq0 = exploredInfo.ExistTQ (_T('\''), pSeparator->GetSeparator(), 0);
				BOOL bSq2 = exploredInfo.ExistTQ (_T('\''), pSeparator->GetSeparator(), 2);
				if (bSq0 || bSq2)
				{
					delete pNewSet;
					delete pSeparator;
					bWarning = TRUE;
					continue;
				}
				*/
				if (!bCSO)
				{
					pNewSet->SetConsecutiveSeparatorsAsOne (FALSE);
				}
				else
				{
					pNewSet->SetConsecutiveSeparatorsAsOne (TRUE);
					if (!exploredInfo.ExistCS (pSeparator->GetSeparator(), 2))
					{
						delete pNewSet;
						delete pSeparator;
						continue;
					}
				}
				if (bTS)
				{
					pNewSet->SetIgnoreTrailingSeparator(TRUE);
					if (!exploredInfo.ExistTS (pSeparator->GetSeparator(), 2))
					{
						delete pNewSet;
						delete pSeparator;
						continue;
					}
				}
			}

			CTypedPtrList< CObList, CaSeparator* >& listStrSepCur  = pNewSet->GetListStrSeparator();
			listStrSepCur.AddTail (pSeparator);

			listChoice.AddTail (pNewSet);
		}
	}

	while (!listSolutionSet.IsEmpty())
		delete listSolutionSet.RemoveHead();
	if (bWarning)
	{
		//
		// Some fields are qualified by \" and some by '.\nSuch a solution is not proposed.
		CString strMsg;
		strMsg.LoadString(IDS_MULTIPLE_TEXT_QUALIFIER);
		AfxMessageBox(strMsg);
	}

	//
	// Ensure that the final list has no redundant elements:
	POSITION px = NULL;
	pos = listChoice.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		CaSeparatorSet* pSet1 = listChoice.GetNext(pos);

		px = pos;
		while (px != NULL)
		{
			pDel = px;
			CaSeparatorSet* pSetX = listChoice.GetNext(px);
			//
			// Compare if pSet1 and pSetX are equal, if so we remove the pSetX:
			// NOTE: lsep1 and lsepx contain only one element
			// (see the comment above at Create a new list 'listChoice')
			CaSeparator* pS1 = (pSet1->GetListStrSeparator()).GetHead();
			CaSeparator* pS2 = (pSetX->GetListStrSeparator()).GetHead();
			if (pS1 && pS2 && pS1->GetSeparator().CompareNoCase(pS2->GetSeparator()) == 0)
			{
				if (pSet1->GetConsecutiveSeparatorsAsOne() == pSetX->GetConsecutiveSeparatorsAsOne() &&
				    pSet1->GetIgnoreTrailingSeparator() == pSetX->GetIgnoreTrailingSeparator() &&
				    pSet1->GetTextQualifier() == pSetX->GetTextQualifier())
				{
					listChoice.RemoveAt(pDel);
					delete pSetX;
				}
			}
		}

		//
		// Reposition the iterator index in case of the old one has been removed:
		pos = p;
		listChoice.GetNext(pos);
	}
}

