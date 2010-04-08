/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmlrole.h: interface for the CaRole class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Role object
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/


#if !defined (DMLROLE_HEADER)
#define DMLROLE_HEADER
#include "dmlbase.h"
#include "dmloburp.h"

//
// Object: CaRole 
// ************************************************************************************************
class CaRole: public CaDBObject
{
	DECLARE_SERIAL (CaRole)
public:
	CaRole(): CaDBObject(){SetObjectID(OBT_ROLE);}
	CaRole(LPCTSTR lpszName): CaDBObject(lpszName, _T("")){SetObjectID(OBT_ROLE);}
	CaRole (const CaRole& c);
	CaRole operator = (const CaRole& c);

	virtual ~CaRole(){};
	virtual void Serialize (CArchive& ar);
	BOOL Matched (CaRole* pObj, MatchObjectFlag nFlag = MATCHED_NAME);

protected:
	void Copy (const CaRole& c);

};


//
// Object: CaRoleDetail 
// ************************************************************************************************
class CaRoleDetail: public CaRole, public CaURPPrivileges
{
	DECLARE_SERIAL (CaRoleDetail)
public:
	CaRoleDetail(): CaRole(), CaURPPrivileges(){Init();}
	CaRoleDetail(LPCTSTR lpszName): CaRole(lpszName), CaURPPrivileges(){Init();}
	CaRoleDetail (const CaRoleDetail& c);
	CaRoleDetail operator = (const CaRoleDetail& c);

	virtual ~CaRoleDetail(){};
	virtual void Serialize (CArchive& ar);
	BOOL Matched (CaRoleDetail* pObj, MatchObjectFlag nFlag = MATCHED_NAME);
	void Init()
	{
		CaURPPrivileges::Init();
		m_bExternalPassword = FALSE;
	}
	BOOL IsExternalPassword(){return m_bExternalPassword;};
	void SetExternalPassword(BOOL bSet){m_bExternalPassword = bSet;}

	//
	// The role object must not use these 3 funtions:
	BOOL IsDefaultPrivilege(UserRoleProfleFlags nFlag){ASSERT(FALSE); return FALSE;}
	void SetDefaultPrivilege(UserRoleProfleFlags nFlag){ASSERT(FALSE);}
	void DelDefaultPrivilege(UserRoleProfleFlags nFlag){ASSERT(FALSE);}

	CaURPPrivileges* GetURPPrivileges(){return (CaURPPrivileges*)this;}
protected:
	void Copy (const CaRoleDetail& c);

	BOOL m_bExternalPassword;

};


#endif // DMLROLE_HEADER