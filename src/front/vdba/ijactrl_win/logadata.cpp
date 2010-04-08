/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
** 
**    Source   : logadata.cpp , Implementation File 
**    Project  : Ingres Journal Analyser
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Log Analyser Data
**
** History:
**
** 17-Dec-1999 (uk$so01)
**    Created
** 23-Apr-2001 (uk$so01)
**    Bug #104487, 
**    When creating the temporary table, make sure to use the exact type based on its length.
**    If the type is integer and its length is 2 then use smallint or int2.
**    If the type is float and its length is 4 then use float(4).
** 13-Jun-2001 (noifr01)
**    (bug 104954) before dropping the temp tables, set the session 
**    authorization to the owner of the table, because the session 
**    authorization may have changed because of the "Impersonate 
**    user(s) of initial transaction(s)" option. ALso commit after dropping
**    the temp table(s) (now possible because now a commit or rollback
**    already has been done immediatly after the undo/recover execution, and
**    in no case we want the drop temp table to be rollbacked..)
** 07-Jun-2002 (uk$so01)
**    SIR #107951, Display date format according to the LOCALE.
** 05-Sep-2003 (uk$so01)
**    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
** 16-Nov-2004 (noifr01)
**    (bug 113465) updated display for create, alter and drop "table"
**    because of the ambiguity with views. Just leave "create", "alter", or 
**    "drop"
** 01-Dec-2004 (uk$so01)
**    IJA BUG #113551
**    Ija should internally specify the fixed date format expected by auditdb 
**    command independently of the LOCALE setting.
**/

#include "stdafx.h"
#include "logadata.h"
#include "ijadmlll.h"
#include "ijactrl.h"
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// This will work correctly if the date format is: 
// dd-mm-yyyy HH:MM:SS:xx or dd-MMM-yyyy HH:MM:SS:xx
// where MMM is an english language !
static int IJA_Month (LPCTSTR lpszDate)
{
	//
	// Actual date format is: 22-Dec-1999 10:20:10:20
	TCHAR tchszMonth [12][4] = 
	{
		_T("JAN"),
		_T("FEB"),
		_T("MAR"),
		_T("APR"),
		_T("MAY"),
		_T("JUN"),
		_T("JUL"),
		_T("AUG"),
		_T("SEP"),
		_T("OCT"),
		_T("NOV"),
		_T("DEC"),
	};

	CString s = lpszDate;
	int i1 = s.FindOneOf(_T("-/"));
	ASSERT (i1 != -1 && i1 > 0);
	if (i1 != -1 && i1 > 0)
	{
		s = s.Mid(i1+1);
		int i2 = s.FindOneOf(_T("-/"));
		ASSERT (i2 != -1 && i2 > 0);
		if (i2 != -1 && i2 > 0)
		{
			s = s.Left(i2);
			if (s.GetLength() == 3)
			{
				s.MakeUpper();
				for (int i = 0; i<12; i++)
				{
					if (s.CompareNoCase (tchszMonth[i]) == 0)
						return i+1;
				}
			}
		}
	}
	
	//
	// Default set to January !
	return 1;
}

static int IJA_Month2 (LPCTSTR lpszMonth)
{
	TCHAR tchszMonth [12][4] = 
	{
		_T("JAN"),
		_T("FEB"),
		_T("MAR"),
		_T("APR"),
		_T("MAY"),
		_T("JUN"),
		_T("JUL"),
		_T("AUG"),
		_T("SEP"),
		_T("OCT"),
		_T("NOV"),
		_T("DEC"),
	};

	CString s = lpszMonth;
	for (int i=0; i<12; i++)
	{
		if (s.CompareNoCase (tchszMonth[i]) == 0)
			return i+1;
	}
	//
	// Default set to January !
	ASSERT (FALSE);
	return 1;
}

int IJA_CompareDate (LPCTSTR lpszDate1, LPCTSTR lpszDate2, BOOL bFromIngresColumn)
{
	TCHAR tchsz1Day   [8];
	TCHAR tchsz1Month [8];
	TCHAR tchsz1Year  [8];
	TCHAR tchsz2Day   [8];
	TCHAR tchsz2Month [8];
	TCHAR tchsz2Year  [8];
	CString s;
	
	//
	// If bFromIngresColumn = TRUE, the date column is resulting from the select statement
	// though, the date is depent on the II_DATE_FORMAT:
	if (bFromIngresColumn)
	{
#if defined (SIMULATION)
		return _tcsicmp (lpszDate1, lpszDate2);
#else
		CString strD1;
		CString strD2;
		IJA_MakeDateStr (theApp.m_dateFormat, lpszDate1, strD1);
		IJA_MakeDateStr (theApp.m_dateFormat, lpszDate2, strD2);
		return strD1.Compare (strD2);
#endif
	}

	//
	// Actual date format is: 22-Dec-1999 10:20:10:20
	CString s1 = lpszDate1;
	CString s2 = lpszDate2;

	int idx = s1.Find (_T(' '));
	CString t1 = s1.Mid(idx+1);
	idx = s2.Find (_T(' '));
	CString t2 = s2.Mid(idx+1);


	tchsz1Day[0] = s1 [0];
	tchsz1Day[1] = s1 [1];
	tchsz1Day[2] = _T('\0');
	int nM1 = IJA_Month(lpszDate1);
	s.Format (_T("%02d"), nM1);
	tchsz1Month[0] = s [0];
	tchsz1Month[1] = s [1];
	tchsz1Month[2] = _T('\0');
	tchsz1Year [0] = s1 [7];
	tchsz1Year [1] = s1 [8];
	tchsz1Year [2] = s1 [9];
	tchsz1Year [3] = s1[10];
	tchsz1Year [4] = _T('\0');

	tchsz2Day[0] = s2[0];
	tchsz2Day[1] = s2[1];
	tchsz2Day[2] = _T('\0');
	int nM2 = IJA_Month(lpszDate2);
	s.Format (_T("%02d"), nM2);
	tchsz2Month[0] = s [0];
	tchsz2Month[1] = s [1];
	tchsz2Month[2] = _T('\0');
	tchsz2Year [0] = s2 [7];
	tchsz2Year [1] = s2 [8];
	tchsz2Year [2] = s2 [9];
	tchsz2Year [3] = s2[10];
	tchsz2Year [4] = _T('\0');

	s1 = CString (tchsz1Year) + CString(tchsz1Month) + CString(tchsz1Day) + t1;
	s2 = CString (tchsz2Year) + CString(tchsz2Month) + CString(tchsz2Day) + t2;

	return s1.Compare (s2);
}

CTime IJA_GetCTime(CString& strIjaDate)
{
	TCHAR tchsz[6][5];
	//
	// Actual format date in errlog.log is: dd-mmm-yyyy hh:nn:ss.ll 
	// ex: 06-Jun-2002 11:40:47.15
	
	//
	// In this conversion, we assume that the date format is always in the described above.
	// Otherwise this conversion will not work 
	int i;
	TCHAR tchszMonth  [12][4] = 
	{
		_T("Jan"),
		_T("Feb"),
		_T("Mar"),
		_T("Apr"),
		_T("May"),
		_T("Jun"),
		_T("Jul"),
		_T("Aug"),
		_T("Sep"),
		_T("Oct"),
		_T("Nov"),
		_T("Dec")
	};


	if (strIjaDate.GetLength() < 20)
	{
		TRACE0 ("IJA_GetCTime: strIjaDate has incompatible format or empty\n");
		throw (int)100;
	}

	// Year:
	tchsz[0][0] = strIjaDate.GetAt( 7);
	tchsz[0][1] = strIjaDate.GetAt( 8);
	tchsz[0][2] = strIjaDate.GetAt( 9);
	tchsz[0][3] = strIjaDate.GetAt(10);
	tchsz[0][4] = _T('\0');
	// Month:
	tchsz[1][0] = strIjaDate.GetAt(3);
	tchsz[1][1] = strIjaDate.GetAt(4);
	tchsz[1][2] = strIjaDate.GetAt(5);
	tchsz[1][3] = _T('\0');
	BOOL bMonthOK = FALSE;
	for (i=0; i<12; i++)
	{
		if (lstrcmpi (tchsz[1], tchszMonth[i]) == 0)
		{
			wsprintf (tchsz[1], _T("%02d"), i+1);
			bMonthOK = TRUE;
			break;
		}
	}
	if (!bMonthOK)
	{
		// Default to the current month:
		CTime t = CTime::GetCurrentTime();
		wsprintf (tchsz[1], _T("%02d"), t.GetMonth());
	}
	// Day:
	tchsz[2][0] = strIjaDate.GetAt(0);
	tchsz[2][1] = strIjaDate.GetAt(1);
	tchsz[2][2] = _T('\0');
	// Hour:
	tchsz[3][0] = strIjaDate.GetAt(12);
	tchsz[3][1] = strIjaDate.GetAt(13);
	tchsz[3][2] = _T('\0');
	// Minute:
	tchsz[4][0] = strIjaDate.GetAt(15);
	tchsz[4][1] = strIjaDate.GetAt(16);
	tchsz[4][2] = _T('\0');
	// Second:
	tchsz[5][0] = strIjaDate.GetAt(18);
	tchsz[5][1] = strIjaDate.GetAt(19);
	tchsz[5][2] = _T('\0');

	CTime t(_ttoi (tchsz[0]), _ttoi (tchsz[1]), _ttoi (tchsz[2]), _ttoi (tchsz[3]), _ttoi (tchsz[4]), _ttoi (tchsz[5]));
	return t;
}


BOOL isUndoRedoable(TxType txtyp)
{
	switch (txtyp) {
		case T_DELETE:
		case T_INSERT:
		case T_BEFOREUPDATE:
		case T_AFTERUPDATE:
			return TRUE;
		default:
			return FALSE;
	}
	return FALSE;
}

CaQueryTransactionInfo::CaQueryTransactionInfo()
{
	m_strNode = _T("");
	m_strDatabase = _T("");
	m_strDatabaseOwner = _T("");
	m_strTable = _T("");
	m_strTableOwner = _T("");
	m_strConnectedUser = _T("");

	m_strBefore= _T("");
	m_strAfter= _T("");
	m_strUser= _T("");
	
	m_bInconsistent = FALSE;
	m_bWait = FALSE;
	m_nCheckPointNumber = 0;
	m_bAfterCheckPoint = FALSE;
}

CaQueryTransactionInfo::CaQueryTransactionInfo(const CaQueryTransactionInfo& c)
{
	Copy (c);
}

void CaQueryTransactionInfo::Copy (const CaQueryTransactionInfo& c)
{
	m_strNode = c.m_strNode;
	m_strDatabase = c.m_strDatabase;
	m_strDatabaseOwner = c.m_strDatabaseOwner;
	m_strTable = c.m_strTable;
	m_strTableOwner = c.m_strTableOwner;
	m_strConnectedUser = c.m_strConnectedUser;

	m_strBefore= c.m_strBefore;
	m_strAfter= c.m_strAfter;
	m_strUser= c.m_strUser;
	
	m_bInconsistent = c.m_bInconsistent;
	m_bWait = c.m_bWait;
	m_nCheckPointNumber = c.m_nCheckPointNumber;
	m_bAfterCheckPoint = c.m_bAfterCheckPoint;
}

CaQueryTransactionInfo CaQueryTransactionInfo::operator = (const CaQueryTransactionInfo& c)
{
	Copy(c);
	return *this;
}

void CaQueryTransactionInfo::SetDatabase (LPCTSTR lpszDatabase, LPCTSTR lpszDatabaseOwner)
{
	m_strDatabase = lpszDatabase;
	m_strDatabaseOwner = lpszDatabaseOwner;
}

void CaQueryTransactionInfo::SetTable (LPCTSTR lpszTable, LPCTSTR lpszTableOwner)
{
	m_strTable = lpszTable;
	m_strTableOwner = lpszTableOwner;
}

void CaQueryTransactionInfo::GetDatabase (CString& strDatabase, CString& strDatabaseOwner)
{
	strDatabase = m_strDatabase;
	strDatabaseOwner = m_strTableOwner;
}

void CaQueryTransactionInfo::GetTable (CString& strTable, CString& strTableOwner)
{
	strTable = m_strTable;
	strTableOwner = m_strTableOwner;
}


//
// Base Transaction:
// *****************
CaBaseTransactionItemData::CaBaseTransactionItemData()
{
	m_strTable = _T("");
	m_strTableOwner = _T("");
	m_strData = _T("");
	m_nTxType = T_UNKNOWN;
#ifdef SIMULATION
	m_strTransactionID = _T("");
#endif
	m_bCommitted = TRUE;
	m_bJournaled = TRUE;
	m_bRollback2SavePoint = FALSE;

	m_bBeginEnd = FALSE;
	m_bBegin = FALSE;
	m_lLsnHigh = 0;
	m_lLsnLow = 0;
	m_lTranIdHigh = 0;
	m_lTranIdLow = 0;
	m_strLSNEnd = _T("???");

	m_bMsgTidModLaterDisplayed = FALSE;
	m_bMsgTblModLaterDisplayed = FALSE;

}

CaBaseTransactionItemData:: ~CaBaseTransactionItemData()
{
	m_strlistData.RemoveAll();
}


CString CaBaseTransactionItemData::GetStrTransactionID()
{
#if defined (SIMULATION)
	return GetTransactionID();
#else
	CString strID;
	strID.Format (_T("%08x%08x"), m_lTranIdHigh, m_lTranIdLow);
	return strID;
#endif
}


CString CaBaseTransactionItemData::GetLsn ()
{
#if defined (SIMULATION)
	return m_strLSN;
#else
	CString strID;
	strID.Format (_T("%08x%08x"), m_lLsnHigh, m_lLsnLow);
	return strID;
#endif
}



CString CaBaseTransactionItemData::GetStrAction()
{
	CString strTxt;
	switch (m_nTxType)
	{
	case T_DELETE:
		strTxt = _T("delete");
		break;
	case T_INSERT:
		strTxt = _T("insert");
		break;
	case T_BEFOREUPDATE:
		strTxt = _T("before update");
		break;
	case T_AFTERUPDATE:
		strTxt = _T("after update");
		break;
	case T_CREATETABLE:
		strTxt = _T("create");
		break;
	case T_ALTERTABLE:
		strTxt = _T("alter");
		break;
	case T_DROPTABLE:
		strTxt = _T("drop");
		break;
	case T_RELOCATE:
		strTxt = _T("relocate");
		break;
	case T_MODIFY:
		strTxt = _T("modify");
		break;
	case T_INDEX:
		strTxt = _T("index");
		break;

	default:
		strTxt = _T("????");
		break;
	}
	return strTxt;
}

CString CaBaseTransactionItemData::GetData()
{
#if defined (SIMULATION)
	return m_strData;
#endif

	BOOL bOne = TRUE;
	m_strData = _T("");
	POSITION pos = m_strlistData.GetHeadPosition();
	while (pos != NULL)
	{
		CString strItem = m_strlistData.GetNext (pos);
		if (!bOne)
			m_strData += _T(",");
		m_strData += strItem;
		bOne = FALSE;
	}
	return m_strData;
}

int CaBaseTransactionItemData::GetImageId()
{
	if (m_bBeginEnd)
	{
		if (m_bBegin)
		{
			if (m_bCommitted)
			{
				if (m_bRollback2SavePoint)
				{
					if (m_bJournaled)
						return IM_TX_BEGIN_COMMIT_JOURNAL_ROLLBACK_2_SAVEPOINT;
					else
						return IM_TX_BEGIN_COMMIT_JOURNAL_NO_ROLLBACK_2_SAVEPOINT;
				}
				else
					return m_bJournaled? IM_TX_BEGIN_COMMIT_JOURNAL: IM_TX_BEGIN_COMMIT_JOURNAL_NO;
			}
			else
			{
				if (m_bRollback2SavePoint)
				{
					if (m_bJournaled)
						return IM_TX_BEGIN_ROLLBACK_JOURNAL_ROLLBACK_2_SAVEPOINT;
					else
						return IM_TX_BEGIN_ROLLBACK_JOURNAL_NO_ROLLBACK_2_SAVEPOINT;
				}
				else
					return m_bJournaled? IM_TX_BEGIN_ROLLBACK_JOURNAL: IM_TX_BEGIN_ROLLBACK_JOURNAL_NO;
			}
		}
		else // end
		{
			if (m_bCommitted)
			{
				if (m_bRollback2SavePoint)
				{
					if (m_bJournaled)
						return IM_TX_END_COMMIT_JOURNAL_ROLLBACK_2_SAVEPOINT;
					else
						return IM_TX_END_COMMIT_JOURNAL_NO_ROLLBACK_2_SAVEPOINT;
				}
				else
					return m_bJournaled? IM_TX_END_COMMIT_JOURNAL: IM_TX_END_COMMIT_JOURNAL_NO;
			}
			else // end, rollbacked
			{
				if (m_bRollback2SavePoint)
				{
					if (m_bJournaled)
						return IM_TX_END_ROLLBACK_JOURNAL_ROLLBACK_2_SAVEPOINT;
					else
						return IM_TX_END_ROLLBACK_JOURNAL_NO_ROLLBACK_2_SAVEPOINT;
				}
				else
					return m_bJournaled? IM_TX_END_ROLLBACK_JOURNAL: IM_TX_END_ROLLBACK_JOURNAL_NO;
			}
		}
	}
	else
	{
		if (m_bCommitted)
		{
			if (m_bRollback2SavePoint)
			{
				if (m_bJournaled)
					return IM_COMMIT_JOURNAL_ROLLBACK_2_SAVEPOINT;
				else
					return IM_COMMIT_JOURNAL_NO_ROLLBACK_2_SAVEPOINT;
			}
			else
				return m_bJournaled? IM_COMMIT_JOURNAL: IM_COMMIT_JOURNAL_NO;
		}
		else
		{
			if (m_bRollback2SavePoint)
			{
				if (m_bJournaled)
					return IM_ROLLBACK_JOURNAL_ROLLBACK_2_SAVEPOINT;
				else
					return IM_ROLLBACK_JOURNAL_NO_ROLLBACK_2_SAVEPOINT;
			}
			else
				return m_bJournaled? IM_ROLLBACK_JOURNAL: IM_ROLLBACK_JOURNAL_NO;
		}
	}
}



//
// Database Transaction:
// *********************


CaDatabaseTransactionItemData::CaDatabaseTransactionItemData()
{
	m_strUser = _T("");
	m_strBegin = _T("");
	m_strEnd  = _T("");
	m_strDuration = _T("");
	m_strTransactionID = _T("");

	m_lCountInsert = 0;
	m_lCountUpdate = 0;
	m_lCountDelete = 0;
	m_lCountTotal = 0;
	m_bCommitted = TRUE;
	m_bJournaled = TRUE;
	m_bRollback2SavePoint = FALSE;

	m_strLSNBegin = _T("???");
	m_strLSNEnd = _T("???");

	m_beginTransaction.SetBegin();
	m_endTransaction.SetEnd();
}

CaDatabaseTransactionItemData::~CaDatabaseTransactionItemData()
{
	while (!m_listDetailTransaction.IsEmpty())
		delete m_listDetailTransaction.RemoveHead();
}
	
CString CaDatabaseTransactionItemData::GetBeginLocale()
{
	try
	{
		CTime t = IJA_GetCTime(m_strBegin);
		CString strLocaleDate = t.Format (_T("%x "));
		if (m_strBegin.GetLength() >= 12)
		{
			strLocaleDate += m_strBegin.Mid(11);
		}
		return strLocaleDate;
	}
	catch(...)
	{
		return m_strBegin;
	}
}

CString CaDatabaseTransactionItemData::GetEndLocale()
{
	try
	{
		CTime t = IJA_GetCTime(m_strEnd);
		CString strLocaleDate = t.Format (_T("%x "));
		if (m_strEnd.GetLength() >= 12)
		{
			strLocaleDate += m_strEnd.Mid(11);
		}
		return strLocaleDate;
	}
	catch(...)
	{

	}
	return m_strEnd;
}

CString CaDatabaseTransactionItemData::GetStrTransactionID()
{
	return GetTransactionID();
}


CString CaDatabaseTransactionItemData::GetStrCountInsert()
{
	CString strTxt;
	strTxt.Format (_T("%d"), m_lCountInsert);
	return strTxt;
}

CString CaDatabaseTransactionItemData::GetStrCountUpdate()
{
	CString strTxt;
	strTxt.Format (_T("%d"), m_lCountUpdate);
	return strTxt;
}

CString CaDatabaseTransactionItemData::GetStrCountDelete()
{
	CString strTxt;
	strTxt.Format (_T("%d"), m_lCountDelete);
	return strTxt;
}

CString CaDatabaseTransactionItemData::GetStrCountTotal()
{
	CString strTxt;
	strTxt.Format (_T("%d"), m_lCountTotal);
	return strTxt;
}


int CaDatabaseTransactionItemData::GetImageId()
{
	if (m_bCommitted)
	{
		if (m_bRollback2SavePoint)
		{
			if (m_bJournaled)
				return IM_COMMIT_JOURNAL_ROLLBACK_2_SAVEPOINT;
			else
				return IM_COMMIT_JOURNAL_NO_ROLLBACK_2_SAVEPOINT;
		}
		else
			return m_bJournaled? IM_COMMIT_JOURNAL: IM_COMMIT_JOURNAL_NO;
	}
	else
	{
		if (m_bRollback2SavePoint)
		{
			if (m_bJournaled)
				return IM_ROLLBACK_JOURNAL_ROLLBACK_2_SAVEPOINT;
			else
				return IM_ROLLBACK_JOURNAL_NO_ROLLBACK_2_SAVEPOINT;
		}
		else
			return m_bJournaled? IM_ROLLBACK_JOURNAL: IM_ROLLBACK_JOURNAL_NO;
	}
}

int CaDatabaseTransactionItemData::Compare (CaDatabaseTransactionItemData* pItem, int nColumn)
{
	switch (nColumn)
	{
	case 0: // Transaction ID
		return GetStrTransactionID().CompareNoCase (pItem->GetStrTransactionID());
		break;
	case 1: // User
		return GetUser().CompareNoCase (pItem->GetUser());
		break;
	case 2: // Begin (Date, use the lsn begin)
		return GetLsnBegin().CompareNoCase(pItem->GetLsnBegin());
		break;
	case 3: // End (Date, use the lsn end)
		return GetLsnEnd().CompareNoCase(pItem->GetLsnEnd());
		break;
	case 4: // Duration
#if defined SIMULATION
		return GetDuration().CompareNoCase (pItem->GetDuration());
#else
		return GetEllapsed().Compare(pItem->GetEllapsed());
#endif
		break;
	case 5: // Insert
		return (GetCountInsert() - pItem->GetCountInsert());
		break;
	case 6: // Update
		return (GetCountUpdate() - pItem->GetCountUpdate());
		break;
	case 7: // Delete
		return (GetCountDelete() - pItem->GetCountDelete());
		break;
	case 8: // Total
		return (GetCountTotal() - pItem->GetCountTotal());
		break;
	default:
		return 0;
	}
	return 0;
}

CString CaDatabaseTransactionItemData::GetDuration ()
{
#if defined (SIMULATION)
	return m_strDuration;
#else
	CString csTemp;
	m_EllapsedTime.GetDisplayString(csTemp);
	return csTemp;
#endif
}


//
// Database Detail Transaction:
// ****************************
int CaDatabaseTransactionItemDataDetail::Compare (CaDatabaseTransactionItemDataDetail* pItem, int nColumn)
{
	switch (nColumn)
	{
	case 0: // Transaction ID
		return GetStrTransactionID().CompareNoCase (pItem->GetStrTransactionID());
		break;
	case 1: // LSN
		return GetLsn().CompareNoCase (pItem->GetLsn());
		break;
	case 2: // User
		return GetUser().CompareNoCase (pItem->GetUser());
		break;
	case 3: // Action
		return GetStrAction().CompareNoCase (pItem->GetStrAction());
		break;
	case 4: // Table
		return GetTable().CompareNoCase (pItem->GetTable());
		break;
	case 5: // Data
		return GetData().CompareNoCase (pItem->GetData());
		break;
	default:
		return 0;
	}
	return 0;
}

CString CaDatabaseTransactionItemDataDetail::GetStrAction()
{
	if (!IsBeginEnd())
		return CaBaseTransactionItemData::GetStrAction();

	CString strAct;
	if (m_bBegin)
	{
		try
		{
			CTime t = IJA_GetCTime(m_strTxTimeBegin);
			strAct = t.Format (_T("begin %x "));
			if (m_strTxTimeBegin.GetLength() >= 12)
			{
				strAct += m_strTxTimeBegin.Mid(11);
			}
			return strAct;
		}
		catch (...)
		{

		}
		strAct.Format (_T("begin %s"), (LPCTSTR)m_strTxTimeBegin);
		return strAct;
	}
	else
	{
		try
		{
			CTime t = IJA_GetCTime(m_strTxTimeEnd);
			strAct = t.Format (_T("end %x "));
			if (m_strTxTimeEnd.GetLength() >= 12)
			{
				strAct += m_strTxTimeEnd.Mid(11);
			}
			return strAct;
		}
		catch (...)
		{

		}
		strAct.Format (_T("end %s"), (LPCTSTR)m_strTxTimeEnd);
		return strAct;
	}
}


//
// Table Transaction:
// *********************

CaTableTransactionItemData::CaTableTransactionItemData():CaBaseTransactionItemData()
{
	m_strDate = _T("");
	m_strName = _T("");
}

static CString GetNthItem (CStringList& listStr, int nItem)
{
	CString strItem;
	int nStart = 0;
	POSITION pos = listStr.GetHeadPosition();
	while (pos != NULL)
	{
		strItem = listStr.GetNext (pos);
		nStart++;
		if (nStart == nItem)
			break;
	}
	return strItem;
}

int CaTableTransactionItemData::Compare (CaTableTransactionItemData* pItem, int nColumn)
{
	switch (nColumn)
	{
	case 0: // Transaction ID
		return GetStrTransactionID().CompareNoCase (pItem->GetStrTransactionID());
		break;
	case 1: // LSN
		return GetLsn().CompareNoCase (pItem->GetLsn());
		break;
	case 2: // Commit Date
		return IJA_CompareDate (GetDate(), pItem->GetDate(), TRUE);
		break;
	case 3: // User
		return GetUser().CompareNoCase (pItem->GetUser());
		break;
	case 4: // Operation
		return GetStrAction().CompareNoCase (pItem->GetStrAction());
		break;
	default:
		{
#if defined (SIMULATION)
			//
			// These are real columns of the audited table:
			int nHeader = 5; // case 0, case 1, ... case 4
			CStringList& thisListStr = GetListData();
			CStringList& otherListStr = pItem->GetListData();
			ASSERT (nColumn >= nHeader);
			if (nColumn < nHeader)
				return 0;
			CString str1 = GetNthItem(thisListStr,  nColumn - (nHeader - 1));
			CString str2 = GetNthItem(otherListStr, nColumn - (nHeader - 1));
			return str1.CompareNoCase (str2);
#else
			//
			// TODO: Manage the sort on the correct data type (integer, string, date, ...)
			ASSERT (FALSE);
#endif
		}
		return 0;
	}

}

#define _DATE_of_TABLE_FROM_DBMS
CString CaTableTransactionItemData::GetDateLocale()
{
	try
	{
#if defined (_DATE_of_TABLE_FROM_DBMS)
		return GetDate();
#else
		CTime t = IJA_GetCTime(m_strDate);
		CString strLocaleDate = t.Format (_T("%x "));
		if (m_strDate.GetLength() >= 12)
		{
			strLocaleDate += m_strDate.Mid(11);
		}
		return strLocaleDate;
#endif
	}
	catch(...)
	{
		return m_strDate;
	}
}


//
// CaRecoverRedoInformation:
CaRecoverRedoInformation::~CaRecoverRedoInformation()
{
	while (!m_listTable.IsEmpty())
		delete m_listTable.RemoveHead();
	POSITION pos = m_listTempFile.GetHeadPosition();
	while (pos != NULL)
	{
		CString strFile = m_listTempFile.GetNext (pos);
		//
		// Check if the file has been successfuly created:
		if (!strFile.IsEmpty() && (_taccess(strFile, 0) != -1))
			DeleteFile (strFile);
	}
	m_listTempFile.RemoveAll();
	DestroyTemporaryTable();
}

void CaRecoverRedoInformation::DestroyTemporaryTable()
{
	// Remove the temporaty table objects.
	// Note: If we need to destroy the temporary table from the database 
	//       at this point we are not ensured that the session is always available
	//       so, we need a new session (if everything is ok, then the session is commited,
	//       and the temporary tables will appear in the new session)
	BOOL bLocal = m_pSession? m_pSession->IsLocalNode(): FALSE;
	BOOL bHasSession = (m_pSession && m_pSession->IsConnected())? TRUE: FALSE;
	CString strStatement;
	CaLowlevelAddAlterDrop param (_T(""), _T(""), _T(""));
	param.NeedSession(FALSE);

	POSITION pos = m_listTempTable.GetHeadPosition();
	while (pos != NULL)
	{
		CaDBObject* pObj = m_listTempTable.GetNext(pos);
		//
		// Delete Table from database:
		if (!bLocal)
		{
			CString strTable = pObj->GetItem();
			CString strTableOwner = pObj->GetOwner();
			if (bHasSession)
			{
				strStatement.Format (_T("set session authorization %s"),(LPCTSTR)strTableOwner);
				param.SetStatement(strStatement);
				param.ExecuteStatement(NULL);
				strStatement.Format (_T("drop table %s.%s"), (LPCTSTR)strTableOwner, (LPCTSTR)strTable);
				param.SetStatement(strStatement);
				param.ExecuteStatement(NULL);
				/* commit must be done here so that the temp table is dropped anyhow */
				/* the undo/redo statements already have been rollbacked if needed   */
				strStatement.Format (_T("commit"), (LPCTSTR)strTableOwner, (LPCTSTR)strTable);
				param.SetStatement(strStatement);
				param.ExecuteStatement(NULL);
			}
		}
		delete pObj;
	}
	if (!m_listTempTable.IsEmpty())
		m_listTempTable.RemoveAll();

	TRACE0 ("CaRecoverRedoInformation::DestroyTemporaryTable() \n");
}



//
// CLASS CaEllapsedTime:
CaEllapsedTime::CaEllapsedTime(LPCTSTR lpSinceString, LPCTSTR lpAfterString)
{
	m_bError = FALSE;
	Set(lpSinceString, lpAfterString);
}

void CaEllapsedTime::Set(LPCTSTR lpSinceString, LPCTSTR lpAfterString)
{

	if (!ReadStringAndFillVariable(lpSinceString, TRUE))
		m_bError = TRUE;
	if (!ReadStringAndFillVariable(lpAfterString, FALSE))
		m_bError = TRUE;

	if (m_bError) {
		CTimeSpan ctstemp(0,0,0,0);
		m_TimeSpanWithNoCs = ctstemp;
		m_icsEnd = m_icsStart = 0;
	}
	else
		m_TimeSpanWithNoCs = m_ct2 - m_ct1;

	int ics = m_icsEnd - m_icsStart;
	if (ics >= 0 ) 
		m_ics = ics;
	else {
		CTimeSpan cts2(0,0,0,1);
		m_TimeSpanWithNoCs -= cts2;
		m_ics= 100+ics;
	}
}

BOOL CaEllapsedTime::ReadStringAndFillVariable(LPCTSTR lpCurString, BOOL bStart)
{
	int nDay , nMonth , nYear , nHour, nMin, nSec, nHundredSec , nDST = -1;
	if (_tcslen(lpCurString)<23)
		return FALSE;
	if (lpCurString[2] != _T('-') || lpCurString[6] != _T('-') ||
		lpCurString[11] != _T(' ') ||lpCurString[14] != _T(':')||
		lpCurString[17] != _T(':'))
	{
		nDay = nMonth = nYear = nHour = nMin = nSec = 0;
		return FALSE;
	}

	// Day (1..31)
	nDay = (lpCurString[0] - _T('0')) * 10 + (lpCurString[1] - _T('0'));
	if (nDay < 1 || nDay > 31)
	{
		nDay = nMonth = nYear = nHour = nMin = nSec = 0;
		return FALSE;
	}

	// Month (1..12)
	nMonth = IJA_Month((LPCTSTR)lpCurString);

	// Year (1970...2038)
	nYear  =(lpCurString[7] - _T('0')) * 1000 +
			 (lpCurString[8] - _T('0')) * 100  +
			 (lpCurString[9] - _T('0')) * 10   +
			 (lpCurString[10] - _T('0'));
	if (nYear < 1970 || nYear > 2038)
	{
		nDay = nMonth = nYear = nHour = nMin = nSec = 0;
		return FALSE;
	}

	// Hour (0..23) 
	nHour = (lpCurString[12] - _T('0')) * 10 + (lpCurString[13] - _T('0'));
	if (nHour < 0 || nHour > 23)
	{
		nDay = nMonth = nYear = nHour = nMin = nSec = 0;
		return FALSE;
	}

	// Minute (0..59) 
	nMin = (lpCurString[15] - _T('0')) * 10 + (lpCurString[16] - _T('0'));
	if (nMin < 0 || nMin > 59)
	{
		nDay = nMonth = nYear = nHour = nMin = nSec = 0;
		return FALSE;
	}

	// Second (0...59)
	nSec = (lpCurString[18] - _T('0')) * 10 + (lpCurString[19] - _T('0'));
	if (nSec < 0 || nSec > 59)
	{
		nDay = nMonth = nYear = nHour = nMin = nSec = 0;
		return FALSE;
	}
	
	// Hundredth second (0...99)
	nHundredSec = (lpCurString[21] - _T('0')) * 10 + (lpCurString[22] - _T('0'));
	if (nHundredSec < 0 || nHundredSec > 99)
	{
		nDay = nMonth = nYear = nHour = nMin = nSec = nHundredSec = 0;
		return FALSE;
	}

	if (bStart)
	{
		CTime ct1( nYear,nMonth,nDay,nHour,nMin,nSec, nDST);
		m_ct1 = ct1;
		m_icsStart = nHundredSec;
	}
	else
	{
		CTime ct2( nYear ,nMonth ,nDay ,nHour ,nMin ,nSec , nDST);
		m_ct2 = ct2;
		m_icsEnd = nHundredSec;
	}

	return TRUE;
}


void CaEllapsedTime::GetDisplayString( CString& csTempDisplay)
{
	CString csNoCs;
	if (m_bError) {
		csTempDisplay = _T("N/A");
		return;
	}
	if (m_TimeSpanWithNoCs.GetDays() > 0)
		csNoCs = m_TimeSpanWithNoCs.Format(_T("%Dd%H:%M:%S"));
	else if (m_TimeSpanWithNoCs.GetTotalHours() > 0)
		csNoCs = m_TimeSpanWithNoCs.Format(_T("%H:%M:%S"));
	else if (m_TimeSpanWithNoCs.GetTotalMinutes() > 0)
		csNoCs = m_TimeSpanWithNoCs.Format(_T("%M:%S"));
	else if (m_TimeSpanWithNoCs.GetTotalSeconds() > 0)
		csNoCs = m_TimeSpanWithNoCs.Format(_T("%S"));
	else 
		csNoCs = _T("0");

	CString cscs;
	cscs.Format(_T(".%02d"),m_ics);
	csTempDisplay = csNoCs + cscs;

}

int  CaEllapsedTime::Compare ( CaEllapsedTime& Ellapsed)
{
	if (m_bError) {
		if (Ellapsed.m_bError)
			return 0;
		return -1;
	}
	if (Ellapsed.m_bError) 
		return 1; // m_bError from the source one already handled

	if (m_TimeSpanWithNoCs == Ellapsed.m_TimeSpanWithNoCs)
	{
		if (m_ics == Ellapsed.m_ics)
			return 0;
		if (m_ics < Ellapsed.m_ics)
			return -1;
		return 1;
	}
	if (m_TimeSpanWithNoCs < Ellapsed.m_TimeSpanWithNoCs)
		return -1;
	return 1;
}

//
// dateFormat: the enum value of II_DATE_FORMAT
// lpszDate  : the current output date due to the II_DATE_FORMAT
// strConvertDate: converted date to the unique format yyyy-mm-dd [HH:MM:SS]
void IJA_MakeDateStr (II_DATEFORMAT dateFormat, LPCTSTR lpszDate, CString& strConvertDate)
{
	strConvertDate = lpszDate;
	switch (dateFormat)
	{
	case II_DATE_MULTINATIONAL:  // Output: dd/mm/yy:
	case II_DATE_MULTINATIONAL4: // Output: dd/mm/yyyy:
		// In order to compare correctly, we must add the century number:
		if (theApp.m_dateCenturyBoundary == 0 && dateFormat == II_DATE_MULTINATIONAL)
			break;
		if ((_tcslen (lpszDate) >= 8 && dateFormat == II_DATE_MULTINATIONAL) || 
		    (_tcslen (lpszDate) >=10 && dateFormat == II_DATE_MULTINATIONAL4))
		{
			TCHAR tchszPrevCentury[] = {_T('1'), _T('9'), 0x00};
			TCHAR tchszCurrCentury[] = {_T('2'), _T('0'), 0x00};
			TCHAR tchszNextCentury[] = {_T('2'), _T('1'), 0x00};
			CTime t = CTime::GetCurrentTime();
			int nCurrntYear = t.GetYear();
			CString strInputYear;

			if (dateFormat == II_DATE_MULTINATIONAL)
			{
				strInputYear.Format (_T("%c%c"), lpszDate[6], lpszDate[7]);
				if (nCurrntYear < theApp.m_dateCenturyBoundary)
				{
					if (_ttoi (strInputYear) < theApp.m_dateCenturyBoundary)
						strInputYear.Format (_T("%s%c%c"), tchszCurrCentury, lpszDate[6], lpszDate[7]);
					else
						strInputYear.Format (_T("%s%c%c"), tchszPrevCentury, lpszDate[6], lpszDate[7]);
				}
				else
				{
					if (_ttoi (strInputYear) < theApp.m_dateCenturyBoundary)
						strInputYear.Format (_T("%s%c%c"), tchszNextCentury, lpszDate[6], lpszDate[7]);
					else
						strInputYear.Format (_T("%s%c%c"), tchszCurrCentury, lpszDate[6], lpszDate[7]);
				}
			}
			else
			{
				strInputYear.Format (_T("%c%c%c%c"), lpszDate[6], lpszDate[7],lpszDate[8], lpszDate[9]);
			}
			
			strConvertDate.Format( 
				_T("%s-%c%c%-%c%c"),
				(LPCTSTR)strInputYear,
				lpszDate[3], lpszDate[4],
				lpszDate[0], lpszDate[1]);
			
			CString strTail = (dateFormat == II_DATE_MULTINATIONAL4)?  &lpszDate[10] : &lpszDate[8];
			strConvertDate += strTail;
		}
		break;

	case II_DATE_ISO:
		// Output: yymmdd:
		// In order to compare correctly, we must add the century number:
		if (theApp.m_dateCenturyBoundary == 0)
			break;
		if (_tcslen (lpszDate) >= 6)
		{
			TCHAR tchszPrevCentury[] = {_T('1'), _T('9'), 0x00};
			TCHAR tchszCurrCentury[] = {_T('2'), _T('0'), 0x00};
			TCHAR tchszNextCentury[] = {_T('2'), _T('1'), 0x00};
			CTime t = CTime::GetCurrentTime();
			int nCurrntYear = t.GetYear();
			CString strInputYear;
			strInputYear.Format (_T("%c%c"), lpszDate[0], lpszDate[1]);

			if (nCurrntYear < theApp.m_dateCenturyBoundary)
			{
				if (_ttoi (strInputYear) < theApp.m_dateCenturyBoundary)
					strInputYear.Format (_T("%s%c%c"), tchszCurrCentury, lpszDate[0], lpszDate[1]);
				else
					strInputYear.Format (_T("%s%c%c"), tchszPrevCentury, lpszDate[0], lpszDate[1]);
			}
			else
			{
				if (_ttoi (strInputYear) < theApp.m_dateCenturyBoundary)
					strInputYear.Format (_T("%s%c%c"), tchszNextCentury, lpszDate[0], lpszDate[1]);
				else
					strInputYear.Format (_T("%s%c%c"), tchszCurrCentury, lpszDate[0], lpszDate[1]);
			}
			strConvertDate.Format (
				_T("%s-%c%c%-%c%c"),
				(LPCTSTR)strInputYear,
				lpszDate[2], lpszDate[3],
				lpszDate[4], lpszDate[5]);
			CString strTail = &lpszDate[6];
			strConvertDate += strTail;
		}
		break;

	case II_DATE_SWEDEN:
	case II_DATE_FINLAND:
		// Output: yyyy-mm-dd
		// Nothing to do !
		break;

	case II_DATE_GERMAN:
		// Output: dd.mm.yyyy
		if (_tcslen (lpszDate) >= 10)
		{
			strConvertDate.Format 
				(_T("%c%c%c%c-%c%c%-%c%c"), 
				lpszDate[6], lpszDate[7], lpszDate[8], lpszDate[9],
				lpszDate[3], lpszDate[4],
				lpszDate[0], lpszDate[1]);
			CString strTail = &lpszDate[10];
			strConvertDate += strTail;
		}
		break;
	case II_DATE_YMD:
		// Output: yyyy-mmm-dd
		if (_tcslen (lpszDate) >= 11)
		{
			CString strMonth;
			strMonth.Format (_T("%c%c%c"), lpszDate[5], lpszDate[6], lpszDate[7]);
			int nMonth = IJA_Month2(strMonth);

			strConvertDate.Format 
				(_T("%c%c%c%c-%02d%-%c%c"), 
				lpszDate[0], lpszDate[1], lpszDate[2], lpszDate[3],
				nMonth,
				lpszDate[9], lpszDate[10]);
			CString strTail = &lpszDate[11];
			strConvertDate += strTail;
		}
		break;
	case II_DATE_US:
	case II_DATE_DMY:
		// Output: dd-mmm-yyyy
		if (_tcslen (lpszDate) >= 11)
		{
			CString strMonth;
			strMonth.Format (_T("%c%c%c"), lpszDate[3], lpszDate[4], lpszDate[5]);
			int nMonth = IJA_Month2(strMonth);

			strConvertDate.Format 
				(_T("%c%c%c%c-%02d%-%c%c"), 
				lpszDate[7], lpszDate[8], lpszDate[9], lpszDate[10],
				nMonth,
				lpszDate[0], lpszDate[1]);
			CString strTail = &lpszDate[11];
			strConvertDate += strTail;
		}
		break;
	case II_DATE_MDY:
		// Output: mmm-dd-yyyy
		if (_tcslen (lpszDate) >= 11)
		{
			CString strMonth;
			strMonth.Format (_T("%c%c%c"), lpszDate[0], lpszDate[1], lpszDate[2]);
			int nMonth = IJA_Month2(strMonth);

			strConvertDate.Format 
				(_T("%c%c%c%c-%02d%-%c%c"), 
				lpszDate[7], lpszDate[8], lpszDate[9], lpszDate[10],
				nMonth,
				lpszDate[4], lpszDate[5]);
			CString strTail = &lpszDate[11];
			strConvertDate += strTail;
		}
		break;
	default:
		break;
	}
}


static CString IJA_AuditdbMonth (int m)
{
	//
	// Actual date format is: 22-Dec-1999 10:20:10:20
	TCHAR tchszMonth [12][4] = 
	{
		_T("jan"),
		_T("feb"),
		_T("mar"),
		_T("apr"),
		_T("may"),
		_T("jun"),
		_T("jul"),
		_T("aug"),
		_T("sep"),
		_T("oct"),
		_T("nov"),
		_T("dec"),
	};

	if ((m < 1) || (m > 12))
		return tchszMonth[0];
	return tchszMonth[m-1];
}

CString toAuditdbInputDate(CTime& t)
{
	CString s = t.Format (_T("%d-mmm-%Y:%H:%M:%S"));
	int m = t.GetMonth();
	CString strMonth = IJA_AuditdbMonth(m);
	s.Replace(_T("mmm"), strMonth);
	return s;
}

