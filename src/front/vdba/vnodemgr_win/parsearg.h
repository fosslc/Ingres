/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : parsearg.h , header File
**    Project  : Ingres II / Ingnet
**    Author   : Schalk Philippe (schph01)
**    Purpose  : Parse the arguments
**
** History:
**
** 03-Oct-2003 (schph01)
**    SIR 109864 manage -vnode command line option for ingnet utility
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

	CString m_strNode;
protected:
	BOOL m_bParam;
};


#endif // PARSEARG_HEADER