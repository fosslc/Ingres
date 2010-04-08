/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmlseq.h: interface for the CaSequence and CaSequenceDetail class
**    Project  : Meta data library 
**    Author   : Philippe Schalk (schph01)
**    Purpose  : Sequence object
**
** History:
**
** 22-Apr-2003 (schph01)
**    SIR 107523 Add sequence Object
**/


#if !defined (DMLSEQ_HEADER)
#define DMLSEQ_HEADER
#include "dmlbase.h"


//
// Object: Sequence 
// ************************************************************************************************
class CaSequence: public CaDBObject
{
	DECLARE_SERIAL (CaSequence)
public:
	CaSequence():CaDBObject() {SetObjectID(OBT_SEQUENCE);}
	CaSequence(LPCTSTR lpszItem, LPCTSTR lpszOwner, BOOL bSystem = FALSE)
		:CaDBObject(lpszItem, lpszOwner, bSystem) {SetObjectID(OBT_SEQUENCE);};
	CaSequence (const CaSequence& c);
	CaSequence operator = (const CaSequence& c);

	virtual ~CaSequence(){}
	virtual void Serialize (CArchive& ar);
	BOOL Matched (CaSequence* pObj, MatchObjectFlag nFlag = MATCHED_NAME);

	int  GetFlag(){return m_nFlag;}
	void SetFlag(int nVal){ m_nFlag = nVal;}

protected:
	void Copy(const CaSequence& c);

	int  m_nFlag;           // OBJTYPE_UNKNOWN, OBJTYPE_STARNATIVE, OBJTYPE_STARLINK, OBJTYPE_NOTSTAR
};


//
// Object: Sequence Detail
// ************************************************************************************************
class CaSequenceDetail: public CaSequence
{
	DECLARE_SERIAL (CaSequenceDetail)
public:
	CaSequenceDetail():CaSequence() {}
	CaSequenceDetail(LPCTSTR lpszItem, LPCTSTR lpszOwner, BOOL bSystem = FALSE)
		:CaSequence(lpszItem, lpszOwner, bSystem) {};
	CaSequenceDetail (const CaSequenceDetail& c);
	CaSequenceDetail operator = (const CaSequenceDetail& c);

	virtual ~CaSequenceDetail(){}
	virtual void Serialize (CArchive& ar);
	BOOL Matched (CaSequenceDetail* pObj, MatchObjectFlag nFlag = MATCHED_NAME);

	void SetDecimalType(BOOL bVal){m_bDecimalType = bVal;}
	BOOL GetDecimalType(){return m_bDecimalType;}

	BOOL    GetCycle(){return m_bCycle;}
	void    SetCycle(BOOL bVal){m_bCycle = bVal;}

	BOOL    GetOrder(){return m_bOrder;}
	void    SetOrder(BOOL bVal){m_bOrder = bVal;}

	CString GetMaxValue(){return m_csMaxValue;}
	void    SetMaxValue(LPCTSTR lpStr){m_csMaxValue = lpStr;}

	CString GetMinValue(){return m_csMinValue;}
	void    SetMinValue(LPCTSTR lpStr){m_csMinValue = lpStr;}

	CString GetStartWith(){return m_csStartWith;}
	void    SetStartWith(LPCTSTR lpStr){m_csStartWith= lpStr;}

	CString GetIncrementBy(){return m_csIncrementBy;}
	void    SetIncrementBy(LPCTSTR lpStr){m_csIncrementBy= lpStr;}

	CString GetNextValue(){return m_csNextValue;}
	void    SetNextValue(LPCTSTR lpStr){m_csNextValue= lpStr;}

	CString GetCacheSize(){return m_csCacheSize;}
	void    SetCacheSize(LPCTSTR lpStr){m_csCacheSize= lpStr;}

	CString GetDecimalPrecision(){return m_csDecimalPrecision;}
	void    SetDecimalPrecision(LPCTSTR lpStr){m_csDecimalPrecision = lpStr;}

protected:
	void Copy(const CaSequenceDetail& c);

	BOOL    m_bDecimalType;
	BOOL    m_bCycle;
	BOOL    m_bOrder;

	CString m_csMaxValue;
	CString m_csMinValue;
	CString m_csStartWith;
	CString m_csIncrementBy;
	CString m_csNextValue;
	CString m_csCacheSize;
	CString m_csDecimalPrecision;
};


#endif // DMLSEQ_HEADER
