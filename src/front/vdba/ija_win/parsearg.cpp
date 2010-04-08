/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : parsearg.cpp , Implementation File
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

#include "stdafx.h"
#include "parsearg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static void GetNextChar (TCHAR* pText, CString& strNextChar)
{
	TCHAR ch [3];
	if (IsDBCSLeadByte (*pText))
	{
		ch[0] = pText[0];
		ch[1] = pText[1];
		ch[2] = _T('\0');
	}
	else
	{
		ch[0] = pText[0];
		ch[1] = _T('\0');
	}
	strNextChar = ch;
}

//
// This function is DBCS compliant.
static BOOL VarableExtract (LPCTSTR lpszVar, CString& strName, CString& strValue)
{
	CString strV = lpszVar;
	int n0A = strV.Find (0xA);
	if (n0A != -1)
		strV.SetAt(n0A, _T(' '));
	strV.TrimRight();
	int nEq = strV.Find (_T('='));
	if (nEq != -1)
	{
		strName = strV.Left (nEq);
		strValue= strV.Mid (nEq+1);
	}
	else
	{
		strName = lpszVar;
		strValue = _T("");
	}

	return TRUE;
}


CaCommandLine::CaCommandLine()
{
	m_nArgs = 0;
	m_strNode = _T("");
	m_strDatabase = _T("");
	m_strTable = _T("");
	m_strTableOwner = _T("");

	m_bSplitLeft = FALSE;
	m_bNoToolbar = FALSE;
	m_bNoStatusbar = FALSE;
}


int CaCommandLine::Parse (CString& strCmdLine)
{
	//
	// I define the keyword (no space is allowed): 
	//   -node
	//   /node
	//   -database
	//   /database
	//   -table
	//   /table

	CString strText = strCmdLine;
	CStringList listToken;
	CString strNextChar;
	CString strToken = _T("");
	strText.MakeLower();

	TCHAR tchBack = _T('0');
	BOOL bDQuote = FALSE;
	BOOL bSQuote = FALSE;
	TCHAR* pch = (LPTSTR)(LPCTSTR)strText;
	while (*pch != _T('\0'))
	{
		if (*pch == _T('-') || *pch == _T('/'))
		{
			if (!bDQuote && !bSQuote && (tchBack == _T(' ')) && !strToken.IsEmpty())
			{
				listToken.AddTail (strToken);
				strToken = _T("");
			}
		}

		if (*pch == _T('\"') && tchBack != _T('\\'))
			bDQuote = !bDQuote;
		if (*pch == _T('\'') && tchBack != _T('\\'))
			bSQuote = !bSQuote;

		GetNextChar(pch, strNextChar);
		if (!(*pch == _T('\"') && tchBack != _T('\\')))
			strToken += strNextChar;

		tchBack = *pch;
		pch = _tcsinc(pch);

		if (*pch == _T('\0'))
			listToken.AddTail (strToken);
	}
	
	POSITION pos = listToken.GetHeadPosition();
	while (pos != NULL)
	{
		CString strTok = listToken.GetNext (pos);
		CString strName;
		CString strValue;

		if (!VarableExtract(strTok, strName, strValue))
		{
			m_nArgs = -1;
			return -1;
		}

		strName.TrimLeft();
		strName.TrimRight();
		strValue.TrimLeft();
		strValue.TrimRight();
		
		if (strName.Compare ("-node") == 0 || strName.Compare ("/node") == 0)
		{
			m_nArgs++;
			if (!m_strNode.IsEmpty())
			{
				m_nArgs = -1;
				return -1;
			}
			m_strNode = strValue;
		}
		else
		if (strName.Compare ("-database") == 0 || strName.Compare ("/database") == 0)
		{
			m_nArgs++;
			if (!m_strDatabase.IsEmpty())
			{
				m_nArgs = -1;
				return -1;
			}
			m_strDatabase = strValue;
		}
		else
		if (strName.Compare ("-table") == 0 || strName.Compare ("/table") == 0)
		{
			m_nArgs++;
			if (!m_strTable.IsEmpty())
			{
				m_nArgs = -1;
				return -1;
			}
			//
			// Check if strValue is pecified as <owner.tablename>:
			strToken = _T("");
			bDQuote = FALSE;
			bSQuote = FALSE;
			pch = (LPTSTR)(LPCTSTR)strValue;
			while (*pch != _T('\0'))
			{
				if (*pch == _T('.'))
				{
					if (!bDQuote && !bSQuote && !strToken.IsEmpty())
					{
						m_strTableOwner = strToken;
						strToken = _T("");

						tchBack = *pch;
						pch = _tcsinc(pch);
						continue;
					}
				}

				if (*pch == _T('\"') && tchBack != _T('\\'))
					bDQuote = !bDQuote;
				if (*pch == _T('\'') && tchBack != _T('\\'))
					bSQuote = !bSQuote;

				GetNextChar(pch, strNextChar);
				strToken += strNextChar;
				tchBack = *pch;
				pch = _tcsinc(pch);

				if (*pch == _T('\0'))
					m_strTable = strToken;
			}
		}
		else
		if (strName.Compare ("-splitleft") == 0 || strName.Compare ("/splitleft") == 0)
		{
			m_bSplitLeft = TRUE;
		}
		else
		if (strName.Compare ("-notoolbar") == 0 || strName.Compare ("/notoolbar") == 0)
		{
			m_bNoToolbar = TRUE;
		}
		else
		if (strName.Compare ("-nostatusbar") == 0 || strName.Compare ("/nostatusbar") == 0)
		{
			m_bNoStatusbar = TRUE;
		}
		else
		{
			m_nArgs = -1;
			return m_nArgs;
		}
	}

	//
	// Check if everything is OK:
	if (m_nArgs == 1 && m_strNode.IsEmpty())
	{
		//
		// Must specify node name !
		m_nArgs = -1; 
	}
	else
	if (m_nArgs == 2 && (m_strNode.IsEmpty() || m_strDatabase.IsEmpty()))
	{
		//
		// Must specify node name  and Database name !
		m_nArgs = -1; 
	}
	else
	if (m_nArgs == 3 && 
	   (m_strNode.IsEmpty() || m_strDatabase.IsEmpty() || m_strTable.IsEmpty() || m_strTableOwner.IsEmpty()))
	{
		//
		// Must specify node name , Database name and TableOwner.TableName !
		m_nArgs = -1; 
	}

	return m_nArgs;
}

