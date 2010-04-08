/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmlbase.h , Implementation File 
**    Project  : Ingres II / Visual DBA
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Base class of database object
**
** History:
**
** 05-Jan-1998 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/

#if !defined (DMLBASE_HEADER)
#define DMLBASE_HEADER
#include "constdef.h"

typedef enum
{
	MATCHED_NAME = 0,
	MATCHED_NAMExOWNER,
	MATCHED_ALL
} MatchObjectFlag;

//
// Object: General (Item + Owner)
// *****************************
class CaDBObject: public CObject
{
	DECLARE_SERIAL (CaDBObject)
public:
	CaDBObject();
	CaDBObject(LPCTSTR lpszItem, LPCTSTR lpszItemOwner, BOOL bSystem = FALSE);
	
	virtual ~CaDBObject(){}
	//
	// Manually manage the object version:
	// When you detect that you must manage the newer version (class members have been changed),
	// the derived class should override GetObjectVersion() and return
	// the value N same as IMPLEMENT_SERIAL (CaXyz, CaDBObject, N).
	virtual UINT GetObjectVersion(){return 1;}

	virtual void Serialize (CArchive& ar);
	virtual CString GetName() {return m_strItem;}
	virtual CString GetItem() {return m_strItem;}
	virtual CString GetOwner(){return m_strItemOwner;}
	BOOL IsSystem() {return m_bSystem;}

	void SetSerialize  (BOOL bMin) {;}
	BOOL IsSerializeMin() {return TRUE;}

	virtual void SetItem (LPCTSTR lpszItem) {m_strItem = lpszItem;}
	virtual void SetOwner(LPCTSTR lpszOwner){m_strItemOwner = lpszOwner;}
	void SetSystem(BOOL bSystem) {m_bSystem = bSystem;}
	BOOL Matched (CaDBObject* pObj, MatchObjectFlag nFlag=MATCHED_NAME);


	CaDBObject(const CaDBObject& c);
	CaDBObject operator = (const CaDBObject& c);
	void SetItem (LPCTSTR lpszItem, LPCTSTR lpszItemOwner, BOOL bSystem = FALSE);
	void SetObjectID (int nID){m_nObjectType = nID;}
	int  GetObjectID (){return m_nObjectType;}

	//
	// Last query objects: (if applicable)
	CTime GetLastQuery(){return m_tTimeStamp;}
	void  SetLastQuery(){m_tTimeStamp = CTime::GetCurrentTime();}
	void  ResetLastQuery(){m_tTimeStamp = -1;}

	//
	// Last query of property (detail)
	CTime GetLastDetail(){return m_tTimeStampDetail;}
	void  SetLastDetail(){m_tTimeStampDetail = CTime::GetCurrentTime();}
	void  ResetLastDetail(){m_tTimeStampDetail = -1;}

	void SetSerializeAll(BOOL bSet){m_bSerializeAllData = bSet;}
	BOOL IstSerializeAll(){return m_bSerializeAllData;}

	static CaDBObject* FindDBObject(LPCTSTR lpszName, CTypedPtrList < CObList, CaDBObject* >& l);
	static CaDBObject* FindDBObject(LPCTSTR lpszName, LPCTSTR lpszOwner, CTypedPtrList < CObList, CaDBObject* >& l);
protected:
	void Copy (const CaDBObject& c);

	//
	// Own properties: (minimal data to serialize)

	CString m_strItem;        // The name of object
	CString m_strItemOwner;   // The owner of object;
	BOOL    m_bSystem;        // TRUE if Object is belong to the System.
	int     m_nObjectType;    // Object ID
private:
	//
	// By default, the object only serialize the minimum of its properties.
	// Ex: A CaDatabase (derived from this class) does not serialize the list of
	//     Tables, Views, Procedures, ...
	BOOL m_bSerializeAllData;

	CTime   m_tTimeStamp;       // The time (the last queried object, local time)
	CTime   m_tTimeStampDetail; // The time (the last get detail of object, local time)
};


#endif
