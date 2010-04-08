/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : parsearg.cpp , Implementation File
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
**    SIR  #106952, Enhance library + command line.
** 22-Aug-2003 (uk$so01)
**    SIR #106648, Add silent mode in IEA. 
** 02-Feb-2004 (uk$so01)
**    SIR #106648, Vdba-Split, -loop option. 
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
	m_bParam = FALSE;
	m_bLoop = FALSE;
	m_strNode = _T("");
	m_strDatabase = _T("");
	m_strIIParamFile = _T("");
	m_strStatement = _T("");
	m_strExportFile = _T("");
	m_strExportMode = _T("");

	m_bBatch= FALSE;
	m_strLogFile = _T("");
	m_strListFile= _T("");
	m_bOverWrite= FALSE;
}

BOOL CaCommandLine::HandleCommandLine()
{
	int nSemantic1 = 2; // 0|2 (OK)
	int nSemantic2 = 0; // if nSemantic1 == 0 => 1 (OK)
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

#if defined (_IEA_STANDARD_OPTION)
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
			if (strKey.CompareNoCase(_T("-statement")) == 0)
			{
				strValue = pKey->GetValue();
				m_strStatement = strValue;
				m_bParam = TRUE;
			}
			else
			if (strKey.CompareNoCase(_T("-exportmode")) == 0)
			{
				strValue = pKey->GetValue();
				m_strExportMode = strValue;
				m_bParam = TRUE;
			}
			else
			if (strKey.CompareNoCase(_T("-exportfile")) == 0)
			{
				strValue = pKey->GetValue();
				m_strExportFile = strValue;
				m_bParam = TRUE;
			}
			else
#endif
			if (strKey.CompareNoCase(_T("-loop")) == 0)
			{
				strValue = pKey->GetValue();
				if (strValue.CompareNoCase(_T("TRUE")) == 0)
				{
					m_bLoop = TRUE;
					m_bParam = TRUE;
				}
			}
			else
			if (strKey.CompareNoCase(_T("-file")) == 0)
			{
				strValue = pKey->GetValue();
				m_strIIParamFile = strValue;
				m_bParam = TRUE;
				nSemantic2++;
			}
			else
			if (strKey.CompareNoCase(_T("-silent")) == 0)
			{
				strValue = pKey->GetValue();
				if (strValue.CompareNoCase(_T("TRUE")) == 0)
				{
					m_bBatch = TRUE;
					m_bParam = TRUE;
				}
				nSemantic1--;
			}
			else
			if (strKey.CompareNoCase(_T("-logfilename")) == 0)
			{
				strValue = pKey->GetValue();
				m_strLogFile = strValue;
				m_bParam = TRUE;
				nSemantic1--;
			}
			else
			if (strKey.CompareNoCase(_T("-l")) == 0)
			{
				strValue = pKey->GetValue();
				m_strListFile = strValue;
				m_bParam = TRUE;
				nSemantic2++;
			}
			else
			if (strKey.CompareNoCase(_T("-o")) == 0)
			{
				strValue = pKey->GetValue();
				if (strValue.CompareNoCase(_T("TRUE")) == 0)
				{
					m_bOverWrite = TRUE;
					m_bParam = TRUE;
				}
			}
		}
	}

	if (nSemantic1 == 2)
		return TRUE;
	if (nSemantic1 == 0)
	{
		if (nSemantic2 == 1)
			return TRUE;
		else
			return FALSE;
	}
	else
		return FALSE;
}



