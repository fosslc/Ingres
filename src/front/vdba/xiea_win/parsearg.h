/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : parsearg.h , header File
**    Project  : Ingres II / IEA
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Parse the arguments
**
** History:
**
** 16-Oct-2001 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
** 26-Feb-2003 (uk$so01)
**    SIR  #106952, Enhance library + command line
** 22-Aug-2003 (uk$so01)
**    SIR #106648, Add silent mode in IEA. 
**/

#if !defined (PARSEARG_HEADER)
#define PARSEARG_HEADER
#include "cmdargs.h"

class CaCommandLine: public CaArgumentLine
{
public:
	CaCommandLine();
	~CaCommandLine(){}
	BOOL HandleCommandLine();
	BOOL HasCmdLine(){return m_bParam;}
	
	BOOL    m_bLoop;
	CString m_strNode;
	CString m_strServerClass;
	CString m_strDatabase;
	CString m_strStatement;
	CString m_strExportFile;
	CString m_strExportMode;
	CString m_strIIParamFile;

	BOOL    m_bBatch;      // Silent mode
	CString m_strListFile; // only if m_bBatch = TRUE
	CString m_strLogFile;  // only if m_bBatch = TRUE
	BOOL    m_bOverWrite;  // only if m_bBatch = TRUE
protected:
	BOOL m_bParam;

};



#endif // PARSEARG_HEADER