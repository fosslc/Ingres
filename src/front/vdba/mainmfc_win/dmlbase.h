/*
**
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmlbase.h Header File
**
**
**    Project  : CA-OpenIngres
**    Author   : UK Sotheavut
**
**
**    Purpose  : Base class for Data manipulation
**
** History:
**
** 28-may-2001 (schph01)
**     BUG 104636 Add variable member m_nLength into CaColumn class to keep the actual
**     length of float column ( float4 or float8 ).
** 29-May-2001 (uk$so01)
**    SIR #104736, Integrate Ingres Import Assistant.
*/

#if !defined (DMLBASE_HEADER)
#define DMLBASE_HEADER


//
// Object: General (Item + Owner)
// -----------------------------
class CaDBObject: public CObject
{
	 DECLARE_SERIAL (CaDBObject)
public:
	CaDBObject(): m_strItem(""), m_strItemOwner(""), m_bSystem (FALSE), m_nOT(-1) {}
	CaDBObject(LPCTSTR lpszItem, LPCTSTR lpszItemOwner, BOOL bSystem = FALSE)
		: m_strItem(lpszItem), m_strItemOwner(lpszItemOwner), m_bSystem (bSystem), m_nOT(-1) {}
	
	virtual ~CaDBObject(){}
	virtual void Serialize (CArchive& ar);
	CString GetName(){return m_strItem;}
	CString GetOwner(){return m_strItemOwner;}

	CString m_strItem;        // The name of object
	CString m_strItemOwner;   // The owner of object;
	CTime   m_tTimeStamp;     // The time (the last access to object, local time)
	BOOL    m_bSystem;        // TRUE if Object is belong to the System.
	int     m_nOT;            // OBJect Type (if needed)
};


//
// Object: Column 
// --------------
class CaColumn: public CaDBObject
{
	DECLARE_SERIAL (CaColumn)
public:
	CaColumn(): CaDBObject(){}
	CaColumn(LPCTSTR lpszItem, BOOL bSystem = FALSE): CaDBObject(lpszItem, "", bSystem),m_nLength ( 0 ){}

	virtual ~CaColumn(){}
	virtual void Serialize (CArchive& ar);

	void SetLength(int nVal){m_nLength = nVal;}
	int  GetLength(){return m_nLength;}

	int     m_iDataType;    // Type code. For example IISQ_VCH_TYPE is 21, 
	CString m_strDataType;  // Type code. For example IISQ_VCH_TYPE is VARCHAR

	int     m_nLength;

};


class CaKey: public CObject
{
	DECLARE_SERIAL (CaKey)
public:
	CaKey():m_strKeyName(""){};
	CaKey(LPCTSTR lpszKeyName):m_strKeyName(lpszKeyName){};
	virtual ~CaKey(){};
	virtual void Serialize (CArchive& ar);


	CString     m_strKeyName;
	CStringList m_listColumn;

};


class CaForeignKey: public CaKey
{
	DECLARE_SERIAL (CaForeignKey)
public:
	CaForeignKey():CaKey() {}
	CaForeignKey(LPCTSTR lpszKeyName):CaKey(lpszKeyName) {}
	virtual ~CaForeignKey(){};
	virtual void Serialize (CArchive& ar);

	CString     m_strReferencedTable;
	CString     m_strReferencedTableOwner;
	CString     m_strReferencedKey;
	CStringList m_listReferringColumn;
};



#endif
