/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : readmsg.cpp: implementation file
**    Project  : Ingres II/IVM
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Read and parse the message description text from file
**               $(II_SYSTEM)\IngresII\ingres\files\english\messages\messages.txt.
**               The file is maintained opened. Each message read is stored in a couple
**               (Text=MessageID, pos = file position).
**               The set of couples is stored in a sorted array (quick sort). 
**
** History:
**
** 12-Feb-2003 (uk$so01)
**    Created
**/


#include "stdafx.h"
#include "readmsg.h"
#include "tlsfunct.h"
#include "constdef.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define MAXMESSAGELEN  12000
//#define _TEST_FILEPOS_MAP

static int MatchMsg (MSGCLASSANDID* o1, MSGCLASSANDID* o2)
{
	if (o1->lMsgClass > o2->lMsgClass)
		return 1;
	else
	if (o1->lMsgClass < o2->lMsgClass)
		return -1;
	else
	{
		if (o1->lMsgFullID > o2->lMsgFullID)
			return 1;
		else
		if (o1->lMsgFullID < o2->lMsgFullID)
			return -1;
		else
			return 0;
	}
}

static int compareMsg (const void* e1, const void* e2)
{
	MSGxFILEPOSENTRY* p1 = (MSGxFILEPOSENTRY*)e1;
	MSGxFILEPOSENTRY* p2 = (MSGxFILEPOSENTRY*)e2;

	MSGCLASSANDID* o1 = &(p1->msg);
	MSGCLASSANDID* o2 = &(p2->msg);
	return MatchMsg(o1, o2);
}



static int DichoFind (MSGCLASSANDID* pMsg, MSGxFILEPOSENTRY* pArray, int first, int last)
{
	if (first <= last)
	{
		int mid = (first + last) / 2;
		int nFound = MatchMsg(&(pArray [mid].msg), pMsg);
		if (nFound == 0)
			return mid;
		else
		if (nFound > 0)
			return DichoFind (pMsg, pArray, first, mid-1);
		else
			return DichoFind (pMsg, pArray, mid+1, last);
	}
	return -1;
}

static void ReadInformation(FILE* pFile, CaMessageDescription& msgDescription)
{
	const int nCount = 4096;
	TCHAR tchszReturn[] = {0x0A, 0x0D, 0x00};
	TCHAR tchszText  [nCount];
	TCHAR* pLine = _fgetts(tchszText, nCount, pFile);
	CString strResult = _T("");
	CString strText = pLine;
	strText.TrimRight(consttchszReturn);
	msgDescription.SetMessage(strText); // Message.

	//
	// Read parameter:
	while ( !feof(pFile) ) 
	{
		pLine = _fgetts(tchszText, nCount, pFile);
		if (!pLine)
			break;
		strText = pLine;
		strText.TrimLeft(_T("\t "));
		if (strText.Find(_T('_')) == 1 && ((int)strText[0] >= 65 && (int)strText[0] <= 90) || feof(pFile))
		{
			strResult = _T("");
			return;;
		}

		strText.MakeLower();
		if (strText.Find(_T("parameters")) == 0)
		{
			//
			// Read parameters:
			strResult = _T("");
			continue;
		}
		else
		if (strText.Find(_T("explanation")) == 0)
		{
			strResult.TrimRight(tchszReturn);
			msgDescription.SetParemeter(strResult);
			strResult = _T("");
			break;
		}

		strText = pLine;
		strText.TrimLeft(_T("\t "));
		strText.TrimRight(consttchszReturn);
		strText += consttchszReturn;
		strResult += strText;
	}

	//
	// Read Explanation:
	while ( !feof(pFile) ) 
	{
		pLine = _fgetts(tchszText, nCount, pFile);
		if (!pLine)
			break;
		strText = pLine;
		strText.TrimLeft(_T("\t "));
		if (strText.Find(_T('_')) == 1 && ((int)strText[0] >= 65 && (int)strText[0] <= 90) || feof(pFile))
		{
			strResult = _T("");
			return;;
		}

		strText.MakeLower();
		if ( strText.Find(_T("system status")) == 0)
		{
			strResult.TrimRight(tchszReturn);
			msgDescription.SetDescription(strResult);
			strResult = _T("");
			break;
		}

		strText = pLine;
		strText.TrimLeft(_T("\t "));
		strText.TrimRight(consttchszReturn);
		strText += consttchszReturn;
		strResult += strText;
	}

	//
	// Read System Status:
	while ( !feof(pFile) ) 
	{
		pLine = _fgetts(tchszText, nCount, pFile);
		if (!pLine)
			break;
		strText = pLine;
		strText.TrimLeft(_T("\t "));
		if (strText.Find(_T('_')) == 1 && ((int)strText[0] >= 65 && (int)strText[0] <= 90) || feof(pFile))
		{
			strResult = _T("");
			return;;
		}

		strText.MakeLower();
		if ( strText.Find(_T("recommendation")) == 0)
		{
			strResult.TrimRight(tchszReturn);
			msgDescription.SetStatus(strResult);
			strResult = _T("");
			break;
		}

		strText = pLine;
		strText.TrimLeft(_T("\t "));
		strText.TrimRight(consttchszReturn);
		strText += consttchszReturn;
		strResult += strText;
	}

	//
	// Read Recommendation:
	while ( !feof(pFile) ) 
	{
		pLine = _fgetts(tchszText, nCount, pFile);
		if (!pLine)
			break;
		strText = pLine;
		if (feof(pFile) || strText.Find(_T('_')) == 1 && ((int)pLine[0] >= 65 && (int)pLine[0] <= 90)) // May be E_, W_, I_, S_
		{
			strResult.TrimRight(tchszReturn);
			msgDescription.SetRecommendation(strResult);
			strResult = _T("");
			break;
		}

		strText = pLine;
		strText.TrimLeft(_T("\t "));
		strText.TrimRight(consttchszReturn);
		strText += consttchszReturn;
		strResult += strText;
	}
}

CaMessageDescriptionManager::CaMessageDescriptionManager(LPCTSTR lpszFile)
{
	m_strFileName = lpszFile;
	m_pFile = NULL;
	m_pArrayFilePos = NULL;
	m_nArrayFilePosSize = 1500;
	m_nMsgIndex = -1;
}

CaMessageDescriptionManager::~CaMessageDescriptionManager()
{
	if (m_pFile)
		fclose(m_pFile);
	if (m_pArrayFilePos)
		delete m_pArrayFilePos;
}

void CaMessageDescriptionManager::Initialize()
{
	const int nCount = 4096;
	long lFilePosition = -1L;
	int  nLen = 0;
	TCHAR tchszBuffer [256];
	TCHAR tchszText  [nCount];

	if (m_pFile)
	{
		fclose(m_pFile);
		m_pFile = NULL;
	}
	if (m_pArrayFilePos)
		delete m_pArrayFilePos;
	m_pArrayFilePos = new MSGxFILEPOSENTRY[m_nArrayFilePosSize];
	m_nMsgIndex = -1;

	m_pFile = _tfopen((LPCTSTR)m_strFileName, _T("r"));
	if (m_pFile == NULL)
	{
		throw (int)100; // Cannot open file
	}

	CString strText = _T("");
	tchszText[0] = _T('\0');
	while ( !feof(m_pFile) ) 
	{
		lFilePosition = ftell(m_pFile);
		TCHAR* pLine = _fgetts(tchszText, nCount, m_pFile);
		if (!pLine)
			break; // Error or end of file
		nLen = _tcslen (tchszText);
		RemoveEndingCRLF(tchszText);
		if (!tchszText[0])
			continue;
		strText = tchszText;
		//
		// We limit to MAXMESSAGELEN characters per line:
		if (nLen < (nCount-1))
		{
			if (strText.Find(_T('_')) == 1 && ((int)strText[0] >= 65 && (int)strText[0] <= 90)) // May be E_, W_, I_, S_
			{
				MSGCLASSANDID msg = getMsgStringClassAndId(tchszText, tchszBuffer, sizeof(tchszBuffer));
				if (msg.lMsgClass != CLASS_OTHERS && msg.lMsgFullID != MSG_NOCODE)
				{
#if defined (_DEBUG) && defined (_TEST_FILEPOS_MAP)
					TRACE2 ("(POS, MSG) = %08X  %s\n", lFilePosition, strText);
#endif
					m_nMsgIndex++;
					if (m_nMsgIndex >= m_nArrayFilePosSize)
					{
						MSGxFILEPOSENTRY* pNewArrayFilePos = new MSGxFILEPOSENTRY[m_nArrayFilePosSize + 300];
						memmove(pNewArrayFilePos, m_pArrayFilePos, m_nArrayFilePosSize*sizeof(MSGxFILEPOSENTRY));
						m_nArrayFilePosSize += 300;
						delete m_pArrayFilePos;
						m_pArrayFilePos = pNewArrayFilePos;
					}

					memcpy ((void*)&(m_pArrayFilePos[m_nMsgIndex].msg), (void*)&msg, sizeof(MSGCLASSANDID));
					m_pArrayFilePos[m_nMsgIndex].lFilePosition = lFilePosition;
				}
				else
				{
#if defined (_DEBUG) && defined (_TEST_FILEPOS_MAP)
					TRACE2 ("!(POS, MSG) = %08X  %s\n", lFilePosition, strText);
#endif
				}
				strText = _T("");
			}
		}
		else
		{
			continue;
		}
	}

	if (m_nMsgIndex > 0)
	{
		qsort ((void*)m_pArrayFilePos, (size_t)(m_nMsgIndex+1), (size_t)sizeof(MSGxFILEPOSENTRY), compareMsg);
#if defined (_DEBUG) && defined (_TEST_FILEPOS_MAP)
		CString strMsg;
		for (int i=0; i< (m_nMsgIndex+1); i++)
		{
			strMsg.Format(
				_T("(MSG{class, ID}, POS) = {%08X %08X},     %08X\n"), 
				m_pArrayFilePos[i].msg.lMsgClass, 
				m_pArrayFilePos[i].msg.lMsgFullID,
				m_pArrayFilePos[i].lFilePosition);
			TRACE0(strMsg);
		}
#endif
	}
}

BOOL CaMessageDescriptionManager::Lookup (MSGCLASSANDID* pMessage, CaMessageDescription& msgDescription)
{
	int nFound = DichoFind (pMessage, m_pArrayFilePos, 0, m_nMsgIndex + 1);
	if (m_pFile && nFound >= 0 && nFound <= m_nMsgIndex)
	{
		if (fseek (m_pFile, m_pArrayFilePos[nFound].lFilePosition, SEEK_SET) == 0)
		{
			ReadInformation(m_pFile, msgDescription);
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
	return FALSE;
}


