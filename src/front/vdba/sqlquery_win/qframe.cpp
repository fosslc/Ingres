/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qframe.cpp, Implementation file    (MDI Child Frame)
**    Project  : Visual DBA.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Frame window for the SQL Test
**
** History:
**
** xx-Oct-1997 (uk$so01)
**    Created.
** 07-Jan-2000 (uk$so01)
**    (Bug #99940)
*     The non-selected statements will not be exected any more in the QEP mode.
** 31-Jan-2000 (uk$so01)
**    (Bug #100235)
**    Special SQL Test Setting when running on Context.
** 22-May-2000 (schph01)
**     bug 99242 (DBCS) replace isspace() by _istspace()
** 22-May-2000 (uk$so01)
**    bug 99242 (DBCS) modify the functions:
**    NextWord(), MarkCommentStart(), MarkCommentEnd(), Find(), ParseStatement()
** 26-Sep-2000 (uk$so01)
**    SIR #102711: Callable in context command line improvement (for Manage-IT)
**    Select the input database if specified.
** 26-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
** 28-may-2001 (schph01)
**    BUG 104636 get the "SQL Test Preferences"
** 29-May-2001 (uk$so01)
**    SIR #104736, Integrate Ingres Import Assistant.
** 27-Jun-2001 (noifr01)
**    (sir 103694) invoke new special functions for executing statements that
**    include unicode constants
** 05-Jul-2001 (uk$so01)
**    SIR #105199. Integrate & Generate XML.
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 22-Fev-2002 (uk$so01)
**    SIR #107133. Use the select loop instead of cursor when we get
**    all rows at one time.
** 21-Oct-2002 (uk$so01)
**    BUG/SIR #106156 Manually integrate the change #453850 into ingres30
** 26-Feb-2003 (uk$so01)
**    SIR #106648, conform to the change of DDS VDBA split
** 13-Jun-2003 (uk$so01)
**    SIR #106648, Take into account the STAR database for connection.
** 17-Jul-2003 (uk$so01)
**    SIR #106648, Vdba-Split, ensure that the ingres sessions 
**    have the descriptions.
** 03-Oct-2003 (uk$so01)
**    SIR #106648, Vdba-Split, Additional fix for GATEWAY Enhancement 
** 15-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
** 17-Oct-2003 (uk$so01)
**    SIR #106648, Additional change to change #464605 (role/password)
** 07-Nov-2003 (uk$so01)
**    SIR #106648. Additional fix the session conflict between the container and ocx.
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 02-feb-2004 (somsa01)
**    Changed CFile::WriteHuge()/ReadHuge() to CFile::Write()/Read(), as
**    WriteHuge()/ReadHuge() is obsolete and in WIN32 programming
**    Write()/Read() can also write more than 64K-1 bytes of data.
** 17-Feb-2004 (schph01)
**    BUG 99242,  cleanup for DBCS compliance
** 26-Mar-2004 (uk$so01)
**    SIR #111701, The default help page is available now!. 
**    Activate the default page if no specific ID help is specified.
** 01-Sept-2004 (schph01)
**    SIR #106648 add method SetErrorFileName in CfSqlQueryFrame class
** 05-Nov-2004 (uk$so01)
**    BUG #113389 / ISSUE #13765425 Simplified Chinese WinXP
**    Part of the statement in multiple statements execution could
**    be incorrectly highlighted.
** 09-Nov-2004 (uk$so01)
**    BUG #113416 / ISSUE #13771051 On Simplified Chinese WinXP.
**    In VDBA, if the SQL window is closed without a commit after inserting some 
**    queries, the query seem to be pending (uncommitted) until the VDBA 
**    application is closed.
** 28-Feb-2007 (wonca01) BUG 117804
**    Add code in CfSqlQueryFrame::ParseStatement() to remove comments
**    and extra whitespaces in the query statement.
** 21-Aug-2008 (whiro01)
**    Removed redundant <afx...> include which is already in "stdafx.h"
** 28-Sep-2009 (smeke01) b122456
**    Amend code in CfSqlQueryFrame::ParseStatement() to ensure that comments 
**    and spaces embedded in quotes are not removed. Also corrected check for 
**    whether the statement is a 'select' or not.
**/

#include "stdafx.h"
#include <htmlhelp.h>
#include "rcdepend.h"
#include "qframe.h"
#include "qpageres.h" // Result page for normal select
#include "qpagexml.h" // Result page for XML
#include "libguids.h" // guids
#include "sqlasinf.h" // sql assistant interface
#include "taskxprm.h"
#include "parse.h"
#include "ingobdml.h"
#include "sqltrace.h"
#include "a2stream.h"
#include "xmlgener.h"
#include "tlsfunct.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
extern BOOL HasUniConstInWhereClause(char *rawstm,char *pStmtWithoutWhereClause);
static int Find (const CString& strText, TCHAR chFind, int nStart =0, BOOL bIgnoreInsideBeginEnd = TRUE);

static BOOL IsScriptCharSetInvalid (TCHAR tch)
{
	switch (tch)
	{
	case _T('g'):
	case _T('q'):
	case _T('G'):
	case _T('Q'):
		return TRUE;
	default:
		return FALSE;
	}
	return FALSE;
}

#define SQL_KW_USE_SEMICOLUMN  0
#define SQL_KW_IGNORE_ACCEPTED 1
#define SQL_KW_NOT_ACCEPTED    2

struct tagKwtype 
{
	TCHAR * keyword;
	int itype;
} kwTypes[] = 
{
	// substring Keywords must be placed AFTER corresponding longer string
	{_T("go")        ,  SQL_KW_USE_SEMICOLUMN},
	{_T("g")         ,  SQL_KW_USE_SEMICOLUMN},
	{_T("q")         ,  SQL_KW_USE_SEMICOLUMN},
	{_T("reset")     ,  SQL_KW_NOT_ACCEPTED},
	{_T("r")         ,  SQL_KW_NOT_ACCEPTED},
	{_T("print")     ,  SQL_KW_NOT_ACCEPTED},
	{_T("p")         ,  SQL_KW_NOT_ACCEPTED},
	{_T("editor")    ,  SQL_KW_NOT_ACCEPTED},
	{_T("edit")      ,  SQL_KW_NOT_ACCEPTED},
	{_T("ed")        ,  SQL_KW_NOT_ACCEPTED},
	{_T("e")         ,  SQL_KW_NOT_ACCEPTED},
	{_T("append")    ,  SQL_KW_NOT_ACCEPTED},
	{_T("a")         ,  SQL_KW_NOT_ACCEPTED},
	{_T("time")      ,  SQL_KW_IGNORE_ACCEPTED},
	{_T("date")      ,  SQL_KW_IGNORE_ACCEPTED},
	{_T("shell")     ,  SQL_KW_IGNORE_ACCEPTED},
	{_T("sh")        ,  SQL_KW_IGNORE_ACCEPTED},
	{_T("s")         ,  SQL_KW_IGNORE_ACCEPTED},
	{_T("cd")        ,  SQL_KW_NOT_ACCEPTED},
	{_T("chdir")     ,  SQL_KW_NOT_ACCEPTED},
	{_T("include")   ,  SQL_KW_NOT_ACCEPTED},
	{_T("i")         ,  SQL_KW_NOT_ACCEPTED},
	{_T("read")      ,  SQL_KW_NOT_ACCEPTED},
	{_T("write")     ,  SQL_KW_NOT_ACCEPTED},
	{_T("w")         ,  SQL_KW_NOT_ACCEPTED},
	{_T("script")    ,  SQL_KW_NOT_ACCEPTED},
	{_T("bell")      ,  SQL_KW_NOT_ACCEPTED},
	{_T("nobell")    ,  SQL_KW_NOT_ACCEPTED},
	{_T("nocontinue"),  SQL_KW_NOT_ACCEPTED},
	{_T("noco")      ,  SQL_KW_NOT_ACCEPTED},
	{_T("continue")  ,  SQL_KW_NOT_ACCEPTED},
	{_T("co")        ,  SQL_KW_NOT_ACCEPTED},
	{NULL            ,  0                  }  
};
struct tagKwtype *pkwtype;

static BOOL IsSpecialSQLSequence(CString strKW, int * pitype,int * pnlen)
{
	for (pkwtype =kwTypes;pkwtype->keyword!=NULL;pkwtype++) {
		CString str0 = pkwtype->keyword;
		CString str = strKW.Left(str0.GetLength());
		if (str.CompareNoCase(str0)==0) {
			*pitype = pkwtype->itype;
			*pnlen = str0.GetLength();
			return TRUE;
		}
	}
	return FALSE;
}

//
// Caller must ensure that the position i is always the position of a charactter
// ie leading byte.
// Test the sub-string started at <i> if it matchs the string <lpszWord>
// The sub-string, if it matchs <lpszWord>, must be terminated by <' '> or <;> or end of line.
static BOOL NextWord (const CString& strText, int index, LPCTSTR lpszWord)
{
	TCHAR* tchString = (TCHAR*)(LPCTSTR)strText;
	int nLen  = strText.GetLength();
	int nWLen = _tcslen (lpszWord);
	if (index > 0)
	{
		TCHAR* tchStringCurr = tchString + index;
		TCHAR* tchStringPrev = _tcsdec (tchString, tchStringCurr);
		TCHAR chBack = tchStringPrev? *tchStringPrev: _T('\0');
		switch (chBack)
		{
		case _T(' '):
		case _T(';'):
		case _T('\t'):
		case 0x0D:
		case 0x0A:
			break;
		default:
			return FALSE;
		}
	}

	if ((index + nWLen) <= nLen)
	{
		CString strExtract = strText.Mid (index, nWLen);
		if (strExtract.CompareNoCase (lpszWord) == 0)
		{
			if ((index + nWLen) == nLen )
				return TRUE;
			else
			{
				TCHAR tchszC = strText.GetAt (index + nWLen);
				switch (tchszC)
				{
				case _T(' '):
				case _T(';'):
				case _T('\t'):
				case 0x0D:
				case 0x0A:
					return TRUE;
				default:
					break;
				}
			}
		}
	}
	return FALSE;
}



//
// Caller must ensure that the position i is always the position of a charactter
// ie leading byte.
static void MarkCommentStart (const CString& strText, int i, int& nCommentStart)
{
	int nBytes = sizeof (TCHAR);
	if ((i+nBytes) >= strText.GetLength())
		return;
	TCHAR* tchString = (TCHAR*)(LPCTSTR)strText;
	TCHAR tchsz1 = *(tchString + i);
	tchString = _tcsinc (tchString + i);
	TCHAR tchsz2 = *(tchString);

	if (tchsz1 == _T('/') && tchsz2 == _T('*'))
		nCommentStart = 1;
	else
	if (tchsz1 == _T('-') && tchsz2 == _T('-'))
		nCommentStart = 2;
}

//
// Caller must ensure that the position i is always the position of a charactter
// ie leading byte.
static void MarkCommentEnd (const CString& strText, int i, int& nCommentEnd)
{
	int nBytes = sizeof (TCHAR);
	TCHAR* tchString = (TCHAR*)(LPCTSTR)strText;
	if ((i+nBytes) < strText.GetLength())
	{
		TCHAR tchsz1 = *(tchString + i);
		tchString = _tcsinc (tchString + i);
		TCHAR tchsz2 = *(tchString);
		if (tchsz1 == _T('*') && tchsz2 == _T('/'))
			nCommentEnd = 1;
		if (tchsz1 == 0x0D)
			nCommentEnd = 2;
	}
	else
	if (i < strText.GetLength())
	{
		TCHAR tchsz1 = *(tchString + i);
		if (tchsz1 == _T('\n'))
			nCommentEnd = 2;
	}
}


static int Find (const CString& strText, TCHAR chFind, int nStart, BOOL bIgnoreInsideBeginEnd)
{
	int   nBeginCount = 0;
	int   nCommentStart = 0; // 1: '/*'and 2: '--'.
	int   nCommentEnd   = 0; // 1: '*/'and 2: '\n'.
	BOOL  bSQuote = FALSE; 
	BOOL  bDQuote = FALSE;
	BOOL  bBegin  = FALSE;
	BOOL  bDeclare= FALSE;
	TCHAR cChar;
	ASSERT (chFind != _T('\"'));
	ASSERT (chFind != _T('\''));

	if (strText.IsEmpty())
		return -1;
	int i, nLen = strText.GetLength();
	
	for ( i=nStart; i<nLen; i+= _tclen((const TCHAR *)strText + i) )
	{
		if (nCommentStart > 0)
		{
			MarkCommentEnd (strText, i, nCommentEnd);
			if ((nCommentStart == 1 && nCommentEnd == 1) || (nCommentStart == 2 && nCommentEnd == 2))
			{
				//
				// DBCS, i += 1 will not work:
				i+= _tclen((const TCHAR *)strText + i);
				nCommentStart = 0;
				nCommentEnd   = 0;
			}
			continue;
		}
		else
		{
			//
			// The comment inside the double or single quote is considered 
			// as normal string:
			if (!(bDQuote || bSQuote))
			{
				MarkCommentStart (strText, i, nCommentStart);
				if (nCommentStart > 0)
				{
					//
					// DBCS, i += 1 will not work:
					i+= _tclen((const TCHAR *)strText + i);
					continue;
				}
			}
		}

		cChar = strText.GetAt (i);
		if (cChar == _T('\"') && !bSQuote)
		{
			bDQuote = !bDQuote;
			continue;
		}
		if (cChar == _T('\'') && !bDQuote)
		{
			//
			// If the next character is a '\'' (It means double the quote: '' to appear 
			// the quote inside the literal string)
			int iNext = i + _tclen((const TCHAR *)strText + i);
			if (iNext < nLen && strText.GetAt (iNext) != _T('\''))
			{
				bSQuote = !bSQuote;
				continue;
			}
			else
			{
				//
				// Simple Quote has been doubled:
				// Ignore the current and the next quote:
				
				//
				// DBCS, i += 1 will not work:
				i+= _tclen((const TCHAR *)strText + i);
			}
			continue;
		}
		if (bSQuote || bDQuote)
			continue; // Ignore the character inside the simple or double quote.

		if (bIgnoreInsideBeginEnd)
		{
			if (!bDeclare && (cChar == _T('D') || cChar == _T('d')))
			{
				if (NextWord (strText, i, _T("DECLARE")))
				{
					//
					// The 'bDeclare' will be set to false only if a 'begin' is found
					// some where far after the 'declare', otherwise the erronous syntax will
					// be generated !!!.
					bDeclare = TRUE;
					i += _tcslen (_T("DECLAR")); // i += strlen (_T("DECLARE")) -1;
					continue;
				}
			}

			if (!bBegin && (cChar == _T('B') || cChar == _T('b')))
			{
				if (NextWord (strText, i, _T("BEGIN")))
				{
					bBegin = TRUE;
					bDeclare = FALSE;
					nBeginCount++;
					i += _tcslen (_T("BEGI")); // i += strlen (_T("BEGIN")) -1;
					continue;
				}
			}
			
			if (bBegin)
			{
				//
				// Nesting of Begin ?
				if (cChar == _T('B') || cChar == _T('b'))
				{
					if (NextWord (strText, i, _T("BEGIN")))
					{
						nBeginCount++;
						i += _tcslen (_T("BEGI")); // i += strlen (_T("BEGIN")) -1;
						continue;
					}
				}
			
				if (cChar == _T('E') || cChar == _T('e'))
				{
					if (NextWord (strText, i, _T("END")))
					{
						nBeginCount--;
						if (nBeginCount == 0)
							bBegin = FALSE;
						i += _tcslen (_T("EN")); // i += strlen (_T("END")) -1;
					}
				}
				//
				// Ignore the character after the word <BEGIN>, ie do not
				// find the ';' inside the begin ... end.
				continue;
			}

			if (bDeclare)
			{
				//
				// Ignore the character after the word <DECLARE>, ie do not
				// find the ';' inside the declare ... begin.
				continue;
			}
		}
		if (cChar == chFind)
			return i;
	}
	return -1;
}

static int CharacterCount(CString& s)
{
	int nCount = 0;
	TCHAR tchszCur;
	TCHAR* pch = (LPTSTR)(LPCTSTR)s;
	while (pch && *pch != _T('\0'))
	{
		nCount++;
		tchszCur = *pch;
		if (_istleadbyte(tchszCur))
		{
			pch = _tcsinc(pch);
			continue;
		}
		pch = _tcsinc(pch);
	}
	return nCount;
}

BOOL CfSqlQueryFrame::ParseStatement (int nExec, CString& strRawStatement)
{
	int pos = -1, nStart = 0, nEnd =0;
	CString strText = strRawStatement; // Raw statement from the edit box
	CString strStatement;              // A statement after parsing
	CString strSelectKey;              // Key word select.
	BOOL    bSelectStmt; 
	CaSqlQueryData* pData = NULL;
	CaSqlQueryStatement* pStatementInfo = NULL;
	BOOL bShowMsg = FALSE;
	
	pos = Find (strText, _T(';'));
	while (pos != -1 || !strText.IsEmpty())
	{
		bSelectStmt    = FALSE;
		pStatementInfo = NULL;
		if (pos != -1)
		{
			strStatement   = strText.Left (pos);
			strText        = strText.Mid (pos+1);
			nEnd = nStart + CharacterCount(strStatement);
		}
		else
		{
			strStatement = strText;
			nEnd = nStart + CharacterCount(strStatement);
			strStatement.TrimLeft();
			strStatement.TrimRight();
			strText = _T("");
		}
		pos = Find (strText, _T(';'));
		strStatement.TrimLeft ();
		strStatement.TrimRight ();

		if (strStatement.IsEmpty())
		{
			nStart = nEnd;
			nStart+= _tclen((const TCHAR *)strRawStatement + nStart);
			continue;
		}
		
		int commentStart=-1;
		int commentEnd=-1;
		int singlequoteStart = -1;
		int singlequoteEnd = -1;
		int pos2 = 0;

		do
		{ 
			singlequoteStart = strStatement.Find("\'", pos2);
			if (singlequoteStart != -1)
				singlequoteEnd = strStatement.Find("\'", singlequoteStart + 1);
			
			commentStart = strStatement.Find("/*", pos2);
			if (commentStart != -1)
				commentEnd = strStatement.Find("*/", commentStart + 1);
		
			if (commentStart == -1)
			{
				//There are no comments (left) to deal with, so leave the (remainder of the) statement unchanged
				break;
			}
			
			if (((singlequoteStart != -1) && (singlequoteStart < commentStart)))
			{
				if (singlequoteEnd != -1)
				{
					// Quotes mean include everything between the quotes. Move up and go round again
					pos2 = singlequoteEnd + 1;
					singlequoteEnd = -1;
					commentEnd = -1;
					continue;
				}
				else
				{
					// We have an unbalanced quote so something's wrong. Pass the lot on so the server can abuse the end-user
					break;
				}
			}
			
			// Implicit else: assert( commentStart != -1 && (singlequoteStart == -1 || commentStart > singlequoteStart) ) 

			if ( (commentEnd != -1) && (commentStart < commentEnd))
			{
				// Comments mean exclude everything between the comment start and end marker. Delete, move up and go round again
				strStatement.Delete (commentStart, (commentEnd + 2) - commentStart);
				pos2 = commentStart + 1;
				singlequoteEnd = -1;
				commentEnd = -1;
				continue;
			}
			else 
			{
				// We have an unbalanced comment so something's wrong. Pass the lot on so the server can abuse the end-user
				break;
			}
		} while ( pos2 < (strStatement.GetLength() - 1) );
	
		strStatement.Replace("\n"," ");
		strStatement.Replace("\r"," ");
		strStatement.Replace("\t"," ");
		
		singlequoteStart = -1;
		singlequoteEnd = -1;
		CString strAccumulate, strIntermediate;
		while (!strStatement.IsEmpty())
		{ 
			singlequoteStart = strStatement.Find("\'", 0);
			if (singlequoteStart != -1)
				singlequoteEnd = strStatement.Find("\'", singlequoteStart + 1);
			
			if (singlequoteStart == -1 || singlequoteEnd == -1)
			{			
				//There are no (more) quotes to worry about - just blast through the 
				//(remaining) statement reducing space sequences to 1 space character
				for ( ; strStatement.Replace("  "," "); )
					;
				strAccumulate += strStatement;
				//All done
				break;
			}
			
			//Implicit else: assert( singlequoteStart != -1 && singlequoteEnd != -1)
			if (singlequoteStart > 0)
			{
				//Grab the text up to and including the start quote. Replace sequences 
				//of spaces with 1 space character.
				strIntermediate = strStatement.Left(singlequoteStart + 1);
				for ( ; strIntermediate.Replace("  "," "); )
					;
				//Copy over the amended text
				strAccumulate += strIntermediate;
				//Remove from the string being analysed the text we've copied over plus
				//the start quote character.
				strStatement.Delete (0, singlequoteStart + 1);
			}				

			//Copy over the quoted text up to and including the end quote. 
			strAccumulate += strStatement.Left(singlequoteEnd + 1);
			//Remove from the string being analysed the quoted text 
			//and the end quote character
			strStatement.Delete (0, singlequoteEnd + 1);
			singlequoteEnd = -1;
		}
		strStatement = strAccumulate;

		strStatement.TrimLeft();
		strStatement.TrimRight();

		if (!strStatement.IsEmpty() && strStatement.GetLength() >= 6)
		{
			strSelectKey = strStatement.Left (6);
			if (strSelectKey.CompareNoCase (_T("select")) == 0)
				bSelectStmt = TRUE;
		}

		if (bSelectStmt) 
		{
			if (nExec == QEP)
			{
				pData = (CaSqlQueryQepData*)new CaSqlQueryQepData();
				pStatementInfo = new CaSqlQueryStatement ((LPCTSTR)strStatement, (LPCTSTR)m_pDoc->m_strDatabase, TRUE);
				pStatementInfo->SetStatementPosition (nStart, nEnd);
				pData->m_strStatement = strStatement;
				pData->m_pQueryStatement = pStatementInfo;
				m_listResultPage.AddTail (pData);
			}
			else
			{
				pData = (CaSqlQuerySelectData*)new CaSqlQuerySelectData();
				pStatementInfo = new CaSqlQueryStatement ((LPCTSTR)strStatement, (LPCTSTR)m_pDoc->m_strDatabase, TRUE);
				pStatementInfo->SetStatementPosition (nStart, nEnd);
				pData->m_strStatement = strStatement;
				pData->m_pQueryStatement = pStatementInfo;
				m_listResultPage.AddTail (pData);
			}
		}
		else
		{
			//
			// Non-select statement 'strStatement':
			if (nExec == QEP || nExec == XML)
			{
				//
				// SPEC ON 06-JAN-2000:
				// -------------------
				// Do not execute the non-selected statements in QEP mode:
				BOOL bExecNonSelect = FALSE;
				if (!bShowMsg)
				{
					if (nExec == QEP)
					{
						//
						// MSG=No QEP will be displayed for the non-select statements.
						AfxMessageBox (IDS_MSG_NONSELECT_IN_QEP, MB_ICONEXCLAMATION|MB_OK);
					}
					else
					{
						//
						// MSG=No XML will be displayed for the non-select statements.
						AfxMessageBox (IDS_MSG_NONSELECT_IN_XML, MB_ICONEXCLAMATION|MB_OK);
					}
					bShowMsg = TRUE;
				}
				//
				// Execute like a normal statement for non-qep:
				if (bExecNonSelect)
				{
					pData = (CaSqlQueryNonSelectData*)new CaSqlQueryNonSelectData();
					pData->m_pPage = NULL;
					pData->m_strStatement = strStatement;
					pStatementInfo = new CaSqlQueryStatement ((LPCTSTR)strStatement, (LPCTSTR)m_pDoc->m_strDatabase, FALSE);
					pStatementInfo->SetStatementPosition (nStart, nEnd);
					pData->m_pQueryStatement = pStatementInfo;
					m_listResultPage.AddTail (pData);
				}
			}
			else
			{
				pData = (CaSqlQueryNonSelectData*)new CaSqlQueryNonSelectData();
				pData->m_pPage = NULL;
				pData->m_strStatement = strStatement;
				pStatementInfo = new CaSqlQueryStatement ((LPCTSTR)strStatement, (LPCTSTR)m_pDoc->m_strDatabase, FALSE);
				pStatementInfo->SetStatementPosition (nStart, nEnd);
				pData->m_pQueryStatement = pStatementInfo;
				m_listResultPage.AddTail (pData);
			}
		}
		nStart = nEnd;
		nStart+= _tclen((const TCHAR *)strRawStatement + nStart);
	}
	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CfSqlQueryFrame

IMPLEMENT_DYNCREATE(CfSqlQueryFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CfSqlQueryFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CfSqlQueryFrame)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_CLOSE()
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



CuDlgSqlQueryResult* CfSqlQueryFrame::GetDlgSqlQueryResult()
{
	ASSERT (m_bAllViewCreated);
	if (!m_bAllViewCreated)
		return NULL;
	CvSqlQueryLowerView* pView = (CvSqlQueryLowerView*)m_wndNestSplitter.GetPane(1, 0);
	ASSERT (pView);
	if (!pView)
		return NULL;
	return pView->GetDlgSqlQueryResult();
}


CuDlgSqlQueryStatement* CfSqlQueryFrame::GetDlgSqlQueryStatement()
{
	ASSERT (m_bAllViewCreated);
	if (!m_bAllViewCreated)
		return NULL;
	CvSqlQueryUpperView* pView = (CvSqlQueryUpperView*)m_wndSplitter.GetPane(0, 0);
	ASSERT (pView);
	if (!pView)
		return NULL;
	return pView->GetDlgSqlQueryStatement();
}

CvSqlQueryRichEditView* CfSqlQueryFrame::GetRichEditView()
{
	ASSERT (m_bAllViewCreated);
	if (!m_bAllViewCreated)
		return NULL;
	CvSqlQueryRichEditView* pView = (CvSqlQueryRichEditView*)m_wndNestSplitter.GetPane(0, 0);
	ASSERT (pView);
	if (!pView)
		return NULL;
	return pView;
}


/////////////////////////////////////////////////////////////////////////////
// CfSqlQueryFrame construction/destruction
void CfSqlQueryFrame::Initialize()
{
	m_bAllViewCreated =  FALSE;
	m_pDoc = NULL;
	m_bUseCursor = TRUE;
	m_bTraceIsUp = FALSE;
	m_bEned = FALSE;
}

CfSqlQueryFrame::CfSqlQueryFrame()
{
	Initialize();
}

CfSqlQueryFrame::CfSqlQueryFrame(CdSqlQueryRichEditDoc* pDoc)
{
	Initialize();
	m_pDoc = pDoc;
}

CfSqlQueryFrame::~CfSqlQueryFrame()
{
}

BOOL CfSqlQueryFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	return CFrameWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CfSqlQueryFrame diagnostics

#ifdef _DEBUG
void CfSqlQueryFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}
void CfSqlQueryFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}
#endif //_DEBUG



int CfSqlQueryFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	CvSqlQueryRichEditView* pRichEditView = GetRichEditView();
	if (!m_pDoc)
		m_pDoc = (CdSqlQueryRichEditDoc*)pRichEditView->GetDocument();
	CaSqlQueryProperty& property = m_pDoc->GetProperty();
	if (property.IsTraceActivated())
	{
		CuDlgSqlQueryResult* pDlgResult  = GetDlgSqlQueryResult();
		ASSERT(pDlgResult);
		if (pDlgResult)
			pDlgResult->DisplayRawPage (TRUE);
	}

	return 0;
}

BOOL CfSqlQueryFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
	//
	// Information View (CvSqlQueryUpperView)
	// It contains a dialog box, which itsel contains the static control
	// to display the information about compile time, execute time, ...
	// 
	// ********************************************* <-- Main Splitter
	//
	// Sql Editor View (CvSqlQueryRichEditView)
	//
	// ********************************************* <-- Nested Splitter 
	//
	// Query Result View    (CvSqlQueryLowerView)
	//
	
	//
	// Create a main splitter of 2 rows and 1 column.
	//
	CCreateContext context1, context2, context3;
	CRuntimeClass* pView1 = RUNTIME_CLASS (CvSqlQueryUpperView);
	CRuntimeClass* pView2 = RUNTIME_CLASS (CvSqlQueryRichEditView);
	CRuntimeClass* pView3 = RUNTIME_CLASS (CvSqlQueryLowerView);
	context1.m_pNewViewClass = pView1;
	context1.m_pCurrentDoc   = m_pDoc? m_pDoc: new CdSqlQueryRichEditDoc();
	context2.m_pNewViewClass = pView2;
	context2.m_pCurrentDoc   = context1.m_pCurrentDoc;
	context3.m_pNewViewClass = pView3;
	context3.m_pCurrentDoc   = context1.m_pCurrentDoc;
	//
	// Information View (CvSqlQueryUpperView)
	// It contains a dialog box, which itsel contains the static control
	// to display the information about compile time, execute time, ...
	// 
	// ********************************************* <-- Main Splitter
	//
	// Sql Editor View (CvSqlQueryRichEditView)
	//
	// ********************************************* <-- Nested Splitter 
	//
	// Query Result View    (CvSqlQueryLowerView)
	//
	
	//
	// Create a main splitter of 2 rows and 1 column.
	//
	if (!m_wndSplitter.CreateStatic (this, 2, 1))
	{
		TRACE0 ("CfSqlQueryFrame::OnCreateClient: Failed to create Splitter\n");
		return FALSE;
	}
	//
	// Create a nested splitter which has 2 rows and one column.
	// It is child of the second pane of the main splitter.
	//
	BOOL b = m_wndNestSplitter.CreateStatic (
		&m_wndSplitter,     // Parent is the main splitter.
		2,                  // Two rows.
		1,                  // One column.
		WS_CHILD|WS_VISIBLE|WS_BORDER,  m_wndSplitter.IdFromRowCol (1, 0));
	if (!b)
	{
		TRACE0 ("CfSqlQueryFrame::OnCreateClient: Failed to create Nested Splitter\n");
		return FALSE;
	}
	//
	// Add the first splitter pane - the view (CvSqlQueryUpperView) in line 0
	// of the main splitter.
	//
	if (!m_wndSplitter.CreateView (0, 0, context1.m_pNewViewClass, CSize (120, 30), &context1))
	{
		TRACE0 ("CfSqlQueryFrame::OnCreateClient: Failed to create first pane\n");
		return FALSE;
	}
	//
	// Add the first splitter pane of the Nested Splitter -
	// the Rich Edit View (the default view, CvQuerySqlRichEditView) in Row 0
	//
	if (!m_wndNestSplitter.CreateView (0, 0, context2.m_pNewViewClass, CSize (130, 100), &context2))
	{
		TRACE0 ("CfSqlQueryFrame::OnCreateClient: Failed to create first pane\n");
		return FALSE;
	}

	//
	// Add the second splitter pane of the Nested Splitter - 
	// the Query Result View (CvSqlQueryLowerView) in Row 1
	//
	if (!m_wndNestSplitter.CreateView (1, 0, context3.m_pNewViewClass, CSize (130, 100), &context3))
	{
		TRACE0 ("CfQueryFrame::OnCreateClient: Failed to create second pane\n");
		return FALSE;
	}
	
	CvSqlQueryUpperView*    pUpperView = (CvSqlQueryUpperView*)m_wndSplitter.GetPane(0, 0);
	CuDlgSqlQueryStatement* pUpperDlg  = pUpperView->GetDlgSqlQueryStatement();
	CRect r;
	pUpperDlg->GetWindowRect (r);

	m_wndNestSplitter.SetRowInfo    (0, 60,    10);
	m_wndNestSplitter.SetRowInfo    (1, 400,   10);
	m_wndSplitter.SetTracking (FALSE);
	m_wndSplitter.SetRowInfo        (0, r.Height(),   10);
	m_wndSplitter.SetRowInfo        (1, 400,   10);
	m_wndSplitter.RecalcLayout();
	m_wndNestSplitter.RecalcLayout();
	m_bAllViewCreated = TRUE;
	SetActiveView ((CView*)m_wndNestSplitter.GetPane(0, 0));

	return TRUE;
}


long CfSqlQueryFrame::ConnectIfNeeded(BOOL bDisplayMessage)
{
	CaSession* pCurrentSession = NULL;
	try
	{
		if (!m_pDoc)
			m_pDoc = (CdSqlQueryRichEditDoc*)GetActiveDocument();
		CaSqlQueryProperty& property = m_pDoc->GetProperty();
		m_bUseCursor = (property.GetSelectMode() == 1)? TRUE: FALSE;
		CuDlgSqlQueryResult* pDlgResult  = GetDlgSqlQueryResult();
		if (pDlgResult)
		{
			pDlgResult->CloseCursor();
			pDlgResult->DisplayPageHeader(); // Old page, its header is visible.
		}

		//
		// Check if we need the new session:
		CaSessionManager& sessionMgr = theApp.GetSessionManager();
		CaSession session;
		session.SetFlag (m_pDoc->GetDatabaseFlag());
		session.SetIndependent(TRUE);
		session.SetNode(m_pDoc->GetNode());
		session.SetDatabase(m_pDoc->GetDatabase());

		session.SetServerClass(m_pDoc->GetServerClass());
		session.SetUser(m_pDoc->GetUser());
		session.SetOptions(m_pDoc->GetConnectionOption());
		session.SetDescription(sessionMgr.GetDescription());
		pCurrentSession = m_pDoc->GetCurrentSession();
		SETLOCKMODE lockmode;
		memset (&lockmode, 0, sizeof(lockmode));
		lockmode.nReadLock = property.IsReadLock()? LM_SHARED: LM_NOLOCK;
		lockmode.nTimeOut  = property.GetTimeout();
		if (!pCurrentSession)
		{
			//
			// This is the first connection:
			pCurrentSession = sessionMgr.GetSession(&session);
			m_pDoc->SetCurrentSession (pCurrentSession);
			if (pCurrentSession)
			{
				pCurrentSession->Commit();
				pCurrentSession->SetAutoCommit(property.IsAutoCommit());
				pCurrentSession->SetLockMode(&lockmode);
				pCurrentSession->Commit();
			}
		}
		else
		{
			//
			// The old connection existed. Do we need the new connection or
			// use the old one ?
			if (session == *pCurrentSession)
			{
				//
				// Use the old session.
				pCurrentSession->Activate();
			}
			else
			{
				//
				// Must connect the new session one. The old session is automatically committed
				pCurrentSession->Release (SESSION_COMMIT);
				//
				// Create the new session:
				pCurrentSession = sessionMgr.GetSession(&session);
				m_pDoc->SetCurrentSession (pCurrentSession);
				if (pCurrentSession)
				{
					pCurrentSession->Commit();
					pCurrentSession->SetAutoCommit(property.IsAutoCommit());
					pCurrentSession->SetLockMode(&lockmode);
					pCurrentSession->Commit();
				}
			}
		}

		return 0;
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
		return -1;
	}
	catch (CeSqlQueryException e)
	{
		CString strText = e.GetReason();
		if (bDisplayMessage && !strText.IsEmpty())
			AfxMessageBox (strText, MB_ICONEXCLAMATION|MB_OK);
		return e.GetErrorCode();
	}
	catch (CeSqlException e)
	{
		if (bDisplayMessage)
			AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
		return e.GetErrorCode();
	}
	catch (...)
	{
		if (bDisplayMessage)
			AfxMessageBox (_T("Internal error: CfSqlQueryFrame::ConnectIfNeeded()"), MB_ICONEXCLAMATION|MB_OK);
		return -1;
	}
	return -1;
}



BOOL CfSqlQueryFrame::Execute (int nExec)
{
	BOOL bAllExecuteOK = FALSE;
	CaSession* pCurrentSession = NULL;
	try
	{
		long nsCompile = 0;     // Compile time (ms)
		CString strText;        // Raw statement from the edit box
		int iret = -1;
		BOOL bExecuteOK = FALSE;
		BOOL bStatInfo  = TRUE; // Exec, Cost, and Elapse are available.
		CaSqlQueryData* pData = NULL;
		CWaitCursor waitCursor;
		CvSqlQueryRichEditView* pRichEditView = GetRichEditView();
		CuDlgSqlQueryStatement* pDlgStatement = GetDlgSqlQueryStatement();
		CuDlgSqlQueryResult* pDlgResult       = GetDlgSqlQueryResult();
		CaSqlQueryStatement* pStatementInfo = NULL;
		
		ASSERT (pRichEditView && pDlgStatement && pDlgResult);
		if (!(pRichEditView || pDlgStatement || pDlgResult))
			return FALSE;

		if (!m_pDoc)
			m_pDoc = (CdSqlQueryRichEditDoc*)pRichEditView->GetDocument();
		CaSqlQueryProperty& property = m_pDoc->GetProperty();
		m_bUseCursor = (property.GetSelectMode() == 1)? TRUE: FALSE;
	
		CaSqlQueryMeasureData& measureData = m_pDoc->GetMeasureData();
		m_pDoc->SetStatementExecuted(TRUE);

		//
		// Every page of the result is considered to be old even the statement remains
		// unchanged. So when we select Tab (that old page) we do not need to highlight the
		// statement in the Rich Edit control.
		pDlgResult->CloseCursor();
		pDlgResult->DisplayPageHeader(); // Old page, its header is visible.
		
		//
		// Check if we need the new session:
		long lResult = ConnectIfNeeded();
		if (lResult != 0)
			throw (long)lResult;
		pCurrentSession = m_pDoc->GetCurrentSession();
		ASSERT(pCurrentSession);
		if (!pCurrentSession)
			return FALSE;
		int nSessionVersion = pCurrentSession->GetVersion();
		m_pDoc->SetConnectParamInfo(nSessionVersion);
		if (nSessionVersion == INGRESVERS_NOTOI)
		{
			//
			// If the TRACE output is being used, then remove the Trace Tab:
			if (m_pDoc->IsUsedTracePage())
			{
				CuDlgSqlQueryResult* pDlgResult  = GetDlgSqlQueryResult();
				ASSERT(pDlgResult);
				if (pDlgResult)
					pDlgResult->DisplayRawPage (FALSE);
			}

			measureData.SetGateway (TRUE);
			if (nExec == QEP)
			{
				AfxMessageBox (IDS_MSG_QEPUNAVAIABLE);
				return FALSE;
			}
		}

		//
		// Ensure that the list of statement is cleaned:
		while (!m_listResultPage.IsEmpty())
			delete m_listResultPage.RemoveHead();

		pRichEditView->GetWindowText (strText);
		//
		// ParseStatement: Re-construct the list m_listResultPage where
		// the elements are (CaSqlQueryQepData*, CaSqlQuerySelectData*, CaSqlQueryNonSelectData*)
		ParseStatement (nExec, strText);

		POSITION pos = m_listResultPage.GetHeadPosition();
		if (!m_pDoc->IsMultipleSelectAllowed() && nExec != QEP)
		{
			//
			// Check if multiple select statements:
			int nSelectCount = 0;
			while (pos != NULL)
			{
				pData = m_listResultPage.GetNext (pos);
				if (pData && pData->m_pQueryStatement && pData->m_pQueryStatement->m_bSelect)
					nSelectCount++;
			}
			if (nSelectCount > 1)
			{
				//
				// Has bugs and multiple select statements, do not allow execution:
				// MSG ="Execution of multiple select statements together is not available against this version of DBMS Server"
				AfxMessageBox (IDS_MSG_MULTI_STATEMENT, MB_ICONEXCLAMATION|MB_OK);
				while (!m_listResultPage.IsEmpty())
					delete m_listResultPage.RemoveHead();
				return FALSE;
			}
		}
		if (m_pDoc->GetProperty().IsAutoCommit() && m_listResultPage.GetCount() > 1 && nExec != QEP)
		{
			//
			// Autocommit on and multiple statements, do not allow execution if there is a select statement:
			int nSelectCount = 0;
			pos = m_listResultPage.GetHeadPosition();
			while (pos != NULL)
			{
				pData = m_listResultPage.GetNext (pos);
				if (pData && pData->m_pQueryStatement && pData->m_pQueryStatement->m_bSelect)
					nSelectCount++;
			}
			if (nSelectCount > 0)
			{
				//
				// MSG = "Execution of multiple statements including at least one select statement is not available when autocommit is on.\n"
				//       "Please execute the statements separately, or set autocommit off";
				AfxMessageBox (IDS_MSG_STATEMENT_COMMIT, MB_ICONEXCLAMATION|MB_OK);
				while (!m_listResultPage.IsEmpty())
					delete m_listResultPage.RemoveHead();
				return FALSE;
			}
		}

		//
		// Set the previous mode: set qep, set noqep, set optimyonly, ...

		// if it is a gateway, these states should not have been changed
		// since no compile statistics have been calculated
		// and executing the statements would result in an SQL error
		BOOL b2QueryStat = TRUE;
		if (!measureData.IsGateway())
			b2QueryStat = SetPreviousMode ();
		//
		// Get the compile time information:
		measureData.Initialize();
		measureData.SetAvailable(FALSE);
		if (nExec != QEP)
		{
			nsCompile = -1;
			if (b2QueryStat)
			{
				measureData.StartMeasure (TRUE);
				//
				// Exec in an OPTIMIZEONLY for all the statements:
				if (!measureData.IsGateway())
				{
					nsCompile = CompileSqlStatement (pCurrentSession->GetVersion());
				}
			}
			else
				measureData.SetAvailable(FALSE);

			measureData.SetCompileTime (nsCompile);
		}
		pDlgStatement->DisplayStatistic (&measureData);

		//
		// Execute the statements:
		pos = m_listResultPage.GetHeadPosition();
		while (pos != NULL)
		{
			pData = m_listResultPage.GetNext (pos);
			if (pData && pData->m_pQueryStatement && pData->m_pQueryStatement->m_bSelect)
			{
				//
				// Execute select statement:
				if (nExec == QEP)
					bExecuteOK = QEPExcuteSelectStatement ((CaSqlQueryQepData*)pData);
				else
				if (nExec == RUN)
					bExecuteOK = ExcuteSelectStatement ((CaSqlQuerySelectData*)pData, bStatInfo, pCurrentSession->GetVersion());
				else
					bExecuteOK = ExcuteGenerateXML ((CaSqlQuerySelectData*)pData, bStatInfo);

				if (!bExecuteOK)
				{
					//
					// If the bExecuteOK = FALSE then we may encountered an abnormal error.
					// If any error causing by the execution of the statement then the exception is raiseed ...
					AfxMessageBox (IDS_MSG_UNKNOWN_FAIL_2_EXECUTE_STATEMENT, MB_ICONEXCLAMATION|MB_OK);
					throw CeSqlQueryException(_T("")); // Interupt execution;
				}
			}
			else
			if (pData && pData->m_pQueryStatement && !pData->m_pQueryStatement->m_bSelect)
			{
				if (pCurrentSession)
					pCurrentSession->SetCommitted(FALSE);
				//
				// Execute a non-select statement 'pData->m_strStatement':
				// If the statement is 'commit' then set all the previous opencursors as broken cursors:
				// The elapse time become available.
				BOOL bActionCommit = FALSE;
				if (pData->m_strStatement.CompareNoCase(_T("commit")) == 0)
				{
					bStatInfo = TRUE;
					pDlgResult->CloseCursor();
					bActionCommit = TRUE;
				}

				bExecuteOK = ExcuteNonSelectStatement ((CaSqlQueryNonSelectData*)pData);
				if (!bExecuteOK)
				{
					//
					// If the bExecuteOK = FALSE then we may encountered an abnormal error.
					// If any error causing by the execution of the statement then the exception
					// is raiseed ...
					AfxMessageBox (IDS_MSG_UNKNOWN_FAIL_2_EXECUTE_STATEMENT, MB_ICONEXCLAMATION|MB_OK);
					throw CeSqlQueryException(_T("")); // Interupt execution;
				}
				if (bActionCommit && pCurrentSession)
					pCurrentSession->SetCommitted(TRUE);
			}
		}
		//
		// IF the statistic is available, display it (not available for the QEP)
		if (nExec != QEP)
		{
			CaSession* pActiveSession = m_pDoc->GetCurrentSession();
			if (b2QueryStat)
			{
				if (bStatInfo)
				{
					pActiveSession->Activate();
					//
					// TRUE: it means that the ellapsed time is available:
					measureData.StopMeasure (TRUE);
					pActiveSession->SetSessionNone();
				}
				else
				{
					//
					// Only the compile time is still available:
					measureData.StopEllapse();
					measureData.SetAvailable (FALSE);
				}
			}
		}
		pDlgStatement->DisplayStatistic (&measureData);

		//
		// Display the pages (they already contain the data):
		while (!m_listResultPage.IsEmpty())
		{
			pData = m_listResultPage.RemoveHead();
			if (pData->m_pPage)
				pDlgResult->DisplayPage ((CDialog *)pData->m_pPage);
			if (m_listResultPage.IsEmpty())
			{
				//
				// Pick up the last statement to be highlighted.
				pStatementInfo = pData->m_pQueryStatement;
				pData->m_pQueryStatement = NULL;
			}
			delete pData;
		}
		//
		// Here pStatementInfo (if exist) is the last statement has been executed.
		if (pStatementInfo)
		{
			//
			// Hightlight the last statement executed
			// and select the page in the Tab Control.
			CWnd* pCurrentPage = pDlgResult->GetCurrentPage();
			if (pCurrentPage && pCurrentPage->SendMessage (WM_SQL_TAB_BITMAP, 0, 0) != BM_TRACE)
			{
				pCurrentPage->SendMessage (WM_SQL_QUERY_PAGE_HIGHLIGHT);
				pDlgResult->m_cTab1.SetCurSel (0);
				pDlgResult->UpdatePage(0);
			}
			delete pStatementInfo;
		}

		pDlgResult->SelectRawPage();
		pDlgResult->UpdateTabBitmaps();
		bAllExecuteOK = TRUE;
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch (CeSqlQueryException e)
	{
		CString strText = e.GetReason();
		if (!strText.IsEmpty())
			AfxMessageBox (strText, MB_ICONEXCLAMATION|MB_OK);
		
		while (!m_listResultPage.IsEmpty())
			delete m_listResultPage.RemoveHead();
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
		while (!m_listResultPage.IsEmpty())
			delete m_listResultPage.RemoveHead();
	}

	try
	{
		if (pCurrentSession)
			m_pDoc->UpdateAutocommit(pCurrentSession->GetAutoCommit());
	}
	catch(...)
	{

	}

	return bAllExecuteOK;
}


static BOOL GetInterfacePointer(IUnknown* pObj, REFIID riid, PVOID* ppv)
{
	BOOL bResult = FALSE;
	HRESULT hError;

	*ppv=NULL;

	if (NULL != pObj)
	{
		hError = pObj->QueryInterface(riid, ppv);
	}
	return FAILED(hError)? FALSE: TRUE;
}





void CfSqlQueryFrame::OnClose() 
{
	if (m_pDoc && !m_bEned)
	{
		m_bEned = TRUE;
		//
		// Do not want the default message "Save changes to Sqltest1?"
		m_pDoc->SetModifiedFlag (FALSE); 
		CaSession* pCurrentSession = m_pDoc->GetCurrentSession ();
		if (pCurrentSession)
			pCurrentSession->Release (SESSION_COMMIT); // Commit the current session
	}
	CFrameWnd::OnClose();
}

BOOL CfSqlQueryFrame::QEPExcuteSelectStatement (CaSqlQueryQepData* pData)
{
	CuDlgSqlQueryResult* pDlgResult = GetDlgSqlQueryResult();
	pData->m_pQepDoc = new CdQueryExecutionPlanDoc();
	pData->m_pQepDoc->m_bAutoDelete = FALSE;
	pData->m_bDeleteQepDoc = TRUE;
	BOOL bOK = QEPParse (m_pDoc, pData->m_strStatement, TRUE, pData->m_pQepDoc);
	if (!bOK)
		return FALSE;
	if (!m_pDoc)
		return FALSE;
	pData->m_bDeleteQepDoc = FALSE;
	POSITION pos = pData->m_pQepDoc->m_listQepData.GetHeadPosition();
	CaSqlQueryExecutionPlanData* pQepData = NULL;
	while (pos != NULL)
	{
		pQepData = pData->m_pQepDoc->m_listQepData.GetNext(pos);
		pQepData->SetDisplayMode ((m_pDoc->GetProperty().GetQepDisplayMode() == 1));
	}
	//
	// Request for creating the page for the "QEP":
	pData->m_pPage = (CWnd*)(LRESULT)pDlgResult->SendMessage (WM_SQL_STATEMENT_EXECUTE, (WPARAM)1, (LPARAM)pData);

	return TRUE;
}


BOOL CfSqlQueryFrame::QEPExcuteNonSelectStatement (CaSqlQueryQepData* pData)
{
	ASSERT (FALSE);
	//
	// You must call the member 'ExcuteNonSelectStatement' as a normal
	// non-select statement.
	return FALSE;
}

//
// This function only prepares for execution only.
BOOL CfSqlQueryFrame::ExcuteSelectStatement (CaSqlQuerySelectData* pData, BOOL& bStatInfo, int nIngresVersion)
{
	CString strMsgBoxTitle;
	CString strInterruptFetching;
	CString strFetchInfo;
	strMsgBoxTitle.LoadString(AFX_IDS_APP_TITLE);
	strInterruptFetching.LoadString(IDS_MSG_INTERRUPT_FETCHING);
	strFetchInfo.LoadString (IDS_ROWS_FETCHED);

	bStatInfo = TRUE;
	//
	// Use the new code: cursor
	CuDlgSqlQueryResult* pDlgResult  = GetDlgSqlQueryResult();
	if (!m_pDoc)
		m_pDoc = (CdSqlQueryRichEditDoc*)GetActiveDocument();
	CaSqlQueryProperty& property = m_pDoc->GetProperty();

	BOOL bUseCursor = m_bUseCursor;
	pData->m_pCursor = NULL;
	if (bUseCursor) 
	{
		pData->m_pCursor = new CaCursor (theApp.GetCursorSequence(), pData->m_strStatement, nIngresVersion);
	}
	else
	{
		//
		// In reality, if the setting indicates that we should fetch all rows at once, previously we used the select loop.
		// BUT, if the select statement contains the unicode constant N'xyz', then it failed to work with the select loop.
		// NOW: the resolution is to keep the stuff of unicode constant work around !
		//      That is, if the select statement contains the unicode constant N'xyz', then we force to use the CURSOR
		//      because the implementation of the cursor has the unicode constant N'xyz' work around stuff !.
		if (nIngresVersion >= INGRESVERS_26 && HasUniConstInWhereClause((LPTSTR)(LPCTSTR)pData->m_strStatement, NULL))
		{
			pData->m_pCursor = new CaCursor (theApp.GetCursorSequence(), pData->m_strStatement, nIngresVersion);
			bUseCursor = TRUE;
		}
	}

	//
	// Request to create the page for the "SELECT" Result:
	pData->m_pPage = (CWnd*)(LRESULT)pDlgResult->SendMessage (WM_SQL_STATEMENT_EXECUTE, (WPARAM)0, (LPARAM)pData);

	//
	// When we have got the page...:
	if (pData->m_pPage)
	{
		int nRowLimint = (property.GetSelectMode() == 1)? property.GetSelectBlock(): 0;
		CaSqlTrace aTrace;
		if (bUseCursor && pData->m_pCursor)
		{
			CaCursorInfo& fetchInfo = pData->m_pCursor->GetCursorInfo();
			TCHAR tchszF4Exp = property.IsF4Exponential()? _T('e'): _T('n');
			TCHAR tchszF8Exp = property.IsF8Exponential()? _T('e'): _T('n');
			fetchInfo.SetFloat4Format(property.GetF4Width(), property.GetF4Decimal(), tchszF4Exp);
			fetchInfo.SetFloat8Format(property.GetF8Width(), property.GetF8Decimal(), tchszF8Exp);
		}

		//
		// Create  Query Row Param, and initialize its parameters
		CaExecParamQueryRows* pQueryRowParam = new CaExecParamQueryRows(bUseCursor);
		pQueryRowParam->SetSession (m_pDoc->GetCurrentSession());
		pQueryRowParam->SetShowRawPage (m_pDoc->IsUsedTracePage());
		pQueryRowParam->SetRowLimit (nRowLimint);
		pQueryRowParam->SetStrings(strMsgBoxTitle, strInterruptFetching, strFetchInfo);
		pQueryRowParam->SetStatement(pData->m_strStatement);
		pQueryRowParam->SetCursor (pData->m_pCursor);
		CaFloatFormat& fetchInfo = pQueryRowParam->GetFloatFormat();
		TCHAR tchszF4Exp = property.IsF4Exponential()? _T('e'): _T('n');
		TCHAR tchszF8Exp = property.IsF8Exponential()? _T('e'): _T('n');
		fetchInfo.SetFloat4Format(property.GetF4Width(), property.GetF4Decimal(), tchszF4Exp);
		fetchInfo.SetFloat8Format(property.GetF8Width(), property.GetF8Decimal(), tchszF8Exp);

		pData->m_pCursor = NULL;
		CuDlgSqlQueryPageResult* pRowPage = (CuDlgSqlQueryPageResult*)pData->m_pPage;
		pRowPage->SetQueryRowParam(pQueryRowParam);

		if (m_pDoc->IsUsedTracePage())
			aTrace.Start();

		CString strCallerID = _T("");
		// 
		// Construct list headers:
		pQueryRowParam->ConstructListHeader();
		strCallerID = pQueryRowParam->GetCursorName();

		if (m_pDoc->IsUsedTracePage())
		{
			aTrace.Stop();
			pDlgResult->DisplayTraceLine (pData->m_strStatement, NULL, strCallerID);
			CString& strTraceBuffer = aTrace.GetTraceBuffer();
			RCTOOL_CR20x0D0x0A(strTraceBuffer);
			if (!strTraceBuffer.IsEmpty())
				pDlgResult->DisplayTraceLine (NULL, strTraceBuffer, strCallerID);
		}

		//
		// Page that contains the Result of the select statement
		if (pData->m_pPage->SendMessage (WM_SQL_STATEMENT_SHOWRESULT, 0, (LPARAM)pData) != 0)
		{
			//
			// The Cursor is still opened and available:
			bStatInfo = FALSE;
		}
	}
	return TRUE;
}

//
// This function only prepares for execution only.
BOOL CfSqlQueryFrame::ExcuteGenerateXML (CaSqlQuerySelectData* pData, BOOL& bStatInfo)
{
	CString strMsgBoxTitle;
	CString strInterruptFetching;
	CString strFetchInfo;
	strMsgBoxTitle.LoadString(AFX_IDS_APP_TITLE);
	strInterruptFetching.LoadString(IDS_MSG_INTERRUPT_FETCHING);
	strFetchInfo.LoadString (IDS_ROWS_FETCHED);

	bStatInfo = TRUE;
	//
	// Use the select loop instead of cursor:
	pData->m_pCursor = NULL;
	CuDlgSqlQueryResult* pDlgResult  = GetDlgSqlQueryResult();
	if (!m_pDoc)
		m_pDoc = (CdSqlQueryRichEditDoc*)GetActiveDocument();
	CaSqlQueryProperty& property = m_pDoc->GetProperty();

	//
	// Request to create the page for the "XML":
	pData->m_pPage = (CWnd*)(LRESULT)pDlgResult->SendMessage (WM_SQL_STATEMENT_EXECUTE, (WPARAM)2, (LPARAM)pData);
	if (pData->m_pPage)
	{
		CaSqlTrace aTrace;
		//
		// Create  Query Row XML Param, and initialize its parameters
		CuDlgSqlQueryPageXML* pRowPage = (CuDlgSqlQueryPageXML*)pData->m_pPage;
		CaGenerateXmlFile* pQueryRowParam = pRowPage->GetQueryRowParam();
		pQueryRowParam->SetStatement(pData->m_strStatement);
		CaFloatFormat& fetchInfo = pQueryRowParam->GetFloatFormat();
		TCHAR tchszF4Exp = property.IsF4Exponential()? _T('e'): _T('n');
		TCHAR tchszF8Exp = property.IsF8Exponential()? _T('e'): _T('n');
		fetchInfo.SetFloat4Format(property.GetF4Width(), property.GetF4Decimal(), tchszF4Exp);
		fetchInfo.SetFloat8Format(property.GetF8Width(), property.GetF8Decimal(), tchszF8Exp);
		pQueryRowParam->SetSession (m_pDoc->GetCurrentSession());
		pQueryRowParam->SetShowRawPage (m_pDoc->IsUsedTracePage());
		pQueryRowParam->SetStrings(strMsgBoxTitle, strInterruptFetching, strFetchInfo);

		if (m_pDoc->IsUsedTracePage())
			aTrace.Start();

		//
		// Initialize list of headers:
		pQueryRowParam->ConstructListHeader();

		if (m_pDoc->IsUsedTracePage())
		{
			aTrace.Stop();
			pDlgResult->DisplayTraceLine (pData->m_strStatement, NULL, _T(""));
			CString& strTraceBuffer = aTrace.GetTraceBuffer();
			RCTOOL_CR20x0D0x0A(strTraceBuffer);
			if (!strTraceBuffer.IsEmpty())
				pDlgResult->DisplayTraceLine (NULL, strTraceBuffer, _T(""));
		}

		//
		// Page that contains the Result of the select statement
		if (pData->m_pPage->SendMessage (WM_SQL_STATEMENT_SHOWRESULT, 0, (LPARAM)pData) != 0)
		{
			//
			// The Cursor is still opened and available:
			bStatInfo = FALSE;
		}
	}
	return TRUE;
}

BOOL CfSqlQueryFrame::ExcuteNonSelectStatement (CaSqlQueryNonSelectData* pData)
{
	CuDlgSqlQueryResult* pDlgResult = GetDlgSqlQueryResult();
	ASSERT (m_pDoc && pDlgResult);
	if (!(m_pDoc || pDlgResult))
		return FALSE;

	//
	// Parse the Set qep, set optimize only ....
	CString strRaw = pData->m_strStatement;
	int nLen = strRaw.GetLength();
	if (nLen > 3)
	{
		CString strSet = strRaw.Left (3);
		if (strSet.CompareNoCase (_T("set")) == 0)
		{
			int i =3;
			while (i < nLen && strRaw.GetAt(i) == _T(' '))
				i++;
			CString strValue = strRaw.Mid (i);
			if (strValue.CompareNoCase (_T("optimizeonly")) == 0)
				m_pDoc->m_bSetOptimizeOnly = TRUE;
			else
			if (strValue.CompareNoCase (_T("nooptimizeonly")) == 0)
				m_pDoc->m_bSetOptimizeOnly = FALSE;
			else
			if (strValue.CompareNoCase (_T("qep")) == 0)
				m_pDoc->m_bSetQep = TRUE;
			else
			if (strValue.CompareNoCase (_T("noqep")) == 0)
				m_pDoc->m_bSetQep = FALSE;
		}
	}
	CaSqlTrace aTrace;
	if (m_pDoc->IsUsedTracePage())
		aTrace.Start();
	CaLLAddAlterDrop execStatement (m_pDoc->GetNode(), m_pDoc->GetDatabase(), pData->m_strStatement);
	execStatement.SetUser(m_pDoc->GetUser());
	execStatement.SetServerClass(m_pDoc->GetServerClass());
	execStatement.SetOptions(m_pDoc->GetConnectionOption());
	execStatement.SetCommitInfo(FALSE);
	CaSession* pCurrentSession = m_pDoc->GetCurrentSession();
	if (pCurrentSession)
		pCurrentSession->Activate();

	BOOL bINSERTxUPDATExDELETE = FALSE;
	LPTSTR p1= (LPTSTR)(LPCTSTR)pData->m_strStatement;
	LPTSTR stmtInsert = _T("insert");
	LPTSTR stmtUpdate = _T("update");
	LPTSTR stmtDelete = _T("delete");
	while (*p1 == _T(' ') || *p1== 0x0D || *p1== 0x0A || *p1==_T('\t'))
		p1++; //checked DBCS
	if (_tcsnicmp(p1,stmtInsert,_tcslen(stmtInsert))==0 ||
	    _tcsnicmp(p1,stmtUpdate,_tcslen(stmtUpdate))==0 ||
	    _tcsnicmp(p1,stmtDelete,_tcslen(stmtDelete))==0)
		bINSERTxUPDATExDELETE = TRUE;

	pData->m_nAffectedRows = -1;
	INGRESII_llExecuteImmediate (&execStatement, NULL);
	if (bINSERTxUPDATExDELETE && execStatement.GetAffectedRows() != -1)
		pData->m_nAffectedRows = execStatement.GetAffectedRows();

	//
	// At this point, the statement has been run but not committed
	if (m_pDoc->IsUsedTracePage())
	{
		aTrace.Stop();
		//
		// Always display the statement in the trace window:
		pDlgResult->DisplayTraceLine (pData->m_strStatement, NULL);
		CString& strTraceBuffer = aTrace.GetTraceBuffer();
		RCTOOL_CR20x0D0x0A(strTraceBuffer);
		if (!strTraceBuffer.IsEmpty())
			pDlgResult->DisplayTraceLine (NULL, strTraceBuffer);

		if (pData->m_nAffectedRows != -1)
		{
			CString strLineCount;
			strLineCount.Format (IDS_F_ROWS, pData->m_nAffectedRows);//_T("<%d row(s)>")
			pDlgResult->DisplayTraceLine (NULL, strLineCount);
		}
	}

	if (m_pDoc->GetProperty().IsShowNonSelect())
	{
		//
		// Create the page for the non-select statement (normal):
		pData->m_pPage = (CWnd*)(LRESULT)pDlgResult->SendMessage (WM_SQL_STATEMENT_EXECUTE, (WPARAM)0, (LPARAM)pData);
	}
	else
		pData->m_pPage = NULL;

	return TRUE;
}



long CfSqlQueryFrame::CompileSqlStatement (int nIngresVersion)
{
	CStringList listStatement;
	CaSqlQueryData* pData = NULL;

	POSITION pos = m_listResultPage.GetHeadPosition();
	while (pos != NULL)
	{
		pData = m_listResultPage.GetNext (pos);
		BOOL ok = FALSE;
		if (pData && pData->m_pQueryStatement && pData->m_pQueryStatement->m_bSelect)
		{
			listStatement.AddTail(pData->m_strStatement);
		}
	}

	return INGRESII_CompileStatement(listStatement, nIngresVersion);
}


BOOL CfSqlQueryFrame::SetPreviousMode()
{
	TCHAR tchszOptimizeonly[]   = _T("set optimizeonly"); 
	TCHAR tchszNoOptimizeonly[] = _T("set nooptimizeonly");
	TCHAR tchszQEP[]            = _T("set qep"); 
	TCHAR tchszNoQEP[]          = _T("set noqep"); 
	if (!m_pDoc)
		return FALSE;

	try
	{
		CaLLAddAlterDrop execStatement (m_pDoc->GetNode(), m_pDoc->GetDatabase(), _T(""));
		execStatement.SetUser(m_pDoc->GetUser());
		execStatement.SetServerClass(m_pDoc->GetServerClass());
		execStatement.SetOptions(m_pDoc->GetConnectionOption());
		execStatement.SetCommitInfo(FALSE);
		CaSession* pCurrentSession = m_pDoc->GetCurrentSession();
		if (pCurrentSession)
			pCurrentSession->Activate();
		//
		// The following executions must not be committed.
		if (m_pDoc->IsSetQep())
		{
			execStatement.SetStatement (tchszQEP);
			INGRESII_llExecuteImmediate (&execStatement, NULL);
		}
		else
		{
			execStatement.SetStatement (tchszNoQEP);
			INGRESII_llExecuteImmediate (&execStatement, NULL);
		}
		if (m_pDoc->IsSetOptimizeOnly())
		{
			execStatement.SetStatement (tchszOptimizeonly);
			INGRESII_llExecuteImmediate (&execStatement, NULL);
		}
		else
		{
			execStatement.SetStatement (tchszNoOptimizeonly);
			INGRESII_llExecuteImmediate (&execStatement, NULL);
		}
		return TRUE;
	}
	catch (CeSqlException e)
	{
		if (e.GetErrorCode() == -34000L) // No previleges
		{
			TRACE1("CfSqlQueryFrame::SetPreviousMode:\n\t%s\n", e.GetReason());
			return FALSE;
		}
	}
	catch (...)
	{

	}
	return FALSE;
}



void CfSqlQueryFrame::OnDestroy() 
{
	if (m_pDoc && !m_bEned)
	{
		m_bEned = TRUE;
		//
		// Do not want the default message "Save changes to Sqltest1?"
		m_pDoc->SetModifiedFlag (FALSE); 
		CaSession* pCurrentSession = m_pDoc->GetCurrentSession ();
		if (pCurrentSession)
			pCurrentSession->Release (SESSION_COMMIT); // Commit the current session
	}
	CFrameWnd::OnDestroy();
}



BOOL CfSqlQueryFrame::ShouldButtonsBeEnable()
{
	BOOL bEnable = FALSE;
	CvSqlQueryRichEditView* pView = GetRichEditView();
	ASSERT (pView);
	if (!pView)
		return bEnable;
	bEnable = (pView->GetTextLength() == 0)? FALSE: TRUE;
	return bEnable;
}



BOOL CfSqlQueryFrame::IsRunEnable()
{
	return ShouldButtonsBeEnable();
}

BOOL CfSqlQueryFrame::IsQepEnable()
{
	CvSqlQueryRichEditView* pView = GetRichEditView();
	if (!pView)
		return FALSE;
	if (!m_pDoc)
		m_pDoc = (CdSqlQueryRichEditDoc*)pView->GetDocument();
	if (m_pDoc && m_pDoc->GetConnectParamInfo() == INGRESVERS_NOTOI)
		return FALSE;
	return ShouldButtonsBeEnable();
}

BOOL CfSqlQueryFrame::IsXmlEnable()
{
	return ShouldButtonsBeEnable();
}


BOOL CfSqlQueryFrame::IsClearEnable()
{
	return ShouldButtonsBeEnable();
}

BOOL CfSqlQueryFrame::IsTraceEnable()
{
	BOOL bEnable = TRUE;
	CvSqlQueryRichEditView* pView = GetRichEditView();
	if (!pView)
		return FALSE;
	if (!m_pDoc)
		m_pDoc = (CdSqlQueryRichEditDoc*)pView->GetDocument();
	if (m_pDoc && m_pDoc->GetConnectParamInfo() == INGRESVERS_NOTOI)
		bEnable = FALSE;
	return bEnable;
}

BOOL CfSqlQueryFrame::IsSaveAsEnable()
{
	return ShouldButtonsBeEnable();
}

BOOL CfSqlQueryFrame::IsUsedTracePage()
{
	return m_pDoc->IsUsedTracePage();
}



void CfSqlQueryFrame::Initiate(LPCTSTR lpszNode, LPCTSTR lpszServer, LPCTSTR lpszUser, LPCTSTR lpszOption)
{
	CvSqlQueryRichEditView* pView = GetRichEditView();
	ASSERT (pView);
	if (!pView)
		return;
	if (!m_pDoc)
		m_pDoc = (CdSqlQueryRichEditDoc*)pView->GetDocument();
	if (m_pDoc)
	{
		m_pDoc->SetNode(lpszNode);
		m_pDoc->SetServerClass(lpszServer);
		m_pDoc->SetUser(lpszUser);
		if (lpszOption)
			m_pDoc->SetConnectionOption(lpszOption);
		else
			m_pDoc->SetConnectionOption(_T(""));
	}
}

void CfSqlQueryFrame::SetDatabase(LPCTSTR lpszDatabase, long nFlag)
{
	CvSqlQueryRichEditView* pView = GetRichEditView();
	ASSERT (pView);
	if (!pView)
		return;
	if (!m_pDoc)
		m_pDoc = (CdSqlQueryRichEditDoc*)pView->GetDocument();
	if (m_pDoc)
		m_pDoc->SetDatabase(lpszDatabase, nFlag);
}

void CfSqlQueryFrame::SetErrorFileName(LPCTSTR lpszFileName)
{
	CvSqlQueryRichEditView* pView = GetRichEditView();
	ASSERT (pView);
	if (!pView)
		return;
	if (!m_pDoc)
		m_pDoc = (CdSqlQueryRichEditDoc*)pView->GetDocument();
	if (m_pDoc)
		m_pDoc->SetErrorFileName(lpszFileName);
}

void CfSqlQueryFrame::SqlWizard()
{
	USES_CONVERSION;
	CString strMsg;
	IUnknown*   pAptSqlAssistant = NULL;
	ISqlAssistant* pSqlAssistant;
	HRESULT hError = NOERROR;

	try
	{
		CvSqlQueryRichEditView* pView = GetRichEditView();
		ASSERT (pView);
		if (!pView)
			return;
		pView->SetFocus();
		if (!m_pDoc)
		{
			m_pDoc = (CdSqlQueryRichEditDoc*)pView->GetDocument();
			if (!m_pDoc)
				return;
		}
		CString strDatabase = m_pDoc->GetDatabase();
		if (strDatabase.IsEmpty() || strDatabase.CompareNoCase(_T("(none)")) == 0)
		{
			//
			// Please select a database.
			AfxMessageBox (IDS_MSG_SELECT_DATABASE, MB_ICONEXCLAMATION|MB_OK);
			return;
		}

		hError = CoCreateInstance(
			CLSID_SQLxASSISTANCT,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IUnknown,
			(PVOID*)&pAptSqlAssistant);

		if (SUCCEEDED(hError))
		{
			CString strSessionDescription;
			CString strUser = m_pDoc->GetUser();
			CString strOption = m_pDoc->GetConnectionOption();
			strSessionDescription.Format(_T("%s / %s"), (LPCTSTR)theApp.GetSessionManager().GetDescription(), _T("Sql Assistant"));
			BOOL bOk = GetInterfacePointer(pAptSqlAssistant, IID_ISqlAssistant, (LPVOID*)&pSqlAssistant);
			if (bOk)
			{
				BSTR bstrStatement = NULL;
				SQLASSISTANTSTRUCT iiparam;
				memset(&iiparam, 0, sizeof(iiparam));
				wcscpy (iiparam.wczNode, T2W((LPTSTR)(LPCTSTR)m_pDoc->GetNode()));
				wcscpy (iiparam.wczDatabase, T2W((LPTSTR)(LPCTSTR)strDatabase));
				wcscpy (iiparam.wczSessionDescription, T2W((LPTSTR)(LPCTSTR)strSessionDescription));
				if (!strUser.IsEmpty())
					wcscpy (iiparam.wczUser, T2W((LPTSTR)(LPCTSTR)strUser));
				if (!strOption.IsEmpty())
					wcscpy (iiparam.wczConnectionOption, T2W((LPTSTR)(LPCTSTR)strOption));
				iiparam.nDbFlag = m_pDoc->GetDatabaseFlag();
				iiparam.nSessionStart = theApp.GetSessionManager().GetSessionStart() + theApp.GetSqlAssistantSessionStart();
				iiparam.nActivation   = 0;

				hError = pSqlAssistant->SqlAssistant (m_hWnd, &iiparam, &bstrStatement);
				if (SUCCEEDED(hError) && bstrStatement)
				{
					CString strStatement = W2T(bstrStatement);
					SysFreeString(bstrStatement);
					strStatement.TrimLeft();
					strStatement.TrimRight();
					if (!strStatement.IsEmpty())
					{
						strStatement += _T(";");
						CRichEditCtrl& redit = pView->GetRichEditCtrl();
						redit.ReplaceSel(strStatement, TRUE);
					}
				}
				pSqlAssistant->Release();
			}
			else
			{
				//
				// Failed to query interface of SQL Assistant
				AfxMessageBox (IDS_MSG_FAIL_2_QUERYINTERFACE_SQLASSISTANT);
			}
			
			pAptSqlAssistant->Release();
			CoFreeUnusedLibraries();
		}
		else
		{
			//
			// Failed to create an SQL Assistant COM Object
			AfxMessageBox (IDS_MSG_FAIL_2_CREATE_SQLASSISTANT);
		}
	}
	catch(...)
	{

	}
}

void CfSqlQueryFrame::Clear() 
{
	CvSqlQueryRichEditView* pView = GetRichEditView();
	ASSERT (pView);
	if (!pView)
		return;
	pView->SetWindowText (_T(""));
}

//
//  Queries for a file name containing a sql statement,
//  and loads the statement into the edit control
void CfSqlQueryFrame::Open() 
{
	try
	{
		CString strFullName;
		CString strFilter;
		CString strFilterItem;
		CvSqlQueryRichEditView* pView = GetRichEditView();
		ASSERT (pView);
		if (!pView)
			return;
		pView->SetFocus();

		if (!strFilterItem.LoadString (IDS_FILTER_SQL_FILES))
			strFilterItem = _T("SQL Query Files (*.qry;*.sql)|*.qry;*.sql");
		strFilter = strFilterItem;
		if (!strFilterItem.LoadString (IDS_FILTER_ALL_FILES))
			strFilterItem = _T("All Files (*.*)|*.*");
		strFilter += _T("|");
		strFilter += strFilterItem;
		strFilter += _T("||");

		CFileDialog dlg(
			TRUE,
			NULL,
			NULL,
			OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
			(LPCTSTR)strFilter);

		if (dlg.DoModal() != IDOK)
			return; 

		strFullName = dlg.GetPathName ();
		CString strExt = _T("");
		int nDot = strFullName.ReverseFind(_T('.'));
		if (nDot != -1)
		{
			strExt = strFullName.Mid(nDot+1);
		}

		if (!strExt.IsEmpty() && strExt.CompareNoCase (_T("qry")) == 0)
		{
			//
			// Read as a private format .qry:
			int nQueryLen;
			LPTSTR lpszBuffer = NULL;
			CFile f(strFullName, CFile::modeRead|CFile::typeBinary);
			//
			// Step 1: size of the query string
			f.Read((void *)&nQueryLen, sizeof(nQueryLen));
		
			//
			// step 2: query string
			lpszBuffer = new TCHAR[nQueryLen+1];
			UINT nRead = f.Read((void *)lpszBuffer, nQueryLen);
			if ((int)nRead != nQueryLen) 
			{
				// Cannot Load SQL Query : File Problem:
				AfxMessageBox(IDS_MSG_FAIL_2_OPENQUERY, MB_ICONEXCLAMATION|MB_OK);
				return;
			}
			lpszBuffer[nQueryLen] = _T('\0');
			pView->SetWindowText ((LPCTSTR)lpszBuffer);
			delete lpszBuffer;
			f.Close();
		}
		else
		{
			//
			// Read as a text file:
			UINT nByte = 0;
			TCHAR tchszBuffer [512];
			CFile fTextFile (strFullName, CFile::modeRead);
			CString strStatement = _T("");

			while ((nByte = fTextFile.Read (tchszBuffer, sizeof(tchszBuffer)-1)) > 0)
			{
				tchszBuffer [nByte] = '\0';
				strStatement += tchszBuffer;
			}
			fTextFile.Close();
			//
			// Replace by ' ' of the following occurences: '\g'; '\q'.
			BOOL bAsk = TRUE;
			BOOL bAskError = TRUE;
			BOOL bReplace = FALSE;
			int iG = 0,  nLen = strStatement.GetLength();
			while (iG != -1 && iG <nLen)
			{
				int itype, nlenkw;
				iG = Find (strStatement, _T('\\'), (iG>0)? (iG+1): iG, FALSE);
				if (iG != -1 && (iG +1) < nLen && IsSpecialSQLSequence (strStatement.Mid(iG+1),&itype,&nlenkw))
				{
					if (itype == SQL_KW_USE_SEMICOLUMN ||itype==SQL_KW_IGNORE_ACCEPTED) {
						if (bAsk) {
							//
							// MSG=The file contains commands such as '\\g', '\\q','\\s','\\shell', etc..\nDo you want to remove them?
							if (AfxMessageBox (IDS_MSG_REMOVE_COMMAND1, MB_ICONQUESTION|MB_YESNO) == IDYES)
								bReplace = TRUE;
							bAsk = FALSE;
						}
					}
					if (itype == SQL_KW_NOT_ACCEPTED) {
						if (bAskError) {
							//
							// MSG = The file contains commands such as '\\a', '\\append','\\w','\\write', etc..\n
							//       These commands will not work, but you may want to change them manually.
							AfxMessageBox (IDS_MSG_REMOVE_COMMAND2, MB_ICONSTOP|MB_OK);
							bAskError = FALSE;
						}
					}
					while (nlenkw+1) { // +1 because the value returned by IsSpecialSQLSequence doesn't include the \ character
						if (bReplace && (itype == SQL_KW_USE_SEMICOLUMN ||itype==SQL_KW_IGNORE_ACCEPTED)
						   ) {
							if (itype==SQL_KW_USE_SEMICOLUMN && nlenkw==0)
								strStatement.SetAt (iG, _T(';'));
							else
								strStatement.SetAt (iG,   _T(' '));
						}
						iG ++;
						nlenkw--;
					}
				}
			}
			pView->SetWindowText (strStatement);
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage ();
		e->Delete();
	}
	catch (CFileException* e)
	{
		e->ReportError();
		e->Delete();
	}
	catch (...)
	{
		AfxMessageBox(IDS_MSG_UNKNOWN_FAIL_2_OPENQUERY, MB_ICONEXCLAMATION|MB_OK);
		TRACE0 ("CfSqlQueryFrame::OnButtonQueryOpen-> Unknown error\n");
	}
}

void CfSqlQueryFrame::Saveas() 
{
	try
	{
		CString strFullName;
		CString strFilter;
		CString strFilterItem;
		CvSqlQueryRichEditView* pView = GetRichEditView();
		ASSERT (pView);
		if (!pView)
			return;
		pView->SetFocus();

		if (!strFilterItem.LoadString (IDS_FILTER_SQL_FILES1))
			strFilterItem = _T("SQL Query Files (*.qry)|*.qry");
		strFilter = strFilterItem;
		strFilter += _T("|");
		if (!strFilterItem.LoadString (IDS_FILTER_SQL_FILES2))
			strFilterItem = _T("SQL Query Files (*.sql)|*.sql");
		strFilter += strFilterItem;
		strFilter += _T("|");
		if (!strFilterItem.LoadString (IDS_FILTER_ALL_FILES))
			strFilterItem = _T("All Files (*.*)|*.*");
		strFilter += strFilterItem;
		strFilter += _T("||");

		CFileDialog dlg(
			FALSE,
			NULL,
			NULL,
			OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
			strFilter);

		if (dlg.DoModal() != IDOK)
			return; 
		strFullName = dlg.GetPathName ();
		CString strExt = _T("");
		int nDot = strFullName.ReverseFind(_T('.'));
		if (nDot != -1)
		{
			strExt = strFullName.Mid(nDot+1);
		}
		else
		{
			switch (dlg.m_ofn.nFilterIndex)
			{
			case 1: // *.qry
				strExt = _T("qry");
				strFullName += _T(".qry");
				break;
			case 2:
				//
				// Force the file extention to be .sql
				strFullName += _T(".sql");
				break;
			default:
				break;
			}
		}

		if (!strExt.IsEmpty() && strExt.CompareNoCase (_T("qry")) == 0)
		{
			//
			// Save as a private format .qry:
			int nQueryLen;
			CFile f(strFullName, CFile::modeCreate|CFile::modeWrite|CFile::typeBinary);
			//
			// Step 1: size of the query string
			CString strStatement;
			pView->GetWindowText (strStatement);
			nQueryLen = strStatement.GetLength();
			f.Write((void *)&nQueryLen, sizeof(nQueryLen));

			//
			// step 2: query string
			f.Write((void *)(LPCTSTR)strStatement, nQueryLen);
			f.Close();
		}
		else
		{
			//
			// Write File as text format only:
			CString strStatement;
			CFile fTextFile (strFullName, CFile::modeCreate | CFile::modeWrite);
			pView->GetWindowText (strStatement);
			fTextFile.Write(strStatement, strStatement.GetLength());
			fTextFile.Close();
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage ();
		e->Delete();
	}
	catch (CFileException* e)
	{
		e->ReportError();
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("CfSqlQueryFrame::OnButtonQuerySaveas-> Unknown error\n");
		AfxMessageBox(IDS_MSG_UNKNOWN_FAIL_2_SAVEQUERY, MB_ICONEXCLAMATION|MB_OK);
	}
}

void CfSqlQueryFrame::TracePage()
{
	try
	{
		CuDlgSqlQueryResult* pDlgResult = GetDlgSqlQueryResult();
		ASSERT (pDlgResult);
		if (!pDlgResult)
			return;
		if (m_pDoc->IsUsedTracePage())
			m_bTraceIsUp = FALSE;
		else
			m_bTraceIsUp = TRUE;
		pDlgResult->DisplayRawPage (m_bTraceIsUp);
	}
	catch (...)
	{
		//
		// Failed to create the Raw Tab.
		AfxMessageBox (IDS_MSG_FAIL_2_CREATE_PAGERAW, MB_ICONEXCLAMATION|MB_OK);
	}
}

BOOL CfSqlQueryFrame::IsPrintEnable()
{
	BOOL bEnable = FALSE;
	CuDlgSqlQueryResult* pResultDlg = GetDlgSqlQueryResult();
	if (pResultDlg)
	{
		int nBmpID = 0;
		CWnd* pCurrentPageResult = pResultDlg->GetCurrentPage();
		if (pCurrentPageResult && IsWindow (pCurrentPageResult->m_hWnd))
			nBmpID = pCurrentPageResult->SendMessage (WM_SQL_TAB_BITMAP, 0, 0);
		switch (nBmpID)
		{
		case BM_SELECT_BROKEN:
		case BM_SELECT_OPEN:
		case BM_SELECT:
		case BM_SELECT_CLOSE:
			bEnable = TRUE;
			break;
		default:
			break;
		}
	}
	return bEnable;
}

void CfSqlQueryFrame::DoPrint() 
{
	ASSERT (m_bAllViewCreated);
	if (!m_bAllViewCreated)
		return;
	CvSqlQueryLowerView* pLowerView = (CvSqlQueryLowerView*)m_wndNestSplitter.GetPane(1, 0);
	ASSERT (pLowerView);
	if (pLowerView) 
	{
		QueryPageDataType pgType = QUERYPAGETYPE_NONE;
		CaQueryPageData* pPageData = pLowerView->GetDlgSqlQueryResult()->GetCurrentPageData();
		if (pPageData) {
			pgType = pPageData->GetQueryPageType();
			switch (pgType) 
			{
				case QUERYPAGETYPE_SELECT:
				case QUERYPAGETYPE_NONSELECT:
				case QUERYPAGETYPE_RAW:
					pLowerView->DoFilePrint();  // Since OnFilePrint is protected
					break;
				case QUERYPAGETYPE_QEP:
					{
						AfxMessageBox (_T("Query Execution Plan cannot be printed with this version."));
					}
					break;
			}
			delete pPageData;
		}
	}
}

void CfSqlQueryFrame::DoPrintPreview()
{
	ASSERT (m_bAllViewCreated);
	if (!m_bAllViewCreated)
		return;
	CvSqlQueryLowerView* pLowerView = (CvSqlQueryLowerView*)m_wndNestSplitter.GetPane(1, 0);
	ASSERT (pLowerView);
	if (pLowerView) 
	{
		pLowerView->DoFilePrintPreview();
	}
}


void CfSqlQueryFrame::GetData (IStream** ppStream)
{
	if (!m_pDoc)
		m_pDoc = (CdSqlQueryRichEditDoc*)GetActiveDocument();
	ASSERT (m_pDoc);
	if (m_pDoc)
	{
		BOOL bOk = CObjectToIStream (m_pDoc, ppStream);
		if (!bOk)
		{
			*ppStream = NULL;
		}
	}
}

void CfSqlQueryFrame::SetData (IStream* pStream)
{
	if (!m_pDoc)
		m_pDoc = (CdSqlQueryRichEditDoc*)GetActiveDocument();
	ASSERT (m_pDoc);
	if (m_pDoc)
	{
		Cleanup();
		BOOL bOk = CObjectFromIStream(m_pDoc, pStream);
		if (bOk)
		{
			m_pDoc->UpdateAllViews(NULL, (LPARAM)UPDATE_HINT_LOAD, m_pDoc);
		}
	}
}


void CfSqlQueryFrame::Cleanup()
{
	if (!m_pDoc)
		m_pDoc = (CdSqlQueryRichEditDoc*)GetActiveDocument();
	CuDlgSqlQueryResult* pDlgResult = GetDlgSqlQueryResult();
	ASSERT (pDlgResult);
	if (!pDlgResult)
		return;
	pDlgResult->Cleanup();
}

void CfSqlQueryFrame::SetIniFleName(LPCTSTR lpszFileIni)
{
	if (!m_pDoc)
		m_pDoc = (CdSqlQueryRichEditDoc*)GetActiveDocument();
	ASSERT(m_pDoc);
	if (m_pDoc)
		m_pDoc->SetIniFleName(lpszFileIni);

}

void CfSqlQueryFrame::SetSessionStart(long nStart)
{
	CaSessionManager& sessMgr = theApp.GetSessionManager();
	int nCurrStart = sessMgr.GetSessionStart();
	if (nCurrStart == 1) 
	{
		sessMgr.SetSessionStart(nStart);
	}
}

void CfSqlQueryFrame::InvalidateCursor()
{
	try
	{
		CWaitCursor waitCursor;
		CuDlgSqlQueryResult* pDlgResult = GetDlgSqlQueryResult();
		//
		// Every page of the result is considered to be old even the statement remains
		// unchanged. So when we select Tab (that old page) we do not need to highlight the
		// statement in the Rich Edit control.
		pDlgResult->CloseCursor();
		pDlgResult->DisplayPageHeader(); // Old page, its header is visible.
	}
	catch(...)
	{

	}
}

void CfSqlQueryFrame::Commit()
{
	if (!m_pDoc)
		m_pDoc = (CdSqlQueryRichEditDoc*)GetActiveDocument();
	CaSession* pCurrentSession = m_pDoc->GetCurrentSession();
	if (pCurrentSession)
	{
		pCurrentSession->Commit();
	}
}

void CfSqlQueryFrame::Rollback()
{
	if (!m_pDoc)
		m_pDoc = (CdSqlQueryRichEditDoc*)GetActiveDocument();
	CaSession* pCurrentSession = m_pDoc->GetCurrentSession();
	if (pCurrentSession)
	{
		pCurrentSession->Rollback();
	}
}

BOOL CfSqlQueryFrame::IsCommittable()
{
	if (!m_pDoc)
		m_pDoc = (CdSqlQueryRichEditDoc*)GetActiveDocument();
	if (!m_pDoc)
		return FALSE;
	CaSession* pCurrentSession = m_pDoc->GetCurrentSession();
	if (!pCurrentSession)
		return FALSE;
	return !pCurrentSession->IsCommitted();
}

short CfSqlQueryFrame::GetReadLock()
{
	if (!m_pDoc)
		m_pDoc = (CdSqlQueryRichEditDoc*)GetActiveDocument();
	CaSqlQueryProperty& property = m_pDoc->GetProperty();
	return property.IsReadLock() ? 1: 0;
}


void CfSqlQueryFrame::SetSessionOption(UINT nMask, CaSqlQueryProperty* pProperty)
{
	try
	{
		if (!m_pDoc)
			m_pDoc = (CdSqlQueryRichEditDoc*)GetActiveDocument();
		if (!m_pDoc)
			return;
		CaSession* pCurrentSession = m_pDoc->GetCurrentSession();
		if (pCurrentSession)
		{
			SETLOCKMODE lockmode;
			memset (&lockmode, 0, sizeof(lockmode));
			lockmode.nReadLock = pProperty->IsReadLock()? LM_SHARED: LM_NOLOCK;
			if (nMask & SQLMASK_TIMEOUT)
				lockmode.nTimeOut  = pProperty->GetTimeout();
			pCurrentSession->Activate();
			if (nMask & SQLMASK_AUTOCOMMIT)
				pCurrentSession->SetAutoCommit(pProperty->IsAutoCommit());
			if ((nMask & SQLMASK_READLOCK) || (nMask & SQLMASK_TIMEOUT))
				pCurrentSession->SetLockMode(&lockmode);
		}
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch(...)
	{

	}

	try
	{
		if (!m_pDoc)
			m_pDoc = (CdSqlQueryRichEditDoc*)GetActiveDocument();
		if (!m_pDoc)
			return;
		CaSession* pCurrentSession = m_pDoc->GetCurrentSession();
		if (pCurrentSession)
			m_pDoc->UpdateAutocommit(pCurrentSession->GetAutoCommit());
	}
	catch(...)
	{

	}
}

BOOL CfSqlQueryFrame::GetTransactionState(short nWhat)
{
	if (!m_pDoc)
		m_pDoc = (CdSqlQueryRichEditDoc*)GetActiveDocument();
	switch (nWhat)
	{
	case 1: // autocommit 
		// 
		// [m_pDoc->IsAutoCommit() may differ from m_pDoc->GetProperty().IsAutocommit()] because
		// there is an inconsistence when set autocommit failed, the property page marks the autocommit
		// property as checked but the session fails to set autocommit !
		// m_pDoc->IsAutoCommit() will return the session autocommit state.= select dbmsinfo('autocommit_state')
		return m_pDoc->IsAutoCommit();
	case 2: // commit
		{
			CaSession* pSession = m_pDoc->GetCurrentSession();
			if (pSession)
				return pSession->IsCommitted();
			return TRUE;
		}
	case 3: // readlock
		// Not implemented (how to get read lock state from DBMS Session ?
		return m_pDoc->GetProperty().IsReadLock();
	default:
		ASSERT(FALSE);
		return FALSE;
	}
}



BOOL CfSqlQueryFrame::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	APP_HelpInfo (m_hWnd, 8003); // Old sql test in vdba 2.6 (main.c (1747))
	return TRUE;
}

BOOL APP_HelpInfo(HWND hWnd, UINT nHelpID)
{
	int  nPos = 0;
	CString strHelpFilePath;
	CString strTemp = theApp.m_pszHelpFilePath;
#if defined(MAINWIN)
	nPos = strTemp.ReverseFind( '/'); 
#else
	nPos = strTemp.ReverseFind( '\\'); 
#endif
	if (nPos != -1) 
	{
		strHelpFilePath = strTemp.Left( nPos +1 );
		strHelpFilePath += theApp.m_strHelpFile;
	}

	TRACE1 ("APP_HelpInfo, Help Context ID = %d\n", nHelpID);
	if (nHelpID == 0)
		HtmlHelp(hWnd, strHelpFilePath, HH_DISPLAY_TOPIC, NULL);
	else
		HtmlHelp(hWnd, strHelpFilePath, HH_HELP_CONTEXT, nHelpID);

	return TRUE;
}
