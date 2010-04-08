/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqltrace.h, Header File 
**    Project  : Visual DBA.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : SQL Trace functionality.
**
** History:
**
** 23-Oct-2001 (uk$so01)
**    Created
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
**    Rewrite SQL Trace to be independent as library.
**    Initially, the code is writtem by francois noirot-nerin in vdba/parse.cpp
*/

#if !defined (SQL_TRACE_FILE_HEADER)
#define SQL_TRACE_FILE_HEADER

class CaSqlTrace: public CObject
{
public:
	CaSqlTrace();
	~CaSqlTrace();
	
	void Start();
	void Stop();
	TCHAR* GetFirstTraceLine(void);  // returns NULL if no more data
	TCHAR* GetNextTraceLine(void);   // returns NULL if no more data
	TCHAR* GetNextSignificantTraceLine();

	CString& GetTraceBuffer() {return m_strTraceBuffer;}
	BOOL GetTraceResult(){return bTraceResult;}
protected:
	CString m_strTraceBuffer;
	TCHAR* TLCurPtr;
	TCHAR TRACELINESEP;
	BOOL bTraceResult;
	BOOL bTraceLinesInit;
	BOOL bStarted;


	void ReleaseTraceLines();

};

#endif // SQL_TRACE_FILE_HEADER
