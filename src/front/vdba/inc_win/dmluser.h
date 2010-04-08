/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmluser.h: interface for the CaUser class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : User object
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/

#if !defined (DMLUSER_HEADER)
#define DMLUSER_HEADER
#include "dmlbase.h"
#include "dmloburp.h"

//
// Object: User 
// ************************************************************************************************
class CaUser: public CaDBObject
{
	DECLARE_SERIAL (CaUser)
public:
	CaUser(): CaDBObject(){Init();}
	CaUser(LPCTSTR lpszName): CaDBObject(lpszName, _T("")){Init();}
	CaUser(const CaUser& c);
	CaUser operator = (const CaUser& c);

	virtual ~CaUser(){}
	virtual void Serialize (CArchive& ar);

	BOOL Matched (CaUser* pObj, MatchObjectFlag nFlag = MATCHED_NAME);

protected:
	void Copy (const CaUser& c);
	void Init();
};


//
// Object: UserDetail
// ************************************************************************************************
class CaUserDetail: public CaUser, public CaURPPrivileges
{
	DECLARE_SERIAL (CaUserDetail)
public:
	CaUserDetail():CaUser(), CaURPPrivileges(){Init();}
	CaUserDetail(LPCTSTR lpszName):CaUser(lpszName), CaURPPrivileges(){Init();}
	CaUserDetail(const CaUserDetail& c);
	CaUserDetail operator = (const CaUserDetail& c);

	virtual ~CaUserDetail(){}
	virtual void Serialize (CArchive& ar);

	BOOL Matched (CaUserDetail* pObj, MatchObjectFlag nFlag = MATCHED_NAME);
	void GenerateStatement (CString& strColumText);

	//
	// GET:
	CString GetDefaultGroup(){return m_strDefaultGroup;}
	CString GetDefaultProfile(){return m_strDefaultProfile;}
	CString GetExpireDate(){return m_strExpireDate;}
	BOOL IsExternalPassword(){return m_bExternalPassword;};
	//
	// SET:
	void SetDefaultGroup(LPCTSTR lpszText){m_strDefaultGroup = lpszText;}
	void SetDefaultProfile(LPCTSTR lpszText){m_strDefaultProfile = lpszText;}
	void SetExpireDate(LPCTSTR lpszText){m_strExpireDate = lpszText;}
	void SetExternalPassword(BOOL bSet){m_bExternalPassword = bSet;}

	void Init()
	{
		CaURPPrivileges::Init();
		m_strDefaultGroup = _T("");
		m_strExpireDate = _T("");
		m_strDefaultProfile = _T("");
		m_bExternalPassword = FALSE;
		SetObjectID(OBT_USER);
	}
	CaURPPrivileges* GetURPPrivileges(){return (CaURPPrivileges*)this;}
protected:
	void Copy (const CaUserDetail& c);

	CString m_strDefaultGroup;
	CString m_strDefaultProfile;
	CString m_strExpireDate;
	BOOL m_bExternalPassword;
};

#endif // DMLUSER_HEADER
