/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmlgrant.h: interface for the CaGrant class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : CaGrant object
**
** History:
**
** 24-Oct-2002 (uk$so01)
**    SIR #109220 created. Enhance the library.
**/


#if !defined (DMLGRANT_HEADER)
#define DMLGRANT_HEADER
#include "dmlbase.h"
#include "dmluser.h"
//
// Object: CaPrivilegeItem 
// ************************************************************************************************
class CaPrivilegeItem: public CaDBObject
{
	DECLARE_SERIAL (CaPrivilegeItem)
	CaPrivilegeItem();
public:
	CaPrivilegeItem(LPCTSTR lpszName, BOOL bPrivilege = TRUE);
	CaPrivilegeItem(LPCTSTR lpszName, BOOL bPrivilege, int nVal);
	CaPrivilegeItem(const CaPrivilegeItem& c);
	CaPrivilegeItem operator =(const CaPrivilegeItem& c);

	BOOL Matched (CaPrivilegeItem* pObj);
	virtual ~CaPrivilegeItem(){};
	virtual void Serialize (CArchive& ar);
	CString GetPrivilege(){return GetName();}
	BOOL IsPrivilege(){return m_bPrivilege;}
	int  GetLimit(){return m_nLimitValue;}

	static CaPrivilegeItem* FindPrivilege(CaPrivilegeItem* pRivilege, CTypedPtrList< CObList, CaPrivilegeItem* >& l);
	static CaPrivilegeItem* FindPrivilege(LPCTSTR lpszName, BOOL bPrivilege, CTypedPtrList< CObList, CaPrivilegeItem* >& l);
protected:
	void Copy(const CaPrivilegeItem& c);

	BOOL m_bPrivilege;
	int  m_nLimitValue;

};

//
// Object: CaGrantee 
// ************************************************************************************************
class CaGrantee: public CaUser
{
	DECLARE_SERIAL (CaGrantee)
public:
	CaGrantee(): CaUser(){SetObjectID(OBT_GRANTEE); SetIdentity(OBT_USER);}
	CaGrantee(LPCTSTR lpszName): CaUser(lpszName){SetObjectID(OBT_GRANTEE); SetIdentity(OBT_USER);}
	CaGrantee(const CaGrantee& c);
	CaGrantee operator =(const CaGrantee& c);

	BOOL Matched (CaGrantee* pObj);
	virtual ~CaGrantee();
	virtual void Serialize (CArchive& ar);
	CTypedPtrList < CObList, CaPrivilegeItem* >& GetListPrivilege(){return m_listPrivilege;}
	void SetIdentity(int nId){m_nRealIdentity = nId;}
	int  GetIdentity(){return m_nRealIdentity;}
	void SetPrivilege(LPCTSTR lpszPriv, BOOL bPrivilege = TRUE);
	void SetPrivilege2(LPCTSTR lpszPriv, int nVal);

protected:
	void Copy(const CaGrantee& c);

	CTypedPtrList < CObList, CaPrivilegeItem* > m_listPrivilege;
	int  m_nRealIdentity; // OBT_USER, OBT_GROUP, OBT_ROLE, (-1: public)

};

#endif // DMLGRANT_HEADER