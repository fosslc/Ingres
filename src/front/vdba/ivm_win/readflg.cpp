/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : readflg.cpp, Implementation File.
**    Project  : Ingres II / Visual Manager 
**    Author   : Sotheavut UK (uk$so01) 
** 
**    Purpose  : Array of boolean to indicate if an event has been read or not 
**               The zero base index indicate the event.
**
** History:
**
** 15-Mar-1999 (uk$so01)
**    created.
** 07-May-2003 (uk$so01)
**    BUG #110195, issue #11179264. Ivm show the alerted messagebox to indicate that
**    there are alterted messages but the are not displayed in the right pane
**    according to the preferences
*/

#include "stdafx.h"
#include "readflg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_SERIAL (CaReadFlag, CObject, 1)
CaReadFlag::CaReadFlag(int nBaseGrow)
{
	m_nArray = NULL;
	Init(nBaseGrow);
}

void CaReadFlag::Init(int nBaseGrow)
{
	if (m_nArray)
		return;
	m_nBaseGrow = nBaseGrow;
	m_nCurrentSize = m_nBaseGrow;
	m_nBase = 0;
	m_nSndArraySize = SIZE_ARRAY_SECONDARY;

	m_nArray = new shortptr[m_nCurrentSize*sizeof (short*)];
	short* pArray = new short [m_nSndArraySize];
	for (int i=0; i<m_nSndArraySize; i++)
	{
		pArray[i] = -1;
	}
	m_nArray [m_nBase]=pArray;
	m_nCount = 0;
}


CaReadFlag::~CaReadFlag()
{
	Cleanup();
}

void CaReadFlag::Cleanup()
{
	for (int i=0; i<=m_nBase; i++)
	{
		short* pSndArray = (short*)m_nArray[i];
		if (pSndArray)
			delete []pSndArray;
	}
	delete []m_nArray;
	m_nArray = NULL;
}


short CaReadFlag::GetAt(long nIndex) const
{
	short bf = -1;
	int base = nIndex/m_nSndArraySize;
	ASSERT (base <= m_nBase);
	if (base <= m_nBase)
	{
		int i = nIndex - (base*m_nSndArraySize);
		short* pSndArray = (short*)m_nArray[base];
		if (pSndArray)
			return pSndArray[i];
		else
			return -1;
	}
	return bf;
}

long CaReadFlag::Add(long nIndex, BOOL bFlag)
{
	int base = nIndex/m_nSndArraySize;
	if (base <= m_nBase && GetAt (nIndex) != -1)
		return -1;

	SetAt (nIndex, bFlag);
	m_nCount++;
	return GetCount();
}


void CaReadFlag::SetAt(long nIndex, BOOL bFlag)
{
	int base = nIndex/m_nSndArraySize;
	while (base > m_nBase)
	{
		short* pArray = new short [m_nSndArraySize];
		for (int i=0; i<m_nSndArraySize; i++)
		{
			pArray[i] = -1;
		}
		m_nBase++;
		if (m_nBase >= m_nCurrentSize)
		{
			m_nCurrentSize +=m_nBaseGrow; 
			short** pNewArray = new shortptr[m_nCurrentSize*sizeof (short*)];
			memmove (pNewArray, m_nArray, m_nBase*sizeof(short*));
			delete m_nArray;
			m_nArray = pNewArray;
		}
		m_nArray[m_nBase] = pArray;
	}

	if (base <= m_nBase)
	{
		int i = nIndex - (base*m_nSndArraySize);
		short* pSndArray = (short*)m_nArray[base];
		if (pSndArray)
			pSndArray[i] = (short)bFlag;
		else
		{
			//
			// No Entry:
			ASSERT (FALSE);
		}
	}
}

void CaReadFlag::Serialize (CArchive& ar)
{
	TCHAR tchszMsg[] = _T("CaReadFlag::Serialize: Failed to serialize the read/unread event flags");
	try
	{
		int i;
		if (ar.IsStoring())
		{
			ar << m_nCurrentSize;
			ar << m_nBaseGrow;
			ar << m_nBase;
			ar << m_nSndArraySize;
			ar << m_nCount;
			for (i=0; i<=m_nBase; i++)
			{
				ar.Write (m_nArray[i], m_nSndArraySize*sizeof(short));
			}
		}
		else
		{
			for (i=0; i<=m_nBase; i++)
			{
				short* pSndArray = (short*)m_nArray[i];
				if (pSndArray)
					delete []pSndArray;
			}
			delete []m_nArray;

			ar >> m_nCurrentSize;
			ar >> m_nBaseGrow;
			ar >> m_nBase;
			ar >> m_nSndArraySize;
			ar >> m_nCount;
			m_nArray = new shortptr[m_nCurrentSize*sizeof (short*)];

			for (i=0; i<=m_nBase; i++)
			{
				short* pArray = new short [m_nSndArraySize];
				m_nArray[i] = pArray;
				ar.Read (m_nArray[i], m_nSndArraySize*sizeof(short));
			}
		}
	}
	catch (...)
	{
		MessageBeep (MB_ICONEXCLAMATION);
		AfxMessageBox (tchszMsg);
	}
}
