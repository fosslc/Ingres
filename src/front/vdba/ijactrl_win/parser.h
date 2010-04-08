/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
** 
**    Source   : parser.h , Header File 
**    Project  : Ingres Journal Analyser
**    Purpose  : misc classes or functions for the auditdb parsing
**
** History:
**
** 04-Jan-2000 (uk$so01)
**    Created
** 15-Mar-2000 (noifr01)
**    created the ScanOutputLines class
**/


#if !defined(PARSER_HEADER)
#define PARSER_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "logadata.h"

#define MAXCONCURRENTSCANS 4

typedef enum 
{
	SKIP_NONE = 0, 
	SKIP_4_AUDITDBPARSE
} SkipLinesType;

class ScanOutputLines: public CObject
{
public:
	ScanOutputLines(CString csString ,SkipLinesType SkipLinesT=SKIP_NONE);
	~ScanOutputLines() {m_csFullString.ReleaseBuffer();}
	LPTSTR GetFirstTraceLine(int i);
	LPTSTR GetFirstTraceLineAfter(int i, int iini); // scan with cursor i, starting first line after current of curosr iini
	LPTSTR GetNextTraceLine(int i);
	
protected:
	CString m_csFullString;
	int m_istrlen;
	LPTSTR m_stringstart;
	int m_icurpos[MAXCONCURRENTSCANS];
	SkipLinesType m_SkipLinesType;
};


BOOL IJA_DatabaseParse4Transactions (
    CString& strAuditdbOutput,
    CTypedPtrList < CObList, CaDatabaseTransactionItemData* >& listTransaction);

BOOL Get2LongsFromHexString(LPCTSTR lpstring, unsigned long&l1, unsigned long&l2);

unsigned long _ttoul( LPCTSTR lpstring);

#endif // PARSER_HEADER