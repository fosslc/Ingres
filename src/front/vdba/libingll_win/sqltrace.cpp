/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**  Source   : parse.h, Header File 
**  Project  : Visual DBA.
**  Author   : UK Sotheavut (uk$so01)
**  Purpose  : Function prototype for accessing low level data 
**             (Implemented by francois noirot-nerin)
**
**  History:
**	24-Jul-2001 (uk$so01)
**	    Created. Split the old code from VDBA (mainmfc\parse.cpp)
**	23-Oct-2001 (uk$so01)
**	    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process
**	    COM Server)
**	20-nov-2001 (somsa01)
**	    The definition of TRACELINESEP should not be 'extern "C"'
**	20-Aug-2008 (whiro01)
**	    Remove redundant <afx...> includes (already in stdafx.h)
*/

#include "stdafx.h"
#include "sqltrace.h"
extern "C"
{
#include <compat.h>
#include <cm.h>
#include <st.h>
}


static char TRACELINESEP = '\n';
extern "C" int IILQsthSvrTraceHdlr(VOID (*rtn)(void* ptr, char* buf, int i),void* cb, bool active, bool no_dbterm);
extern "C" VOID fnSqlTraceCallback(void* p1, char* outbuf, int i)
{
	CaSqlTrace* pTrace = (CaSqlTrace*)p1;
	ASSERT (pTrace);
	if (!pTrace)
		return;
	CString& strTraceBuffer = pTrace->GetTraceBuffer();
	strTraceBuffer +=outbuf;
	return;
}

static TCHAR * _tstrgetlastcharptr(TCHAR* pstring)
{
	int ilen = _tcslen(pstring);
	int ipos = 0;
	while (ipos<ilen) {
		int cb = CMbytecnt(pstring+ipos);
		if (ipos+cb>=ilen) 
			return (pstring+ipos);
		ipos+=cb;
	}
	return NULL; /* case where ilen==0 returns here */
}


static BOOL suppspace(TCHAR* pu)
{
	USES_CONVERSION;
	int iprevlen = STlength(T2A(pu));
	int inewlen  = STtrmwhite(T2A(pu));
	if  (inewlen<iprevlen)
		return TRUE;
	else
		return FALSE;
}

//
// CLASS: CaSqlTrace
// ************************************************************************************************
CaSqlTrace::CaSqlTrace()
{
	m_strTraceBuffer = _T("");
	TRACELINESEP = _T('\n');
	bTraceResult = TRUE;
	bTraceLinesInit = FALSE;
	bStarted = FALSE;
}

CaSqlTrace::~CaSqlTrace()
{
	if (bStarted)
		Stop();
}


void CaSqlTrace::Start()
{
	m_strTraceBuffer = _T("");
	bStarted = TRUE;
	IILQsthSvrTraceHdlr(fnSqlTraceCallback,(void *)this, TRUE, FALSE);
}

void CaSqlTrace::Stop()
{
	bStarted = FALSE;
	IILQsthSvrTraceHdlr(fnSqlTraceCallback,(void *)this, FALSE, FALSE);
}


TCHAR* CaSqlTrace::GetFirstTraceLine()
{
	TLCurPtr = m_strTraceBuffer.GetBuffer(0);
	if (TLCurPtr[0]==_T('\0'))
		TLCurPtr=NULL;
	bTraceLinesInit=TRUE;
	return GetNextTraceLine();
}


static TCHAR * TLCurPtr;
TCHAR* CaSqlTrace::GetNextTraceLine()
{
	TCHAR * plastchar;
	if (!TLCurPtr) {
		ReleaseTraceLines();
		return (TCHAR *)NULL;
	}
	TCHAR * presult = TLCurPtr;
	TCHAR * pc = _tcschr(TLCurPtr,TRACELINESEP);
	if (pc) {
		*pc=_T('\0');
		TLCurPtr=pc+1;
		if (TLCurPtr[0]==_T('\0'))
			TLCurPtr=NULL;

	}
	else
		TLCurPtr=NULL;
	while ( (plastchar = _tstrgetlastcharptr(presult))!=NULL ) {
		TCHAR c = *plastchar;
		if (c==_T('\n') || c==_T('\r'))
			*plastchar ='\0';
		else
			break;
	}
	suppspace(presult);
	return presult;
}

TCHAR* CaSqlTrace::GetNextSignificantTraceLine()
{
	TCHAR* presult;
	while ((presult= GetNextTraceLine())!=(TCHAR *) NULL) {
		if (*presult!= _T('\0') && _tcsncmp(presult, _T("----"),4))
			return presult;
	}
	return (TCHAR *)NULL;
}

void CaSqlTrace::ReleaseTraceLines()
{
	if (!bTraceLinesInit)
		return;
	m_strTraceBuffer.ReleaseBuffer();
	m_strTraceBuffer=_T("");
	bTraceLinesInit=FALSE;
}
