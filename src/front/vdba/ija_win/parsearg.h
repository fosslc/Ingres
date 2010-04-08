/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : parsearg.h , header File
**    Project  : Ingres II / IJA
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Parse the arguments
**
** History:
**
** 12-Sep-2000 (uk$so01)
**    Created
**
**/

#if !defined (PARSEARG_HEADER)
#define PARSEARG_HEADER

class CaCommandLine
{
public:
	CaCommandLine();
	~CaCommandLine(){}


	//
	// Return Value:
	//    -1: error, the program continues like no command lines are specified
	//     0: no command lines
	//     1: node level, -node=nn
	//     2: database level, -node=nn -database=dbxx
	//     3: table level, -node=nn -database=dbxx -table=owner.tbxx
	int Parse (CString& strCmdLine);

	int     m_nArgs;
	CString m_strNode;
	CString m_strDatabase;
	CString m_strTable;
	CString m_strTableOwner;

	BOOL m_bSplitLeft;
	BOOL m_bNoToolbar;
	BOOL m_bNoStatusbar;
};



#endif // PARSEARG_HEADER