/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : cmdargs.cpp , Implementation File
**    Project  : Ingres II / vdbamon
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Parse the arguments
**
** History:
**
** 22-Mar-2002 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
** 04-Feb-2009 (drivi01)
**    In efforts to port to Visual Studio 2008, typecast 
**    return value of _tcsrchr to (TCHAR *).
**/

#include "stdafx.h"
#include "cmdargs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Get the next character:
// It does not modify the parameter 'lpcText'
// ************************************************************************************************
static TCHAR PeekNextChar (const TCHAR* lpcText)
{
	TCHAR* pch = (TCHAR*)lpcText;
	if (!pch)
		return _T('\0');
	pch = _tcsinc(pch);

	if (!pch)
		return _T('\0');
	if (_istleadbyte(*pch))
		return _T('\0');
	return *pch;
}

//
// This function is DBCS compliant.
void ExtractNameAndOwner (LPCTSTR lpszObject, CString& strOwner, CString& strName, BOOL bRemoveTextQualifier)
{
	CString strText = lpszObject;
	CString strToken = _T("");
	TCHAR tchszCur = _T('0');
	struct StateStack
	{
		BOOL IsEmpty(){return m_stack.IsEmpty();}
		TCHAR Top()
		{
			TCHAR state;
			if (IsEmpty())
				return _T('\0');
			state = m_stack.GetHead();
			return state;
		}
		void PushState(TCHAR state){ m_stack.AddHead(state); };
		void PopState() { m_stack.RemoveHead(); }
	
		CList< TCHAR, TCHAR > m_stack;
	};
	StateStack stack;

	TCHAR* pch = (LPTSTR)(LPCTSTR)strText;
	while (pch && *pch != _T('\0'))
	{
		tchszCur = *pch;
		if (_istleadbyte(tchszCur))
		{
			pch = _tcsinc(pch);
			strToken += tchszCur;
			continue;
		}

		if (!stack.IsEmpty()) // We're inside the quote (single or double)
		{
			TCHAR tchTop = stack.Top();
			if (tchTop == tchszCur) // The current character is a quote (single or double)
			{
				if (PeekNextChar(pch) != tchTop) // The next char is not a quote. (Here we found the end quote)
				{
					stack.PopState();
					if (!bRemoveTextQualifier)
						strToken += tchszCur;
				}
				else
				{
					//
					// We consider the current quote and next quote (consecutive quotes) as the
					// normal characters:
					strToken += tchszCur;
					strToken += PeekNextChar(pch);
					pch = _tcsinc(pch);
				}
			}
			else
				strToken += tchszCur;

			pch = _tcsinc(pch);
			continue;
		}

		// 
		// We found a dot:
		if (tchszCur == _T('.'))
		{
			strOwner = strToken;
			pch = _tcsinc(pch);
			strName  = pch;
			return;
		}
		else
		{
			if (tchszCur == _T('\"') || tchszCur == _T('\'')) // Found the begin quote.
			{
				//
				// Note: the begin quote cannot be an escape of quote because 
				// the quote can be escaped only if it is inside the quote
				stack.PushState (tchszCur);
				if (!bRemoveTextQualifier)
					strToken += tchszCur;
			}
			else
				strToken += tchszCur;
		}

		pch = _tcsinc(pch);
	}
	strOwner = _T("");
	strName = lpszObject;
}


IMPLEMENT_DYNCREATE(CaKeyValueArg, CObject)
CaKeyValueArg::CaKeyValueArg():m_strKey(_T("")), m_strValue(_T(""))
{
	Init();
}

CaKeyValueArg::CaKeyValueArg(LPCTSTR lpszKey):m_strKey(lpszKey), m_strValue(_T(""))
{
	Init();
}

IMPLEMENT_DYNCREATE(CaKeyValueArgObjectWithOwner, CaKeyValueArg)
CaKeyValueArgObjectWithOwner::CaKeyValueArgObjectWithOwner():CaKeyValueArg(_T(""))
{
	Init(); 
	SetArgType(OBT_OBJECTWITHOWNER);
}

CaKeyValueArgObjectWithOwner::CaKeyValueArgObjectWithOwner(LPCTSTR lpszKey , int nOBjType):CaKeyValueArg(lpszKey)
{
	Init(); 
	SetArgType(nOBjType);
}


CaArgumentLine::CaArgumentLine(LPCTSTR lpszCmdLine)
{
	m_nCurrentKey = K_UNKNOWN;
	m_nArgs = 0;
	m_strFile = _T("");
	m_strCmdLine = lpszCmdLine;
}


CaArgumentLine::~CaArgumentLine()
{
	while (!m_listKeyValue.IsEmpty())
		delete m_listKeyValue.RemoveHead();
	m_listKey.RemoveAll();
}

void CaArgumentLine::AddKey (LPCTSTR lpszKey, BOOL bEqualSign, BOOL bValue, BOOL bOjectWithOwner)
{
	if (bOjectWithOwner)
	{
		ASSERT(bValue);
		CaKeyValueArgObjectWithOwner* pKey = NULL;
		pKey = new CaKeyValueArgObjectWithOwner(lpszKey, OBT_OBJECTWITHOWNER);
		pKey->SetEqualSign(bEqualSign);
		pKey->SetNeedValue(TRUE);

		m_listKeyValue.AddTail(pKey);
	}
	else
	{
		CaKeyValueArg* pKey = NULL;
		pKey = new CaKeyValueArg(lpszKey);
		pKey->SetEqualSign(bEqualSign);
		pKey->SetNeedValue(bValue);

		m_listKeyValue.AddTail(pKey);
	}
}


CString CaArgumentLine::RemoveQuote (LPCTSTR lpszText, TCHAR tchQuote)
{
	BOOL bBeginQuote = FALSE;
	BOOL bEndQuote = FALSE;
	TCHAR tchszCur = _T('0');
	TCHAR* pszTemp = (TCHAR *)_tcsrchr(lpszText, tchQuote);
	if ( pszTemp && (*_tcsinc(pszTemp) == _T('\0')))
		bEndQuote = TRUE;
	if (lpszText && !_istleadbyte (*lpszText))
	{
		if (*lpszText == tchQuote)
			bBeginQuote = TRUE;
	}

	CString strNewString = lpszText;
	if (bBeginQuote && bEndQuote)
	{
		strNewString = _T("");
		TCHAR* pch = (LPTSTR)lpszText;
		while (pch && *pch != _T('\0'))
		{
			tchszCur = *pch;
			if (_istleadbyte(tchszCur))
			{
				pch = _tcsinc(pch);
				strNewString += tchszCur;
				continue;
			}
			if (tchszCur == tchQuote)
			{
				if (PeekNextChar(pch) == tchQuote)
				{
					strNewString += tchQuote;
					pch = _tcsinc(pch);
				}
			}
			else
			{
				strNewString += tchszCur;
			}
			
			pch = _tcsinc(pch);
		}
	}

	return strNewString;
}


//
// Tokens are separated by a space or quote:
// Ex1: "ABC=WXX /BN""X"/CL --> 2 token (inside the quote, the "" is equivalent to \")
//      1) "ABC=WXX /BN""X"
//      2) /CL
// Ex2: MAKE IT -->
//      1) MAKE
//      2) IT
BOOL CaArgumentLine::MakeListTokens(LPCTSTR lpzsText, CStringArray& arrayToken, LPCTSTR lpzsSep)
{
	CString strText = lpzsText;
	CString strTokenSeparator = lpzsSep;
	CString strToken = _T("");
	TCHAR tchszCur = _T('0');
	StateStack stack;

	TCHAR* pch = (LPTSTR)(LPCTSTR)strText;
	while (*pch != _T('\0'))
	{
		tchszCur = *pch;
		if (_istleadbyte(tchszCur))
		{
			pch = _tcsinc(pch);
			strToken += tchszCur;
			continue;
		}
		
		if (!stack.IsEmpty()) // We're inside the quote (single or double)
		{
			TCHAR tchTop = stack.Top();
			if (tchTop == tchszCur) // The current character is a quote (single or double)
			{
				if (PeekNextChar(pch) != tchTop) // The next char is not a quote. (Here we found the end quote)
				{
					stack.PopState();
					strToken += tchszCur;
				}
				else
				{
					//
					// We consider the current quote the escaped of the next quote (consecutive quotes).
					strToken += tchszCur;
					strToken += PeekNextChar(pch);
					pch = _tcsinc(pch);
				}
			}
			else
			{
				strToken += tchszCur;
			}
			pch = _tcsinc(pch);
			continue;
		} // (!stack.IsEmpty())

		// 
		// We found a space (token separator):
		if (tchszCur == _T(' ') || (!strTokenSeparator.IsEmpty() && strTokenSeparator.Find(tchszCur) != -1))
		{
			if (!strToken.IsEmpty())
			{
				arrayToken.Add(strToken);
				strToken = _T("");
			}
		}
		else
		{
			if (tchszCur == _T('\"') || tchszCur == _T('\'')) // Found the begin quote.
			{
				//
				// Note: the begin quote cannote be an escape of quote because 
				// the quote can be escaped only if it is inside the quote
				stack.PushState (tchszCur);

				if (!strToken.IsEmpty())
				{
					arrayToken.Add(strToken);
					strToken = _T("");
				}
			}
			strToken += tchszCur;
		}

		pch = _tcsinc(pch);
		if (*pch == _T('\0'))
		{
			if (!strToken.IsEmpty())
			{
				arrayToken.Add(strToken);
				strToken = _T("");
			}
		}
	}
	if (!strToken.IsEmpty())
	{
		arrayToken.Add(strToken);
		strToken = _T("");
	}

	if (!stack.IsEmpty())
		return FALSE;

	return TRUE;
}


int CaArgumentLine::Parse ()
{
	if (m_nArgs == -1)
		return m_nArgs;
	CStringArray arrayToken;
	BOOL bParseOK = CaArgumentLine::MakeListTokens (m_strCmdLine, arrayToken);
	if (!bParseOK)
	{
		m_nArgs = -1;
		return m_nArgs;
	}
	int i = 0, nSize = arrayToken.GetSize();
#if defined (_DEBUG)
	for (i=0; i<nSize; i++)
	{
		TRACE2("Token= %s len=%d\n", arrayToken.GetAt(i), arrayToken.GetAt(i).GetLength());
	}
#endif
	i=0;

	while (i<nSize)
	{
		CString strToken = arrayToken.GetAt(i);
		int nLen = strToken.GetLength();
		POSITION pos = m_listKeyValue.GetHeadPosition();
		BOOL bCheck = FALSE;
		while (pos != NULL)
		{
			CaKeyValueArg* pKey = m_listKeyValue.GetNext(pos);
			if (pKey->IsMatched())
				continue; // Skip the already matched argument.

			CString strKey = pKey->GetKey();
			int nKeyLen = strKey.GetLength();
			if (_tcsnicmp (strToken, strKey, nKeyLen) == 0)
			{
				if (pKey->GetEqualSign()) // Implicit need value
				{
					if (!AnalyseKeyEqValue(arrayToken, i, pKey))
						return -1;
				}
				else
				if (pKey->GetNeedValue())
				{
					if (!AnalyseKeyValue(arrayToken, i, pKey))
						return -1;
				}
				else
				{
					if (!AnalyseKey(arrayToken, i, pKey))
						return -1;
				}
				bCheck = TRUE;
				break;
			}
		}

		if (!bCheck)
		{
			//
			// The token is not matched to the specified key. Is it a file name ?
			if (!AnalyseNonKey(arrayToken, i))
				return -1;
		}
		i++;
	}

#if defined (_DEBUG)
	POSITION p = m_listKeyValue.GetHeadPosition();
	while (p != NULL)
	{
		CaKeyValueArg* pArg = m_listKeyValue.GetNext(p);
		if (pArg && pArg->IsMatched())
		{
			TRACE2("ARG===> KEY=%-10s  VALUE=%s\n", (LPCTSTR)pArg->GetKey(), (LPCTSTR)pArg->GetValue());
		}
	}
#endif

	return m_nArgs;
}


//
// The command line is: -k
BOOL CaArgumentLine::AnalyseNonKey(CStringArray& arryToken, int& nCurrentIndex)
{
	int nSize = arryToken.GetSize();
	CString strToken = arryToken.GetAt(nCurrentIndex);
	int nLen = strToken.GetLength();
	if (m_nCurrentKey == K_NODE)
	{
		if (_tcsnicmp (strToken, _T("/"), 1) == 0) 
		{
			//
			// When found /XXX after node name, XXX is ALWAYS considered to be a SERVER CLASS
			if (nLen == 1)
			{
				//
				// Check the next token, it must be the server.
				ASSERT ((nCurrentIndex+1) < nSize);
				if ((nCurrentIndex+1 >= nSize))
				{
					m_nArgs = -1;
					return FALSE;
				}
				nCurrentIndex++;
				strToken = arryToken.GetAt(nCurrentIndex);
			}
			else
			{
				strToken = strToken.Mid(1);
				strToken.TrimLeft  ();
				strToken.TrimRight ();
				if (strToken.GetAt(0) == _T('\''))
					strToken = RemoveQuote (strToken, _T('\''));
				else
				if (strToken.GetAt(0) == _T('\"'))
					strToken = RemoveQuote (strToken, _T('\"'));

				//
				// Implicit add a new key -server just after the -node Key:
				POSITION p, pos = m_listKeyValue.GetHeadPosition();
				while (pos != NULL)
				{
					p = pos;
					CaKeyValueArg* pCur = m_listKeyValue.GetNext (pos);
					if (pCur->GetKey().CompareNoCase(_T("-node")) == 0)
					{
						CaKeyValueArg* pObj = new CaKeyValueArg(_T("-server"));
						pObj->SetEqualSign(TRUE);
						pObj->SetNeedValue(TRUE);
						pObj->SetValue(strToken);
						m_listKeyValue.InsertAfter(p, pObj);
						break;
					}
				}
			}
			m_nCurrentKey = K_SERVER;
		}
		else
		{
			m_nArgs = -1;
			return FALSE;
		}
	}
	else
	{
		//
		// Everything that has no key is considered to be a FILE NAME:
		strToken.TrimLeft  (_T('\"'));
		strToken.TrimRight (_T('\"'));
		CString strFile = RemoveQuote (strToken, _T('\''));
		m_nCurrentKey = K_FILE;

		if (!strFile.IsEmpty() && (strFile.GetAt(0) == _T('-') || strFile.GetAt(0) == _T('/')))
		{
			//
			// Do not consider the /xyz or -xyz as a file even though -xyz can be a file:
			m_nArgs = -1;
			return FALSE;
		}

		CaKeyValueArg* pObj = new CaKeyValueArg(_T("-file"));
		pObj->SetEqualSign(TRUE);
		pObj->SetNeedValue(TRUE);
		pObj->SetValue(strFile);
		m_listKeyValue.AddTail(pObj);

	}
	return TRUE;
}

//
// The command line is: -k
BOOL CaArgumentLine::AnalyseKey(CStringArray& arryToken, int& nCurrentIndex, CaKeyValueArg* pKey)
{
	int nSize = arryToken.GetSize();
	CString strToken = arryToken.GetAt(nCurrentIndex);
	int nLen = strToken.GetLength();
	//
	// Analyse token : Syntax: -k
	CString strKey = pKey->GetKey();
	int nKeyLen = _tcslen(strKey);
	if (nLen == nKeyLen) // first token is "-k"
	{
		pKey->SetValue(_T("TRUE"));
	}
	else
	{
		m_nArgs = -1;
		return FALSE;
	}

	m_nCurrentKey = K_UNKNOWN;
	return TRUE;
}



//
// The command line is: -k<value>
// But the user may type the space character after the '-k'.
// This can lead to the 2 possibilities:
//    1) -k<value>        => 1 token  = "-k<value>"
//    2) -k <value>       => 2 tokens = "-k", "<value>"
BOOL CaArgumentLine::AnalyseKeyValue(CStringArray& arryToken, int& nCurrentIndex, CaKeyValueArg* pKey)
{
	int nSize = arryToken.GetSize();
	CString strToken = arryToken.GetAt(nCurrentIndex);
	int nLen = strToken.GetLength();
	//
	// Analyse token : Syntax: -k<value>
	CString strKey = pKey->GetKey();
	int nKeyLen = _tcslen(strKey);
	if (nLen == nKeyLen) // first token is "-k"
	{
		//
		// Check the next token, it must be the value'.
		ASSERT ((nCurrentIndex+1) < nSize);
		if ((nCurrentIndex+1 >= nSize))
		{
			m_nArgs = -1;
			return FALSE;
		}
		nCurrentIndex++;
		strToken = arryToken.GetAt(nCurrentIndex);
	}
	else
	{
		strToken = strToken.Mid(nKeyLen);
	}

	//
	// At this point, we have 'strToken' represents the value.
	strToken.TrimLeft();
	strToken.TrimRight();
	if (strToken.IsEmpty())
	{
		m_nArgs = -1;
		return FALSE;
	}

	if (strToken.GetAt(0) == _T('\''))
		strToken = RemoveQuote (strToken, _T('\''));
	else
	if (strToken.GetAt(0) == _T('\"'))
		strToken = RemoveQuote (strToken, _T('\"'));

	pKey->SetValue(strToken);
	m_nCurrentKey = K_UNKNOWN;
	return TRUE;
}


//
// The command line is: "-key=<value>". For the node, it can be "-node=<value>[/SERVER]"
// But the user may type the space character before or after the equal sign '='.
// This can lead to the 4 possibilities:
//    1) -key=<value>        => 1 token  = "-key=<value>"
//    2) -key = <value>      => 3 tokens = "-key", "=", "<value>"
//    3) -key=  <value>      => 2 tokens = "-key=", "<value>"
//    4) -key  =<value>      => 2 tokens = "-key", "=<value>"
BOOL CaArgumentLine::AnalyseKeyEqValue(CStringArray& arryToken, int& nCurrentIndex, CaKeyValueArg* pKey)
{
	BOOL b2ParseValue  = FALSE;
	BOOL bRequireValue = TRUE;
	int nSize = arryToken.GetSize();
	CString strToken = arryToken.GetAt(nCurrentIndex);
	int nLen = strToken.GetLength();
	CString strKey = pKey->GetKey();
	int nKeyLen = _tcslen(strKey);
	//
	// Analyse token: Syntax: -node=<nodename>[/SERVER]
	if (nLen == nKeyLen) // first token is "-key"
	{
		//
		// Check the next token, it must contain an equal sign '='.
		ASSERT ((nCurrentIndex+1) < nSize);
		if ((nCurrentIndex+1 >= nSize))
		{
			m_nArgs = -1;
			return FALSE;
		}
		nCurrentIndex++;
		strToken = arryToken.GetAt(nCurrentIndex);
		if (_tcsnicmp (strToken, _T("="), 1) != 0)
		{
			m_nArgs = -1;
			return FALSE;
		}
		if( strToken.GetLength() == 1) // seconde token is "=".
		{
			//
			// Check the next token, it must be a node value.
			ASSERT ((nCurrentIndex+1) < nSize); // value 
			if ((nCurrentIndex+1 >= nSize))
			{
				m_nArgs = -1;
				return FALSE;
			}
			nCurrentIndex++;
			strToken = arryToken.GetAt(nCurrentIndex);
		}
		else
		{
			strToken = strToken.Mid (1);
			strToken.TrimLeft();
		}
	}
	else
	{
		strToken = strToken.Mid(nKeyLen);
		strToken.TrimLeft();
		if (_tcsnicmp (strToken, _T("="), 1) != 0)
		{
			m_nArgs = -1;
			return FALSE;
		}

		if (strToken.GetLength() == 1) // case first token contains "-key="
		{
			//
			// Check the next token, it must be a node value.
			ASSERT ((nCurrentIndex+1) < nSize); // value 
			if ((nCurrentIndex+1 >= nSize))
			{
				m_nArgs = -1;
				return FALSE;
			}
			nCurrentIndex++;
			strToken = arryToken.GetAt(nCurrentIndex);
		}
		else
		{
			strToken = strToken.Mid (1);
			strToken.TrimLeft();
		}
	}

	//
	// At this point, we have 'strToken' represents the value.
	// Check if it is a node value, it may contain the /<SERVER> ?
	strToken.TrimLeft();
	strToken.TrimRight();
	if (strToken.IsEmpty())
	{
		m_nArgs = -1;
		return FALSE;
	}

	if (_tcsicmp(strKey, _T("-node")) == 0)
		m_nCurrentKey = K_NODE;
	else
	if (_tcsicmp(strKey, _T("-server")) == 0)
		m_nCurrentKey = K_SERVER;
	else
	if (_tcsicmp(strKey, _T("-database")) == 0)
		m_nCurrentKey = K_DATABASE;
	else
	if (_tcsicmp(strKey, _T("-user")) == 0)
		m_nCurrentKey = K_USER;
	else
	if (_tcsicmp(strKey, _T("-table")) == 0)
		m_nCurrentKey = K_TABLE;
	else
		m_nCurrentKey = K_UNKNOWN;

	if (pKey->GetArgType() != OBT_OBJECTWITHOWNER)
	{
		if (strToken.GetAt(0) == _T('\'') || strToken.GetAt(0) == _T('\"'))
		{
			//
			// If the token is a quote string, it cannot conain the /SERVER because
			// we consider the quote string is a single token.
			if (strToken.GetAt(0) == _T('\"'))
				strToken = RemoveQuote (strToken, _T('\"'));
			else
			if (strToken.GetAt(0) == _T('\''))
				strToken = RemoveQuote (strToken, _T('\''));

			pKey->SetValue(strToken);
		}
		else
		{
			pKey->SetValue(strToken);

			if (m_nCurrentKey == K_NODE)
			{
				int nPos = strToken.ReverseFind(_T('/'));
				if (nPos != -1)
				{
					CString strNode = strToken.Left(nPos);
					CString strServer = strToken.Mid(nPos+1);
					pKey->SetValue(strNode);

					//
					// Implicit add a new key -server just after the Current Key:
					POSITION pos = m_listKeyValue.Find(pKey);
					ASSERT(pos);
					if (pos != NULL)
					{
						CaKeyValueArg* pObj = new CaKeyValueArg(_T("-server"));
						pObj->SetEqualSign(TRUE);
						pObj->SetNeedValue(TRUE);
						pObj->SetValue(strServer);
						m_listKeyValue.InsertAfter(pos, pObj);
					}
				}
			}
		}
	}
	else
	if (pKey->GetArgType() == OBT_OBJECTWITHOWNER)
	{
		CaKeyValueArgObjectWithOwner* pExactKey = NULL;
		ASSERT (pKey->IsKindOf(RUNTIME_CLASS(CaKeyValueArgObjectWithOwner)));
		if (pKey->IsKindOf(RUNTIME_CLASS(CaKeyValueArgObjectWithOwner)))
		{
			pExactKey = (CaKeyValueArgObjectWithOwner*)pKey;
		}
		//
		// User may specifies: [OWNER.]Object 
		// Then we can have 3 possibilities:
		//    1) 3 tokens: {owner}{.}{table}. Ex: "Oscar Wilde"."table 1" --> {"Oscar Wilde"},{.} , {"table 1"}
		//    2) 2 tokens: {owner.}{table}    Ex: Ingres."table 1"        --> {ingres.},  {"table 1"}
		//    3) 2 tokens: {owner}{.table}    Ex: "Ingres".table"         --> {"ingres"}, {.table}
		//    4) 1 token : {owner.table}      Ex: Ingres.table"           --> {ingres.table}

		CString strFullName = strToken;
		int nl = strToken.GetLength();
		if ((nCurrentIndex +1) < nSize)
		{
			if ((nl > 0) && (strToken.GetAt(0) == _T('\"')) && (strToken.GetAt(nl-1)  == _T('\"')))
			{
				CString strNextTok = arryToken.GetAt(nCurrentIndex +1 );
				if (strNextTok.GetAt(0) == _T('.'))
				{
					if (pExactKey)
					{
						strToken = RemoveQuote (strToken, _T('\"'));
						pExactKey->SetOwner(strToken);
					}
					if (strNextTok.GetLength() == 1) // the object is in quote
					{
						if ((nCurrentIndex +2) >= nSize)
						{
							m_nArgs = -1;
							return FALSE;
						}
						strFullName += _T(".");
						strToken = arryToken.GetAt(nCurrentIndex +2 );
						strFullName += strToken;
						if (pExactKey)
						{
							strToken = RemoveQuote (strToken, _T('\"'));
							pExactKey->SetObject(strToken);
						}
						nCurrentIndex+= 2;
					}
					else
					{
						strFullName += strNextTok;
						if (pExactKey)
						{
							strNextTok = strNextTok.Mid(1);
							strNextTok = RemoveQuote (strToken, _T('\"'));
							pExactKey->SetObject(strNextTok);
						}

						nCurrentIndex++;
					}
				}
			}
			else
			if (strToken.GetAt(nl-1) == _T('.'))
			{
				ASSERT((nCurrentIndex +1) < nSize);
				if ((nCurrentIndex +1) >= nSize)
				{
					m_nArgs = -1;
					return FALSE;
				}

				nCurrentIndex++;
				CString strNextTok = arryToken.GetAt(nCurrentIndex);
				strFullName += strNextTok;

				if (pExactKey)
				{
					CString strOwner = strToken.Left(nl-1);
					CString strObject = RemoveQuote (strNextTok, _T('\"'));
					pExactKey->SetOwner(strOwner);
					pExactKey->SetObject(strObject);
				}
			}
		}
		pKey->SetValue(strFullName);
	}

	return TRUE;
}


