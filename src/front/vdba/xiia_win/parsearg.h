/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : parsearg.h , header File
**    Project  : Ingres II / IIA
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Parse the arguments
**
** History:
**
** 29-Mar-2001 (uk$so01)
**    Created
** 26-Feb-2003 (uk$so01)
**    SIR  #106952, Enhance library + command line
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
	void SetNoParam(){m_bParam = FALSE;}

	BOOL    m_bLoop;
	CString m_strNode;
	CString m_strServerClass;
	CString m_strDatabase;
	CString m_strTable;
	CString m_strTableOwner;

	CString m_strIIImportFile;
	CString m_strLogFile;
	CString m_strNewTable;
	BOOL    m_bBatch;
	BOOL    m_bNewTable;
protected:
	BOOL m_bParam;
};


#endif // PARSEARG_HEADER