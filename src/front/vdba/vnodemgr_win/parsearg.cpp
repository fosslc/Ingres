/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : parsearg.cpp , Implementation File
**    Project  : Ingres II / Ingnet
**    Author   : Schalk Philippe (schph01)
**    Purpose  : Parse the arguments
**
** History:
**
** 03-Oct-2003 (schph01)
**    SIR 109864 manage -vnode command line option for ingnet utility
**/

#include "stdafx.h"
#include "parsearg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CaCommandLine::CaCommandLine():CaArgumentLine()
{
	m_nArgs = 0;

	m_strNode = _T("");
	m_bParam = FALSE;
}


BOOL CaCommandLine::HandleCommandLine()
{
	CTypedPtrList <CObList, CaKeyValueArg*>& listKey = GetKeys();
	POSITION pos = listKey.GetHeadPosition();
	m_bParam = FALSE;
	while (pos != NULL)
	{
		CaKeyValueArg* pKey = listKey.GetNext (pos);
		if (pKey && pKey->IsMatched())
		{
			CString strValue;
			CString strKey = pKey->GetKey();
			if (strKey.CompareNoCase(_T("-vnode")) == 0)
			{
				strValue = pKey->GetValue();
				m_strNode = strValue;
				m_bParam = TRUE;
			}
		}
	}
	return TRUE;
}

