/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlselect.cpp, Implementation file 
**    Project  : CA-OpenIngres Visual DBA.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : cpp file that includes the Generated Embedded SQL file (sqlselec.inc)
**
** History: after 30-may-2001
**
** 30-May-2001 (uk$so01)
**    SIR #104736, Integrate IIA and Cleanup old code of populate.
** 21-Mar-2002 (uk$so01)
**    SIR #106648, Integrate (sqlquery).ocx.
*/


#include "stdafx.h"
#include "sqlselec.h"
#include "sqlselec.inc"


static TCHAR PeekNextChar (const TCHAR* lpcText)
{
	TCHAR* pch = (TCHAR*)lpcText;
	if (!pch)
		return _T('\0');
	pch = _tcsinc(pch);
	if (!pch)
		return _T('\0');
	return *pch;
}

static BOOL NextKeyWord(TCHAR* pch)
{
	CString strText = pch;
	if (strText.IsEmpty())
		return FALSE;
	if (strText[0] == _T(';'))
		return TRUE;
	if (strText.Find (_T("where ")) == 0)
		return TRUE;
	if (strText.Find (_T("group by ")) == 0)
		return TRUE;
	if (strText.Find (_T("having ")) == 0)
		return TRUE;
	if (strText.Find (_T("union ")) == 0)
		return TRUE;
	if (strText.Find (_T("order by ")) == 0)
		return TRUE;
	return FALSE;
}

static void ParseJoin(CStringList& listToken, CStringList& listSource)
{
	POSITION p, pos = listToken.GetTailPosition();
	while (pos != NULL)
	{
		p = pos;
		CString& strTok = listToken.GetPrev (pos);
		if (strTok.CompareNoCase(_T("on")) == 0)
		{
			//
			// When encounter a token "ON", everything including "ON" to the right is discarded
			// and removed out of the token list.
			while (listToken.GetTailPosition() != p)
			{
				listToken.RemoveTail();
			}
			
			listToken.RemoveAt(p);
			pos = listToken.GetTailPosition();
		}
		else
		if (strTok.CompareNoCase (_T("join")) == 0)
		{
			//
			// When encounter a token "JOIN", the token after the token "JOIN" is a table.
			// Every token, including "JOIN", is removed out of the token list.
			POSITION pNext = p;
			listToken.GetNext (pNext);
			CString& strTable = listToken.GetNext (pNext);
			if (!strTable.IsEmpty())
				listSource.AddHead(strTable);

			while (listToken.GetTailPosition() != p)
			{
				listToken.RemoveTail();
			}
			
			listToken.RemoveAt(p);
			pos = listToken.GetTailPosition();
		}
	}

	if (!listToken.IsEmpty())
	{
		pos = listToken.GetHeadPosition();

		CString& strTable = listToken.GetNext (pos);
		if (!strTable.IsEmpty())
			listSource.AddHead(strTable);
	}

	listToken.RemoveAll();
}


static void MakeTokenList (CString& strLine, CStringList& listToken, CString& strIgnoreChars, CString& strSep)
{
	CString strObject = _T("");
	TCHAR tchszC  = _T('\0');
	CString strPrev = _T("");
	TCHAR tchszTextQualifier = _T('\"');
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


	TCHAR* pch = (LPTSTR)(LPCTSTR)strLine;
	while (pch && *pch != _T('\0'))
	{
		tchszC = *pch;
		if (IsDBCSLeadByte(tchszC))
		{
			pch = _tcsinc(pch);
			strObject += tchszC;
			continue;
		}

		if (!stack.IsEmpty())
		{
			TCHAR tchTop = stack.Top();
			if (tchTop == tchszC && PeekNextChar(pch) != tchTop)
				stack.PopState();
			else
			if (tchTop == tchszC && PeekNextChar(pch) == tchTop)
			{
				strObject += tchszC;
				strPrev = tchszC;
				pch = _tcsinc(pch);
			}
			else
			{
				strObject += tchszC;
			}
		}
		else
		{
			if (!strIgnoreChars.IsEmpty() && strIgnoreChars.Find(tchszC) != -1)
			{
				//
				// Ignore the character in tchszC.
				strPrev = tchszC;
				pch = _tcsinc(pch);
				continue;
			}

			if (!strSep.IsEmpty() && strSep.Find(tchszC) != -1)
			{
				if (!strObject.IsEmpty())
					listToken.AddTail (strObject);
				strObject = _T("");
				strPrev = tchszC;
				pch = _tcsinc(pch);
				continue;
			}

			if (tchszC == _T('\"'))
			{
				stack.PushState(tchszC);

				strPrev = tchszC;
				pch = _tcsinc(pch);
				continue;
			}
			else
			{
				if (tchszC == _T(','))
				{
					if (!strObject.IsEmpty())
						listToken.AddTail (strObject);
					strObject = _T("");
				}
				else
				if (NextKeyWord(pch))
				{
					if (!strObject.IsEmpty())
						listToken.AddTail (strObject);
					strObject = _T("");
					return;
				}
				else
				{
					strObject+= tchszC;
				}
			}
		}

		strPrev = tchszC;
		pch = _tcsinc(pch);
		if (pch && *pch == _T('\0'))
		{
			if (!strObject.IsEmpty())
				listToken.AddTail (strObject);
			strObject = _T("");
		}
	}

}



static void ParseTable(CString& strLine, CStringList& listStrObject)
{
	CString strObject = _T("");
	TCHAR tchszC  = _T('\0');
	CString strPrev = _T("");
	TCHAR tchszTextQualifier = _T('\"');
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

	TCHAR* pch = (LPTSTR)(LPCTSTR)strLine;
	while (pch && *pch != _T('\0'))
	{
		tchszC = *pch;
		if (IsDBCSLeadByte(tchszC))
		{
			pch = _tcsinc(pch);
			strObject += tchszC;
			continue;
		}

		if (!stack.IsEmpty())
		{
			TCHAR tchTop = stack.Top();
			if (tchTop == tchszC && PeekNextChar(pch) != tchTop)
			{
				strObject += tchszC;
				stack.PopState();
			}
			else
			if (tchTop == tchszC && PeekNextChar(pch) == tchTop)
			{
				strObject += tchszC;
				strObject += tchszC;
				strPrev = tchszC;
				pch = _tcsinc(pch);
			}
			else
			{
				strObject += tchszC;
			}
		}
		else
		{
			if (tchszC == _T('\"'))
			{
				strObject += tchszC;
				stack.PushState(tchszC);

				strPrev = tchszC;
				pch = _tcsinc(pch);
				continue;
			}
			else
			{
				if (tchszC == _T(','))
				{
					strObject.TrimLeft();
					strObject.TrimRight();
					if (!strObject.IsEmpty())
						listStrObject.AddTail (strObject);
					strObject = _T("");
				}
				else
				if (NextKeyWord(pch))
				{
					strObject.TrimLeft();
					strObject.TrimRight();
					if (!strObject.IsEmpty())
						listStrObject.AddTail (strObject);

					strObject = _T("");
					return;
				}
				else
				{
					strObject+= tchszC;
				}
			}
		}

		strPrev = tchszC;
		pch = _tcsinc(pch);
		if (pch && *pch == _T('\0'))
		{
			strObject.TrimLeft();
			strObject.TrimRight();
			if (!strObject.IsEmpty())
				listStrObject.AddTail (strObject);
			strObject = _T("");
		}
	}
}


void ParseJoinTable (CString& strLine, CStringList& listSource)
{
	//
	// Parse the FROM clause:
	TCHAR tchszC  = _T('\0');
	POSITION pos;
	CString strIgnoreChars = _T("()");
	CString strSep = _T(" ");
	CString strStmt = strLine;
	strStmt.MakeLower();

	//
	// First of all, transform each character nside "" to x, so that the Find() function
	// won't return the match string inside "".
	// Ex: select * from "my tb from uk" will be transformed to select * from "xxxxxxxxxxxxx"
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

	TCHAR* pch = (LPTSTR)(LPCTSTR)strStmt;
	while (pch && *pch != _T('\0'))
	{
		tchszC = *pch;
		if (IsDBCSLeadByte(tchszC))
		{
			pch = _tcsinc(pch);
			continue;
		}

		if (!stack.IsEmpty())
		{
			TCHAR tchTop = stack.Top();
			if (tchTop == tchszC && PeekNextChar(pch) != tchTop)
			{
				stack.PopState();
			}
			else
			if (tchTop == tchszC && PeekNextChar(pch) == tchTop)
			{
				*pch = _T('x');
				pch = _tcsinc(pch);
				*pch = _T('x');
			}
			else
			{
				*pch = _T('x');
			}
		}
		else
		{
			if (tchszC == _T('\"'))
			{
				stack.PushState(tchszC);

				pch = _tcsinc(pch);
				continue;
			}
		}

		pch = _tcsinc(pch);
	}

	//
	// Find the " from " in the select:
	CArray <int, int> arrayPos;
	int nLen = strStmt.GetLength();
	int n = strStmt.Find (_T(" from "));
	while (n != -1)
	{
		arrayPos.Add (n);
		if ((n + 6) < nLen)
			n = strStmt.Find (_T(" from "), n + 6);
		else
			n = -1;
	}
	
	//
	// Restore to the origin string:
	strStmt = strLine;
	strStmt.MakeLower();

	int i, nSize = arrayPos.GetSize();
	for (i=nSize-1; i>=0; i--)
	{
		n = arrayPos.GetAt(i);
		CString strObject = _T("");
		CString strObjectOwner = _T("");
		CStringList listStrObject;

		CString strPart = strStmt.Mid (n + 6);
		strStmt = strStmt.Left (n);

		ParseTable(strPart, listStrObject);
		pos = listStrObject.GetHeadPosition();
		while (pos != NULL)
		{
			CString strObject = listStrObject.GetNext(pos);
			TRACE1("Source = %s\n", (LPCTSTR)strObject);

			CStringList listToken;
			MakeTokenList (strObject, listToken, strIgnoreChars, strSep);

			ParseJoin(listToken, listSource);
			listToken.RemoveAll();
		}
	}

	pos = listSource.GetHeadPosition();
	while (pos != NULL)
	{
		CString& strSource = listSource.GetNext (pos);

		TRACE1("\tTABLE=%s\n", (LPCTSTR)strSource);
	}
}

