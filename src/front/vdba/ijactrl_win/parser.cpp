/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : parser.cpp , Implementation File
**    Project  : Ingres Journal Analyser
**    Purpose  : misc classes or functions for the auditdb parsing
**
** History:
**
** 04-Jan-2000 (uk$so01)
**    Created (empty file)
** 15-Mar-2000 (noifr01)
**    created the ScanOutputLines class
** 14-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management and put string into the resource files
** 17-May-2001 (noifr01)
**    (SIR 101169) fixed an error in a FormatString() related to the
**    move of strings into resources
** 05-Sep-2003 (uk$so01)
**    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
** 16-Nov-2004 (noifr01)
**    (bug 113465) parsing failed if there is a journaled create view
**    statement, because in this case there is no "location" line after
**    the "create" one
** 16-Feb-2004 (lakvi01)
**	  (Bug 113868) Made IJA catch bad dates errors thrown by 'auditdb'.
** 21-Feb-2004 (lakvi01)
**	  (Bug 113927) Made IJA catch error if the user tries to audit a database
**	  that is not journaled. It should not give the option of continuing or not
**	  to the user. 
** 09-Jul-2009 (drivi01)
**	  Update the _ttoul function to expect both hex and decimal values
**        for LSN conversions.  auditdb was updated to print hex LSN for
**	  transactions, but the older version of Ingres will still print
**        decimal values, IJA needs to be prepared for both.
** 
**/


#include "stdafx.h"
#include "parser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static TCHAR TRACELINESEP = _T('\n');

static LPTSTR szBeginTransact         = _T("Begin   : Transaction Id ");
static LPTSTR szBeginTransactUsername = _T("Username ");
static LPTSTR szEndTransact           = _T("End     : Transaction Id ");
static LPTSTR szEndCommit             = _T("        Operation commit");
static LPTSTR szEndAbort              = _T("        Operation abort");
static LPTSTR szEndCommitNJ           = _T("        Operation commitnj");
static LPTSTR szEndAbortNJ            = _T("        Operation abortnj");

static LPTSTR szAbortSave             = _T("  AbortSave  : Transaction Id ");
static LPTSTR szAbort2SavePoint       = _T("  SAVEPOINT LSN=(");

static LPTSTR szCreateTableRec        = _T("  Create  : Transaction Id ");
static LPTSTR szDropTableRec          = _T("  Drop/Destroy : Transaction Id ");
static LPTSTR szCreDropTableLoc       = _T("\tLocation ");

static LPTSTR szAlterTableRec         = _T("  Alter   : Transaction Id ");

static LPTSTR szInsert_Append         =_T("  Insert/Append  : Transaction Id ");
static LPTSTR szInsDel_Record         =_T("    Record: ");
static LPTSTR szLSN_equ               =_T("LSN=(");

static LPTSTR szTid                =_T(" TID");
static LPTSTR szOldTid                =_T(" OLD TID");
static LPTSTR szNewTid                =_T(", NEW TID");

static LPTSTR szUpdateRow             =_T("  Update/Replace : Transaction Id ");
//static LPTSTR szID                    =_T(" Id (");
static LPTSTR szTable                 =_T("Table [");
static LPTSTR szUpdateOldRow          =_T("    Old: ");
static LPTSTR szUpdateNewRow          =_T("    New: ");

static LPTSTR szdeleteRow             =_T("  Delete : Transaction Id ");

static LPTSTR szRelocate              =_T("  Relocate: Transaction Id ");
static LPTSTR szRelocOldLoc           =_T("\tOld Location ");
static LPTSTR szRelocNewLoc           =_T("\tNew Location ");

static LPTSTR szModify                =_T("  Modify  : Transaction Id ");

static LPTSTR szIndex                 =_T("  Index   : Transaction Id ");

// understood "start of lines" for detecting errors
LPTSTR ParsedStrings[]= {
	szBeginTransact,
	szEndTransact,
	szEndCommit,
	szEndAbort,
	szEndCommitNJ,
	szEndAbortNJ,
	szAbortSave,
	szAbort2SavePoint,
	szCreateTableRec,
	szDropTableRec,
	szCreDropTableLoc,
	szAlterTableRec,
	szInsert_Append,
	szInsDel_Record,
	szLSN_equ,
//	szTid,
//	szOldTid,
//	szNewTid,
	szUpdateRow,
//	szID,
//	szTable,
	szUpdateOldRow,
	szUpdateNewRow,
	szdeleteRow,
	szRelocate,
	szRelocOldLoc,
	szRelocNewLoc,
	szModify,
	szIndex,
	NULL
};

LPTSTR suppspace(LPTSTR lpchar)
{
	int il;
	while ((il = strlen(lpchar))>0){
		if (*(lpchar+il-1)==' ' || *(lpchar+il-1)=='\t')
			*(lpchar+il-1)='\0';
		else
			break;
	}
	return lpchar;
}

static TCHAR bufb1[1000];
static LPTSTR GetStringBetween2Pointers(LPTSTR p1, LPTSTR p2)
{
	int ilen = (p2-p1);
	if (ilen+ 1> sizeof(bufb1))
		return NULL; // should force an exception to be captured by the caller
	_tcsncpy(bufb1,p1,ilen+1);
	bufb1[ilen]=_T('\0');
	return bufb1;
}

BOOL Get2LongsFromHexString(LPCTSTR lpstring, unsigned long &l1, unsigned long &l2)
{
	if (_tcslen(lpstring)!=16)
		return FALSE;
	unsigned long li1,li2;
	_stscanf(lpstring,_T("%8x%8x"),&li1,&li2);
	l1=li1;
	l2=li2;
	return TRUE;
}

unsigned long _ttoul( LPCTSTR lpstring)
{
	char * p;
	unsigned long ul;
	if (_tcsstr(lpstring, _T("0x")) == NULL)
		ul = _tcstoul(lpstring, &p, 10);
	else
		ul = _tcstoul(lpstring, &p,16);
	return ul;
	
}


ScanOutputLines::ScanOutputLines(CString csString,SkipLinesType SkipLinesT)
{
	m_csFullString = csString; 
	m_SkipLinesType = SkipLinesT;
	m_stringstart = m_csFullString. GetBuffer(0);
	m_istrlen = _tcslen(m_stringstart);
	for (int icur =0;icur<m_istrlen;icur+=_tcslen(m_stringstart+icur)+1) {
		LPTSTR p =m_stringstart+icur;
		LPTSTR pc = _tcschr(p,TRACELINESEP);
		if (pc) {
			*pc=_T('\0');
			while (_tcslen(p) >0 ) {
				TCHAR c = p[_tcslen(p)-1];
				if (c==_T('\n') || c==_T('\r') || c==_T(' '))
					p[_tcslen(p)-1] =_T('\0');
				else
					break;
			}
		}
	}
}

LPTSTR ScanOutputLines::GetFirstTraceLine(int i)
{

	ASSERT (i>=0 && i<MAXCONCURRENTSCANS);
	m_icurpos[i]=0;
	return GetNextTraceLine(i);
}

LPTSTR ScanOutputLines::GetFirstTraceLineAfter(int i, int iini)
{
	ASSERT (i>=0    &&    i<MAXCONCURRENTSCANS);
	ASSERT (iini>=0 && iini<MAXCONCURRENTSCANS);
	m_icurpos[i]=m_icurpos[iini];
	return GetNextTraceLine(i);
}

static LPTSTR szbeginpage = _T("\fAudit for database");
LPTSTR ScanOutputLines::GetNextTraceLine(int i)
{
	ASSERT (i>=0 && i<MAXCONCURRENTSCANS);
	while ( TRUE) {
		int iresult = m_icurpos[i];
		if (iresult>=m_istrlen)
			return NULL;
		m_icurpos[i]+=_tcslen(m_stringstart+iresult)+1;
		if (m_stringstart[iresult]!=_T('\0')) {
			switch (m_SkipLinesType) {
				case SKIP_4_AUDITDBPARSE:
					if (!_tcsncmp(m_stringstart+iresult,szbeginpage,_tcslen(szbeginpage)))
						break;
					return (m_stringstart+iresult);
				case SKIP_NONE:
				default:
					return (m_stringstart+iresult);
			}
		}
	}
	ASSERT (FALSE);
	return NULL;
}


BOOL IJA_DatabaseParse4Transactions (
    CString& strAuditdbOutput,
    CTypedPtrList < CObList, CaDatabaseTransactionItemData* >& listTransaction)
{
	unsigned long l1;
	unsigned long l2;
	LPTSTR p;
	LPTSTR p1;
	CString csBuf;
	CString csTransUser;

	ScanOutputLines OutputLines(strAuditdbOutput,SKIP_4_AUDITDBPARSE);
	//strAuditdbOutput=_T(""); /* no need to use more than twice the memory size, and memory now needed for filling the transactions/lines lists*/
	//this memory will be freed after we display the errors (if any), until then keep it 
	int itransloop;
	LPTSTR ptransloop;

	// Look first for unexpected output of auditdb
	for (ptransloop = OutputLines.GetFirstTraceLine(0); ptransloop;ptransloop=OutputLines.GetNextTraceLine(0)) {
		BOOL bKnownStringToParse = FALSE;
		if (*ptransloop == '\0')
			bKnownStringToParse = TRUE;
		for (LPTSTR * pl1 = ParsedStrings;*pl1 && !bKnownStringToParse;pl1++) {
			if (!_tcsncmp(ptransloop,*pl1, strlen(*pl1))) {
				bKnownStringToParse = TRUE;
				break;
			}
		}
		p = ptransloop;
		while (*p == ' ' || *p=='\t')
			p++;
		if (*p==']') {
			p++;
			while (*p == ' ' || *p=='\t')
				p++;
			if (*p=='\0')
				bKnownStringToParse = TRUE;
		}

		if (!_tcsncmp(p,szLSN_equ, strlen(szLSN_equ))) 
			bKnownStringToParse = TRUE;
		
		if (!bKnownStringToParse) {
			CString strMsg;
			CString strFormat;
			if (_tcsstr(ptransloop,_T(" E_")))
			{
				// 
				// %s\n\nContinue ?\n(This may lead to inconsistent information)
				strFormat.LoadString (IDS_PARSE_ERROR1);
				if (_tcsstr(ptransloop,_T("E_DM1025_JSP_BAD_DATE_RANGE")) || _tcsstr(ptransloop,_T("E_DM1200_ATP_DB_NOT_JNL")))
				{
					AfxMessageBox(strAuditdbOutput);
					strAuditdbOutput=_T("");
					return FALSE;
				}
			}
			else 
			{
				// 
				// Unexpected Message :\n%s\n\nContinue ?\n(This may lead to inconsistent information)
				strFormat.LoadString (IDS_PARSE_ERROR2);
			}

			strMsg.Format(strFormat, ptransloop);
			int nAnswer = AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_YESNO);
			if (nAnswer != IDYES)
				return FALSE;
		}
	}
	strAuditdbOutput=_T("");

	for (itransloop=0,ptransloop = OutputLines.GetFirstTraceLine(0); ptransloop; itransloop++, ptransloop=OutputLines.GetNextTraceLine(0)) {
		if (!_tcsncmp(ptransloop,szBeginTransact, _tcslen(szBeginTransact))) {
			unsigned long l_lsn_begin_high;
			unsigned long l_lsn_begin_low;
			unsigned long l_lsn_end_high;
			unsigned long l_lsn_end_low;
			CString csTransEndTime;
			BOOL bCommitted = TRUE;
			BOOL bAllJournaled = TRUE;
			BOOL bIncludePartialRollback = FALSE;
			p=ptransloop+_tcslen(szBeginTransact); // points to begin of transaction sub-string
			p1=_tcschr(p,_T(' '));
			if (!p1)
				return FALSE;
			CString csTransact = GetStringBetween2Pointers(p, p1);
			p1++; // begin of start timestamp
			p=_tcsstr(p1,szBeginTransactUsername);
			if (!p)
				return FALSE;
			CString csTransBeginTime = suppspace(GetStringBetween2Pointers(p1, p));
			csTransUser = suppspace(p+_tcslen(szBeginTransactUsername));
			p=OutputLines.GetNextTraceLine(0);
			while (*p==_T(' ') || *p==_T('\t'))
				p++;
			if (_tcsncmp(p,szLSN_equ,_tcslen(szLSN_equ)))
				return FALSE;
			p+=_tcslen(szLSN_equ);
			p1=_tcschr(p,_T(','));
			if (!p1)
				return FALSE;
			csBuf =  GetStringBetween2Pointers(p, p1);
			l_lsn_begin_high=_ttoul(csBuf);
			p=p1+1;
			p1=_tcschr(p,_T(')'));
			if (!p)
				return FALSE;
			csBuf =  GetStringBetween2Pointers(p, p1);
			l_lsn_begin_low=_ttoul(csBuf);
			CString csLSNBegin;
			csLSNBegin.Format (_T("%08x%08x"), l_lsn_begin_high, l_lsn_begin_low);

			CString csLSNEnd;
			int isubloop;
			LPTSTR psubloop;
			BOOL bEndFound = FALSE; // next loop searches for the transaction end and partial rollback information
			for (isubloop=0,psubloop = OutputLines.GetFirstTraceLineAfter(1, 0); psubloop; isubloop++, psubloop=OutputLines.GetNextTraceLine(1)) {
				if (!_tcsncmp(psubloop,szEndTransact,_tcslen(szEndTransact))) {
					p=psubloop+_tcslen(szEndTransact); // points to begin of transaction sub-string
					p1=_tcschr(p,_T(' '));
					if (!p1)
						return FALSE;
					CString csTransact1 = GetStringBetween2Pointers(p, p1);
					if (csTransact1.Compare(csTransact)!=0)
						continue;
					bEndFound = TRUE;
					p1++; // start of end timestamp
					csTransEndTime = suppspace(p1);
					p=OutputLines.GetNextTraceLine(1);
					CString cst1 = p;
					cst1.TrimRight();
					if (cst1.Compare((LPCTSTR)szEndCommit)==0) {
						bCommitted = TRUE;
						bAllJournaled = TRUE;
					}
					else {
						if (cst1.Compare((LPCTSTR)szEndAbort)==0) {
							bCommitted = FALSE;
							bAllJournaled = TRUE;
						}
						else  {
							if (cst1.Compare((LPCTSTR)szEndCommitNJ)==0) {
								bCommitted = TRUE;
								bAllJournaled = FALSE;
							}
							else {
								if (cst1.Compare((LPCTSTR)szEndAbortNJ)==0) {
									bCommitted = FALSE;
									bAllJournaled = FALSE;
								}
								else
									return FALSE;
							}
						}
					}
					p=OutputLines.GetNextTraceLine(1);
					while (*p==_T(' ') || *p==_T('\t'))
						p=_tcsinc(p);
					if (_tcsncmp(p,szLSN_equ,_tcslen(szLSN_equ)))
						return FALSE;
					p+=_tcslen(szLSN_equ);
					p1=_tcschr(p,_T(','));
					if (!p1)
						return FALSE;
					csBuf =  GetStringBetween2Pointers(p, p1);
					l_lsn_end_high=_ttoul(csBuf);
					p=p1+1;
					p1=_tcschr(p,_T(')'));
					if (!p)
						return FALSE;
					csBuf =  GetStringBetween2Pointers(p, p1);
					l_lsn_end_low=_ttoul(csBuf);
					csLSNEnd.Format (_T("%08x%08x"), l_lsn_end_high, l_lsn_end_low);
					break;
				}
				if (!_tcsncmp(psubloop,szAbortSave,_tcslen(szAbortSave))) {
					p=psubloop+_tcslen(szAbortSave); // points to begin of transaction sub-string
					p1=_tcschr(p,_T(' '));
					if (!p1)
						p1=p+_tcslen(p);
					CString csTransact1 = GetStringBetween2Pointers(p, p1);
					if (csTransact1.Compare(csTransact)==0)
						bIncludePartialRollback = TRUE;
				}
			}
			if (!bEndFound)
				continue; // don't display non complete transactions
			CaDatabaseTransactionItemData* pTran = new CaDatabaseTransactionItemData();
			pTran->SetTransactionID(csTransact);
			pTran->SetLsnBegin(csLSNBegin);
			pTran->SetLsnEnd  (csLSNEnd);
			pTran->SetUser (csTransUser);
			pTran->SetBegin(csTransBeginTime);/*CString (tchszBegin1) + CString (_T(' ')) + CString (tchszBegin2));*/
			pTran->SetEnd  (csTransEndTime);/*CString (tchszBegin1) + CString (_T(' ')) + CString (tchszBegin2)); // Same as begin !!!*/
			pTran->GetEllapsed().Set((LPCTSTR)csTransBeginTime,(LPCTSTR)csTransEndTime);
			pTran->SetCommit (bCommitted);
			pTran->SetJournal (bAllJournaled); 
			pTran->SetRollback2SavePoint(bIncludePartialRollback);

			// search for detail 
			BOOL bDetailFound = FALSE;

			CTypedPtrList < CObList, CaDatabaseTransactionItemDataDetail* >& lPdt = pTran->GetDetailTransaction();

			for (isubloop=0,psubloop = OutputLines.GetFirstTraceLineAfter(1, 0); psubloop; isubloop++, psubloop=OutputLines.GetNextTraceLine(1)) {
				TxType txStatement =  T_UNKNOWN;
				unsigned long l_lsn_high;
				unsigned long l_lsn_low;
				unsigned long l_tid;
				unsigned long l_tid_new;
				CString csRecord;
				CString csBeforeRecord;
				CString csAfterRecord;
				CString csLocation;

				// look for AbortSave Record for the given transaction
				if (!_tcsncmp(psubloop,szAbortSave,_tcslen(szAbortSave))) {
					unsigned long lsn_tp_high;
					unsigned long lsn_tp_low;
					p=psubloop+_tcslen(szAbortSave); // points to begin of transaction sub-string
					p1=_tcschr(p,_T(' '));
					if (!p1)
						p1=p+_tcslen(p);
					CString csTransact1 = GetStringBetween2Pointers(p, p1);
					if (csTransact1.Compare(csTransact)!=0)
						continue;
					p=OutputLines.GetNextTraceLine(1);
					if (_tcsncmp(p,szAbort2SavePoint,_tcslen(szAbort2SavePoint))) {
						delete pTran;
						return FALSE;
					}
					p+=_tcslen(szAbort2SavePoint);
					p1=_tcschr(p,_T(','));
					if (!p1)
						return FALSE;
					csBuf =  GetStringBetween2Pointers(p, p1);
					lsn_tp_high=_ttoul(csBuf);
					p=p1+1;
					p1=_tcschr(p,_T(')'));
					if (!p)
						return FALSE;
					csBuf =  GetStringBetween2Pointers(p, p1);
					lsn_tp_low=_ttoul(csBuf);

					// remove from list those statements with an LSN > tracepoint one
					BOOL bREDOloop = TRUE;
					while (bREDOloop) {
						bREDOloop = FALSE;
						POSITION pos = lPdt.GetHeadPosition();
						while (pos !=  NULL)
						{
							POSITION poscur = pos;
							unsigned long l_lsn_high1, l_lsnlow1;
							CaDatabaseTransactionItemDataDetail* pItem = lPdt.GetNext (pos);
							ASSERT (pItem);
							if (!pItem)
								continue;
							pItem->GetLsn(l_lsn_high1, l_lsnlow1);
							if ( l_lsn_high1>lsn_tp_high ||
								(l_lsn_high1 == lsn_tp_high && l_lsnlow1>lsn_tp_low)
								) {
								delete pItem;
								lPdt.RemoveAt(poscur);
								bREDOloop = TRUE;
								break;
							}
						}
					}
					continue;
				}

				// look for insert/Update/Delete row
				if (!_tcsncmp(psubloop,szInsert_Append,_tcslen(szInsert_Append))) {
					txStatement = T_INSERT;
					p=psubloop+_tcslen(szInsert_Append); // points to begin of transaction sub-string
				}
				if (!_tcsncmp(psubloop,szdeleteRow,_tcslen(szdeleteRow))) {
					txStatement = T_DELETE;
					p=psubloop+_tcslen(szdeleteRow); // points to begin of transaction sub-string
				}
				if (!_tcsncmp(psubloop,szUpdateRow,_tcslen(szUpdateRow))) {
					txStatement = T_BEFOREUPDATE;
					p=psubloop+_tcslen(szUpdateRow); // points to begin of transaction sub-string
				}
				if (!_tcsncmp(psubloop,szCreateTableRec,_tcslen(szCreateTableRec))) {
					txStatement = T_CREATETABLE;
					p=psubloop+_tcslen(szCreateTableRec); // points to begin of transaction sub-string
				}
				if (!_tcsncmp(psubloop,szAlterTableRec,_tcslen(szAlterTableRec))) {
					txStatement = T_ALTERTABLE;
					p=psubloop+_tcslen(szAlterTableRec); // points to begin of transaction sub-string
				}
				if (!_tcsncmp(psubloop,szDropTableRec,_tcslen(szDropTableRec))) {
					txStatement = T_DROPTABLE;
					p=psubloop+_tcslen(szDropTableRec); // points to begin of transaction sub-string
				}
				if (!_tcsncmp(psubloop,szRelocate,_tcslen(szRelocate))) {
					txStatement = T_RELOCATE;
					p=psubloop+_tcslen(szRelocate); // points to begin of transaction sub-string
				}
				if (!_tcsncmp(psubloop,szModify,_tcslen(szModify))) {
					txStatement = T_MODIFY;
					p=psubloop+_tcslen(szModify); // points to begin of transaction sub-string
				}
				if (!_tcsncmp(psubloop,szIndex,_tcslen(szIndex))) {
					txStatement = T_INDEX;
					p=psubloop+_tcslen(szIndex); // points to begin of transaction sub-string
				}

				if (txStatement != T_UNKNOWN) {
					p1=_tcschr(p,_T(' '));
					if (!p1)
						return FALSE;
					CString csTransact1 = GetStringBetween2Pointers(p, p1);
					if (csTransact1.Compare(csTransact)!=0)
						continue;
					bDetailFound = TRUE;
					p1 = _tcsstr(p1,szTable);
					if (!p1) {
						delete pTran;
						return FALSE;
					}
					p=p1+_tcslen(szTable);
					p1=_tcschr(p,_T(','));
					if (!p1) {
						delete pTran;
						return FALSE;
					}
					CString csTable = suppspace( GetStringBetween2Pointers(p, p1));
					p=p1+1;
					p1=_tcschr(p,_T(']'));
					if (!p1) {
						char * p2=OutputLines.GetNextTraceLine(1);
						if (p2) {
							while (*p2==' ')
								p2=_tcsinc(p2);
							if (*p2==']') // case where ']' is isolated on the next line ( "Drop/Destroy)" )
								p1 = p+_tcslen(p);
						}
					}
					if (!p1) {
						delete pTran;
						return FALSE;
					}
					CString csSchema = suppspace( GetStringBetween2Pointers(p, p1));
					p=OutputLines.GetNextTraceLine(1);
					if (txStatement == T_CREATETABLE ||
						txStatement == T_DROPTABLE      ) {
						/* the location information is not always present: in particular,
						   it is not there for a create view. But it is there for 
						   drop view... */
						if (_tcsncmp(p,szCreDropTableLoc,_tcslen(szCreDropTableLoc))==0) {
							csLocation = p+_tcslen(szCreDropTableLoc);
							csRecord = "Location: "+csLocation;
							p=OutputLines.GetNextTraceLine(1);
						}
					}
					if (txStatement == T_RELOCATE) {
						if (_tcsncmp(p,szRelocOldLoc,_tcslen(szRelocOldLoc))) {
							delete pTran;
							return FALSE;
						}
						csLocation = p+_tcslen(szRelocOldLoc);
						p=OutputLines.GetNextTraceLine(1);
						if (_tcsncmp(p,szRelocNewLoc,_tcslen(szRelocNewLoc))) {
							delete pTran;
							return FALSE;
						}
						csLocation += " -> ";
						csLocation += p+_tcslen(szRelocNewLoc);
						csRecord = "Location: "+csLocation;
						p=OutputLines.GetNextTraceLine(1);
					}
					while (*p ==_T(' ') || *p==_T('\t'))
						p++;
					if (_tcsncmp(p,szLSN_equ,_tcslen(szLSN_equ))) {
						delete pTran;
						return FALSE;
					}
					p+=_tcslen(szLSN_equ);
					p1=_tcschr(p,_T(','));
					if (!p1)
						return FALSE;
					csBuf =  GetStringBetween2Pointers(p, p1);
					l_lsn_high=_ttoul(csBuf);
					p=p1+1;
					p1=_tcschr(p,_T(')'));
					if (!p)
						return FALSE;
					csBuf =  GetStringBetween2Pointers(p, p1);
					l_lsn_low=_ttoul(csBuf);
					l_tid = -1L;
					l_tid_new = -1L;

					if (txStatement == T_INSERT ||
						txStatement == T_DELETE ||
						txStatement == T_BEFOREUPDATE ) { // get the data
						// get TID (of after image)
						if (txStatement == T_BEFOREUPDATE) {
							p = _tcsstr(p1,szOldTid);
							if (!p)
								return FALSE;
							p += _tcslen(szOldTid);
							p1  = _tcsstr(p,szNewTid);
							if (!p1)
								return FALSE;
							csBuf =  GetStringBetween2Pointers(p, p1);
							l_tid=_ttol(csBuf);
							p1=p1 +_tcslen(szNewTid);
							while (*p1==' ')
								p1++;
							l_tid_new = _ttol(p1);
						}
						else {
							p1 = _tcsstr(p1,szTid);
							if (!p1)
								return FALSE;
							p1 += _tcslen(szTid);
							while (*p1==' ')
								p1++;
							l_tid = _ttol(p1);
						}

						p=OutputLines.GetNextTraceLine(1);
						if (txStatement == T_BEFOREUPDATE) {
							if (_tcsncmp(p,szUpdateOldRow,_tcslen(szUpdateOldRow))) { /* old row*/
								delete pTran;
								return FALSE;
							}
							p+=_tcslen(szUpdateOldRow);
							while (*p==_T(' '))
								p++;
							if (*p==_T('<')) {
								p++;
								if (_tcslen(p))
									p[_tcslen(p)-1]=_T('\0'); // remove trailing ">" (if multiline, truncates anyhow. temporary code until additional options of auditdb are implemented
							}
							csBeforeRecord = p;
							while ( (p=OutputLines.GetNextTraceLine(1)) !=NULL) {
								if (! _tcsncmp(p,szUpdateNewRow,_tcslen(szUpdateNewRow))) { /* new row (the line will always be the next one when additional auditdsb options will be available*/
									p+=_tcslen(szUpdateNewRow);
									while (*p==_T(' '))
										p++;
									if (*p==_T('<')) {
										p++;
										if (_tcslen(p))
											p[_tcslen(p)-1]=_T('\0'); // remove trailing ">" (if multiline, truncates anyhow. temporary code until additional options of auditdb are implemented
									}
									csAfterRecord = p;
									break;
								}
							}
							if (!p) {
								delete pTran;
								return FALSE;
							}
						}
						else { /* insert / delete have an identical parsing */
							if (_tcsncmp(p,szInsDel_Record,_tcslen(szInsDel_Record))) {
								delete pTran;
								return FALSE;
							}
							p+=_tcslen(szInsDel_Record);
							if (*p==_T('<')) {
								p++;
								if (_tcslen(p))
									p[_tcslen(p)-1]=_T('\0'); // remove trailing ">" (if multiline, truncates anyhow. temporary code until additional options of auditdb are implemented
							}
							csRecord = p;
						}
					}

					CaDatabaseTransactionItemDataDetail* pDetail = new CaDatabaseTransactionItemDataDetail();
					unsigned long l1,l2;
					if (!Get2LongsFromHexString((LPCTSTR)csTransact, l1, l2)) {
							delete pTran;
							return FALSE;
					}
					pDetail->SetTransactionID (l1,l2);
					pDetail->SetLsn (l_lsn_high,l_lsn_low);
					pDetail->SetTid (l_tid);
					pDetail->SetAction (txStatement);
					pDetail->SetUser  (pTran->GetUser());
					pDetail->SetTable ((LPCTSTR)csTable);
					pDetail->SetTableOwner ((LPCTSTR)csSchema);
					CStringList& theList = pDetail-> GetListData();

					pDetail->SetCommit (bCommitted);
					pDetail->SetJournal (bAllJournaled);
					pDetail->SetRollback2SavePoint(bIncludePartialRollback);

					switch (txStatement) {
						case T_INSERT:
							theList.AddTail((LPCTSTR)csRecord);
							pTran->SetCountTotal (pTran->GetCountTotal () + 1);
							pTran->SetCountInsert(pTran->GetCountInsert() + 1);
							break;
						case T_DELETE:
							theList.AddTail((LPCTSTR)csRecord);
							pTran->SetCountTotal (pTran->GetCountTotal () + 1);
							pTran->SetCountDelete(pTran->GetCountDelete() + 1);
							break;
						case T_BEFOREUPDATE:
							theList.AddTail((LPCTSTR)csBeforeRecord);
							pTran->SetCountTotal (pTran->GetCountTotal () + 1);
							pTran->SetCountUpdate(pTran->GetCountUpdate() + 1);
							break;
						case T_CREATETABLE:
						case T_DROPTABLE:
						case T_RELOCATE:
							theList.AddTail((LPCTSTR)csRecord);
							break;
						case T_ALTERTABLE:
							theList.AddTail((LPCTSTR)_T(""));
							break;
					}

					lPdt.AddTail (pDetail);

					if (txStatement ==T_BEFOREUPDATE) { // add the "after image" record
						pDetail = new CaDatabaseTransactionItemDataDetail();
						if (!Get2LongsFromHexString((LPCTSTR)csTransact, l1, l2)) {
							delete pTran;
							return FALSE;
						}
						pDetail->SetTransactionID (l1,l2);
						pDetail->SetLsn (l_lsn_high,l_lsn_low);
						pDetail->SetTid (l_tid_new);
						pDetail->SetAction (T_AFTERUPDATE);
						pDetail->SetUser  (pTran->GetUser());
						pDetail->SetTable ((LPCTSTR)csTable);
						pDetail->SetTableOwner ((LPCTSTR)csSchema);
						//theList = pDetail-> GetListData();
						 pDetail-> GetListData().AddTail((LPCTSTR)csAfterRecord);;

						pDetail->SetCommit (bCommitted);
						pDetail->SetJournal (bAllJournaled);
						pDetail->SetRollback2SavePoint(bIncludePartialRollback);

						lPdt.AddTail (pDetail);
					}
				}
			}
			if (!bDetailFound) 
				delete pTran;
			else {
				CaDatabaseTransactionItemDataDetail& begin = pTran->GetBeginTransaction();
				CaDatabaseTransactionItemDataDetail& end = pTran->GetEndTransaction();
				if (!Get2LongsFromHexString((LPCTSTR)csTransact, l1, l2)) {
					delete pTran;
					return FALSE;
				}

				begin.SetTransactionID(l1,l2);
				begin.SetLsn (l_lsn_begin_high,l_lsn_begin_low);
				begin.SetUser(csTransUser);
				begin.SetTransactionBegin(pTran->GetBegin());
				begin.SetTable(_T(""));
				begin.SetTableOwner(_T(""));
				begin.SetCommit (bCommitted);
				begin.SetJournal (bAllJournaled);
				begin.SetRollback2SavePoint(bIncludePartialRollback);

				end.SetTransactionID(l1,l2);
				end.SetLsn (l_lsn_end_high,l_lsn_end_low);
				end.SetUser(csTransUser);
				end.SetTransactionEnd(pTran->GetEnd());
				end.SetTable(_T(""));
				end.SetTableOwner(_T(""));
				end.SetCommit (bCommitted);
				end.SetJournal (bAllJournaled);
				end.SetRollback2SavePoint(bIncludePartialRollback);
				listTransaction.AddTail (pTran);
			}
		}
	}
	return TRUE;
}
