/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : parsearg.cpp , Implementation File
**    Project  : Ingres II / IIA
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Parse the arguments
**
** History:
**
** 12-Sep-2000 (uk$so01)
**    Created
** 31-Oct-2001 (hanje04)
**    Replaced out of date 'CString& name=' constructs with CString name() as
**    they was causing problems on Linux.
** 18-Mar-2002 (uk$so01)
**    Use the correct character mapping.
** 26-Feb-2003 (uk$so01)
**    SIR  #106952, Enhance library + command line.
** 31-Mar-2003 (hanje04)
**    Bug 109829 
**    Stop iia treating -notaflag as a file, and print usage message.
** 22-Sep-2003 (uk$so01)
**    Bug 109829, correction the previous change #464254
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
	m_bLoop = FALSE;
	m_bBatch= FALSE;
	m_bNewTable = FALSE;

	m_strNode = _T("");
	m_strDatabase = _T("");
	m_strTable = _T("");
	m_strTableOwner = _T("");

	m_strIIImportFile = _T("");
	m_strLogFile = _T("");
	m_strNewTable = _T("");
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
			if (strKey.CompareNoCase(_T("-node")) == 0)
			{
				strValue = pKey->GetValue();
				m_strNode = strValue;
				m_bParam = TRUE;
			}
			else
			if (strKey.CompareNoCase(_T("-server")) == 0)
			{
				strValue = pKey->GetValue();
				m_strServerClass = strValue;
				m_bParam = TRUE;
			}
			else
			if (strKey.CompareNoCase(_T("-database")) == 0)
			{
				strValue = pKey->GetValue();
				m_strDatabase = strValue;
				m_bParam = TRUE;
			}
			else
			if (strKey.CompareNoCase(_T("-table")) == 0)
			{
				strValue = pKey->GetValue();
				ExtractNameAndOwner (strValue, m_strTableOwner, m_strTable);
				m_bParam = TRUE;
			}
			else
			if (strKey.CompareNoCase(_T("-log")) == 0)
			{
				strValue = pKey->GetValue();
				m_strLogFile = strValue;
				m_bParam = TRUE;
			}
			else
			if (strKey.CompareNoCase(_T("-createdtable")) == 0)
			{
				strValue = pKey->GetValue();
				m_strNewTable = strValue;
				m_bParam = TRUE;
			}
			else
			if (strKey.CompareNoCase(_T("-file")) == 0)
			{
				strValue = pKey->GetValue();
				m_strIIImportFile = strValue;
				m_bParam = TRUE;
			}
			else
			if (strKey.CompareNoCase(_T("-batch")) == 0)
			{
				strValue = pKey->GetValue();
				if (strValue.CompareNoCase(_T("TRUE")) == 0)
				{
					m_bBatch = TRUE;
					m_bParam = TRUE;
				}
			}
			else
			if (strKey.CompareNoCase(_T("-LOOP")) == 0)
			{
				strValue = pKey->GetValue();
				if (strValue.CompareNoCase(_T("TRUE")) == 0)
				{
					m_bLoop = TRUE;
					m_bParam = TRUE;
				}
			}
		}
	}
	return TRUE;
}

