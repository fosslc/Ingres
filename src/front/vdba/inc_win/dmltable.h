/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmltable.h: interface for the CaTable class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Table object
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
** 27-Feb-2003 (schph01)
**    SIR #109220 Manage compare locations in VDDA
**/

#if !defined (DMLTABLE_HEADER)
#define DMLTABLE_HEADER
#include "dmlbase.h"
#include "dmlcolum.h"

class CaIndexOption: public CObject
{
	DECLARE_SERIAL (CaIndexOption)
public:
	CaIndexOption();
	CaIndexOption(const CaIndexOption& c);
	CaIndexOption operator = (const CaIndexOption& c);
	virtual ~CaIndexOption(){}
	virtual void Serialize (CArchive& ar);
	virtual UINT GetObjectVersion(){return 1;}

	void SetStructure(LPCTSTR lpszStructure)
	{
		m_strStructure = lpszStructure;
		m_strStructure.TrimRight();
	}
	void SetFillFactor(int nVal){m_nFillFactor = nVal;}
	void SetMinPage(int nVal){m_nMinPage = nVal;}
	void SetMaxPage(int nVal){m_nMaxPage = nVal;}
	void SetLeafFill(int nVal){m_nLeafFill = nVal;}
	void SetNonLeafFill(int nVal){m_nNonLeafFill = nVal;}
	void SetAllocation(int nVal){m_nAllocation = nVal;}
	void SetExtend(int nVal){m_nExtend = nVal;}

	CString GetStructure(){return m_strStructure;}
	int GetFillFactor(){return m_nFillFactor;}
	int GetMinPage(){return m_nMinPage;}
	int GetMaxPage(){return m_nMaxPage ;}
	int GetLeafFill(){return m_nLeafFill;}
	int GetNonLeafFill(){return m_nNonLeafFill;}
	int GetAllocation(){return m_nAllocation ;}
	int GetExtend(){return m_nExtend;}


	void SetIndexName(LPCTSTR lpszName){m_strIndexName = lpszName;}
	CString GetIndexName(){return m_strIndexName;}
	CStringList& GetListLocation(){return m_listLocation;}

private:
	CString m_strIndexName;
	CStringList m_listLocation;

protected:
	void Copy (const CaIndexOption& c);

	int m_nFillFactor; // ISAM, HASH, BTREE
	int m_nMinPage;    // HASH
	int m_nMaxPage;    // HASH
	int m_nLeafFill;   // BTREE
	int m_nNonLeafFill;// BTREE
	int m_nAllocation;
	int m_nExtend;
	CString m_strStructure;
};

//
// Object: key column object 
// ************************************************************************************************
class CaColumnKey: public CaDBObject
{
	DECLARE_SERIAL (CaColumnKey)
public:
	CaColumnKey():CaDBObject(){}
	CaColumnKey(LPCTSTR lpszName, LPCTSTR lpszType, int nDataType = -1):CaDBObject(lpszName, lpszType, FALSE)
	{
		m_iDataType = nDataType;
		m_nLength = 0;
		m_nScale = 0;
		m_nOrder = 0;
	}
	CaColumnKey(const CaColumnKey& c);
	CaColumnKey operator = (const CaColumnKey& c);

	virtual ~CaColumnKey(){}
	virtual void Serialize (CArchive& ar);
	void SetLength(int nVal){m_nLength = nVal;}
	void SetScale (int nVal){m_nScale = nVal;}
	void SetKeyOrder (int nVal){m_nOrder = nVal;}
	
	int GetType(){return m_iDataType;}
	int GetLength(){return m_nLength;}
	int GetScale (){return m_nScale;}
	int GetKeyOrder (){return m_nOrder;}
	CString GetTypeStr(){return m_strItemOwner;}

protected:
	void Copy(const CaColumnKey& c);

	int m_nLength;      // Lenght if data type requires
	int m_nScale;       // If data type requires. Ex for dacimal, then DECIMAL (m_nLength, m_nScale)
	int m_iDataType;    // Type code. For example IISQ_VCH_TYPE is 21, 
	int m_nOrder;       // Position of columns

public:
	static CaColumnKey* FindColumn(LPCTSTR lpszName, CTypedPtrList< CObList, CaColumnKey* >& listColumn);
};



//
// Object: Primary key object 
// ************************************************************************************************
class CaPrimaryKey: public CaDBObject
{
	DECLARE_SERIAL (CaPrimaryKey)
public:
	CaPrimaryKey():CaDBObject(), m_strConstraintText(_T("")){}
	CaPrimaryKey(LPCTSTR lpszName):CaDBObject()
	{
		m_strConstraintName = lpszName;
		m_strConstraintName.TrimRight();
		m_strConstraintText = _T("");
	}
	CaPrimaryKey(const CaPrimaryKey& c);
	CaPrimaryKey operator = (const CaPrimaryKey& c);

	virtual ~CaPrimaryKey()
	{
		while (!m_listColumn.IsEmpty())
			delete m_listColumn.RemoveHead();
	}
	
	virtual void Serialize (CArchive& ar);
	void SetConstraintName(LPCTSTR lpszSet)
	{
		m_strConstraintName = lpszSet;
		m_strConstraintName.TrimRight();
	}
	void SetTable (LPCTSTR lpszName, LPCTSTR lpszOwner){SetItem(lpszName, lpszOwner);}

	CString GetConstraintName(){return m_strConstraintName;}
	CString GetConstraintText(){return m_strConstraintText;}
	CTypedPtrList < CObList, CaColumnKey* >& GetListColumn() {return m_listColumn;}
	CaIndexOption& GetWithClause() {return m_constraintWithClause;}

	void SetConstraint(LPCTSTR lpszText)
	{
		m_strConstraintText = lpszText;
		m_strConstraintText.TrimRight();
	}
	void ConcatStatement(LPCTSTR lpszText)
	{
		m_strConstraintText += lpszText;
		m_strConstraintText.TrimLeft();
		m_strConstraintText.TrimRight();
	}
protected:
	void Copy(const CaPrimaryKey& c);

	CString m_strConstraintName;
	CString m_strConstraintText;
	CTypedPtrList < CObList, CaColumnKey* > m_listColumn;
	CaIndexOption m_constraintWithClause;
};


//
// Object: Unique key object 
// ************************************************************************************************
class CaUniqueKey: public CaPrimaryKey
{
	DECLARE_SERIAL (CaUniqueKey)
public:
	CaUniqueKey():CaPrimaryKey(){}
	CaUniqueKey(LPCTSTR lpszName):CaPrimaryKey(lpszName){}
	virtual ~CaUniqueKey(){}
	
	virtual void Serialize (CArchive& ar){CaPrimaryKey::Serialize(ar);}
	static CaUniqueKey* FindUniqueKey(CTypedPtrList < CObList, CaUniqueKey* >& l, CaUniqueKey* pKey);
};


//
// Object: Foreign key object 
// ************************************************************************************************
class CaForeignKey: public CaPrimaryKey
{
	DECLARE_SERIAL (CaForeignKey)
public:
	CaForeignKey():CaPrimaryKey(){}
	CaForeignKey(LPCTSTR lpszName, LPCTSTR lpszReferedName):CaPrimaryKey(lpszName)
	{
		m_strReferedConstraintName = lpszReferedName;
		m_strReferedConstraintName.TrimRight();
	}
	CaForeignKey(const CaForeignKey& c);
	CaForeignKey operator = (const CaForeignKey& c);

	virtual ~CaForeignKey()
	{
		while (!m_listReferedColumn.IsEmpty())
			delete m_listReferedColumn.RemoveHead();
	}
	
	virtual void Serialize (CArchive& ar);
	static CaForeignKey* FindForeignKey(CTypedPtrList < CObList, CaForeignKey* >& l, CaForeignKey* pKey);

	void SetReferedTable (LPCTSTR lpszName, LPCTSTR lpszOwner)
	{
		m_strReferedTable = lpszName;
		m_strReferedTableOwner = lpszOwner;
		m_strReferedTable.TrimRight();
		m_strReferedTableOwner.TrimRight();
	}

	CString GetReferedTable(){return m_strReferedTable;}
	CString GetReferedTableOwner(){return m_strReferedTableOwner;}
	CString GetReferedConstraintName(){return m_strReferedConstraintName;}
	CTypedPtrList < CObList, CaColumnKey* >& GetReferedListColumn() {return m_listReferedColumn;}

protected:
	void Copy(const CaForeignKey& c);

	CString m_strReferedConstraintName;
	CString m_strReferedTable;
	CString m_strReferedTableOwner;
	CTypedPtrList < CObList, CaColumnKey* > m_listReferedColumn;
};


//
// Object: Check key object 
// ************************************************************************************************
class CaCheckKey: public CaDBObject
{
	DECLARE_SERIAL (CaCheckKey)
public:
	CaCheckKey():CaDBObject(){}
	CaCheckKey(LPCTSTR lpszName):CaDBObject()
	{
		m_strConstraintName = lpszName;
		m_strConstraintName.TrimRight();
	}
	CaCheckKey(const CaCheckKey& c);
	CaCheckKey operator = (const CaCheckKey& c);

	virtual ~CaCheckKey()
	{
	}
	virtual void Serialize (CArchive& ar);
	static CaCheckKey* FindCheckKey(CTypedPtrList < CObList, CaCheckKey* >& l, CaCheckKey* pKey);

	void SetConstraintName(LPCTSTR lpszSet)
	{
		m_strConstraintName = lpszSet;
		m_strConstraintName.TrimRight();
	}
	void SetStatement(LPCTSTR lpszSet)
	{
		m_strStatement = lpszSet;
		m_strStatement.TrimRight();
	}
	void ConcatStatement(LPCTSTR lpszSet)
	{
		m_strStatement += lpszSet;
		m_strStatement.TrimRight();
	}

	void SetTable (LPCTSTR lpszName, LPCTSTR lpszOwner){SetItem(lpszName, lpszOwner);}

	CString GetConstraintName(){return m_strConstraintName;}
	CString GetConstraintText(){return m_strStatement;}
	CaIndexOption& GetWithClause() {return m_constraintWithClause;}

protected:
	void Copy(const CaCheckKey& c);

	CString m_strConstraintName;
	CString m_strStatement;
	CaIndexOption m_constraintWithClause;
};


//
// Object: Storage Structure 
// ************************************************************************************************
class CaStorageStructure: public CaIndexOption
{
	DECLARE_SERIAL (CaStorageStructure)
public:
	CaStorageStructure():CaIndexOption(){m_nPageSize = 0;}
	CaStorageStructure(const CaStorageStructure& c);
	CaStorageStructure operator = (const CaStorageStructure& c);
	virtual ~CaStorageStructure(){}
	virtual void Serialize (CArchive& ar);

	//
	// SET:
	void SetPageSize(int nVal){m_nPageSize = nVal;}
	void SetUniqueScope(TCHAR chSet){m_cUniqueScope = chSet;}
	void SetCompress(TCHAR chSet){m_cIsCompress = chSet;}
	void SetKeyCompress(TCHAR chSet){m_cKeyCompress = chSet;}
	void SetAllocatedPage(int nVal){m_nAllocatedPage = nVal;}
	void SetUniqueRule(TCHAR chSet){m_cUniqueRule = chSet;}
	void SetPriorityCache(int iVal){m_nCachePriority = iVal;}

	//
	// GET:
	int GetPageSize(){return m_nPageSize;}
	TCHAR GetUniqueScope(){return m_cUniqueScope;}
	TCHAR GetCompress(){return m_cIsCompress;}
	TCHAR GetKeyCompress(){return m_cKeyCompress;}
	int GetAllocatedPage(){return m_nAllocatedPage;}
	TCHAR GetUniqueRule(){return m_cUniqueRule;}
	int GetPriorityCache(){return m_nCachePriority;}

protected:
	void Copy(const CaStorageStructure& c);
	void Init();

	int m_nPageSize;
	TCHAR m_cUniqueScope;   // ISAM,BTREE (R = row, S = statement, blank=not applicable)
	TCHAR m_cIsCompress;    // Y=Compress, N=uncompress, blank=unknown
	TCHAR m_cKeyCompress;   // Y=uses key compression, N=no key compression, blank=unknown
	int m_nAllocatedPage;
	TCHAR m_cUniqueRule;   // D, U, blank
	int   m_nCachePriority; //Indicates a table’s priority in the buffer cache.
	                        //Values can be between 0 - 9.
};


//
// Object: Table 
// ************************************************************************************************
class CaTable: public CaDBObject
{
	DECLARE_SERIAL (CaTable)
public:
	CaTable():CaDBObject() {Initialize();}
	CaTable(LPCTSTR lpszItem, LPCTSTR lpszOwner, BOOL bSystem = FALSE):CaDBObject(lpszItem, lpszOwner, bSystem) 
	{
		Initialize();
	}
	CaTable (const CaTable& c);
	CaTable operator = (const CaTable& c);

	void Initialize();
	BOOL Matched (CaTable* pObj, MatchObjectFlag nFlag = MATCHED_NAME);

	virtual ~CaTable();
	virtual void Serialize (CArchive& ar);

	int  GetFlag(){return m_nFlag;}
	void SetFlag(int nVal){ m_nFlag = nVal;}

protected:
	void Copy (const CaTable& c);
	int  m_nFlag;           // OBJTYPE_UNKNOWN, OBJTYPE_STARNATIVE, OBJTYPE_STARLINK
};

//
// Object: TableDetail
// ************************************************************************************************
enum QueryFlag
{
	DTQUERY_PROPERTY    = 1,
	DTQUERY_COLUMNS     = DTQUERY_PROPERTY << 1,
	DTQUERY_PRIMARYKEY  = DTQUERY_COLUMNS << 1,
	DTQUERY_UNIQUEKEY   = DTQUERY_PRIMARYKEY << 1,
	DTQUERY_FOREIGNKEY  = DTQUERY_UNIQUEKEY << 1,
	DTQUERY_CHECKKEY    = DTQUERY_FOREIGNKEY << 1
};
#define DTQUERY_ALL (DTQUERY_PROPERTY|DTQUERY_COLUMNS|DTQUERY_PRIMARYKEY|DTQUERY_UNIQUEKEY|DTQUERY_FOREIGNKEY|DTQUERY_CHECKKEY)


class CaTableDetail: public CaTable
{
	DECLARE_SERIAL (CaTableDetail)
public:

	CaTableDetail():CaTable() {Initialize();}
	CaTableDetail(LPCTSTR lpszItem, LPCTSTR lpszOwner, BOOL bSystem = FALSE):CaTable(lpszItem, lpszOwner, bSystem) 
	{
		Initialize();
	}
	CaTableDetail (const CaTableDetail& c);
	CaTableDetail operator = (const CaTableDetail& c);
	void Initialize();
	virtual ~CaTableDetail();
	virtual void Serialize (CArchive& ar);
	BOOL Matched (CaTable* pObj, MatchObjectFlag nFlag = MATCHED_NAME);

	CTypedPtrList < CObList, CaColumn* >& GetListColumns(){return m_listColumn;}
	BOOL GenerateSyntaxCreate(CString& strSyntax);
	CaStorageStructure& GetStorageStructure(){return m_storageStructure;}
	void SetQueryFlag(UINT nSet){m_nQueryFlag = nSet;}
	UINT GetQueryFlag(){return m_nQueryFlag;}
	CaPrimaryKey& GetPrimaryKey(){return m_primaryKey;}
	CTypedPtrList < CObList, CaUniqueKey* >& GetUniqueKeys(){return m_listUniqueKey;}
	CTypedPtrList < CObList, CaForeignKey* >& GetForeignKeys(){return m_listForeignKey;}
	CTypedPtrList < CObList, CaCheckKey* >& GetCheckKeys(){return m_listCheckKey;}

	void SetJournaling (TCHAR tchSet){m_cJournaling = tchSet;}
	void SetReadOnly (TCHAR tchSet){m_cReadOnly = tchSet;}
	void SetDuplicatedRows (TCHAR tchSet){m_cDuplicatedRows = tchSet;}
	void SetEstimatedRowCount(int nSet){m_nEstimatedRowCount = nSet;}
	void SetComment(LPCTSTR lpszSet){m_strComment = lpszSet;}
	void SetExpireDate(LPCTSTR lpszSet){m_strExpireDate = lpszSet;}

	TCHAR GetJournaling (){return m_cJournaling;}
	TCHAR GetReadOnly (){return m_cReadOnly;}
	TCHAR GetDuplicatedRows(){return m_cDuplicatedRows;}
	int   GetEstimatedRowCount(){return m_nEstimatedRowCount;}
	CString GetComment(){return m_strComment;}
	CString GetExpireDate(){return m_strExpireDate;}

	void GenerateListLocation(CString& csListLoc);
	CStringList& GetListLocation(){return m_listLocation;}

	void SetTablePhysInConsistent(BOOL bVal){m_bTablePhysInConsistent=bVal;}
	void SetTableLogInConsistent(BOOL bVal){m_bTableLogInConsistent=bVal;}
	void SetTableRecoveryDisallowed(BOOL bVal){m_bTableRecoveryDisallowed=bVal;}

	BOOL GetTablePhysInConsistent(){return m_bTablePhysInConsistent;}
	BOOL GetTableLogInConsistent(){return m_bTableLogInConsistent;}
	BOOL GetTableRecoveryDisallowed(){return m_bTableRecoveryDisallowed;}

protected:
	void Copy (const CaTableDetail& c);

	CTypedPtrList < CObList, CaColumn* > m_listColumn;
	CString m_strComment;
	TCHAR   m_cJournaling;        // Y=Active, N=Inactive, C=After CKPDB
	TCHAR   m_cReadOnly;          // Y=Read only, N=No
	TCHAR   m_cDuplicatedRows;    // D=Allow duplicated rows, U= Not allow, blank=unknown
	int     m_nEstimatedRowCount;
	CString m_strExpireDate;

	CStringList m_listLocation;

	CaStorageStructure m_storageStructure;

	BOOL  m_bTablePhysInConsistent;
	BOOL  m_bTableLogInConsistent;
	BOOL  m_bTableRecoveryDisallowed;

	//
	// List of constraints
	CaPrimaryKey m_primaryKey;
	CTypedPtrList < CObList, CaUniqueKey* >  m_listUniqueKey;
	CTypedPtrList < CObList, CaForeignKey* > m_listForeignKey;
	CTypedPtrList < CObList, CaCheckKey* >   m_listCheckKey;

private:
	UINT m_nQueryFlag;
};

#endif // DMLTABLE_HEADER
