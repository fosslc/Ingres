/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmlindex.h: interface for the CaIndex class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Index object
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/


#if !defined (DMLINDEX_HEADER)
#define DMLINDEX_HEADER
#include "dmlbase.h"
#include "dmltable.h"

//
// Object: Index 
// ************************************************************************************************
class CaIndex: public CaDBObject
{
	DECLARE_SERIAL (CaIndex)
public:
	CaIndex():CaDBObject() {SetObjectID(OBT_INDEX);}
	CaIndex(LPCTSTR lpszItem, LPCTSTR lpszOwner, BOOL bSystem = FALSE):CaDBObject(lpszItem, lpszOwner, bSystem) 
	{
		SetObjectID(OBT_INDEX);
	};
	CaIndex(const CaIndex& c);
	CaIndex operator =(const CaIndex& c);
	virtual ~CaIndex();
	virtual void Serialize (CArchive& ar);
	BOOL Matched (CaIndex* pObj, MatchObjectFlag nFlag = MATCHED_NAME);

	int  GetFlag(){return m_nFlag;}
	void SetFlag(int nVal){ m_nFlag = nVal;}

protected:
	void Copy(const CaIndex& c);
	int  m_nFlag;           // OBJTYPE_UNKNOWN, OBJTYPE_STARNATIVE, OBJTYPE_STARLINK

};



//
// Object: Index Detail 
// ************************************************************************************************
class CaIndexDetail: public CaIndex
{
	DECLARE_SERIAL (CaIndexDetail)
public:
	CaIndexDetail():CaIndex() {}
	CaIndexDetail(LPCTSTR lpszItem, LPCTSTR lpszOwner, BOOL bSystem = FALSE):CaIndex(lpszItem, lpszOwner, bSystem){};
	CaIndexDetail(const CaIndexDetail& c);
	CaIndexDetail operator =(const CaIndexDetail& c);
	virtual ~CaIndexDetail();
	virtual void Serialize (CArchive& ar);
	BOOL Matched (CaIndexDetail* pObj, MatchObjectFlag nFlag = MATCHED_NAME);

	CaStorageStructure& GetStorageStructure(){return m_storageStructure;}
	CTypedPtrList < CObList, CaColumnKey* >& GetListColumns() {return m_listColumn;}

	void SetPersistent(TCHAR chSet){m_cPersistent = chSet;}
	void SetCreateDate(LPCTSTR lpszText){m_strCreateDate = lpszText;}
	void SetBaseTable (LPCTSTR lpszTable, LPCTSTR lpszTableOwner)
	{
		m_strBaseTable = lpszTable;
		m_strBaseTableOwner = lpszTableOwner;
	}
	TCHAR GetPersistent(){return m_cPersistent;}
	CString GetCreateDate(){return m_strCreateDate;}
	CString GetBaseTable (){return m_strBaseTable;}
	CString GetBaseTableOwner (){return m_strBaseTableOwner;}

protected:
	void Copy(const CaIndexDetail& c);

	CString m_strBaseTable;
	CString m_strBaseTableOwner;
	CTypedPtrList < CObList, CaColumnKey* > m_listColumn;
	CString m_strCreateDate;
	TCHAR   m_cPersistent;  // Y=if the index re-created after a modify of the table, N=if not
	CaStorageStructure m_storageStructure;
};


#endif // DMLINDEX_HEADER
