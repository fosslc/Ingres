/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : readflg.h, Header File.
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

#if !defined (READFLG_HEADER)
#define READFLG_HEADER
typedef byte* byteptr;
typedef short* shortptr;

#define SIZE_ARRAY_SECONDARY 2000
#define SIZE_ARRAY_BASEGROW  20


class CaReadFlag: public CObject
{
	DECLARE_SERIAL (CaReadFlag)
public:
	CaReadFlag(int nBaseGrow = SIZE_ARRAY_BASEGROW);
	virtual ~CaReadFlag();
	void Cleanup();
	void Init(int nBaseGrow = SIZE_ARRAY_BASEGROW);

	void SetAt(long nIndex, BOOL bFlag);
	short GetAt(long nIndex) const;

	long Add(long nIndex, BOOL bFlag = FALSE);
	long GetCount(){return m_nCount;}
	virtual void Serialize (CArchive& ar);

private:
	short** m_nArray;

	int  m_nCurrentSize;  // Size of the main array (m_nArray)
	int  m_nBaseGrow;     // m_nArray will grow at this size if needed
	int  m_nBase;         // index to indicate the use of the element of m_nArray
	int  m_nSndArraySize; // Size of array which is an element of m_nArray
	long m_nCount;        // Total number of elements.
};

#endif